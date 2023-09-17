#ifndef __MAPOBJECT_UI_H__
#define __MAPOBJECT_UI_H__

/* Externally visible variables */
/* ============================ */

extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

gboolean main_dialog (GimpProcedure       *procedure,
                      GimpProcedureConfig *config,
                      GimpDrawable        *drawable);

#endif  /* __MAPOBJECT_UI_H__ */
