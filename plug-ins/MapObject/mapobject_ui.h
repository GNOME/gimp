#ifndef __MAPOBJECT_UI_H__
#define __MAPOBJECT_UI_H__

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include <gck/gck.h>

/* Externally visible variables */
/* ============================ */

extern GckVisualInfo *visinfo;

extern GdkGC     *gc;
extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

gboolean main_dialog (GDrawable *drawable);

#endif  /* __MAPOBJECT_UI_H__ */
