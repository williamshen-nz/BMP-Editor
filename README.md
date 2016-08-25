# bmpedit - William Shen

## Summary
bmpedit is a command-line run C program which allows for the simple editing of BMP images through several image filters, and color space manipulation. This program is aimed at users who want to quickly and efficiently implement changes to their images without using a processor-intensive desktop image editing package, like GIMP or Photoshop.

bmpedit contains most of the features required for amateur and enthusiast photographers to manipulate their images, including automatic color balance, brightness, contrast, hue, saturation and gamma filters. The program can be run from the command-line after successful compilation, using `./bmpedit`. To see the usage message, simply apply the `-h` flag to the program (i.e. `./bmpedit -h`).

To store the BMP image, bmpedit uses different structures. The bitmap file header is stored in the `BITMAPFILEHEADER` structure, while the bitmap information header (DIB header) is stored in the `BITMAPINFOHEADER` structure. The RGB pixel data for the BMP is stored in a memory block of size `height * width * sizeof(pixel)` of `pixel` structure type, which has been allocated using `malloc`.  The use of structures means that data can easily be viewed, altered and accessed.

bmpedit uses a single `filter_flag` to allow multiple filters and options to be run at a time. Each filter is given a corresponding 'sum' value from the infinite series whose terms are the successive powers of two starting from 1 (i.e. 1, 2, 4, 8, 16 ...). After adding the corresponding 'sum' value to `filter_flag`, while parsing through the command-line arguments, bmpedit uses a bitwise operator (bitwise AND) to determine which filters should be executed. This means that only one variable is required to check for filters, and that new filters can easily be added to the system, by simply defining the next macro in the sequence, and checking `filter_flag` against this macro during runtime.

---

## Compilation
To compile bmpedit, use the `make` command in the terminal while in the main directory of the project. This will compile the program using the options set in the 'Makefile'.

Alternatively, you can choose your own compilation settings and compile bmpedit manually using GCC. Please note that bmpedit complies with the C99 standard, so use the `-std=c99` flag when compiling.

## Using bmpedit
To use bmpedit, simply run it through the terminal with `./bmpedit` (while in the directory of the project) and the relevant flags. For example, to shift the hue of `cup.bmp` by +20 degrees, convert the image to sepia and write the result as `sepia_cup.bmp`, simply run `./bmpedit -H 20 -s -o sepia_cup.bmp cup.bmp`.

The order in which the flags are input do not affect the order in which the filters are run. See the 'Limitations' section for more information. bmpedit will output detailed error messages if the program cannot continue running due to a fault in the file, or the arguments for the flags.

To see the usage message and all options available, simply apply the `-h` flag to the program (i.e. `./bmpedit -h`). Note that if the `-h` flag is run with filters, no image manipulation will occur as the program exits after the usage message is printed.

## Testing
Testing was completed manually, both during and after completion of the program, on a wide range of images including `cup.bmp`. These images included genres such as landscapes, cityscapes, architecture, animals, and sports; thus representing the sort of images that a user may input. BMP images of different widths, and heights, and ones with padding were also used to test the program.

Each filter was tested individually, and as combinations of filters to check for the validity of the image output. Attempts to break the program and cause it to act abnormally, such as inputting a non-BMP file with the `.bmp` extension, lead to several glitches being fixed.

To test for any leakages in memory and for the correct implementation of pointers and mallocs, valgrind was used with the parameters `valgrind --leak-check=full -v ./bmpedit [flags]`. The filters tested with valgrind ranged from one filter only, to all the filters to ensure program coherence.

---

## Additional Features
bmpedit contains several image editing and filtering options. These features allow users to quickly and efficiently manipulate images with little hassle or user interaction. The data for an input BMP image is stored in a structure for maintainability and for the ease of implementing new extensions or 'grabbing' data.

### Hue, Saturation and Lightness Filter
This filter allows users to correct and modify the hue (color tint), saturation (intensity of color) and lightness (brightness) of a BMP image. This is achieved by first converting each RGB pixel into HSL color space, applying the relevant filters, and then converting the updated HSL 'pixel' into RGB.

**Command Line Arguments:**  
Hue: `-H -360 to 360`  
Saturation: `-S 0 to 100`  
Lightness: `-L 0 to 100`

### Contrast Filter ###
The contrast filter can be used to increase or decrease the differences in color and tone in an image. This function is implemented by calculating the contrast correction coefficient and then applying an adjustment to each RGB pixel.

**Command Line Argument:** `-c -100 to 100`

### Automatic White Balance Filter ###
The color temperature of an image is very important in representing an image in the light of what the human eye would see it. The automatic white balance filter uses the grey world assumption in calculating a relatively accurate color temperature.

The grey world assumption argues that the mean reflectance of a given scene is achromatic, implying that the mean of all the RGB colors in this scene is a neutral grey. bmpedit implements this color correction filter by first calculating the mean of all the RGB pixels, then calculating relevant amplifiers (the gain) for the red and blue channels, and finally applying these amplifiers to the original pixels.

**Command Line Argument:** `-w`

### Gamma Correction Filter ###
Gamma can be used to maximise the visual quality of an image to a human eye and could also be described as the 'intensity' of the RGB pixels. Different computer displays and operating systems use separate methods to encode and decode gamma, so this filter may be used to optimise the image for viewing.

The gamma correction filter is implemented by first calculating the 'gain' to be applied to the input signal (using the user input gamma value), and then applying the 'gain' to the RGB pixels in the image.

**Command Line Argument:** `-y 0.01 to 7.99`

### Threshold Filter ###
Thresholding is used to set a pixel to either black or white if it is less than or greater than the user input threshold respectively. The threshold filter is implemented in bmpedit by calculating the mean RGB value of a pixel and then comparing it to the threshold, and setting the pixel to its relevant color (i.e. black or white).

**Command Line Argument:** `-t 0.0 to 1.0``

### Greyscale, Sepia and Inverse Filters ###
As their names describe, the greyscale, sepia and inverse filters are used to give an image a specific look. The greyscale filter is implemented by calculating the human-perceived brightness of each RGB pixel, while the sepia filter calculates each pixel with a much warmer tone. Finally, the inverse filter is implemented by adding 255 to take negative of the RGB channels of a pixel.

**Command Line Arguments:**  
Greyscale: `-g`  
Sepia: `-s`  
Inverse: `-i`  

---

## Limitations
1. Error handling
    * Currently, bmpedit individually checks for errors and outputs custom error messages.
    * Future improvements would feature a dedicated error handler with specific error codes.
2. Only 24 bit per pixel BMP images are accepted
3. Image filters can only be run in a specific sequence regardless of what order they are input into the command line.  
    **Order of precedence:**  
    1. Hue, Saturation and Lightness
    2. Contrast
    3. Automatic White Balance
    4. Gamma Correction
    5. Threshold
    6. Greyscale
    7. Sepia
    8. Inverse

*Note*: a workaround to limitation (3) is to run bmpedit multiple times in the order of the filters you wish to apply to the image.

---

## Credits
- RGB to HSL conversion algorithms are based off mathematical formulas on http://www.rapidtables.com/convert/color/rgb-to-hsl.htm
- HSL to RGB conversion algorithms are based off mathematical formulas on http://www.rapidtables.com/convert/color/hsl-to-rgb.htm
- Sepia tone algorithm based off 'Sepia Tone Filter' on https://code.msdn.microsoft.com/windowsapps/ColorMatrix-Image-Filters-f6ed20ae
