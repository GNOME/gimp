#ifndef __LIGHTING_UI_H__
#define __LIGHTING_UI_H__

/* Externally visible variables */
/* ============================ */

extern GtkWidget *previewarea;

extern GtkWidget *spin_pos_x;
extern GtkWidget *spin_pos_y;
extern GtkWidget *spin_pos_z;
extern GtkWidget *spin_dir_x;
extern GtkWidget *spin_dir_y;
extern GtkWidget *spin_dir_z;

/* Externally visible functions */
/* ============================ */

gboolean main_dialog (GimpProcedure       *procedure,
                      GimpProcedureConfig *config,
                      GimpDrawable        *drawable);

#endif  /* __LIGHTING_UI_H__ */
