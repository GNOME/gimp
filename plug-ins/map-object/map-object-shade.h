#ifndef __MAPOBJECT_SHADE_H__
#define __MAPOBJECT_SHADE_H__

typedef LigmaRGB (* get_ray_color_func) (LigmaVector3 *pos);

extern get_ray_color_func get_ray_color;

LigmaRGB   get_ray_color_plane    (LigmaVector3 *pos);
LigmaRGB   get_ray_color_sphere   (LigmaVector3 *pos);
LigmaRGB   get_ray_color_box      (LigmaVector3 *pos);
LigmaRGB   get_ray_color_cylinder (LigmaVector3 *pos);
void     compute_bounding_box   (void);

void     vecmulmat              (LigmaVector3 *u,
                                 LigmaVector3 *v,
                                 gfloat       m[16]);
void     rotatemat              (gfloat       angle,
                                 LigmaVector3 *v,
                                 gfloat       m[16]);
void     transpose_mat          (gfloat       m[16]);
void     matmul                 (gfloat       a[16],
                                 gfloat       b[16],
                                 gfloat       c[16]);
void     ident_mat              (gfloat       m[16]);

#endif  /* __MAPOBJECT_SHADE_H__ */
