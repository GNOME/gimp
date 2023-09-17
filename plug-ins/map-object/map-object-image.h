#ifndef __MAPOBJECT_IMAGE_H__
#define __MAPOBJECT_IMAGE_H__

/* Externally visible variables */
/* ============================ */

extern GimpImage    *image;

extern GimpDrawable *input_drawable;
extern GimpDrawable *output_drawable;
extern GeglBuffer   *source_buffer;
extern GeglBuffer   *dest_buffer;

extern GimpDrawable *box_drawables[6];
extern GeglBuffer   *box_buffers[6];

extern GimpDrawable *cylinder_drawables[2];
extern GeglBuffer   *cylinder_buffers[2];

extern guchar          *preview_rgb_data;
extern gint             preview_rgb_stride;
extern cairo_surface_t *preview_surface;

extern glong    maxcounter, old_depth, max_depth;
extern gint     width, height;
extern GimpRGB  background;

extern gint border_x1, border_y1, border_x2, border_y2;

/* Externally visible functions */
/* ============================ */

extern gint        image_setup              (GimpDrawable *drawable,
                                             gint          interactive,
                                             GimpProcedureConfig
                                                          *config);
extern glong       in_xy_to_index           (gint          x,
                                             gint          y);
extern glong       out_xy_to_index          (gint          x,
                                             gint          y);
extern gint        checkbounds              (gint          x,
                                             gint          y);
extern GimpRGB     peek                     (gint          x,
                                             gint          y);
extern void        poke                     (gint          x,
                                             gint          y,
                                             GimpRGB      *color,
                                             gpointer      user_data);
extern GimpVector3 int_to_pos               (gint          x,
                                             gint          y);
extern void        pos_to_int               (gdouble       x,
                                             gdouble       y,
                                             gint         *scr_x,
                                             gint         *scr_y);

extern GimpRGB     get_image_color          (gdouble      u,
                                             gdouble      v,
                                             gint        *inside);
extern GimpRGB     get_box_image_color      (gint         image,
                                             gdouble      u,
                                             gdouble      v);
extern GimpRGB     get_cylinder_image_color (gint         image,
                                             gdouble      u,
                                             gdouble      v);

#endif  /* __MAPOBJECT_IMAGE_H__ */
