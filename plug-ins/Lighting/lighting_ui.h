#ifndef LIGHTINGUIH
#define LIGHTINGUIH

#include <stdlib.h>
#include <stdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpmenu.h>

#include "lighting_main.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_preview.h"

/* Externally visible variables */
/* ============================ */

extern GckApplicationWindow *appwin;

extern GdkGC *gc;
extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

extern void create_main_dialog (void);

#endif
