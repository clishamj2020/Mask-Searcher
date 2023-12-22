#ifndef PNG_HELPER_H
#define PNG_HELPER_H

//--------------------------------------------------------------------
//
// Copyright (C) 2015 raodm@miamiOH.edu
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
// Authors:   Dhananjai M. Rao          raodm@muohio.edu
//
//---------------------------------------------------------------------

#include <png.h>
#include <stdexcept>
#include <cstdio>
#include <vector>
#include <string>

/**
   A convenience union to access individual components of a pixel. For
   example, the values can be accessed as:

   Pixel p;
   p.color.red = 10;
*/
typedef union Pix {
    class {
    public:
        unsigned char red, green, blue, alpha;
    } color;
    unsigned int rgba;
} Pixel;

class PNG {
public:
    /** The default constructor which creates an empty PNG image in memory.

        This constructor is present so that it is convenient to use
        this PNG class in conjunction without various standard C++
        containers.
    */
    PNG();

	/**
	   Copy constructor.

	   \param[in] src The source PNG image from where the image data
	   is to be copied.
	*/
    PNG(const PNG& src);

	/**
	   Assignment operator.

	   \param[in] src The source PNG from where the information is to
	   be copied.
	*/
    PNG& operator=(const PNG& src);

    /**
       The destructor frees the dynamic memory allocated to the
       various instance variables encapsulated by this class.
    */
    virtual ~PNG();

    /** \brief Create a blank image (to be filled-in)

        This method creates a sufficiently large buffer to hold an
        image of the specified size in RGBA mode, as well as preparing
        a libpng handle to write the said image, if desired. Note that
        this method cannot be used on a PNG instance that already has
        a PNG buffer (previously created or loaded from a PNG file).
        
        \param[in] width The width of the image.

        \param[in] height The height of the image.

        \see getImage
    */
    void create(int width, int height);
    
    /** \brief Open and load the specified PNG

        This method reads the image data from a given file into
        internal buffers.

        \param[in] fileName The path to the PNG file from where the
        image is to be loaded.

        \see getImage (to access the pixel data for the image)
    */
    void load(const std::string& fileName);

    /** \brief Write the image from internal buffers to a given PNG
        file.

        This method writes the image data from internal buffers into a
        given file.

        \param[in] fileName The path to the PNG file to where the
        image is to be written.
    */
    void write(const std::string& fileName);

    /** Returns the widht of this PNG image.

        \return The width of this PNG image.
    */
    int getWidth() const { return width; }

    /** Returns the height of this PNG image.

        \return The height of this PNG image.
    */    
    int getHeight() const { return height; }
    
    /** \brief Determine the size of the buffer that is needed to hold
        the entire image

        This method computes the size of the buffer that must be
        allocated to safely hold the entire image data. This depends
        on the width, height, and number of bytes per pixel.

     */
    int getBufferSize() const;

    /** Return the pixel at a given location.

        \param[in] row The row within the image.
        \param[in] col The column within the image

        \return The Pixel (red, gree, blue, alpha) at the given location.
    */
    Pixel getPixel(const int row, const int col) const {
        const int idx = (row * width + col) * 4;
        const unsigned int* pix = 
            reinterpret_cast<const unsigned int*>(flatImageBuffer.data() + idx);
        return Pixel{ .rgba = *pix };
    }
    
    /** Get an immutable reference to the flat buffer image.

        The pixels associated with this image. Each pixel is in RGBA
        format.  The pixels are stored in a row-major format.
        
        \return A reference to the flat image buffer that contains the
        pixels.
    */
    inline const std::vector<unsigned char>& getBuffer() const
    { return flatImageBuffer; }

    /** Get an mutable reference to the flat buffer image.

        The pixels associated with this image. Each pixel is in RGBA
        format.  The pixels are stored in a row-major format.
        
        \return A reference to the flat image buffer that contains the
        pixels.
    */    
    inline std::vector<unsigned char>& getBuffer() { return flatImageBuffer; }

	/** Set a given pixel in the PNG image to red color.

		\param[in] row The row of the image to be set to red color. No
		checks are made on this value.  Passing in an invalid row
		results in undefined behavior.

		\param[in] col The column of the image to be set to red color.
		No checks are made on this value.  Passing in an invalid row
		results in undefined behavior.
	*/
    void setRed(const int row, const int col);

protected:
    /** \brief Open the specified PNG file

        This method attempts to open the specified PNG file, load the
        headers and information structures about the PNG. The actual
        image data will not be read.

        \param[in] fileName A relative or absolute path to the PNG file

        \throws std::runtime_error If the the file specified cannot be
        read or is not a PNG file.
    */
    void open(const char* fileName);

    /** Loads the actual image via calls to libpng.

        This method is an internal helper method that has been
        introduced to streamline the code for loading PNG files from
        disk.  This method performs the core task of loading the image
        data from disk.  This method is invoked from the open() method.

        \param[in,out] libpngHandle The handle to be used to read the
        actual image data.
    */
    void load(png_structp libpngHandle, png_infop pngInfo);


    /** Resize the flatImageBuffer vector to contain all the pixels in
        this PNG.

        This is a convenience method that is used to setup the
        necessary buffer size and establish the cross reference
        pointers used internally by this PNG object.
    */
    void prepareBuffer();

    /** Create read/write png handle and setup jump handle as required
        by libPNG.

        This is a helper method that is used to create png handles and
        set cross reference jump handle required by libPNG.  This is
        required because libpng is a C library, and it can't use
        exceptions. Instead, it uses the longjmp() mechanism in lieu
        of exception.  Here, we have the error catching code to pass
        an exception up to the rest of the C++ implementation for
        *subsequent* libpng calls.

        \param[in] rw If this flag is true then the PNG handle is
        created for reading. Otherwise the handle is created for
        writing.

        \param[in] pngInfo An optional pointer to an libPNG
        information structure. If this pointer is not NULL, then the
        supplied structure is initialized.

        \return A valid PNG handle on
        success. On errors this method throws an exception.
     */
    png_structp createPNGHandle(bool readFlag, png_infop* pngInfo = NULL);

    /** Refactored method to open and validate PNG header.

        This method is called from the PNG::open(const char *) method.
        This method opens a given file and checks to ensure it is
        indeed a PNG file.

        \param[in] fileName Path to the file to be opened by this
        method.

        \return This method returns a pointer to the FILE stream
        opened by this method on success. On errrors, this method
        throws an exception.
    */
    FILE* validateHeader(const char* fileName);
    
private:
    /** \brief Handle to low-level libpng

        This instance variable stores the handle to the underlying PNG
        library, libpng. All operations on PNGs are passed to libpng
        using this handle.

     */
    // png_structp libpngHandle;

    /** \brief Information about the PNG (width, height, etc.)

        This structure is provided by libpng to store basic
        information about the PNG file being read, such as width,
        height, color type, and bit depth.

    */
    // png_infop pngInfo;

    /**
       The width of this PNG image.
     */
    int width;

    /**
       The height of this PNG image.
     */
    int height;

    /** The flat buffer that contains all the pixels in the
        image. Each pixel is a 32-bit number that contains data in
        RGBA format. The pixels are stored in row major format.
    */
    std::vector<unsigned char> flatImageBuffer;

    /**
       Pointers to the starting entry in each row of the image. This
       set of pointers is needed and used by libpng to load and write
       images  to-and-from files.
    */
    std::vector<unsigned char*> rowPointers;
};

#endif
