/***********************************************
*      __                             ___ __   *
*     / /_  ____ ___  ____  ___  ____/ (_) /_  *
*    / __ \/ __ `__ \/ __ \/ _ \/ __  / / __/  *
*   / /_/ / / / / / / /_/ /  __/ /_/ / / /_    *
*  /_.___/_/ /_/ /_/ .___/\___/\__,_/_/\__/    *
*                 /_/                          *
***********************************************/
/* Header file for bmpedit. Contains:
    - structures to store bitmap header data
    - structure to store bitmap rgb values
    - function definitions
*/

/* Data to be read into structure is of exact fit, pragma disables padding */
#pragma pack(push, 1)

typedef struct {
    unsigned short int type;
    unsigned int size;
    unsigned short int reserved1, reserved2;
    unsigned int offset;
} BITMAPFILEHEADER;

typedef struct {
    unsigned int header_size;
    int width, height;
    unsigned short int colors_planes;
    unsigned short int bpp;
    unsigned int compression;
    unsigned int image_size;
    unsigned int h_res, v_res;
    unsigned int colors;
    unsigned int important_colors;
} BITMAPINFOHEADER;

// Structure used to store BGR values of pixels - reversed as little endian
typedef struct {
    unsigned char blue;
    unsigned char green;
    unsigned char red;
} pixel;

#pragma pack(pop)

// HSL Structure
typedef struct {
    float h;
    float s;
    float l;
} hsl_struct;

void read_headers(FILE *, BITMAPFILEHEADER *, BITMAPINFOHEADER *);
void write_bmp(FILE *, BITMAPFILEHEADER *, BITMAPINFOHEADER *, pixel *, long int pixel_total);

void rgb_to_hsl(hsl_struct *, float r, float g, float b);
void hsl_calc(hsl_struct *, double hue, double saturation, double lightness);
void hsl_to_rgb(hsl_struct *, pixel *);
void hsl_filter(double hue, double saturation, double lightness, BITMAPINFOHEADER *, pixel *, int padding);

void wb_filter(BITMAPINFOHEADER *, pixel *, int padding);
void gamma_filter(double gamma, BITMAPINFOHEADER *, pixel *, int padding);
void contrast_filter(double contrast, BITMAPINFOHEADER *, pixel *, int padding);
void greyscale_filter(BITMAPINFOHEADER *, pixel *, int padding);
void inverse_filter(BITMAPINFOHEADER *, pixel *, int padding);
void sepia_filter(BITMAPINFOHEADER *, pixel *, int padding);
void threshold_filter(double threshold, BITMAPINFOHEADER *, pixel *, int padding);
