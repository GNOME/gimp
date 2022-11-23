/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2003  Henrik Brix Andersen <brix@ligma.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"
#include "core/ligmapaintinfo.h"
#include "core/ligmastrokeoptions.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmacontainercombobox.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmaviewabledialog.h"
#include "widgets/ligmastrokeeditor.h"

#include "stroke-dialog.h"

#include "ligma-intl.h"


#define RESPONSE_RESET 1


typedef struct _StrokeDialog StrokeDialog;

struct _StrokeDialog
{
  LigmaItem           *item;
  GList              *drawables;
  LigmaContext        *context;
  LigmaStrokeOptions  *options;
  LigmaStrokeCallback  callback;
  gpointer            user_data;

  GtkWidget          *tool_combo;
};


/*  local function prototypes  */

static void  stroke_dialog_free     (StrokeDialog *private);
static void  stroke_dialog_response (GtkWidget    *dialog,
                                     gint          response_id,
                                     StrokeDialog *private);


/*  public function  */

GtkWidget *
stroke_dialog_new (LigmaItem           *item,
                   GList              *drawables,
                   LigmaContext        *context,
                   const gchar        *title,
                   const gchar        *icon_name,
                   const gchar        *help_id,
                   GtkWidget          *parent,
                   LigmaStrokeOptions  *options,
                   LigmaStrokeCallback  callback,
                   gpointer            user_data)
{
  StrokeDialog *private;
  LigmaImage    *image;
  GtkWidget    *dialog;
  GtkWidget    *main_vbox;
  GtkWidget    *radio_box;
  GtkWidget    *cairo_radio;
  GtkWidget    *paint_radio;
  GSList       *group;
  GtkWidget    *frame;

  g_return_val_if_fail (LIGMA_IS_ITEM (item), NULL);
  g_return_val_if_fail (drawables, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  image = ligma_item_get_image (item);

  private = g_slice_new0 (StrokeDialog);

  private->item      = item;
  private->drawables = g_list_copy (drawables);
  private->context   = context;
  private->options   = ligma_stroke_options_new (context->ligma, context, TRUE);
  private->callback  = callback;
  private->user_data = user_data;

  ligma_config_sync (G_OBJECT (options),
                    G_OBJECT (private->options), 0);

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, item), context,
                                     title, "ligma-stroke-options",
                                     icon_name,
                                     _("Choose Stroke Style"),
                                     parent,
                                     ligma_standard_help_func,
                                     help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Stroke"), GTK_RESPONSE_OK,

                                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) stroke_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (stroke_dialog_response),
                    private);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  radio_box = ligma_prop_enum_radio_box_new (G_OBJECT (private->options),
                                            "method", -1, -1);

  group = gtk_radio_button_get_group (g_object_get_data (G_OBJECT (radio_box),
                                                         "radio-button"));

  cairo_radio = g_object_ref (group->next->data);
  gtk_container_remove (GTK_CONTAINER (radio_box), cairo_radio);

  paint_radio = g_object_ref (group->data);
  gtk_container_remove (GTK_CONTAINER (radio_box), paint_radio);

  g_object_ref_sink (radio_box);
  g_object_unref (radio_box);

  ligma_label_set_attributes (GTK_LABEL (gtk_bin_get_child (GTK_BIN (cairo_radio))),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  ligma_label_set_attributes (GTK_LABEL (gtk_bin_get_child (GTK_BIN (paint_radio))),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);

  /*  the stroke frame  */

  frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_frame_set_label_widget (GTK_FRAME (frame), cairo_radio);
  g_object_unref (cairo_radio);

  {
    GtkWidget *stroke_editor;
    gdouble    xres;
    gdouble    yres;

    ligma_image_get_resolution (image, &xres, &yres);

    stroke_editor = ligma_stroke_editor_new (private->options, yres, FALSE);
    gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
    gtk_widget_show (stroke_editor);

    g_object_bind_property (cairo_radio,   "active",
                            stroke_editor, "sensitive",
                            G_BINDING_SYNC_CREATE);
  }


  /*  the paint tool frame  */

  frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_frame_set_label_widget (GTK_FRAME (frame), paint_radio);
  g_object_unref (paint_radio);

  {
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;
    GtkWidget *button;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_widget_show (vbox);

    g_object_bind_property (paint_radio, "active",
                            vbox,        "sensitive",
                            G_BINDING_SYNC_CREATE);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new_with_mnemonic (_("P_aint tool:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combo = ligma_container_combo_box_new (image->ligma->paint_info_list,
                                          LIGMA_CONTEXT (private->options),
                                          16, 0);
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    private->tool_combo = combo;

    button = ligma_prop_check_button_new (G_OBJECT (private->options),
                                         "emulate-brush-dynamics",
                                         _("_Emulate brush dynamics"));
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  }

  return dialog;
}


/*  private functions  */

static void
stroke_dialog_free (StrokeDialog *private)
{
  g_object_unref (private->options);
  g_list_free (private->drawables);

  g_slice_free (StrokeDialog, private);
}

static void
stroke_dialog_response (GtkWidget    *dialog,
                        gint          response_id,
                        StrokeDialog *private)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        LigmaToolInfo *tool_info = ligma_context_get_tool (private->context);

        ligma_config_reset (LIGMA_CONFIG (private->options));

        ligma_container_view_select_item (LIGMA_CONTAINER_VIEW (private->tool_combo),
                                         LIGMA_VIEWABLE (tool_info->paint_info));

      }
      break;

    case GTK_RESPONSE_OK:
      private->callback (dialog,
                         private->item,
                         private->drawables,
                         private->context,
                         private->options,
                         private->user_data);
      break;

    default:
      gtk_widget_destroy (dialog);
      break;
    }
}
