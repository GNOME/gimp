/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   vector-layer-options-dialog.h
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "core/gimpimage-undo-push.h"
#include "core/gimpstrokeoptions.h"

#include "path/gimppath.h"
#include "path/gimpvectorlayer.h"
#include "path/gimpvectorlayeroptions.h"

#include "widgets/gimpcolorpanel.h"
#include "widgets/gimpcontainercombobox.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpstrokeeditor.h"

#include "vector-layer-options-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  local functions  */

static void  vector_layer_options_dialog_notify        (GObject            *options,
                                                        const GParamSpec   *pspec,
                                                        GtkWidget          *dialog);
static void  vector_layer_options_dialog_response      (GtkWidget          *widget,
                                                        gint                response_id,
                                                        GtkWidget          *dialog);
static void  vector_layer_options_dialog_path_selected (GimpContainerView  *view,
                                                        GtkWidget          *dialog);


/*  public function  */

GtkWidget *
vector_layer_options_dialog_new (GimpVectorLayer *layer,
                                 GimpContext     *context,
                                 const gchar     *title,
                                 const gchar     *icon_name,
                                 const gchar     *help_id,
                                 GtkWidget       *parent)
{
  GimpVectorLayerOptions *saved_options;
  GimpFillOptions        *fill_options;
  GimpStrokeOptions      *stroke_options;
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;
  GtkWidget              *combo;

  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  saved_options  = gimp_config_duplicate (GIMP_CONFIG (layer->options));
  fill_options   = gimp_config_duplicate (GIMP_CONFIG (saved_options->fill_options));
  stroke_options = gimp_config_duplicate (GIMP_CONFIG (saved_options->stroke_options));

  dialog = gimp_viewable_dialog_new (g_list_prepend (NULL, GIMP_VIEWABLE (layer)),
                                     context,
                                     title, "gimp-vectorlayer-options",
                                     icon_name,
                                     _("Edit Vector Layer Attributes"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,
                                     _("_Reset"),  RESPONSE_RESET,
                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Apply"),  GTK_RESPONSE_OK,

                                     NULL);

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

  g_signal_connect_object (saved_options, "notify::enable-fill",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);
  g_signal_connect_object (saved_options, "notify::enable-stroke",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);

  g_signal_connect_object (fill_options, "notify",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);
  g_signal_connect_object (stroke_options, "notify",
                           G_CALLBACK (vector_layer_options_dialog_notify),
                           dialog, 0);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_set_visible (main_vbox, TRUE);


  combo = gimp_container_combo_box_new (gimp_image_get_paths (gimp_item_get_image (GIMP_ITEM (layer))),
                                        context,
                                        GIMP_VIEW_SIZE_SMALL, 1);
  gimp_container_view_set_1_selected (GIMP_CONTAINER_VIEW (combo),
                                      GIMP_VIEWABLE (saved_options->path));
  g_signal_connect_object (combo, "selection-changed",
                           G_CALLBACK (vector_layer_options_dialog_path_selected),
                           dialog, 0);

  gtk_box_pack_start (GTK_BOX (main_vbox), combo, FALSE, FALSE, 0);
  gtk_widget_set_visible (combo, TRUE);

  /* The fill editor */
  {
    GtkWidget *frame;
    GtkWidget *fill_editor;

    fill_editor = gimp_fill_editor_new (fill_options, TRUE, TRUE);
    gtk_widget_set_visible (fill_editor, TRUE);

    frame = gimp_prop_expanding_frame_new (G_OBJECT (saved_options),
                                           "enable-fill", NULL, fill_editor,
                                           NULL);
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  }

  /* The stroke editor */
  {
    GtkWidget *frame;
    GtkWidget *stroke_editor;
    gdouble    xres;
    gdouble    yres;

    gimp_image_get_resolution (gimp_item_get_image (GIMP_ITEM (layer)),
                               &xres, &yres);

    stroke_editor = gimp_stroke_editor_new (stroke_options, yres, TRUE, TRUE);
    gtk_widget_set_visible (stroke_editor, TRUE);

    frame = gimp_prop_expanding_frame_new (G_OBJECT (saved_options),
                                           "enable-stroke", NULL, stroke_editor,
                                           NULL);
    gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
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
  gboolean           enable_fill;
  gboolean           enable_stroke;

  layer = g_object_get_data (G_OBJECT (dialog), "layer");

  enable_fill   = layer->options->enable_fill;
  enable_stroke = layer->options->enable_stroke;

  fill_options   = g_object_get_data (G_OBJECT (dialog), "fill-options");
  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke-options");

  gimp_config_sync (G_OBJECT (fill_options),
                    G_OBJECT (layer->options->fill_options), 0);
  gimp_config_sync (G_OBJECT (stroke_options),
                    G_OBJECT (layer->options->stroke_options), 0);

  if (! strcmp (pspec->name, "enable-fill") ||
      ! strcmp (pspec->name, "enable-stroke"))
    {
      GimpVectorLayerOptions *vector_options;

      vector_options = GIMP_VECTOR_LAYER_OPTIONS (options);

      layer->options->enable_fill   = vector_options->enable_fill;
      layer->options->enable_stroke = vector_options->enable_stroke;
    }

  gimp_vector_layer_refresh (layer);
  gimp_image_flush (gimp_item_get_image (GIMP_ITEM (layer)));
  
  layer->options->enable_fill   = enable_fill;
  layer->options->enable_stroke = enable_stroke;
}

static void
vector_layer_options_dialog_response (GtkWidget *widget,
                                      gint       response_id,
                                      GtkWidget *dialog)
{
  GimpVectorLayer        *layer;
  GimpPath               *path;
  GimpVectorLayerOptions *saved_options;
  GimpFillOptions        *fill_options;
  GimpStrokeOptions      *stroke_options;

  layer = g_object_get_data (G_OBJECT (dialog), "layer");

  saved_options  = g_object_get_data (G_OBJECT (dialog), "saved-options");
  fill_options   = g_object_get_data (G_OBJECT (dialog), "fill-options");
  stroke_options = g_object_get_data (G_OBJECT (dialog), "stroke-options");

  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      if (layer && layer->options)
	    {
          layer->options->enable_fill = saved_options->enable_fill;
          gimp_config_sync (G_OBJECT (saved_options->fill_options),
                           G_OBJECT (layer->options->fill_options), 0);
          layer->options->enable_stroke = saved_options->enable_stroke;
          gimp_config_sync (G_OBJECT (saved_options->stroke_options),
                            G_OBJECT (layer->options->stroke_options), 0);

          gimp_image_undo_push_vector_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                             _("Fill/Stroke Vector Layer"),
                                             layer, NULL);

          gimp_config_sync (G_OBJECT (fill_options),
                            G_OBJECT (layer->options->fill_options), 0);
          gimp_config_sync (G_OBJECT (stroke_options),
                            G_OBJECT (layer->options->stroke_options), 0);
	    }

      gtk_widget_destroy (dialog);
      break;

    default:
      gimp_config_sync (G_OBJECT (saved_options->fill_options),
                        G_OBJECT (fill_options), 0);
      gimp_config_sync (G_OBJECT (saved_options->stroke_options),
                        G_OBJECT (stroke_options), 0);
      if (layer && layer->options)
        {
          g_object_get (saved_options, "path", &path, NULL);
          g_object_set (layer->options, "path", path, NULL);

          gimp_vector_layer_refresh (layer);
          gimp_image_flush (gimp_item_get_image (GIMP_ITEM (layer)));
        }

      if (response_id != RESPONSE_RESET)
        gtk_widget_destroy (dialog);
      break;
    }
}

static void
vector_layer_options_dialog_path_selected (GimpContainerView *view,
                                           GtkWidget         *dialog)
{
  GimpViewable    *item = gimp_container_view_get_1_selected (view);
  GimpPath        *path = NULL;
  GimpVectorLayer *layer;

  layer = g_object_get_data (G_OBJECT (dialog), "layer");

  if (item)
    path = GIMP_PATH (item);

  if (path && GIMP_IS_PATH (path))
    {
      g_object_set (layer->options, "path", path, NULL);

      gimp_vector_layer_refresh (layer);
      gimp_image_flush (gimp_item_get_image (GIMP_ITEM (layer)));
    }
}
