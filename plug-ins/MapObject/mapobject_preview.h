#ifndef MAPOBJECTPREVIEWH
#define MAPOBJECTPREVIEWH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "arcball.h"
#include "mapobject_main.h"
#include "mapobject_ui.h"
#include "mapobject_image.h"
#include "mapobject_apply.h"
#include "mapobject_shade.h"

#define PREVIEW_WIDTH 200
#define PREVIEW_HEIGHT 200

#define WIRESIZE 16

typedef struct
{
  gint x1,y1,x2,y2;
  gint linewidth;
  GdkLineStyle linestyle;
} line;

typedef struct
{
  gint x,y,w,h;
  GdkImage *image;
} BackBuffer;

/* Externally visible variables */
/* ============================ */

extern line       linetab[];
extern gdouble    mat[3][4];
extern gint       lightx,lighty;
extern BackBuffer backbuf;

/* Externally visible functions */
/* ============================ */

extern void compute_preview        (gint x,gint y,gint w,gint h,gint pw,gint ph);
extern void draw_wireframe         (gint startx,gint starty,gint pw,gint ph);
extern void clear_wireframe        (void);
extern void draw_preview_image     (gint docompute);
extern void draw_preview_wireframe (void);
extern gint check_light_hit        (gint xpos,gint ypos);
extern void update_light           (gint xpos,gint ypos);

#endif
