#ifndef MAPOBJECTUIH
#define MAPOBJECTUIH

#include <stdlib.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "arcball.h"
#include "mapobject_main.h"
#include "mapobject_image.h"
#include "mapobject_apply.h"
#include "mapobject_preview.h"

/* Externally visible variables */
/* ============================ */

extern GckApplicationWindow *appwin;

extern GdkGC *gc;
extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

extern void create_main_dialog(void);

#endif
