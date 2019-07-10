#ifndef __LIGHTING_IMAGE_H__
#define __LIGHTING_IMAGE_H__

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

extern gint32      input_drawable_id;
extern gint32      output_drawable_id;
extern GeglBuffer *source_buffer;
extern GeglBuffer *dest_buffer;

extern gint32      bump_drawable_id;
extern GeglBuffer *bump_buffer;
extern const Babl *bump_format;

extern gint32      env_drawable_id;
extern GeglBuffer *env_buffer;

extern guchar          *preview_rgb_data;
extern gint             preview_rgb_stride;
extern cairo_surface_t *preview_surface;

extern glong   maxcounter;
extern gint    width,height,env_width,env_height;
extern GimpRGB background;

extern gint   border_x1, border_y1, border_x2, border_y2;

extern guchar sinemap[256], spheremap[256], logmap[256];

guchar         peek_map        (GeglBuffer   *buffer,
                                const Babl   *format,
				gint          x,
				gint          y);
GimpRGB        peek            (gint          x,
				gint          y);
GimpRGB        peek_env_map    (gint          x,
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
GimpRGB        get_image_color (gdouble       u,
				gdouble       v,
				gint         *inside);
gdouble        get_map_value   (GeglBuffer   *buffer,
                                const Babl   *format,
				gdouble       u,
				gdouble       v,
				gint         *inside);
gint           image_setup     (gint32        drawable_id,
				gint          interactive);
void           bumpmap_setup   (gint32        bumpmap_id);
void           envmap_setup    (gint32        envmap_id);

#endif  /* __LIGHTING_IMAGE_H__ */
