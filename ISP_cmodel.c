


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

//
// Created by qin on 2022/7/22.
//

#define BASEUNIT_HALF(x,y)    ((y)==0 ? 0 : ((x)<<((y)-1)))
#define UROUND_BITWISE(x,y)   (((x)+BASEUNIT_HALF(1,(y)))>>(y))
#define ABS(x)                ( (x) < 0 ? -(x) : (x) )
#define MIN(x,y)              ( ((x) < (y)) ? (x) : (y) )
#define MIN3(a,b,c)           (MIN(a, MIN(b, c)))
#define MAX(x,y)              ( ((x) < (y)) ? (y) : (x) )
#define MINMAX(x,min,max)     ( MIN(MAX((x),(min)),(max)))
#define MIRROR(x, width)      ( ((x) < 0)? (-(x)):(((x) > (width)-1)? (2*(width)-2 -(x)):(x)) )
#define PIXEL(frame,x,y,ch) (*(frame->data + (MIRROR((y), frame->height) * frame->width + MIRROR((x),frame->width)) * frame->channel + (ch)))
typedef enum FRAME_TYPE {
    TOP_FIELD = 0,
    BOT_FIELD = 1,
    PROGRESSIVE = 2,
    RAW = 3,
} FRAME_TYPE;
typedef enum COLOR_FORMAT {
    RGB = 0,
    YCBCR = 1,
    GRAY = 2,
    RAW_RGB = 3,
    RAW_RGBIR = 4
} COLOR_FORMAT;
typedef enum BAYER_RGB {
    BAYER_BG = 0,
    BAYER_GB = 1,
    BAYER_GR = 2,
    BAYER_RG = 3
} BAYER_RGB;
typedef struct FRAME{
    FRAME_TYPE type;
    COLOR_FORMAT format;
    int index;
    int width;
    int height;
    int channel;
    int depth;
    BAYER_RGB bayer_id;
    int *data;
}FRAME;


//--------------------------------------------------------------------------------------
FRAME *CM_NewFRAME(FRAME_TYPE type, COLOR_FORMAT format, int width, int height, int channel, int depth)
{
    FRAME *img;
    //alloc memory form image
    img =(FRAME *)malloc(sizeof(FRAME));
    if (!img) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    //memory allocation for pixel data
    //printf("Malloc width: %d height:%d channel:%d\n", width, height, channel);
    img->type   = type;
    img->format = format;
    img->width  = width;
    img->height = height;
    img->depth  = depth;
    img->channel = channel;

    img->data   = (int *)malloc(width * height* channel * sizeof(int));
    if (!img->data) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    return img;
}


//
FRAME *CM_ReadRAW(const char *filename, COLOR_FORMAT format, int fidx, int hdr_mode, int width, int height, int depth)
{
    FRAME *img;
    FILE  *fp;

    //open RAW file for reading
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    img = CM_NewFRAME(RAW, format, width, height, 1, depth);

    // seek the file offset
    int hdr = (hdr_mode!=0)? 1:0;
    int se  = (hdr_mode==2)? 1:0;
    long offset = ((hdr+1)*width*height*fidx + se*width)*sizeof(unsigned short);

    int x, y, byte0, byte1;
    for(y=0; y<height; y++)  {
        fseek(fp, offset, SEEK_SET);
        for(x=0; x<width; x++)   {
            byte0 = fgetc(fp);
            byte1 = fgetc(fp);
            PIXEL(img, x, y, 0) =  (byte1<<8) + byte0 ;
        }
        offset += (hdr+1)*width*sizeof(unsigned short);
    }

    fclose(fp);
    return img;
}

// raw 2 ppm5
//--------------------------------------------------------------------------------------
int CM_RAW2PPM5(const char *filename, FRAME *raw, int target_depth)
{
    FILE *fp;
    int x, y;

    if(raw->channel != 1)   {
//        log_fatal("(RAW2PPM5) Color channel > 1");
        exit(1);
    }

    //open PPM file for writting
    fp = fopen(filename, "wb");
    if (!fp) {
//        log_fatal("(RAW2PPM5) Unable to open file '%s'", filename);
        exit(1);
    }

    //write the header file
    //image format
    fprintf(fp, "P5\n");

    //comments
    fprintf(fp, "# Created by CVITEK DPU Model\n");

    //image size
    fprintf(fp, "%d %d\n",raw->width,raw->height);

    // rgb component depth
    int max_val = (1<<target_depth)-1;
    fprintf(fp, "%d\n", max_val);

    int p0;
    for(y=0; y<raw->height; y++)
        for(x=0; x<raw->width; x++) {
            p0 = MINMAX(PIXEL(raw, x, y, 0) >> (raw->depth - target_depth), 0, max_val);
            if (target_depth > 24) fputc((unsigned char) ((p0 & 0xFF000000) >> 24), fp);
            if (target_depth > 16) fputc((unsigned char) ((p0 & 0xFF0000) >> 16), fp);
            if (target_depth > 8) fputc((unsigned char) ((p0 & 0xFF00) >> 8), fp);
            fputc((unsigned char) (p0 & 0xFF), fp);
        }
    fclose(fp);
    return 0;
}

