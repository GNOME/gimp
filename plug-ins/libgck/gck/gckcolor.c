/***************************************************************************/
/* GCK - The General Convenience Kit. Generally useful conveniece routines */
/* for GIMP plug-in writers and users of the GDK/GTK libraries.            */
/* Copyright (C) 1996 Tom Bech                                             */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify    */
/* it under the terms of the GNU General Public License as published by    */
/* the Free Software Foundation; either version 2 of the License, or       */
/* (at your option) any later version.                                     */
/*                                                                         */
/* This program is distributed in the hope that it will be useful,         */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/* GNU General Public License for more details.                            */
/*                                                                         */
/* You should have received a copy of the GNU General Public License       */
/* along with this program; if not, write to the Free Software             */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,   */
/* USA.                                                                    */
/***************************************************************************/

/*************************************************************/
/* This file contains routines for creating and setting up   */
/* visuals. There's also routines for converting RGB(A) data */
/* to whatever format the current visual is.                 */
/*************************************************************/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "libgimp/gimpmath.h"

#include "gck.h"

#define RESERVED_COLORS 2

#define ROUND_TO_INT(val) ((val) + 0.5)

typedef struct
{
  guchar ready;
  GckRGB color;
} _GckSampleType;


/* returns a static storage */
static GdkColor *gck_rgb_to_gdkcolor       (GckVisualInfo *visinfo,
					    guchar r,guchar g,guchar b);

/* returns a malloc'ed area */
static GdkColor *gck_rgb_to_gdkcolor_r     (GckVisualInfo *visinfo,
					    guchar r,guchar g,guchar b);


/******************/
/* Implementation */
/******************/

/********************************************************/
/* This routine tries to allocate a biggest possible    */
/* color cube (used only on 8 bit pseudo color visuals) */
/* Shamelessly ripped from colormaps.c by S&P.          */
/* I'm not sure if I'm going to use this or not.        */
/********************************************************/

int gck_allocate_color_cube(GckVisualInfo * visinfo, int red, int green, int blue)
{
  int init_r, init_g, init_b;
  int total;
  int success;

  g_assert(visinfo!=NULL);

  init_r = red;
  init_g = green;
  init_b = blue;

  /* First, reduce number of total colors to fit a 8 bit LUT */
  /* ======================================================= */

  total = red * green * blue + RESERVED_COLORS;
  while (total > 256)
    {
      if (blue >= red && blue >= green)
	blue--;
      else if (red >= green && red >= blue)
	red--;
      else
	green--;

      total = red * green * blue + RESERVED_COLORS;
    }

  /* Now, attempt to allocate the colors. If no success, reduce the */
  /* color cube resolution and try again.                           */
  /* ============================================================== */

  success = gdk_colors_alloc(visinfo->colormap, 0, NULL, 0, visinfo->allocedpixels, total);
  while (!success)
    {
      if (blue >= red && blue >= green)
	blue--;
      else if (red >= green && red >= blue)
	red--;
      else
	green--;

      total = red * green * blue + RESERVED_COLORS;
      if (red <= 2 || green <= 2 || blue <= 2)
	success = 1;
      else
	success = gdk_colors_alloc(visinfo->colormap, 0, NULL, 0, visinfo->allocedpixels, total);
    }

  /* If any shades value has been reduced to nothing, return error flag */
  /* ================================================================== */

  if (red > 1 && green > 1 && blue > 1)
    {
      success=TRUE;
      visinfo->shades_r = red;
      visinfo->shades_g = green;
      visinfo->shades_b = blue;
      visinfo->numcolors = total;
    }
  else success=FALSE;

  return(success);
}

/**************************************************/
/* Create 8 bit RGB color cube. Also more or less */
/* ripped from colormaps.c by S&P.                */
/**************************************************/

void gck_create_8bit_rgb(GckVisualInfo * visinfo)
{
  unsigned int r, g, b;
  unsigned int dr, dg, db;
  int i = RESERVED_COLORS;

  g_assert(visinfo!=NULL);

  dr = (visinfo->shades_r > 1) ? (visinfo->shades_r - 1) : (1);
  dg = (visinfo->shades_g > 1) ? (visinfo->shades_g - 1) : (1);
  db = (visinfo->shades_b > 1) ? (visinfo->shades_b - 1) : (1);

  for (r = 0; r < visinfo->shades_r; r++)
    for (g = 0; g < visinfo->shades_g; g++)
      for (b = 0; b < visinfo->shades_b; b++)
	{
	  visinfo->colorcube[i] = visinfo->allocedpixels[i];

	  visinfo->rgbpalette[i].red = (guint) ROUND_TO_INT(255.0 * (double)(r * visinfo->visual->colormap_size) / (double)dr);
	  visinfo->rgbpalette[i].green = (guint) ROUND_TO_INT(255.0 * (double)(g * visinfo->visual->colormap_size) / (double)dg);
	  visinfo->rgbpalette[i].blue = (guint) ROUND_TO_INT(255.0 * (double)(b * visinfo->visual->colormap_size) / (double)db);	  visinfo->rgbpalette[i].pixel = visinfo->allocedpixels[i];
	  visinfo->indextab[r][g][b] = (guchar) visinfo->allocedpixels[i];
	  i++;
	}

  /* Set up mapping tables */
  /* ===================== */

  for (i = 0; i < 256; i++)
    {
      visinfo->map_r[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_r - 1) * ((double)i / 255.0)));
      visinfo->map_g[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_g - 1) * ((double)i / 255.0)));
      visinfo->map_b[i] = (int)ROUND_TO_INT(((double)(visinfo->shades_b - 1) * ((double)i / 255.0)));
      visinfo->invmap_r[i] = (double)visinfo->map_r[i]*(255.0/(double)(visinfo->shades_r - 1));
      visinfo->invmap_g[i] = (double)visinfo->map_g[i]*(255.0/(double)(visinfo->shades_g - 1));
      visinfo->invmap_b[i] = (double)visinfo->map_b[i]*(255.0/(double)(visinfo->shades_b - 1));
    }

  /* Create reserved colors */
  /* ====================== */

  visinfo->rgbpalette[0].red = 0;
  visinfo->rgbpalette[0].green = 0;
  visinfo->rgbpalette[0].blue = 0;
  visinfo->rgbpalette[0].pixel = visinfo->allocedpixels[0];

  visinfo->rgbpalette[1].red = 65535;
  visinfo->rgbpalette[1].green = 65535;
  visinfo->rgbpalette[1].blue = 65535;
  visinfo->rgbpalette[1].pixel = visinfo->allocedpixels[1];
}

/**********************************/
/* Get visual and create colormap */
/**********************************/

GckVisualInfo *gck_visualinfo_new(void)
{
  GckVisualInfo *visinfo;

  visinfo = (GckVisualInfo *) g_malloc(sizeof(GckVisualInfo));
  if (visinfo!=NULL)
    {
      visinfo->visual = gdk_visual_get_best();
      visinfo->colormap = gdk_colormap_new(visinfo->visual, FALSE);
      visinfo->dithermethod = DITHER_FLOYD_STEINBERG;
    
      if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
        {
          /* Allocate colormap and create RGB map */
          /* ==================================== */
    
          if (gck_allocate_color_cube(visinfo, 6, 6, 6) == TRUE)
	    {
	      gck_create_8bit_rgb(visinfo);
	      gdk_colors_store(visinfo->colormap, visinfo->rgbpalette, visinfo->numcolors);
	    }
          else
	    {
	      g_free(visinfo);
	      visinfo = NULL;
	    }
        }
    }

  return (visinfo);
}

