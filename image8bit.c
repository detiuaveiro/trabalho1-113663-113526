/// image8bit - A simple image processing module.
///
/// This module is part of a programming project
/// for the course AED, DETI / UA.PT
///
/// You may freely use and modify this code, at your own risk,
/// as long as you give proper credit to the original and subsequent authors.
///
/// João Manuel Rodrigues <jmr@ua.pt>
/// 2013, 2023

// Student authors (fill in below):
// NMec: 113526  Name: Rodrigo Marques de Jesus
// NMec: 113663  Name: Bruno Guilherme Mendes de Oliveira
// 
// 
// Date:
//

#include "image8bit.h"

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "instrumentation.h"

// The data structure
//
// An image is stored in a structure containing 3 fields:
// Two integers store the image width and height.
// The other field is a pointer to an array that stores the 8-bit gray
// level of each pixel in the image.  The pixel array is one-dimensional
// and corresponds to a "raster scan" of the image from left to right,
// top to bottom.
// For example, in a 100-pixel wide image (img->width == 100),
//   pixel position (x,y) = (33,0) is stored in img->pixel[33];
//   pixel position (x,y) = (22,1) is stored in img->pixel[122].
// 
// Clients should use images only through variables of type Image,
// which are pointers to the image structure, and should not access the
// structure fields directly.

// Maximum value you can store in a pixel (maximum maxval accepted)
const uint8 PixMax = 255;

// Internal structure for storing 8-bit graymap images
struct image {
    int width;
    int height;
    int maxval;   // maximum gray value (pixels with maxval are pure WHITE)
    uint8 *pixel; // pixel data (a raster scan)
};


// This module follows "design-by-contract" principles.
// Read `Design-by-Contract.md` for more details.

/// Error handling functions

// In this module, only functions dealing with memory allocation or file
// (I/O) operations use defensive techniques.
//
// When one of these functions fails, it signals this by returning an error
// value such as NULL or 0 (see function documentation), and sets an internal
// variable (errCause) to a string indicating the failure cause.
// The errno global variable thoroughly used in the standard library is
// carefully preserved and propagated, and clients can use it together with
// the ImageErrMsg() function to produce informative error messages.
// The use of the GNU standard library error() function is recommended for
// this purpose.
//
// Additional information:  man 3 errno;  man 3 error;

// Variable to preserve errno temporarily
static int errsave = 0;

// Error cause
static char *errCause;

/// Error cause.
/// After some other module function fails (and returns an error code),
/// calling this function retrieves an appropriate message describing the
/// failure cause.  This may be used together with global variable errno
/// to produce informative error messages (using error(), for instance).
///
/// After a successful operation, the result is not garanteed (it might be
/// the previous error cause).  It is not meant to be used in that situation!
char *ImageErrMsg() { ///
    return errCause;
}


// Defensive programming aids
//
// Proper defensive programming in C, which lacks an exception mechanism,
// generally leads to possibly long chains of function calls, error checking,
// cleanup code, and return statements:
//   if ( funA(x) == errorA ) { return errorX; }
//   if ( funB(x) == errorB ) { cleanupForA(); return errorY; }
//   if ( funC(x) == errorC ) { cleanupForB(); cleanupForA(); return errorZ; }
//
// Understanding such chains is difficult, and writing them is boring, messy
// and error-prone.  Programmers tend to overlook the intricate details,
// and end up producing unsafe and sometimes incorrect programs.
//
// In this module, we try to deal with these chains using a somewhat
// unorthodox technique.  It resorts to a very simple internal function
// (check) that is used to wrap the function calls and error tests, and chain
// them into a long Boolean expression that reflects the success of the entire
// operation:
//   success =
//   check( funA(x) != error , "MsgFailA" ) &&
//   check( funB(x) != error , "MsgFailB" ) &&
//   check( funC(x) != error , "MsgFailC" ) ;
//   if (!success) {
//     conditionalCleanupCode();
//   }
//   return success;
//
// When a function fails, the chain is interrupted, thanks to the
// short-circuit && operator, and execution jumps to the cleanup code.
// Meanwhile, check() set errCause to an appropriate message.
//
// This technique has some legibility issues and is not always applicable,
// but it is quite concise, and concentrates cleanup code in a single place.
//
// See example utilization in ImageLoad and ImageSave.
//
// (You are not required to use this in your code!)


// Check a condition and set errCause to failmsg in case of failure.
// This may be used to chain a sequence of operations and verify its success.
// Propagates the condition.
// Preserves global errno!
static int check(int condition, const char *failmsg) {
    errCause = (char *) (condition ? "" : failmsg);
    return condition;
}


