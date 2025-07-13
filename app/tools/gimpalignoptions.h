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

#include "core/gimptooloptions.h"


#define GIMP_TYPE_ALIGN_OPTIONS            (gimp_align_options_get_type ())
#define GIMP_ALIGN_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ALIGN_OPTIONS, GimpAlignOptions))
#define GIMP_ALIGN_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ALIGN_OPTIONS, GimpAlignOptionsClass))
#define GIMP_IS_ALIGN_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ALIGN_OPTIONS))
#define GIMP_IS_ALIGN_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ALIGN_OPTIONS))
#define GIMP_ALIGN_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ALIGN_OPTIONS, GimpAlignOptionsClass))


typedef struct _GimpAlignOptions        GimpAlignOptions;
typedef struct _GimpAlignOptionsPrivate GimpAlignOptionsPrivate;
typedef struct _GimpAlignOptionsClass   GimpAlignOptionsClass;

struct _GimpAlignOptions
{
  GimpToolOptions          parent_instance;

  GimpAlignReferenceType   align_reference;

  GimpAlignOptionsPrivate *priv;
};

struct _GimpAlignOptionsClass
{
  GimpToolOptionsClass  parent_class;

  void (* align_button_clicked) (GimpAlignOptions  *options,
                                 GimpAlignmentType  align_type);
};


GType       gimp_align_options_get_type          (void) G_GNUC_CONST;

GtkWidget * gimp_align_options_gui               (GimpToolOptions  *tool_options);

GList     * gimp_align_options_get_objects       (GimpAlignOptions *options);
void        gimp_align_options_get_pivot         (GimpAlignOptions *options,
                                                  gdouble          *x,
                                                  gdouble          *y);

void        gimp_align_options_pick_reference    (GimpAlignOptions *options,
                                                  GObject          *object);
GObject   * gimp_align_options_get_reference     (GimpAlignOptions *options,
                                                  gboolean          blink_if_none);
gboolean    gimp_align_options_align_contents    (GimpAlignOptions *options);

void        gimp_align_options_pick_guide        (GimpAlignOptions *options,
                                                  GimpGuide        *guide,
                                                  gboolean          extend);