/***********************************************************/
/* Free memory associated with the GckVisualInfo structure */
/***********************************************************/

void gck_visualinfo_destroy(GckVisualInfo * visinfo)
{
  g_assert(visinfo!=NULL);

  gdk_colormap_unref(visinfo->colormap);

  g_free(visinfo);
}

/*************************************************/
/* This converts from TrueColor RGB (24bpp) to   */
/* whatever format the visual of our image uses. */
/*************************************************/

GckDitherType gck_visualinfo_get_dither(GckVisualInfo * visinfo)
{
  g_assert(visinfo!=NULL);  
  return (visinfo->dithermethod);
}

void gck_visualinfo_set_dither(GckVisualInfo * visinfo, GckDitherType method)
{
  g_assert(visinfo!=NULL);
  visinfo->dithermethod = method;
}

/*******************/
/* GdkGC functions */
/*******************/

void gck_gc_set_foreground(GckVisualInfo *visinfo,GdkGC *gc,
                           guchar r, guchar g, guchar b)
{  
  g_assert(visinfo!=NULL);
  g_assert(gc!=NULL);

  gdk_gc_set_foreground(gc, gck_rgb_to_gdkcolor(visinfo,r,g,b));
}

void gck_gc_set_background(GckVisualInfo *visinfo,GdkGC *gc,
                           guchar r, guchar g, guchar b)
{
  g_assert(visinfo!=NULL);
  g_assert(gc!=NULL);

  gdk_gc_set_background(gc, gck_rgb_to_gdkcolor(visinfo,r,g,b));
}

/*************************************************/
/* RGB to 8 bpp pseudocolor (indexed) functions. */
/*************************************************/

/*
 * Non-reentrant function - GdkColor is a static storage
 */
static GdkColor *
gck_rgb_to_color8(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  static GdkColor color;
  gint index;

  g_assert(visinfo!=NULL);

  r = visinfo->map_r[r];
  g = visinfo->map_g[g];
  b = visinfo->map_b[b];
  index = visinfo->indextab[r][g][b];
  color=visinfo->rgbpalette[index];

  return (&color);
}

/*
 * Reentrant function - GdkColor will be malloc'ed
 */
static GdkColor *
gck_rgb_to_color8_r(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  gint index;
  GdkColor *color;

  g_assert(visinfo!=NULL);

  color=(GdkColor *)g_malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  r = visinfo->map_r[r];
  g = visinfo->map_g[g];
  b = visinfo->map_b[b];
  index = visinfo->indextab[r][g][b];
  *color=visinfo->rgbpalette[index];

  return (color);
}

/***************************************************/
/* RGB to 8 bpp pseudocolor using error-diffusion  */
/* dithering using the weights proposed by Floyd   */
/* and Steinberg (aka "Floyd-Steinberg dithering") */
/***************************************************/

