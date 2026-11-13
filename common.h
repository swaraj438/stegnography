#ifndef COMMON_H
#define COMMON_H

#include "types.h"

#define MAX_MAGIC_STRING_LEN 10                     // Maximum length for the magic string
extern char MAGIC_STRING[MAX_MAGIC_STRING_LEN];                

/* Function declaration for getting magic string */
Status get_magic_string(void);

#endif
