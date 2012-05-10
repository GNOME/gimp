/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpposterizeconfig.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_POSTERIZE_CONFIG_H__
#define __GIMP_POSTERIZE_CONFIG_H__


#include "core/gimpimagemapconfig.h"


#define GIMP_TYPE_POSTERIZE_CONFIG            (gimp_posterize_config_get_type ())
#define GIMP_POSTERIZE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POSTERIZE_CONFIG, GimpPosterizeConfig))
#define GIMP_POSTERIZE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_POSTERIZE_CONFIG, GimpPosterizeConfigClass))
#define GIMP_IS_POSTERIZE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POSTERIZE_CONFIG))
#define GIMP_IS_POSTERIZE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_POSTERIZE_CONFIG))
#define GIMP_POSTERIZE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_POSTERIZE_CONFIG, GimpPosterizeConfigClass))


typedef struct _GimpPosterizeConfigClass GimpPosterizeConfigClass;

struct _GimpPosterizeConfig
{
  GimpImageMapConfig  parent_instance;

  gint                levels;
};

struct _GimpPosterizeConfigClass
{
  GimpImageMapConfigClass  parent_class;
};


GType   gimp_posterize_config_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_POSTERIZE_CONFIG_H__ */
