/*
 * PALextract.c 
 * Parses the given MPSShow data file and extracts a selected palette to a file
 * 
 * To obtain the .MPS file you can use 'MPSextract' on a MPSShow slideshow demo exe file
 *
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define OUTEXT   ".PAL"   // default extension for the output file

// portability note, this is specific to GCC to pack the structurew without padding
#pragma pack(push,1)
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} pal_entry_t;

// needs to be 835 bytes
typedef struct {
    uint8_t name_len;
    char    name[9];
    uint8_t desc_len;
    char    desc[25];
    uint32_t img_offset;        // offset if image in the file
    uint16_t mode;              // video mode? or length of unknown data after the palette? 
    uint16_t img_len;              // 0x13 = 19, and there are 19 bytes after the palette
    uint32_t unknown32;         // looks to be uninitialized bytes
    pal_entry_t pal[256];
    uint8_t unknown[835 - 816]; // unknown data, but likely show flow and control data
} info_t;
#pragma pack(pop)

size_t filesize(FILE *f);
void drop_extension(char *fn);
char *filename(char *path);
#define fclose_s(A) if(A) fclose(A); A=NULL
#define free_s(A) if(A) free(A); A=NULL

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fi = NULL;
    FILE *fo = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    int xtridx  = -1;
    info_t *slide_info = NULL;

    printf("MPSextract - MPSShow Slide Palette Extractor\n");

    if((argc < 3) || (argc > 4)) {
        printf("USAGE: %s [infile] [extract] <outfile>\n", filename(argv[0]));
        printf("[infile] is the name of the input MPS file to extract from\n");
        printf("[extract] is the index of the slide to extract the palette from\n");
        printf("<outfile> is the optional output filename, otherwise slide name will be used\n");
        return -1;
    }
    argv++; argc--; // consume the first arg (program name)

    // get the file names from the command line
    int namelen = strlen(argv[0]);
    if(NULL == (fi_name = calloc(1, namelen+1))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    strncpy(fi_name, argv[0], namelen);
    argv++; argc--; // consume the arg (input file)

    // get the index of the image to extract, if given
    sscanf(argv[0],"%u",&xtridx);
    argv++; argc--; // consume the arg (extract index)
    if(0 >= xtridx) {
        printf("ERROR: Invalid extraction index\n");
        goto CLEANUP;
    }

    if(argc) { // output file name was provided
        namelen = strlen(argv[0]);
        if(NULL == (fo_name = calloc(1, namelen+1))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, argv[0], namelen);
    }

    // open the input file
    printf("Opening MPS File: '%s'", fi_name);
    if(NULL == (fi = fopen(fi_name,"rb"))) {
        printf("Error: Unable to open input file\n");
        goto CLEANUP;
    }
    // determine size of the MPS file
    size_t fsz = filesize(fi);
    printf("\tFile Size: %zu\n", fsz);

    int num_slides = fgetc(fi); // we already read in the number when scanning
    if((EOF == num_slides) || (0 == num_slides)) {
        printf("File contains no data\n");
        goto CLEANUP;
    }
    printf("Number of slides: %d\n", num_slides);
    if(xtridx > num_slides) {
        printf("ERROR: Extract index '%d' out of range\n", xtridx);
        goto CLEANUP;
    }

    if(NULL == (slide_info = (info_t *)calloc(num_slides,sizeof(info_t)))) {
        printf("Error: Unable to allocate info buffer\n");
        goto CLEANUP;
    }

    int nr = fread(slide_info, sizeof(info_t), num_slides, fi);
    if(nr != num_slides) {
        printf("Error: Unable to read info block\n");
        goto CLEANUP;
    }

    xtridx--; // adjust for 0 based indexing
    if(NULL == fo_name) {
        if(NULL == (fo_name = calloc(1, 16))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        sprintf(fo_name, "%.*s%s", slide_info[xtridx].name_len, slide_info[xtridx].name, OUTEXT);
    }
    printf("Saving: '%s'\n", fo_name);

    // try to open/create output file
    if(NULL == (fo = fopen(fo_name, "wb"))) {
        printf("Unable to create output file\n");
        goto CLEANUP;
    }

    int nw = fwrite(slide_info[xtridx].pal, sizeof(pal_entry_t), 256, fo);

    rval = 0; // clean exit

CLEANUP:
    fclose_s(fi);
    fclose_s(fo);
    free_s(slide_info);
    free_s(fi_name);
    free_s(fo_name);
    return rval;
}

/// @brief determins the size of the file
/// @param f handle to an open file
/// @return returns the size of the file
size_t filesize(FILE *f) {
    size_t szll, cp;
    cp = ftell(f);           // save current position
    fseek(f, 0, SEEK_END);   // find the end
    szll = ftell(f);         // get positon of the end
    fseek(f, cp, SEEK_SET);  // restore the file position
    return szll;             // return position of the end as size
}

/// @brief removes the extension from a filename
/// @param fn sting pointer to the filename
void drop_extension(char *fn) {
    char *extension = strrchr(fn, '.');
    if(NULL != extension) *extension = 0; // strip out the existing extension
}

/// @brief Returns the filename portion of a path
/// @param path filepath string
/// @return a pointer to the filename portion of the path string
char *filename(char *path) {
	int i;

	if(path == NULL || path[0] == '\0')
		return "";
	for(i = strlen(path) - 1; i >= 0 && path[i] != '/'; i--);
	if(i == -1)
		return "";
	return &path[i+1];
}
