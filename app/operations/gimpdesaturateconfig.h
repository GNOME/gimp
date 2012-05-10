/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdesaturateconfig.h
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMP_DESATURATE_CONFIG_H__
#define __GIMP_DESATURATE_CONFIG_H__


#include "core/gimpimagemapconfig.h"


#define GIMP_TYPE_DESATURATE_CONFIG            (gimp_desaturate_config_get_type ())
#define GIMP_DESATURATE_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DESATURATE_CONFIG, GimpDesaturateConfig))
#define GIMP_DESATURATE_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GIMP_TYPE_DESATURATE_CONFIG, GimpDesaturateConfigClass))
#define GIMP_IS_DESATURATE_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DESATURATE_CONFIG))
#define GIMP_IS_DESATURATE_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GIMP_TYPE_DESATURATE_CONFIG))
#define GIMP_DESATURATE_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GIMP_TYPE_DESATURATE_CONFIG, GimpDesaturateConfigClass))


typedef struct _GimpDesaturateConfigClass GimpDesaturateConfigClass;

struct _GimpDesaturateConfig
{
  GimpImageMapConfig  parent_instance;

  GimpDesaturateMode  mode;
};

struct _GimpDesaturateConfigClass
{
  GimpImageMapConfigClass  parent_class;
};


GType   gimp_desaturate_config_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_DESATURATE_CONFIG_H__ */
