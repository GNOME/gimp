/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatoolcompass.h
 * Copyright (C) 2017 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_TOOL_COMPASS_H__
#define __LIGMA_TOOL_COMPASS_H__


#include "ligmatoolwidget.h"


#define LIGMA_TYPE_TOOL_COMPASS            (ligma_tool_compass_get_type ())
#define LIGMA_TOOL_COMPASS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TOOL_COMPASS, LigmaToolCompass))
#define LIGMA_TOOL_COMPASS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TOOL_COMPASS, LigmaToolCompassClass))
#define LIGMA_IS_TOOL_COMPASS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TOOL_COMPASS))
#define LIGMA_IS_TOOL_COMPASS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TOOL_COMPASS))
#define LIGMA_TOOL_COMPASS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TOOL_COMPASS, LigmaToolCompassClass))


typedef struct _LigmaToolCompass        LigmaToolCompass;
typedef struct _LigmaToolCompassPrivate LigmaToolCompassPrivate;
typedef struct _LigmaToolCompassClass   LigmaToolCompassClass;

struct _LigmaToolCompass
{
  LigmaToolWidget          parent_instance;

  LigmaToolCompassPrivate *private;
};

struct _LigmaToolCompassClass
{
  LigmaToolWidgetClass  parent_class;

  void (* create_guides) (LigmaToolCompass *compass,
                          gint             x,
                          gint             y,
                          gboolean         horizontal,
                          gboolean         vertical);
};


GType            ligma_tool_compass_get_type (void) G_GNUC_CONST;

LigmaToolWidget * ligma_tool_compass_new      (LigmaDisplayShell       *shell,
                                             LigmaCompassOrientation  orinetation,
                                             gint                    n_points,
                                             gint                    x1,
                                             gint                    y1,
                                             gint                    x2,
                                             gint                    y2,
                                             gint                    y3,
                                             gint                    x3);


#endif /* __LIGMA_TOOL_COMPASS_H__ */
