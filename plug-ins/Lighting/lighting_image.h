#ifndef LIGHTINGIMAGEH
#define LIGHTINGIMAGEH

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "lighting_main.h"

extern GDrawable *input_drawable,*output_drawable;
extern GPixelRgn source_region, dest_region;

extern GDrawable *bump_drawable;
extern GPixelRgn bump_region;

extern GDrawable *env_drawable;
extern GPixelRgn env_region;

extern guchar *preview_rgb_data;
extern GdkImage *image;

extern glong maxcounter;
extern gint imgtype,width,height,env_width,env_height,in_channels,out_channels;
extern GckRGB background;

extern gint border_x1,border_y1,border_x2,border_y2;

extern guchar sinemap[256],spheremap[256],logmap[256];

guchar         peek_map        (GPixelRgn *region,gint x,gint y);
GckRGB         peek            (gint x,gint y);
GckRGB         peek_env_map    (gint x,gint y);
void           poke            (gint x,gint y,GckRGB *color);
gint           check_bounds    (gint x,gint y);
GckVector3     int_to_pos      (gint x,gint y);
GckVector3     int_to_posf     (gdouble x,gdouble y);
extern void    pos_to_int      (gdouble x,gdouble y,gint *scr_x,gint *scr_y);
extern void    pos_to_float    (gdouble x,gdouble y,gdouble *xf,gdouble *yf);
extern GckRGB  get_image_color (gdouble u,gdouble v,gint *inside);
extern gdouble get_map_value   (GPixelRgn *region, gdouble u,gdouble v, gint *inside);
extern gint    image_setup     (GDrawable *drawable,gint interactive);

#endif
