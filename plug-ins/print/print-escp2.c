/*
 * "$Id$"
 *
 *   Print plug-in EPSON ESC/P2 driver for the GIMP.
 *
 *   Copyright 1997-2000 Michael Sweet (mike@easysw.com) and
 *	Robert Krawitz (rlk@alum.mit.edu)
 *
 *   This program is free software; you can redistribute it and/or modify it
 *   under the terms of the GNU General Public License as published by the Free
 *   Software Foundation; either version 2 of the License, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Contents:
 *
 *   escp2_parameters()     - Return the parameter values for the given
 *                            parameter.
 *   escp2_imageable_area() - Return the imageable area of the page.
 *   escp2_print()          - Print an image to an EPSON printer.
 *   escp2_write()          - Send 6-color ESC/P2 graphics using TIFF packbits
 *                            compression.
 *
 * Revision History:
 *
 *   See ChangeLog
 */

/*
 * Stylus Photo EX added by Robert Krawitz <rlk@alum.mit.edu> August 30, 1999
 */

#include "print.h"

/*
 * Local functions...
 */

static void escp2_write(FILE *, unsigned char *, int, int, int, int, int,
			int, int);
static void initialize_weave(int jets, int separation,
			     int oversample, int horizontal);
static void escp2_flush(int model, int width, int hoffset, int ydpi,
			int xdpi, FILE *prn);
static void
escp2_write_weave(FILE *, int, int, int, int, int, int,
		  unsigned char *c,
		  unsigned char *m,
		  unsigned char *y,
		  unsigned char *k,
		  unsigned char *C,
		  unsigned char *M);


/*
 * Printer capabilities.
 *
 * Various classes of printer capabilities are represented by bitmasks.
 */

typedef unsigned long long model_cap_t;
typedef model_cap_t model_featureset_t;
typedef model_cap_t model_class_t;

#define MODEL_PAPER_SIZE_MASK	0x300
#define MODEL_PAPER_SMALL 	0x000
#define MODEL_PAPER_LARGE 	0x100
#define MODEL_PAPER_1200	0x200

#define MODEL_IMAGEABLE_MASK	0xc00
#define MODEL_IMAGEABLE_DEFAULT	0x000
#define MODEL_IMAGEABLE_PHOTO	0x400
#define MODEL_IMAGEABLE_600	0x800

#define MODEL_INIT_MASK		0xf000
#define MODEL_INIT_COLOR	0x0000
#define MODEL_INIT_PRO		0x1000
#define MODEL_INIT_1500		0x2000
#define MODEL_INIT_600		0x3000
#define MODEL_INIT_PHOTO	0x4000

#define MODEL_HASBLACK_MASK	0x10000
#define MODEL_HASBLACK_YES	0x00000
#define MODEL_HASBLACK_NO	0x10000

#define MODEL_6COLOR_MASK	0x20000
#define MODEL_6COLOR_NO		0x00000
#define MODEL_6COLOR_YES	0x20000

#define MODEL_720DPI_MODE_MASK	0xc0000
#define MODEL_720DPI_DEFAULT	0x00000
#define MODEL_720DPI_600	0x40000
#define MODEL_720DPI_PHOTO	0x40000 /* 0x80000 for experimental stuff */

#define MODEL_1440DPI_MASK	0x100000
#define MODEL_1440DPI_NO	0x000000
#define MODEL_1440DPI_YES	0x100000

#define MODEL_NOZZLES_MASK	0xff000000
#define MODEL_MAKE_NOZZLES(x) 	((long long) ((x)) << 24)
#define MODEL_GET_NOZZLES(x) 	(((x) & MODEL_NOZZLES_MASK) >> 24)
#define MODEL_SEPARATION_MASK	0xf00000000ll
#define MODEL_MAKE_SEPARATION(x) 	(((long long) (x)) << 32)
#define MODEL_GET_SEPARATION(x)	(((x) & MODEL_SEPARATION_MASK) >> 32)

/*
 * SUGGESTED SETTINGS FOR STYLUS PHOTO EX:
 * Brightness 127
 * Blue 92
 * Saturation 1.2
 *
 * Another group of settings that has worked well for me is
 * Brightness 110
 * Gamma 1.2
 * Contrast 97
 * Blue 88
 * Saturation 1.1
 * Density 1.5
 *
 * With the current code, the following settings seem to work nicely:
 * Brightness ~110
 * Gamma 1.3
 * Contrast 80
 * Green 94
 * Blue 89
 * Saturation 1.15
 * Density 1.6
 *
 * The green and blue will vary somewhat with different inks
 */


model_cap_t model_capabilities[] =
{
  /* Stylus Color */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_DEFAULT | MODEL_INIT_COLOR
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(1) | MODEL_MAKE_SEPARATION(1)),
  /* Stylus Color Pro/Pro XL/400/500 */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_DEFAULT | MODEL_INIT_PRO
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(1) | MODEL_MAKE_SEPARATION(1)),
  /* Stylus Color 1500 */
  (MODEL_PAPER_LARGE | MODEL_IMAGEABLE_DEFAULT | MODEL_INIT_1500
   | MODEL_HASBLACK_NO | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(1) | MODEL_MAKE_SEPARATION(1)),
  /* Stylus Color 600 */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_600 | MODEL_INIT_600
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_600
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(1) | MODEL_MAKE_SEPARATION(1)),
  /* Stylus Color 800 */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_600 | MODEL_INIT_600
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(64) | MODEL_MAKE_SEPARATION(8)),
  /* Stylus Color 1520/3000 */
  (MODEL_PAPER_LARGE | MODEL_IMAGEABLE_600 | MODEL_INIT_600
   | MODEL_HASBLACK_YES | MODEL_6COLOR_NO | MODEL_720DPI_DEFAULT
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(64) | MODEL_MAKE_SEPARATION(8)),
  /* Stylus Photo 700 */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_PHOTO | MODEL_INIT_PHOTO
   | MODEL_HASBLACK_YES | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO
   | MODEL_1440DPI_YES | MODEL_MAKE_NOZZLES(32) | MODEL_MAKE_SEPARATION(8)),
  /* Stylus Photo EX */
  (MODEL_PAPER_LARGE | MODEL_IMAGEABLE_PHOTO | MODEL_INIT_PHOTO
   | MODEL_HASBLACK_YES | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO
   | MODEL_1440DPI_YES | MODEL_MAKE_NOZZLES(32) | MODEL_MAKE_SEPARATION(8)),
  /* Stylus Photo */
  (MODEL_PAPER_SMALL | MODEL_IMAGEABLE_PHOTO | MODEL_INIT_PHOTO
   | MODEL_HASBLACK_YES | MODEL_6COLOR_YES | MODEL_720DPI_PHOTO
   | MODEL_1440DPI_NO | MODEL_MAKE_NOZZLES(32) | MODEL_MAKE_SEPARATION(8)),
};

typedef struct {
  const char name[65];
  int hres;
  int vres;
  int softweave;
  int horizontal_passes;
  int vertical_passes;
} res_t;

