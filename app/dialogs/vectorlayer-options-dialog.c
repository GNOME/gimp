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

#include "vectors/gimpvectorlayer.h"

#include "vectorlayer-options-dialog.h"

#include "gimp-intl.h"


#define RESPONSE_RESET 1


/*  local functions  */

static void  vectorlayer_options_dialog_response            (GtkWidget         *widget,
                                                             gint               response_id,
                                                             GtkWidget         *dialog);


/*  public function  */

GtkWidget *
vectorlayer_options_dialog_new (GimpItem    *item,
                                GimpContext *context,
                                const gchar *title,
                                const gchar *stock_id,
                                const gchar *help_id,
                                GtkWidget   *parent)
{
  GtkWidget              *dialog;
  GtkWidget              *main_vbox;
  GimpVectorLayerOptions *vector_layer_options;
  
  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);
  /*g_return_val_if_fail (help_id != NULL, NULL);*/
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);
  
  g_object_get (item, "vector-layer-options", &vector_layer_options, NULL);
  
  dialog = gimp_viewable_dialog_new (GIMP_VIEWABLE (item),
                                     context,
                                     title, "gimp-vectorlayer-options",
                                     stock_id,
                                     _("Choose vector layer options"),
                                     parent,
                                     gimp_standard_help_func,
                                     help_id,

                                     GIMP_STOCK_RESET, RESPONSE_RESET,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_APPLY,  GTK_RESPONSE_APPLY,
                                     stock_id,         GTK_RESPONSE_OK,

                                     NULL);
  
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_APPLY,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  
  g_signal_connect (dialog, "response",
                    G_CALLBACK (vectorlayer_options_dialog_response),
                    dialog);
  
  g_object_set_data (G_OBJECT (dialog), "gimp-item", item);
  
  main_vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), main_vbox);
  gtk_widget_show (main_vbox);
  
  /* The fill color */
  {
    GtkWidget       *fill_label   = NULL;
    GimpColorButton *fill_entry   = NULL;
    GimpFillOptions *fill_options = NULL;
    GimpRGB         *fill_color   = NULL;
    
    fill_label = gtk_label_new ("Fill");
    gtk_misc_set_alignment (GTK_MISC (fill_label), 0.0, 0.0);
    
    gtk_container_add (GTK_CONTAINER(main_vbox), GTK_WIDGET (fill_label));
    gtk_widget_show (GTK_WIDGET (fill_label));
    
    g_object_get (G_OBJECT(vector_layer_options), "fill-options", &fill_options, NULL);
    g_object_get (G_OBJECT(fill_options), "foreground", &fill_color, NULL);
    
    fill_entry = GIMP_COLOR_BUTTON (gimp_color_button_new ("Fill Color",
                                                           32, 32,
                                                           fill_color,
                                                           GIMP_COLOR_AREA_FLAT));
    
    gtk_container_add (GTK_CONTAINER(main_vbox), GTK_WIDGET (fill_entry));
    gtk_widget_show (GTK_WIDGET (fill_entry));
    
    g_object_set_data_full (G_OBJECT (dialog), "gimp-fill-entry", g_object_ref (fill_entry),
                            (GDestroyNotify) g_object_unref);
    
    g_object_unref (fill_options);
  }
  
  /* The stroke color and editor */
  {
    GtkWidget         *stroke_label   = NULL;
    GimpColorButton   *stroke_entry   = NULL;
    GimpStrokeEditor  *stroke_editor  = NULL;
    GimpStrokeDesc    *stroke_desc    = NULL;
    GimpStrokeOptions *stroke_options = NULL;
    GimpStrokeOptions *stroke_options_copy = NULL;
    GimpRGB           *stroke_color   = NULL;
    
    stroke_label = gtk_label_new ("Stroke");
    gtk_misc_set_alignment (GTK_MISC (stroke_label), 0.0, 0.0);
    
    gtk_container_add (GTK_CONTAINER(main_vbox), GTK_WIDGET (stroke_label));
    gtk_widget_show (GTK_WIDGET (stroke_label));
    
    g_object_get (G_OBJECT(vector_layer_options), "stroke-desc", &stroke_desc, NULL);
    g_object_get (G_OBJECT(stroke_desc), "stroke-options", &stroke_options, NULL);
    g_object_get (G_OBJECT(stroke_options), "foreground", &stroke_color, NULL);
    
    /* stroke color */
    stroke_entry = GIMP_COLOR_BUTTON (gimp_color_button_new ("Stroke Color",
                                                           32, 32,
                                                           stroke_color,
                                                           GIMP_COLOR_AREA_FLAT));
    
    gtk_container_add (GTK_CONTAINER(main_vbox), GTK_WIDGET (stroke_entry));
    gtk_widget_show (GTK_WIDGET (stroke_entry));
    
    /* stroke editor */
    stroke_options_copy = gimp_config_duplicate (GIMP_CONFIG (stroke_options));
    stroke_editor = GIMP_STROKE_EDITOR (gimp_stroke_editor_new (stroke_options_copy, 72.0)); /* TODO: put in real resolution */
    
    gtk_container_add (GTK_CONTAINER(main_vbox), GTK_WIDGET (stroke_editor));
    gtk_widget_show (GTK_WIDGET (stroke_editor));
    
    g_object_set_data_full (G_OBJECT (dialog), "gimp-stroke-entry", g_object_ref (stroke_entry),
                            (GDestroyNotify) g_object_unref);
    g_object_set_data_full (G_OBJECT (dialog), "gimp-stroke-editor", g_object_ref (stroke_editor),
                            (GDestroyNotify) g_object_unref);
    
    g_object_unref (stroke_desc);
    g_object_unref (stroke_options);
    g_object_unref (stroke_options_copy);
  }
  
  return dialog;
}

