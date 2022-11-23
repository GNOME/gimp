/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "core/ligmacontext.h"
#include "core/ligmadrawable-filters.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"

#include "text/ligmatext.h"
#include "text/ligmatextlayer.h"

#include "widgets/ligmacontainertreeview.h"
#include "widgets/ligmalayermodebox.h"
#include "widgets/ligmaviewabledialog.h"

#include "item-options-dialog.h"
#include "layer-options-dialog.h"

#include "ligma-intl.h"


typedef struct _LayerOptionsDialog LayerOptionsDialog;

struct _LayerOptionsDialog
{
  LigmaLayer                *layer;
  LigmaLayerMode             mode;
  LigmaLayerColorSpace       blend_space;
  LigmaLayerColorSpace       composite_space;
  LigmaLayerCompositeMode    composite_mode;
  gdouble                   opacity;
  LigmaFillType              fill_type;
  gboolean                  lock_alpha;
  gboolean                  rename_text_layers;
  LigmaLayerOptionsCallback  callback;
  gpointer                  user_data;

  GtkWidget                *mode_box;
  GtkWidget                *blend_space_combo;
  GtkWidget                *composite_space_combo;
  GtkWidget                *composite_mode_combo;
  GtkWidget                *size_se;
  GtkWidget                *offset_se;
};


/*  local function prototypes  */

static void   layer_options_dialog_free           (LayerOptionsDialog *private);
static void   layer_options_dialog_callback       (GtkWidget          *dialog,
                                                   LigmaImage          *image,
                                                   LigmaItem           *item,
                                                   LigmaContext        *context,
                                                   const gchar        *item_name,
                                                   gboolean            item_visible,
                                                   LigmaColorTag        item_color_tag,
                                                   gboolean            item_lock_content,
                                                   gboolean            item_lock_position,
                                                   gpointer            user_data);
static void
     layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private);
static void   layer_options_dialog_mode_notify    (GtkWidget          *widget,
                                                   const GParamSpec   *pspec,
                                                   LayerOptionsDialog *private);
static void   layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                                   LayerOptionsDialog *private);


/*  public functions  */

