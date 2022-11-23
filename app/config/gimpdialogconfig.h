/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaDialogConfig class
 * Copyright (C) 2016  Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DIALOG_CONFIG_H__
#define __LIGMA_DIALOG_CONFIG_H__

#include "config/ligmaguiconfig.h"


/* We don't want to include stuff from core/ here, instead do the next
 * less ugly hack...
 */
typedef struct _LigmaFillOptions   LigmaFillOptions;
typedef struct _LigmaStrokeOptions LigmaStrokeOptions;


#define LIGMA_TYPE_DIALOG_CONFIG            (ligma_dialog_config_get_type ())
#define LIGMA_DIALOG_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DIALOG_CONFIG, LigmaDialogConfig))
#define LIGMA_DIALOG_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DIALOG_CONFIG, LigmaDialogConfigClass))
#define LIGMA_IS_DIALOG_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DIALOG_CONFIG))
#define LIGMA_IS_DIALOG_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DIALOG_CONFIG))


typedef struct _LigmaDialogConfigClass LigmaDialogConfigClass;

struct _LigmaDialogConfig
{
  LigmaGuiConfig             parent_instance;

  LigmaColorProfilePolicy     color_profile_policy;
  LigmaMetadataRotationPolicy metadata_rotation_policy;

  gchar                    *color_profile_path;

  LigmaColorRenderingIntent  image_convert_profile_intent;
  gboolean                  image_convert_profile_bpc;

  GeglDitherMethod          image_convert_precision_layer_dither_method;
  GeglDitherMethod          image_convert_precision_text_layer_dither_method;
  GeglDitherMethod          image_convert_precision_channel_dither_method;

  LigmaConvertPaletteType    image_convert_indexed_palette_type;
  gint                      image_convert_indexed_max_colors;
  gboolean                  image_convert_indexed_remove_duplicates;
  LigmaConvertDitherType     image_convert_indexed_dither_type;
  gboolean                  image_convert_indexed_dither_alpha;
  gboolean                  image_convert_indexed_dither_text_layers;

  LigmaFillType              image_resize_fill_type;
  LigmaItemSet               image_resize_layer_set;
  gboolean                  image_resize_resize_text_layers;

  gchar                    *layer_new_name;
  LigmaLayerMode             layer_new_mode;
  LigmaLayerColorSpace       layer_new_blend_space;
  LigmaLayerColorSpace       layer_new_composite_space;
  LigmaLayerCompositeMode    layer_new_composite_mode;
  gdouble                   layer_new_opacity;
  LigmaFillType              layer_new_fill_type;

  LigmaFillType              layer_resize_fill_type;

  LigmaAddMaskType           layer_add_mask_type;
  gboolean                  layer_add_mask_invert;

  LigmaMergeType             layer_merge_type;
  gboolean                  layer_merge_active_group_only;
  gboolean                  layer_merge_discard_invisible;

  gchar                    *channel_new_name;
  LigmaRGB                   channel_new_color;

  gchar                    *vectors_new_name;

  gchar                    *vectors_export_path;
  gboolean                  vectors_export_active_only;

  gchar                    *vectors_import_path;
  gboolean                  vectors_import_merge;
  gboolean                  vectors_import_scale;

  gdouble                   selection_feather_radius;
  gboolean                  selection_feather_edge_lock;

  gdouble                   selection_grow_radius;

  gdouble                   selection_shrink_radius;
  gboolean                  selection_shrink_edge_lock;

  gdouble                   selection_border_radius;
  gboolean                  selection_border_edge_lock;
  LigmaChannelBorderStyle    selection_border_style;

  LigmaFillOptions          *fill_options;
  LigmaStrokeOptions        *stroke_options;
};

struct _LigmaDialogConfigClass
{
  LigmaGuiConfigClass  parent_class;
};


GType  ligma_dialog_config_get_type (void) G_GNUC_CONST;


#endif /* LIGMA_DIALOG_CONFIG_H__ */
