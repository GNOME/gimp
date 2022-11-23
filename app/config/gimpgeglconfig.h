/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGeglConfig class
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_GEGL_CONFIG_H__
#define __LIGMA_GEGL_CONFIG_H__


#define LIGMA_TYPE_GEGL_CONFIG            (ligma_gegl_config_get_type ())
#define LIGMA_GEGL_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_GEGL_CONFIG, LigmaGeglConfig))
#define LIGMA_GEGL_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_GEGL_CONFIG, LigmaGeglConfigClass))
#define LIGMA_IS_GEGL_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_GEGL_CONFIG))
#define LIGMA_IS_GEGL_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_GEGL_CONFIG))


typedef struct _LigmaGeglConfigClass LigmaGeglConfigClass;

struct _LigmaGeglConfig
{
  GObject   parent_instance;

  gchar    *temp_path;
  gchar    *swap_path;
  gchar    *swap_compression;
  gint      num_processors;
  guint64   tile_cache_size;
  gboolean  use_opencl;
};

struct _LigmaGeglConfigClass
{
  GObjectClass  parent_class;
};


GType  ligma_gegl_config_get_type (void) G_GNUC_CONST;


#endif /* LIGMA_GEGL_CONFIG_H__ */