GtkWidget *
layer_options_dialog_new (LigmaImage                *image,
                          LigmaLayer                *layer,
                          LigmaContext              *context,
                          GtkWidget                *parent,
                          const gchar              *title,
                          const gchar              *role,
                          const gchar              *icon_name,
                          const gchar              *desc,
                          const gchar              *help_id,
                          const gchar              *layer_name,
                          LigmaLayerMode             layer_mode,
                          LigmaLayerColorSpace       layer_blend_space,
                          LigmaLayerColorSpace       layer_composite_space,
                          LigmaLayerCompositeMode    layer_composite_mode,
                          gdouble                   layer_opacity,
                          LigmaFillType              layer_fill_type,
                          gboolean                  layer_visible,
                          LigmaColorTag              layer_color_tag,
                          gboolean                  layer_lock_content,
                          gboolean                  layer_lock_position,
                          gboolean                  layer_lock_alpha,
                          LigmaLayerOptionsCallback  callback,
                          gpointer                  user_data)
{
  LayerOptionsDialog   *private;
  GtkWidget            *dialog;
  GtkWidget            *grid;
  GtkListStore         *space_model;
  GtkWidget            *combo;
  GtkWidget            *scale;
  GtkWidget            *label;
  GtkAdjustment        *adjustment;
  GtkWidget            *spinbutton;
  GtkWidget            *button;
  LigmaLayerModeContext  mode_context;
  gdouble               xres;
  gdouble               yres;
  gint                  row = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || LIGMA_IS_LAYER (layer), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (LayerOptionsDialog);

  private->layer              = layer;
  private->mode               = layer_mode;
  private->blend_space        = layer_blend_space;
  private->composite_space    = layer_composite_space;
  private->composite_mode     = layer_composite_mode;
  private->opacity            = layer_opacity * 100.0;
  private->fill_type          = layer_fill_type;
  private->lock_alpha         = layer_lock_alpha;
  private->rename_text_layers = FALSE;
  private->callback           = callback;
  private->user_data          = user_data;

  if (layer && ligma_item_is_text_layer (LIGMA_ITEM (layer)))
    private->rename_text_layers = LIGMA_TEXT_LAYER (layer)->auto_rename;

  dialog = item_options_dialog_new (image, LIGMA_ITEM (layer), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    _("Layer _name:"),
                                    LIGMA_ICON_TOOL_PAINTBRUSH,
                                    _("Lock _pixels"),
                                    _("Lock position and _size"),
                                    layer_name,
                                    layer_visible,
                                    layer_color_tag,
                                    layer_lock_content,
                                    layer_lock_position,
                                    layer_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_options_dialog_free, private);

  if (! layer || ligma_viewable_get_children (LIGMA_VIEWABLE (layer)) == NULL)
    mode_context = LIGMA_LAYER_MODE_CONTEXT_LAYER;
  else
    mode_context = LIGMA_LAYER_MODE_CONTEXT_GROUP;

  private->mode_box = ligma_layer_mode_box_new (mode_context);
  item_options_dialog_add_widget (dialog, _("_Mode:"), private->mode_box);
  ligma_layer_mode_box_set_mode (LIGMA_LAYER_MODE_BOX (private->mode_box),
                                private->mode);

  g_signal_connect (private->mode_box, "notify::layer-mode",
                    G_CALLBACK (layer_options_dialog_mode_notify),
                    private);

  space_model =
    ligma_enum_store_new_with_range (LIGMA_TYPE_LAYER_COLOR_SPACE,
                                    LIGMA_LAYER_COLOR_SPACE_AUTO,
                                    LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL);

  private->blend_space_combo = combo =
    ligma_enum_combo_box_new_with_model (LIGMA_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("_Blend space:"), combo);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (combo),
                                       "ligma-layer-color-space");
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              private->blend_space,
                              G_CALLBACK (ligma_int_combo_box_get_active),
                              &private->blend_space, NULL);

  private->composite_space_combo = combo =
    ligma_enum_combo_box_new_with_model (LIGMA_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("Compos_ite space:"), combo);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (combo),
                                       "ligma-layer-color-space");
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              private->composite_space,
                              G_CALLBACK (ligma_int_combo_box_get_active),
                              &private->composite_space, NULL);

  g_object_unref (space_model);

  private->composite_mode_combo = combo =
    ligma_enum_combo_box_new (LIGMA_TYPE_LAYER_COMPOSITE_MODE);
  item_options_dialog_add_widget (dialog, _("Composite mo_de:"), combo);
  ligma_enum_combo_box_set_icon_prefix (LIGMA_ENUM_COMBO_BOX (combo),
                                       "ligma-layer-composite");
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              private->composite_mode,
                              G_CALLBACK (ligma_int_combo_box_get_active),
                              &private->composite_mode, NULL);

  /*  set the sensitivity of above 3 menus  */
  layer_options_dialog_update_mode_sensitivity (private);

  adjustment = gtk_adjustment_new (private->opacity, 0.0, 100.0,
                                   1.0, 10.0, 0.0);
  scale = ligma_spin_scale_new (adjustment, NULL, 1);
  item_options_dialog_add_widget (dialog, _("_Opacity:"), scale);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (ligma_double_adjustment_update),
                    &private->opacity);

  grid = item_options_dialog_get_grid (dialog, &row);

  ligma_image_get_resolution (image, &xres, &yres);

  if (! layer)
    {
      /*  The size labels  */
      label = gtk_label_new (_("Width:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
      gtk_widget_show (label);

      label = gtk_label_new (_("Height:"));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
      gtk_widget_show (label);

      /*  The size sizeentry  */
      adjustment = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
      spinbutton = ligma_spin_button_new (adjustment, 1.0, 2);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      private->size_se = ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                              TRUE, TRUE, FALSE, 10,
                                              LIGMA_SIZE_ENTRY_UPDATE_SIZE);

      ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->size_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_grid_attach (GTK_GRID (private->size_se), spinbutton, 1, 0, 1, 1);
      gtk_widget_show (spinbutton);

      gtk_grid_attach (GTK_GRID (grid), private->size_se, 1, row, 1, 2);
      gtk_widget_show (private->size_se);

      ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (private->size_se),
                                LIGMA_UNIT_PIXEL);

      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                      xres, FALSE);
      ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                      yres, FALSE);

      ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                             LIGMA_MIN_IMAGE_SIZE,
                                             LIGMA_MAX_IMAGE_SIZE);
      ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                             LIGMA_MIN_IMAGE_SIZE,
                                             LIGMA_MAX_IMAGE_SIZE);

      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                0, ligma_image_get_width  (image));
      ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                0, ligma_image_get_height (image));

      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->size_se), 0,
                                  ligma_image_get_width  (image));
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->size_se), 1,
                                  ligma_image_get_height (image));

      row += 2;
    }

  /*  The offset labels  */
  label = gtk_label_new (_("Offset X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("Offset Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_show (label);

  /*  The offset sizeentry  */
  adjustment = gtk_adjustment_new (0, 1, 1, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  private->offset_se = ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                            TRUE, TRUE, FALSE, 10,
                                            LIGMA_SIZE_ENTRY_UPDATE_SIZE);

  ligma_size_entry_add_field (LIGMA_SIZE_ENTRY (private->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (private->offset_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gtk_grid_attach (GTK_GRID (grid), private->offset_se, 1, row, 1, 2);
  gtk_widget_show (private->offset_se);

  ligma_size_entry_set_unit (LIGMA_SIZE_ENTRY (private->offset_se),
                            LIGMA_UNIT_PIXEL);

  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->offset_se), 0,
                                  xres, FALSE);
  ligma_size_entry_set_resolution (LIGMA_SIZE_ENTRY (private->offset_se), 1,
                                  yres, FALSE);

  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->offset_se), 0,
                                         -LIGMA_MAX_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (private->offset_se), 1,
                                         -LIGMA_MAX_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);

  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (private->offset_se), 0,
                            0, ligma_image_get_width  (image));
  ligma_size_entry_set_size (LIGMA_SIZE_ENTRY (private->offset_se), 1,
                            0, ligma_image_get_height (image));

  if (layer)
    {
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset_se), 0,
                                  ligma_item_get_offset_x (LIGMA_ITEM (layer)));
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset_se), 1,
                                  ligma_item_get_offset_y (LIGMA_ITEM (layer)));
    }
  else
    {
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset_se), 0, 0);
      ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (private->offset_se), 1, 0);
    }

  row += 2;

  if (! layer)
    {
      /*  The fill type  */
      combo = ligma_enum_combo_box_new (LIGMA_TYPE_FILL_TYPE);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                _("_Fill with:"), 0.0, 0.5,
                                combo, 1);
      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  private->fill_type,
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &private->fill_type, NULL);
    }

  if (layer)
    {
      GtkWidget     *left_vbox = item_options_dialog_get_vbox (dialog);
      GtkWidget     *frame;
      LigmaContainer *filters;
      GtkWidget     *view;

      frame = ligma_frame_new (_("Active Filters"));
      gtk_box_pack_start (GTK_BOX (left_vbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      filters = ligma_drawable_get_filters (LIGMA_DRAWABLE (layer));

      view = ligma_container_tree_view_new (filters, context,
                                           LIGMA_VIEW_SIZE_SMALL, 0);
      gtk_container_add (GTK_CONTAINER (frame), view);
      gtk_widget_show (view);
    }

  button = item_options_dialog_get_lock_position (dialog);

  if (private->size_se)
    g_object_bind_property (G_OBJECT (button),           "active",
                            G_OBJECT (private->size_se), "sensitive",
                            G_BINDING_SYNC_CREATE |
                            G_BINDING_INVERT_BOOLEAN);

  g_object_bind_property (G_OBJECT (button),             "active",
                          G_OBJECT (private->offset_se), "sensitive",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  button = item_options_dialog_add_switch (dialog,
                                           LIGMA_ICON_TRANSPARENCY,
                                           _("Lock _alpha"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->lock_alpha);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &private->lock_alpha);

  /*  For text layers add a toggle to control "auto-rename"  */
  if (layer && ligma_item_is_text_layer (LIGMA_ITEM (layer)))
    {
      button = item_options_dialog_add_switch (dialog,
                                               LIGMA_ICON_TOOL_TEXT,
                                               _("Set name from _text"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    private->rename_text_layers);
      g_signal_connect (button, "toggled",
                        G_CALLBACK (ligma_toggle_button_update),
                        &private->rename_text_layers);

      g_signal_connect (button, "toggled",
                        G_CALLBACK (layer_options_dialog_rename_toggled),
                        private);
    }

  return dialog;
}


/*  private functions  */

static void
layer_options_dialog_free (LayerOptionsDialog *private)
{
  g_slice_free (LayerOptionsDialog, private);
}

static void
layer_options_dialog_callback (GtkWidget    *dialog,
                               LigmaImage    *image,
                               LigmaItem     *item,
                               LigmaContext  *context,
                               const gchar  *item_name,
                               gboolean      item_visible,
                               LigmaColorTag  item_color_tag,
                               gboolean      item_lock_content,
                               gboolean      item_lock_position,
                               gpointer      user_data)
{
  LayerOptionsDialog *private = user_data;
  gint                width   = 0;
  gint                height  = 0;
  gint                offset_x;
  gint                offset_y;

  if (private->size_se)
    {
      width =
        RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (private->size_se),
                                          0));
      height =
        RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (private->size_se),
                                          1));
    }

  offset_x =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (private->offset_se),
                                      0));
  offset_y =
    RINT (ligma_size_entry_get_refval (LIGMA_SIZE_ENTRY (private->offset_se),
                                      1));

  private->callback (dialog,
                     image,
                     LIGMA_LAYER (item),
                     context,
                     item_name,
                     private->mode,
                     private->blend_space,
                     private->composite_space,
                     private->composite_mode,
                     private->opacity / 100.0,
                     private->fill_type,
                     width,
                     height,
                     offset_x,
                     offset_y,
                     item_visible,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     private->lock_alpha,
                     private->rename_text_layers,
                     private->user_data);
}

