/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimppaintinfo.h"
#include "core/gimpstrokedesc.h"
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
                                                GimpStrokeDesc    *desc);


/*  public function  */

GtkWidget *
stroke_dialog_new (GimpItem    *item,
                   GimpContext *context,
                   const gchar *title,
                   const gchar *stock_id,
                   const gchar *help_id,
                   GtkWidget   *parent)
{
  GimpStrokeDesc *desc;
  GimpStrokeDesc *saved_desc;
  GimpImage      *image;
  GtkWidget      *dialog;
  GtkWidget      *main_vbox;
  GtkWidget      *radio_box;
  GtkWidget      *libart_radio;
  GtkWidget      *paint_radio;
  GSList         *group;
  GtkWidget      *frame;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  image = gimp_item_get_image (item);

  desc = gimp_stroke_desc_new (context->gimp, context);

  saved_desc = g_object_get_data (G_OBJECT (context->gimp),
                                  "saved-stroke-desc");

  if (saved_desc)
    gimp_config_sync (G_OBJECT (saved_desc), G_OBJECT (desc), 0);

  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (item), context,
                                     title, "gimp-stroke-options",
                                     stock_id,
                                     _("Choose Stroke Style"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     stock_id,         GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (stroke_dialog_response),
                    dialog);

  g_object_set_data (G_OBJECT (dialog), "gimp-item", item);
  g_object_set_data_full (G_OBJECT (dialog), "gimp-stroke-desc", desc,
                          (GDestroyNotify) g_object_unref);

  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);

  radio_box = gimp_prop_enum_radio_box_new (G_OBJECT (desc), "method", -1, -1);

  group = gtk_radio_button_get_group (g_object_get_data (G_OBJECT (radio_box),
                                                         "radio-button"));

  libart_radio = g_object_ref (group->next->data);
  gtk_container_remove (GTK_CONTAINER (radio_box), libart_radio);

  paint_radio = g_object_ref (group->data);
  gtk_container_remove (GTK_CONTAINER (radio_box), paint_radio);

  g_object_ref_sink (radio_box);
  g_object_unref (radio_box);

  {
    PangoFontDescription *font_desc;

    font_desc = pango_font_description_new ();
    pango_font_description_set_weight (font_desc, PANGO_WEIGHT_BOLD);

    gtk_widget_modify_font (GTK_BIN (libart_radio)->child, font_desc);
    gtk_widget_modify_font (GTK_BIN (paint_radio)->child,  font_desc);

    pango_font_description_free (font_desc);
  }


  /*  the stroke frame  */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_frame_set_label_widget (GTK_FRAME (frame), libart_radio);
  g_object_unref (libart_radio);

  g_signal_connect (libart_radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_sensitive_update),
                    NULL);

  {
    GtkWidget *stroke_editor;

    stroke_editor = gimp_stroke_editor_new (desc->stroke_options,
                                            image->yresolution);
    gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
    gtk_widget_show (stroke_editor);

    gtk_widget_set_sensitive (stroke_editor,
                              desc->method == GIMP_STROKE_METHOD_LIBART);
    g_object_set_data (G_OBJECT (libart_radio), "set_sensitive", stroke_editor);
  }


  /*  the paint tool frame  */

  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_frame_set_label_widget (GTK_FRAME (frame), paint_radio);
  g_object_unref (paint_radio);

  g_signal_connect (paint_radio, "toggled",
                    G_CALLBACK (gimp_toggle_button_sensitive_update),
                    NULL);

  {
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *combo;

    hbox = gtk_hbox_new (FALSE, 6);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_widget_show (hbox);

    gtk_widget_set_sensitive (hbox,
                              desc->method == GIMP_STROKE_METHOD_PAINT_CORE);
    g_object_set_data (G_OBJECT (paint_radio), "set_sensitive", hbox);

    label = gtk_label_new (_("Paint tool:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    combo = gimp_container_combo_box_new (image->gimp->paint_info_list,
                                          context,
                                          16, 0);
    gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo),
                                     GIMP_VIEWABLE (desc->paint_info));
    gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
    gtk_widget_show (combo);

    g_signal_connect (combo, "select-item",
                      G_CALLBACK (stroke_dialog_paint_info_selected),
                      desc);


    g_object_set_data (G_OBJECT (dialog), "gimp-tool-menu", combo);
  }

  return dialog;
}


/*  private functions  */

static void
stroke_dialog_response (GtkWidget  *widget,
                        gint        response_id,
                        GtkWidget  *dialog)
{
  GimpStrokeDesc *desc;
  GimpItem       *item;
  GimpImage      *image;
  GimpContext    *context;

  item = g_object_get_data (G_OBJECT (dialog), "gimp-item");
  desc = g_object_get_data (G_OBJECT (dialog), "gimp-stroke-desc");

  image   = gimp_item_get_image (item);
  context = GIMP_VIEWABLE_DIALOG (dialog)->context;

  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        GimpToolInfo *tool_info = gimp_context_get_tool (context);
        GtkWidget    *combo     = g_object_get_data (G_OBJECT (dialog),
                                                     "gimp-tool-menu");;

        gimp_config_reset (GIMP_CONFIG (desc));

        gimp_container_view_select_item (GIMP_CONTAINER_VIEW (combo),
                                         GIMP_VIEWABLE (tool_info->paint_info));

      }
      break;

    case GTK_RESPONSE_OK:
      {
        GimpDrawable   *drawable = gimp_image_get_active_drawable (image);
        GimpStrokeDesc *saved_desc;

        if (! drawable)
          {
            gimp_message (context->gimp, G_OBJECT (widget),
                          GIMP_MESSAGE_WARNING,
                          _("There is no active layer or channel "
                            "to stroke to."));
            return;
          }

        saved_desc = g_object_get_data (G_OBJECT (context->gimp),
                                        "saved-stroke-desc");

        if (saved_desc)
          g_object_ref (saved_desc);
        else
          saved_desc = gimp_stroke_desc_new (context->gimp, context);

        gimp_config_sync (G_OBJECT (desc), G_OBJECT (saved_desc), 0);

        g_object_set_data_full (G_OBJECT (context->gimp), "saved-stroke-desc",
                                saved_desc,
                                (GDestroyNotify) g_object_unref);

        gimp_item_stroke (item, drawable, context, desc, FALSE, NULL);
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
                                   GimpStrokeDesc    *desc)
{
  g_object_set (desc, "paint-info", viewable, NULL);
}
