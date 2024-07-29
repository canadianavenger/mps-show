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
#include "util.h"
#include "mps-show.h"

#define OUTEXT   ".PAL"   // default extension for the output file

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

    // read in the mpsshow information block
    int num_slides = -1;
    if(NULL == (slide_info = read_mps_show_info_header(fi, &num_slides))) {
        printf("Error reading MPSShow info block\n");
        goto CLEANUP;
    }

    // make sure the requested extraction index is valid
    if(xtridx > num_slides) {
        printf("ERROR: Extract index '%d' out of range\n", xtridx);
        goto CLEANUP;
    }

    xtridx--; // adjust for 0 based indexing

    // create the output filename, if one wasn't provided
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

    // save out the palette
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