res_t reslist[] = {
  { "360 DPI", 360, 360, 0, 1, 1 },
  { "720 DPI Microweave", 720, 720, 0, 1, 1 },
  { "720 DPI Softweave", 720, 720, 1, 1, 1 },
  { "720 DPI High Quality", 720, 720, 1, 1, 2 },
  { "720 DPI Highest Quality", 720, 720, 1, 1, 4 },
  { "1440 x 720 DPI Microweave", 1440, 720, 0, 1, 1 },
  { "1440 x 720 DPI Softweave", 1440, 720, 1, 2, 2 },
  { "1440 x 720 DPI Highest Quality", 1440, 720, 1, 2, 4 },
  { "1440 x 720 DPI Two-pass", 2880, 720, 1, 2, 4 },
  { "1440 x 720 DPI Two-pass Microweave", 2880, 720, 0, 1, 1 },
  { "", 0, 0, 0, 0, 0 }
};

static int
escp2_has_cap(int model, model_featureset_t featureset, model_class_t class)
{
  return ((model_capabilities[model] & featureset) == class);
}

static model_class_t
escp2_cap(int model, model_featureset_t featureset)
{
  return (model_capabilities[model] & featureset);
}

static int
escp2_nozzles(int model)
{
  return MODEL_GET_NOZZLES(model_capabilities[model]);
}

static int
escp2_nozzle_separation(int model)
{
  return MODEL_GET_SEPARATION(model_capabilities[model]);
}

/*
 * 'escp2_parameters()' - Return the parameter values for the given parameter.
 */

char **					/* O - Parameter values */
escp2_parameters(int  model,		/* I - Printer model */
                 char *ppd_file,	/* I - PPD file (not used) */
                 char *name,		/* I - Name of parameter */
                 int  *count)		/* O - Number of values */
{
  int		i;
  char		**valptrs;
  static char	*media_sizes[] =
		{
		  ("Letter"),
		  ("Legal"),
		  ("A4"),
		  ("Tabloid"),
		  ("A3"),
		  ("12x18")
		};

  if (count == NULL)
    return (NULL);

  *count = 0;

  if (name == NULL)
    return (NULL);

  if (strcmp(name, "PageSize") == 0)
  {
    if (escp2_has_cap(model, MODEL_PAPER_SIZE_MASK, MODEL_PAPER_LARGE))
      *count = 6;
    else
      *count = 3;

    valptrs = malloc(*count * sizeof(char *));
    for (i = 0; i < *count; i ++)
      {
	/* strdup doesn't appear to be POSIX... */
	valptrs[i] = malloc(strlen(media_sizes[i]) + 1);
	strcpy(valptrs[i], media_sizes[i]);
      }
    return (valptrs);
  }
  else if (strcmp(name, "Resolution") == 0)
  {
    res_t *res = &(reslist[0]);
    valptrs = malloc(sizeof(char *) * sizeof(reslist) / sizeof(res_t));
    *count = 0;
    while(res->hres)
      {
	if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES) ||
	    (res->hres <= 720 && res->vres <= 720))
	  {
	    int nozzles = escp2_nozzles(model);
	    int separation = escp2_nozzle_separation(model);
	    int max_weave = nozzles / separation;
	    if (! res->softweave ||
		(nozzles > 1 && res->vertical_passes <= max_weave))
	      {
		valptrs[*count] = malloc(strlen(res->name) + 1);
		strcpy(valptrs[*count], res->name);
		(*count)++;
	      }
	  }
	res++;
      }
    return (valptrs);
  }
  else
    return (NULL);

}


/*
 * 'escp2_imageable_area()' - Return the imageable area of the page.
 */

void
escp2_imageable_area(int  model,	/* I - Printer model */
                     char *ppd_file,	/* I - PPD file (not used) */
                     char *media_size,	/* I - Media size */
                     int  *left,	/* O - Left position in points */
                     int  *right,	/* O - Right position in points */
                     int  *bottom,	/* O - Bottom position in points */
                     int  *top)		/* O - Top position in points */
{
  int	width, length;			/* Size of page */


  default_media_size(model, ppd_file, media_size, &width, &length);

  switch (escp2_cap(model, MODEL_IMAGEABLE_MASK))
  {
  case MODEL_IMAGEABLE_PHOTO:
        *left   = 9;
        *right  = width - 9;
        *top    = length;
        *bottom = 80;
        break;

  case MODEL_IMAGEABLE_600:
        *left   = 8;
        *right  = width - 9;
        *top    = length - 32;
        *bottom = 40;
        break;

  case MODEL_IMAGEABLE_DEFAULT:
  default:
        *left   = 14;
        *right  = width - 14;
        *top    = length - 14;
        *bottom = 40;
        break;
  }
}


/*
 * 'escp2_print()' - Print an image to an EPSON printer.
 */
