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

// MS-DOS EXE File Header
typedef struct {
    char signature[2];      // "MZ"
    uint16_t len_final;     // number of bytes used in the final block of the file
    uint16_t num_blocks;    // number of 512 byte blocks, incl final block and "MZ"
    uint16_t num_reloc;     // number of 4 byte relocation table entries
    uint16_t pg_header;     // number of 16 byte paragraphs in the file header, incl "MZ"
    uint16_t pg_mem_extra;  // min number of 16 byte paragraphs extra memory that must be allocated to run
    uint16_t pg_mem_max;    // max number of 16 byte paragraphs extra memory
    uint16_t seg_ss;        // stack segment
    uint16_t seg_sp;        // initial value of SP [SS:SP]
    uint16_t checksum;      // 0 if unused, otherwas value set so sum=0
    uint16_t reg_ip;        // initial IP value [CS:IP] (code start within the file, usually 0x100)
    uint16_t reg_cs;        // offset of the CS segment
    uint16_t off_reloc;     // offset to relocation table relative to start of exe
    uint16_t overlay_index; // overlay number, 0 for main exe.
} dos_exe_hdr_t;

// PORTABILITY note, this pragma is GCC specific
// and may need to be changed for other compilers
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
    uint32_t img_offset;        // offset if image in the file (relative to full EXE)
    uint16_t mode;              // video mode? or length of unknown data after the palette? 
    uint16_t img_len;              // 0x13 = 19, and there are 19 bytes after the palette
    uint32_t unknown32;         // looks to be uninitialized bytes
    pal_entry_t pal[256];
    uint8_t unknown[835 - 816]; // unknown data, but likely show flow and control data
} info_t;
#pragma pack(pop)

#define MPSRECSZ (835)    // size of MPSShow info record
#define BUFSZ    (16384)  // size of buffer to use when copying over data
#define OUTEXT   ".MPS"   // default extension for the output file

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
    if(strncmp("MZ", hdr.signature, 2)) {
        printf("Invalid EXE header\n");
        goto CLEANUP;
    }

    // do a basic size check
    size_t exe_sz = ((size_t)hdr.num_blocks - 1) * 512 + hdr.len_final;
    if(fsz == exe_sz) {
        printf("EXE does not contain appended data\n");
        goto CLEANUP;
    }
    printf("Reported EXE size: %zu bytes\n", exe_sz);

    // go to end of true EXE file
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
    // allocate our copy buffer
    if(NULL == (buf = malloc(BUFSZ))) {
        printf("Unable to allocate buffer\n");
        goto CLEANUP;
    }
    // open the output file
    if(NULL == (fo = fopen(fo_name,"wb"))) {
        printf("Error: Unable to open output file\n");
        goto CLEANUP;
    }

    int num_slides = fgetc(fi); // we already read in the number when scanning
    if((EOF == num_slides) || (0 == num_slides)) {
        printf("File contains no data\n");
        goto CLEANUP;
    }
    printf("Number of slides: %d\n", num_slides);

    if(NULL == (slide_info = (info_t *)calloc(num_slides,sizeof(info_t)))) {
        printf("Error: Unable to allocate info buffer\n");
        goto CLEANUP;
    }

    nr = fread(slide_info, sizeof(info_t), num_slides, fi);
    if(nr != num_slides) {
        printf("Error: Unable to read info block\n");
        goto CLEANUP;
    }

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
