#ifndef MAPOBJECTAPPLYH
#define MAPOBJECTAPPLYH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "mapobject_main.h"
#include "mapobject_image.h"

extern gdouble imat[4][4];
extern gfloat rotmat[16];
extern void init_compute(void);
extern void compute_image(void);

#endif
