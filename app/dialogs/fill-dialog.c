/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * fill-dialog.c
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpfilloptions.h"

#include "widgets/gimpfilleditor.h"
#include "widgets/gimpviewabledialog.h"

#include "fill-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  local functions  */

static void  fill_dialog_response (GtkWidget *widget,
                                   gint       response_id,
                                   GtkWidget *dialog);


/*  public function  */

GtkWidget *
fill_dialog_new (GimpItem    *item,
                 GimpContext *context,
                 const gchar *title,
                 const gchar *icon_name,
                 const gchar *help_id,
                 GtkWidget   *parent)
{
  GimpFillOptions *options;
  GimpFillOptions *saved_options;
  GtkWidget       *dialog;
  GtkWidget       *main_vbox;
  GtkWidget       *fill_editor;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  options = gimp_fill_options_new (context->gimp, context, TRUE);

  saved_options = g_object_get_data (G_OBJECT (context->gimp),
                                     "saved-fill-options");

  if (saved_options)
    gimp_config_sync (G_OBJECT (saved_options), G_OBJECT (options), 0);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (item), context,
                                     title, "gimp-fill-options",
                                     icon_name,
                                     _("Choose Fill Style"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     _("_Fill"),       GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (fill_dialog_response),
                    dialog);

  g_object_set_data (G_OBJECT (dialog), "gimp-item", item);
  g_object_set_data_full (G_OBJECT (dialog), "gimp-fill-options", options,
                          (GDestroyNotify) g_object_unref);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  fill_editor = gimp_fill_editor_new (options, FALSE);
  gtk_box_pack_start (GTK_BOX (main_vbox), fill_editor, FALSE, FALSE, 0);
  gtk_widget_show (fill_editor);

  return dialog;
}


/*  private functions  */

static void
fill_dialog_response (GtkWidget  *widget,
                      gint        response_id,
                      GtkWidget  *dialog)
{
  GimpFillOptions *options;
  GimpItem        *item;
  GimpImage       *image;
  GimpContext     *context;

  item    = g_object_get_data (G_OBJECT (dialog), "gimp-item");
  options = g_object_get_data (G_OBJECT (dialog), "gimp-fill-options");

  image   = gimp_item_get_image (item);
  context = GIMP_VIEWABLE_DIALOG (dialog)->context;

  switch (response_id)
    {
    case RESPONSE_RESET:
      gimp_config_reset (GIMP_CONFIG (options));
      break;

    case GTK_RESPONSE_OK:
      {
        GimpDrawable    *drawable = gimp_image_get_active_drawable (image);
        GimpFillOptions *saved_options;
        GError          *error    = NULL;

        if (! drawable)
          {
            gimp_message_literal (context->gimp, G_OBJECT (widget),
                                  GIMP_MESSAGE_WARNING,
                                  _("There is no active layer or channel "
                                    "to fill."));
            return;
          }

        saved_options = g_object_get_data (G_OBJECT (context->gimp),
                                           "saved-fill-options");

        if (saved_options)
          g_object_ref (saved_options);
        else
          saved_options = gimp_fill_options_new (context->gimp, context, TRUE);

        gimp_config_sync (G_OBJECT (options), G_OBJECT (saved_options), 0);

        g_object_set_data_full (G_OBJECT (context->gimp), "saved-fill-options",
                                saved_options,
                                (GDestroyNotify) g_object_unref);

        if (! gimp_item_fill (item, drawable, options, TRUE, NULL, &error))
          {
            gimp_message_literal (context->gimp,
                                  G_OBJECT (widget),
                                  GIMP_MESSAGE_WARNING,
                                  error ? error->message : "NULL");

            g_clear_error (&error);
            return;
          }

        gimp_image_flush (image);
      }
      /* fallthrough */

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}
