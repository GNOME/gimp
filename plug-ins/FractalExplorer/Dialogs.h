#ifndef __FRACTALEXPLORER_DIALOGS_H__
#define __FRACTALEXPLORER_DIALOGS_H__

gint                explorer_dialog(void);
void                dialog_update_preview(void);

void                dialog_create_value(char *title, GtkTable * table, 
					int row, gdouble * value, 
					int left, int right, 
					const char *desc, 
					scaledata * scalevalues);

void                dialog_create_int_value(char *title, GtkTable * table, 
					    int row, gdouble * value,
					    int left, int right, 
					    const char *desc, 
					    scaledata * scalevalues);
void         set_cmap_preview(void);
void         make_color_map(void);

GtkWidget          *explorer_logo_dialog();
GtkWidget          *explorer_load_dialog();
void                set_tooltip(GtkTooltips * tooltips, GtkWidget * widget, const char *desc);
void         dialog_change_scale(void);
void         create_warn_dialog(gchar * msg);
void         save_options(FILE * fp);
void         save_callback();
void         file_selection_ok(GtkWidget * w,
		  GtkFileSelection * fs,
		  gpointer data);
void         destroy_window(GtkWidget * widget,
	       GtkWidget ** window);
void         load_file_selection_ok(GtkWidget * w,
		       GtkFileSelection * fs,
		       gpointer data);
void                create_load_file_selection();
void                create_file_selection();
char *get_line(gchar * buf, gint s, FILE * from, gint init);
gint load_options(fractalexplorerOBJ * xxx, FILE * fp);
gint gradient_load_options(gradientOBJ * xxx, FILE * fp);
void                explorer_load();

#endif