void
escp2_print(int       model,		/* I - Model */
            int       copies,		/* I - Number of copies */
            FILE      *prn,		/* I - File to print to */
	    Image     image,		/* I - Image to print */
            unsigned char    *cmap,	/* I - Colormap (for indexed images) */
	    lut_t     *lut,		/* I - Brightness lookup table */
	    vars_t    *v)
{
  char 		*ppd_file = v->ppd_file;
  char 		*resolution = v->resolution;
  char 		*media_size = v->media_size;
  int 		output_type = v->output_type;
  int		orientation = v->orientation;
  float 	scaling = v->scaling;
  int		top = v->top;
  int		left = v->left;
  int		x, y;		/* Looping vars */
  int		xdpi, ydpi;	/* Resolution */
  int		n;		/* Output number */
  unsigned short *out;	/* Output pixels (16-bit) */
  unsigned char	*in,		/* Input pixels */
		*black,		/* Black bitmap data */
		*cyan,		/* Cyan bitmap data */
		*magenta,	/* Magenta bitmap data */
		*lcyan,		/* Light cyan bitmap data */
		*lmagenta,	/* Light magenta bitmap data */
		*yellow;	/* Yellow bitmap data */
  int		page_left,	/* Left margin of page */
		page_right,	/* Right margin of page */
		page_top,	/* Top of page */
		page_bottom,	/* Bottom of page */
		page_width,	/* Width of page */
		page_height,	/* Height of page */
		page_length,	/* True length of page */
		out_width,	/* Width of image on page */
		out_height,	/* Height of image on page */
		out_bpp,	/* Output bytes per pixel */
		temp_width,	/* Temporary width of image on page */
		temp_height,	/* Temporary height of image on page */
		landscape,	/* True if we rotate the output 90 degrees */
		length,		/* Length of raster data */
		errdiv,		/* Error dividend */
		errmod,		/* Error modulus */
		errval,		/* Current error value */
		errline,	/* Current raster line */
		errlast;	/* Last raster line loaded */
  convert_t	colorfunc = 0;	/* Color conversion function... */
  int           image_height,
                image_width,
                image_bpp;
  int		use_softweave = 0;
  int		nozzles = 1;
  int		nozzle_separation = 1;
  int		horizontal_passes = 1;
  int		vertical_passes = 1;
  res_t 	*res;

 /*
  * Setup a read-only pixel region for the entire image...
  */

  Image_init(image);
  image_height = Image_height(image);
  image_width = Image_width(image);
  image_bpp = Image_bpp(image);

 /*
  * Choose the correct color conversion function...
  */

  if (image_bpp < 3 && cmap == NULL)
    output_type = OUTPUT_GRAY;		/* Force grayscale output */

  if (output_type == OUTPUT_COLOR)
  {
    out_bpp = 3;

    if (image_bpp >= 3)
      colorfunc = rgb_to_rgb;
    else
      colorfunc = indexed_to_rgb;
  }
  else
  {
    out_bpp = 1;

    if (image_bpp >= 3)
      colorfunc = rgb_to_gray;
    else if (cmap == NULL)
      colorfunc = gray_to_gray;
    else
      colorfunc = indexed_to_gray;
  }

 /*
  * Figure out the output resolution...
  */
  for (res = &reslist[0];;res++)
    {
      if (!strcmp(resolution, res->name))
	{
	  use_softweave = res->softweave;
	  horizontal_passes = res->horizontal_passes;
	  vertical_passes = res->vertical_passes;
	  xdpi = res->hres;
	  ydpi = res->vres;
	  nozzles = escp2_nozzles(model);
	  nozzle_separation = escp2_nozzle_separation(model);
	  break;
	}
      else if (!strcmp(resolution, ""))
	{
	  return;	  
	}
    }
  if (!use_softweave)
    {
      /*
       * In microweave mode, correct for the loss of page height that
       * would happen in softweave mode.  The divide by 10 is to convert
       * lines into points (Epson printers all have 720 ydpi);
       */
      int extra_points = ((escp2_nozzles(model) - 1) *
			  escp2_nozzle_separation(model) + 5) / 10;
      top += extra_points;
    }

 /*
  * Compute the output size...
  */

  landscape   = 0;
  escp2_imageable_area(model, ppd_file, media_size, &page_left, &page_right,
                       &page_bottom, &page_top);

  page_width  = page_right - page_left;
  page_height = page_top - page_bottom;

  default_media_size(model, ppd_file, media_size, &n, &page_length);

 /*
  * Portrait width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    out_width  = image_width * -72.0 / scaling;
    out_height = image_height * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    out_width  = page_width * scaling / 100.0;
    out_height = out_width * image_height / image_width;
    if (out_height > page_height)
    {
      out_height = page_height * scaling / 100.0;
      out_width  = out_height * image_width / image_height;
    }
  }

  if (out_width == 0)
    out_width = 1;
  if (out_height == 0)
    out_height = 1;

 /*
  * Landscape width/height...
  */

  if (scaling < 0.0)
  {
   /*
    * Scale to pixels per inch...
    */

    temp_width  = image_height * -72.0 / scaling;
    temp_height = image_width * -72.0 / scaling;
  }
  else
  {
   /*
    * Scale by percent...
    */

    temp_width  = page_width * scaling / 100.0;
    temp_height = temp_width * image_width / image_height;
    if (temp_height > page_height)
    {
      temp_height = page_height;
      temp_width  = temp_height * image_height / image_width;
    }
  }

 /*
  * See which orientation has the greatest area (or if we need to rotate the
  * image to fit it on the page...)
  */

  if (orientation == ORIENT_AUTO)
  {
    if (scaling < 0.0)
    {
      if ((out_width > page_width && out_height < page_width) ||
          (out_height > page_height && out_width < page_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
    else
    {
      if ((temp_width * temp_height) > (out_width * out_height))
	orientation = ORIENT_LANDSCAPE;
      else
	orientation = ORIENT_PORTRAIT;
    }
  }

  if (orientation == ORIENT_LANDSCAPE)
  {
    out_width  = temp_width;
    out_height = temp_height;
    landscape  = 1;

   /*
    * Swap left/top offsets...
    */

    x    = top;
    top  = left;
    left = page_width - x - out_width;
  }

  if (left < 0)
    left = (page_width - out_width) / 2 + page_left;
  else
    left = left + page_left;

  if (top < 0)
    top  = (page_height + out_height) / 2 + page_bottom;
  else
    top = page_height - top + page_bottom;

 /*
  * Let the user know what we're doing...
  */

  Image_progress_init(image);

 /*
  * Send ESC/P2 initialization commands...
  */

  fputs("\033@", prn); 				/* ESC/P2 reset */

#if 0
  if (escp2_has_cap(model, MODEL_INIT_MASK, MODEL_INIT_PHOTO))
    {
      fwrite("\033@", 2, 1, prn);
      fwrite("\033(R\010\000\000REMOTE1PM\002\000\000\000SN\003\000\000\000\003MS\010\000\000\000\010\000\364\013x\017\033\000\000\000", 42, 1, prn);
    }
#endif
  fwrite("\033(G\001\000\001", 6, 1, prn);	/* Enter graphics mode */
  switch (ydpi)					/* Set line feed increment */
  {
    case 180 :
        fwrite("\033(U\001\000\024", 6, 1, prn);
        break;

    case 360 :
        fwrite("\033(U\001\000\012", 6, 1, prn);
        break;

    case 720 :
        fwrite("\033(U\001\000\005", 6, 1, prn);
        break;
  }

  if (use_softweave)
    initialize_weave(nozzles, nozzle_separation, horizontal_passes,
		     vertical_passes);
  switch (escp2_cap(model, MODEL_INIT_MASK)) /* Printer specific initialization */
  {
    case MODEL_INIT_COLOR : /* ESC */
        if (output_type == OUTPUT_COLOR && ydpi > 360 && !use_softweave)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case MODEL_INIT_PRO : /* ESC Pro, Pro XL, 400, 500 */
        fwrite("\033(e\002\000\000\001", 7, 1, prn);	/* Small dots */

        if (ydpi > 360 && !use_softweave)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case MODEL_INIT_1500 : /* ESC 1500 */
        fwrite("\033(e\002\000\000\001", 7, 1, prn);	/* Small dots */

        if (ydpi > 360 && !use_softweave)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case MODEL_INIT_600 : /* ESC 600, 800, 1520, 3000 */
	if (output_type == OUTPUT_GRAY)
	  fwrite("\033(K\002\000\000\001", 7, 1, prn);	/* Fast black printing */
	else
	  fwrite("\033(K\002\000\000\002", 7, 1, prn);	/* Color printing */

        fwrite("\033(e\002\000\000\002", 7, 1, prn);	/* Small dots */

        if (ydpi > 360 && !use_softweave)
      	  fwrite("\033(i\001\000\001", 6, 1, prn);	/* Microweave mode on */
        break;

    case MODEL_INIT_PHOTO:
	if (output_type == OUTPUT_GRAY)
	  fwrite("\033(K\002\000\000\001", 7, 1, prn);	/* Fast black printing */
	else
	  fwrite("\033(K\002\000\000\002", 7, 1, prn);	/* Color printing */
        if (ydpi > 360)
	  {
	    fwrite("\033U\000", 3, 1, prn); /* Unidirectional */
	    if (!use_softweave)
	      fwrite("\033(i\001\000\001", 6, 1, prn); /* Microweave on */
	    fwrite("\033(e\002\000\000\004", 7, 1, prn);	/* Microdots */
	  }
	else
	  fwrite("\033(e\002\000\000\003", 7, 1, prn);	/* Whatever dots */
        break;
  }

  fwrite("\033(C\002\000", 5, 1, prn);		/* Page length */
  n = ydpi * page_length / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);

  fwrite("\033(c\004\000", 5, 1, prn);		/* Top/bottom margins */
  n = ydpi * (page_length - page_top) / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);
  n = ydpi * (page_length - page_bottom) / 72;
  if (use_softweave)
    n += 320 * ydpi / 720;
  putc(n & 255, prn);
  putc(n >> 8, prn);

  fwrite("\033(V\002\000", 5, 1, prn);		/* Absolute vertical position */
  n = ydpi * (page_length - top) / 72;
  putc(n & 255, prn);
  putc(n >> 8, prn);

 /*
  * Convert image size to printer resolution...
  */

  out_width  = xdpi * out_width / 72;
  out_height = ydpi * out_height / 72;

  left = ydpi * (left - page_left) / 72;

 /*
  * Allocate memory for the raster data...
  */

  length = (out_width + 7) / 8;

  if (output_type == OUTPUT_GRAY)
  {
    black   = malloc(length * 2);
    cyan    = NULL;
    magenta = NULL;
    lcyan    = NULL;
    lmagenta = NULL;
    yellow  = NULL;
  }
  else
  {
    cyan    = malloc(length);
    magenta = malloc(length);
    yellow  = malloc(length);
  
    if (escp2_has_cap(model, MODEL_HASBLACK_MASK, MODEL_HASBLACK_YES))
      black = malloc(length);
    else
      black = NULL;
    if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES)) {
      lcyan = malloc(length);
      lmagenta = malloc(length);
    } else {
      lcyan = NULL;
      lmagenta = NULL;
    }
  }
    
 /*
  * Output the page, rotating as necessary...
  */

  if (landscape)
  {
    in  = malloc(image_height * image_bpp);
    out = malloc(image_height * out_bpp * 2);

    errdiv  = image_width / out_height;
    errmod  = image_width % out_height;
    errval  = 0;
    errlast = -1;
    errline  = image_width - 1;
    
    for (x = 0; x < out_height; x ++)
    {
      if ((x & 255) == 0)
	Image_note_progress(image, x, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_col(image, in, errline);
      }

      (*colorfunc)(in, out, image_height, image_bpp, lut, cmap, v);

      if (output_type == OUTPUT_GRAY)
      {
        dither_black(out, x, image_height, out_width, black);
	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, lcyan, lmagenta);
	else
	  escp2_write(prn, black, length, 0, 0, ydpi, model, out_width, left);
      }
      else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
      {
        dither_cmyk(out, x, image_height, out_width, cyan, lcyan,
		      magenta, lmagenta, yellow, 0, black, horizontal_passes);

	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, lcyan, lmagenta);
	else
	  {
	    escp2_write(prn, black, length, 0, 0, ydpi, model, out_width,
			left);
	    escp2_write(prn, cyan, length, 0, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, magenta, length, 0, 1, ydpi, model, out_width,
			left);
	    escp2_write(prn, yellow, length, 0, 4, ydpi, model, out_width,
			left);
	    escp2_write(prn, lcyan, length, 1, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, lmagenta, length, 1, 1, ydpi, model, out_width,
			left);
	  }
      }
      else
      {
        dither_cmyk(out, x, image_height, out_width, cyan, 0, magenta, 0,
		      yellow, 0, black, horizontal_passes);
	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, NULL, NULL);
	else
	  {
	    escp2_write(prn, cyan, length, 0, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, magenta, length, 0, 1, ydpi, model, out_width,
			left);
	    escp2_write(prn, yellow, length, 0, 4, ydpi, model, out_width,
			left);
	    if (black != NULL)
	      escp2_write(prn, black, length, 0, 0, ydpi, model, out_width,
			  left);
	  }
      }

      if (!use_softweave)
	fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */

      errval += errmod;
      errline -= errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline --;
      }
    }
    if (use_softweave)
      escp2_flush(model, out_width, left, ydpi, xdpi, prn);
  }
  else
  {
    in  = malloc(image_width * image_bpp);
    out = malloc(image_width * out_bpp * 2);

    errdiv  = image_height / out_height;
    errmod  = image_height % out_height;
    errval  = 0;
    errlast = -1;
    errline  = 0;
    
    for (y = 0; y < out_height; y ++)
    {
      if ((y & 255) == 0)
	Image_note_progress(image, y, out_height);

      if (errline != errlast)
      {
        errlast = errline;
	Image_get_row(image, in, errline);
      }

      (*colorfunc)(in, out, image_width, image_bpp, lut, cmap, v);

      if (output_type == OUTPUT_GRAY)
      {
        dither_black(out, y, image_width, out_width, black);
	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, lcyan, lmagenta);
	else
	  escp2_write(prn, black, length, 0, 0, ydpi, model, out_width, left);
      }
      else if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
      {
        dither_cmyk(out, y, image_width, out_width, cyan, lcyan,
		      magenta, lmagenta, yellow, 0, black, horizontal_passes);
	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, lcyan, lmagenta);
	else
	  {
	    escp2_write(prn, black, length, 0, 0, ydpi, model, out_width,
			left);
	    escp2_write(prn, cyan, length, 0, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, magenta, length, 0, 1, ydpi, model, out_width,
			left);
	    escp2_write(prn, yellow, length, 0, 4, ydpi, model, out_width,
			left);
	    escp2_write(prn, lcyan, length, 1, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, lmagenta, length, 1, 1, ydpi, model, out_width,
			left);
	  }
      }
      else
      {
        dither_cmyk(out, y, image_width, out_width, cyan, 0, magenta, 0,
		      yellow, 0, black, horizontal_passes);
	if (use_softweave)
	  escp2_write_weave(prn, length, ydpi, model, out_width, left, xdpi,
			    cyan, magenta, yellow, black, NULL, NULL);
	else
	  {
	    escp2_write(prn, cyan, length, 0, 2, ydpi, model, out_width,
			left);
	    escp2_write(prn, magenta, length, 0, 1, ydpi, model, out_width,
			left);
	    escp2_write(prn, yellow, length, 0, 4, ydpi, model, out_width,
			left);
	    if (black != NULL)
	      escp2_write(prn, black, length, 0, 0, ydpi, model, out_width,
			  left);
	  }
      }

      if (!use_softweave)
	fwrite("\033(v\002\000\001\000", 7, 1, prn);	/* Feed one line */

      errval += errmod;
      errline += errdiv;
      if (errval >= out_height)
      {
        errval -= out_height;
        errline ++;
      }
    }
    if (use_softweave)
      escp2_flush(model, out_width, left, ydpi, xdpi, prn);
  }

 /*
  * Cleanup...
  */

  free(in);
  free(out);

  if (black != NULL)
    free(black);
  if (cyan != NULL)
  {
    free(cyan);
    free(magenta);
    free(yellow);
  }

  putc('\014', prn);			/* Eject page */
  fputs("\033@", prn);			/* ESC/P2 reset */
}