/// Init Image library.  (Call once!)
/// Currently, simply calibrate instrumentation and set names of counters.
void ImageInit(void) { ///
    InstrCalibrate();
    InstrName[0] = "pixmem";  // InstrCount[0] will count pixel array acesses
    // Name other counters here...
    // TO DO
    InstrName[1] = "comparison";  // InstrCount[1] will count number of pixel comparisons between 2 images
}

// Macros to simplify accessing instrumentation counters:
#define PIXMEM InstrCount[0]
// Add more macros here...
// TO DO
// TIP: Search for PIXMEM or InstrCount to see where it is incremented!
#define COMP InstrCount[1]


/// Image management functions

/// Create a new black image.
///   width, height : the dimensions of the new image.
///   maxval: the maximum gray level (corresponding to white).
/// Requires: width and height must be non-negative, maxval > 0.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCreate(int width, int height, uint8 maxval) { ///
    assert (width >= 0);
    assert (height >= 0);
    assert (0 < maxval && maxval <= PixMax);
    // Altered

    // Allocating a new image
    Image img = (Image)malloc(sizeof(struct image));

    // Initializing image properties
    img->width = width;
    img->height = height;
    img->maxval = maxval;

    // Allocating memory for the pixel data
    img->pixel = (uint8*)malloc(width * height * sizeof(uint8));

    if (img->pixel == NULL) {
        free(img); // Freeing Image
        return NULL; // If allocation fails return null
    }

    // Initializing pixels (set all pixels to black)
    for(int i = 0; i < width * height; i++) {
        img->pixel[i] = 0;
    }
    return img;
}

/// Destroy the image pointed to by (*imgp).
///   imgp : address of an Image variable.
/// If (*imgp)==NULL, no operation is performed.
/// Ensures: (*imgp)==NULL.
/// Should never fail, and should preserve global errno/errCause.
void ImageDestroy(Image *imgp) { ///
    assert (imgp != NULL);
    // Altered

    if (*imgp != NULL) {
        free((*imgp)->pixel); //Freeing pixel array
        *imgp = NULL;
    }
}


/// PGM file operations

// See also:
// PGM format specification: http://netpbm.sourceforge.net/doc/pgm.html

// Match and skip 0 or more comment lines in file f.
// Comments start with a # and continue until the end-of-line, inclusive.
// Returns the number of comments skipped.
static int skipComments(FILE *f) {
    char c;
    int i = 0;
    while (fscanf(f, "#%*[^\n]%c", &c) == 1 && c == '\n') {
        i++;
    }
    return i;
}

/// Load a raw PGM file.
/// Only 8 bit PGM files are accepted.
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageLoad(const char *filename) { ///
    int w, h;
    int maxval;
    char c;
    FILE *f = NULL;
    Image img = NULL;

    int success =
            check((f = fopen(filename, "rb")) != NULL, "Open failed") &&
            // Parse PGM header
            check(fscanf(f, "P%c ", &c) == 1 && c == '5', "Invalid file format") &&
            skipComments(f) >= 0 &&
            check(fscanf(f, "%d ", &w) == 1 && w >= 0, "Invalid width") &&
            skipComments(f) >= 0 &&
            check(fscanf(f, "%d ", &h) == 1 && h >= 0, "Invalid height") &&
            skipComments(f) >= 0 &&
            check(fscanf(f, "%d", &maxval) == 1 && 0 < maxval && maxval <= (int) PixMax, "Invalid maxval") &&
            check(fscanf(f, "%c", &c) == 1 && isspace(c), "Whitespace expected") &&
            // Allocate image
            (img = ImageCreate(w, h, (uint8) maxval)) != NULL &&
            // Read pixels
            check(fread(img->pixel, sizeof(uint8), w * h, f) == w * h, "Reading pixels");
    PIXMEM += (unsigned long) (w * h);  // count pixel memory accesses

    // Cleanup
    if ( !success ) {
        errsave = errno;
        ImageDestroy(&img);
        errno = errsave;
    }
    if ( f != NULL) fclose(f);
    return img;
}

