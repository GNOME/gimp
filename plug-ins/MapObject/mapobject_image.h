#ifndef __MAPOBJECT_IMAGE_H__
#define __MAPOBJECT_IMAGE_H__

/* Externally visible variables */
/* ============================ */

extern GDrawable *input_drawable,*output_drawable;
extern GPixelRgn  source_region,dest_region;

extern GDrawable *box_drawables[6];
extern GPixelRgn  box_regions[6];

extern GDrawable *cylinder_drawables[2];
extern GPixelRgn  cylinder_regions[2];

extern guchar   *preview_rgb_data;
extern GdkImage *image;

extern glong   maxcounter, old_depth, max_depth;
extern gint    imgtype, width,height, in_channels, out_channels;
extern GckRGB  background;
extern gdouble oldtreshold;

extern gint border_x1, border_y1, border_x2, border_y2;

extern GTile *current_in_tile, *current_out_tile;

/* Externally visible functions */
/* ============================ */

extern gint        image_setup     (GDrawable *drawable,
				    gint       interactive);
extern glong       in_xy_to_index  (gint       x,
				    gint       y);
extern glong       out_xy_to_index (gint       x,
				    gint       y);
extern gint        checkbounds     (gint       x,
				    gint       y);
extern GckRGB      peek            (gint       x,
				    gint       y);
extern void        poke            (gint       x,
				    gint       y,
				    GckRGB    *color);
extern GimpVector3 int_to_pos      (gint      x,
				    gint      y);
extern void        pos_to_int      (gdouble    x,
				    gdouble    y,
				    gint      *scr_x,
				    gint      *scr_y);

extern GckRGB      get_image_color          (gdouble  u,
					     gdouble  v,
					     gint    *inside);
extern GckRGB      get_box_image_color      (gint     image,
					     gdouble  u,
					     gdouble  v);
extern GckRGB      get_cylinder_image_color (gint     image,
					     gdouble  u,
					     gdouble  v);

#endif  /* __MAPOBJECT_IMAGE_H__ */
