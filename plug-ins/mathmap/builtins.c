#include <math.h>
#include <string.h>
#ifdef _GIMP
#include <stdlib.h>
#else
#include <libc.h>
#endif

#include "builtins.h"

extern int imageWidth,
    imageHeight,
    inputBPP;
extern double middleX,
    middleY;
extern unsigned char *imageData;
extern double stack[];
extern int stackp;
extern int intersamplingEnabled,
    oversamplingEnabled;

builtin *firstBuiltin = 0;

double
color_to_double (unsigned int red, unsigned int green, unsigned int blue, unsigned int alpha)
{
    double val;

    *(unsigned int*)&val = (alpha << 24) | (red << 16) | (green << 8) | blue;

    return val;
}

double
pixel_to_double (unsigned char *pixel)
{
    if (inputBPP == 1)
	return color_to_double(pixel[0], pixel[0], pixel[0], 255);
    else if (inputBPP == 2)
	return color_to_double(pixel[0], pixel[0], pixel[0], pixel[1]);
    else if (inputBPP == 3)
	return color_to_double(pixel[0], pixel[1], pixel[2], 255);
    else
	return color_to_double(pixel[0], pixel[1], pixel[2], pixel[3]);
}

void
double_to_color (double val, unsigned int *red, unsigned int *green,
		 unsigned int *blue, unsigned int *alpha)
{
    unsigned int color = *(unsigned int*)&val;

    *alpha = color >> 24;
    *red = (color >> 16) & 0xff;
    *green = (color >> 8) & 0xff;
    *blue = color & 0xff;
}

unsigned int
alpha_component (double val)
{
    unsigned int color = *(unsigned int*)&val;

    return color >> 24;
}

unsigned int
red_component (double val)
{
    unsigned int color = *(unsigned int*)&val;

    return (color >> 16) & 0xff;
}

unsigned int
green_component (double val)
{
    unsigned int color = *(unsigned int*)&val;

    return (color >> 8) & 0xff;
}