/// Save image to PGM file.
/// On success, returns nonzero.
/// On failure, returns 0, errno/errCause are set appropriately, and
/// a partial and invalid file may be left in the system.
int ImageSave(Image img, const char *filename) { ///
    assert (img != NULL);
    int w = img->width;
    int h = img->height;
    uint8 maxval = img->maxval;
    FILE *f = NULL;

    int success =
            check((f = fopen(filename, "wb")) != NULL, "Open failed") &&
            check(fprintf(f, "P5\n%d %d\n%u\n", w, h, maxval) > 0, "Writing header failed") &&
            check(fwrite(img->pixel, sizeof(uint8), w * h, f) == w * h, "Writing pixels failed");
    PIXMEM += (unsigned long) (w * h);  // count pixel memory accesses

    // Cleanup
    if ( f != NULL) fclose(f);
    return success;
}


/// Information queries

/// These functions do not modify the image and never fail.

/// Get image width
int ImageWidth(Image img) { ///
    assert (img != NULL);
    return img->width;
}

/// Get image height
int ImageHeight(Image img) { ///
    assert (img != NULL);
    return img->height;
}

/// Get image maximum gray level
int ImageMaxval(Image img) { ///
    assert (img != NULL);
    return img->maxval;
}

/// Pixel stats
/// Find the minimum and maximum gray levels in image.
/// On return,
/// *min is set to the minimum gray level in the image,
/// *max is set to the maximum.
void ImageStats(Image img, uint8 *min, uint8 *max) { ///
    assert (img != NULL);
    // Altered

    // Initializing min and max
    *min = 255;
    *max = 0;

    // Iterating on pixel array to find min and max
    for (int i = 1; i < img->width * img->height;i++) {
        if (img->pixel[i] < *min) {
            *min = img->pixel[i];
        } else if (img->pixel[i] > *max) {
            *max = img->pixel[i];
        }
    }

}

/// Check if pixel position (x,y) is inside img.
int ImageValidPos(Image img, int x, int y) { ///
    assert (img != NULL);
    return (0 <= x && x < img->width) && (0 <= y && y < img->height);
}

/// Check if rectangular area (x,y,w,h) is completely inside img.
int ImageValidRect(Image img, int x, int y, int w, int h) { ///
    assert (img != NULL);
    // Altered

    // Checking if the first corner is inside the image
    if (!ImageValidPos(img,x, y)) {
        return 0;
    }

    // Initializing coordinates of the opposite corner
    int x2 = x + w - 1;
    int y2 = y + h - 1;

    // Checking if the opposite corner is inside the image
    if (!ImageValidPos(img,x2, y2)) {
        return 0;
    }

    return 1;
}

/// Pixel get & set operations

/// These are the primitive operations to access and modify a single pixel
/// in the image.
/// These are very simple, but fundamental operations, which may be used to
/// implement more complex operations.

// Transform (x, y) coords into linear pixel index.
// This internal function is used in ImageGetPixel / ImageSetPixel.
// The returned index must satisfy (0 <= index < img->width*img->height)
static inline int G(Image img, int x, int y) {
    // Altered

    int index = y * img->width + x;

    assert (0 <= index && index < img->width * img->height);
    return index;
}

/// Get the pixel (level) at position (x,y).
uint8 ImageGetPixel(Image img, int x, int y) { ///
    assert (img != NULL);
    assert (ImageValidPos(img, x, y));
    PIXMEM += 1;  // count one pixel access (read)
    return img->pixel[G(img, x, y)];
}

/// Set the pixel at position (x,y) to new level.
void ImageSetPixel(Image img, int x, int y, uint8 level) { ///
    assert (img != NULL);
    assert (ImageValidPos(img, x, y));
    PIXMEM += 1;  // count one pixel access (store)
    img->pixel[G(img, x, y)] = level;
}


/// Pixel transformations

/// These functions modify the pixel levels in an image, but do not change
/// pixel positions or image geometry in any way.
/// All of these functions modify the image in-place: no allocation involved.
/// They never fail.


/// Transform image to negative image.
/// This transforms dark pixels to light pixels and vice-versa,
/// resulting in a "photographic negative" effect.
void ImageNegative(Image img) { ///
    assert (img != NULL);
    // Altered

    for (int i = 0; i < img->width * img->height; i++) {
        img->pixel[i] = PixMax - img->pixel[i]; // Invert pixel values by doing 255 - current value
    }
}

/// Apply threshold to image.
/// Transform all pixels with level<thr to black (0) and
/// all pixels with level>=thr to white (maxval).
void ImageThreshold(Image img, uint8 thr) { ///
    assert (img != NULL);
    // Altered

    for (int i = 0; i < img->width * img->height; i++) {
        if (img->pixel[i] < thr) {
            img->pixel[i] = 0; // Set pixel to black
        } else img->pixel[i] = img->maxval; // Set pixel to maxval
    }
}

