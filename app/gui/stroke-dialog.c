/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@gimp.org>
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

#include "gui-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-utils.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokeoptions.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpstrokeeditor.h"

#include "stroke-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  local functions  */

static void  stroke_dialog_response            (GtkWidget         *widget,
                                                gint               response_id,
                                                GtkWidget         *dialog);
static void  stroke_dialog_paint_info_selected (GimpContainerView *view,
                                                GimpViewable      *viewable,
                                                gpointer           insert_data,
                                                GtkWidget         *dialog);


/*  public function  */


GtkWidget *
stroke_dialog_new (GimpItem    *item,
                   const gchar *stock_id,
                   const gchar *help_id,
                   GtkWidget   *parent)
{
  GimpContext       *context;
  GimpStrokeOptions *options;
  GimpStrokeOptions *saved_options;
  GimpImage         *image;
  GtkWidget         *dialog;
  GtkWidget         *main_vbox;
  GtkWidget         *button;
  GSList            *group;
  GtkWidget         *frame;
  GimpToolInfo      *tool_info;
  gboolean           libart_stroking = TRUE;


  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  image     = gimp_item_get_image (item);
  context   = gimp_get_user_context (image->gimp);
  tool_info = gimp_context_get_tool (context);

  options = g_object_new (GIMP_TYPE_STROKE_OPTIONS,
                          "gimp", image->gimp,
                          NULL);
  g_object_set_data (G_OBJECT (options), "libart-stroking",
                     GINT_TO_POINTER (libart_stroking));
  g_object_set_data (G_OBJECT (options), "gimp-paint-info",
                     gimp_context_get_tool (context)->paint_info);

  saved_options = g_object_get_data (G_OBJECT (context),
                                     "saved-stroke-options");
  if (saved_options)
    {
      gimp_config_sync (GIMP_CONFIG (saved_options), GIMP_CONFIG (options), 0);
      libart_stroking = GPOINTER_TO_INT (g_object_get_data
                                             (G_OBJECT (saved_options),
                                              "libart-stroking"));

      g_object_set_data (G_OBJECT (options), "libart-stroking",
                         GINT_TO_POINTER (libart_stroking));
      g_object_set_data (G_OBJECT (options), "gimp-paint-info",
                         g_object_get_data (G_OBJECT (saved_options),
                                            "gimp-paint-info"));
    }

  gimp_context_set_parent (GIMP_CONTEXT (options), context);
  gimp_context_define_properties (GIMP_CONTEXT (options),
                                  GIMP_CONTEXT_FOREGROUND_MASK |
                                  GIMP_CONTEXT_PATTERN_MASK,
                                  FALSE);

  /* the dialog */
  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (item),
                              _("Stroke Options"), "gimp-stroke-options",
                              stock_id,
                              _("Choose Stroke Style"),
                              parent,
                              gimp_standard_help_func,
                              help_id,

                              GIMP_STOCK_RESET, RESPONSE_RESET,
                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (stroke_dialog_response),
                    dialog);

  g_object_set_data (G_OBJECT (dialog), "gimp-item", item);
  g_object_set_data_full (G_OBJECT (dialog), "gimp-stroke-options", options,
                          (GDestroyNotify) g_object_unref);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);


  /*  the stroke frame  */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_radio_button_new_with_label (NULL, _("Stroke Line"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_sensitive_update),
                    NULL);

  g_object_set_data (G_OBJECT (dialog), "gimp-stroke-button", button);

  {
    GtkWidget *stroke_editor;

    stroke_editor = gimp_stroke_editor_new (options, image->yresolution);
    gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
    gtk_widget_show (stroke_editor);

    g_object_set_data (G_OBJECT (button), "set_sensitive", stroke_editor);
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), libart_stroking);

  /*  the paint tool frame  */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_radio_button_new_with_label (group,
                                            _("Stroke With a Paint Tool"));
  group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_sensitive_update),
                    NULL);

  {
    GtkWidget     *hbox;
    GtkWidget     *label;
    GtkWidget     *combo;
    GimpPaintInfo *paint_info;

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_widget_show (hbox);

    gtk_widget_set_sensitive (GTK_WIDGET (hbox), FALSE);
    g_object_set_data (G_OBJECT (button), "set_sensitive", hbox);

    label = gtk_label_new (_("Paint Tool:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combo = gimp_container_combo_box_new (image->gimp->paint_info_list, NULL,
                                          16, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    g_signal_connect (combo, "select_item",
                      G_CALLBACK (stroke_dialog_paint_info_selected),
                      dialog);

    paint_info = GIMP_PAINT_INFO (g_object_get_data (G_OBJECT (options),
                                                     "gimp-paint-info"));
    gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo),
                                     GIMP_VIEWABLE (paint_info));

    g_object_set_data (G_OBJECT (dialog), "gimp-tool-menu",  combo);
    g_object_set_data (G_OBJECT (dialog), "gimp-paint-info", paint_info);
  }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), !libart_stroking);

  return dialog;
}


