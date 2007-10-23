/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpviewabledialog.h"

#include "layer-options-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   layer_options_dialog_toggle_rename (GtkWidget          *widget,
                                                  LayerOptionsDialog *options);
static void   layer_options_dialog_free          (LayerOptionsDialog *options);


/*  public functions  */

LayerOptionsDialog *
layer_options_dialog_new (GimpImage    *image,
                          GimpLayer    *layer,
                          GimpContext  *context,
                          GtkWidget    *parent,
                          const gchar  *layer_name,
                          GimpFillType  layer_fill_type,
                          const gchar  *title,
                          const gchar  *role,
                          const gchar  *stock_id,
                          const gchar  *desc,
                          const gchar  *help_id)
{
  LayerOptionsDialog *options;
  GimpViewable       *viewable;
  GtkWidget          *vbox;
  GtkWidget          *table;
  GtkWidget          *label;
  GtkObject          *adjustment;
  GtkWidget          *spinbutton;
  GtkWidget          *frame;
  GtkWidget          *button;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  options = g_slice_new0 (LayerOptionsDialog);

  options->image     = image;
  options->context   = context;
  options->layer     = layer;
  options->fill_type = layer_fill_type;

  if (layer)
    viewable = GIMP_VIEWABLE (layer);
  else
    viewable = GIMP_VIEWABLE (image);

  options->dialog =
    gimp_viewable_dialog_new (viewable, context,
                              title, role, stock_id, desc,
                              parent,
                              gimp_standard_help_func, help_id,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              GTK_STOCK_OK,     GTK_RESPONSE_OK,

                              NULL);

  g_object_weak_ref (G_OBJECT (options->dialog),
                     (GWeakNotify) layer_options_dialog_free, options);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (options->dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  vbox = gtk_vbox_new (FALSE, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (options->dialog)->vbox),
                     vbox);
  gtk_widget_show (vbox);

  table = gtk_table_new (layer ? 1 : 3, 2, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The name label and entry  */
  options->name_entry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Layer _name:"), 0.0, 0.5,
                             options->name_entry, 1, FALSE);

  gtk_entry_set_activates_default (GTK_ENTRY (options->name_entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (options->name_entry), layer_name);

  if (! layer)
    {
      /*  The size labels  */
      label = gtk_label_new (_("Width:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
      gtk_widget_show (label);

      /*  The size sizeentry  */
      spinbutton = gimp_spin_button_new (&adjustment,
                                         1, 1, 1, 1, 10, 1,
                                         1, 2);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      options->size_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                              TRUE, TRUE, FALSE, 10,
                                              GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gtk_table_set_col_spacing (GTK_TABLE (options->size_se), 1, 4);
      gtk_table_set_row_spacing (GTK_TABLE (options->size_se), 0, 2);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (options->size_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_table_attach_defaults (GTK_TABLE (options->size_se), spinbutton,
                                 1, 2, 0, 1);
      gtk_widget_show (spinbutton);

      gtk_table_attach (GTK_TABLE (table), options->size_se, 1, 2, 1, 3,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (options->size_se);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (options->size_se),
                                GIMP_UNIT_PIXEL);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 0,
                                      image->xresolution, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (options->size_se), 1,
                                      image->yresolution, FALSE);

      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 0,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (options->size_se), 1,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 0,
                                0, image->width);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (options->size_se), 1,
                                0, image->height);

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 0,
                                  image->width);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (options->size_se), 1,
                                  image->height);

      /*  The radio frame  */
      frame = gimp_enum_radio_frame_new_with_range (GIMP_TYPE_FILL_TYPE,
                                                    GIMP_FOREGROUND_FILL,
                                                    GIMP_TRANSPARENT_FILL,
                                                    gtk_label_new (_("Layer Fill Type")),
                                                    G_CALLBACK (gimp_radio_button_update),
                                                    &options->fill_type,
                                                    &button);
      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                       options->fill_type);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }
  else
    {
      /*  For text layers add a toggle to control "auto-rename"  */
      if (gimp_drawable_is_text_layer (GIMP_DRAWABLE (layer)))
        {
          options->rename_toggle =
            gtk_check_button_new_with_mnemonic (_("Set name from _text"));

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (options->rename_toggle),
                                        GIMP_TEXT_LAYER (layer)->auto_rename);

          gtk_table_attach (GTK_TABLE (table), options->rename_toggle, 1, 2, 1, 2,
                            GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (options->rename_toggle);

          g_signal_connect (options->rename_toggle, "toggled",
                            G_CALLBACK (layer_options_dialog_toggle_rename),
                            options);
        }
    }

  return options;
}


/*  private functions  */

static void
layer_options_dialog_toggle_rename (GtkWidget          *widget,
                                    LayerOptionsDialog *options)
{
  if (GTK_TOGGLE_BUTTON (widget)->active &&
      gimp_drawable_is_text_layer (GIMP_DRAWABLE (options->layer)))
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (options->layer);
      GimpText      *text       = gimp_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          gchar *name = gimp_utf8_strtrim (text->text, 30);

          gtk_entry_set_text (GTK_ENTRY (options->name_entry), name);

          g_free (name);
        }
    }
}

static void
layer_options_dialog_free (LayerOptionsDialog *options)
{
  g_slice_free (LayerOptionsDialog, options);
}