unsigned int
blue_component (double val)
{
    unsigned int color = *(unsigned int*)&val;

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
builtin_pow (void *arg)
{
    stack[stackp - 2] = pow(stack[stackp - 2], stack[stackp - 1]);
    --stackp;
}

void
builtin_abs (void *arg)
{
    stack[stackp - 1] = fabs(stack[stackp - 1]);
}

void
builtin_floor (void *arg)
{
    stack[stackp - 1] = floor(stack[stackp - 1]);
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
builtin_not (void *arg)
{
    if (stack[stackp - 1] != 0.0)
	stack[stackp - 1] = 0.0;
    else
	stack[stackp - 1] = 1.0;
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
builtin_equal (void *arg)
{
    if (stack[stackp - 2] == stack[stackp - 1])
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
builtin_greater (void *arg)
{
    if (stack[stackp - 2] > stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_lessequal (void *arg)
{
    if (stack[stackp - 2] <= stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_greaterequal (void *arg)
{
    if (stack[stackp - 2] >= stack[stackp - 1])
	stack[stackp - 2] = 1.0;
    else
	stack[stackp - 2] = 0.0;
    --stackp;
}

void
builtin_notequal (void *arg)
{
    if (stack[stackp - 2] != stack[stackp - 1])
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
    double x = stack[stackp - 2],
	y = stack[stackp - 1];
    unsigned char pixel[4];

    if (!oversamplingEnabled)
    {
	x += 0.5;
	y += 0.5;
    }

    mathmap_get_pixel(floor(x + originX + middleX), floor(y + originY + middleY), pixel);

    stack[stackp - 2] = pixel_to_double(pixel);
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
	pixel4a[4],
	resultPixel[4];
    unsigned char *pixel1 = pixel1a,
	*pixel2 = pixel2a,
	*pixel3 = pixel3a,
	*pixel4 = pixel4a;
    int i;

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

    for (i = 0; i < inputBPP; ++i)
	resultPixel[i] = pixel1[i] * p1fact
	    + pixel2[i] * p2fact
	    + pixel3[i] * p3fact
	    + pixel4[i] * p4fact;

    stack[stackp - 2] = pixel_to_double(resultPixel);
    --stackp;
}

void
builtin_origValRA (void *arg)
{
    double x = cos(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleX,
	y = sin(stack[stackp - 1] * M_PI / 180) * stack[stackp - 2] + middleY;
    unsigned char pixel[4];

    if (!oversamplingEnabled)
    {
	x += 0.5;
	y += 0.5;
    }
  
    mathmap_get_pixel(floor(x + originX), floor(y + originY), pixel);
    
    stack[stackp - 2] = pixel_to_double(pixel);
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
	pixel4a[4],
	resultPixel[4];
    unsigned char *pixel1 = pixel1a,
	*pixel2 = pixel2a,
	*pixel3 = pixel3a,
	*pixel4 = pixel4a;
    int i;

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

    for (i = 0; i < inputBPP; ++i)
	resultPixel[i] = pixel1[i] * p1fact
	    + pixel2[i] * p2fact
	    + pixel3[i] * p3fact
	    + pixel4[i] * p4fact;

    stack[stackp - 2] = pixel_to_double(resultPixel);
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
builtin_gray (void *arg)
{
    int red,
	green,
	blue,
	alpha;

    double_to_color(stack[stackp - 1], &red, &green, &blue, &alpha);
    stack[stackp - 1] = (0.299 * red + 0.587 * green + 0.114 * blue) / 255.0;
}

void
builtin_alpha (void *arg)
{
    stack[stackp - 1] = alpha_component(stack[stackp - 1]) / 255.0;
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

    stack[stackp - 1] = color_to_double(redComponent, greenComponent, blueComponent, 255);
}

void
builtin_rgbaColor (void *arg)
{
    int redComponent = stack[stackp - 4] * 255,
	greenComponent = stack[stackp - 3] * 255,
	blueComponent = stack[stackp - 2] * 255,
	alphaComponent = stack[stackp - 1] * 255;

    stackp -= 3;

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

    if (alphaComponent < 0)
	alphaComponent = 0;
    else if (alphaComponent > 255)
	alphaComponent = 255;

    stack[stackp - 1] = color_to_double(redComponent, greenComponent,
					blueComponent, alphaComponent);
}

void
builtin_grayColor (void *arg)
{
    int grayLevel = stack[stackp - 1] * 255;

    if (grayLevel < 0)
	grayLevel = 0;
    else if (grayLevel > 255)
	grayLevel = 255;

    stack[stackp - 1] = color_to_double(grayLevel, grayLevel, grayLevel, 255);
}

void
builtin_grayaColor (void *arg)
{
    int grayLevel = stack[stackp - 2] * 255,
	alphaComponent = stack[stackp - 1] * 255;

    --stackp;

    if (grayLevel < 0)
	grayLevel = 0;
    else if (grayLevel > 255)
	grayLevel = 255;

    if (alphaComponent < 0)
	alphaComponent = 0;
    else if (alphaComponent > 255)
	alphaComponent = 255;

    stack[stackp - 1] = color_to_double(grayLevel, grayLevel, grayLevel, alphaComponent);
}

builtin*
builtin_with_name (const char *name)
{
    builtin *next;

    if (intersamplingEnabled)
    {
	if (strcmp(name, "origValXY") == 0)
	    return builtin_with_name("origValXYIntersample");
	else if (strcmp(name, "origValRA") == 0)
	    return builtin_with_name("origValRAIntersample");
    }

    for (next = firstBuiltin; next != 0; next = next->next)
	if (strcmp(name, next->name) == 0)
	    return next;

    return 0;
}

void
register_builtin (const char *name, builtinFunction function, int numParams)
{
    builtin *theBuiltin = (builtin*)malloc(sizeof(builtin));

    strncpy(theBuiltin->name, name, MAX_BUILTIN_LENGTH);
    theBuiltin->name[MAX_BUILTIN_LENGTH] = '\0';
    theBuiltin->function = function;
    theBuiltin->numParams = numParams;
    theBuiltin->next = firstBuiltin;

    firstBuiltin = theBuiltin;
}

void
init_builtins (void)
{
    register_builtin("sin", builtin_sin, 1);
    register_builtin("cos", builtin_cos, 1);
    register_builtin("tan", builtin_tan, 1);
    register_builtin("asin", builtin_asin, 1);
    register_builtin("acos", builtin_acos, 1);
    register_builtin("atan", builtin_atan, 1);
    register_builtin("pow", builtin_pow, 2);
    register_builtin("abs", builtin_abs, 1);
    register_builtin("floor", builtin_floor, 1);
    register_builtin("sign", builtin_sign, 1);
    register_builtin("min", builtin_min, 2);
    register_builtin("max", builtin_max, 2);
    register_builtin("not", builtin_not, 1);
    register_builtin("or", builtin_or, 2);
    register_builtin("and", builtin_and, 2);
    register_builtin("equal", builtin_equal, 2);
    register_builtin("less", builtin_less, 2);
    register_builtin("greater", builtin_greater, 2);
    register_builtin("lessequal", builtin_lessequal, 2);
    register_builtin("greaterequal", builtin_greaterequal, 2);
    register_builtin("notequal", builtin_notequal, 2);
    register_builtin("inintv", builtin_inintv, 3);
    register_builtin("rand", builtin_rand, 2);
    register_builtin("origValXY", builtin_origValXY, 2);
    register_builtin("origValXYIntersample", builtin_origValXYIntersample, 2);
    register_builtin("origValRA", builtin_origValRA, 2);
    register_builtin("origValRAIntersample", builtin_origValRAIntersample, 2);
    register_builtin("red", builtin_red, 1);
    register_builtin("green", builtin_green, 1);
    register_builtin("blue", builtin_blue, 1);
    register_builtin("gray", builtin_gray, 1);
    register_builtin("alpha", builtin_alpha, 1);
    register_builtin("rgbColor", builtin_rgbColor, 3);
    register_builtin("rgbaColor", builtin_rgbaColor, 4);
    register_builtin("grayColor", builtin_grayColor, 1);
    register_builtin("grayaColor", builtin_grayaColor, 2);
}
