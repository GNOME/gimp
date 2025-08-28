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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "core/gimpcontext.h"
#include "core/gimpdrawable-filters.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplink.h"
#include "core/gimplinklayer.h"

#include "text/gimptext.h"
#include "text/gimptextlayer.h"

#include "widgets/gimpcontainerlistview.h"
#include "widgets/gimplayermodebox.h"
#include "widgets/gimpopendialog.h"
#include "widgets/gimpviewabledialog.h"

#include "item-options-dialog.h"
#include "layer-options-dialog.h"

#include "gimp-intl.h"


typedef struct _LayerOptionsDialog LayerOptionsDialog;

struct _LayerOptionsDialog
{
  Gimp                     *gimp;
  GimpLayer                *layer;
  GimpLayerMode             mode;
  GimpLayerColorSpace       blend_space;
  GimpLayerColorSpace       composite_space;
  GimpLayerCompositeMode    composite_mode;
  gdouble                   opacity;
  GimpFillType              fill_type;
  gboolean                  lock_alpha;
  gboolean                  rename_text_layers;
  GimpLayerOptionsCallback  callback;
  gpointer                  user_data;

  GtkWidget                *mode_box;
  GtkWidget                *blend_space_combo;
  GtkWidget                *composite_space_combo;
  GtkWidget                *composite_mode_combo;
  GtkWidget                *size_se;
  GtkWidget                *offset_se;

  GimpLink                 *link;
};


/*  local function prototypes  */

static void   layer_options_dialog_free           (LayerOptionsDialog *private);
static void   layer_options_dialog_callback       (GtkWidget          *dialog,
                                                   GimpImage          *image,
                                                   GimpItem           *item,
                                                   GimpContext        *context,
                                                   const gchar        *item_name,
                                                   gboolean            item_visible,
                                                   GimpColorTag        item_color_tag,
                                                   gboolean            item_lock_content,
                                                   gboolean            item_lock_position,
                                                   gboolean            item_lock_visibility,
                                                   gpointer            user_data);
static void
     layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private);
static void   layer_options_dialog_mode_notify    (GtkWidget          *widget,
                                                   const GParamSpec   *pspec,
                                                   LayerOptionsDialog *private);
static void   layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                                   LayerOptionsDialog *private);

