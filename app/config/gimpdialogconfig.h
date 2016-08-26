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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_DIALOG_CONFIG_H__
#define __GIMP_DIALOG_CONFIG_H__

#include "config/gimpguiconfig.h"


#define GIMP_TYPE_DIALOG_CONFIG            (gimp_dialog_config_get_type ())
#define GIMP_DIALOG_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DIALOG_CONFIG, GimpDialogConfig))
#define GIMP_DIALOG_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DIALOG_CONFIG, GimpDialogConfigClass))
#define GIMP_IS_DIALOG_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DIALOG_CONFIG))
#define GIMP_IS_DIALOG_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DIALOG_CONFIG))


typedef struct _GimpDialogConfigClass GimpDialogConfigClass;

struct _GimpDialogConfig
{
  GimpGuiConfig           parent_instance;

  GimpColorProfilePolicy  color_profile_policy;

  gchar                  *layer_new_name;
  GimpFillType            layer_new_fill_type;

  GimpAddMaskType         layer_add_mask_type;
  gboolean                layer_add_mask_invert;

  gchar                  *channel_new_name;
  GimpRGB                 channel_new_color;

  gchar                  *vectors_new_name;

  gdouble                 selection_feather_radius;

  gdouble                 selection_grow_radius;

  gdouble                 selection_shrink_radius;
  gboolean                selection_shrink_edge_lock;

  gdouble                 selection_border_radius;
  gboolean                selection_border_edge_lock;
  GimpChannelBorderStyle  selection_border_style;
};

struct _GimpDialogConfigClass
{
  GimpGuiConfigClass  parent_class;
};


GType  gimp_dialog_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_DIALOG_CONFIG_H__ */