static void
vectorlayer_options_dialog_response            (GtkWidget         *widget,
                                                gint               response_id,
                                                GtkWidget         *dialog)
{
  GimpImage         *image;
  GimpColorButton   *fill_entry;
  GimpColorButton   *stroke_entry;
  GimpStrokeEditor  *stroke_editor;
  GimpItem          *item;
  GimpVectorLayerOptions *vector_layer_options;
  GimpFillOptions   *fill_options;
  GimpStrokeDesc    *stroke_desc;
  GimpStrokeOptions *stroke_options;

  item = g_object_get_data (G_OBJECT (dialog), "gimp-item");
  g_object_get (G_OBJECT (item), "vector-layer-options", &vector_layer_options, NULL);
  image = gimp_item_get_image(item);
  
  fill_entry = g_object_get_data (G_OBJECT (dialog), "gimp-fill-entry");
  stroke_entry = g_object_get_data (G_OBJECT (dialog), "gimp-stroke-entry");
  stroke_editor = g_object_get_data (G_OBJECT (dialog), "gimp-stroke-editor");
  
  g_object_get (G_OBJECT (vector_layer_options), "fill-options", &fill_options, NULL);
  
  g_object_get (G_OBJECT (vector_layer_options), "stroke-desc", &stroke_desc, NULL);
  g_object_get (G_OBJECT (stroke_desc), "stroke-options", &stroke_options, NULL);
  
  switch (response_id)
    {
    case RESPONSE_RESET:
      {
        GimpRGB           *fill_color;
        GimpRGB           *stroke_color;
        GimpStrokeOptions *stroke_options_copy;
        
        g_object_get (G_OBJECT (fill_options), "foreground", &fill_color, NULL);
        gimp_color_button_set_color (fill_entry, fill_color);
        
        g_object_get (G_OBJECT (stroke_options), "foreground", &stroke_color, NULL);
        gimp_color_button_set_color (stroke_entry, stroke_color);
        
        stroke_options_copy = gimp_config_duplicate (GIMP_CONFIG (stroke_options));
        g_object_set (G_OBJECT (stroke_editor), "options", stroke_options_copy, NULL);
        g_object_unref (stroke_options_copy);
      }
      break;

    case GTK_RESPONSE_OK:
    case GTK_RESPONSE_APPLY:
      {
        GimpRGB          fill_color;
        GimpRGB          stroke_color;
        
        gimp_color_button_get_color (fill_entry, &fill_color);
        g_object_set (G_OBJECT (fill_options), "foreground", &fill_color, NULL);
        //g_object_set (G_OBJECT (item), "fill-options", fill_options, NULL);
        
        
        g_object_unref (stroke_options);
        g_object_get (G_OBJECT (stroke_editor), "options", &stroke_options, NULL);
        gimp_color_button_get_color (stroke_entry, &stroke_color);
        g_object_set (G_OBJECT (stroke_options), "foreground", &stroke_color, NULL);
        g_object_set (G_OBJECT (stroke_desc), "stroke-options", stroke_options, NULL);
        
        gimp_vector_layer_refresh (GIMP_VECTOR_LAYER (item));
        gimp_image_flush(image);
        
        if (response_id == GTK_RESPONSE_APPLY)
          break;
      }

    default:
      gtk_widget_destroy (dialog);
      break;
    }
    
    g_object_unref (fill_options);
    g_object_unref (stroke_desc);
    g_object_unref (stroke_options);
}






























