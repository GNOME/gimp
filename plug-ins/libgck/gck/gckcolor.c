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

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "gck.h"


#define RESERVED_COLORS 2

#define ROUND_TO_INT(val) ((val) + 0.5)


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
