#include <math.h>
#ifdef _GIMP
#include <stdlib.h>
#else
#include <libc.h>
#endif

#include "builtins.h"

extern int imageWidth,
    imageHeight;
extern double middleX,
    middleY;
extern unsigned char *imageData;
extern double stack[];
extern int stackp;

double
color_to_double (int red, int green, int blue)
{
    double val;

    *(int*)&val = (red << 16) | (green << 8) | blue;

    return val;
}

void
double_to_color (double val, int *red, int *green, int *blue)
{
    int color = *(int*)&val;

    *red = color >> 16;
    *green = (color >> 8) & 0xff;
    *blue = color & 0xff;
}

int
red_component (double val)
{
    int color = *(int*)&val;

    return color >> 16;
}

int
green_component (double val)
{
    int color = *(int*)&val;

    return (color >> 8) & 0xff;
}

int
blue_component (double val)
{
    int color = *(int*)&val;

    return color & 0xff;
}

void
builtin_sin (void *arg)
{
    stack[stackp - 1] = sin(stack[stackp - 1] * M_PI / 180.0);
}

void
builtin_cos (void *arg)
{
    stack[stackp - 1] = cos(stack[stackp - 1] * M_PI / 180.0);
}

void
builtin_tan (void *arg)
{
    stack[stackp - 1] = tan(stack[stackp - 1] * M_PI / 180.0);
}

void
builtin_asin (void *arg)
{
    stack[stackp - 1] = asin(stack[stackp - 1]) * 180.0 / M_PI;
}

void
builtin_acos (void *arg)
{
    stack[stackp - 1] = acos(stack[stackp - 1]) * 180.0 / M_PI;
}

void
builtin_atan (void *arg)
{
    stack[stackp - 1] = atan(stack[stackp - 1]) * 180.0 / M_PI;
}

void
builtin_abs (void *arg)
{
    stack[stackp - 1] = fabs(stack[stackp - 1]);
}

void
builtin_sign (void *arg)
{
    if (stack[stackp - 1] < 0)
	stack[stackp - 1] = -1.0;
    else if (stack[stackp - 1] > 0)
	stack[stackp - 1] = 1.0;
    else
	stack[stackp - 1] = 0.0;
}

void
builtin_min (void *arg)
{
    if (stack[stackp - 2] >= stack[stackp - 1])
	stack[stackp - 2] = stack[stackp - 1];
    --stackp;
}

void
builtin_max (void *arg)
{
    if (stack[stackp - 2] <= stack[stackp - 1])
	stack[stackp - 2] = stack[stackp - 1];
    --stackp;
}

