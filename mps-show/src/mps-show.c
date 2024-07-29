#include <stdio.h>
#include <stdlib.h>
#include "mps-show.h"
#include "util.h"

/// @brief Reads in the slide information block
/// @param fp pointer to an open file with the MpsShow data
/// @param count pointer to a var for holding the slide count
/// @return pointer to allocated memory containing all the info records for all the slides
/// returns NULL on failure
info_t *read_mps_show_info_header(FILE *fp, int *count) {
    info_t *slide_info = NULL;

    int num_slides = fgetc(fp); 
    if((EOF == num_slides) || (0 == num_slides)) {
        return NULL;
    }
    *count = num_slides;

    // allocate the buffer for the all the slide records
    if(NULL == (slide_info = (info_t *)calloc(num_slides,sizeof(info_t)))) {
        return NULL;
    }

    int nr = fread(slide_info, sizeof(info_t), num_slides, fp);
    if(nr != num_slides) {
        free(slide_info);
        return NULL;
    }

    return slide_info;
}

/// @brief RLE decompresses the input datastream
/// RLE format is as a stream of 16 bit records (count and data)
/// @param dst pointer to a memstream buffer to hold teh uncompressed data
/// @param src pointer to a memstream buffer with hte compressed datastream
/// @return 0 on success, -1 if destination is too small
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

int read_mps_show_image(memstream_buf_t *dst, FILE *fp, info_t *slide) {
    int rval = -1;
    memstream_buf_t src = {0, 0, NULL};

    // allocate our input buffer
    if(NULL == (src.data = calloc(1, slide->img_len))) {
        goto cleanup;
    }
    src.len = slide->img_len;
    src.pos = 0;

    // goto the image in the file
    fseek(fp, slide->img_offset, SEEK_SET);

    // read in the compressed data
    int nr = fread(src.data, src.len, 1, fp);
    if(1 != nr) {
        goto cleanup;
    }

    // decompress the image
    if(0 != rle_decompress(dst, &src)) {
        goto cleanup;
    }

    rval = 0;
cleanup:
    free_s(src.data);
    return rval;
}