/*  private functions  */

static void
stroke_dialog_response (GtkWidget  *widget,
                        gint        response_id,
                        GtkWidget  *dialog)
{
  GimpContext *context;
  GimpItem    *item;
  GtkWidget   *button;
  GimpImage   *image;

  item   = g_object_get_data (G_OBJECT (dialog), "gimp-item");
  button = g_object_get_data (G_OBJECT (dialog), "gimp-stroke-button");

  image   = gimp_item_get_image (item);
  context = gimp_get_user_context (image->gimp);

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        GObject      *options;
        GtkWidget    *combo;
        GimpToolInfo *tool_info;

        options = g_object_get_data (G_OBJECT (dialog), "gimp-stroke-options");
        combo   = g_object_get_data (G_OBJECT (dialog), "gimp-tool-menu");

        tool_info = gimp_context_get_tool (context);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
        gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo),
                                         GIMP_VIEWABLE (tool_info->paint_info));

        gimp_config_reset (GIMP_CONFIG (options));
      }
      break;

    case GTK_RESPONSE_OK:
      {
        GimpDrawable *drawable;
        GimpObject   *options;
        GObject      *saved_options;

        drawable = gimp_image_active_drawable (image);

        if (! drawable)
          {
            g_message (_("There is no active layer or channel to stroke to."));
            return;
          }

        saved_options = g_object_get_data (G_OBJECT (context),
                                           "saved-stroke-options");
        options = g_object_get_data (G_OBJECT (dialog),
                                     "gimp-stroke-options");

        if (saved_options)
          {
            gimp_config_sync (GIMP_CONFIG (options),
                              GIMP_CONFIG (saved_options), 0);
          }
        else
          {
            g_object_set_data_full (G_OBJECT (context),
                                    "saved-stroke-options",
                                    g_object_ref (options),
                                    (GDestroyNotify) g_object_unref);
            saved_options = G_OBJECT (options);
          }

        g_object_set_data (saved_options, "gimp-paint-info",
                           g_object_get_data (G_OBJECT (dialog),
                                              "gimp-paint-info"));

        if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button)))
          {
            g_object_set_data (saved_options, "libart-stroking",
                               GINT_TO_POINTER (TRUE));
          }
        else
          {
            g_object_set_data (saved_options, "libart-stroking",
                               GINT_TO_POINTER (FALSE));
            options = g_object_get_data (G_OBJECT (dialog), "gimp-paint-info");
          }

        gimp_item_stroke (item, drawable, context, options, FALSE);
        gimp_image_flush (image);
      }
      /* fallthrough */

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}

static void
stroke_dialog_paint_info_selected (GimpContainerView *view,
                                   GimpViewable      *viewable,
                                   gpointer           insert_data,
                                   GtkWidget         *dialog)
{
  g_object_set_data (G_OBJECT (dialog), "gimp-paint-info", viewable);
}
