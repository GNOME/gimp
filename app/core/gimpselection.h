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

#include "gimpchannel.h"


#define GIMP_TYPE_SELECTION            (gimp_selection_get_type ())
#define GIMP_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SELECTION, GimpSelection))
#define GIMP_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SELECTION, GimpSelectionClass))
#define GIMP_IS_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SELECTION))
#define GIMP_IS_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SELECTION))
#define GIMP_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SELECTION, GimpSelectionClass))


typedef struct _GimpSelectionClass GimpSelectionClass;

struct _GimpSelection
{
  GimpChannel parent_instance;

  gint        suspend_count;
};

struct _GimpSelectionClass
{
  GimpChannelClass parent_class;
};


GType         gimp_selection_get_type (void) G_GNUC_CONST;

GimpChannel * gimp_selection_new      (GimpImage     *image,
                                       gint           width,
                                       gint           height);

gint          gimp_selection_suspend  (GimpSelection *selection);
gint          gimp_selection_resume   (GimpSelection *selection);

GeglBuffer  * gimp_selection_extract  (GimpSelection *selection,
                                       GList         *pickables,
                                       GimpContext   *context,
                                       gboolean       cut_image,
                                       gboolean       keep_indexed,
                                       gboolean       add_alpha,
                                       gint          *offset_x,
                                       gint          *offset_y,
                                       GError       **error);

GimpLayer   * gimp_selection_float    (GimpSelection *selection,
                                       GList         *drawables,
                                       GimpContext   *context,
                                       gboolean       cut_image,
                                       gint           off_x,
                                       gint           off_y,
                                       GError       **error);
