/*
 * MPSexplore.c 
 * Parses the given MPSShow data file (.MPS) and displays the metadata, Can also be used
 * to extract images from the file. 
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
#include "pal-tools.h"
#include "bmp.h"

#define OUTEXT   ".BMP"   // default extension for the output file
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (200)

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fi = NULL;
    FILE *fo = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    int xtridx  = -1;
    info_t *slide_info = NULL;
    memstream_buf_t img = {0, 0, NULL};

    printf("MPSextract - MPSShow Image Extractor\n");

    if((argc < 2) || (argc > 4)) {
        printf("USAGE: %s [infile] <extract> <outfile>\n", filename(argv[0]));
        printf("[infile] is the name of the input MPS file to extract from\n");
        printf("<extract> is the optional numerical index of the image to extract\n");
        printf("if <extract> is omitted, a listing of assets will be printed.\n");
        printf("A value of 0 for <extract> will call all image to be extracted.\n");
        printf("<outfile> optinal name for the output file, ignored if <extract> is 0\n");
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
    if(argc) {
        sscanf(argv[0],"%u",&xtridx);
    }
    argv++; argc--; // consume the arg (extract index)

    // get the optional output filename, if extracting a single image
    if((argc) && (xtridx > 0)) {
        namelen = strlen(argv[0]);
        if(NULL == (fo_name = calloc(1, namelen+1))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, argv[0], namelen);
    }
    argv++; argc--; // consume the arg (output file)

    // open the input file
    printf("Opening MPS File: '%s'", fi_name);
    if(NULL == (fi = fopen(fi_name,"rb"))) {
        printf("Error: Unable to open input file\n");
        goto CLEANUP;
    }

    // determine size of the EXE file
    size_t fsz = filesize(fi);
    printf("\tFile Size: %zu\n", fsz);

    // read in the mpsshow information block
    int num_slides = -1;
    if(NULL == (slide_info = read_mps_show_info_header(fi, &num_slides))) {
        printf("Error reading MPSShow info block\n");
        goto CLEANUP;
    }
    printf("Number of slides: %d\n", num_slides);
    // unset extract index if out of range
    if(xtridx > num_slides) {
        printf("Extract index '%d' out of range\n", xtridx);
        xtridx = -1;
    }

    if(-1 == xtridx) { // extract index not set, list all slides
        for(int i = 0; i < num_slides; i++) {
            printf("%2d: %9.*s - %-25.*s ofs:%06x len:%-6d mode:%02xh [%08x]\n", i+1, 
                slide_info[i].name_len, slide_info[i].name, 
                slide_info[i].desc_len, slide_info[i].desc, 
                slide_info[i].img_offset, slide_info[i].img_len, 
                slide_info[i].mode, 
                slide_info[i].unknown32);
        }
        goto DONE; // nothing more to do
    }

    // allocate image buffer
    if(NULL == (img.data = calloc(IMAGE_HEIGHT, IMAGE_WIDTH))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    img.len = (IMAGE_HEIGHT * IMAGE_WIDTH);


    if(0 == xtridx) { // extract all images

        // create the output filename buffer
        free_s(fo_name); // if prefiously allocated, free it
        if(NULL == (fo_name = calloc(1, 16))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }

        for(int i = 0; i < num_slides; i++) {
            // create the output filename based on the name in the slide
            sprintf(fo_name, "%.*s%s", slide_info[i].name_len, slide_info[i].name, OUTEXT);
            printf("Saving: '%s'\n", fo_name);

            // reset the image buffer
            img.pos = 0;
            memset(img.data,0,img.len);

            // read in the image
            if(0 != read_mps_show_image(&img, fi, &slide_info[i])) {
                printf("Error: Unable to read image\n");
                goto CLEANUP;
            }

            // convert it for BMP output
            // convert from 6-bit/component (VGA) to 8-bit/component (BMP)
            pal6_to_pal8(slide_info[i].pal, slide_info[i].pal, 256);
            if(0 != save_bmp(fo_name, &img, IMAGE_WIDTH, IMAGE_HEIGHT, slide_info[i].pal)) {
                printf("Error: Unable to save BMP image\n");
                goto CLEANUP;
            }
        }
        goto DONE;
    }

    xtridx--; // adjust for 0 based indexing

    // create the output filename if not set
    if(NULL == fo_name) {
        if(NULL == (fo_name = calloc(1, 16))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        sprintf(fo_name, "%.*s%s", slide_info[xtridx].name_len, slide_info[xtridx].name, OUTEXT);
    }
    printf("Saving: '%s'\n", fo_name);

    // read in the image
    if(0 != read_mps_show_image(&img, fi, &slide_info[xtridx])) {
        printf("Error: Unable to read image\n");
        goto CLEANUP;
    }

    // convert it for BMP output
    // convert from 6-bit/component (VGA) to 8-bit/component (BMP)
    pal6_to_pal8(slide_info[xtridx].pal, slide_info[xtridx].pal, 256);
    if(0 != save_bmp(fo_name, &img, IMAGE_WIDTH, IMAGE_HEIGHT, slide_info[xtridx].pal)) {
        printf("Error: Unable to save BMP image\n");
        goto CLEANUP;
    }

DONE:
    rval = 0; // clean exit

CLEANUP:
    fclose_s(fi);
    fclose_s(fo);
    free_s(img.data);
    free_s(slide_info);
    free_s(fi_name);
    free_s(fo_name);
    return rval;
}
