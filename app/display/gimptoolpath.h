/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolpath.h
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

#ifndef __GIMP_TOOL_PATH_H__
#define __GIMP_TOOL_PATH_H__


#include "gimptoolwidget.h"


#define GIMP_TYPE_TOOL_PATH            (gimp_tool_path_get_type ())
#define GIMP_TOOL_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_PATH, GimpToolPath))
#define GIMP_TOOL_PATH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_PATH, GimpToolPathClass))
#define GIMP_IS_TOOL_PATH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_PATH))
#define GIMP_IS_TOOL_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_PATH))
#define GIMP_TOOL_PATH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_PATH, GimpToolPathClass))


typedef struct _GimpToolPath        GimpToolPath;
typedef struct _GimpToolPathPrivate GimpToolPathPrivate;
typedef struct _GimpToolPathClass   GimpToolPathClass;

struct _GimpToolPath
{
  GimpToolWidget       parent_instance;

  GimpToolPathPrivate *private;
};

struct _GimpToolPathClass
{
  GimpToolWidgetClass  parent_class;

  void (* begin_change) (GimpToolPath    *path,
                         const gchar     *desc);
  void (* end_change)   (GimpToolPath    *path,
                         gboolean         success);
  void (* activate)     (GimpToolPath    *path,
                         GdkModifierType  state);
};


GType            gimp_tool_path_get_type    (void) G_GNUC_CONST;

GimpToolWidget * gimp_tool_path_new         (GimpDisplayShell *shell);

void             gimp_tool_path_set_vectors (GimpToolPath     *path,
                                             GimpPath         *vectors);

void             gimp_tool_path_get_popup_state (GimpToolPath *path,
                                                 gboolean     *on_handle,
                                                 gboolean     *on_curve);

void             gimp_tool_path_delete_anchor  (GimpToolPath *path);
void             gimp_tool_path_shift_start    (GimpToolPath *path);
void             gimp_tool_path_insert_anchor  (GimpToolPath *path);
void             gimp_tool_path_delete_segment (GimpToolPath *path);
void             gimp_tool_path_reverse_stroke (GimpToolPath *path);

#endif /* __GIMP_TOOL_PATH_H__ */
