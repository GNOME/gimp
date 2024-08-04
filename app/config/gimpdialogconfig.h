/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpDialogConfig class
 * Copyright (C) 2016  Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_DIALOG_CONFIG_H__
#define __GIMP_DIALOG_CONFIG_H__

#include "config/gimpguiconfig.h"


/* We don't want to include stuff from core/ here, instead do the next
 * less ugly hack...
 */
typedef struct _GimpFillOptions   GimpFillOptions;
typedef struct _GimpStrokeOptions GimpStrokeOptions;


#define GIMP_TYPE_DIALOG_CONFIG            (gimp_dialog_config_get_type ())
#define GIMP_DIALOG_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DIALOG_CONFIG, GimpDialogConfig))
#define GIMP_DIALOG_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_CONFIG, GimpDialogConfigClass))
#define GIMP_IS_DIALOG_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DIALOG_CONFIG))
#define GIMP_IS_DIALOG_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_CONFIG))


typedef struct _GimpDialogConfigClass GimpDialogConfigClass;

struct _GimpDialogConfig
{
  GimpGuiConfig             parent_instance;

  GimpColorProfilePolicy     color_profile_policy;
  GimpMetadataRotationPolicy metadata_rotation_policy;

  gchar                    *color_profile_path;

  GimpColorRenderingIntent  image_convert_profile_intent;
  gboolean                  image_convert_profile_bpc;

  GeglDitherMethod          image_convert_precision_layer_dither_method;
  GeglDitherMethod          image_convert_precision_text_layer_dither_method;
  GeglDitherMethod          image_convert_precision_channel_dither_method;

  GimpConvertPaletteType    image_convert_indexed_palette_type;
  gint                      image_convert_indexed_max_colors;
  gboolean                  image_convert_indexed_remove_duplicates;
  GimpConvertDitherType     image_convert_indexed_dither_type;
  gboolean                  image_convert_indexed_dither_alpha;
  gboolean                  image_convert_indexed_dither_text_layers;

  GimpFillType              image_resize_fill_type;
  GimpItemSet               image_resize_layer_set;
  gboolean                  image_resize_resize_text_layers;

  gchar                    *layer_new_name;
  GimpLayerMode             layer_new_mode;
  GimpLayerColorSpace       layer_new_blend_space;
  GimpLayerColorSpace       layer_new_composite_space;
  GimpLayerCompositeMode    layer_new_composite_mode;
  gdouble                   layer_new_opacity;
  GimpFillType              layer_new_fill_type;

  GimpFillType              layer_resize_fill_type;

  GimpAddMaskType           layer_add_mask_type;
  gboolean                  layer_add_mask_invert;

  GimpMergeType             layer_merge_type;
  gboolean                  layer_merge_active_group_only;
  gboolean                  layer_merge_discard_invisible;

  gchar                    *channel_new_name;
  GeglColor                *channel_new_color;

  gchar                    *path_new_name;

  gchar                    *path_export_path;
  gboolean                  path_export_active_only;

  gchar                    *path_import_path;
  gboolean                  path_import_merge;
  gboolean                  path_import_scale;

  gdouble                   selection_feather_radius;
  gboolean                  selection_feather_edge_lock;

  gdouble                   selection_grow_radius;

  gdouble                   selection_shrink_radius;
  gboolean                  selection_shrink_edge_lock;

  gdouble                   selection_border_radius;
  gboolean                  selection_border_edge_lock;
  GimpChannelBorderStyle    selection_border_style;

  GimpFillOptions          *fill_options;
  GimpStrokeOptions        *stroke_options;
};

struct _GimpDialogConfigClass
{
  GimpGuiConfigClass  parent_class;
};


GType  gimp_dialog_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_DIALOG_CONFIG_H__ */
