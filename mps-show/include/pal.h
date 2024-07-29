/*
 * pal.h 
 * structure definitions for a RGB PAlette file
 * 
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdint.h>

#ifndef IMG_PAL
#define IMG_PAL

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} pal_entry_t;

/// @brief upscales 4 bit per component palette data to 6 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal4_to_pal6(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);

/// @brief upscales 4 bit per component palette data to 8 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal4_to_pal8(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);

/// @brief upscales 6 bit per component palette data to 8 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal6_to_pal8(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);

/// @brief downscales 6 bit per component palette data to 4 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal6_to_pal4(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);

/// @brief downscales 8 bit per component palette data to 4 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal8_to_pal4(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);

/// @brief downscales 8 bit per component palette data to 6 bits per component
/// @param pal pointer to buffer of RGB palette entries
/// @param entries number of entries in the palette
void pal8_to_pal6(pal_entry_t *in_pal, pal_entry_t *out_pal, int entries);



#endif
