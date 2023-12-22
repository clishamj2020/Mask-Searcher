#ifndef PNG_CPP
#define PNG_CPP

//--------------------------------------------------------------------
//
// Copyright (C) 2022 raodm@miamiOH.edu
//
// Miami University makes no representations or warranties about the
// suitability of the software, either express or implied, including
// but not limited to the implied warranties of merchantability,
// fitness for a particular purpose, or non-infringement.  Miami
// University shall not be liable for any damages suffered by licensee
// as a result of using, result of using, modifying or distributing
// this software or its derivatives.
//
// By using or copying this Software, Licensee agrees to abide by the
// intellectual property laws, and all other applicable laws of the
// U.S., and the terms of GNU General Public License (version 3).
//
// Authors:   DJ Rao          raodm@miamiOH.edu
//
//---------------------------------------------------------------------

#include "PNG.h"
#include "Assert.h"
#include <string>

PNG::PNG() {
    width  = 0;
    height = 0;
}

PNG::PNG(const PNG& src) : width(src.width), height(src.height) {
    prepareBuffer();
    flatImageBuffer = src.flatImageBuffer;
}

PNG::~PNG() {
}

PNG&
PNG::operator=(const PNG& src) {
    this->width  = src.width;
    this->height = src.height;
    prepareBuffer();
    flatImageBuffer = src.flatImageBuffer;
    return *this;
}

FILE*
PNG::validateHeader(const char* fileName) {
    // Try to open the file
    FILE *pngFile = fopen(fileName, "rb");
    if (pngFile == NULL) {
        std::string errMsg = "PNG File (";
        errMsg += fileName;
        errMsg += ") could not be opened for reading";
        throw std::runtime_error(errMsg);
    }
    // Read in the PNG header to verify that this is a valid PNG. The
    // header is 8 bytes.
    unsigned char pngHeader[8];
    if (fread(pngHeader, sizeof(char), 8, pngFile) != 8) {
        throw std::runtime_error("Error reading header from PNG file");
    }
    if (png_sig_cmp(pngHeader, 0, 8) != 0) {
        // The PNG header did not match
        throw std::runtime_error("File specified is not a valid PNG file");
    }
    // Return the opened file handle for further use.
    return pngFile;
}

void
PNG::open(const char* fileName) {
    ASSERT(pngInfo == NULL);
    // Try to open the file and validate the PNG header in it.
    FILE *pngFile = validateHeader(fileName);
    // Set up libpng reading and setup pngInfo structure
    png_infop pngInfo = NULL;
    png_structp libpngHandle = createPNGHandle(true, &pngInfo);
    ASSERT(pngInfo != NULL);
    // Set up PNG IO
    png_init_io(libpngHandle, pngFile);
    png_set_sig_bytes(libpngHandle, 8);
    // Actually read the info
    png_read_info(libpngHandle, pngInfo);
    // Unfortunately, we now need to consider interlace handling
    png_set_interlace_handling(libpngHandle);
    // And update the information struc accordingly
    png_read_update_info(libpngHandle, pngInfo);

    // Finally, make sure this is a PNG we can handle
    if (png_get_color_type(libpngHandle, pngInfo) != PNG_COLOR_TYPE_RGBA) {
        throw std::runtime_error("Specified PNG is not in RGBA color mode");
    }
    if (png_get_bit_depth(libpngHandle, pngInfo) != 8) {
        throw std::runtime_error("Specified PNG does not have bit depth of 8");
    }
    // Use helper method to load the actual image data
    load(libpngHandle, pngInfo);
    // Close the PNG file
    fclose(pngFile);
    png_destroy_read_struct(&libpngHandle, &pngInfo, NULL);
}

void
PNG::load(const std::string& fileName) {
    open(fileName.c_str());
}

