/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolcompass.h
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
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

#include "gimptoolwidget.h"


#define GIMP_TYPE_TOOL_COMPASS            (gimp_tool_compass_get_type ())
#define GIMP_TOOL_COMPASS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_COMPASS, GimpToolCompass))
#define GIMP_TOOL_COMPASS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_COMPASS, GimpToolCompassClass))
#define GIMP_IS_TOOL_COMPASS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_COMPASS))
#define GIMP_IS_TOOL_COMPASS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_COMPASS))
#define GIMP_TOOL_COMPASS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_COMPASS, GimpToolCompassClass))


typedef struct _GimpToolCompass        GimpToolCompass;
typedef struct _GimpToolCompassPrivate GimpToolCompassPrivate;
typedef struct _GimpToolCompassClass   GimpToolCompassClass;

struct _GimpToolCompass
{
  GimpToolWidget          parent_instance;

  GimpToolCompassPrivate *private;
};

struct _GimpToolCompassClass
{
  GimpToolWidgetClass  parent_class;

  void (* create_guides) (GimpToolCompass *compass,
                          gint             x,
                          gint             y,
                          gboolean         horizontal,
                          gboolean         vertical);
};


GType            gimp_tool_compass_get_type (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_compass_new      (GimpDisplayShell       *shell,
                                             GimpCompassOrientation  orinetation,
                                             gint                    n_points,
                                             gint                    x1,
                                             gint                    y1,
                                             gint                    x2,
                                             gint                    y2,
                                             gint                    y3,
                                             gint                    x3);
