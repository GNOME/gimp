/* LIBGIMP - The GIMP Library                  
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialog.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *                                                   
 * This library is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include "libgimp/gimpdialog.h"

#include "pixmaps/wilber.xpm"

/*  local callbacks of gimp_dialog_new ()  */
static gint
gimp_dialog_delete_callback (GtkWidget *widget,
			     GdkEvent  *event,
			     gpointer   data) 
{
  GtkSignalFunc  cancel_callback;
  GtkWidget     *cancel_widget;

  cancel_callback =
    (GtkSignalFunc) gtk_object_get_data (GTK_OBJECT (widget),
					 "gimp_dialog_cancel_callback");
  cancel_widget =
    (GtkWidget*) gtk_object_get_data (GTK_OBJECT (widget),
				      "gimp_dialog_cancel_widget");

  /*  the cancel callback has to destroy the dialog  */
  if (cancel_callback)
    (* cancel_callback) (cancel_widget, data);

  return TRUE;
}

static void
gimp_dialog_realize_callback (GtkWidget *widget,
			      gpointer   data) 
{
  static GdkPixmap *wilber_pixmap = NULL;
  static GdkBitmap *wilber_mask   = NULL;
  GtkStyle         *style;

  style = gtk_widget_get_style (widget);

  if (wilber_pixmap == NULL)
    wilber_pixmap =
      gdk_pixmap_create_from_xpm_d (widget->window,
				    &wilber_mask,
				    &style->bg[GTK_STATE_NORMAL],
				    wilber_xpm);

  gdk_window_set_icon (widget->window, NULL,
		       wilber_pixmap, wilber_mask);
}

/**
 * gimp_dialog_new:
 * @title: The dialog's title which will be set with gtk_window_set_title().
 * @wmclass_name: The dialog's @wmclass_name which will be set with
 *                gtk_window_set_wmclass(). The @wmclass_class will be
 *                automatically set to "Gimp".
 * @help_func: The function which will be called if the user presses "F1".
 * @help_data: The data pointer which will be passed to @help_func.
 * @position: The dialog's initial position which will be set with
 *            gtk_window_set_position().
 * @allow_shrink: The dialog's @allow_shrink flag, ...
 * @allow_grow: ... it't @allow_grow flag and ...
 * @auto_shrink: ... it's @auto_shrink flag which will all be set with
 *               gtk_window_set_policy().
 * @...: A #NULL terminated @va_list destribing the action_area buttons.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to gimp_dialog_newv().
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gimp_dialog_create_action_areav().
 *
 * Returns: A #GtkDialog.
 *
 */
GtkWidget *
gimp_dialog_new (const gchar       *title,
		 const gchar       *wmclass_name,
		 GimpHelpFunc       help_func,
		 gchar             *help_data,
		 GtkWindowPosition  position,
		 gint               allow_shrink,
		 gint               allow_grow,
		 gint               auto_shrink,

		 /* specify action area buttons as va_list:
		  *  gchar          *label,
		  *  GtkSignalFunc   callback,
		  *  gpointer        data,
		  *  GtkObject      *slot_object,
		  *  GtkWidget     **widget_ptr,
		  *  gboolean        default_action,
		  *  gboolean        connect_delete,
		  */

		 ...)
{
  GtkWidget *dialog;
  va_list    args;

  va_start (args, auto_shrink);

  dialog = gimp_dialog_newv (title,
			     wmclass_name,
			     help_func,
			     help_data,
			     position,
			     allow_shrink,
			     allow_grow,
			     auto_shrink,
			     args);

  va_end (args);

  return dialog;
}

/**
 * gimp_dialog_newv:
 * @title: The dialog's title which will be set with gtk_window_set_title().
 * @wmclass_name: The dialog's @wmclass_name which will be set with
 *                gtk_window_set_wmclass(). The @wmclass_class will be
 *                automatically set to "Gimp".
 * @help_func: The function which will be called if the user presses "F1".
 * @help_data: The data pointer which will be passed to @help_func.
 * @position: The dialog's initial position which will be set with
 *            gtk_window_set_position().
 * @allow_shrink: The dialog's @allow_shrink flag, ...
 * @allow_grow: ... it't @allow_grow flag and ...
 * @auto_shrink: ... it's @auto_shrink flag which will all be set with
 *               gtk_window_set_policy().
 * @args: A @va_list as obtained with va_start() describing the action_area
 *        buttons.
 *
 * This function performs all neccessary setps to set up a standard GIMP
 * dialog.
 *
 * The @va_list describing the action_area buttons will be passed to
 * gimp_dialog_create_action_areav().
 *
 * Returns: A #GtkDialog.
 *
 */