static void
escp2_pack(unsigned char *line,
	   int length,
	   unsigned char *comp_buf,
	   unsigned char **comp_ptr)
{
  unsigned char *start;			/* Start of compressed data */
  unsigned char repeat;			/* Repeating char */
  int count;			/* Count of compressed bytes */
  int tcount;			/* Temporary count < 128 */

  /*
   * Compress using TIFF "packbits" run-length encoding...
   */

  (*comp_ptr) = comp_buf;

  while (length > 0)
    {
      /*
       * Get a run of non-repeated chars...
       */

      start  = line;
      line   += 2;
      length -= 2;

      while (length > 0 && (line[-2] != line[-1] || line[-1] != line[0]))
	{
	  line ++;
	  length --;
	}

      line   -= 2;
      length += 2;

      /*
       * Output the non-repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = tcount - 1;
	  memcpy((*comp_ptr) + 1, start, tcount);

	  (*comp_ptr) += tcount + 1;
	  start    += tcount;
	  count    -= tcount;
	}

      if (length <= 0)
	break;

      /*
       * Find the repeated sequences...
       */

      start  = line;
      repeat = line[0];

      line ++;
      length --;

      while (length > 0 && *line == repeat)
	{
	  line ++;
	  length --;
	}

      /*
       * Output the repeated sequences (max 128 at a time).
       */

      count = line - start;
      while (count > 0)
	{
	  tcount = count > 128 ? 128 : count;

	  (*comp_ptr)[0] = 1 - tcount;
	  (*comp_ptr)[1] = repeat;

	  (*comp_ptr) += 2;
	  count    -= tcount;
	}
    }
}


	   
/*
 * 'escp2_write()' - Send ESC/P2 graphics using TIFF packbits compression.
 */