static void
gck_rgb_to_image8_fs_dither(GckVisualInfo * visinfo, guchar * RGB_data, GdkImage * image,
			    int width, int height)
{
  guchar *imagedata;
  gint or, og, ob, mr, mg, mb, dr, dg, db;
  gint *row1, *row2, *temp, rowcnt;
  int xcnt, ycnt, diffx;
  long count = 0, rowsize;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  /* Allocate memory for FS errors */
  /* ============================= */

  rowsize = 3 * width;
  row1 = (gint *) g_malloc(sizeof(gint) * (size_t) rowsize);
  row2 = (gint *) g_malloc(sizeof(gint) * (size_t) rowsize);

  /* Initialize to zero */
  /* ================== */

  memset(row1, 0, 3 * sizeof(gint) * width);
  memset(row2, 0, 3 * sizeof(gint) * width);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  if (image->width < width)
    width = image->width;
  if (image->height < height)
    height = image->height;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      /* It's rec. to move from left to right on even  */
      /* rows and right to left on odd rows, so we do. */
      /* ============================================= */

      if ((ycnt % 1) == 0)
	{
	  rowcnt = 0;
	  for (xcnt = 0; xcnt < width; xcnt++)
	    {
	      /* Get exact (original) value */
	      /* ========================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      /* Extract and add the accumulated error for this pixel */
	      /* ==================================================== */

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      /* Make sure we don't run into an under- or overflow */
	      /* ================================================= */

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      /* Compute difference */
	      /* ================== */

	      dr = or - (gint) visinfo->invmap_r[or];
	      dg = og - (gint) visinfo->invmap_g[og];
	      db = ob - (gint) visinfo->invmap_b[ob];

	      /* Spread the error to the neighboring pixels.    */
	      /* We use the weights proposed by Floyd-Steinberg */
	      /* for 3x3 neighborhoods (1*, 3*, 5* and 7*1/16). */
	      /* ============================================== */

	      if (xcnt < width - 1)
		{
		  row1[(rowcnt + 3)] += 7 * (gint) dr;
		  row1[(rowcnt + 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt + 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt + 3)] += (gint) dr;
		      row2[(rowcnt + 3) + 1] += (gint) dg;
		      row2[(rowcnt + 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt > 0 && ycnt < height - 1)
		{
		  row2[(rowcnt - 3)] += 3 * (gint) dr;
		  row2[(rowcnt - 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt - 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      /* Clear the errorvalues of the processed row */
	      /* ========================================== */

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      /* Map RGB values to color cube and write pixel */
	      /* ============================================ */

	      mr = visinfo->map_r[(guchar) or];
	      mg = visinfo->map_g[(guchar) og];
	      mb = visinfo->map_b[(guchar) ob];

	      imagedata[xcnt] = visinfo->indextab[mr][mg][mb];
	      rowcnt += 3;
	    }
	}
      else
	{
	  rowcnt = rowsize - 3;
	  for (xcnt = width - 1; xcnt >= 0; xcnt--)
	    {
	      /* Same as above but in the other direction */
	      /* ======================================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      dr = or - (gint) visinfo->invmap_r[or];
	      dg = og - (gint) visinfo->invmap_g[og];
	      db = ob - (gint) visinfo->invmap_b[ob];

	      if (xcnt > 0)
		{
		  row1[(rowcnt - 3)] += 7 * (gint) dr;
		  row1[(rowcnt - 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt - 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt - 3)] += (gint) dr;
		      row2[(rowcnt - 3) + 1] += (gint) dg;
		      row2[(rowcnt - 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt < width - 1 && ycnt < height - 1)
		{
		  row2[(rowcnt + 3)] += 3 * (gint) dr;
		  row2[(rowcnt + 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt + 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      mr = visinfo->map_r[(guchar) or];
	      mg = visinfo->map_g[(guchar) og];
	      mb = visinfo->map_b[(guchar) ob];

	      imagedata[xcnt] = visinfo->indextab[mr][mg][mb];
	      rowcnt -= 3;
	    }
	}

      /* We're finished with this row, swap row-pointers */
      /* =============================================== */

      temp = row1;
      row1 = row2;
      row2 = temp;

      imagedata += width + diffx;
      count += rowsize;
    }

  g_free(row1);
  g_free(row2);
}

/***********************************************************/
/* Plain (no dithering) RGB to 8 bpp pseudocolor (indexed) */
/***********************************************************/

static void
gck_rgb_to_image8(GckVisualInfo * visinfo,
		  guchar * RGB_data,
		  GdkImage * image,
		  int width, int height)
{
  guchar *imagedata, r, g, b;
  int xcnt, ycnt, diffx;
  long count = 0;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      r = RGB_data[count];
	      g = RGB_data[count + 1];
	      b = RGB_data[count + 2];

	      r = visinfo->map_r[r];
	      g = visinfo->map_g[g];
	      b = visinfo->map_b[b];

	      *imagedata = visinfo->indextab[r][g][b];
	      imagedata++;
	    }
	  count += 3;
	}
      imagedata += diffx;
    }
}

/************************************/
/* RGB to 16/15 bpp RGB ("HiColor") */
/************************************/

/*
 * Non-reentrant function - GdkColor is a static storage
 */
static GdkColor *
gck_rgb_to_color16(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  static GdkColor color;
  guint32 red, green, blue;

  g_assert(visinfo!=NULL);

  color.red = ((guint16) r) << 8;
  color.green = ((guint16) g) << 8;
  color.blue = ((guint16) b) << 8;

  red = ((guint32) r) >> (8 - visinfo->visual->red_prec);
  green = ((guint32) g) >> (8 - visinfo->visual->green_prec);
  blue = ((guint32) b) >> (8 - visinfo->visual->blue_prec);

  red = red << visinfo->visual->red_shift;
  green = green << visinfo->visual->green_shift;
  blue = blue << visinfo->visual->blue_shift;

  color.pixel = red | green | blue;

  return (&color);
}

/*
 * Reentrant function - GdkColor will be malloc'ed
 */
static GdkColor *
gck_rgb_to_color16_r(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_assert(visinfo!=NULL);

  color=(GdkColor *)g_malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r) >> (8 - visinfo->visual->red_prec);
  green = ((guint32) g) >> (8 - visinfo->visual->green_prec);
  blue = ((guint32) b) >> (8 - visinfo->visual->blue_prec);

  red = red << visinfo->visual->red_shift;
  green = green << visinfo->visual->green_shift;
  blue = blue << visinfo->visual->blue_shift;

  color->pixel = red | green | blue;

  return (color);
}

/***************************************************/
/* RGB to 16 bpp truecolor using error-diffusion   */
/* dithering using the weights proposed by Floyd   */
/* and Steinberg (aka "Floyd-Steinberg dithering") */
/***************************************************/

static void 
gck_rgb_to_image16_fs_dither(GckVisualInfo * visinfo,
			     guchar * RGB_data,
			     GdkImage * image,
			     int width, int height)
{
  guint16 *imagedata, pixel;
  gint16 or, og, ob, dr, dg, db;
  gint16 *row1, *row2, *temp, rowcnt, rmask, gmask, bmask;
  int xcnt, ycnt, diffx;
  long count = 0, rowsize;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  /* Allocate memory for FS errors */
  /* ============================= */

  rowsize = 3 * width;
  row1 = (gint16 *) g_malloc(sizeof(gint16) * (size_t) rowsize);
  row2 = (gint16 *) g_malloc(sizeof(gint16) * (size_t) rowsize);

  /* Initialize to zero */
  /* ================== */

  memset(row1, 0, 3 * sizeof(gint16) * width);
  memset(row2, 0, 3 * sizeof(gint16) * width);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  if (image->width < width)
    width = image->width;
  if (image->height < height)
    height = image->height;

  rmask = (0xff << (8 - visinfo->visual->red_prec)) & 0xff;
  gmask = (0xff << (8 - visinfo->visual->green_prec)) & 0xff;
  bmask = (0xff << (8 - visinfo->visual->blue_prec)) & 0xff;

  imagedata = (guint16 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      /* It is rec. to move from left to right on even */
      /* rows and right to left on odd rows, so we do. */
      /* ============================================= */

      if ((ycnt % 1) == 0)
	{
	  rowcnt = 0;
	  for (xcnt = 0; xcnt < width; xcnt++)
	    {
	      /* Get exact (original) value */
	      /* ========================== */

	      pixel = 0;
	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      /* Extract and add the accumulated error for this pixel */
	      /* ==================================================== */

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      /* Make sure we don't run into an under- or overflow */
	      /* ================================================= */

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      /* Compute difference */
	      /* ================== */

	      dr = or - (or & rmask);
	      dg = og - (og & gmask);
	      db = ob - (ob & bmask);

	      /* Spread the error to the neighboring pixels.    */
	      /* We use the weights proposed by Floyd-Steinberg */
	      /* for 3x3 neighborhoods (1*, 3*, 5* and 7*1/16). */
	      /* ============================================== */

	      if (xcnt < width - 1)
		{
		  row1[(rowcnt + 3)] += 7 * (gint) dr;
		  row1[(rowcnt + 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt + 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt + 3)] += (gint) dr;
		      row2[(rowcnt + 3) + 1] += (gint) dg;
		      row2[(rowcnt + 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt > 0 && ycnt < height - 1)
		{
		  row2[(rowcnt - 3)] += 3 * (gint) dr;
		  row2[(rowcnt - 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt - 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      /* Clear the errorvalues of the processed row */
	      /* ========================================== */

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      or = or >> (8 - visinfo->visual->red_prec);
	      og = og >> (8 - visinfo->visual->green_prec);
	      ob = ob >> (8 - visinfo->visual->blue_prec);

	      or = or << visinfo->visual->red_shift;
	      og = og << visinfo->visual->green_shift;
	      ob = ob << visinfo->visual->blue_shift;

	      pixel = pixel | or | og | ob;

	      imagedata[xcnt] = pixel;
	      rowcnt += 3;
	    }
	}
      else
	{
	  rowcnt = rowsize - 3;
	  for (xcnt = width - 1; xcnt >= 0; xcnt--)
	    {
	      /* Same as above but in the other direction */
	      /* ======================================== */

	      or = (gint) RGB_data[count + rowcnt];
	      og = (gint) RGB_data[count + rowcnt + 1];
	      ob = (gint) RGB_data[count + rowcnt + 2];

	      or = or + (row1[rowcnt] >> 4);
	      og = og + (row1[rowcnt + 1] >> 4);
	      ob = ob + (row1[rowcnt + 2] >> 4);

	      if (or > 255)
		or = 255;
	      else if (or < 0)
		or = 0;
	      if (og > 255)
		og = 255;
	      else if (og < 0)
		og = 0;
	      if (ob > 255)
		ob = 255;
	      else if (ob < 0)
		ob = 0;

	      dr = or - (or & rmask);
	      dg = og - (og & gmask);
	      db = ob - (ob & bmask);

	      if (xcnt > 0)
		{
		  row1[(rowcnt - 3)] += 7 * (gint) dr;
		  row1[(rowcnt - 3) + 1] += 7 * (gint) dg;
		  row1[(rowcnt - 3) + 2] += 7 * (gint) db;
		  if (ycnt < height - 1)
		    {
		      row2[(rowcnt - 3)] += (gint) dr;
		      row2[(rowcnt - 3) + 1] += (gint) dg;
		      row2[(rowcnt - 3) + 2] += (gint) db;
		    }
		}

	      if (xcnt < width - 1 && ycnt < height - 1)
		{
		  row2[(rowcnt + 3)] += 3 * (gint) dr;
		  row2[(rowcnt + 3) + 1] += 3 * (gint) dg;
		  row2[(rowcnt + 3) + 2] += 3 * (gint) db;
		  row2[rowcnt] += 5 * (gint) dr;
		  row2[rowcnt + 1] += 5 * (gint) dg;
		  row2[rowcnt + 2] += 5 * (gint) db;
		}

	      row1[rowcnt] = row1[rowcnt + 1] = row1[rowcnt + 2] = 0;

	      or = or >> (8 - visinfo->visual->red_prec);
	      og = og >> (8 - visinfo->visual->green_prec);
	      ob = ob >> (8 - visinfo->visual->blue_prec);

	      or = or << visinfo->visual->red_shift;
	      og = og << visinfo->visual->green_shift;
	      ob = ob << visinfo->visual->blue_shift;

	      pixel = pixel | or | og | ob;

	      imagedata[xcnt] = pixel;
	      rowcnt -= 3;
	    }
	}

      /* We're finished with this row, swap row-pointers */
      /* =============================================== */

      temp = row1;
      row1 = row2;
      row2 = temp;

      imagedata += width + diffx;
      count += rowsize;
    }

  g_free(row1);
  g_free(row2);
}

static void
gck_rgb_to_image16(GckVisualInfo * visinfo,
		   guchar * RGB_data,
		   GdkImage * image,
		   int width, int height)
{
  guint16 *imagedata, pixel, r, g, b;
  int xcnt, ycnt, diffx;
  long count = 0;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;
  else
    diffx = 0;

  imagedata = (guint16 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt <= image->width && ycnt <= image->height)
	    {
	      pixel = 0;
	      r = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->red_prec);
	      g = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->green_prec);
	      b = ((guint16) RGB_data[count++]) >> (8 - visinfo->visual->blue_prec);

	      r = r << visinfo->visual->red_shift;
	      g = g << visinfo->visual->green_shift;
	      b = b << visinfo->visual->blue_shift;

	      pixel = pixel | r | g | b;

	      *imagedata = pixel;
	      imagedata++;
	    }
	}
      imagedata += diffx;
    }
}

/************************/
/* RGB to RGB (sic!) :) */
/************************/

/*
 * Non-reentrant function - GdkColor is a static storage
 */
static GdkColor *
gck_rgb_to_color24(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  static GdkColor color;
  guint32 red, green, blue;

  g_assert(visinfo!=NULL);

  color.red = ((guint16) r) << 8;
  color.green = ((guint16) g) << 8;
  color.blue = ((guint16) b) << 8;

  red = ((guint32) r << 16);
  green = ((guint32) g) << 8;
  blue = ((guint32) b);

  color.pixel = red | green | blue;

  return (&color);
}

/*
 * Reentrant function - GdkColor will be malloc'ed
 */
static GdkColor *
gck_rgb_to_color24_r(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_assert(visinfo!=NULL);

  color=(GdkColor *)g_malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r << 16);
  green = ((guint32) g) << 8;
  blue = ((guint32) b);

  color->pixel = red | green | blue;

  return (color);
}

static void
gck_rgb_to_image24(GckVisualInfo * visinfo,
		   guchar * RGB_data,
		   GdkImage * image,
		   int width, int height)
{
  guchar *imagedata;
  int xcnt, ycnt, diffx;
  long count = 0, count2 = 0;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = 3 * (image->width - width);
  else
    diffx = 0;

  imagedata = (guchar *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < height; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      imagedata[count2++] = RGB_data[count + 2];
	      imagedata[count2++] = RGB_data[count + 1];
	      imagedata[count2++] = RGB_data[count];
	    }
	  count += 3;
	}
      count2 += diffx;
    }
}

