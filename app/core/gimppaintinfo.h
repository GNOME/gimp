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

#pragma once

#include "gimpviewable.h"


#define GIMP_TYPE_PAINT_INFO            (gimp_paint_info_get_type ())
#define GIMP_PAINT_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PAINT_INFO, GimpPaintInfo))
#define GIMP_PAINT_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PAINT_INFO, GimpPaintInfoClass))
#define GIMP_IS_PAINT_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PAINT_INFO))
#define GIMP_IS_PAINT_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PAINT_INFO))
#define GIMP_PAINT_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PAINT_INFO, GimpPaintInfoClass))


typedef struct _GimpPaintInfoClass GimpPaintInfoClass;

struct _GimpPaintInfo
{
  GimpViewable      parent_instance;

  Gimp             *gimp;

  GType             paint_type;
  GType             paint_options_type;

  gchar            *blurb;

  GimpPaintOptions *paint_options;
};

struct _GimpPaintInfoClass
{
  GimpViewableClass  parent_class;
};


GType           gimp_paint_info_get_type     (void) G_GNUC_CONST;

GimpPaintInfo * gimp_paint_info_new          (Gimp          *gimp,
                                              GType          paint_type,
                                              GType          paint_options_type,
                                              const gchar   *identifier,
                                              const gchar   *blurb,
                                              const gchar   *icon_name);

void            gimp_paint_info_set_standard (Gimp          *gimp,
                                              GimpPaintInfo *paint_info);
GimpPaintInfo * gimp_paint_info_get_standard (Gimp          *gimp);