static void
escp2_write(FILE          *prn,	/* I - Print file or command */
	     unsigned char *line,	/* I - Output bitmap data */
	     int           length,	/* I - Length of bitmap data */
	     int	   density,     /* I - 0 for dark, 1 for light */
	     int           plane,	/* I - Which color */
	     int           ydpi,	/* I - Vertical resolution */
	     int           model,	/* I - Printer model */
	     int           width,	/* I - Printed width */
	     int           offset)	/* I - Offset from left side */
{
  unsigned char	comp_buf[3072],		/* Compression buffer */
    *comp_ptr;
  static int    last_density = 0;       /* Last density printed */
  static int	last_plane = 0;		/* Last color plane printed */


 /*
  * Don't send blank lines...
  */

  if (line[0] == 0 && memcmp(line, line + 1, length - 1) == 0)
    return;

  escp2_pack(line, length, comp_buf, &comp_ptr);

 /*
  * Set the print head position.
  */

  putc('\r', prn);
 /*
  * Set the color if necessary...
  */

  if (last_plane != plane || last_density != density)
  {
    last_plane = plane;
    last_density = density;
    if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
      fprintf(prn, "\033(r\002%c%c%c", 0, density, plane);
    else
      fprintf(prn, "\033r%c", plane);
  }

  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES))
    fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
	    (offset * 1440 / ydpi) & 255, (offset * 1440 / ydpi) >> 8);
  else
    fprintf(prn, "\033\\%c%c", offset & 255, offset >> 8);

 /*
  * Send a line of raster graphics...
  */

  switch (ydpi)				/* Raster graphics header */
  {
    case 180 :
        fwrite("\033.\001\024\024\001", 6, 1, prn);
        break;
    case 360 :
        fwrite("\033.\001\012\012\001", 6, 1, prn);
        break;
    case 720 :
        if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK, MODEL_720DPI_600))
          fwrite("\033.\001\050\005\001", 6, 1, prn);
#if 0
        else if (model == 7)
          fwrite("\033.\000\050\005\040", 6, 1, prn);
#endif
	else
          fwrite("\033.\001\005\005\001", 6, 1, prn);
        break;
  }

  putc(width & 255, prn);		/* Width of raster line in pixels */
  putc(width >> 8, prn);

  fwrite(comp_buf, comp_ptr - comp_buf, 1, prn);
}


/*
 * "Soft" weave
 *
 * The Epson Stylus Color/Photo printers don't have memory to print
 * using all of the nozzles in the print head.  For example, the Stylus Photo
 * 700/EX has 32 nozzles.  At 720 dpi, with an 8" wide image, a single line
 * requires (8 * 720 * 6 / 8) bytes, or 4320 bytes (because the Stylus Photo
 * printers have 6 ink colors).  To use 32 nozzles would require 138240 bytes.
 * It's actually worse than that, though, because the nozzles are spaced 8
 * rows apart.  Therefore, in order to store enough data to permit sending the
 * page as a simple raster, the printer would require enough memory to store
 * 256 rows, or 1105920 bytes.  Considering that the Photo EX can print
 * 11" wide, we're looking at more like 1.5 MB.  In fact, these printers are
 * capable of 1440 dpi horizontal resolution.  This would require 3 MB.  The
 * printers actually have 64K.
 *
 * With the newer (750 and 1200) printers it's even worse, since these printers
 * support multiple dot sizes.  But that's neither here nor there.
 *
 * The printer is capable of printing an image fed to it as single raster
 * lines.  This is called MicroWeave (tm).  It actually produces extremely
 * high quality output, but it only uses one nozzle per color per pass.
 * This means that it has to make a lot of passes to print a page, so it's
 * extremely slow (a full 8.5x11" page takes over 30 minutes!).  It's also
 * not possible to print very close to the bottom of the page with MicroWeave
 * since only the first nozzle is used, and the head cannot get closer than
 * some distance from the edge of the page.
 *
 * The solution is to have the host rearrange the output so that a single
 * pass is fed to the print head.  This means that we have to feed the printer
 * every 8th line as a single pass, and we then have to interleave ("weave")
 * the other raster lines as separate passes.  This allows us to use all 32
 * nozzles, and achieve much higher printing speed.
 *
 * What makes this interesting is that there are many different ways of
 * of accomplishing this goal.  The naive way would be to divide the image
 * up into groups of 256 rows, and print all the mod8=0 rows in the first pass,
 * mod8=1 rows in the second, and so forth.  The problem with this approach
 * is that the individual ink jets are not perfectly uniform; some emit
 * slightly bigger or smaller drops than others.  Since each group of 8
 * adjacent rows is printed with the same nozzle, that means that there will
 * be distinct streaks of lighter and darker bands within the image (8 rows
 * is 1/90", which is visible; 1/720" is not).  Possibly worse is that these
 * patterns will repeat every 256 rows.  This creates banding patterns that
 * are about 1/3" wide.
 *
 * So we have to do something to break up this patterning.
 *
 * Epson does not publish the weaving algorithms that they use in their
 * bundled drivers.  Indeed, their developer web site
 * (http://www.ercipd.com/isv/edr_docs.htm) does not even describe how to
 * do this weaving at all; it says that the only way to achieve 720 dpi
 * is to use MicroWeave.  It does note (correctly) that 1440 dpi horizontal
 * can only be achieved by the driver (i. e. in software).  The manual
 * actually makes it fairly clear how to do this (it requires two passes
 * with horizontal head movement between passes), and it is presumably
 * possible to do this with MicroWeave.
 *
 * The information about how to do this is apparently available under NDA.
 * It's actually easy enough to reverse engineer what's inside a print file
 * with a simple Perl script.  There are presumably other printer commands
 * that are not documented and may not be as easy to reverse engineer.
 *
 * I considered a few algorithms to perform the weave.  The first one I
 * devised let me use only (jets - distance_between_jets + 1) nozzles, or
 * 25.  This is OK in principle, but it's slower than using all nozzles.
 * By playing around with it some more, I came up with an algorithm that
 * lets me use all of the nozzles, except near the top and bottom of the
 * page.
 *
 * This still produces some banding, though.  Even better quality can be
 * achieved by using multiple nozzles on the same line.  How do we do this?
 * In 1440x720 mode, we're printing two output lines at the same vertical
 * position.  However, if we want four passes, we have to effectively print
 * each line twice.  Actually doing this would increase the density, so
 * what we do is print half the dots on each pass.  This produces near-perfect
 * output, and it's far faster than using "MicroWeave".
 *
 * The current algorithm is not completely general.  The number of passes
 * is limited to (nozzles / gap).  On the Photo EX class printers, that limits
 * it to 4 -- 32 nozzles, an inter-nozzle gap of 8 lines.  Furthermore, there
 * are a number of routines that are only coded up to 4 passes.
 *
 * The routine initialize_weave calculates the basic parameters, given
 * the number of jets and separation between jets, in rows.
 *
 * -- Robert Krawitz <rlk@alum.mit.edu) November 3, 1999
 */


