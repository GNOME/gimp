/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpBaseConfig class
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_BASE_CONFIG_H__
#define __GIMP_BASE_CONFIG_H__

#include "base/base-enums.h"


#define GIMP_TYPE_BASE_CONFIG            (gimp_base_config_get_type ())
#define GIMP_BASE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_BASE_CONFIG, GimpBaseConfig))
#define GIMP_BASE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_BASE_CONFIG, GimpBaseConfigClass))
#define GIMP_IS_BASE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_BASE_CONFIG))
#define GIMP_IS_BASE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_BASE_CONFIG))


typedef struct _GimpBaseConfigClass GimpBaseConfigClass;

struct _GimpBaseConfig
{
  GObject   parent_instance;

  gchar    *temp_path;
  gchar    *swap_path;
  guint     num_processors;
  guint64   tile_cache_size;
};

struct _GimpBaseConfigClass
{
  GObjectClass           parent_class;
};


GType  gimp_base_config_get_type (void) G_GNUC_CONST;


#endif /* GIMP_BASE_CONFIG_H__ */