GtkWidget *
gimp_dialog_newv (const gchar       *title,
		  const gchar       *wmclass_name,
		  GimpHelpFunc       help_func,
		  gchar             *help_data,
		  GtkWindowPosition  position,
		  gint               allow_shrink,
		  gint               allow_grow,
		  gint               auto_shrink,
		  va_list            args)
{
  GtkWidget *dialog;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (wmclass_name != NULL, NULL);

  dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, "Gimp");
  gtk_window_set_position (GTK_WINDOW (dialog), position);
  gtk_window_set_policy (GTK_WINDOW (dialog),
			 allow_shrink, allow_grow, auto_shrink);

  /*  prepare the action_area  */
  gimp_dialog_create_action_areav (GTK_DIALOG (dialog), args);

  /*  connect the "F1" help key  */
  if (help_func)
    gimp_help_connect_help_accel (dialog, help_func, help_data);

  return dialog;
}

/**
 * gimp_dialog_set_icon:
 * @dialog: The #GtkWindow you want to set the pixmap icon for.
 *
 * This function sets the WM pixmap icon for the dialog which will appear
 * e.g. in GNOME's or KDE's window list.
 *
 * Note that this function is automatically called by
 * gimp_help_connect_help_accel() which in turn is called by
 * gimp_dialog_newv(), so you only have to call it for #GtkWindow's which
 * have no help page (like tear-off menus).
 *
 */
void
gimp_dialog_set_icon (GtkWindow *dialog)
{
  g_return_if_fail (dialog);
  g_return_if_fail (GTK_IS_WINDOW (dialog));

  if (GTK_WIDGET_REALIZED (GTK_WIDGET (dialog)))
    gimp_dialog_realize_callback (GTK_WIDGET (dialog), NULL);
  else
    gtk_signal_connect (GTK_OBJECT (dialog), "realize",
			GTK_SIGNAL_FUNC (gimp_dialog_realize_callback),
			NULL);
}

/**
 * gimp_dialog_create_action_area:
 * @dialog: The #GtkDialog you want to create the action_area for.
 * @...: A #NULL terminated @va_list destribing the action_area buttons.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to
 * gimp_dialog_create_action_areav().
 *
 */
void
gimp_dialog_create_action_area (GtkDialog *dialog,

				/* specify action area buttons as va_list:
				 *  gchar          *label,
				 *  GtkSignalFunc   callback,
				 *  gpointer        data,
				 *  GtkObject      *slot_object,
				 *  GtkWidget     **widget_ptr,
				 *  gboolean        default_action,
				 *  gboolean        connect_delete,
				 */

				...)
{
  va_list args;

  va_start (args, dialog);

  gimp_dialog_create_action_areav (dialog, args);

  va_end (args);
}

/**
 * gimp_dialog_create_action_areav:
 * @dialog: The #GtkDialog you want to create the action_area for.
 * @args: A @va_list as obtained with va_start() describing the action_area
 *        buttons.
 *
 */
void
gimp_dialog_create_action_areav (GtkDialog *dialog,
				 va_list    args)
{
  GtkWidget *hbbox;
  GtkWidget *button;

  /*  action area variables  */
  gchar          *label;
  GtkSignalFunc   callback;
  gpointer        data;
  GtkObject      *slot_object;
  GtkWidget     **widget_ptr;
  gboolean        default_action;
  gboolean        connect_delete;

  gboolean delete_connected = FALSE;

  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  /*  prepare the action_area  */
  label = va_arg (args, gchar *);

  if (label)
    {
      gtk_container_set_border_width (GTK_CONTAINER (dialog->action_area), 2);
      gtk_box_set_homogeneous (GTK_BOX (dialog->action_area), FALSE);

      hbbox = gtk_hbutton_box_new ();
      gtk_button_box_set_spacing (GTK_BUTTON_BOX (hbbox), 4);
      gtk_box_pack_end (GTK_BOX (dialog->action_area), hbbox, FALSE, FALSE, 0);
      gtk_widget_show (hbbox);
    }

  /*  the action_area buttons  */
  while (label)
    {
      callback       = va_arg (args, GtkSignalFunc);
      data           = va_arg (args, gpointer);
      slot_object    = va_arg (args, GtkObject *);
      widget_ptr     = va_arg (args, GtkWidget **);
      default_action = va_arg (args, gboolean);
      connect_delete = va_arg (args, gboolean);

      button = gtk_button_new_with_label (label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (hbbox), button, FALSE, FALSE, 0);

      if (slot_object == (GtkObject *) 1)
	slot_object = GTK_OBJECT (dialog);

      if (data == NULL)
	data = dialog;

      if (callback)
	{
	  if (slot_object)
	    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				       GTK_SIGNAL_FUNC (callback),
				       slot_object);
	  else
	    gtk_signal_connect (GTK_OBJECT (button), "clicked",
				GTK_SIGNAL_FUNC (callback),
				data);
	}

      if (widget_ptr)
	*widget_ptr = button;

      if (connect_delete && callback && !delete_connected)
	{
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_callback",
			       callback);
	  gtk_object_set_data (GTK_OBJECT (dialog),
			       "gimp_dialog_cancel_widget",
			       slot_object ? slot_object : GTK_OBJECT (button));

	  /*  catch the WM delete event  */
	  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			      GTK_SIGNAL_FUNC (gimp_dialog_delete_callback),
			      data);

	  delete_connected = TRUE;
	}

      if (default_action)
	gtk_widget_grab_default (button);
      gtk_widget_show (button);

      label = va_arg (args, gchar *);
    }
}
