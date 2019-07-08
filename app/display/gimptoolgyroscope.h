/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolgyroscope.h
 * Copyright (C) 2018 Ell
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

#ifndef __GIMP_TOOL_GYROSCOPE_H__
#define __GIMP_TOOL_GYROSCOPE_H__


#include "gimptoolwidget.h"


#define GIMP_TYPE_TOOL_GYROSCOPE            (gimp_tool_gyroscope_get_type ())
#define GIMP_TOOL_GYROSCOPE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_GYROSCOPE, GimpToolGyroscope))
#define GIMP_TOOL_GYROSCOPE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_GYROSCOPE, GimpToolGyroscopeClass))
#define GIMP_IS_TOOL_GYROSCOPE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_GYROSCOPE))
#define GIMP_IS_TOOL_GYROSCOPE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_GYROSCOPE))
#define GIMP_TOOL_GYROSCOPE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_GYROSCOPE, GimpToolGyroscopeClass))


typedef struct _GimpToolGyroscope        GimpToolGyroscope;
typedef struct _GimpToolGyroscopePrivate GimpToolGyroscopePrivate;
typedef struct _GimpToolGyroscopeClass   GimpToolGyroscopeClass;

struct _GimpToolGyroscope
{
  GimpToolWidget            parent_instance;

  GimpToolGyroscopePrivate *private;
};

struct _GimpToolGyroscopeClass
{
  GimpToolWidgetClass  parent_class;
};


GType            gimp_tool_gyroscope_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_gyroscope_new      (GimpDisplayShell *shell);


#endif /* __GIMP_TOOL_GYROSCOPE_H__ */