//--------------------------------------------------------------------------------------
// ReadPPM: read a GRAY PPM into a frame
//--------------------------------------------------------------------------------------
FRAME *CM_ReadPPM5(const char *filename, FRAME_TYPE type)
{
    char  buff[16], c;
    FRAME *img;
    FILE  *fp;
    int width, height, max_rgb;

    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file  '%s'\n", filename);
        exit(1);
    }

    //read image format
    if (!fgets(buff, sizeof(buff), fp)) {
        perror(filename);
        exit(1);
    }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '5') {
            fprintf(stderr, "Invalid image format (must be 'P5') (error loading '%s')\n", filename);
            exit(1);
        }

        //check for comments
        c = getc(fp);
        while (c == '#') {
            while (getc(fp) != '\n') ;
            c = getc(fp);
        }

        ungetc(c, fp);
        //read image size information
        if (fscanf(fp, "%d %d", &width, &height) != 2) {
            fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
            exit(1);
        }

        //read max_rgb
        if (fscanf(fp, "%d", &max_rgb) != 1) {
            fprintf(stderr, "Invalid max_rgb (error loading '%s')\n", filename);
            exit(1);
        }
        while (fgetc(fp) != '\n') ;

        //check max_rgb depth
        int depth = (int) ceil(log2((double) max_rgb));
//        log_debug("Color depth (%d) max_rgb:(%d)\n", depth, max_rgb);

        //new frame
        int channel = 1;
        img = (FRAME *) CM_NewFRAME(type,GRAY, width,height,channel,depth);
        if (!img) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }

        //read pixel data from file
        int x, y;
        int byte0, byte1, byte2, byte3;
        for(y=0; y<height; y++)
            for(x=0; x<width; x++)
                for(c=0; c<1; c++)
                {
                    byte0 = byte1 = byte2 = byte3 = 0;
                    if(depth>24)   byte3 = fgetc(fp);
                    if(depth>16)   byte2 = fgetc(fp);
                    if(depth>8)    byte1 = fgetc(fp);
                    if(depth>0)    byte0 = fgetc(fp);
                    PIXEL(img,x,y,c) = (int)((byte3<<24)+(byte2<<16)+(byte1<<8)+(byte0));
                    if(feof(fp))      break;
                }
        //printf("SUCCESS\n");
        fclose(fp);

        return img;
}

//--------------------------------------------------------------------------------------
// ReadPPM: read a RGB PPM into a frame
//--------------------------------------------------------------------------------------
FRAME *CM_ReadPPM(const char *filename, FRAME_TYPE type) {
    char buff[16], c;
    FRAME *img;
    FILE *fp;
    int width, height, max_rgb;

    // open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    // read image format
    if (!fgets(buff, sizeof(buff), fp)) {
        perror(filename);
        exit(1);
    }

    // check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
        fprintf(stderr, "Invalid image format (must be 'P6') (error loading '%s')\n", filename);
        exit(1);
    }// check for comments
    c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n') ;
        c = getc(fp);
    }

    ungetc(c, fp);
    // read image size information
    if (fscanf(fp, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        exit(1);
    }

    // read max_rgb
    if (fscanf(fp, "%d", &max_rgb) != 1) {
        fprintf(stderr, "Invalid max_rgb (error loading '%s')\n", filename);
        exit(1);
    }
    while (fgetc(fp) != '\n') ;

    // check max_rgb depth
    int depth = (int) ceil(log2((double) max_rgb));
//    log_debug("Color depth (%d) max_rgb:(%d)", depth, max_rgb);
// new frame
    int channel = 3;
    img = CM_NewFRAME(type,RGB, width, height, channel, depth);
    if (!img) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

        int x, y;
int byte0 = 0;
int byte1 = 0;
int byte2 = 0;
int byte3 = 0;
for(y=0; y<height; y++) {
for(x=0; x<width; x++) {
for(c=0; c<3; c++) {
if(depth > 24) {byte3 = fgetc(fp);}
if(depth > 16) {byte2 = fgetc(fp);}
if(depth > 8) {byte1 = fgetc(fp);}
if(depth > 0) {byte0 = fgetc(fp);}
PIXEL(img,x,y,c) = (int)((byte3<<24)+(byte2<<16)+(byte1<<8)+(byte0)) & max_rgb;
if(feof(fp)) break;
}
}
}
fclose(fp);

return img;
}