/// Brighten image by a factor.
/// Multiply each pixel level by a factor, but saturate at maxval.
/// This will brighten the image if factor>1.0 and
/// darken the image if factor<1.0.
void ImageBrighten(Image img, double factor) { ///
    assert (img != NULL);
    assert (factor >= 0.0);
    // Altered

    for (int i = 0; i < img->width * img->height; i++) {
        if ((factor * img->pixel[i]) > img->maxval) {
            img->pixel[i] = img->maxval; //Verifying if pixel levels don't exceed maxval
        } else
            img->pixel[i] = (uint8)(factor * img->pixel[i] + 0.5); // Adding 0.5 ensures it will round correctly when converting to uint8
    }
}


/// Geometric transformations

/// These functions apply geometric transformations to an image,
/// returning a new image as a result.
///
/// Success and failure are treated as in ImageCreate:
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.

// Implementation hint:
// Call ImageCreate whenever you need a new image!

/// Rotate an image.
/// Returns a rotated version of the image.
/// The rotation is 90 degrees anti-clockwise.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageRotate(Image img) { ///
    assert (img != NULL);
    // Altered
    // Creating a new image with swapped width and height
    Image rImg = ImageCreate(img->height, img->width, img->maxval);

    if (rImg == NULL) {
        errno = ENOMEM;
        errCause = "Failed to allocate memory for rotated image";
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            // Swap x and y coordinates
            int rx = y;
            int ry = img->width - x - 1;
            uint8 level = ImageGetPixel(img,x,y);
            ImageSetPixel(rImg, rx, ry, level);
        }
    }
    return rImg;
}

/// Mirror an image = flip left-right.
/// Returns a mirrored version of the image.
/// Ensures: The original img is not modified.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageMirror(Image img) { ///
    assert (img != NULL);
    // Altered
    // Creating a new image with same width and height
    Image mImg = ImageCreate(img->width, img->height, img->maxval);

    if (mImg == NULL) {
        errno = ENOMEM;
        errCause = "Failed to allocate memory for mirrored image";
        return NULL;
    }

    for (int y = 0; y < img->height; y++) {
        for (int x =0; x < img->width; x++) {
            // Flip x coordinates and keep y the same
            int rx = img->height - x- 1;
            int ry = y;
            uint8 level = ImageGetPixel(img,x,y);
            ImageSetPixel(mImg, rx, ry, level);
        }
    }
    return mImg;
}


/// Crop a rectangular subimage from img.
/// The rectangle is specified by the top left corner coords (x, y) and
/// width w and height h.
/// Requires:
///   The rectangle must be inside the original image.
/// Ensures:
///   The original img is not modified.
///   The returned image has width w and height h.
///
/// On success, a new image is returned.
/// (The caller is responsible for destroying the returned image!)
/// On failure, returns NULL and errno/errCause are set accordingly.
Image ImageCrop(Image img, int x, int y, int w, int h) { ///
    assert (img != NULL);
    assert (ImageValidRect(img, x, y, w, h));
    // Altered
    Image cImg = ImageCreate(w, h, img->maxval);

    if (cImg == NULL) {
        errno = ENOMEM;
        errCause = "Failed to allocate memory for cropped image";
        return NULL;
    }

    // Set all pixels of cImg to correspondent ones of img
    for(int i = 0; i < w * h; i++) {
        int index = G(img, x + i % w, y + i / w);
        cImg->pixel[i] = img->pixel[index];
        if (cImg->pixel[i] > cImg->maxval){
            cImg->maxval = cImg->pixel[i];
        }
    }

    return cImg;
}


/// Operations on two images

/// Paste an image into a larger image.
/// Paste img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
void ImagePaste(Image img1, int x, int y, Image img2) { ///
    assert (img1 != NULL);
    assert (img2 != NULL);
    assert (ImageValidRect(img1, x, y, img2->width, img2->height));
    // Altered

    int w = img2->width;
    int h = img2->height;
    // Set all pixels of newImg to correspondent ones of img
    for(int i = 0; i < w * h; i++) {
        int index = G(img1, x + i % w, y + i / w);
        img1->pixel[index] = img2->pixel[i];
    }
}

