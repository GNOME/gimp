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

#include "config.h"

#include <string.h> /* strcmp */

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimphelpui.h"


static void       gimp_dialog_class_init   (GimpDialogClass *klass);
static void       gimp_dialog_init         (GimpDialog      *dialog);

static gboolean   gimp_dialog_delete_event (GtkWidget       *widget,
                                            GdkEventAny     *event);


static GtkDialogClass *parent_class = NULL;


GType
gimp_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_dialog_init,
      };

      dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
					    "GimpDialog",
					    &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_dialog_class_init (GimpDialogClass *klass)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->delete_event = gimp_dialog_delete_event;
}

static void
gimp_dialog_init (GimpDialog *dialog)
{
}

static gboolean
gimp_dialog_delete_event (GtkWidget   *widget,
                          GdkEventAny *event)
{
  GtkWidget *cancel_widget;

  cancel_widget = (GtkWidget *)
    g_object_get_data (G_OBJECT (widget), "gimp-dialog-cancel-button");

  if (GTK_IS_WIDGET (cancel_widget))
    gtk_widget_activate (cancel_widget);

  return TRUE;
}

/**
 * gimp_dialog_new:
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @wmclass_name: The dialog's @wmclass_name which will be set with
 *                gtk_window_set_wmclass(). The @wmclass_class will be
 *                automatically set to "Gimp".
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_data:    The data pointer which will be passed to @help_func.
 * @position:     The dialog's initial position which will be set with
 *                gtk_window_set_position().
 * @allow_shrink: The dialog's @allow_shrink flag, ...
 * @allow_grow:   ... it't @allow_grow flag and ...
 * @auto_shrink:  ... it's @auto_shrink flag which will all be set with
 *                gtk_window_set_policy().
 * @...:          A #NULL terminated @va_list destribing the
 *                action_area buttons.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to gimp_dialog_newv().
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gimp_dialog_create_action_areav().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_new (const gchar       *title,
		 const gchar       *wmclass_name,
		 GimpHelpFunc       help_func,
		 const gchar       *help_data,
		 GtkWindowPosition  position,
		 gint               allow_shrink,
		 gint               allow_grow,
		 gint               auto_shrink,

		 /* specify action area buttons as va_list:
		  *  const gchar   *label,
		  *  GCallback      callback,
		  *  gpointer       callback_data,
		  *  GObject       *slot_object,
		  *  GtkWidget    **widget_ptr,
		  *  gboolean       default_action,
		  *  gboolean       connect_delete,
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
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @wmclass_name: The dialog's @wmclass_name which will be set with
 *                gtk_window_set_wmclass(). The @wmclass_class will be
 *                automatically set to "Gimp".
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_data:    The data pointer which will be passed to @help_func.
 * @position:     The dialog's initial position which will be set with
 *                gtk_window_set_position().
 * @allow_shrink: The dialog's @allow_shrink flag, ...
 * @allow_grow:   ... it't @allow_grow flag and ...
 * @auto_shrink:  ... it's @auto_shrink flag which will all be set with
 *                gtk_window_set_policy().
 * @args:         A @va_list as obtained with va_start() describing the
 *                action_area buttons.
 *
 * This function performs all neccessary setps to set up a standard GIMP
 * dialog.
 *
 * The @va_list describing the action_area buttons will be passed to
 * gimp_dialog_create_action_areav().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_newv (const gchar       *title,
		  const gchar       *wmclass_name,
		  GimpHelpFunc       help_func,
		  const gchar       *help_data,
		  GtkWindowPosition  position,
		  gint               allow_shrink,
		  gint               allow_grow,
		  gint               auto_shrink,
		  va_list            args)
{
  GtkWidget *dialog;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (wmclass_name != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_DIALOG, NULL);

  gtk_window_set_title (GTK_WINDOW (dialog), title);
  gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, "Gimp");
  gtk_window_set_position (GTK_WINDOW (dialog), position);

#ifdef __GNUC__
#warning FIXME: need a new API for gimp_dialog_new()
#endif

  gtk_window_set_policy (GTK_WINDOW (dialog),
			 allow_shrink, allow_grow, auto_shrink);

  /*  prepare the action_area  */
  gimp_dialog_create_action_areav (GIMP_DIALOG (dialog), args);

  /*  connect the "F1" help key  */
  if (help_func)
    gimp_help_connect (dialog, help_func, help_data);

  return dialog;
}

/**
 * gimp_dialog_create_action_area:
 * @dialog: The #GimpDialog you want to create the action_area for.
 * @...:    A #NULL terminated @va_list destribing the action_area buttons.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to
 * gimp_dialog_create_action_areav().
 **/
void
gimp_dialog_create_action_area (GimpDialog *dialog,

				/* specify action area buttons as va_list:
				 *  const gchar    *label,
				 *  GCallback       callback,
				 *  gpointer        callback_data,
				 *  GObject        *slot_object,
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
 * @dialog: The #GimpDialog you want to create the action_area for.
 * @args: A @va_list as obtained with va_start() describing the action_area
 *        buttons.
 **/
void
gimp_dialog_create_action_areav (GimpDialog *dialog,
				 va_list     args)
{
  GtkWidget    *button;

  /*  action area variables  */
  const gchar  *label;
  GCallback     callback;
  gpointer      callback_data;
  GObject      *slot_object;
  GtkWidget   **widget_ptr;
  gboolean      default_action;
  gboolean      connect_delete;

  gboolean delete_connected = FALSE;

  g_return_if_fail (GIMP_IS_DIALOG (dialog));

  /*  the action_area buttons  */
  while ((label = va_arg (args, const gchar *)))
    {
      callback       = va_arg (args, GCallback);
      callback_data  = va_arg (args, gpointer);
      slot_object    = va_arg (args, GObject *);
      widget_ptr     = va_arg (args, GtkWidget **);
      default_action = va_arg (args, gboolean);
      connect_delete = va_arg (args, gboolean);

      if (slot_object == (GObject *) 1)
	slot_object = G_OBJECT (dialog);

      if (callback_data == NULL)
	callback_data = dialog;

      button = gtk_dialog_add_button (GTK_DIALOG (dialog), label,
                                      GTK_RESPONSE_NONE);

      if (callback)
        {
          if (slot_object)
            g_signal_connect_swapped (G_OBJECT (button), "clicked",
                                      G_CALLBACK (callback),
                                      slot_object);
          else
            g_signal_connect (G_OBJECT (button), "clicked",
                              G_CALLBACK (callback),
                              callback_data);
        }

      if (widget_ptr)
        *widget_ptr = button;

      if (connect_delete && callback && ! delete_connected)
        {
          g_object_set_data (G_OBJECT (dialog), "gimp-dialog-cancel-button",
                             button);

          delete_connected = TRUE;
        }

      if (default_action)
        gtk_widget_grab_default (button);
    }
}
