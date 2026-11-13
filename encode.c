#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Define the magic string variable */
char MAGIC_STRING[MAX_MAGIC_STRING_LEN];

/* Function Definitions */

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;
    // Seek to 18th byte
    fseek(fptr_image, 18, SEEK_SET);

    // Read the width (4 bytes)
    fread(&width, sizeof(int), 1, fptr_image);

    // Read the height (4 bytes)
    fread(&height, sizeof(int), 1, fptr_image);

    // Return image capacity
    return width * height * 3;
}

/* Get File size
 * Input: File pointer
 * Output: Size of file in bytes
 */
uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);
    return ftell(fptr);
}

/* 
 * Read and validate command line arguments
 * Input: argv and encInfo
 * Output: e_success or e_failure
 */
Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if(strstr(argv[2], ".bmp") != NULL)    //checking for valid bmp file extension.
    {
        encInfo->src_image_fname = argv[2];      //source image file name assignment
        printf("INFO: Source image file is %s\n", encInfo->src_image_fname);
    }
    else
    {
        printf("ERROR: Source image should be .bmp file\n");
        return e_failure;
    }

    if(strstr(argv[3], ".txt") != NULL)             //checking for valid secret file extension.
    {
        encInfo->secret_fname = argv[3];
        printf("INFO: Secret file is %s\n", encInfo->secret_fname);
    }
    else
    {
        printf("ERROR: Secret file should be .txt file\n");
        return e_failure;
    }

    if(argv[4] != NULL)                 //if name is provided to store encoded file assigning it
    {
        encInfo->stego_image_fname = argv[4];
    }
    else
    {
        encInfo->stego_image_fname = "stego.bmp";  //if name is not provided assigning default name.
    }
    printf("INFO: Output file name is %s\n", encInfo->stego_image_fname);

    return e_success;
}

/* Open files required for encoding */
Status open_files(EncodeInfo *encInfo)
{
    // Open source image file
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");    //opening the source image file in read mode
    if(encInfo->fptr_src_image == NULL)
    {
        printf("ERROR: Unable to open source image file\n");
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->src_image_fname);              

    // Open secret file
    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if(encInfo->fptr_secret == NULL)
    {
        printf("ERROR: Unable to open secret file\n");
        fclose(encInfo->fptr_src_image);
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->secret_fname);

    // Open stego image file
    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if(encInfo->fptr_stego_image == NULL)
    {
        printf("ERROR: Unable to open stego image file\n");
        fclose(encInfo->fptr_src_image);
        fclose(encInfo->fptr_secret);
        return e_failure;
    }
    printf("INFO: Opened %s\n", encInfo->stego_image_fname);

    return e_success;
}

/* Copy BMP Header */
Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)  //function to copy the bmp header from source image to stego image
{
    unsigned char header[54];
    fseek(fptr_src_image, 0, SEEK_SET);
    
    // Read header
    if(fread(header, sizeof(unsigned char), 54, fptr_src_image) != 54)   //reading 54 bytes of header from source image file
    {
        printf("ERROR: Failed to read header\n");
        return e_failure;
    }

    // Write header
    if(fwrite(header, sizeof(unsigned char), 54, fptr_dest_image) != 54)  //writing 54 bytes of header to stego image file
    {
        printf("ERROR: Failed to write header\n");
        return e_failure;
    }

    return e_success;
}

/* Encode a byte into LSB of image bytes */
Status encode_byte_to_lsb(char data, char *image_buffer)     //function to encode a byte into LSB of 8 bytes of image buffer.
{
    for(int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);   //encoding by clearing LSB and OR ing with data bit
    }
    return e_success;
}

/* Encode magic string */
Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)  //function to encode the magic string into the image buffer
{
    for(int i = 0; i < strlen(magic_string); i++)    //iterating through each character of the magic string 
    {
        char image_buffer[8];
        fread(image_buffer, 1, 8, encInfo->fptr_src_image);   //reading 8 bytes from the source image file
        
        if(encode_byte_to_lsb(magic_string[i], image_buffer) == e_failure)   //encoding each character into LSBs of 8 bytes
        {
            return e_failure;
        }
        
        fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image);   //writing the modified 8 bytes to the stego image file(encoded file)
    }
    return e_success;
}

