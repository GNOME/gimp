#ifndef __LIGHTING_UI_H__
#define __LIGHTING_UI_H__

/* Externally visible variables */
/* ============================ */

extern GckVisualInfo *visinfo;

extern GdkGC     *gc;
extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

gboolean main_dialog (GDrawable *drawable);

#endif  /* __LIGHTING_UI_H__ */
