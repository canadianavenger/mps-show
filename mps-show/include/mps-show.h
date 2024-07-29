/*
 * mps-show.h 
 * structure definitions for a MicroProse MPSShow file
 * 
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdint.h>
#include "pal.h"
#include "memstream.h"

#ifndef MPS_SHOW
#define MPS_SHOW

// portability note, this is specific to GCC to pack the structurew without padding
#pragma pack(push,1)
// needs to be 835 bytes
typedef struct {
    uint8_t name_len;           // pascal string length prefix
    char    name[9];            // name of image file, without extension
    uint8_t desc_len;           // pascal string length prefix
    char    desc[25];           // brief description of the slide
    uint32_t img_offset;        // offset if image in the file
    uint16_t mode;              // video mode? or length of unknown data after the palette? 
                                // 0x13 = 19, and there are 19 bytes after the palette
    uint16_t img_len;           // length of compressed image data
    uint32_t unknown32;         // looks to be uninitialized bytes
    pal_entry_t pal[256];       // 768 bytes of R G B palette data (0-63 per component)
    uint8_t unknown[19];        // unknown data, but likely show flow and control data
} info_t;
#pragma pack(pop)

#define MPSRECSZ (835)    // size of MPSShow info record

/// @brief Reads in the slide information block
/// @param fp pointer to an open file with the MpsShow data
/// @param count pointer to a var for holding the slide count
/// @return pointer to allocated memory containing all the info records for all the slides
/// returns NULL on failure
info_t *read_mps_show_info_header(FILE *fp, int *count);

/// @brief Reads in the image referenced by 'slide'
/// @param dst pointer to an allocaed buffer large enough to hold the uncompressed image
/// @param fp pointer to an open file with the image data
/// @param slide pointer to a slide record for the image to load
/// @return returns 0 on success
int read_mps_show_image(memstream_buf_t *dst, FILE *fp, info_t *slide);

#endif
