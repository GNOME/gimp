#ifndef LIGHTINGSHADEH
#define LIGHTINGSHADEH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gck/gck.h>

#include "lighting_main.h"
#include "lighting_image.h"

typedef GckRGB (*get_ray_func) (GimpVector3 *);

extern GckRGB get_ray_color                 (GimpVector3 *position);
extern GckRGB get_ray_color_no_bilinear     (GimpVector3 *position);
extern GckRGB get_ray_color_ref             (GimpVector3 *position);
extern GckRGB get_ray_color_no_bilinear_ref (GimpVector3 *position);

extern void precompute_init    (gint w,gint h);
extern void precompute_normals (gint x1,gint x2,gint y);

#endif
