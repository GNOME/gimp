#ifndef __LIGHTING_SHADE_H__
#define __LIGHTING_SHADE_H__

typedef LigmaRGB (* get_ray_func) (LigmaVector3 *vector);

LigmaRGB get_ray_color                 (LigmaVector3 *position);
LigmaRGB get_ray_color_no_bilinear     (LigmaVector3 *position);
LigmaRGB get_ray_color_ref             (LigmaVector3 *position);
LigmaRGB get_ray_color_no_bilinear_ref (LigmaVector3 *position);

void    precompute_init               (gint         w,
				       gint         h);
void    precompute_normals            (gint         x1,
				       gint         x2,
				       gint         y);
void    interpol_row                  (gint         x1,
                                       gint         x2,
                                       gint         y);

#endif  /* __LIGHTING_SHADE_H__ */
