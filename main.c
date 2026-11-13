/*
NAME : SWARAJ AS
DATE : 16-03-2026.
DESCRIPTION : C program to perform encoding and decoding of steganography using LSB technique.

Encoding Part:

Hides text files within BMP images
Preserves original image quality
Uses custom magic string for security
Supports variable length messages.

Decoding Part:

Extracts hidden messages from stego images
Validates magic string before decoding
Recovers original text content
Creates output text file
*/
#include <stdio.h>
#include "decode.h"
#include "encode.h"
#include "types.h"
#include<string.h>
/*Checking for the operations to be carried*/
OperationType check_operation_type(char *argv[])
{
    if(strcmp(argv[1],"-e")==0)
        {
            return e_encode ;
        }
    else if(strcmp(argv[1],"-d")==0)
        {
            return e_decode; 
        }
    else
        {
           return e_unsupported;
        }
}
int main(int argc ,char*argv[])
{
    if(argc < 3)                                // minimum arguments for both encoding and decoding operations
    {
        printf("Error: Insufficient arguments provided\n");
        return e_failure;
    }
    else                                         //checking for encoding or decoding operation
    {
        if(check_operation_type(argv) == e_encode)
            {
                printf("----------you have choosed encoding----------\n");
                EncodeInfo encInfo;                                             //structure declaration for encoding
                if(read_and_validate_encode_args(argv,&encInfo)==e_success)
                    {
                        printf("Read and validate is success\n");
                        if(do_encoding(&encInfo)==e_success)
                            {
                                printf("Encoding sucessfull\n");
                                return e_success;
                            }
                        else
                            {
                                printf("Error: Encoding failed\n");
                                return e_failure;
                            }
                    }
                else
                    {
                        printf("read and validaiate unsucessfull\n");
                        return e_failure;
                    }
            }
        else  if(check_operation_type(argv) == e_decode)                            //checking for decoding operation
            {
                printf("you have choosed decoding\n");
                DecodeInfo decInfo;
                if(read_and_validate_decode_args(argv, &decInfo) == e_success)
                    {
                        printf("Read and validate is success\n");
                        if(do_decoding(&decInfo) == e_success)
                            {
                                printf("Decoding successful\n");
                                return e_success;
                            }
                        else
                        {
                            printf("Error: Decoding failed\n");
                            return e_failure;
                        }
                    }
                else
                {
                    printf("Read and validate unsuccessful\n");
                    return e_failure;
                }
            }   
        else
            {
                printf("Error: Unsupported operation\n");
                return e_failure;
            }
        return e_success;
    }
}