#ifndef __FRACTALEXPLORER_CALLBACKS_H__
#define __FRACTALEXPLORER_CALLBACKS_H__

#include "gtk/gtk.h"

void                dialog_close_callback(GtkWidget * widget, gpointer data);
void                dialog_save_callback(GtkWidget * widget, gpointer data);
void                load_button_press(GtkWidget * widget, gpointer data);
void                dialog_ok_callback(GtkWidget * widget, gpointer data);
void                dialog_reset_callback(GtkWidget * widget, gpointer data);
void                dialog_redraw_callback(GtkWidget * widget, gpointer data);
void                dialog_cancel_callback(GtkWidget * widget, gpointer data);
void                dialog_undo_zoom_callback(GtkWidget * widget, gpointer data);
void                dialog_redo_zoom_callback(GtkWidget * widget, gpointer data);
void                dialog_step_in_callback(GtkWidget * widget, gpointer data);
void                dialog_step_out_callback(GtkWidget * widget, gpointer data);
void                explorer_logo_ok_callback(GtkWidget * widget, gpointer data);
void                explorer_about_callback(GtkWidget * widget, gpointer data);
void                explorer_toggle_update(GtkWidget * widget, gpointer data);
void                dialog_scale_update(GtkAdjustment * adjustment, gdouble * value);
void                dialog_scale_int_update(GtkAdjustment * adjustment, gint * value);
void                dialog_entry_update(GtkWidget * widget, gdouble * value);
void                dialog_entry_int_update(GtkWidget * widget, gint * value);

#endif