//--------------------------------------------------------------------------------------
// WritePPM: write a PPM into a frame
//--------------------------------------------------------------------------------------
int CM_WritePPM(const char *filename,FRAME *img)
{
    FILE *fp;
    //open file for output
    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    //write the header file
    //image format
    fprintf(fp, "P6\n");

    //comments
    fprintf(fp, "# Created by CVITEK DPU Model\n");

    //image size
    fprintf(fp, "%d %d\n",img->width,img->height);

    // rgb component depth
    fprintf(fp, "%d\n", (1<<img->depth)-1);

    // pixel data
    int x, y;
    for(y=0; y < img->height; y++)
        for(x=0; x < img->width; x++)
        {
            int iR, iG, iB;
            if(img->channel == 3)
            {
                iR = PIXEL(img, x, y, 0);
                iG = PIXEL(img, x, y, 1);
                iB = PIXEL(img, x, y, 2);
            } else
            {
                iR = PIXEL(img, x, y, 0);
                iG = PIXEL(img, x, y, 0);
                iB = PIXEL(img, x, y, 0);
            }
            unsigned char c;
            if(img->depth > 24){
                c = (iR & 0xFF000000)>>24;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0xFF000000)>>24;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0xFF000000)>>24;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else if(img->depth > 16)
            {
                c = (iR & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else if( img->depth > 8 )
            {
                c = (iR & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iR & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else
            {
                c = (iR & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
                c = (iB & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
        }
    fclose(fp);

    return 0;
}

// Bayer ID
typedef enum {
    BAYER_ID_BG = 0,
    BAYER_ID_GB = 1,
    BAYER_ID_GR = 2,
    BAYER_ID_RG = 3,
    BAYER_ID_RGB = 4,
    BAYER_ID_YUV = 5,
    BAYER_ID_IR  = 6,
    BAYER_ID_DUMMY = 7,
    IRBAYER_ID_GRGBI = 8,  // Convert to "RGGB"
    IRBAYER_ID_RGBGI = 9,  // Convert to "RGGB"
    IRBAYER_ID_GBGRI = 10, // Convert to BGGR
    IRBAYER_ID_BGRGI = 11, // Convert to BGGR
    IRBAYER_ID_IGRGB = 12, // Convert to "RGGB"
    IRBAYER_ID_IRGBG = 13, // Convert to "RGGB"
    IRBAYER_ID_IBGRG = 14, // Convert to BGGR
    IRBAYER_ID_IGBGR = 15  // Convert to BBAYER_ID_BGGGR
} BAYER_ID;

//--------------------------------------------------------------------------------------
// Get color ID
//--------------------------------------------------------------------------------------
int CM_GET_COLOR_ID(int row, int col, int id) {


    BAYER_ID ColorPattern[16][16] = {
            {BAYER_ID_BG, BAYER_ID_GB, BAYER_ID_GR, BAYER_ID_RG}, // id = 0
            {BAYER_ID_GB, BAYER_ID_BG, BAYER_ID_RG, BAYER_ID_GR}, // id = 1
            {BAYER_ID_GR, BAYER_ID_RG, BAYER_ID_BG, BAYER_ID_GB}, // id = 2
            {BAYER_ID_RG, BAYER_ID_GR, BAYER_ID_GB, BAYER_ID_BG}, // id = 3

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY},
            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY},
            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY},
            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY},

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 8

            {BAYER_ID_RG, BAYER_ID_GR, BAYER_ID_BG, BAYER_ID_GR,
             BAYER_ID_GR, BAYER_ID_IR, BAYER_ID_GR, BAYER_ID_IR,
             BAYER_ID_BG, BAYER_ID_GR, BAYER_ID_RG, BAYER_ID_GR,
             BAYER_ID_GR, BAYER_ID_IR, BAYER_ID_GR, BAYER_ID_IR}, // id = 9

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 10

            {BAYER_ID_BG, BAYER_ID_GR, BAYER_ID_RG, BAYER_ID_GR,
             BAYER_ID_GR, BAYER_ID_IR, BAYER_ID_GR, BAYER_ID_IR,
             BAYER_ID_RG, BAYER_ID_GR, BAYER_ID_BG, BAYER_ID_GR,
             BAYER_ID_GR, BAYER_ID_IR, BAYER_ID_GR, BAYER_ID_IR}, // id = 11

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 12

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 13

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 14

            {BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY,
             BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY, BAYER_ID_DUMMY}, // id = 15
    };

    if (id < 4) {

        return
                ((((row & 0x01) == 0) && ((col & 0x01) == 0)) ? (ColorPattern[id][0]) :
                 ((((row & 0x01) == 0) && ((col & 0x01) == 1)) ? (ColorPattern[id][1]) :
                  ((((row & 0x01) == 1) && ((col & 0x01) == 0)) ? (ColorPattern[id][2]) :
                   ((ColorPattern[id][3])))));
    }
    else {

        return
                ((((row & 0x03) == 0) && ((col & 0x03) == 0)) ? (ColorPattern[id][0]) :
                 ((((row & 0x03) == 0) && ((col & 0x03) == 1)) ? (ColorPattern[id][1]) :
                  ((((row & 0x03) == 0) && ((col & 0x03) == 2)) ? (ColorPattern[id][2]) :
                   ((((row & 0x03) == 0) && ((col & 0x03) == 3)) ? (ColorPattern[id][3]) :

                    ((((row & 0x03) == 1) && ((col & 0x03) == 0)) ? (ColorPattern[id][4]) :
                     ((((row & 0x03) == 1) && ((col & 0x03) == 1)) ? (ColorPattern[id][5]) :
                      ((((row & 0x03) == 1) && ((col & 0x03) == 2)) ? (ColorPattern[id][6]) :
                       ((((row & 0x03) == 1) && ((col & 0x03) == 3)) ? (ColorPattern[id][7]) :

                        ((((row & 0x03) == 2) && ((col & 0x03) == 0)) ? (ColorPattern[id][8]) :
                         ((((row & 0x03) == 2) && ((col & 0x03) == 1)) ? (ColorPattern[id][9]) :
                          ((((row & 0x03) == 2) && ((col & 0x03) == 2)) ? (ColorPattern[id][10]) :
                           ((((row & 0x03) == 2) && ((col & 0x03) == 3)) ? (ColorPattern[id][11]) :

                            ((((row & 0x03) == 3) && ((col & 0x03) == 0)) ? (ColorPattern[id][12]) :
                             ((((row & 0x03) == 3) && ((col & 0x03) == 1)) ? (ColorPattern[id][13]) :
                              ((((row & 0x03) == 3) && ((col & 0x03) == 2)) ? (ColorPattern[id][14]) :
                               ((ColorPattern[id][15])))))))))))))))));
    }
}
int WB_R = 10;

int CM_WritePPM5(const char *filename, FRAME *img)
{
    FILE *fp;
    //open file for output
    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    fprintf(fp, "P5\n");

    //comments
    fprintf(fp, "# Created by CVITEK DPU Model\n");

    //image size
    fprintf(fp, "%d %d\n",img->width,img->height);
    unsigned int max_value; // supports up to 2**32 - 1
    if (img->depth >= 32) {
        max_value = 65536;
    } else {
        max_value = (1<<img->depth)-1;
    }
    fprintf(fp, "%u\n", max_value);

    // pixel data
    int x, y;
    for(y=0; y < img->height; y++)
        for(x=0; x < img->width; x++)
        {
            int iG;
            iG = PIXEL(img, x, y, 0);

            unsigned char c;
            if(img->depth > 24)
            {
                c = (iG & 0xFF000000)>>24;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else if(img->depth > 16)
            {c = (iG & 0x00FF0000)>>16;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else if( img->depth > 8 )
            {
                c = (iG & 0x0000FF00)>>8 ;     fwrite(&c, 1, 1, fp);
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
            else
            {
                c = (iG & 0x000000FF)>>0 ;     fwrite(&c, 1, 1, fp);
            }
        }
    fclose(fp);

    return 0;
}


void wbg_hw(FRAME *in0, FRAME *out0) {
    uint32_t IRGain = (1 << WB_R);
    uint32_t x, y;
    uint32_t reg_img_bayerid = 3;
    uint32_t IRBayer_ID = (in0->format == RAW_RGBIR) ? (reg_img_bayerid + 8) : reg_img_bayerid;

    uint32_t reg_wbg_rgain = 3;
    uint32_t reg_wbg_ggain = 1024;
    uint32_t reg_wbg_bgain = 1024;
    uint32_t reg_wbg_enable = 0;

    uint32_t pix_mul_gain = 0;
    uint32_t pix_mul_gain_shft_pre = 0;
    uint32_t pix_mul_gain_shft = 0;
    uint32_t Gain = 0;


    for (y = 0; y < in0->height; y++) {
        for (x = 0; x < in0->width; x++) {
            BAYER_RGB eColorID = (BAYER_RGB) CM_GET_COLOR_ID(y, x, IRBayer_ID);
            if (in0->format != RAW_RGBIR) {  // 走单通道
                switch (eColorID) {
                    case BAYER_BG:
                        Gain = reg_wbg_bgain;
                        break;
                    case BAYER_GB:
                    case BAYER_GR:
                        Gain = reg_wbg_ggain;
                        break;
                    case BAYER_RG:
                        Gain = reg_wbg_rgain;
                        break;
                }
            } else {
                BAYER_RGB Bayer_ID = (BAYER_RGB) CM_GET_COLOR_ID(0, 0, IRBayer_ID); // Convert IRBayerID to BayerID

                switch (Bayer_ID) {

                    case BAYER_BG: // Input Pattern is BGGR
                        switch (eColorID) {
                            case BAYER_BG:
                                if ((y % 4 == 0 && x % 4 == 0) || (y % 4 == 2 && x % 4 == 2)) { // BGain
                                    Gain = reg_wbg_bgain;
                                } else { // RGain
                                    Gain = reg_wbg_rgain;
                                }
                                break;
                            case BAYER_RG: // IR
                                Gain = IRGain;
                                break;
                            case BAYER_GB:
                                Gain = reg_wbg_ggain;
                                break;
                        }
                        break;

                    case BAYER_RG: // Input Pattern is RGGB
                        switch (eColorID) {
                            case BAYER_RG:
                                if ((y % 4 == 0 && x % 4 == 0) || (y % 4 == 2 && x % 4 == 2)) { // RGain
                                    Gain = reg_wbg_rgain;
                                } else { // BGain
                                    Gain = reg_wbg_bgain;
                                }
                                break;
                            case BAYER_BG: // IR
                                Gain = IRGain;
                                break;
                            case BAYER_GB:
                            case BAYER_GR:
                                Gain = reg_wbg_ggain;
                                break;
                        }
                        break;
                }
            }
            pix_mul_gain = PIXEL(in0, x, y, 0) * Gain;
            //pix_mul_gain_shft_pre = pix_mul_gain + WB_BASE_MAXD2;
            pix_mul_gain_shft = UROUND_BITWISE(pix_mul_gain, WB_R);
            //uint32_t test_rounding = pix_mul_gain_shft_pre >>
            PIXEL(out0, x, y, 0) = MINMAX(pix_mul_gain_shft, 0, ((1 << in0->depth) - 1));
        }
    }
}

void customize_wbg(FRAME *in0, FRAME *out0) {
    int x, y;
    long G_gain = 0l, B_gain = 0l, R_gain = 0l;
    uint32_t IRGain = (1 << WB_R);

    uint32_t reg_img_bayerid = 3;
    uint32_t IRBayer_ID = (in0->format == RAW_RGBIR) ? (reg_img_bayerid + 8) : reg_img_bayerid;
    for (y = 0; y < in0->height; y++) {
        for (x = 0; x < in0->width; x++) {
            BAYER_RGB eColorID = (BAYER_RGB) CM_GET_COLOR_ID(y, x, IRBayer_ID);
            if (eColorID == BAYER_GB || eColorID == BAYER_GR)
                G_gain += PIXEL(in0, x, y, 0) * 80 / 100 ;
            else if (eColorID == BAYER_RG)
                R_gain += PIXEL(in0, x, y, 0);
            else
                B_gain += PIXEL(in0, x, y, 0);
        }
    }
    for (y = 0; y < in0->height; y++) {
        for (x = 0; x < in0->width; x++) {
            BAYER_RGB eColorID = (BAYER_RGB) CM_GET_COLOR_ID(y, x, IRBayer_ID);

            if (eColorID == BAYER_RG)
                PIXEL(out0,x,y,0) = MINMAX(PIXEL(in0,x,y,0) * 98 / 100  * (G_gain / R_gain),0,4095) ;
            else if(eColorID == BAYER_BG)
                PIXEL(out0,x,y,0) = MINMAX(PIXEL(in0,x,y,0) * 98 / 100  * (G_gain / B_gain),0,4095);
            else
                PIXEL(out0,x,y,0) = PIXEL(in0,x,y,0);
        }
    }

}


//--------------------------------------------------------------------------------------
// ReadPPM: read a GRAY PPM into a frame
//--------------------------------------------------------------------------------------
FRAME *CM_ReadPPM6(const char *filename, FRAME_TYPE type)
{
    char  buff[16], c;
    FRAME *img;
    FILE  *fp;
    int width, height, max_rgb;

    //open PPM file for reading
    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Unable to open file  '%s'\n", filename);
        exit(1);
    }

    //read image format
    if (!fgets(buff, sizeof(buff), fp)) {
        perror(filename);
        exit(1);
    }

    //check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
        fprintf(stderr, "Invalid image format (must be 'P6') (error loading '%s')\n", filename);
        exit(1);
    }

    //check for comments
    c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n') ;
        c = getc(fp);
    }

    ungetc(c, fp);
    //read image size information
    if (fscanf(fp, "%d %d", &width, &height) != 2) {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        exit(1);
    }

    //read max_rgb
    if (fscanf(fp, "%d", &max_rgb) != 1) {
        fprintf(stderr, "Invalid max_rgb (error loading '%s')\n", filename);
        exit(1);
    }
    while (fgetc(fp) != '\n') ;

    //check max_rgb depth
    int depth = (int) ceil(log2((double) max_rgb));
//        log_debug("Color depth (%d) max_rgb:(%d)\n", depth, max_rgb);

    //new frame
    int channel = 3;
    img = (FRAME *) CM_NewFRAME(type,GRAY, width,height,channel,depth);
    if (!img) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    //read pixel data from file
    int x, y;
    int byte0, byte1, byte2, byte3;
    for(y=0; y<height; y++)
        for(x=0; x<width; x++)
            for(c=0; c<3; c++)
            {
                byte0 = byte1 = byte2 = byte3 = 0;
                if(depth>24)   byte3 = fgetc(fp);
                if(depth>16)   byte2 = fgetc(fp);
                if(depth>8)    byte1 = fgetc(fp);
                if(depth>0)    byte0 = fgetc(fp);
                PIXEL(img,x,y,c) = (int)((byte3<<24)+(byte2<<16)+(byte1<<8)+(byte0));
                if(feof(fp))      break;
            }
    //printf("SUCCESS\n");
    fclose(fp);

    return img;
}

void CM_PPM62PPM5(FRAME *src, FRAME *dst) {
    int width, height, i, j, k;

    for (i = 0; i < src->height; i += 2)
        for (j = 0; j < src->width; j += 2) {

            PIXEL(dst, j, i, 0) = PIXEL(src, j, i, 0);
            PIXEL(dst, j + 1, i, 0) = PIXEL(src, j + 1, i, 1);
            PIXEL(dst, j, i + 1, 0) = PIXEL(src, j, i + 1, 1);
            PIXEL(dst, j + 1, i + 1, 0) = PIXEL(src, j + 1, i + 1, 2);
        }
}
int add(int x,int y){
    return x + y;
}

void crop_raw(int* raw,int h_s,int h_e,int w_s,int w_e,char* dst_path){
    FRAME *dst;
    int i,j;
    dst = CM_NewFRAME(RAW, GRAY, w_e - w_s, h_e - h_s, 1, 12);
    for(i=h_s;i<h_e;i++)
        for(j=w_s;j<w_e;j++){
            PIXEL(dst,j-w_s,i-h_s,0) = raw[i*2560 + j];
        }

    CM_WritePPM5(dst_path, dst);
}
//int main(int argc, char *argv[]) {
//
//    FRAME *src,*dst;
////    src = CM_ReadRAW()
//    dst = CM_NewFRAME(RAW, GRAY, 1920, 1080, 1, 12);
//    src = CM_ReadPPM6("E:\\cfa_out_le_000.ppm",PROGRESSIVE);
//    CM_PPM62PPM5(src,dst);
//    CM_WritePPM5("E:\\cfa_mosaic_000.ppm", dst);
//    return 0;
//}