void
PNG::load(png_structp libpngHandle, png_infop pngInfo) {
    // Okay... This is where things get weird. Since libpng is a C
    // library, it can't use exceptions. Instead, it uses the
    // longjmp() mechanism. Here, we have the error catching code to
    // pass an exception up to the rest of the C++ implementation for
    // *subsequent* libpng calls.
    if (setjmp(png_jmpbuf(libpngHandle)) != 0) {
        // An error occured
        throw std::runtime_error("libpng failed to load image bytes");
    }

    ASSERT(libpngHandle != NULL);
    ASSERT(pngInfo      != NULL);
    // Save width and height for future use.
    width  = png_get_image_width(libpngHandle, pngInfo);
    height = png_get_image_height(libpngHandle, pngInfo);
    // Prepare the in-memory buffer to load image
    prepareBuffer();
    // Read the data into our internal buffers.
    png_read_image(libpngHandle, &rowPointers[0]);
}

void
PNG::create(int width, int height) {
    this->width  = width;
    this->height = height;
    // Finally, prepare a buffer
    prepareBuffer();    
}

png_structp
PNG::createPNGHandle(bool readFlag, png_infop* pngInfo) {
    png_structp libpngHandle = NULL;
    if (!readFlag) {
        libpngHandle = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               NULL, NULL, NULL);
    } else {
        libpngHandle = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    }
    ASSERT(libpngHandle != NULL);
    // Prepare the PNG information structure, if supplied
    if (pngInfo != NULL) {
        *pngInfo = png_create_info_struct(libpngHandle);
        if (*pngInfo == NULL) {
            throw std::runtime_error("Unable to set up PNG information");
        }
    }
    // Okay... This is where things get weird. Since libpng is a C
    // library, it can't use exceptions. Instead, it uses the
    // longjmp() mechanism. Here, we have the error catching code to
    // pass an exception up to the rest of the C++ implementation for
    // *subsequent* libpng calls.
    if (setjmp(png_jmpbuf(libpngHandle)) != 0) {
        // An error occured
        throw std::runtime_error("libpng failed to read/write PNG");
    }
    return libpngHandle;
}

void
PNG::write(const std::string& fileName) {
    // Prepare a PNG write handle
    png_structp libpngHandle = createPNGHandle(false, NULL);
    // Prepare the information structure
    png_infop pngInfo = png_create_info_struct(libpngHandle);
    if (pngInfo == NULL) {
        throw std::runtime_error("Unable to set up PNG information");
    }
    // Fill the header with good information
    png_set_IHDR(libpngHandle, pngInfo, width, height,
                 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    // Try to open the file
    FILE* pngFile = fopen(fileName.c_str(), "wb");
    if (pngFile == NULL) {
        throw std::runtime_error("PNG File could not be opened for writing");
    }
    // Uncomment this line for BMP-like PNGs (huge files, no compression)
    // png_set_compression_level(libpngHandle, 0);

    // Set up PNG IO
    png_init_io(libpngHandle, pngFile);
    // Write the info struct
    png_write_info(libpngHandle, pngInfo);
    // Write the actual image bytes
    png_write_image(libpngHandle, &rowPointers[0]);
    png_write_end(libpngHandle, NULL);
    png_destroy_write_struct(&libpngHandle, &pngInfo);
    fclose(pngFile);
}

int
PNG::getBufferSize() const {
    return height * width * 4;
}

void
PNG::prepareBuffer() {
    flatImageBuffer.clear();
    flatImageBuffer.resize(getBufferSize());
    rowPointers.resize(height);
    unsigned char* const bufStart = &flatImageBuffer[0];
    const int rowBytes            = width * 4;
    for (int row = 0; (row < height); row++) {
        rowPointers[row] = bufStart + (row * rowBytes);
    }
}

void
PNG::setRed(const int row, const int col) {
    const int idx = (row * width + col) * 4;
    flatImageBuffer[idx + 1] = flatImageBuffer[idx + 2] = 0;
    flatImageBuffer[idx]     = flatImageBuffer[idx + 3] = 255;        
}

#endif
