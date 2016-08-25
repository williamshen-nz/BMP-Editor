/***********************************************
*      __                             ___ __   *
*     / /_  ____ ___  ____  ___  ____/ (_) /_  *
*    / __ \/ __ `__ \/ __ \/ _ \/ __  / / __/  *
*   / /_/ / / / / / / /_/ /  __/ /_/ / / /_    *
*  /_.___/_/ /_/ /_/ .___/\___/\__,_/_/\__/    *
*                 /_/                          *
***********************************************/
/* bmpedit - a simple bmp manipulator
    by William Shen */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <math.h>
#include <unistd.h>

#include "bmpedit.h"

/* Macros for filter_flag */
#define FLAG_HSL 1
#define FLAG_CONTRAST 2
#define FLAG_WB 4
#define FLAG_GAMMA 8
#define FLAG_THRESHOLD 16
#define FLAG_GREYSCALE 32
#define FLAG_SEPIA 64
#define FLAG_INVERSE 128

#define ERROR_HEADER "bmpedit - error:"
#define FH_SIZE 14      // file header size
#define IH_SIZE 40      // info header size
#define PI 3.14159265359

/* Reads BMP headers into structure, checks and outputs relevant data */
void read_headers(FILE *fp, BITMAPFILEHEADER *file_header, BITMAPINFOHEADER *info_header) {
    /* Reading in header and checking for success */
    if (fread(file_header, FH_SIZE, 1, fp) != 1) {
        fprintf(stderr, "%s reading the bmp file header failed.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
    if (fread(info_header, IH_SIZE, 1, fp) != 1) {
        fprintf(stderr, "%s reading the bmp information header failed.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
    /* Checking BMP type is 'BM' and color depth is 24 bpp*/
    if ((*file_header).type != 0x4d42) {
        fprintf(stderr, "%s the input file is not in Windows BMP format.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    } else if ((*info_header).bpp != 24) {
        fprintf(stderr, "%s the color depth of the input file is not 24 bits per pixel.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
}

/* Writes BMP header and image data */
void write_bmp(FILE *fp, BITMAPFILEHEADER *file_header, BITMAPINFOHEADER *info_header, pixel *pixel_array, long int pixel_total) {
    fwrite(file_header, FH_SIZE, 1, fp);
    fwrite(info_header, IH_SIZE, 1, fp);
    fwrite(pixel_array, sizeof(pixel), pixel_total, fp);
}

/* RGB to HSL algorithm */
void rgb_to_hsl(hsl_struct *hsl, float r, float g, float b) {
    r /= 255.0, g /= 255.0, b /= 255.0;
    // Calculating maximum and minimum rgb values
    float max, min;
    max = r;
    if (g > max) max = g;
    if (b > max) max = b;
    min = r;
    if (g < min) min = g;
    if (b < min) min = b;

    // Setting and calculating HSL
    (*hsl).l = (max + min) / 2.0;
    float diff = max - min;
    if (diff == 0.0) {
        (*hsl).h = 0;
        (*hsl).s = 0;
    } else {
        (*hsl).s = (diff) / (1 - fabs(2 * (*hsl).l - 1));
        if (r > g && r > b) {
            (*hsl).h = fabs(60 * fmod((g - b) / diff, 6.0));
        } else if (g > b) {
            (*hsl).h = fabs(60 * (((b - r) / diff) + 2));
        } else {
            (*hsl).h = fabs(60 * (((r - g) / diff) + 4));
        }
    }
}

/* Calculates HSL values after filtering */
void hsl_calc(hsl_struct *hsl, double hue, double saturation, double lightness) {
    // h (hue) is measured between 0 and 360 degrees
    (*hsl).h += hue;
    if ((*hsl).h > 360) (*hsl).h = 360;
    if ((*hsl).h < 0) (*hsl).h = 0;
    // s (saturation) is measured between 0 and 1
    (*hsl).s += (saturation / 100.0);
    if ((*hsl).s > 1) (*hsl).s = 1;
    if ((*hsl).s < 0) (*hsl).s = 0;
    // l (lightness) is measured between 0 and 1
    (*hsl).l += (lightness / 200.0);
    if ((*hsl).l > 1) (*hsl).l = 1;
    if ((*hsl).l < 0) (*hsl).l = 0;
}

/* HSL to RGB algorithm */
void hsl_to_rgb(hsl_struct *hsl, pixel *data) {
    float r = 0.0, g = 0.0, b = 0.0;
    float c, x, m;
    c = (1 - fabs(2 * (*hsl).l - 1)) * (*hsl).s;
    x = c * (1 - fabs(fmod(((*hsl).h / 60), 2.0) -1));
    m = (*hsl).l - (c / 2);
    // Checking different cases for angles
    if ((*hsl).h >= 0 && (*hsl).h < 60) {
        r = c;
        g = x;
        b = 0;
    } else if ((*hsl).h >= 60 && (*hsl).h < 120) {
        r = x;
        g = c;
        b = 0;
    } else if ((*hsl).h >= 120 && (*hsl).h < 180) {
        r = 0;
        g = c;
        b = x;
    } else if ((*hsl).h >= 180 && (*hsl).h < 240) {
        r = 0;
        g = x;
        b = c;
    } else if ((*hsl).h >= 240 && (*hsl).h < 300) {
        r = x;
        g = 0;
        b = c;
    } else if ((*hsl).h >= 300 && (*hsl).h < 360) {
        r = c;
        g = 0;
        b = x;
    }
    r = (int)(255 * (r + m) + 0.5);
    g = (int)(255 * (g + m) + 0.5);
    b = (int)(255 * (b + m) + 0.5);
    // Setting to pixel array
    (*data) = (pixel) {b, g, r};
}

/* Hue, Saturation and Lightness filter. Uses HSL color space for extreme accuracy */
void hsl_filter(double hue, double saturation, double lightness, BITMAPINFOHEADER *header, pixel *data, int padding) {
    // Declare and initalise hsl_struct
    hsl_struct *hsl;
    hsl = (hsl_struct *) malloc (sizeof(hsl_struct));
    if (hsl == NULL) {
        fprintf(stderr, "%s memory allocation failed.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            // Converts RGB to HSl
            rgb_to_hsl(hsl, (*data).red, (*data).green, (*data).blue);
            // Calculates the HSL values with filters applied
            hsl_calc(hsl, hue, saturation, lightness);
            // Converts HSL to RGB and sets pixel array
            hsl_to_rgb(hsl, data);
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
    free(hsl);      // free the malloc
}

/* Contrast filter */
void contrast_filter(double contrast, BITMAPINFOHEADER *header, pixel *data, int padding) {
    // Calculate contrast coefficient
    contrast = 2.5 * contrast;
    double coeff = (259 * (contrast + 255))/(255 * (259 - contrast));
    // Loops through the whole pixel array
    double red, green, blue;
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            // Calculating Values and Rounding
            red = coeff * ((*data).red - 128) + 128;
            green = coeff * ((*data).green - 128) + 128;
            blue = coeff * ((*data).blue - 128) + 128;
            if (red > 255) red = 255;
            if (red < 0) red = 0;
            if (green > 255) green = 255;
            if (green < 0) green = 0;
            if (blue > 255) blue = 255;
            if (blue < 0) blue = 0;
            // Setting pixel value
            (*data) = (pixel) {(int)(blue + 0.5), (int)(green + 0.5), (int)(red + 0.5)};
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* Automatic white balance using the Gray World assumption */
void wb_filter(BITMAPINFOHEADER *header, pixel *data, int padding) {
    pixel *ptr = data;
    // Finding average RGB values for whole image
    float r_avg = 0, g_avg = 0, b_avg = 0;
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            r_avg += (*ptr).red;
            g_avg += (*ptr).green;
            b_avg += (*ptr).blue;
            ptr++;
        }
        ptr = (pixel*)((size_t)ptr + padding);    // accounting for padding
    }
    long int pixel_total = (*header).height * (*header).width;
    r_avg /= pixel_total, g_avg /= pixel_total, b_avg /= pixel_total;
    ptr = data;     // return to beggining of pointer
    // Applying calculations to pixel_array
    float r_gain = (g_avg / r_avg), b_gain = (g_avg / b_avg);
    data -= (pixel_total - (*header).height * padding);     // go back to start of pixel array
    for (int y = 0; y < (*header).height; y++) {
        for (int x = 0; x < (*header).width; x++) {
            float r, g, b;
            r = r_gain * (*ptr).red;
            g = (*ptr).green;
            b = b_gain * (*ptr).blue;
            if (r > 255) r = 255;
            if (b > 255) b = 255;
            (*ptr) = (pixel) {(int)(b + 0.5), (int)(g + 0.5), (int)(r + 0.5)};
            ptr++;
        }
        ptr = (pixel*)((size_t)ptr + padding);    // adding padding
    }
}

/* Gamma correction filter */
void gamma_filter(double gamma, BITMAPINFOHEADER *header, pixel *data, int padding) {
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            // Applying gamme correction formula
            float red, green, blue;
            red = powf((*data).red / 255.0, 1 / gamma) * 255;
            green = powf((*data).green / 255.0, 1 / gamma) * 255;
            blue = powf((*data).blue / 255.0, 1 / gamma) * 255;
            if (red > 255) red = 255;
            if (green > 255) green = 255;
            if (blue > 255) blue = 255;
            // Setting pixel data
            (*data) = (pixel) {(int)(blue + 0.5), (int)(green + 0.5), (int)(red + 0.5)};
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* Threshold filter. Blackens or whitens pixels accordingly */
void threshold_filter(double threshold, BITMAPINFOHEADER *header, pixel *data, int padding) {
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            // Checking pixel intensity against user input threshold
            if (((*data).red + (*data).green + (*data).blue) / (double)(255*3) < threshold) {
                (*data) = (pixel) {0, 0, 0};
            } else {
                (*data) = (pixel) {255, 255, 255};
            }
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* Greyscale filter. Converts image to black and white using luminance formula */
void greyscale_filter(BITMAPINFOHEADER *header, pixel *data, int padding) {
    double grey = 0;
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            grey = (*data).red * 0.2126 + (*data).green * 0.7152 + (*data).blue * 0.0722;
            (*data) = (pixel) {(int)(grey + 0.5), (int)(grey + 0.5), (int)(grey + 0.5)};       // typecasting and using pixel struct
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* Sepia filter. Gives the image a 'warm', yellow tone */
void sepia_filter(BITMAPINFOHEADER *header, pixel *data, int padding) {
    double red, green, blue;
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            // Calculations and rounding
            red = (*data).red * 0.393 + (*data).green * 0.769 + (*data).blue * 0.189;
            green = (*data).red * 0.349 + (*data).green * 0.686 + (*data).blue * 0.168;
            blue = (*data).red * 0.272 + (*data).green * 0.534 + (*data).blue * 0.131;
            if (red > 255) red = 255;
            if (green > 255) green = 255;
            if (blue > 255) blue = 255;
            // Setting rgb for pixel and typecasting
            (*data) = (pixel) {(int)(blue + 0.5), (int)(green + 0.5), (int)(red + 0.5)};
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* Inverse filter. Inverts the color of the pixel */
void inverse_filter(BITMAPINFOHEADER *header, pixel *data, int padding) {
    // Loops through the whole pixel array
    for (int i = 0; i < (*header).height; i++) {
        for (int j = 0; j < (*header).width; j++) {
            (*data) = (pixel) {255 - (*data).blue, 255 - (*data).green, 255 - (*data).red};
            data++;
        }
        data = (pixel*)((size_t)data + padding);    // adding padding
    }
}

/* bmpedit ascii logo */
static void logo(void) {
    printf(
        "***********************************************\n"
        "*     __                             ___ __   *\n"
        "*    / /_  ____ ___  ____  ___  ____/ (_) /_  *\n"
        "*   / __ \\/ __ `__ \\/ __ \\/ _ \\/ __  / / __/  *\n"
        "*  / /_/ / / / / / / /_/ /  __/ /_/ / / /_    *\n"
        "* /_.___/_/ /_/ /_/ .___/\\___/\\__,_/_/\\__/    *\n"
        "*                /_/                          *\n"
        "*                                             *\n"
        "***********************************************\n"
    );
}

/* Prints usage text */
static void usage(void) {
    printf(
        "\nUsage: bmpedit [OPTIONS...] [input.bmp]\n\n"
        "DESCRIPTION:\n"
        "   This program does simple edits of BMP image files. When the program runs it\n"
        "   first prints out the width and the height of the input image within the BMP\n"
        "   file. Once this is done if there is a filter (or sequence of filters) then\n"
        "   they are applied to the image. The resulting image is also stored using BMP\n"
        "   format into an output file. Without any filters only the width and height of\n"
        "   the image is output.\n\n"
        "NOTE:\n"
        "   0.0 will be assumed as the argument for a command if the argument begins\n"
        "   with a character.\n\n"
        "OPTIONS:\n"
        "   -h            Displays this usage message.\n"
        "   -o FILE       Sets the output file for modified images (default output file\n"
        "                 is \"out.bmp\").\n"
        "   -c            Apply a color tint filter to the image - the program will ask\n"
        "                 you for RGB tint values.\n"
        "   -g            Apply a greyscale (black and white) filter to the image.\n"
        "   -i            Apply a filter to invert the pixel colors on the image.\n"
        "   -s            Apply a sepia filter to the image (gives it a warmer tone).\n"
        "   -t 0.0-1.0    Apply a threshold filter to the image with the threshold as\n"
        "                 the value given.\n"
        "   -y 0.01-7.99  Apply a gamma correction filter to the image.\n"
        "   -w            Apply an automatic white balance filter to correct the color\n"
        "                 temperature of the image.\n"
        "   -H -360-360   Apply a hue (color) shift to the image.\n"
        "   -S -100-100   Increase or decrease saturation of the image.\n"
        "   -L -100-100   Increase or decrease the lightness (similar to brightness) of \n"
        "                 the image.\n\n"
    );
}

int main(int argc, char *argv[]) {
    /* Initializing variables */
    char *ptr, *input_file, *output_file = "out.bmp";
    double hue = 0, saturation = 0, lightness = 0;
    double contrast = 0, gamma = 0, threshold = 0;
    int filter_flag = 0;    // filter flag adds values from macros to consider all possibilties
    int hsl_flag = 0;   // flag to check if H, S or L filter has already been selected
    /* Initializing Structs */
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;
    pixel *pixel_array;

    /* If user passes no options */
    if (argc == 1) {
      fprintf(stderr, "%s no valid arguments have been entered.\n"
                      "Enter ./bmpedit -h to view usage message.\n", ERROR_HEADER);
      exit(EXIT_FAILURE);
    }

    /* Checking command line options */
    int c;
    while ((c = getopt(argc, argv, "c:ghio:st:wy:H:S:L:")) != -1) {
        switch (c) {
            case 'c':
                contrast = strtod(optarg, &ptr);
                if (contrast < -100 || contrast > 100) {
                    fprintf(stderr, "%s the contrast must be between -100 and 100, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    filter_flag += FLAG_CONTRAST;
                }
                break;
            case 'g':
                filter_flag += FLAG_GREYSCALE;
                break;
            case 'h':
                logo();
                usage();
                exit(EXIT_SUCCESS);
            case 'i':
                filter_flag += FLAG_INVERSE;
                break;
            case 'o':
                if ((strstr(optarg, ".bmp") != NULL) || (strstr(optarg, ".BMP") != NULL)) {   // simple check for .bmp or .BMP
                    output_file = optarg;
                } else {
                    fprintf(stderr, "%s the output file must be in .bmp or .BMP format.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                }
                break;
            case 's':
                filter_flag += FLAG_SEPIA;
                break;
            case 't':
                threshold = strtod(optarg, &ptr);
                if (threshold < 0.0 || threshold > 1.0) {
                    fprintf(stderr, "%s the threshold must be between 0.0 and 1.0, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    filter_flag += FLAG_THRESHOLD;
                }
                break;
            case 'y':
                gamma = strtod(optarg, &ptr);
                if (gamma < 0.01 || gamma > 7.99) {
                    fprintf(stderr, "%s the gamma value must be between 0.01 and 7.99, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    filter_flag += FLAG_GAMMA;
                }
                break;
            case 'w':
                filter_flag += FLAG_WB;
                break;
            case 'H':
                hue = strtod(optarg, &ptr);
                if (hue < -360 || hue > 360) {
                    fprintf(stderr, "%s the hue shift must be between -360 and 360, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    if (hsl_flag == 0) filter_flag += FLAG_HSL;
                    hsl_flag = 1;
                }
                break;
            case 'S':
                saturation = strtod(optarg, &ptr);
                if (saturation < -100 || saturation > 100) {
                    fprintf(stderr, "%s the saturation must be between -100 and 100, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    if (hsl_flag == 0) filter_flag += FLAG_HSL;
                    hsl_flag = 1;
                }
                break;
            case 'L':
                lightness = strtod(optarg, &ptr);
                if (lightness < -100 || lightness > 100) {
                    fprintf(stderr, "%s the lightness must be between -100 and 100, inclusive.\n", ERROR_HEADER);
                    exit(EXIT_FAILURE);
                } else {
                    if (hsl_flag == 0) filter_flag += FLAG_HSL;
                    hsl_flag = 1;
                }
                break;
            default:
                fprintf(stderr, "%s no valid arguments have been entered.\n"
                                "Enter ./bmpedit -h to view usage message.\n", ERROR_HEADER);
                exit(EXIT_FAILURE);
        }
    }

    /* Check for extra argument (i.e. input file) and performs simple validility test */
    if (argc - optind == 1) {
       input_file = *(argv + optind);       // get data from the address containing argument
       /* Program stops if it is not readable or not in .bmp or .BMP */
       if (!((strstr(input_file, ".bmp") != NULL) || (strstr(input_file, ".BMP") != NULL))) {   // simple check for NOT .bmp or .BMP
           fprintf(stderr, "%s input file is not in .bmp or .BMP format.\n", ERROR_HEADER);
           exit(EXIT_FAILURE);
       }
       if (access(input_file, R_OK) != 0) {
           fprintf(stderr, "%s input file either does not exist or is not readable.\n", ERROR_HEADER);
           exit(EXIT_FAILURE);
       }
    } else {
        fprintf(stderr, "%s no input file has been set.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }

    /* Open file for reading, read headers into structure */
    FILE * input = fopen(input_file, "r");
    read_headers(input, &file_header, &info_header);
    printf("Image width: %dpx\nImage height: %dpx\n", info_header.width, info_header.height);

    /* Exits program if no filters selected */
    if (filter_flag == 0) {
        printf("bmpedit: Success!\n");
        exit(EXIT_SUCCESS);
    }

    /* Checks for padding */
    int padding_bytes = 0;
    if ((info_header.width * 3) % 4 != 0) {
        padding_bytes = 4 - ((info_header.width * 3) % 4);
        printf("Padding Bytes: %d\n", padding_bytes);
    }

    /* Allocates memory and reads in pixel data */
    long int pixel_total = info_header.width * info_header.height;
    pixel_array = (pixel *) malloc (sizeof(pixel) * pixel_total);
    if (pixel_array == NULL) {
        fprintf(stderr, "%s memory allocation failed.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
    if (fread(pixel_array, sizeof(pixel), pixel_total, input) != pixel_total) {
        fprintf(stderr, "%s reading the image data failed.\n", ERROR_HEADER);
        exit(EXIT_FAILURE);
    }
    fclose(input);

    /* Checking and running filters */
    if (filter_flag & FLAG_HSL) {               // filter_flag & 1
        hsl_filter(hue, saturation, lightness, &info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_CONTRAST) {          // filter_flag & 2
        contrast_filter(contrast, &info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_WB) {                // filter_flag & 4
        wb_filter(&info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_GAMMA) {             // filter_flag & 8
        gamma_filter(gamma, &info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_THRESHOLD) {         // filter_flag & 16
        threshold_filter(threshold, &info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_GREYSCALE) {         // filter_flag & 32
        greyscale_filter(&info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_SEPIA) {             // filter_flag & 64
        sepia_filter(&info_header, pixel_array, padding_bytes);
    }
    if (filter_flag & FLAG_INVERSE) {           // filter_flag & 128
        inverse_filter(&info_header, pixel_array, padding_bytes);
    }

    /* Write bmp image, close file and free malloc */
    FILE * output = fopen(output_file, "w");
    write_bmp(output, &file_header, &info_header, pixel_array, pixel_total);
    fclose(output);
    free(pixel_array);

    printf("bmpedit: Success!\n");
    return EXIT_SUCCESS;
}
