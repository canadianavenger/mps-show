# mps-show
This repository is a companion to my blog post about the extracting and reverse engineering of the MPSShow data format used with some slideshow demos distributed by MicroProse. For more detail please read [my blog post](https://canadianavenger.io/2024/06/13/show-me-the-money/).


## The Code
The code included here is based on what was outlined in the blog post, but is arranged differently than presented there. In this repo there are three C programs, each is a standalone utility for extracting the slideshow MPSShow slideshow data. The code is mostly written to be portable (POSIX), and should be able to be compiled for Windows, Linux, or Mac. Though some changes may be necessary for declaring the structures as ***packed***, if not using GCC. The code is offered without warranty under the MIT License. Use it as you will personally or commercially, just give credit if you do.

- `mpsextract.c` extracts the slideshow data from the given `.exe` file and saves it as a `.mps` file. All the other programs are written to work with the `.mps` file.
- `palextract.c` extracts just the palette information for the given slide index. The palette is saved as a 768 byte RGB data file.
- `mpsexplore.c` lists all the slides along with their meta data, can also be used to extract specific images into a Windows BMP format image.

Currently I have not written any code to encode a custimized slideshow. Some more reverse engineering to decode the remaining data would be needed before this could really be useful. My main goal was to extract teh palette for my MicroProse `.PIC` File Format decoding and rendering efforts.

## Project Structure
The main command-line utilities can be found in the `executables/` directory. While the core code for handling the mps-show file format itself can be found in the `mps-show/` directory. The core code has been arranged as its own cmake project making it easier to extract and incorporate into other projects. The `tools/` directory contains some simple helper functions, and the `quickbmp/` directory contains a basic library for handling Windows BMP files. For imformation pertaining to the mpsshow file format see `mps-show/README.md`