typedef struct			/* Weave parameters for a specific row */
{
  int row;			/* Absolute row # */
  int pass;			/* Computed pass # */
  int jet;			/* Which physical nozzle we're using */
  int missingstartrows;		/* Phantom rows (nonexistent rows that */
				/* would be printed by nozzles lower than */
				/* the first nozzle we're using this pass; */
				/* with the current algorithm, always zero */
  int logicalpassstart;		/* Offset in rows (from start of image) */
				/* that the printer must be for this row */
				/* to print correctly with the specified jet */
  int physpassstart;		/* Offset in rows to the first row printed */
				/* in this pass.  Currently always equal to */
				/* logicalpassstart */
  int physpassend;		/* Offset in rows (from start of image) to */
				/* the last row that will be printed this */
				/* pass (assuming that we're printing a full */
				/* pass). */
} weave_t;

typedef struct			/* Weave parameters for a specific pass */
{
  int pass;			/* Absolute pass number */
  int missingstartrows;		/* All other values the same as weave_t */
  int logicalpassstart;
  int physpassstart;
  int physpassend;
} pass_t;

typedef union {			/* Offsets from the start of each line */
  off_t v[6];			/* (really pass) */
  struct {
    off_t k;
    off_t m;
    off_t c;
    off_t y;
    off_t M;
    off_t C;
  } p;
} lineoff_t;

typedef union {			/* Base pointers for each pass */
  unsigned char *v[6];
  struct {
    unsigned char *k;
    unsigned char *m;
    unsigned char *c;
    unsigned char *y;
    unsigned char *M;
    unsigned char *C;
  } p;
} linebufs_t;

static unsigned char *linebufs;	/* Actual data buffers */
static linebufs_t *linebases;	/* Base address of each row buffer */
static lineoff_t *lineoffsets;	/* Offsets within each row buffer */
static int *linecounts;		/* How many rows we've printed this pass */
static pass_t *passes;		/* Circular list of pass numbers */
static int last_pass_offset;	/* Starting row (offset from the start of */
				/* the image) of the most recently printed */
				/* pass (so we can determine how far to */
				/* advance the paper) */
static int last_pass;		/* Number of the most recently printed pass */

static int njets;		/* Number of jets in use */
static int separation;		/* Separation between jets */

static int weavefactor;		/* Interleave factor (jets / separation) */
static int jetsused;		/* How many jets we can actually use */
static int initialoffset;	/* Distance between the first row we're */
				/* printing and the logical first row */
				/* (first nozzle of the first pass). */
				/* Currently this is zero. */
static int jetsleftover;	/* How many jets we're *not* using. */
				/* This can be used to rotate exactly */
				/* what jets we're using.  Currently this */
				/* is not used. */
static int weavespan;		/* How many rows total are bracketed by */
				/* one pass (separation * (jets - 1) */
static int horizontal_weave;	/* Number of horizontal passes required */
				/* This is > 1 for some of the ultra-high */
				/* resolution modes */
static int vertical_subpasses;	/* Number of passes per line (for better */
				/* quality) */
static int vmod;		/* Number of banks of passes */
static int oversample;		/* Excess precision per row */
static int is_monochrome = 0;

/*
 * Mapping between color and linear index.  The colors are
 * black, magenta, cyan, yellow, light magenta, light cyan
 */

static int color_indices[16] = { 0, 1, 2, -1,
				 3, -1, -1, -1,
				 -1, 4, 5, -1,
				 -1, -1, -1, -1 };
static int colors[6] = { 0, 1, 2, 4, 1, 2 };
static int densities[6] = { 0, 0, 0, 0, 1, 1 };

static int
get_color_by_params(int plane, int density)
{
  if (plane > 4 || plane < 0 || density > 1 || density < 0)
    return -1;
  return color_indices[density * 8 + plane];
}

/*
 * Initialize the weave parameters
 */
static void
initialize_weave(int jets, int sep, int osample, int v_subpasses)
{
  int i;
  int k;
  char *bufbase;
  if (jets <= 1)
    separation = 1;
  if (sep <= 0)
    separation = 1;
  else
    separation = sep;
  njets = jets;
  if (v_subpasses <= 0)
    v_subpasses = 1;
  oversample = osample;
  vertical_subpasses = v_subpasses;
  njets /= vertical_subpasses;
  vmod = separation * vertical_subpasses;
  horizontal_weave = 1;

  weavefactor = njets / separation;
  jetsused = ((weavefactor) * separation);
  initialoffset = (jetsused - weavefactor - 1) * separation;
  jetsleftover = njets - jetsused + 1;
  weavespan = (jetsused - 1) * separation;

  last_pass_offset = 0;
  last_pass = -1;

  linebufs = malloc(6 * 3072 * vmod * jets * horizontal_weave);
  lineoffsets = malloc(vmod * sizeof(lineoff_t) * horizontal_weave);
  linebases = malloc(vmod * sizeof(linebufs_t) * horizontal_weave);
  passes = malloc(vmod * sizeof(pass_t));
  linecounts = malloc(vmod * sizeof(int));

  bufbase = linebufs;
  
  for (i = 0; i < vmod; i++)
    {
      int j;
      passes[i].pass = -1;
      for (k = 0; k < horizontal_weave; k++)
	{
	  for (j = 0; j < 6; j++)
	    {
	      linebases[k * vmod + i].v[j] = bufbase;
	      bufbase += 3072 * jets;
	    }
	}
    }
}

/*
 * Compute the weave parameters for the given row.  This computation is
 * rather complex, and I need to go back and write down very carefully
 * what's going on here.
 */

static void
weave_parameters_by_row(int row, int vertical_subpass, weave_t *w)
{
  int passblockstart = (row + initialoffset) / jetsused;
  int internaljetsused = jetsused * vertical_subpasses;
  int subpass_adjustment;

  w->row = row;
  w->pass = (passblockstart - (separation - 1)) +
    (separation + row - passblockstart - 1) % separation;
  subpass_adjustment = ((w->pass + 1) / separation) % vertical_subpasses;
  subpass_adjustment = vertical_subpasses - subpass_adjustment - 1;
  vertical_subpass = (vertical_subpass + subpass_adjustment) % vertical_subpasses;
  w->pass += separation * vertical_subpass;
  w->logicalpassstart = (w->pass * jetsused) - initialoffset +
    (w->pass % separation);
  w->jet = ((row - w->logicalpassstart) / separation);
  w->jet += jetsused * (vertical_subpasses - 1);
  w->logicalpassstart = w->row - (w->jet * separation);
  if (w->logicalpassstart >= 0)
    w->physpassstart = w->logicalpassstart;
  else
    w->physpassstart = w->logicalpassstart +
      (separation * ((separation - 1 - w->logicalpassstart) / separation));
  w->physpassend = (internaljetsused - 1) * separation +
    w->logicalpassstart;
  w->missingstartrows = (w->physpassstart - w->logicalpassstart) / separation;
  if (w->pass < 0)
    {
      w->logicalpassstart -= w->pass * separation;
      w->physpassend -= w->pass * separation;
      w->jet += w->pass;
      w->missingstartrows += w->pass;
    }
  w->pass++;
}

static lineoff_t *
get_lineoffsets(int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(row, subpass, &w);
  return &(lineoffsets[w.pass % vmod]);
}

static int *
get_linecount(int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(row, subpass, &w);
  return &(linecounts[w.pass % vmod]);
}

static const linebufs_t *
get_linebases(int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(row, subpass, &w);
  return &(linebases[w.pass % vmod]);
}