/***************/
/* RGB to RGBX */
/***************/

/*
 * Non-reentrant function - GdkColor is a static storage
 */
static GdkColor *
gck_rgb_to_color32(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  static GdkColor color;
  guint32 red, green, blue;

  g_assert(visinfo!=NULL);

  color.red = ((guint16) r) << 8;
  color.green = ((guint16) g) << 8;
  color.blue = ((guint16) b) << 8;

  red = ((guint32) r) << 8;
  green = ((guint32) g) << 16;
  blue = ((guint32) b) << 24;

  color.pixel = red | green | blue;

  return (&color);
}

/*
 * Reentrant function - GdkColor will be malloc'ed
 */
static GdkColor *
gck_rgb_to_color32_r(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  guint32 red, green, blue;
  GdkColor *color;

  g_assert(visinfo!=NULL);

  color=(GdkColor *)g_malloc(sizeof(GdkColor));
  if (color==NULL)
    return(NULL);

  color->red = ((guint16) r) << 8;
  color->green = ((guint16) g) << 8;
  color->blue = ((guint16) b) << 8;

  red = ((guint32) r) << 8;
  green = ((guint32) g) << 16;
  blue = ((guint32) b) << 24;

  color->pixel = red | green | blue;

  return (color);
}

static void
gck_rgb_to_image32(GckVisualInfo * visinfo,
		   guchar * RGB_data,
		   GdkImage * image,
		   int width, int height)
{
  guint32 *imagedata, pixel, r, g, b;
  int xcnt, ycnt, diffx=0;
  long count = 0;

  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (width < image->width)
    diffx = image->width - width;

  imagedata = (guint32 *) image->mem;
  for (ycnt = 0; ycnt < height; ycnt++)
    {
      for (xcnt = 0; xcnt < width; xcnt++)
	{
	  if (xcnt < image->width && ycnt < image->height)
	    {
	      r = (guint32) RGB_data[count++];
	      g = (guint32) RGB_data[count++];
	      b = (guint32) RGB_data[count++];

	      /* changed to work on 32 bit displays */
	      r = r << 16;
	      g = g << 8;
	      b = b;

	      pixel = r | g | b;
	      *imagedata = pixel;
	      imagedata++;
	    }
	}
      imagedata += diffx;
    }
}

/**************************/
/* Conversion dispatchers */
/**************************/