/// Blend an image into a larger image.
/// Blend img2 into position (x, y) of img1.
/// This modifies img1 in-place: no allocation involved.
/// Requires: img2 must fit inside img1 at position (x, y).
/// alpha usually is in [0.0, 1.0], but values outside that interval
/// may provide interesting effects.  Over/underflows should saturate.
void ImageBlend(Image img1, int x, int y, Image img2, double alpha) { ///
    assert (img1 != NULL);
    assert (img2 != NULL);
    assert (ImageValidRect(img1, x, y, img2->width, img2->height));
    // Altered

    int w = img2->width;
    int h = img2->height;
    // Set all pixels of newImg to correspondent ones of img
    for(int i = 0; i < w * h; i++) {
        int index = G(img1, x + i % w, y + i / w); // RELATORIO

        double pixel1 = (double)img1->pixel[index];
        double pixel2 = (double)img2->pixel[i];

        double blendedValue = ((1 - alpha) * pixel1 + alpha * pixel2) + 0.5; // Adding 0.5 ensures it will round correctly when converting to uint8

        img1->pixel[index] = (uint8)blendedValue;
    }
}

/// Compare an image to a subimage of a larger image.
/// Returns 1 (true) if img2 matches subimage of img1 at pos (x, y).
/// Returns 0, otherwise.
int ImageMatchSubImage(Image img1, int x, int y, Image img2) { ///
    assert (img1 != NULL);
    assert (img2 != NULL);
    assert (ImageValidPos(img1, x, y));
    // Altered
    assert(img2->width <= img1->width - x);
    assert(img2->height <= img1->height - y);

    Image subImg = ImageCrop(img1, x, y, img2->width, img2->height);
    // uint8 *min, *max, *min2, *max2;
    // Alocar memória para min, max, min2 e max2
    // min = (uint8 *)malloc(sizeof(uint8));
    // max = (uint8 *)malloc(sizeof(uint8));
    // min2 = (uint8 *)malloc(sizeof(uint8));
    // max2 = (uint8 *)malloc(sizeof(uint8));

    // ImageStats(subImg, min, max);
    // ImageStats(img2, min2, max2);

    //if (*min == *min2 && *max == *max2){ // REFERIR
    for (int i = 0; i < img2->width * img2->height; i++){
        COMP++;
        if(img2->pixel[i] != subImg->pixel[i]){
            return 0;
        }
    }
    return 1;
    //}
    //return 0;
}

/// Locate a subimage inside another image.
/// Searches for img2 inside img1.
/// If a match is found, returns 1 and matching position is set in vars (*px, *py).
/// If no match is found, returns 0 and (*px, *py) are left untouched.
int ImageLocateSubImage(Image img1, int *px, int *py, Image img2) { ///
    assert (img1 != NULL);
    assert (img2 != NULL);
    // Altered

    COMP = 0;
    // Iterate through all possible positions in img1
    for (int y = 0; y <= img1->height - img2->height; y++) {
        for (int x = 0; x <= img1->width - img2->width; x++) {
            if (ImageMatchSubImage(img1, x, y, img2)) {
                px = &x;
                py = &y;
                return 1;
            }
        }
    }

    return 0;
}


/// Filtering

/// Blur an image by a applying a (2dx+1)x(2dy+1) mean filter.
/// Each pixel is substituted by the mean of the pixels in the rectangle
/// [x-dx, x+dx]x[y-dy, y+dy].
/// The image is changed in-place.
void ImageBlur(Image img, int dx, int dy) { ///
    // Altered
    assert(img != NULL);
    assert(dx >= 0 && dy >= 0);
    Image imgCopy = ImageCreate(img->width, img->height, img->maxval);
    for (int i = 0; i < img->width * img->height; i++){
        imgCopy->pixel[i] = img->pixel[i];
    }

    int count, index, indexCopy, nx, ny, mean;
    double sum;
    for (int y = 0; y < imgCopy->height; y++) {
        for (int x = 0; x < imgCopy->width; x++) {
            // Calculate the mean value of the surrounding pixels
            sum = 0.0;
            count = 0;

            for (int j = -dy; j <= dy; j++) {
                for (int i = -dx; i <= dx; i++) {
                    nx = x + i;
                    ny = y + j;

                    // Check if the coordinates are within the bounds of the image
                    if (nx >= 0 && nx < imgCopy->width && ny >= 0 && ny < imgCopy->height) {
                        indexCopy = G(imgCopy, nx, ny);
                        sum += imgCopy->pixel[indexCopy];
                        count++;
                    }
                }
            }

            // Calculate the mean and update the pixel value in the temporary image
            mean = count > 0 ? (int)(sum / count + 0.5) : 0;
            index = G(img, x, y);
            img->pixel[index] = (uint8)mean;
        }
    }
    ImageDestroy(&imgCopy);
}