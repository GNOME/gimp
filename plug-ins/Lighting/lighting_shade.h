#ifndef __LIGHTING_SHADE_H__
#define __LIGHTING_SHADE_H__

typedef GckRGB (* get_ray_func) (GimpVector3 *vector);

GckRGB get_ray_color                 (GimpVector3 *position);
GckRGB get_ray_color_no_bilinear     (GimpVector3 *position);
GckRGB get_ray_color_ref             (GimpVector3 *position);
GckRGB get_ray_color_no_bilinear_ref (GimpVector3 *position);

void   precompute_init               (gint         w,
				      gint         h);
void   precompute_normals            (gint         x1,
				      gint         x2,
				      gint         y);

#endif  /* __LIGHTING_SHADE_H__ */
