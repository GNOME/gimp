/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpcontainertreeview.h"
#include "widgets/gimpspinscale.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-constructors.h"

#include "layer-options-dialog.h"

#include "gimp-intl.h"


typedef struct _LayerOptionsDialog LayerOptionsDialog;

struct _LayerOptionsDialog
{
  GimpImage                *image;
  GimpLayer                *layer;
  GimpContext              *context;
  GimpLayerModeEffects      mode;
  gdouble                   opacity;
  GimpFillType              fill_type;
  GimpLayerOptionsCallback  callback;
  gpointer                  user_data;

  GtkWidget                *name_entry;
  GtkWidget                *size_se;
  GtkWidget                *offset_se;
  GtkWidget                *rename_toggle;
};


/*  local function prototypes  */

static void   layer_options_dialog_response      (GtkWidget          *dialog,
                                                  gint                response_id,
                                                  LayerOptionsDialog *private);
static void   layer_options_dialog_toggle_rename (GtkWidget          *widget,
                                                  LayerOptionsDialog *private);
static void   layer_options_dialog_free          (LayerOptionsDialog *private);


/*  public functions  */

GtkWidget *
layer_options_dialog_new (GimpImage                *image,
                          GimpLayer                *layer,
                          GimpContext              *context,
                          GtkWidget                *parent,
                          const gchar              *title,
                          const gchar              *role,
                          const gchar              *icon_name,
                          const gchar              *desc,
                          const gchar              *help_id,
                          const gchar              *layer_name,
                          GimpLayerModeEffects      layer_mode,
                          gdouble                   layer_opacity,
                          GimpFillType              layer_fill_type,
                          GimpLayerOptionsCallback  callback,
                          gpointer                  user_data)
{
  LayerOptionsDialog *private;
  GtkWidget          *dialog;
  GimpViewable       *viewable;
  GtkWidget          *vbox;
  GtkWidget          *table;
  GtkWidget          *combo;
  GtkWidget          *scale;
  GtkWidget          *label;
  GtkAdjustment      *adjustment;
  GtkWidget          *spinbutton;
  gdouble             xres;
  gdouble             yres;
  gint                row = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (LayerOptionsDialog);

  private->image     = image;
  private->layer     = layer;
  private->context   = context;
  private->mode      = layer_mode;
  private->opacity   = layer_opacity * 100.0;
  private->fill_type = layer_fill_type;
  private->callback  = callback;
  private->user_data = user_data;

  if (layer)
    viewable = GIMP_VIEWABLE (layer);
  else
    viewable = GIMP_VIEWABLE (image);

  dialog = gimp_viewable_dialog_new (viewable, context,
                                     title, role, icon_name, desc,
                                     parent,
                                     gimp_standard_help_func, help_id,

                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_OK,     GTK_RESPONSE_OK,

                                     NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (layer_options_dialog_response),
                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_options_dialog_free, private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_set_spacing (GTK_BOX (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  table = gtk_table_new (layer ? 5 : 8, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 3, 4);
  if (! layer)
    gtk_table_set_row_spacing (GTK_TABLE (table), 5, 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  The name label and entry  */
  private->name_entry = gtk_entry_new ();
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("Layer _name:"), 0.0, 0.5,
                             private->name_entry, 1, FALSE);

  gtk_entry_set_activates_default (GTK_ENTRY (private->name_entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (private->name_entry), layer_name);

  combo = gimp_paint_mode_menu_new (FALSE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Mode:"), 0.0, 0.5,
                             combo, 1, FALSE);
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->mode,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->mode);

  adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (private->opacity, 0.0, 100.0,
                                                   1.0, 10.0, 0.0));
  scale = gimp_spin_scale_new (adjustment, NULL, 1);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                             _("_Opacity:"), 0.0, 0.5,
                             scale, 1, FALSE);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &private->opacity);

  gimp_image_get_resolution (image, &xres, &yres);

  if (! layer)
    {
      /*  The size labels  */
      label = gtk_label_new (_("Width:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_table_attach (GTK_TABLE (table), label, 0, 1, row + 1, row + 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
      gtk_widget_show (label);

      /*  The size sizeentry  */
      adjustment = (GtkAdjustment *)
        gtk_adjustment_new (1, 1, 1, 1, 10, 0);
      spinbutton = gtk_spin_button_new (adjustment, 1.0, 2);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      private->size_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                              TRUE, TRUE, FALSE, 10,
                                              GIMP_SIZE_ENTRY_UPDATE_SIZE);
      gtk_table_set_col_spacing (GTK_TABLE (private->size_se), 1, 4);
      gtk_table_set_row_spacing (GTK_TABLE (private->size_se), 0, 2);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->size_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_table_attach_defaults (GTK_TABLE (private->size_se), spinbutton,
                                 1, 2, 0, 1);
      gtk_widget_show (spinbutton);

      gtk_table_attach (GTK_TABLE (table), private->size_se, 1, 2, row, row + 2,
                        GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (private->size_se);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->size_se),
                                GIMP_UNIT_PIXEL);

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 0,
                                      xres, FALSE);
      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->size_se), 1,
                                      yres, FALSE);

      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->size_se), 0,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);
      gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->size_se), 1,
                                             GIMP_MIN_IMAGE_SIZE,
                                             GIMP_MAX_IMAGE_SIZE);

      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->size_se), 0,
                                0, gimp_image_get_width  (image));
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->size_se), 1,
                                0, gimp_image_get_height (image));

      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se), 0,
                                  gimp_image_get_width  (image));
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->size_se), 1,
                                  gimp_image_get_height (image));

      row += 2;
    }

  /*  The offset labels  */
  label = gtk_label_new (_("Offset X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row + 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Offset Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row + 1, row + 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK, 0, 0);
  gtk_widget_show (label);

  /*  The offset sizeentry  */
  adjustment = (GtkAdjustment *)
    gtk_adjustment_new (0, 1, 1, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  private->offset_se = gimp_size_entry_new (1, GIMP_UNIT_PIXEL, "%a",
                                            TRUE, TRUE, FALSE, 10,
                                            GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_table_set_col_spacing (GTK_TABLE (private->offset_se), 1, 4);
  gtk_table_set_row_spacing (GTK_TABLE (private->offset_se), 0, 2);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (private->offset_se), spinbutton,
                             1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gtk_table_attach (GTK_TABLE (table), private->offset_se, 1, 2, row, row + 2,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (private->offset_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->offset_se),
                            GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->offset_se), 0,
                                  xres, FALSE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (private->offset_se), 1,
                                  yres, FALSE);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->offset_se), 0,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (private->offset_se), 1,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->offset_se), 0,
                            0, gimp_image_get_width  (image));
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (private->offset_se), 1,
                            0, gimp_image_get_height (image));

  if (layer)
    {
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 0,
                                  gimp_item_get_offset_x (GIMP_ITEM (layer)));
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 1,
                                  gimp_item_get_offset_y (GIMP_ITEM (layer)));
    }
  else
    {
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 0, 0);
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (private->offset_se), 1, 0);
    }

  row += 2;

  if (! layer)
    {
      /*  The fill type  */
      combo = gimp_enum_combo_box_new (GIMP_TYPE_FILL_TYPE);
      gimp_table_attach_aligned (GTK_TABLE (table), 0, row++,
                                 _("_Fill with:"), 0.0, 0.5,
                                 combo, 1, FALSE);
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  private->fill_type,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &private->fill_type);
    }

  if (layer)
    {
      GimpContainer *filters;
      GtkWidget     *frame;
      GtkWidget     *view;

      /*  For text layers add a toggle to control "auto-rename"  */
      if (gimp_item_is_text_layer (GIMP_ITEM (layer)))
        {
          private->rename_toggle =
            gtk_check_button_new_with_mnemonic (_("Set name from _text"));

          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (private->rename_toggle),
                                        GIMP_TEXT_LAYER (layer)->auto_rename);

          gtk_table_attach (GTK_TABLE (table), private->rename_toggle, 1, 2, 1, 2,
                            GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (private->rename_toggle);

          g_signal_connect (private->rename_toggle, "toggled",
                            G_CALLBACK (layer_options_dialog_toggle_rename),
                            private);
        }

      frame = gimp_frame_new ("Active Filters");
      gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      filters = gimp_drawable_get_filters (GIMP_DRAWABLE (layer));

      view = gimp_container_tree_view_new (filters, context,
                                           GIMP_VIEW_SIZE_SMALL, 0);
      gtk_container_add (GTK_CONTAINER (frame), view);
      gtk_widget_show (view);
    }

  return dialog;
}


