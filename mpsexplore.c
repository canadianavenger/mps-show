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
#include "bmp.h"

#define OUTEXT   ".BMP"   // default extension for the output file
#define IMAGE_WIDTH (320)
#define IMAGE_HEIGHT (200)

typedef struct {
    size_t      len;         // length of buffer in bytes
    size_t      pos;         // current byte position in buffer
    uint8_t     *data;       // pointer to bytes in memory
} memstream_buf_t;

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
int save_bmp(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal);
int rle_decompress(memstream_buf_t *dst, memstream_buf_t *src);

int main(int argc, char *argv[]) {
    int rval = -1;
    FILE *fi = NULL;
    FILE *fo = NULL;
    char *fi_name = NULL;
    char *fo_name = NULL;
    int xtridx  = -1;
    info_t *slide_info = NULL;
    memstream_buf_t src = {0, 0, NULL};
    memstream_buf_t dst = {0, 0, NULL};

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

    int num_slides = fgetc(fi); // we already read in the number when scanning
    if((EOF == num_slides) || (0 == num_slides)) {
        printf("File contains no data\n");
        goto CLEANUP;
    }
    printf("Number of slides: %d\n", num_slides);
    // unset extract index if out of range
    if(xtridx > num_slides) {
        printf("Extract index '%d' out of range\n", xtridx);
        xtridx = -1;
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

    if(-1 == xtridx) { // extract index not set, list all slides
        for(int i = 0; i < num_slides; i++) {
            printf("%2d: %9.*s - %-25.*s ofs:%06x len:%-6d mode:%02xh [%08x]\n", i+1, 
                slide_info[i].name_len, slide_info[i].name, 
                slide_info[i].desc_len, slide_info[i].desc, 
                slide_info[i].img_offset, slide_info[i].img_len, 
                slide_info[i].mode, 
                slide_info[i].unknown32);
        }
        goto DONE;
    }

    // allocate image buffer
    if(NULL == (dst.data = calloc(IMAGE_HEIGHT, IMAGE_WIDTH))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    dst.len = (IMAGE_HEIGHT * IMAGE_WIDTH);

    if(0 == xtridx) { // extract all images
        free_s(fo_name);
        if(NULL == (fo_name = calloc(1, 16))) {
            printf("Unable to allocate memory\n");
            goto CLEANUP;
        }
        for(int i = 0; i < num_slides; i++) {
            sprintf(fo_name, "%.*s%s", slide_info[i].name_len, slide_info[i].name, OUTEXT);
            printf("Saving: '%s'\n", fo_name);
            dst.pos = 0;
            memset(dst.data,0,dst.len);
            if(NULL == (src.data = calloc(1, slide_info[i].img_len))) {
                printf("Unable to allocate memory\n");
                goto CLEANUP;
            }
            src.len = slide_info[i].img_len;
            src.pos = 0;

            fseek(fi, slide_info[i].img_offset, SEEK_SET);
            nr = fread(src.data, src.len, 1, fi);
            if(1 != nr) {
                printf("Error: Unable to read image block\n");
                goto CLEANUP;
            }
            if(0 != rle_decompress(&dst, &src)) {
                printf("Error: Decompression error\n");
                goto CLEANUP;
            }
            if(0 != save_bmp(fo_name, &dst, IMAGE_WIDTH, IMAGE_HEIGHT, slide_info[i].pal)) {
                printf("Error: Unable to save BMP image\n");
                goto CLEANUP;
            }
            free_s(src.data);
        }
        goto DONE;
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
    if(NULL == (src.data = calloc(1, slide_info[xtridx].img_len))) {
        printf("Unable to allocate memory\n");
        goto CLEANUP;
    }
    src.len = slide_info[xtridx].img_len;
    src.pos = 0;

    fseek(fi, slide_info[xtridx].img_offset, SEEK_SET);

    nr = fread(src.data, src.len, 1, fi);
    if(1 != nr) {
        printf("Error: Unable to read image block\n");
        goto CLEANUP;
    }

    if(0 != rle_decompress(&dst, &src)) {
        printf("Error: Decompression error\n");
        goto CLEANUP;
    }
    if(0 != save_bmp(fo_name, &dst, IMAGE_WIDTH, IMAGE_HEIGHT, slide_info[xtridx].pal)) {
        printf("Error: Unable to save BMP image\n");
        goto CLEANUP;
    }

DONE:
    rval = 0; // clean exit

CLEANUP:
    fclose_s(fi);
    fclose_s(fo);
    free_s(src.data);
    free_s(dst.data);
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

// allocate a header buffer large enough for all 3 parts, plus 16 bit padding at the start to 
// maintian 32 bit alignment after the 16 bit signature.
#define HDRBUFSZ (sizeof(bmp_signature_t) + sizeof(bmp_header_t))

/// @brief saves the image pointed to by src as a BMP, assumes 256 colour 1 byte per pixel image data
/// @param fn name of the file to create and write to
/// @param src memstream buffer pointer to the source image data
/// @param width  width of the image in pixels
/// @param height height of the image in pixels or lines
/// @param pal pointer to 256 entry RGB palette
/// @return 0 on success, otherwise an error code
int save_bmp(const char *fn, memstream_buf_t *src, uint16_t width, uint16_t height, pal_entry_t *xpal) {
    int rval = 0;
    FILE *fp = NULL;
    uint8_t *buf = NULL; // line buffer, also holds header info

    // do some basic error checking on the inputs
    if((NULL == fn) || (NULL == src) || (NULL == src->data)) {
        rval = -1;  // NULL pointer error
        goto bmp_cleanup;
    }

    // try to open/create output file
    if(NULL == (fp = fopen(fn,"wb"))) {
        rval = -2;  // can't open/create output file
        goto bmp_cleanup;
    }

    // stride is the bytes per line in the BMP file, which are padded
    // out to 32 bit boundaries
    uint32_t stride = ((width + 3) & (~0x0003)); 
    uint32_t bmp_img_sz = (stride) * height;

    // allocate a buffer to hold the header and a single scanline of data
    // this could be optimized if necessary to only allocate the larger of
    // the line buffer, or the header + padding as they are used at mutually
    // exclusive times
    if(NULL == (buf = calloc(1, HDRBUFSZ + stride + 2))) {
        rval = -3;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // signature starts after padding to maintain 32bit alignment for the rest of the header
    bmp_signature_t *sig = (bmp_signature_t *)&buf[stride + 2];

    // bmp header starts after signature
    bmp_header_t *bmp = (bmp_header_t *)&buf[stride + 2 + sizeof(bmp_signature_t)];

    // setup the signature and DIB header fields
    *sig = BMPFILESIG;
    size_t palsz = sizeof(bmp_palette_entry_t) * 256;
    bmp->dib.image_offset = HDRBUFSZ + palsz;
    bmp->dib.file_size = bmp->dib.image_offset + bmp_img_sz;

    // setup the bmi header fields
    bmp->bmi.header_size = sizeof(bmi_header_t);
    bmp->bmi.image_width = width;
    bmp->bmi.image_height = height;
    bmp->bmi.num_planes = 1;           // always 1
    bmp->bmi.bits_per_pixel = 8;       // 256 colour image
    bmp->bmi.compression = 0;          // uncompressed
    bmp->bmi.bitmap_size = bmp_img_sz;
    bmp->bmi.horiz_res = BMP96DPI;
    bmp->bmi.vert_res = BMP96DPI;
    bmp->bmi.num_colors = 256;         // palette has 256 colours
    bmp->bmi.important_colors = 0;     // all colours are important

    // write out the header
    int nr = fwrite(sig, HDRBUFSZ, 1, fp);
    if(1 != nr) {
        rval = -4;  // unable to write file
        goto bmp_cleanup;
    }

    bmp_palette_entry_t *pal = calloc(256, sizeof(bmp_palette_entry_t));
    if(NULL == pal) {
        rval = -3;  // unable to allocate mem
        goto bmp_cleanup;
    }

    // copy the external RGB palette to the BMP BGRA palette
    // also convert the 6 bit VGA DAC value to the 8 bit value BMP expects
    for(int i = 0; i < 256; i++) {
        uint16_t comp = xpal[i].r;
        comp *= 255;
        comp /= 63;
        pal[i].r = comp;

        comp = xpal[i].g;
        comp *= 255;
        comp /= 63;
        pal[i].g = comp;

        comp = xpal[i].b;
        comp *= 255;
        comp /= 63;
        pal[i].b = comp;
    }

    // write out the palette
    nr = fwrite(pal, palsz, 1, fp);
    if(1 != nr) {
        rval = -4;  // can't write file
        goto bmp_cleanup;
    }
    // we can free the palette now as we don't need it anymore
    free(pal);

    // now we need to output the image scanlines. For maximum
    // compatibility we do so in the natural order for BMP
    // which is from bottom to top. 
    // start by pointing to start of last line of data
    uint8_t *px = &src->data[src->len - width];
    // loop through the lines
    for(int y = 0; y < height; y++) {
        memset(buf, 0, stride); // zero out the line in the output buffer
        // loop through all the pixels for a line
        for(int x = 0; x < width; x++) {
            buf[x] = *px++;
        }
        nr = fwrite(buf, stride, 1, fp); // write out the line
        if(1 != nr) {
            rval = -4;  // unable to write file
            goto bmp_cleanup;
        }
        px -= (width * 2); // move back to start of previous line
    }

bmp_cleanup:
    fclose_s(fp);
    free_s(buf);
    return rval;
}

int rle_decompress(memstream_buf_t *dst, memstream_buf_t *src) {
    while(src->pos < src->len) {
        int count = src->data[src->pos++];
        int pix = src->data[src->pos++];

        for(int i=0; i<count; i++) {
            // check for destination overflow
            if(dst->pos >= dst->len) {
                return -1;
            }
            dst->data[dst->pos++] = pix;
        }
    }
    return 0;
}
