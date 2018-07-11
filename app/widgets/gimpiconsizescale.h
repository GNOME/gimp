/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpiconsizescale.h
 * Copyright (C) 2016 Jehan <jehan@girinstud.io>
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

#ifndef __GIMP_ICON_SIZE_SCALE_H__
#define __GIMP_ICON_SIZE_SCALE_H__


#define GIMP_TYPE_ICON_SIZE_SCALE            (gimp_icon_size_scale_get_type ())
#define GIMP_ICON_SIZE_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ICON_SIZE_SCALE, GimpIconSizeScale))
#define GIMP_ICON_SIZE_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ICON_SIZE_SCALE, GimpIconSizeScaleClass))
#define GIMP_IS_ICON_SIZE_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ICON_SIZE_SCALE))
#define GIMP_IS_ICON_SIZE_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((obj), GIMP_TYPE_ICON_SIZE_SCALE))
#define GIMP_ICON_SIZE_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), GIMP_TYPE_ICON_SIZE_SCALE, GimpIconSizeScaleClass))


typedef struct _GimpIconSizeScaleClass GimpIconSizeScaleClass;

struct _GimpIconSizeScale
{
  GimpFrame      parent_instance;
};

struct _GimpIconSizeScaleClass
{
  GimpFrameClass parent_class;
};


GType       gimp_icon_size_scale_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_icon_size_scale_new      (Gimp *gimp);


#endif  /* __GIMP_ICON_SIZE_SCALE_H__ */
