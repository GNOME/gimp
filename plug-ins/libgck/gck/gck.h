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

#define GCK_HSV_UNDEFINED -1.0
#define GCK_HSL_UNDEFINED -1.0

typedef enum
{
  DITHER_NONE,
  DITHER_FLOYD_STEINBERG,
  DITHER_ORDERED
} GckDitherType;

typedef struct
{
  gdouble r, g, b, a;
} GckRGB;

typedef struct
{
  guchar       r, g, b;
  const gchar *name;
} GckNamedRGB;

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

typedef void (* GckRenderFunction)      (gdouble, gdouble, GckRGB *);
typedef void (* GckPutPixelFunction)    (gint, gint, GckRGB *);
typedef void (* GckProgressFunction)    (gint, gint, gint);
typedef void (* GckColorUpdateFunction) (GckRGB *);
typedef gint (* GckEventFunction)       (GtkWidget *, GdkEvent *, gpointer);

typedef struct
{
  GtkWidget     *widget;
  GckVisualInfo *visinfo;
} GckApplicationWindow;

typedef struct
{
  gint      x, y, w, h;
  GdkImage *buffer;
} _GckBackBuffer;



GckVisualInfo *gck_visualinfo_new        (void);
void           gck_visualinfo_destroy    (GckVisualInfo *visinfo);
GckDitherType  gck_visualinfo_get_dither (GckVisualInfo *visinfo);
void           gck_visualinfo_set_dither (GckVisualInfo *visinfo,
                                          GckDitherType dithermethod);

/* RGB to Gdk routines */
/* =================== */

void      gck_rgb_to_gdkimage       (GckVisualInfo *visinfo,
                                     guchar *RGB_data,
                                     GdkImage *image,
                                     int width,int height);

/* returns a static storage */
GdkColor *gck_rgb_to_gdkcolor       (GckVisualInfo *visinfo,guchar r,guchar g,guchar b);

/* returns a malloc'ed area */
GdkColor *gck_rgb_to_gdkcolor_r     (GckVisualInfo *visinfo,guchar r,guchar g,guchar b);

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
GckRGB  gck_bilinear_rgb  (double x,double y, GckRGB *values);
GckRGB  gck_bilinear_rgba (double x,double y, GckRGB *values);

/* RGB pixel operations */
/* ==================== */

void      gck_rgb_add    (GckRGB *p,GckRGB *q);
void      gck_rgb_sub    (GckRGB *p,GckRGB *q);
void      gck_rgb_mul    (GckRGB *p,double b);
void      gck_rgb_clamp  (GckRGB *p);
void      gck_rgb_set    (GckRGB *p,double r,double g,double b);
void      gck_rgb_gamma  (GckRGB *p,double gamma);

void      gck_rgba_add   (GckRGB *p,GckRGB *q);
void      gck_rgba_sub   (GckRGB *p,GckRGB *q);
void      gck_rgba_mul   (GckRGB *p,double b);
void      gck_rgba_clamp (GckRGB *p);
void      gck_rgba_set   (GckRGB *p,double r,double g,double b,double a);
void      gck_rgba_gamma (GckRGB *p,double gamma);

/* Colorspace conversions */
/* ====================== */

void      gck_rgb_to_hsv (GckRGB *p, double *h,double *s,double *v);
void      gck_rgb_to_hsl (GckRGB *p, double *h,double *s,double *l);

void      gck_hsv_to_rgb (double h,double s,double v, GckRGB *p);
void      gck_hsl_to_rgb (double h,double s,double l, GckRGB *p);

void      gck_rgb_to_hwb (GckRGB *rgb, gdouble *hue,gdouble *whiteness,gdouble *blackness);
void      gck_hwb_to_rgb (gdouble H,gdouble W, gdouble B, GckRGB *rgb);

/* Supersampling */
/* ============= */

gulong    gck_adaptive_supersample_area (int x1,int y1,int x2,int y2,
                                         int max_depth,
                                         double threshold,
                                         GckRenderFunction    render_func,
                                         GckPutPixelFunction  put_pixel_func,
                                         GckProgressFunction  progress_func);

extern GckNamedRGB gck_named_colors[];

#ifdef __cplusplus
}
#endif

#endif  /* __GCK_H__ */