/* Encode secret file extension size */
Status encode_secret_file_extn_size(const char *file_extn, EncodeInfo *encInfo)
{
    char len = strlen(file_extn);
    char image_buffer[8];                     //buffer to hold 8 bytes for encoding extension size
    
    fread(image_buffer, 1, 8, encInfo->fptr_src_image);     //reading 8 bytes from the source image file
    if(encode_byte_to_lsb(len, image_buffer) == e_failure)
    {
        return e_failure;
    }
    fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image);    //writing the modified 8 bytes to the stego image file(encoded file)
    
    return e_success;
}

/* Encode secret file extension */
Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    for(int i = 0; i < strlen(file_extn); i++)
    {
        char image_buffer[8];                                       //buffer to hold 8 bytes for encoding each character of extension
        fread(image_buffer, 1, 8, encInfo->fptr_src_image);
        
        if(encode_byte_to_lsb(file_extn[i], image_buffer) == e_failure)   //encoding each character into LSBs of 8 bytes
        {
            return e_failure;
        }
        
        fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image);            //writing the modified 8 bytes to the stego image file(encoded file)
    }
    return e_success;
}

/* Encode secret file size */
Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    char image_buffer[32];
    fread(image_buffer, 1, 32, encInfo->fptr_src_image);
    
    for(int i = 0; i < 32; i++)
    {
        image_buffer[i] = (image_buffer[i] & ~1) | ((file_size >> (31 - i)) & 1);
    }
    
    fwrite(image_buffer, 1, 32, encInfo->fptr_stego_image);
    return e_success;
}

/* Encode secret file data */
Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    fseek(encInfo->fptr_secret, 0, SEEK_SET);
    
    while(fread(&ch, 1, 1, encInfo->fptr_secret) > 0)
    {
        char image_buffer[8];
        fread(image_buffer, 1, 8, encInfo->fptr_src_image);
        
        if(encode_byte_to_lsb(ch, image_buffer) == e_failure)
        {
            return e_failure;
        }
        
        fwrite(image_buffer, 1, 8, encInfo->fptr_stego_image);
    }
    return e_success;
}

/* Copy remaining image bytes */
Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;
    while(fread(&ch, 1, 1, fptr_src) > 0)
    {
        if(fwrite(&ch, 1, 1, fptr_dest) != 1)     //writing remaining bytes to the stego image file
        {
            return e_failure;
        }
    }
    return e_success;
}

/* Get magic string from user */
Status get_magic_string(void)
{
    printf("Enter magic string (max %d chars): ", MAX_MAGIC_STRING_LEN - 1);
    scanf("%9s", MAGIC_STRING);
    if (strlen(MAGIC_STRING) < 2)
    {
        printf("ERROR: Magic string must be at least 2 characters\n");
        return e_failure;
    }
    return e_success;
}

/* Do encoding */
Status do_encoding(EncodeInfo *encInfo)
{
    // Get magic string from user before starting encoding
    if(get_magic_string() == e_failure)
    {
        printf("ERROR: Failed to get magic string\n");
        return e_failure;
    }
    printf("INFO: Magic string saved successfully\n");

    // Open required files
    if(open_files(encInfo) == e_failure)
    {
        printf("ERROR: Opening files failed\n");
        return e_failure;
    }

    printf("INFO: Opened all files successfully\n");

    // Copy BMP header
    if(copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy header\n");
        return e_failure;
    }

    printf("INFO: Done copying header\n");

    // Encode Magic String
    if(encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode magic string\n");
        return e_failure;
    }

    printf("INFO: Done encoding magic string\n");

    // Encode secret file extension size
    if(encode_secret_file_extn_size(".txt", encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode extension size\n");
        return e_failure;
    }

    // Encode secret file extension
    if(encode_secret_file_extn(".txt", encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode extension\n");
        return e_failure;
    }

    // Get and encode secret file size
    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);
    if(encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode file size\n");
        return e_failure;
    }

    printf("INFO: Done encoding file size\n");

    // Encode secret file data
    if(encode_secret_file_data(encInfo) == e_failure)
    {
        printf("ERROR: Failed to encode secret data\n");
        return e_failure;
    }

    printf("INFO: Done encoding secret data\n");

    // Copy remaining image bytes
    if(copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        printf("ERROR: Failed to copy remaining data\n");
        return e_failure;
    }

    // Close all files
    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    printf("INFO: Encoding completed successfully\n");
    return e_success;
}
