/*
 * MPSextract.c 
 * Scans the given EXE file for additional data appended to the end (MPSShow data), 
 * and extracts it to a file. 
 * 
 * Intended to be used with MicroProse MPSShow based demo files like 'F15STORM.EXE',
 * MicroProse pads the EXE file with a large block of null bytes before the data, this
 * null data is skipped. Once the first non-zero byte is located, the contents from
 * that point to EOF are copied to the output file.
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
#include "dos-exe.h"

#define BUFSZ    (16384)  // size of buffer to use when copying over data
#define OUTEXT   ".MPS"   // default extension for the output file

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fi = NULL;
    FILE *fo = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    uint8_t *buf = NULL;
    info_t *slide_info = NULL;

    printf("MPSextract - MPSShow Data Extractor\n");

    if((argc < 2) || (argc > 3)) {
        printf("USAGE: %s [infile] <outfile>\n", filename(argv[0]));
        printf("[infile] is the name of the input EXE file to extract from\n");
        printf("<outfile> is optional and the name of the output file\n");
        printf("if omitted, the output will be named the same as infile, except with a '%s' extension\n", OUTEXT);
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

    if(argc) { // output file name was provided
        int namelen = strlen(argv[0]);
        if(NULL == (fo_name = calloc(1, namelen+1))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, argv[0], namelen);
        argv++; argc--; // consume the arg (input file)
    } else { // no name was provded, so make one
        if(NULL == (fo_name = calloc(1, namelen+5))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        strncpy(fo_name, fi_name, namelen);
        drop_extension(fo_name); // remove exisiting extension
        strncat(fo_name, OUTEXT, namelen+4); // add mps extension
    }

    // open the input file
    printf("Opening EXE File: '%s'", fi_name);
    if(NULL == (fi = fopen(fi_name,"rb"))) {
        printf("Error: Unable to open input file\n");
        goto CLEANUP;
    }

    // determine size of the EXE file
    size_t fsz = filesize(fi);
    printf("\tFile Size: %zu\n", fsz);

    dos_exe_hdr_t hdr;
    int nr = fread(&hdr, sizeof(dos_exe_hdr_t), 1, fi);
    if(1 != nr) {
        printf("Error reading EXE header\n");
        goto CLEANUP;
    }

    // check for the EXE signature
    if(strncmp(EXE_SIG, hdr.signature, sizeof(hdr.signature))) {
        printf("Invalid EXE header\n");
        goto CLEANUP;
    }

    // do a basic size check
    size_t exe_sz = ((size_t)hdr.num_blocks - 1) * EXE_BLOCK_SZ + hdr.len_final;
    if(fsz == exe_sz) {
        printf("EXE does not contain appended data\n");
        goto CLEANUP;
    }
    printf("Reported EXE size: %zu bytes\n", exe_sz);

    // go to end of reported EXE file
    fseek(fi, exe_sz, SEEK_SET);

    // now we can scan for the beginning of non-null data, 
    // which should be our MPSShow data
    if(feof(fi)) {
        printf("Unexpected end of file\n");
        goto CLEANUP;
    }

    printf("Scanning for start of data...");
    int rec_count = 0;

    uint32_t count = 0;
    while((rec_count == 0) && (!feof(fi))) {
        rec_count = fgetc(fi);
        count++;
        // print a "heartbeat" every 1K bytes
        if((count&0x03ff) == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    printf(".done\n");
    if(rec_count==EOF) {
        printf("Reached end of file with no data\n");
        goto CLEANUP;
    }

    size_t mps_pos = ftell(fi) - 1;
    printf("Start of data at: 0x%06zx\n", mps_pos);
    fseek(fi, mps_pos, SEEK_SET); // rewind one byte

    // do a quick sanity check to make sure we have at least enough room
    // for all the reported records.
    size_t mps_sz = fsz - mps_pos;
    size_t mps_info_sz = MPSRECSZ * rec_count;
    if(mps_sz <= mps_info_sz) { // not enough left in the file to be valid
        printf("Remaining data too short to be MPSShow data\n");
        goto CLEANUP;
    }

    printf("Extracting data to: '%s'\tData Size: %zu\n", fo_name, mps_sz);
    // open the output file
    if(NULL == (fo = fopen(fo_name,"wb"))) {
        printf("Error: Unable to open output file\n");
        goto CLEANUP;
    }

    int num_slides = -1;
    if(NULL == (slide_info = read_mps_show_info_header(fi, &num_slides))) {
        printf("Error reading MPSShow info block\n");
        goto CLEANUP;
    }
    printf("Number of slides: %d\n", num_slides);

    // correct the slide offsets from being EXE centric
    // to being relative to the new MPS file
    for(int i = 0; i < num_slides; i++) {
        slide_info[i].img_offset -= mps_pos;
    }

    // write out the info block
    fputc(num_slides, fo);
    int nw = fwrite(slide_info, sizeof(info_t), num_slides, fo);
    if(num_slides != nw) {
        printf("Error writing output\n");
        goto CLEANUP;
    }

    // copy the remaining data, print a heartbeat once every BUFSZ block
    // allocate our copy buffer
    if(NULL == (buf = malloc(BUFSZ))) {
        printf("Unable to allocate buffer\n");
        goto CLEANUP;
    }
    printf("Copying.");
    do {
        nr = fread(buf, 1, BUFSZ, fi);
        printf(".");
        int nw = fwrite(buf, nr, 1, fo);
        if(1 != nw) {
            printf("Error writing output\n");
            goto CLEANUP;
        }
    } while(BUFSZ == nr);
    printf(".done\n");

    rval = 0; // clean exit

CLEANUP:
    fclose_s(fi);
    fclose_s(fo);
    free_s(buf);
    free_s(fi_name);
    free_s(fo_name);
    return rval;
}


