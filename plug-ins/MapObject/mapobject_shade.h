#ifndef MAPOBJECTSHADEH
#define MAPOBJECTSHADEH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gck/gck.h>

#include "mapobject_main.h"
#include "mapobject_image.h"

typedef GckRGB (*get_ray_color_func)(GimpVector3 *pos);

extern get_ray_color_func get_ray_color;
extern GckRGB             get_ray_color_plane    (GimpVector3 *pos);
extern GckRGB             get_ray_color_sphere   (GimpVector3 *pos);
extern GckRGB             get_ray_color_box      (GimpVector3 *pos);
extern GckRGB             get_ray_color_cylinder (GimpVector3 *pos);
extern void               compute_bounding_box   (void);

extern void vecmulmat     (GimpVector3 *u,GimpVector3 *v,gfloat m[16]);
extern void rotatemat     (gfloat angle,GimpVector3 *v,gfloat m[16]);
extern void transpose_mat (gfloat m[16]);
extern void matmul        (gfloat a[16],gfloat b[16],gfloat c[16]);
extern void ident_mat     (gfloat m[16]);

#endif
