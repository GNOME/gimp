/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGeglConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#pragma once


#define GIMP_TYPE_GEGL_CONFIG            (gimp_gegl_config_get_type ())
#define GIMP_GEGL_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_GEGL_CONFIG, GimpGeglConfig))
#define GIMP_GEGL_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_GEGL_CONFIG, GimpGeglConfigClass))
#define GIMP_IS_GEGL_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_GEGL_CONFIG))
#define GIMP_IS_GEGL_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_GEGL_CONFIG))


typedef struct _GimpGeglConfigClass GimpGeglConfigClass;

struct _GimpGeglConfig
{
  GObject   parent_instance;

  gchar    *temp_path;
  gchar    *swap_path;
  gchar    *swap_compression;
  gint      num_processors;
  guint64   tile_cache_size;
  gboolean  use_opencl;
};

struct _GimpGeglConfigClass
{
  GObjectClass  parent_class;
};


GType  gimp_gegl_config_get_type (void) G_GNUC_CONST;
