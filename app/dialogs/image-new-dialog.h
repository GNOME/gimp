#ifndef __FILE_NEW_DIALOG_H__
#define __FILE_NEW_DIALOG_H__

#include "gtk/gtk.h"

#include "image_new.h"

void file_new_cmd_callback (GtkWidget *widget,
			    gpointer   callback_data,
			    guint      callback_action);

void ui_new_image_window_create (const GimpImageNewValues *values);

#endif /* __FILE_NEW_DIALOG_H_H__ */
