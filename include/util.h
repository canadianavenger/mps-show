/*
 * util.h 
 * general purpose convenience utility functions and macros
 * 
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdint.h>
#include <stdio.h>

#ifndef CA_UTILS
#define CA_UTILS

/// @brief determins the size of the file
/// @param f handle to an open file
/// @return returns the size of the file
size_t filesize(FILE *f);

/// @brief removes the extension from a filename
/// @param fn sting pointer to the filename
void drop_extension(char *fn);

/// @brief Returns the filename portion of a path
/// @param path filepath string
/// @return a pointer to the filename portion of the path string
char *filename(char *path);

// convenience "safe" resource release functons
#define fclose_s(A) if(A) fclose(A); A=NULL
#define free_s(A) if(A) free(A); A=NULL

#endif
