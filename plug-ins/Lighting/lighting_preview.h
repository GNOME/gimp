#ifndef LIGHTINGPREVIEWH
#define LIGHTINGPREVIEWH

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "lighting_main.h"
#include "lighting_ui.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_shade.h"

#define PREVIEW_WIDTH 300
#define PREVIEW_HEIGHT 300

typedef struct
{
  gint x,y,w,h;
  GdkImage *image;
} BackBuffer;

/* Externally visible variables */
/* ============================ */

extern gint       lightx,lighty;
extern BackBuffer backbuf;
extern gdouble    *xpostab,*ypostab;

/* Externally visible functions */
/* ============================ */

extern void draw_preview_image (gint recompute);
extern gint check_light_hit    (gint xpos,gint ypos);
extern void update_light       (gint xpos,gint ypos);

#endif
