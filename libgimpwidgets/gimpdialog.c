/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdialog.c
 * Copyright (C) 2000-2003 Michael Natterer <mitch@gimp.org>
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

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimpdialog.h"
#include "gimphelpui.h"


static void       gimp_dialog_class_init   (GimpDialogClass *klass);
static void       gimp_dialog_init         (GimpDialog      *dialog);

static gboolean   gimp_dialog_delete_event (GtkWidget       *widget,
                                            GdkEventAny     *event);
static void       gimp_dialog_close        (GtkDialog       *dialog);


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
  GtkDialogClass *dialog_class;

  widget_class = GTK_WIDGET_CLASS (klass);
  dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  widget_class->delete_event = gimp_dialog_delete_event;

  dialog_class->close        = gimp_dialog_close;
}

static void
gimp_dialog_init (GimpDialog *dialog)
{
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
}

static gboolean
gimp_dialog_delete_event (GtkWidget   *widget,
                          GdkEventAny *event)
{
  return TRUE;
}

static void
gimp_dialog_close (GtkDialog *dialog)
{
  /* Synthesize delete_event to close dialog. */

  GtkWidget *widget = GTK_WIDGET (dialog);

  if (widget->window)
    {
      GdkEvent *event;

      event = gdk_event_new (GDK_DELETE);

      event->any.window     = g_object_ref (widget->window);
      event->any.send_event = TRUE;

      gtk_main_do_event (event);
      gdk_event_free (event);
    }
}

/**
 * gimp_dialog_new:
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @role:         The dialog's @role which will be set with
 *                gtk_window_set_role().
 * @parent:       The @parent widget of this dialog.
 * @flags:        The @flags (see the #GtkDialog documentation).
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_id:      The help_id which will be passed to @help_func.
 * @...:          A %NULL-terminated @va_list destribing the
 *                action_area buttons.
 *
 * Creates a new @GimpDialog widget.
 *
 * This function simply packs the action_area arguments passed in "..."
 * into a @va_list variable and passes everything to gimp_dialog_new_valist().
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gtk_dialog_new_with_buttons().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_new (const gchar    *title,
                 const gchar    *role,
                 GtkWidget      *parent,
                 GtkDialogFlags  flags,
                 GimpHelpFunc    help_func,
                 const gchar    *help_id,
                 ...)
{
  GtkWidget *dialog;
  va_list    args;

  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);

  va_start (args, help_id);

  dialog = gimp_dialog_new_valist (title, role,
                                   parent, flags,
                                   help_func, help_id,
                                   args);

  va_end (args);

  return dialog;
}

/**
 * gimp_dialog_new_valist:
 * @title:        The dialog's title which will be set with
 *                gtk_window_set_title().
 * @role:         The dialog's @role which will be set with
 *                gtk_window_set_role().
 * @parent:       The @parent widget of this dialog.
 * @flags:        The @flags (see the #GtkDialog documentation).
 * @help_func:    The function which will be called if the user presses "F1".
 * @help_id:      The help_id which will be passed to @help_func.
 * @args:         A @va_list destribing the action_area buttons.
 *
 * Creates a new @GimpDialog widget.
 *
 * For a description of the format of the @va_list describing the
 * action_area buttons see gtk_dialog_new_with_buttons().
 *
 * Returns: A #GimpDialog.
 **/
GtkWidget *
gimp_dialog_new_valist (const gchar    *title,
                        const gchar    *role,
                        GtkWidget      *parent,
                        GtkDialogFlags  flags,
                        GimpHelpFunc    help_func,
                        const gchar    *help_id,
                        va_list         args)
{
  GtkWidget *dialog;

  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_DIALOG,
                         "title", title,
                         NULL);

  gtk_window_set_role (GTK_WINDOW (dialog), role);

  if (parent)
    {
      if (GTK_IS_WINDOW (parent))
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (parent));
        }
      else
        {
          gtk_window_set_screen (GTK_WINDOW (dialog),
                                 gtk_widget_get_screen (parent));
          /* TODO */
        }
    }

  if (flags & GTK_DIALOG_MODAL)
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

  if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
    gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

  if (help_func)
    gimp_help_connect (dialog, help_func, help_id, dialog);

  gimp_dialog_add_buttons_valist (GIMP_DIALOG (dialog), args);

  return dialog;
}

/**
 * gimp_dialog_add_buttons_valist:
 * @dialog: The @dialog to add buttons to.
 * @args:   The buttons as va_list.
 *
 * This function is essentially the same as gtk_dialog_add_buttons()
 * except it takes a va_list instead of '...'
 **/
void
gimp_dialog_add_buttons_valist (GimpDialog *dialog,
                                va_list     args)
{
  const gchar *button_text;
  gint         response_id;

  g_return_if_fail (GIMP_IS_DIALOG (dialog));

  while ((button_text = va_arg (args, const gchar *)))
    {
      response_id = va_arg (args, gint);

      gtk_dialog_add_button (GTK_DIALOG (dialog), button_text, response_id);

      if (response_id == GTK_RESPONSE_OK)
        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
    }
}