void
builtin_or (void *arg)
{
    if (stack[stackp - 2] || stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_and (void *arg)
{
    if (stack[stackp - 2] && stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_less (void *arg)
{
    if (stack[stackp - 2] < stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_inintv (void *arg)
{
    if (stack[stackp - 3] >= stack[stackp - 2] && stack[stackp - 3] <= stack[stackp - 1])
	stack[stackp - 3] = 1.0;
    else
	stack[stackp - 3] = 0.0;
    stackp -= 2;
}

void
builtin_rand (void *arg)
{
    stack[stackp - 2] = (random() / (double)0x7fffffff)
	* (stack[stackp - 1] - stack[stackp - 2]) + stack[stackp - 2];
    --stackp;
}

#ifdef _GIMP

void mathmap_get_pixel (int x, int y, unsigned char *pixel);

extern int originX,
    originY,
    wholeImageWidth,
    wholeImageHeight;

void
builtin_origValXY (void *arg)
{
    int x = stack[stackp - 2],
	y = stack[stackp - 1];
    unsigned char pixel[4];

    mathmap_get_pixel(x + originX + middleX, y + originY + middleY, pixel);

    stack[stackp - 2] = color_to_double(pixel[0], pixel[1], pixel[2]);
    --stackp;
}

void
builtin_origValXYIntersample (void *arg)
{
    double x = stack[stackp - 2] + middleX + originX,
	y = stack[stackp - 1] + middleY + originY;
    int x1 = floor(x),
	x2 = x1 + 1,
	y1 = floor(y),
	y2 = y1 + 1;
    double x2fact = (x - x1),
	y2fact = (y - y1),
	x1fact = 1.0 - x2fact,
	y1fact = 1.0 - y2fact,
	p1fact = x1fact * y1fact,
	p2fact = x1fact * y2fact,
	p3fact = x2fact * y1fact,
	p4fact = x2fact * y2fact;
    static unsigned char blackPixel[4] = { 0, 0, 0, 0 };
    unsigned char pixel1a[4],
	pixel2a[4],
	pixel3a[4],
	pixel4a[4];
    unsigned char *pixel1 = pixel1a,
	*pixel2 = pixel2a,
	*pixel3 = pixel3a,
	*pixel4 = pixel4a;
    int red,
	green,
	blue;

    if (x1 < 0 || x1 >= wholeImageWidth || y1 < 0 || y1 >= wholeImageHeight)
	pixel1 = blackPixel;
    else
	mathmap_get_pixel(x1, y1, pixel1);

    if (x1 < 0 || x1 >= wholeImageWidth || y2 < 0 || y2 >= wholeImageHeight)
	pixel2 = blackPixel;
    else
	mathmap_get_pixel(x1, y2, pixel2);

    if (x2 < 0 || x2 >= wholeImageWidth || y1 < 0 || y1 >= wholeImageHeight)
	pixel3 = blackPixel;
    else
	mathmap_get_pixel(x2, y1, pixel3);

    if (x2 < 0 || x2 >= wholeImageWidth || y2 < 0 || y2 >= wholeImageHeight)
	pixel4 = blackPixel;
    else
	mathmap_get_pixel(x2, y2, pixel4);

    red = pixel1[0] * p1fact + pixel2[0] * p2fact + pixel3[0] * p3fact + pixel4[0] * p4fact;
    green = pixel1[1] * p1fact + pixel2[1] * p2fact + pixel3[1] * p3fact + pixel4[1] * p4fact;
    blue = pixel1[2] * p1fact + pixel2[2] * p2fact + pixel3[2] * p3fact + pixel4[2] * p4fact;

    stack[stackp - 2] = color_to_double(red, green, blue);
    --stackp;
}

void
builtin_origValRA (void *arg)
{
    int x = cos(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleX,
	y = sin(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleY;
    unsigned char pixel[4];
  
    mathmap_get_pixel(x + originX, y + originY, pixel);
    
    stack[stackp - 2] = color_to_double(pixel[0], pixel[1], pixel[2]);
    --stackp;
}

void
builtin_origValRAIntersample (void *arg)
{
    double x = cos(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleX + originX,
	y = sin(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleY + originY;
    int x1 = floor(x),
	x2 = x1 + 1,
	y1 = floor(y),
	y2 = y1 + 1;
    double x2fact = (x - x1),
	y2fact = (y - y1),
	x1fact = 1.0 - x2fact,
	y1fact = 1.0 - y2fact,
	p1fact = x1fact * y1fact,
	p2fact = x1fact * y2fact,
	p3fact = x2fact * y1fact,
	p4fact = x2fact * y2fact;
    static unsigned char blackPixel[4] = { 0, 0, 0, 0 };
    unsigned char pixel1a[4],
	pixel2a[4],
	pixel3a[4],
	pixel4a[4];
    unsigned char *pixel1 = pixel1a,
	*pixel2 = pixel2a,
	*pixel3 = pixel3a,
	*pixel4 = pixel4a;
    int red,
	green,
	blue;

    if (x1 < 0 || x1 >= wholeImageWidth || y1 < 0 || y1 >= wholeImageHeight)
	pixel1 = blackPixel;
    else
	mathmap_get_pixel(x1, y1, pixel1);

    if (x1 < 0 || x1 >= wholeImageWidth || y2 < 0 || y2 >= wholeImageHeight)
	pixel2 = blackPixel;
    else
	mathmap_get_pixel(x1, y2, pixel2);

    if (x2 < 0 || x2 >= wholeImageWidth || y1 < 0 || y1 >= wholeImageHeight)
	pixel3 = blackPixel;
    else
	mathmap_get_pixel(x2, y1, pixel3);

    if (x2 < 0 || x2 >= wholeImageWidth || y2 < 0 || y2 >= wholeImageHeight)
	pixel4 = blackPixel;
    else
	mathmap_get_pixel(x2, y2, pixel4);

    red = pixel1[0] * p1fact + pixel2[0] * p2fact + pixel3[0] * p3fact + pixel4[0] * p4fact;
    green = pixel1[1] * p1fact + pixel2[1] * p2fact + pixel3[1] * p3fact + pixel4[1] * p4fact;
    blue = pixel1[2] * p1fact + pixel2[2] * p2fact + pixel3[2] * p3fact + pixel4[2] * p4fact;

    stack[stackp - 2] = color_to_double(red, green, blue);
    --stackp;
}

#else

void
builtin_origValXY (void *arg)
{
    int x = stack[stackp - 2] + middleX,
	y = stack[stackp - 1] + middleY;
    unsigned char *pixel;

    if (x < 0 || y < 0 || x >= imageWidth || y >= imageHeight)
    {
	stack[stackp - 2] = 0.0;
	--stackp;
	return;
    }

    pixel = imageData + (y * imageWidth + x) * 3;
    
    stack[stackp - 2] = color_to_double(pixel[0], pixel[1], pixel[2]);
    --stackp;
}

void
builtin_origValXYIntersample (void *arg)
{
    double x = stack[stackp - 2] + middleX,
	y = stack[stackp - 1] + middleY;
    int x1 = floor(x),
	x2 = x1 + 1,
	y1 = floor(y),
	y2 = y1 + 1;
    double x2fact = (x - x1),
	y2fact = (y - y1),
	x1fact = 1.0 - x2fact,
	y1fact = 1.0 - y2fact,
	p1fact = x1fact * y1fact,
	p2fact = x1fact * y2fact,
	p3fact = x2fact * y1fact,
	p4fact = x2fact * y2fact;
    static unsigned char blackPixel[] = { 0, 0, 0 };
    unsigned char *pixel1,
	*pixel2,
	*pixel3,
	*pixel4;
    int red,
	green,
	blue;

    if (x1 < 0 || x1 >= imageWidth || y1 < 0 || y1 >= imageHeight)
	pixel1 = blackPixel;
    else
	pixel1 = imageData + (y1 * imageWidth + x1) * 3;

    if (x1 < 0 || x1 >= imageWidth || y2 < 0 || y2 >= imageHeight)
	pixel2 = blackPixel;
    else
	pixel2 = imageData + (y2 * imageWidth + x1) * 3;

    if (x2 < 0 || x2 >= imageWidth || y1 < 0 || y1 >= imageHeight)
	pixel3 = blackPixel;
    else
	pixel3 = imageData + (y1 * imageWidth + x2) * 3;

    if (x2 < 0 || x2 >= imageWidth || y2 < 0 || y2 >= imageHeight)
	pixel4 = blackPixel;
    else
	pixel4 = imageData + (y2 * imageWidth + x2) * 3;

    red = pixel1[0] * p1fact + pixel2[0] * p2fact + pixel3[0] * p3fact + pixel4[0] * p4fact;
    green = pixel1[1] * p1fact + pixel2[1] * p2fact + pixel3[1] * p3fact + pixel4[1] * p4fact;
    blue = pixel1[2] * p1fact + pixel2[2] * p2fact + pixel3[2] * p3fact + pixel4[2] * p4fact;

    stack[stackp - 2] = color_to_double(red, green, blue);
    --stackp;
}

void
builtin_origValRA (void *arg)
{
    int x = cos(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleX,
	y = sin(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleY;
    unsigned char *pixel;

    if (x < 0 || y < 0 || x >= imageWidth || y >= imageHeight)
    {
	stack[stackp - 2] = 0.0;
	--stackp;
	return;
    }

    pixel = imageData + (y * imageWidth + x) * 3;
    
    stack[stackp - 2] = color_to_double(pixel[0], pixel[1], pixel[2]);
    --stackp;
}

void
builtin_origValRAIntersample (void *arg)
{
    double x = cos(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleX,
	y = sin(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleY;
    int x1 = floor(x),
	x2 = x1 + 1,
	y1 = floor(y),
	y2 = y1 + 1;
    double x2fact = (x - x1),
	y2fact = (y - y1),
	x1fact = 1.0 - x2fact,
	y1fact = 1.0 - y2fact,
	p1fact = x1fact * y1fact,
	p2fact = x1fact * y2fact,
	p3fact = x2fact * y1fact,
	p4fact = x2fact * y2fact;
    static unsigned char blackPixel[] = { 0, 0, 0 };
    unsigned char *pixel1,
	*pixel2,
	*pixel3,
	*pixel4;
    int red,
	green,
	blue;

    if (x1 < 0 || x1 >= imageWidth || y1 < 0 || y1 >= imageHeight)
	pixel1 = blackPixel;
    else
	pixel1 = imageData + (y1 * imageWidth + x1) * 3;

    if (x1 < 0 || x1 >= imageWidth || y2 < 0 || y2 >= imageHeight)
	pixel2 = blackPixel;
    else
	pixel2 = imageData + (y2 * imageWidth + x1) * 3;

    if (x2 < 0 || x2 >= imageWidth || y1 < 0 || y1 >= imageHeight)
	pixel3 = blackPixel;
    else
	pixel3 = imageData + (y1 * imageWidth + x2) * 3;

    if (x2 < 0 || x2 >= imageWidth || y2 < 0 || y2 >= imageHeight)
	pixel4 = blackPixel;
    else
	pixel4 = imageData + (y2 * imageWidth + x2) * 3;

    red = pixel1[0] * p1fact + pixel2[0] * p2fact + pixel3[0] * p3fact + pixel4[0] * p4fact;
    green = pixel1[1] * p1fact + pixel2[1] * p2fact + pixel3[1] * p3fact + pixel4[1] * p4fact;
    blue = pixel1[2] * p1fact + pixel2[2] * p2fact + pixel3[2] * p3fact + pixel4[2] * p4fact;

    stack[stackp - 2] = color_to_double(red, green, blue);
    --stackp;
}

#endif /* _GIMP */

void
builtin_red (void *arg)
{
    stack[stackp - 1] = red_component(stack[stackp - 1]) / 255.0;
}

void
builtin_green (void *arg)
{
    stack[stackp - 1] = green_component(stack[stackp - 1]) / 255.0;
}

void
builtin_blue (void *arg)
{
    stack[stackp - 1] = blue_component(stack[stackp - 1]) / 255.0;
}

void
builtin_rgbColor (void *arg)
{
    int redComponent = stack[stackp - 3] * 255,
	greenComponent = stack[stackp - 2] * 255,
	blueComponent = stack[stackp - 1] * 255;

    stackp -= 2;

    if (redComponent < 0)
	redComponent = 0;
    else if (redComponent > 255)
	redComponent = 255;

    if (greenComponent < 0)
	greenComponent = 0;
    else if (greenComponent > 255)
	greenComponent = 255;

    if (blueComponent < 0)
	blueComponent = 0;
    else if (blueComponent > 255)
	blueComponent = 255;

    stack[stackp - 1] = color_to_double(redComponent, greenComponent, blueComponent);
}

void
builtin_grayColor (void *arg)
{
    int grayLevel = stack[stackp - 1] * 255;

    if (grayLevel < 0)
	grayLevel = 0;
    else if (grayLevel > 255)
	grayLevel = 255;

    stack[stackp - 1] = color_to_double(grayLevel, grayLevel, grayLevel);
}
