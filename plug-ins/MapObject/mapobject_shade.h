#ifndef MAPOBJECTSHADEH
#define MAPOBJECTSHADEH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gck/gck.h>

#include "mapobject_main.h"
#include "mapobject_image.h"

typedef GckRGB (*get_ray_color_func)(GckVector3 *pos);

extern get_ray_color_func get_ray_color;
extern GckRGB             get_ray_color_plane  (GckVector3 *pos);
extern GckRGB             get_ray_color_sphere (GckVector3 *pos);
extern void               compute_bounding_box (void);


#endif
