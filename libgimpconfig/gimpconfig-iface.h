/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_CONFIG_H__
#define __GIMP_CONFIG_H__

#include "core/core-types.h"
#include "core/gimpobject.h"


#define GIMP_TYPE_CONFIG            (gimp_config_get_type ())
#define GIMP_CONFIG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CONFIG, GimpConfig))
#define GIMP_CONFIG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CONFIG, GimpConfigClass))
#define GIMP_IS_CONFIG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CONFIG))
#define GIMP_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CONFIG))
#define GIMP_CONFIG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CONFIG, GimpConfigClass))


typedef struct _GimpConfig      GimpConfig;
typedef struct _GimpConfigClass GimpConfigClass;

struct _GimpConfig
{
  GimpObject       parent_instance;

  guint            marching_ants_speed;
  GimpPreviewSize  preview_size;
};

struct _GimpConfigClass
{
  GimpObjectClass  parent_class;
};


GType     gimp_config_get_type     (void) G_GNUC_CONST;

void      gimp_config_serialize    (GimpConfig *config);
gboolean  gimp_config_deserialize  (GimpConfig *config);


#endif  /* __GIMP_CONFIG_H__ */
