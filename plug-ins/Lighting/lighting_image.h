#ifndef __LIGHTING_IMAGE_H__
#define __LIGHTING_IMAGE_H__

extern GDrawable *input_drawable,*output_drawable;
extern GPixelRgn  source_region, dest_region;

extern GDrawable *bump_drawable;
extern GPixelRgn  bump_region;

extern GDrawable *env_drawable;
extern GPixelRgn  env_region;

extern guchar   *preview_rgb_data;
extern GdkImage *image;

extern glong  maxcounter;
extern gint   imgtype,width,height,env_width,env_height,in_channels,out_channels;
extern GckRGB background;

extern gint   border_x1,border_y1,border_x2,border_y2;

extern guchar sinemap[256], spheremap[256], logmap[256];

guchar         peek_map        (GPixelRgn *region,
				gint       x,
				gint       y);
GckRGB         peek            (gint       x,
				gint       y);
GckRGB         peek_env_map    (gint       x,
				gint       y);
void           poke            (gint       x,
				gint       y,
				GckRGB    *color);
gint           check_bounds    (gint       x,
				gint       y);
GimpVector3    int_to_pos      (gint       x,
				gint       y);
GimpVector3    int_to_posf     (gdouble    x,
				gdouble    y);
void           pos_to_int      (gdouble    x,
				gdouble    y,
				gint      *scr_x,
				gint      *scr_y);
void           pos_to_float    (gdouble    x,
				gdouble    y,
				gdouble   *xf,
				gdouble   *yf);
GckRGB         get_image_color (gdouble    u,
				gdouble    v,
				gint      *inside);
gdouble        get_map_value   (GPixelRgn *region,
				gdouble    u,
				gdouble    v,
				gint      *inside);
gint           image_setup     (GDrawable *drawable,
				gint       interactive);

#endif  /* __LIGHTING_IMAGE_H__ */