void gck_rgb_to_gdkimage(GckVisualInfo * visinfo,
                         guchar * RGB_data,
                         GdkImage * image,
                         int width, int height)
{
  g_assert(visinfo!=NULL);
  g_assert(RGB_data!=NULL);
  g_assert(image!=NULL);

  if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      if (visinfo->visual->depth == 8)
	{
	  /* Standard 256 color display */
	  /* ========================== */

	  if (visinfo->dithermethod == DITHER_NONE)
	    gck_rgb_to_image8(visinfo, RGB_data, image, width, height);
	  else if (visinfo->dithermethod == DITHER_FLOYD_STEINBERG)
	    gck_rgb_to_image8_fs_dither(visinfo, RGB_data, image, width, height);
	}
    }
  else if (visinfo->visual->type == GDK_VISUAL_TRUE_COLOR ||
	 visinfo->visual->type == GDK_VISUAL_DIRECT_COLOR)
    {
      if (visinfo->visual->depth == 15 || visinfo->visual->depth == 16)
	{
	  /* HiColor modes */
	  /* ============= */

	  if (visinfo->dithermethod == DITHER_NONE)
	    gck_rgb_to_image16(visinfo, RGB_data, image, width, height);
	  else if (visinfo->dithermethod == DITHER_FLOYD_STEINBERG)
            gck_rgb_to_image16_fs_dither(visinfo, RGB_data, image, width, height);
	}
      else if (visinfo->visual->depth == 24 && image->bpp==3)
	{
	  /* Packed 24 bit mode */
	  /* ================== */

	  gck_rgb_to_image24(visinfo, RGB_data, image, width, height);
	}
      else if (visinfo->visual->depth == 32 || (visinfo->visual->depth == 24 && image->bpp==4))
	{
	  /* 32 bpp mode */
	  /* =========== */

	  gck_rgb_to_image32(visinfo, RGB_data, image, width, height);
	}
    }
}

/*
 * Non-reentrant function - GdkColor is a static storage
 */
