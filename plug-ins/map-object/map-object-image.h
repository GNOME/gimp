#ifndef __MAPOBJECT_IMAGE_H__
#define __MAPOBJECT_IMAGE_H__

/* Externally visible variables */
/* ============================ */

extern LigmaImage    *image;

extern LigmaDrawable *input_drawable;
extern LigmaDrawable *output_drawable;
extern GeglBuffer   *source_buffer;
extern GeglBuffer   *dest_buffer;

extern LigmaDrawable *box_drawables[6];
extern GeglBuffer   *box_buffers[6];

extern LigmaDrawable *cylinder_drawables[2];
extern GeglBuffer   *cylinder_buffers[2];

extern guchar          *preview_rgb_data;
extern gint             preview_rgb_stride;
extern cairo_surface_t *preview_surface;

extern glong    maxcounter, old_depth, max_depth;
extern gint     width, height;
extern LigmaRGB  background;

extern gint border_x1, border_y1, border_x2, border_y2;

/* Externally visible functions */
/* ============================ */

extern gint        image_setup              (LigmaDrawable *drawable,
                                             gint          interactive);
extern glong       in_xy_to_index           (gint          x,
                                             gint          y);
extern glong       out_xy_to_index          (gint          x,
                                             gint          y);
extern gint        checkbounds              (gint          x,
                                             gint          y);
extern LigmaRGB     peek                     (gint          x,
                                             gint          y);
extern void        poke                     (gint          x,
                                             gint          y,
                                             LigmaRGB      *color,
                                             gpointer      user_data);
extern LigmaVector3 int_to_pos               (gint          x,
                                             gint          y);
extern void        pos_to_int               (gdouble       x,
                                             gdouble       y,
                                             gint         *scr_x,
                                             gint         *scr_y);

extern LigmaRGB     get_image_color          (gdouble      u,
                                             gdouble      v,
                                             gint        *inside);
extern LigmaRGB     get_box_image_color      (gint         image,
                                             gdouble      u,
                                             gdouble      v);
extern LigmaRGB     get_cylinder_image_color (gint         image,
                                             gdouble      u,
                                             gdouble      v);

#endif  /* __MAPOBJECT_IMAGE_H__ */
