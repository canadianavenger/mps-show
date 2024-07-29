# mps-show library

This library is for reading and extracting images from a MicroProse MPSShow slideshow presentation.  

## The MPSShow Format

The MPSShow data is appended onto the end of the `.exe` file. The first byte of data defines how many slides are in the slideshow. This is followed by an array of informational records, one for each slide. After that is all of the image data as a continuous block. The general structure of the appended data is. (pseudo-c)

```c
typedef struct {
    uint8_t slide_count;       // number of slides in the slideshow 
    info_t  slide_info[];      // Slide information header, has 'slide_count' records
    uint8_t image_data[];      // block of data holding the compressed images
} mps_file_t;
```

The `info_t` records are 835 bytes long and have the following form.

```c
typedef struct {
    uint8_t name_len;           // PASCAL string length prefix
    char    name[9];            // file name of the slide, without extension
    uint8_t desc_len;           // PASCAL string length prefix
    char    desc[25];           // brief descripton of the slide
    uint32_t img_offset;        // offset of image data in the EXE file
    uint16_t mode;              // video mode? or length of unknown data after the palette? 
                                  // (always 0x13 = 19, and there are 19 bytes after the palette)
    uint16_t img_len;           // length of the RLE compressed image
    uint32_t unknown32;         // looks to be uninitialized bytes
    pal_entry_t pal[256];       // RGB palette data (VGA range: 0-63 per component)
    uint8_t unknown[19];        // unknown data, but likely show flow and control data
} info_t;

```

The image data is RLE compressed and takes the form.

```c
typedef struct {
    uint8_t count;              // repeat count for value
    uint8_t value;              // the data value
} rle_t;

typedef struct {
    rle_t data[];               // variable number of RLE records
} rle_data_t;
```

All the images have a resolution of 320x200 with 256 colours. (at least as for the one example we've looked at) There does not appear to be any obvious encoding of the image width and height, so this  either fixed, or inferred through one of the unknown data fileds, possibly the `mode` field.