static pass_t *
get_pass_by_row(int row, int subpass)
{
  weave_t w;
  weave_parameters_by_row(row, subpass, &w);
  return &(passes[w.pass % vmod]);
}

static lineoff_t *
get_lineoffsets_by_pass(int pass)
{
  return &(lineoffsets[pass % vmod]);
}

static int *
get_linecount_by_pass(int pass)
{
  return &(linecounts[pass % vmod]);
}

static const linebufs_t *
get_linebases_by_pass(int pass)
{
  return &(linebases[pass % vmod]);
}

static pass_t *
get_pass_by_pass(int pass)
{
  return &(passes[pass % vmod]);
}

/*
 * If there are phantom rows at the beginning of a pass, fill them in so
 * that the printer knows exactly what it doesn't have to print.  We're
 * using RLE compression here.  Each line must be specified independently,
 * so we have to compute how many full blocks (groups of 128 bytes, or 1024
 * "off" pixels) and how much leftover is needed.  Note that we can only
 * RLE-encode groups of 2 or more bytes; single bytes must be specified
 * with a count of 1.
 */

static void
fillin_start_rows(int row, int subpass, int width, int missingstartrows)
{
  lineoff_t *offsets = get_lineoffsets(row, subpass);
  const linebufs_t *bufs = get_linebases(row, subpass);
  int i = 0;
  int k = 0;
  int j;
  int m;
  width = (width + (oversample - 1)) / oversample;
  for (k = 0; k < missingstartrows; k++)
    {
      int bytes_to_fill = width;
      int full_blocks = bytes_to_fill / 1024;
      int leftover = (7 + (bytes_to_fill % 1024)) / 8;
      int l = 0;
  
      while (l < full_blocks)
	{
	  for (j = 0; j < 6; j++)
	    {
	      for (m = 0; m < horizontal_weave; m++)
		{
		  (bufs[m].v[j][2 * i]) = 129;
		  (bufs[m].v[j][2 * i + 1]) = 0;
		}
	    }
	  i++;
	  l++;
	}
      if (leftover == 1)
	{
	  for (j = 0; j < 6; j++)
	    {
	      for (m = 0; m < horizontal_weave; m++)
		{
		  (bufs[m].v[j][2 * i]) = 1;
		  (bufs[m].v[j][2 * i + 1]) = 0;
		}
	    }
	  i++;
	}
      else if (leftover > 0)
	{
	  for (j = 0; j < 6; j++)
	    {
	      for (m = 0; m < horizontal_weave; m++)
		{
		  (bufs[m].v[j][2 * i]) = 257 - leftover;
		  (bufs[m].v[j][2 * i + 1]) = 0;
		}
	    }
	  i++;
	}
    }
  for (j = 0; j < 6; j++)
    for (m = 0; m < horizontal_weave; m++)
      offsets[m].v[j] = 2 * i;
}

static void
initialize_row(int row, int width)
{
  weave_t w;
  int i;
  for (i = 0; i < vertical_subpasses; i++)
    {
      weave_parameters_by_row(row, i, &w);
      if (w.physpassstart == row)
	{
	  lineoff_t *lineoffs = get_lineoffsets(row, i);
	  int *linecount = get_linecount(row, i);
	  int j, k;
	  pass_t *pass = get_pass_by_row(row, i);
	  pass->pass = w.pass;
	  pass->missingstartrows = w.missingstartrows;
	  pass->logicalpassstart = w.logicalpassstart;
	  pass->physpassstart = w.physpassstart;
	  pass->physpassend = w.physpassend;
	  for (k = 0; k < horizontal_weave; k++)
	    for (j = 0; j < 6; j++)
	      lineoffs[k].v[j] = 0;
	  *linecount = 0;
	  if (w.missingstartrows > 0)
	    fillin_start_rows(row, i, width, w.missingstartrows);
	}
    }
}

/*
 * A fair bit of this code is duplicated from escp2_write.  That's rather
 * a pity.  It's also not correct for any but the 6-color printers.  One of
 * these days I'll unify it.
 */
static void
flush_pass(int passno, int model, int width, int hoffset, int ydpi,
	   int xdpi, FILE *prn)
{
  int j;
  int k;
  lineoff_t *lineoffs = get_lineoffsets_by_pass(passno);
  const linebufs_t *bufs = get_linebases_by_pass(passno);
  pass_t *pass = get_pass_by_pass(passno);
  int *linecount = get_linecount_by_pass(passno);
  int lwidth = (width + (oversample - 1)) / oversample;
  if (pass->physpassstart > last_pass_offset)
    {
      int advance = pass->logicalpassstart - last_pass_offset;
      int alo = advance % 256;
      int ahi = advance / 256;
      fprintf(prn, "\033(v\002%c%c%c", 0, alo, ahi);
      last_pass_offset = pass->logicalpassstart;
    }
  for (k = 0; k < horizontal_weave; k++)
    {
      for (j = 0; j < 6; j++)
	{
	  if (lineoffs[k].v[j] == 0 || (j > 0 && is_monochrome))
	    continue;
	  if (escp2_has_cap(model, MODEL_6COLOR_MASK, MODEL_6COLOR_YES))
	    fprintf(prn, "\033(r\002%c%c%c", 0, densities[j], colors[j]);
	  else if (densities[j] > 0)
	    continue;
	  else
	    fprintf(prn, "\033r%c", colors[j]);
	  if (escp2_has_cap(model, MODEL_1440DPI_MASK, MODEL_1440DPI_YES))
	    {
	      /* FIXME need a more general way of specifying column */
	      /* separation */
	      fprintf(prn, "\033(\\%c%c%c%c%c%c", 4, 0, 160, 5,
		      ((hoffset * 1440 / ydpi) + (k & oversample)) & 255,
		      ((hoffset * 1440 / ydpi) + (k & oversample)) >> 8);
	    }
	  else
	    {
	      fprintf(prn, "\033\\%c%c", hoffset & 255, hoffset >> 8);
	    }
	  switch (ydpi)				/* Raster graphics header */
	    {
	    case 180 :
	      fwrite("\033.\001\024\024\001", 6, 1, prn);
	      break;
	    case 360 :
	      fwrite("\033.\001\012\012\001", 6, 1, prn);
	      break;
	    case 720 :
	      if (escp2_has_cap(model, MODEL_720DPI_MODE_MASK,
				MODEL_720DPI_600))
		fprintf(prn, "\033.%c%c%c%c", 1, 8 * 5, 5,
			*linecount + pass->missingstartrows);
	      else
		fprintf(prn, "\033.%c%c%c%c", 1, 5, 5,
			*linecount + pass->missingstartrows);
	      break;
	    }

	  putc(lwidth & 255, prn);	/* Width of raster line in pixels */
	  putc(lwidth >> 8, prn);
	  fwrite(bufs[k].v[j], lineoffs[k].v[j], 1, prn);
	  putc('\r', prn);
	}
      fwrite("\033\006", 2, 1, prn);
    }
  last_pass = pass->pass;
  pass->pass = -1;
}

static void
add_to_row(int row, unsigned char *buf, size_t nbytes, int plane, int density,
	   int subpass)
{
  weave_t w;
  int color = get_color_by_params(plane, density);
  lineoff_t *lineoffs = get_lineoffsets(row, subpass);
  const linebufs_t *bufs = get_linebases(row, subpass);
  weave_parameters_by_row(row, subpass, &w);
  memcpy(bufs[0].v[color] + lineoffs[0].v[color], buf, nbytes);
  lineoffs[0].v[color] += nbytes;
}

