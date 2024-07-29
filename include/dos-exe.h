/*
 * dos-exe.h 
 * structure definition for a MS-DOS EXE file header
 * 
 * This code is offered without warranty under the MIT License. Use it as you will 
 * personally or commercially, just give credit if you do.
 */
#include <stdint.h>

#ifndef DOS_EXE
#define DOS_EXE

#define EXE_SIG "MZ"
#define EXE_BLOCK_SZ (512)

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

#endif