static void
layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private)
{
  gboolean mutable;

  mutable = ligma_layer_mode_is_blend_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->blend_space_combo, mutable);

  mutable = ligma_layer_mode_is_composite_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_space_combo, mutable);

  mutable = ligma_layer_mode_is_composite_mode_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_mode_combo, mutable);
}

static void
layer_options_dialog_mode_notify (GtkWidget          *widget,
                                  const GParamSpec   *pspec,
                                  LayerOptionsDialog *private)
{
  private->mode = ligma_layer_mode_box_get_mode (LIGMA_LAYER_MODE_BOX (widget));

  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->blend_space_combo),
                                 LIGMA_LAYER_COLOR_SPACE_AUTO);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->composite_space_combo),
                                 LIGMA_LAYER_COLOR_SPACE_AUTO);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (private->composite_mode_combo),
                                 LIGMA_LAYER_COMPOSITE_AUTO);

  layer_options_dialog_update_mode_sensitivity (private);
}

static void
layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                     LayerOptionsDialog *private)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      ligma_item_is_text_layer (LIGMA_ITEM (private->layer)))
    {
      LigmaTextLayer *text_layer = LIGMA_TEXT_LAYER (private->layer);
      LigmaText      *text       = ligma_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          GtkWidget *dialog;
          GtkWidget *name_entry;
          gchar     *name = ligma_utf8_strtrim (text->text, 30);

          dialog = gtk_widget_get_toplevel (widget);

          name_entry = item_options_dialog_get_name_entry (dialog);

          gtk_entry_set_text (GTK_ENTRY (name_entry), name);

          g_free (name);
        }
    }
}
