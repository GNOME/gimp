/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2006 Henk Boom
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpstrokeoptions.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpstrokeeditor.h"

#include "vectors/gimpvectorlayer.h"
#include "vectors/gimpvectorlayeroptions.h"

#include "vector-layer-options-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  local functions  */

static void  vector_layer_options_dialog_notify   (GObject          *options,
                                                   const GParamSpec *pspec,
                                                   GtkWidget        *dialog);
static void  vector_layer_options_dialog_response (GtkWidget        *widget,
                                                   gint              response_id,
                                                   GtkWidget        *dialog);


/*  public function  */

GtkWidget *
vector_layer_options_dialog_new (GimpVectorLayer *layer,
                                 GimpContext     *context,
                                 const gchar     *title,
                                 const gchar     *stock_id,
                                 const gchar     *help_id,
                                 GtkWidget       *parent)
{
  GimpVectorLayerOptions *saved_options;
  GimpFillOptions        *fill_options;
  GimpStrokeOptions      *stroke_options;
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;

  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  /* g_return_val_if_fail (help_id != NULL, NULL); */
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  g_printerr ("--> vector_layer_options_dialog_new\n");

  saved_options  = gimp_config_duplicate (GIMP_CONFIG (layer->options));
  fill_options   = gimp_config_duplicate (GIMP_CONFIG (saved_options->fill_options));
  stroke_options = gimp_config_duplicate (GIMP_CONFIG (saved_options->stroke_options));

  dialog = gimp_viewable_dialog_new (g_list_prepend (NULL, layer),
                                     context,
                                     title, "gimp-vectorlayer-options",
                                     stock_id,
                                     _("Choose vector layer options"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     stock_id,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), RESPONSE_RESET);
  gimp_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            RESPONSE_RESET,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (vector_layer_options_dialog_response),
                    dialog);

  g_object_set_data (G_OBJECT (dialog), "layer", layer);

  g_object_set_data_full (G_OBJECT (dialog), "saved-options",
                          saved_options,
                          (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (dialog), "fill-options",
                          fill_options,
                          (GDestroyNotify) g_object_unref);
  g_object_set_data_full (G_OBJECT (dialog), "stroke-options",
                          stroke_options,
                          (GDestroyNotify) g_object_unref);

  g_signal_connect_object (fill_options, "notify",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);
  g_signal_connect_object (stroke_options, "notify",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  // Wormnest - next copied, check last 3 parameters!
//  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
//                      main_vbox, TRUE, TRUE, 0);
  //gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->box), main_vbox);
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), main_vbox);
  gtk_widget_show (main_vbox);

  /* The fill editor */
  {
    GtkWidget *frame;
    GtkWidget *fill_editor;

    frame = gimp_frame_new (_("Fill Style"));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    fill_editor = gimp_fill_editor_new (fill_options, TRUE);
    gtk_container_add (GTK_CONTAINER (frame), fill_editor);
    gtk_widget_show (fill_editor);
  }

  /* The stroke editor */
  {
    GtkWidget *frame;
    GtkWidget *stroke_editor;
    gdouble    xres;
    gdouble    yres;

    frame = gimp_frame_new (_("Stroke Style"));
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);

    gimp_image_get_resolution (gimp_item_get_image (GIMP_ITEM (layer)),
                               &xres, &yres);

    stroke_editor = gimp_stroke_editor_new (stroke_options, yres, TRUE);
    gtk_container_add (GTK_CONTAINER (frame), stroke_editor);
    gtk_widget_show (stroke_editor);
  }

  return dialog;
}

static void
vector_layer_options_dialog_notify (GObject          *options,
                                    const GParamSpec *pspec,
                                    GtkWidget        *dialog)
{
  GimpVectorLayer   *layer;
  GimpFillOptions   *fill_options;
  GimpStrokeOptions *stroke_options;

  layer = g_object_get_data (G_OBJECT (dialog), "layer");

  fill_options   = g_object_get_data (G_OBJECT (dialog), "fill-options");
  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke-options");

  gimp_config_sync (G_OBJECT (fill_options),
                    G_OBJECT (layer->options->fill_options), 0);
  gimp_config_sync (G_OBJECT (stroke_options),
                    G_OBJECT (layer->options->stroke_options), 0);

  gimp_vector_layer_refresh (layer);
  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (layer)));
}

static void
vector_layer_options_dialog_response (GtkWidget *widget,
                                      gint       response_id,
                                      GtkWidget *dialog)
{
  //GimpVectorLayer        *layer;
  GimpVectorLayerOptions *saved_options;
  GimpFillOptions        *fill_options;
  GimpStrokeOptions      *stroke_options;

  //layer = g_object_get_data (G_OBJECT (dialog), "layer");

  saved_options  = g_object_get_data (G_OBJECT (dialog), "saved-options");
  fill_options   = g_object_get_data (G_OBJECT (dialog), "fill-options");
  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke-options");

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      gtk_widget_destroy (dialog);
      break;

    default:
      gimp_config_sync (G_OBJECT (saved_options->fill_options),
                        G_OBJECT (fill_options), 0);
      gimp_config_sync (G_OBJECT (saved_options->stroke_options),
                        G_OBJECT (stroke_options), 0);

      if (response_id != RESPONSE_RESET)
        gtk_widget_destroy (dialog);
      break;
    }
}