static GdkColor *
gck_rgb_to_gdkcolor(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  GdkColor *color=NULL;

  g_assert(visinfo!=NULL);

  if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      if (visinfo->visual->depth == 8)
	{
	  /* Standard 256 color display */
	  /* ========================== */

	  color=gck_rgb_to_color8(visinfo, r, g, b);
	}
    }
  else if (visinfo->visual->type == GDK_VISUAL_TRUE_COLOR ||
	 visinfo->visual->type == GDK_VISUAL_DIRECT_COLOR)
    {
      if (visinfo->visual->depth == 15 || visinfo->visual->depth == 16)
	{
	  /* HiColor modes */
	  /* ============= */

	  color=gck_rgb_to_color16(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 24)
	{
	  /* Normal 24 bit mode */
	  /* ================== */

	  color=gck_rgb_to_color24(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 32)
	{
	  /* 32 bpp mode */
	  /* =========== */

	  color=gck_rgb_to_color32(visinfo, r, g, b);
	}
    }

  return (color);
}

/*
 * Reentrant function - GdkColor will be malloc'ed
 */
static GdkColor *
gck_rgb_to_gdkcolor_r(GckVisualInfo * visinfo, guchar r, guchar g, guchar b)
{
  GdkColor *color=NULL;

  g_assert(visinfo!=NULL);

  if (visinfo->visual->type == GDK_VISUAL_PSEUDO_COLOR)
    {
      if (visinfo->visual->depth == 8)
	{
	  /* Standard 256 color display */
	  /* ========================== */

	  color=gck_rgb_to_color8_r(visinfo, r, g, b);
	}
    }
  else if (visinfo->visual->type == GDK_VISUAL_TRUE_COLOR ||
	 visinfo->visual->type == GDK_VISUAL_DIRECT_COLOR)
    {
      if (visinfo->visual->depth == 15 || visinfo->visual->depth == 16)
	{
	  /* HiColor modes */
	  /* ============= */

	  color=gck_rgb_to_color16_r(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 24)
	{
	  /* Normal 24 bit mode */
	  /* ================== */

	  color=gck_rgb_to_color24_r(visinfo, r, g, b);
	}
      else if (visinfo->visual->depth == 32)
	{
	  /* 32 bpp mode */
	  /* =========== */

	  color=gck_rgb_to_color32_r(visinfo, r, g, b);
	}
    }

  return (color);
}

/********************/
/* Color operations */
/********************/

/******************************************/
/* Bilinear interpolation stuff (Quartic) */
/******************************************/

double gck_bilinear(double x, double y, double *values)
{
  double xx, yy, m0, m1;

  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  return ((1.0 - yy) * m0 + yy * m1);
}

guchar gck_bilinear_8(double x, double y, guchar * values)
{
  double xx, yy, m0, m1;

  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  return ((guchar) ((1.0 - yy) * m0 + yy * m1));
}

guint16 gck_bilinear_16(double x, double y, guint16 * values)
{
  double xx, yy, m0, m1;

  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  return ((guint16) ((1.0 - yy) * m0 + yy * m1));
}

guint32 gck_bilinear_32(double x, double y, guint32 * values)
{
  double xx, yy, m0, m1;

  g_assert(values!=NULL);

  xx = fmod(x, 1.0);
  yy = fmod(y, 1.0);

  if (x < 0.0)
    x += 1.0;
  if (y < 0.0)
    y += 1.0;

  m0 = (1.0 - xx) * values[0] + xx * values[1];
  m1 = (1.0 - xx) * values[2] + xx * values[3];

  return ((guint32) ((1.0 - yy) * m0 + yy * m1));
}

GckRGB gck_bilinear_rgb(double x, double y, GckRGB *values)
{
  double m0, m1;
  double ix, iy;
  GckRGB v;

  g_assert(values!=NULL);

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */
  /* === */

  m0 = ix * values[0].r + x * values[1].r;
  m1 = ix * values[2].r + x * values[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */
  /* ===== */

  m0 = ix * values[0].g + x * values[1].g;
  m1 = ix * values[2].g + x * values[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */
  /* ==== */

  m0 = ix * values[0].b + x * values[1].b;
  m1 = ix * values[2].b + x * values[3].b;

  v.b = iy * m0 + y * m1;

  return (v);
}				/* bilinear */

GckRGB gck_bilinear_rgba(double x, double y, GckRGB *values)
{
  double m0, m1;
  double ix, iy;
  GckRGB v;

  g_assert(values!=NULL);

  x = fmod(x, 1.0);
  y = fmod(y, 1.0);

  if (x < 0)
    x += 1.0;
  if (y < 0)
    y += 1.0;

  ix = 1.0 - x;
  iy = 1.0 - y;

  /* Red */
  /* === */

  m0 = ix * values[0].r + x * values[1].r;
  m1 = ix * values[2].r + x * values[3].r;

  v.r = iy * m0 + y * m1;

  /* Green */
  /* ===== */

  m0 = ix * values[0].g + x * values[1].g;
  m1 = ix * values[2].g + x * values[3].g;

  v.g = iy * m0 + y * m1;

  /* Blue */
  /* ==== */

  m0 = ix * values[0].b + x * values[1].b;
  m1 = ix * values[2].b + x * values[3].b;

  v.b = iy * m0 + y * m1;

  /* Alpha */
  /* ===== */

  m0 = ix * values[0].a + x * values[1].a;
  m1 = ix * values[2].a + x * values[3].a;

  v.a = iy * m0 + y * m1;

  return (v);
}				/* bilinear */

/********************************/
/* Multiple channels operations */
/********************************/

void gck_rgb_add(GckRGB * p, GckRGB * q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r + q->r;
  p->g = p->g + q->g;
  p->b = p->b + q->b;
  
}

void gck_rgb_sub(GckRGB * p, GckRGB * q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r - q->r;
  p->g = p->g - q->g;
  p->b = p->b - q->b;
}

void gck_rgb_mul(GckRGB * p, double b)
{
  g_assert(p!=NULL);

  p->r = p->r * b;
  p->g = p->g * b;
  p->b = p->b * b;
}

double gck_rgb_dist(GckRGB * p, GckRGB * q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  return (fabs(p->r - q->r) + fabs(p->g - q->g) + fabs(p->b - q->b));
}

double gck_rgb_max(GckRGB * p)
{
  double max;

  g_assert(p!=NULL);

  max = p->r;
  if (p->g > max)
    max = p->g;
  if (p->b > max)
    max = p->b;

  return (max);
}

double gck_rgb_min(GckRGB * p)
{
  double min;

  g_assert(p!=NULL);

  min=p->r;
  if (p->g < min)
    min = p->g;
  if (p->b < min)
    min = p->b;

  return (min);
}

void gck_rgb_clamp(GckRGB * p)
{
  g_assert(p!=NULL);

  if (p->r > 1.0)
    p->r = 1.0;
  if (p->g > 1.0)
    p->g = 1.0;
  if (p->b > 1.0)
    p->b = 1.0;
  if (p->r < 0.0)
    p->r = 0.0;
  if (p->g < 0.0)
    p->g = 0.0;
  if (p->b < 0.0)
    p->b = 0.0;
}

void gck_rgb_set(GckRGB * p, double r, double g, double b)
{
  g_assert(p!=NULL);

  p->r = r;
  p->g = g;
  p->b = b;

}

void gck_rgb_gamma(GckRGB * p, double gamma)
{
  double ig;

  g_assert(p!=NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    (ig = 0.0);

  p->r = pow(p->r, ig);
  p->g = pow(p->g, ig);
  p->b = pow(p->b, ig);
}

void gck_rgba_add(GckRGB * p, GckRGB * q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r + q->r;
  p->g = p->g + q->g;
  p->b = p->b + q->b;
  p->a = p->a + q->a;
}

void gck_rgba_sub(GckRGB * p, GckRGB * q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  p->r = p->r - q->r;
  p->g = p->g - q->g;
  p->b = p->b - q->b;
  p->a = p->a - q->a;
}

void gck_rgba_mul(GckRGB * p, double b)
{
  g_assert(p!=NULL);

  p->r = p->r * b;
  p->g = p->g * b;
  p->b = p->b * b;
  p->a = p->a * b;
}

double gck_rgba_dist(GckRGB *p, GckRGB *q)
{
  g_assert(p!=NULL);
  g_assert(q!=NULL);

  return (fabs(p->r - q->r) + fabs(p->g - q->g) +
	  fabs(p->b - q->b) + fabs(p->a - q->a));
}

/* These two are probably not needed */

double gck_rgba_max(GckRGB * p)
{
  double max;

  g_assert(p!=NULL);

  max = p->r;

  if (p->g > max)
    max = p->g;
  if (p->b > max)
    max = p->b;
  if (p->a > max)
    max = p->a;

  return (max);
}

double gck_rgba_min(GckRGB * p)
{
  double min;

  g_assert(p!=NULL);
  
  min = p->r;
  if (p->g < min)
    min = p->g;
  if (p->b < min)
    min = p->b;
  if (p->a < min)
    min = p->a;

  return (min);
}

void gck_rgba_clamp(GckRGB * p)
{
  g_assert(p!=NULL);

  if (p->r > 1.0)
    p->r = 1.0;
  if (p->g > 1.0)
    p->g = 1.0;
  if (p->b > 1.0)
    p->b = 1.0;
  if (p->a > 1.0)
    p->a = 1.0;
  if (p->r < 0.0)
    p->r = 0.0;
  if (p->g < 0.0)
    p->g = 0.0;
  if (p->b < 0.0)
    p->b = 0.0;
  if (p->a < 0.0)
    p->a = 0.0;
}

void gck_rgba_set(GckRGB * p, double r, double g, double b, double a)
{
  g_assert(p!=NULL);

  p->r = r;
  p->g = g;
  p->b = b;
  p->a = a;
}

/* This one is also not needed */

void gck_rgba_gamma(GckRGB * p, double gamma)
{
  double ig;

  g_assert(p!=NULL);

  if (gamma != 0.0)
    ig = 1.0 / gamma;
  else
    (ig = 0.0);

  p->r = pow(p->r, ig);
  p->g = pow(p->g, ig);
  p->b = pow(p->b, ig);
  p->a = pow(p->a, ig);
}

/**************************/
/* Colorspace conversions */
/**************************/

/***********************************************/
/* (Red,Green,Blue) <-> (Hue,Saturation,Value) */
/***********************************************/

void gck_rgb_to_hsv(GckRGB * p, double *h, double *s, double *v)
{
  double max,min,delta;

  g_assert(p!=NULL);
  g_assert(h!=NULL);
  g_assert(s!=NULL);
  g_assert(v!=NULL);

  max = gck_rgb_max(p);
  min = gck_rgb_min(p);

  *v = max;
  if (max != 0.0)
    {
      *s = (max - min) / max;
    }
  else
    *s = 0.0;
  if (*s == 0.0)
    *h = GCK_HSV_UNDEFINED;
  else
    {
      delta = max - min;
      if (p->r == max)
	{
	  *h = (p->g - p->b) / delta;
	}
      else if (p->g == max)
	{
	  *h = 2.0 + (p->b - p->r) / delta;
	}
      else if (p->b == max)
	{
	  *h = 4.0 + (p->r - p->g) / delta;
	}
      *h = *h * 60.0;
      if (*h < 0.0)
	*h = *h + 360;
    }
}

void gck_hsv_to_rgb(double h, double s, double v, GckRGB * p)
{
  int i;
  double f, w, q, t;

  g_assert(p!=NULL);

  if (s == 0.0)
    {
      if (h == GCK_HSV_UNDEFINED)
	{
	  p->r = v;
	  p->g = v;
	  p->b = v;
	}
    }
  else
    {
      if (h == 360.0)
	h = 0.0;
      h = h / 60.0;
      i = (int)h;
      f = h - i;
      w = v * (1.0 - s);
      q = v * (1.0 - (s * f));
      t = v * (1.0 - (s * (1.0 - f)));
      switch (i)
	{
	case 0:
	  p->r = v;
	  p->g = t;
	  p->b = w;
	  break;
	case 1:
	  p->r = q;
	  p->g = v;
	  p->b = w;
	  break;
	case 2:
	  p->r = w;
	  p->g = v;
	  p->b = t;
	  break;
	case 3:
	  p->r = w;
	  p->g = q;
	  p->b = v;
	  break;
	case 4:
	  p->r = t;
	  p->g = w;
	  p->b = v;
	  break;
	case 5:
	  p->r = v;
	  p->g = w;
	  p->b = q;
	  break;
	}
    }
}

/***************************************************/
/* (Red,Green,Blue) <-> (Hue,Saturation,Lightness) */
/***************************************************/

void gck_rgb_to_hsl(GckRGB * p, double *h, double *s, double *l)
{
  double max,min,delta;

  g_assert(p!=NULL);
  g_assert(h!=NULL);
  g_assert(s!=NULL);
  g_assert(l!=NULL);

  max = gck_rgb_max(p);
  min = gck_rgb_min(p);

  *l = (max + min) / 2.0;

  if (max == min)
    {
      *s = 0.0;
      *h = GCK_HSL_UNDEFINED;
    }
  else
    {
      if (*l <= 0.5)
	*s = (max - min) / (max + min);
      else
	*s = (max - min) / (2.0 - max - min);

      delta = max - min;
      if (p->r == max)
	{
	  *h = (p->g - p->b) / delta;
	}
      else if (p->g == max)
	{
	  *h = 2.0 + (p->b - p->r) / delta;
	}
      else if (p->b == max)
	{
	  *h = 4.0 + (p->r - p->g) / delta;
	}
      *h = *h * 60.0;
      if (*h < 0.0)
	*h = *h + 360.0;
    }
}

double _gck_value(double n1, double n2, double hue)
{
  double val;

  if (hue > 360.0)
    hue = hue - 360.0;
  else if (hue < 0.0)
    hue = hue + 360.0;
  if (hue < 60.0)
    val = n1 + (n2 - n1) * hue / 60.0;
  else if (hue < 180.0)
    val = n2;
  else if (hue < 240.0)
    val = n1 + (n2 - n1) * (240.0 - hue) / 60.0;
  else
    val = n1;

  return (val);
}

void gck_hsl_to_rgb(double h, double s, double l, GckRGB * p)
{
  double m1, m2;

  g_assert(p!=NULL);

  if (l <= 0.5)
    m2 = l * (l + s);
  else
    m2 = l + s + l * s;
  m1 = 2.0 * l - m2;

  if (s == 0)
    {
      if (h == GCK_HSV_UNDEFINED)
	p->r = p->g = p->b = 1.0;
    }
  else
    {
      p->r = _gck_value(m1, m2, h + 120.0);
      p->g = _gck_value(m1, m2, h);
      p->b = _gck_value(m1, m2, h - 120.0);
    }
}

#define GCK_RETURN_RGB(x, y, z) {rgb->r = x; rgb->g = y; rgb->b = z; return; }

/***********************************************************************************/
/* Theoretically, hue 0 (pure red) is identical to hue 6 in these transforms. Pure */
/* red always maps to 6 in this implementation. Therefore UNDEFINED can be         */
/* defined as 0 in situations where only unsigned numbers are desired.             */
/***********************************************************************************/

void gck_rgb_to_hwb(GckRGB *rgb, gdouble *hue,gdouble *whiteness,gdouble *blackness)
{
  /* RGB are each on [0, 1]. W and B are returned on [0, 1] and H is        */
  /* returned on [0, 6]. Exception: H is returned UNDEFINED if W ==  1 - B. */
  /* ====================================================================== */

  gdouble R = rgb->r, G = rgb->g, B = rgb->b, w, v, b, f;
  gint i;

  w = gck_rgb_min(rgb);
  v = gck_rgb_max(rgb);
  b = 1.0 - v;
  
  if (v == w)
    {
      *hue=GCK_HSV_UNDEFINED;
      *whiteness=w;
      *blackness=b;
    }
  else
    {
      f = (R == w) ? G - B : ((G == w) ? B - R : R - G);
      i = (R == w) ? 3.0 : ((G == w) ? 5.0 : 1.0);
    
      *hue=(360.0/6.0)*(i - f /(v - w));
      *whiteness=w;
      *blackness=b;
    }
}

void gck_hwb_to_rgb(gdouble H,gdouble W, gdouble B, GckRGB *rgb)
{
  /* H is given on [0, 6] or UNDEFINED. W and B are given on [0, 1]. */
  /* RGB are each returned on [0, 1].                                */
  /* =============================================================== */
  
  gdouble h = H, w = W, b = B, v, n, f;
  gint i;

  h=6.0*h/360.0;
    
  v = 1.0 - b;
  if (h == GCK_HSV_UNDEFINED)
    {
      rgb->r=v;
      rgb->g=v;
      rgb->b=v;
    }
  else
    {
      i = floor(h);
      f = h - i;
      if (i & 1) f = 1.0 - f;  /* if i is odd */
      n = w + f * (v - w);     /* linear interpolation between w and v */
    
      switch (i)
        {
          case 6:
          case 0: GCK_RETURN_RGB(v, n, w);
            break;
          case 1: GCK_RETURN_RGB(n, v, w);
            break;
          case 2: GCK_RETURN_RGB(w, v, n);
            break;
          case 3: GCK_RETURN_RGB(w, n, v);
            break;
          case 4: GCK_RETURN_RGB(n, w, v);
            break;
          case 5: GCK_RETURN_RGB(v, w, n);
            break;
        }
    }

}

/*********************************************************************/
/* Sumpersampling code (Quartic)                                     */
/* This code is *largely* based on the sources for POV-Ray 3.0. I am */
/* grateful to the POV-Team for such a great program and for making  */
/* their sources available.  All comments / bug reports /            */
/* etc. regarding this code should be addressed to me, not to the    */
/* POV-Ray team.  Any bugs are my responsibility, not theirs.        */
/*********************************************************************/

gulong gck_render_sub_pixel(int max_depth, int depth, _GckSampleType ** block,
			    int x, int y, int x1, int y1, int x3, int y3, double threshold,
			    int sub_pixel_size, GckRenderFunction render_func, GckRGB * color)
{
  int x2, y2, cnt;		/* Coords of center sample */
  double dx1, dy1;		/* Delta to upper left sample */
  double dx3, dy3, weight;	/* Delta to lower right sample */
  GckRGB c[4],tmpcol;
  unsigned long num_samples = 0;

  /* Get offsets for corners */
  /* ======================= */

  dx1 = (double)(x1 - sub_pixel_size / 2) / sub_pixel_size;
  dx3 = (double)(x3 - sub_pixel_size / 2) / sub_pixel_size;

  dy1 = (double)(y1 - sub_pixel_size / 2) / sub_pixel_size;
  dy3 = (double)(y3 - sub_pixel_size / 2) / sub_pixel_size;

  /* Render upper left sample */
  /* ======================== */

  if (!block[y1][x1].ready)
    {
      num_samples++;
      (*render_func) (x + dx1, y + dy1, &c[0]);
      block[y1][x1].ready = 1;
      block[y1][x1].color = c[0];
    }
  else
    c[0] = block[y1][x1].color;

  /* Render upper right sample */
  /* ========================= */

  if (!block[y1][x3].ready)
    {
      num_samples++;
      (*render_func) (x + dx3, y + dy1, &c[1]);
      block[y1][x3].ready = 1;
      block[y1][x3].color = c[1];
    }
  else
    c[1] = block[y1][x3].color;

  /* Render lower left sample */
  /* ======================== */

  if (!block[y3][x1].ready)
    {
      num_samples++;
      (*render_func) (x + dx1, y + dy3, &c[2]);
      block[y3][x1].ready = 1;
      block[y3][x1].color = c[2];
    }
  else
    c[2] = block[y3][x1].color;

  /* Render lower right sample */
  /* ========================= */

  if (!block[y3][x3].ready)
    {
      num_samples++;
      (*render_func) (x + dx3, y + dy3, &c[3]);
      block[y3][x3].ready = 1;
      block[y3][x3].color = c[3];
    }
  else
    c[3] = block[y3][x3].color;

  /* Check for supersampling */
  /* ======================= */

  if (depth <= max_depth)
    {
      /* Check whether we have to supersample */
      /* ==================================== */

      if ((gck_rgba_dist(&c[0], &c[1]) >= threshold) ||
	  (gck_rgba_dist(&c[0], &c[2]) >= threshold) ||
	  (gck_rgba_dist(&c[0], &c[3]) >= threshold) ||
	  (gck_rgba_dist(&c[1], &c[2]) >= threshold) ||
	  (gck_rgba_dist(&c[1], &c[3]) >= threshold) ||
	  (gck_rgba_dist(&c[2], &c[3]) >= threshold))
	{
	  /* Calc coordinates of center subsample */
	  /* ==================================== */

	  x2 = (x1 + x3) / 2;
	  y2 = (y1 + y3) / 2;

	  /* Render sub-blocks */
	  /* ================= */

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y1, x2, y2,
	     threshold, sub_pixel_size, render_func, &c[0]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y1, x3, y2,
	     threshold, sub_pixel_size, render_func, &c[1]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x1, y2, x2, y3,
	     threshold, sub_pixel_size, render_func, &c[2]);

	  num_samples += gck_render_sub_pixel(max_depth, depth + 1, block, x, y, x2, y2, x3, y3,
	     threshold, sub_pixel_size, render_func, &c[3]);
	}
    }

  if (c[0].a==0.0 || c[1].a==0.0 || c[2].a==0.0 || c[3].a==0.0)
    {
      tmpcol.r=0.0;
      tmpcol.g=0.0;
      tmpcol.b=0.0;
      weight=2.0;

      for (cnt=0;cnt<4;cnt++)
        {
          if (c[cnt].a!=0.0)
            {
              tmpcol.r+=c[cnt].r;
              tmpcol.g+=c[cnt].g;
              tmpcol.b+=c[cnt].b;
              weight/=2.0;
            }
        }

      color->r=weight*tmpcol.r;
      color->g=weight*tmpcol.g;
      color->b=weight*tmpcol.b;
    }
  else
    {
      color->r = 0.25 * (c[0].r + c[1].r + c[2].r + c[3].r);
      color->g = 0.25 * (c[0].g + c[1].g + c[2].g + c[3].g);
      color->b = 0.25 * (c[0].b + c[1].b + c[2].b + c[3].b);
    }

  color->a = 0.25 * (c[0].a + c[1].a + c[2].a + c[3].a);

  return (num_samples);
}				/* Render_Sub_Pixel */

gulong gck_adaptive_supersample_area(int x1, int y1, int x2, int y2, int max_depth,
				     double threshold,
			             GckRenderFunction render_func,
		                     GckPutPixelFunction put_pixel_func,
			             GckProgressFunction progress_func)
{
  int x, y, width;		/* Counters, width of region */
  int xt, xtt, yt;		/* Temporary counters */
  int sub_pixel_size;		/* Numbe of samples per pixel (1D) */
  size_t row_size;		/* Memory needed for one row */
  GckRGB color;			/* Rendered pixel's color */
  _GckSampleType tmp_sample;	/* For swapping samples */
  _GckSampleType *top_row, *bot_row, *tmp_row;	/* Sample rows */
  _GckSampleType **block;	/* Sample block matrix */
  unsigned long num_samples;

  /* Initialize color */
  /* ================ */

  color.r = color.b = color.g = color.a = 0.0;

  /* Calculate sub-pixel size */
  /* ======================== */

  sub_pixel_size = 1 << max_depth;

  /* Create row arrays */
  /* ================= */

  width = x2 - x1 + 1;

  row_size = (sub_pixel_size * width + 1) * sizeof(_GckSampleType);

  top_row = (_GckSampleType *) g_malloc(row_size);
  bot_row = (_GckSampleType *) g_malloc(row_size);

  for (x = 0; x < (sub_pixel_size * width + 1); x++)
    {
      top_row[x].ready = 0;

      top_row[x].color.r = 0.0;
      top_row[x].color.g = 0.0;
      top_row[x].color.b = 0.0;
      top_row[x].color.a = 0.0;

      bot_row[x].ready = 0;

      bot_row[x].color.r = 0.0;
      bot_row[x].color.g = 0.0;
      bot_row[x].color.b = 0.0;
      bot_row[x].color.a = 0.0;
    }

  /* Allocate block matrix */
  /* ===================== */

  block = g_malloc((sub_pixel_size + 1) * sizeof(_GckSampleType *));	/* Rows */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    block[y] = g_malloc((sub_pixel_size + 1) * sizeof(_GckSampleType));

  for (y = 0; y < (sub_pixel_size + 1); y++)
    for (x = 0; x < (sub_pixel_size + 1); x++)
      {
	block[y][x].ready = 0;
	block[y][x].color.r = 0.0;
	block[y][x].color.g = 0.0;
	block[y][x].color.b = 0.0;
	block[y][x].color.a = 0.0;
      }

  /* Render region */
  /* ============= */

  num_samples = 0;

  for (y = y1; y <= y2; y++)
    {
      /* Clear the bottom row */
      /* ==================== */

      for (xt = 0; xt < (sub_pixel_size * width + 1); xt++)
	bot_row[xt].ready = 0;

      /* Clear first column */
      /* ================== */

      for (yt = 0; yt < (sub_pixel_size + 1); yt++)
	block[yt][0].ready = 0;

      /* Render row */
      /* ========== */

      for (x = x1; x <= x2; x++)
	{
	  /* Initialize block by clearing all but first row/column */
	  /* ===================================================== */

	  for (yt = 1; yt < (sub_pixel_size + 1); yt++)
	    for (xt = 1; xt < (sub_pixel_size + 1); xt++)
	      block[yt][xt].ready = 0;

	  /* Copy samples from top row to block */
	  /* ================================== */

	  for (xtt = 0, xt = (x - x1) * sub_pixel_size; xtt < (sub_pixel_size + 1); xtt++, xt++)
	    block[0][xtt] = top_row[xt];

	  /* Render pixel on (x, y) */
	  /* ====================== */

	  num_samples += gck_render_sub_pixel(max_depth, 1, block, x, y, 0, 0, sub_pixel_size,
					      sub_pixel_size, threshold, sub_pixel_size, render_func, &color);

	  (*put_pixel_func) (x, y, &color);

	  /* Copy block information to rows */
	  /* ============================== */

	  top_row[(x - x1 + 1) * sub_pixel_size] = block[0][sub_pixel_size];

	  for (xtt = 0, xt = (x - x1) * sub_pixel_size; xtt < (sub_pixel_size + 1); xtt++, xt++)
	    bot_row[xt] = block[sub_pixel_size][xtt];

	  /* Swap first and last columns */
	  /* =========================== */

	  for (yt = 0; yt < (sub_pixel_size + 1); yt++)
	    {
	      tmp_sample = block[yt][0];
	      block[yt][0] = block[yt][sub_pixel_size];
	      block[yt][sub_pixel_size] = tmp_sample;
	    }
	}

      /* Swap rows */
      /* ========= */

      tmp_row = top_row;
      top_row = bot_row;
      bot_row = tmp_row;

      /* Call progress display function (if any) */
      /* ======================================= */

      if (progress_func != NULL)
	(*progress_func) (y1, y2, y);
    }				/* for */

  /* Free memory */
  /* =========== */

  for (y = 0; y < (sub_pixel_size + 1); y++)
    g_free(block[y]);

  g_free(block);
  g_free(top_row);
  g_free(bot_row);

  return (num_samples);
}
