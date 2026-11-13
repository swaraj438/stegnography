#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

/* Read and validate decode args from argv */
Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)   //function to read and validate decode arguments
{
    if(argv[2] == NULL)   //checking for stego image name(encoded image)
    {
        printf("ERROR: Please provide stego image name\n");
        return e_failure;
    }
    
    if(strstr(argv[2], ".bmp") == NULL)    //checking for valid bmp file
    {
        printf("ERROR: Stego image should be .bmp file\n");
        return e_failure;
    }
    
    decInfo->stego_image_fname = argv[2];       //encoded stego image file name assignment
    decInfo->output_fname = argv[3];            // Output file name (It can be null since decoding is not yet done.)
    
    return e_success;
}

/* Open files for decoding */
Status open_decode_files(DecodeInfo *decInfo)    
{
    // Open stego image file in binary mode
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "rb");  //open the encoded file for decoding 
    if(decInfo->fptr_stego_image == NULL)               //if encoded file is not present or equal to null.
    {
        printf("ERROR: Unable to open stego image file\n");
        return e_failure;
    }
    
    // Set default output file name if not provided
    if(decInfo->output_fname == NULL)
    {
        decInfo->output_fname = "decoded_secret.txt";    // if output.txt is not provided  then create the default file.
    }
    
    // Open output file in write mode
    decInfo->fptr_output = fopen(decInfo->output_fname, "w");
    if(decInfo->fptr_output == NULL)
    {
        printf("ERROR: Unable to open output file\n");
        fclose(decInfo->fptr_stego_image);
        return e_failure;
    }
    
    return e_success;
}

/* Decode a byte from LSB of image bytes */
char decode_byte_from_lsb(char *image_buffer)
{
    char data = 0;                   
    for(int i = 0; i < 8; i++)
    {
        data = (data << 1) | (image_buffer[i] & 1);  //decoding the encoded by shifting left and OR ing with LSB
    }
    return data;
}

/* Decode magic string */
Status decode_magic_string(DecodeInfo *decInfo)
{
    char magic_string[MAX_MAGIC_STRING_LEN];            //character array to store the decoded magic string.
    
    for(int i = 0; i < strlen(MAGIC_STRING); i++)
    {
        char image_buffer[8];
        if(fread(image_buffer, 1, 8, decInfo->fptr_stego_image) != 8)   //reading 8 bytes from the stego image file
        {
            printf("ERROR: Unable to read image data\n");
            return e_failure;
        }
        magic_string[i] = decode_byte_from_lsb(image_buffer);
    }
    magic_string[strlen(MAGIC_STRING)] = '\0';                           //null termination for the decoded magic string.
    
    if(strcmp(magic_string, MAGIC_STRING) != 0)                         //comparing the decoded magic string with the original magic string.
    {
        printf("ERROR: Invalid magic string. Not a stego image\n");
        return e_failure;
    }
    else
    {
        return e_success;
    }
}

/* Decode file extension size */
Status decode_file_extn_size(DecodeInfo *decInfo)               //function to decode the file extension size
{
    char image_buffer[8];
    if(fread(image_buffer, 1, 8, decInfo->fptr_stego_image) != 8)    //reading 8 bytes from the stego image file
    {
        printf("ERROR: Unable to read extension size\n");
        return e_failure;
    }
    decInfo->extn_size = decode_byte_from_lsb(image_buffer);
    
    return e_success;
}


/* Decode file extension */
Status decode_file_extn(DecodeInfo *decInfo)
{
    char extn[5];  // Static array for ".txt" + null
    
    for(int i = 0; i < decInfo->extn_size; i++)
    {
        char image_buffer[8];
        if(fread(image_buffer, 1, 8, decInfo->fptr_stego_image) != 8)
        {
            printf("ERROR: Unable to read extension data\n");
            return e_failure;
        }
        extn[i] = decode_byte_from_lsb(image_buffer);
    }
    extn[decInfo->extn_size] = '\0';
    
    if(strcmp(extn, ".txt") != 0)
    {
        printf("ERROR: Invalid file extension\n");
        return e_failure;
    }
    
    return e_success;
}

/* Decode file size */
Status decode_file_size(DecodeInfo *decInfo)
{
    char image_buffer[32];                                          //buffer to hold 32 bytes for file size
    if(fread(image_buffer, 1, 32, decInfo->fptr_stego_image) != 32)
    {
        printf("ERROR: Unable to read file size data\n");
        return e_failure;
    }
    
    uint file_size = 0;
    for(int i = 0; i < 32; i++)
    {
        file_size = (file_size << 1) | (image_buffer[i] & 1);
    }
    
    decInfo->secret_file_size = file_size;                  //assigning the decoded file size to the structure member.
    return e_success;
}

/* Decode secret file data */
Status decode_secret_file_data(DecodeInfo *decInfo)        //function to decode the secret file data
{
    char ch;
    for(uint i = 0; i < decInfo->secret_file_size; i++)
    {
        char image_buffer[8];
        if(fread(image_buffer, 1, 8, decInfo->fptr_stego_image) != 8)   //reading 8 bytes from the stego image file
        {
            printf("ERROR: Unable to read secret data\n");
            return e_failure;
        }
        ch = decode_byte_from_lsb(image_buffer);                        //decoding a byte from the LSBs to character
        if(fwrite(&ch, 1, 1, decInfo->fptr_output) != 1)
        {
            printf("ERROR: Unable to write to output file\n");
            return e_failure;
        }
    }
    
    return e_success;
}

/* Do decoding */
Status do_decoding(DecodeInfo *decInfo)
{
    printf("INFO: ## Decoding Process Started ##\n");
    
    Status status = e_success; /* track overall status */

    // Get magic string from user
    if(get_magic_string() == e_failure)
    {
        printf("ERROR: Failed to get magic string\n");
        return e_failure;
    }
    printf("INFO: Magic string read successfully\n");

    // Open files
    if(open_decode_files(decInfo) == e_failure)
    {
        printf("ERROR: Opening files failed\n");
        return e_failure;
    }
    printf("INFO: Opened files successfully\n");
    
    // Skip BMP header
    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);  //moving the file pointer to 54th byte to skip the header.and validate the magic string
    
    // Decode magic string
    if(decode_magic_string(decInfo) == e_failure)
    {
        printf("ERROR: Magic string decoding failed\n");
        status = e_failure;
    }
    printf("INFO: Magic string decoded successfully\n");
    
    // Decode file extension size
    if(decode_file_extn_size(decInfo) == e_failure)
    {
        printf("ERROR: Extension size decoding failed\n");
        status = e_failure;
    }
    
    // Decode file extension
    if(decode_file_extn(decInfo) == e_failure)
    {
        printf("ERROR: Extension decoding failed\n");
        status = e_failure;
    }
    printf("INFO: File extension decoded successfully\n");
    
    // Decode file size
    if(decode_file_size(decInfo) == e_failure)
    {
        printf("ERROR: File size decoding failed\n");
        status = e_failure;
    }
    printf("INFO: File size: %u bytes\n", decInfo->secret_file_size);
    
    // Decode secret file data
    if(decode_secret_file_data(decInfo) == e_failure)
    {
        printf("ERROR: Secret data decoding failed\n");
        status = e_failure;
    }
    printf("INFO: Secret data decoded successfully\n");
    
    printf("INFO: ## Decoding Process Completed ##\n");
    return status;
    

}