#ifndef __FILE_NEW_DIALOG_H__
#define __FILE_NEW_DIALOG_H__

#include "gtk/gtk.h"

void file_new_cmd_callback (GtkWidget *widget,
			    gpointer   callback_data,
			    guint      callback_action);

void file_new_reset_current_cut_buffer ();

#endif /* __FILE_NEW_DIALOG_H_H__ */