static void   layer_options_file_set              (GtkFileChooserButton *widget,
                                                   LayerOptionsDialog   *private);


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
                          GimpLayerMode             layer_mode,
                          GimpLayerColorSpace       layer_blend_space,
                          GimpLayerColorSpace       layer_composite_space,
                          GimpLayerCompositeMode    layer_composite_mode,
                          gdouble                   layer_opacity,
                          GimpFillType              layer_fill_type,
                          gboolean                  layer_visible,
                          GimpColorTag              layer_color_tag,
                          gboolean                  layer_lock_content,
                          gboolean                  layer_lock_position,
                          gboolean                  layer_lock_visibility,
                          gboolean                  layer_lock_alpha,
                          GimpLayerOptionsCallback  callback,
                          gpointer                  user_data)
{
  LayerOptionsDialog   *private;
  GtkWidget            *dialog;
  GtkWidget            *grid;
  GtkListStore         *space_model;
  GtkWidget            *combo;
  GtkWidget            *file_select;
  GtkWidget            *scale;
  GtkWidget            *label;
  GtkAdjustment        *adjustment;
  GtkWidget            *spinbutton;
  GtkWidget            *button;
  GimpLayerModeContext  mode_context;
  gdouble               xres;
  gdouble               yres;
  gint                  row = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (layer == NULL || GIMP_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);

  private = g_slice_new0 (LayerOptionsDialog);

  private->gimp               = image->gimp;
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

  private->link               = NULL;

  if (layer && gimp_item_is_text_layer (GIMP_ITEM (layer)))
    private->rename_text_layers = GIMP_TEXT_LAYER (layer)->auto_rename;

  dialog = item_options_dialog_new (image, GIMP_ITEM (layer), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    _("Layer _name:"),
                                    GIMP_ICON_LOCK_CONTENT,
                                    _("Lock _pixels"),
                                    _("Lock position and _size"),
                                    _("Lock visibility"),
                                    layer_name,
                                    layer_visible,
                                    layer_color_tag,
                                    layer_lock_content,
                                    layer_lock_position,
                                    layer_lock_visibility,
                                    layer_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) layer_options_dialog_free, private);

  if (! layer || gimp_viewable_get_children (GIMP_VIEWABLE (layer)) == NULL)
    mode_context = GIMP_LAYER_MODE_CONTEXT_LAYER;
  else
    mode_context = GIMP_LAYER_MODE_CONTEXT_GROUP;

  private->mode_box = gimp_layer_mode_box_new (mode_context);
  item_options_dialog_add_widget (dialog, _("_Mode:"), private->mode_box);
  gimp_layer_mode_box_set_mode (GIMP_LAYER_MODE_BOX (private->mode_box),
                                private->mode);

  g_signal_connect (private->mode_box, "notify::layer-mode",
                    G_CALLBACK (layer_options_dialog_mode_notify),
                    private);

  space_model =
    gimp_enum_store_new_with_values (GIMP_TYPE_LAYER_COLOR_SPACE,
                                     4,
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     GIMP_LAYER_COLOR_SPACE_RGB_LINEAR,
                                     GIMP_LAYER_COLOR_SPACE_RGB_NON_LINEAR,
                                     GIMP_LAYER_COLOR_SPACE_RGB_PERCEPTUAL);

  private->blend_space_combo = combo =
    gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("_Blend space:"), combo);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-layer-color-space");
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->blend_space,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->blend_space, NULL);

  private->composite_space_combo = combo =
    gimp_enum_combo_box_new_with_model (GIMP_ENUM_STORE (space_model));
  item_options_dialog_add_widget (dialog, _("Compos_ite space:"), combo);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-layer-color-space");
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->composite_space,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->composite_space, NULL);

  g_object_unref (space_model);

  private->composite_mode_combo = combo =
    gimp_enum_combo_box_new (GIMP_TYPE_LAYER_COMPOSITE_MODE);
  item_options_dialog_add_widget (dialog, _("Composite mo_de:"), combo);
  gimp_enum_combo_box_set_icon_prefix (GIMP_ENUM_COMBO_BOX (combo),
                                       "gimp-layer-composite");
  gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                              private->composite_mode,
                              G_CALLBACK (gimp_int_combo_box_get_active),
                              &private->composite_mode, NULL);

  /*  set the sensitivity of above 3 menus  */
  layer_options_dialog_update_mode_sensitivity (private);

  adjustment = gtk_adjustment_new (private->opacity, 0.0, 100.0,
                                   1.0, 10.0, 0.0);
  scale = gimp_spin_scale_new (adjustment, NULL, 1);
  item_options_dialog_add_widget (dialog, _("_Opacity:"), scale);

  g_signal_connect (adjustment, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &private->opacity);

  grid = item_options_dialog_get_grid (dialog, &row);

  gimp_image_get_resolution (image, &xres, &yres);

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
      spinbutton = gimp_spin_button_new (adjustment, 1.0, 2);
      gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
      gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

      private->size_se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a",
                                              TRUE, TRUE, FALSE, 10,
                                              GIMP_SIZE_ENTRY_UPDATE_SIZE);

      gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->size_se),
                                 GTK_SPIN_BUTTON (spinbutton), NULL);
      gtk_grid_attach (GTK_GRID (private->size_se), spinbutton, 1, 0, 1, 1);
      gtk_widget_show (spinbutton);

      gtk_grid_attach (GTK_GRID (grid), private->size_se, 1, row, 1, 2);
      gtk_widget_show (private->size_se);

      gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->size_se),
                                gimp_unit_pixel ());

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
  gtk_grid_attach (GTK_GRID (grid), label, 0, row, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new (_("Offset Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, row + 1, 1, 1);
  gtk_widget_show (label);

  /*  The offset sizeentry  */
  adjustment = gtk_adjustment_new (0, 1, 1, 1, 10, 0);
  spinbutton = gimp_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);

  private->offset_se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a",
                                            TRUE, TRUE, FALSE, 10,
                                            GIMP_SIZE_ENTRY_UPDATE_SIZE);

  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (private->offset_se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (private->offset_se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gtk_grid_attach (GTK_GRID (grid), private->offset_se, 1, row, 1, 2);
  gtk_widget_show (private->offset_se);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (private->offset_se),
                            gimp_unit_pixel ());

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

  if (layer)
    {
      GtkWidget     *left_vbox = item_options_dialog_get_vbox (dialog);
      GtkWidget     *frame;
      GimpContainer *filters;
      GtkWidget     *view;

      frame = gimp_frame_new (_("Active Filters"));
      gtk_box_pack_start (GTK_BOX (left_vbox), frame, TRUE, TRUE, 0);
      gtk_widget_show (frame);

      filters = gimp_drawable_get_filters (GIMP_DRAWABLE (layer));

      view = gimp_container_list_view_new (filters, context,
                                           GIMP_VIEW_SIZE_SMALL, 0);
      gtk_container_add (GTK_CONTAINER (frame), view);
      gtk_widget_show (view);

      if (GIMP_IS_LINK_LAYER (layer))
        {
          GtkWidget *open_dialog;
          GimpLink  *link;

          /* File chooser dialog. */
          open_dialog = gimp_open_dialog_new (private->gimp);
          gtk_window_set_title (GTK_WINDOW (open_dialog),
                                _("Select Linked Image"));

          /* File chooser button. */
          file_select = gtk_file_chooser_button_new_with_dialog (open_dialog);
          link = gimp_link_layer_get_link (GIMP_LINK_LAYER (layer));
          gtk_file_chooser_set_file (GTK_FILE_CHOOSER (file_select),
                                     gimp_link_get_file (link, NULL, NULL),
                                     NULL);
          gtk_widget_show (file_select);
          gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    _("_Linked image:"), 0.0, 0.5,
                                    file_select, 1);

          g_signal_connect (file_select, "file-set",
                            G_CALLBACK (layer_options_file_set),
                            private);

          private->link = gimp_link_duplicate (link);

          /* Absolute path checkbox. */
          button = gtk_check_button_new_with_mnemonic (_("S_tore with absolute path"));
          gimp_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                                    NULL, 0.0, 0.5,
                                    button, 2);
          g_object_bind_property (G_OBJECT (private->link), "absolute-path",
                                  G_OBJECT (button),        "active",
                                  G_BINDING_SYNC_CREATE |
                                  G_BINDING_BIDIRECTIONAL);
          gtk_widget_set_visible (button, TRUE);
        }
    }
  else
    {
      /*  The fill type  */
      combo = gimp_enum_combo_box_new (GIMP_TYPE_FILL_TYPE);
      gimp_grid_attach_aligned (GTK_GRID (grid), 0, row,
                                _("_Fill with:"), 0.0, 0.5,
                                combo, 1);
      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  private->fill_type,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &private->fill_type, NULL);
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
                                           GIMP_ICON_LOCK_ALPHA,
                                           _("Lock _alpha"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->lock_alpha);
  g_signal_connect (button, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    &private->lock_alpha);

  /*  For text layers add a toggle to control "auto-rename"  */
  if (layer && gimp_item_is_text_layer (GIMP_ITEM (layer)))
    {
      button = item_options_dialog_add_switch (dialog,
                                               GIMP_ICON_TOOL_TEXT,
                                               _("Set name from _text"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                    private->rename_text_layers);
      g_signal_connect (button, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
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
                               GimpImage    *image,
                               GimpItem     *item,
                               GimpContext  *context,
                               const gchar  *item_name,
                               gboolean      item_visible,
                               GimpColorTag  item_color_tag,
                               gboolean      item_lock_content,
                               gboolean      item_lock_position,
                               gboolean      item_lock_visibility,
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

  private->callback (dialog,
                     image,
                     GIMP_LAYER (item),
                     context,
                     item_name,
                     private->mode,
                     private->blend_space,
                     private->composite_space,
                     private->composite_mode,
                     private->opacity / 100.0,
                     private->fill_type,
                     private->link,
                     width,
                     height,
                     offset_x,
                     offset_y,
                     item_visible,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     item_lock_visibility,
                     private->lock_alpha,
                     private->rename_text_layers,
                     private->user_data);
}

static void
layer_options_dialog_update_mode_sensitivity (LayerOptionsDialog *private)
{
  gboolean mutable;

  mutable = gimp_layer_mode_is_blend_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->blend_space_combo, mutable);

  mutable = gimp_layer_mode_is_composite_space_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_space_combo, mutable);

  mutable = gimp_layer_mode_is_composite_mode_mutable (private->mode);
  gtk_widget_set_sensitive (private->composite_mode_combo, mutable);
}

