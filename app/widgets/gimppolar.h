/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppolar.h
 * Copyright (C) 2014 Michael Natterer <mitch@gimp.org>
 *
 * Based on code from the color-rotate plug-in
 * Copyright (C) 1997-1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                         Based on code from Pavel Grinfeld (pavel@ml.com)
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

#ifndef __GIMP_POLAR_H__
#define __GIMP_POLAR_H__


#include "gimpcircle.h"


#define GIMP_TYPE_POLAR            (gimp_polar_get_type ())
#define GIMP_POLAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POLAR, GimpPolar))
#define GIMP_POLAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POLAR, GimpPolarClass))
#define GIMP_IS_POLAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_POLAR))
#define GIMP_IS_POLAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POLAR))
#define GIMP_POLAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POLAR, GimpPolarClass))


typedef struct _GimpPolarPrivate GimpPolarPrivate;
typedef struct _GimpPolarClass   GimpPolarClass;

struct _GimpPolar
{
  GimpCircle       parent_instance;

  GimpPolarPrivate *priv;
};

struct _GimpPolarClass
{
  GimpCircleClass  parent_class;
};


GType       gimp_polar_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_polar_new      (void);


#endif /* __GIMP_POLAR_H__ */
