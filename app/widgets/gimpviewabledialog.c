/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpviewabledialog.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "file/file-utils.h"

#include "gimppreview.h"
#include "gimpviewabledialog.h"


static void   gimp_viewable_dialog_class_init (GimpViewableDialogClass *klass);
static void   gimp_viewable_dialog_init       (GimpViewableDialog      *dialog);

static void   gimp_viewable_dialog_name_changed (GimpObject         *object,
                                                 GimpViewableDialog *dialog);


static GimpDialogClass *parent_class = NULL;


GType
gimp_viewable_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      static const GTypeInfo dialog_info =
      {
        sizeof (GimpViewableDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_viewable_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpViewableDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_viewable_dialog_init,
      };

      dialog_type = g_type_register_static (GIMP_TYPE_DIALOG,
					    "GimpViewableDialog",
					    &dialog_info, 0);
    }

  return dialog_type;
}

static void
gimp_viewable_dialog_class_init (GimpViewableDialogClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);
}

static void
gimp_viewable_dialog_init (GimpViewableDialog *dialog)
{
  GtkWidget *ebox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *sep;

  ebox = gtk_event_box_new ();
  gtk_widget_set_state (ebox, GTK_STATE_PRELIGHT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), ebox,
                      FALSE, FALSE, 0);
  gtk_widget_show (ebox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (ebox), frame);
  gtk_widget_show (frame);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  dialog->icon = gtk_image_new ();
  gtk_misc_set_alignment (GTK_MISC (dialog->icon), 0.5, 0.0);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->icon, FALSE, FALSE, 0);
  gtk_widget_show (dialog->icon);

  sep = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
  gtk_widget_show (sep);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (hbox), vbox);
  gtk_widget_show (vbox);

  dialog->desc_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (dialog->desc_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->desc_label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->desc_label);

  dialog->viewable_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (dialog->viewable_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->viewable_label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->viewable_label);
}

GtkWidget *
gimp_viewable_dialog_new (GimpViewable       *viewable,
                          const gchar        *title,
                          const gchar        *wmclass_name,
                          const gchar        *stock_id,
                          const gchar        *desc,
                          GimpHelpFunc        help_func,
                          const gchar        *help_data,

                          /* specify action area buttons
                           * as va_list:
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
  GimpViewableDialog *dialog;
  va_list             args;
  gchar              *escaped;
  gchar              *str;

  g_return_val_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (wmclass_name != NULL, NULL);

  dialog = g_object_new (GIMP_TYPE_VIEWABLE_DIALOG,
                         "title", title,
                         NULL);

  gtk_window_set_wmclass (GTK_WINDOW (dialog), wmclass_name, "Gimp");

  if (help_func)
    gimp_help_connect (GTK_WIDGET (dialog), help_func, help_data);

  va_start (args, help_data);
  gimp_dialog_create_action_areav (GIMP_DIALOG (dialog), args);
  va_end (args);

  gtk_image_set_from_stock (GTK_IMAGE (dialog->icon), stock_id,
                            GTK_ICON_SIZE_LARGE_TOOLBAR);

  escaped = g_markup_escape_text (desc, -1);
  str = g_strdup_printf ("<b><big>%s</big></b>", escaped);
  g_free (escaped);

  gtk_label_set_markup (GTK_LABEL (dialog->desc_label), str);
  g_free (str);

  if (viewable)
    gimp_viewable_dialog_set_viewable (dialog, viewable);

  return GTK_WIDGET (dialog);
}

void
gimp_viewable_dialog_set_viewable (GimpViewableDialog *dialog,
                                   GimpViewable       *viewable)
{
  g_return_if_fail (GIMP_IS_VIEWABLE_DIALOG (dialog));
  g_return_if_fail (! viewable || GIMP_IS_VIEWABLE (viewable));

  if (dialog->preview)
    {
      GimpViewable *old_viewable;

      old_viewable = GIMP_PREVIEW (dialog->preview)->viewable;

      if (viewable == old_viewable)
        return;

      g_signal_handlers_disconnect_by_func (G_OBJECT (old_viewable),
                                            gimp_viewable_dialog_name_changed,
                                            dialog);

      gtk_widget_destroy (dialog->preview);
      dialog->preview = NULL;
    }

  if (viewable)
    {
      g_signal_connect_object (G_OBJECT (viewable),
                               GIMP_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                               G_CALLBACK (gimp_viewable_dialog_name_changed),
                               G_OBJECT (dialog),
                               0);

      gimp_viewable_dialog_name_changed (GIMP_OBJECT (viewable), dialog);

      dialog->preview = gimp_preview_new (viewable, 32, 1, TRUE);
      gtk_box_pack_end (GTK_BOX (dialog->icon->parent), dialog->preview,
                        FALSE, FALSE, 2);
      gtk_widget_show (dialog->preview);
    }
}


/*  private functions  */

static void
gimp_viewable_dialog_name_changed (GimpObject         *object,
                                   GimpViewableDialog *dialog)
{
  const gchar *name;
  gchar       *str;

  name = gimp_object_get_name (object);

  if (GIMP_IS_ITEM (object))
    {
      const gchar *uri;
      gchar       *basename;

      uri = gimp_image_get_uri (gimp_item_get_image (GIMP_ITEM (object)));

      basename = file_utils_uri_to_utf8_basename (uri);
      str = g_strdup_printf ("%s (%s)", name, basename);
      g_free (basename);
    }
  else if (GIMP_IS_IMAGE (object))
    {
      const gchar *uri;

      uri = gimp_image_get_uri (GIMP_IMAGE (object));

      str = file_utils_uri_to_utf8_basename (uri);
    }
  else
    {
      str = g_strdup (name);
    }

  gtk_label_set_text (GTK_LABEL (dialog->viewable_label), str);
  g_free (str);
}