static void
layer_options_dialog_mode_notify (GtkWidget          *widget,
                                  const GParamSpec   *pspec,
                                  LayerOptionsDialog *private)
{
  private->mode = gimp_layer_mode_box_get_mode (GIMP_LAYER_MODE_BOX (widget));

  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->blend_space_combo),
                                 GIMP_LAYER_COLOR_SPACE_AUTO);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->composite_space_combo),
                                 GIMP_LAYER_COLOR_SPACE_AUTO);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (private->composite_mode_combo),
                                 GIMP_LAYER_COMPOSITE_AUTO);

  layer_options_dialog_update_mode_sensitivity (private);
}

static void
layer_options_dialog_rename_toggled (GtkWidget          *widget,
                                     LayerOptionsDialog *private)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)) &&
      gimp_item_is_text_layer (GIMP_ITEM (private->layer)))
    {
      GimpTextLayer *text_layer = GIMP_TEXT_LAYER (private->layer);
      GimpText      *text       = gimp_text_layer_get_text (text_layer);

      if (text && text->text)
        {
          GtkWidget *dialog;
          GtkWidget *name_entry;
          gchar     *name = gimp_utf8_strtrim (text->text, 30);

          dialog = gtk_widget_get_toplevel (widget);

          name_entry = item_options_dialog_get_name_entry (dialog);

          gtk_entry_set_text (GTK_ENTRY (name_entry), name);

          g_free (name);
        }
    }
}

static void
layer_options_file_set (GtkFileChooserButton *widget,
                        LayerOptionsDialog   *private)
{
  GFile *file;

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (widget));
  if (file)
    {
      gint width  = 0;
      gint height = 0;

      if (private->layer)
        {
          width  = gimp_item_get_width (GIMP_ITEM (private->layer));
          height = gimp_item_get_height (GIMP_ITEM (private->layer));

          if (width == 0 || height == 0)
            {
              GimpImage *image = gimp_item_get_image (GIMP_ITEM (private->layer));

              width  = gimp_image_get_width (image);
              height = gimp_image_get_height (image);
            }
        }

      gimp_link_set_file (private->link, file, width, height, TRUE, NULL, NULL);
      if (gimp_link_is_broken (private->link))
        {
          gimp_link_set_file (private->link, NULL, width, height, TRUE, NULL, NULL);
          g_signal_handlers_block_by_func (widget,
                                           G_CALLBACK (layer_options_file_set),
                                           private);
          gtk_file_chooser_unselect_file (GTK_FILE_CHOOSER (widget), file);
          g_signal_handlers_unblock_by_func (widget,
                                             G_CALLBACK (layer_options_file_set),
                                             private);
        }
    }
  g_clear_object (&file);
}
