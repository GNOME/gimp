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

#ifndef __GCK_H__
#define __GCK_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GCK_CONSTRAIN_RGB      1<<0
#define GCK_CONSTRAIN_RGBA     1<<1
#define GCK_CONSTRAIN_GRAY     1<<2
#define GCK_CONSTRAIN_GRAYA    1<<3
#define GCK_CONSTRAIN_INDEXED  1<<4
#define GCK_CONSTRAIN_INDEXEDA 1<<5
#define GCK_CONSTRAIN_ALL      0xff

typedef enum
{
  DITHER_NONE,
  DITHER_FLOYD_STEINBERG
} GckDitherType;

typedef struct
{
  GdkVisual    *visual;
  GdkColormap  *colormap;
  gulong        allocedpixels[256];
  guint32       colorcube[256];
  GdkColor      rgbpalette[256];
  guchar        map_r[256], map_g[256], map_b[256];
  guchar        indextab[7][7][7];
  guchar        invmap_r[256], invmap_g[256], invmap_b[256];
  gint          shades_r, shades_g, shades_b, numcolors;
  GckDitherType dithermethod;
} GckVisualInfo;

typedef void (* GckRenderFunction)      (gdouble, gdouble, GimpRGB *);
typedef void (* GckPutPixelFunction)    (gint, gint, GimpRGB *);
typedef void (* GckProgressFunction)    (gint, gint, gint);
typedef void (* GckColorUpdateFunction) (GimpRGB *);


GckVisualInfo *gck_visualinfo_new        (void);
void           gck_visualinfo_destroy    (GckVisualInfo *visinfo);


/* RGB to Gdk routines */
/* =================== */

void      gck_rgb_to_gdkimage       (GckVisualInfo *visinfo,
                                     guchar *RGB_data,
                                     GdkImage *image,
                                     int width,
				     int height);

void      gck_gc_set_foreground     (GckVisualInfo *visinfo,GdkGC *gc,
                                     guchar r, guchar g, guchar b); 
void      gck_gc_set_background     (GckVisualInfo *visinfo,GdkGC *gc,
                                     guchar r, guchar g, guchar b); 

/********************/
/* Color operations */
/********************/

double  gck_bilinear      (double x,double y, double    *values);
guchar  gck_bilinear_8    (double x,double y, guchar    *values);
guint16 gck_bilinear_16   (double x,double y, guint16   *values);
guint32 gck_bilinear_32   (double x,double y, guint32   *values);
GimpRGB  gck_bilinear_rgb  (double x,double y, GimpRGB *values);
GimpRGB  gck_bilinear_rgba (double x,double y, GimpRGB *values);

/* Supersampling */
/* ============= */

gulong    gck_adaptive_supersample_area (int x1,int y1,int x2,int y2,
                                         int max_depth,
                                         double threshold,
                                         GckRenderFunction    render_func,
                                         GckPutPixelFunction  put_pixel_func,
                                         GckProgressFunction  progress_func);

#ifdef __cplusplus
}
#endif

#endif  /* __GCK_H__ */
