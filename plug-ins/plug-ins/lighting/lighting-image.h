#ifndef __LIGHTING_IMAGE_H__
#define __LIGHTING_IMAGE_H__

extern GimpDrawable *input_drawable,*output_drawable;
extern GimpPixelRgn  source_region, dest_region;

extern GimpDrawable *bump_drawable;
extern GimpPixelRgn  bump_region;

extern GimpDrawable *env_drawable;
extern GimpPixelRgn  env_region;

extern guchar   *preview_rgb_data;

extern glong  maxcounter;
extern gint   imgtype,width,height,env_width,env_height,in_channels,out_channels;
extern GimpRGB background;

extern gint   border_x1,border_y1,border_x2,border_y2;

extern guchar sinemap[256], spheremap[256], logmap[256];

guchar         peek_map        (GimpPixelRgn *region,
				gint          x,
				gint          y);
GimpRGB         peek            (gint          x,
				gint          y);
GimpRGB         peek_env_map    (gint          x,
				gint          y);
void           poke            (gint          x,
				gint          y,
				GimpRGB       *color);
gint           check_bounds    (gint          x,
				gint          y);
GimpVector3    int_to_pos      (gint          x,
				gint          y);
GimpVector3    int_to_posf     (gdouble       x,
				gdouble       y);
void           pos_to_int      (gdouble       x,
				gdouble       y,
				gint         *scr_x,
				gint         *scr_y);
void           pos_to_float    (gdouble       x,
				gdouble       y,
				gdouble      *xf,
				gdouble      *yf);
GimpRGB         get_image_color (gdouble       u,
				gdouble       v,
				gint         *inside);
gdouble        get_map_value   (GimpPixelRgn *region,
				gdouble       u,
				gdouble       v,
				gint         *inside);
gint           image_setup     (GimpDrawable *drawable,
				gint          interactive);

#endif  /* __LIGHTING_IMAGE_H__ */