static void
finalize_row(int row, int model, int width, int hoffset, int ydpi, int xdpi,
	     FILE *prn)
{
  int i;
  for (i = 0; i < vertical_subpasses; i++)
    {
      weave_t w;
      int *lines = get_linecount(row, i);
      weave_parameters_by_row(row, i, &w);
      (*lines)++;
      if (w.physpassend == row)
	{
	  pass_t *pass = get_pass_by_row(row, i);
	  flush_pass(pass->pass, model, width, hoffset, ydpi, xdpi, prn);
	}
    }
}

static void
escp2_flush(int model, int width, int hoffset, int ydpi, int xdpi, FILE *prn)
{
  while (1)
    {
      pass_t *pass = get_pass_by_pass(last_pass + 1);
      if (pass->pass < 0)
	return;
      flush_pass(pass->pass, model, width, hoffset, ydpi, xdpi, prn);
    }
}

static void
escp2_split_2(int length,
	      const unsigned char *in,
	      unsigned char *outlo,
	      unsigned char *outhi)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = in[i];
      outlo[i] = inbyte & 0x55;
      outhi[i] = inbyte & 0xaa;
    }
}

static void
escp2_split_4(int length,
	      const unsigned char *in,
	      unsigned char *out0,
	      unsigned char *out1,
	      unsigned char *out2,
	      unsigned char *out3)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = in[i];
      out0[i] = inbyte & 0x11;
      out1[i] = inbyte & 0x22;
      out2[i] = inbyte & 0x44;
      out3[i] = inbyte & 0x88;
    }
}


static void
escp2_unpack_2(int length,
	       const unsigned char *in,
	       unsigned char *outlo,
	       unsigned char *outhi)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      if (!(i & 1))
	{
	  *outlo =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 5)) << 1) +
	    ((inbyte & (1 << 3)) << 2) +
	    ((inbyte & (1 << 1)) << 3);
	  *outhi =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 4)) << 2) +
	    ((inbyte & (1 << 2)) << 3) +
	    ((inbyte & (1 << 0)) << 4);
	}
      else
	{
	  *outlo +=
	    ((inbyte & (1 << 1)) >> 1) +
	    ((inbyte & (1 << 3)) >> 2) +
	    ((inbyte & (1 << 5)) >> 3) +
	    ((inbyte & (1 << 7)) >> 4);
	  *outhi +=
	    ((inbyte & (1 << 0)) >> 0) +
	    ((inbyte & (1 << 2)) >> 1) +
	    ((inbyte & (1 << 4)) >> 2) +
	    ((inbyte & (1 << 6)) >> 3);
	  outlo++;
	  outhi++;
	}
      in++;
    }
}

static void
escp2_unpack_4(int length,
	       const unsigned char *in,
	       unsigned char *out0,
	       unsigned char *out1,
	       unsigned char *out2,
	       unsigned char *out3)
{
  int i;
  for (i = 0; i < length; i++)
    {
      unsigned char inbyte = *in;
      switch (i & 3)
	{
	case 0:
	  *out0 =
	    ((inbyte & (1 << 7)) << 0) +
	    ((inbyte & (1 << 3)) << 3);
	  *out1 =
	    ((inbyte & (1 << 6)) << 1) +
	    ((inbyte & (1 << 2)) << 4);
	  *out2 =
	    ((inbyte & (1 << 5)) << 2) +
	    ((inbyte & (1 << 1)) << 5);
	  *out3 =
	    ((inbyte & (1 << 4)) << 3) +
	    ((inbyte & (1 << 0)) << 6);
	  break;
	case 1:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 2) +
	    ((inbyte & (1 << 3)) << 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 1) +
	    ((inbyte & (1 << 2)) << 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 0) +
	    ((inbyte & (1 << 1)) << 3);
	  *out3 +=
	    ((inbyte & (1 << 4)) << 1) +
	    ((inbyte & (1 << 0)) << 4);
	  break;
	case 2:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 4) +
	    ((inbyte & (1 << 3)) >> 1);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 3) +
	    ((inbyte & (1 << 2)) << 0);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 2) +
	    ((inbyte & (1 << 1)) << 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 1) +
	    ((inbyte & (1 << 0)) << 2);
	  break;
	case 3:
	  *out0 +=
	    ((inbyte & (1 << 7)) >> 6) +
	    ((inbyte & (1 << 3)) >> 3);
	  *out1 +=
	    ((inbyte & (1 << 6)) >> 5) +
	    ((inbyte & (1 << 2)) >> 2);
	  *out2 +=
	    ((inbyte & (1 << 5)) >> 4) +
	    ((inbyte & (1 << 1)) >> 1);
	  *out3 +=
	    ((inbyte & (1 << 4)) >> 3) +
	    ((inbyte & (1 << 0)) >> 0);
	  out0++;
	  out1++;
	  out2++;
	  out3++;
	  break;
	}
      in++;
    }
}

static void
escp2_write_weave(FILE          *prn,	/* I - Print file or command */
		  int           length,	/* I - Length of bitmap data */
		  int           ydpi,	/* I - Vertical resolution */
		  int           model,	/* I - Printer model */
		  int           width,	/* I - Printed width */
		  int           offset,
		  int		xdpi,
		  unsigned char *c,
		  unsigned char *m,
		  unsigned char *y,
		  unsigned char *k,
		  unsigned char *C,
		  unsigned char *M)
{
  static int lineno = 0;
  static unsigned char s[4][3072];
  static unsigned char comp_buf[3072];
  unsigned char *comp_ptr;
  int i, j;
  unsigned char *cols[6];
  cols[0] = k;
  cols[1] = m;
  cols[2] = c;
  cols[3] = y;
  cols[4] = M;
  cols[5] = C;
  if (!c)
    is_monochrome = 1;
  else
    is_monochrome = 0;

  initialize_row(lineno, width);
  
  for (j = 0; j < 6; j++)
    {
      if (cols[j])
	{
	  if (vertical_subpasses > 1)
	    {
	      switch (oversample)
		{
		case 2:
		  escp2_unpack_2(length, cols[j], s[0], s[1]);
		  break;
		case 4:
		  escp2_unpack_4(length, cols[j], s[0], s[1], s[2], s[3]);
		  break;
		}
	      switch (vertical_subpasses / oversample)
		{
		case 4:
		  escp2_split_4(length, cols[j], s[0], s[1], s[2], s[3]);
		  break;
		case 2:
		  if (oversample == 1)
		    {		    
		      escp2_split_2(length, cols[j], s[0], s[1]);
		    }
		  else
		    {		    
		      escp2_split_2(length, s[1], s[1], s[3]);
		      escp2_split_2(length, s[0], s[0], s[2]);
		    }
		  break;
		}
	      for (i = 0; i < vertical_subpasses; i++)
		{
		  escp2_pack(s[i], ((length + oversample - 1) / oversample),
			     comp_buf, &comp_ptr);
		  add_to_row(lineno, comp_buf, comp_ptr - comp_buf,
			     colors[j], densities[j], i);
		}
	    }
	  else
	    {
	      escp2_pack(cols[j], length, comp_buf, &comp_ptr);
	      add_to_row(lineno, comp_buf, comp_ptr - comp_buf,
			 colors[j], densities[j], 0);
	    }
	}
    }
  finalize_row(lineno, model, width, offset, ydpi, xdpi, prn);
  lineno++;
}

/*
 * End of "$Id$".
 */