/*  private functions  */

static void
layer_options_dialog_response (GtkWidget          *dialog,
                               gint                response_id,
                               LayerOptionsDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      const gchar *name;
      gint         width             = 0;
      gint         height            = 0;
      gint         offset_x;
      gint         offset_y;
      gboolean     rename_text_layer = FALSE;

      name = gtk_entry_get_text (GTK_ENTRY (private->name_entry));

      if (! private->layer)
        {
          width =
            RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se),
                                              0));
          height =
            RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->size_se),
                                              1));
        }

      offset_x =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se),
                                          0));
      offset_y =
        RINT (gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (private->offset_se),
                                          1));

          if (private->layer &&
          gimp_item_is_text_layer (GIMP_ITEM (private->layer)))
        {
          rename_text_layer =
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (private->rename_toggle));
        }

      private->callback (dialog,
                         private->image,
                         private->layer,
                         private->context,
                         name,
                         private->mode,
                         private->opacity / 100.0,
                         private->fill_type,
                         width,
                         height,
                         offset_x,
                         offset_y,
                         rename_text_layer,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}

static void
layer_options_dialog_toggle_rename (GtkWidget          *widget,
                                    LayerOptionsDialog *private)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      gimp_item_is_text_layer (GIMP_ITEM (private->layer)))
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (private->layer);
      GimpText      *text       = gimp_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          gchar *name = gimp_utf8_strtrim (text->text, 30);

          gtk_entry_set_text (GTK_ENTRY (private->name_entry), name);

          g_free (name);
        }
    }
}

static void
layer_options_dialog_free (LayerOptionsDialog *private)
{
  g_slice_free (LayerOptionsDialog, private);
}
