/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprowdrawablefilter.c
 * Copyright (C) 2025 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpimage.h"

#include "gimprowdrawablefilter.h"


struct _GimpRowDrawableFilter
{
  GimpRowFilter  parent_instance;
};


static void   gimp_row_filter_drawable_active_toggled (GimpRowFilter *row,
                                                       gboolean       active);


G_DEFINE_TYPE (GimpRowDrawableFilter,
               gimp_row_drawable_filter,
               GIMP_TYPE_ROW_FILTER)

#define parent_class gimp_row_drawable_filter_parent_class


static void
gimp_row_drawable_filter_class_init (GimpRowDrawableFilterClass *klass)
{
  GimpRowFilterClass *row_filter_class = GIMP_ROW_FILTER_CLASS (klass);

  row_filter_class->active_toggled = gimp_row_filter_drawable_active_toggled;
}

static void
gimp_row_drawable_filter_init (GimpRowDrawableFilter *row)
{
}

static void
gimp_row_filter_drawable_active_toggled (GimpRowFilter *row,
                                         gboolean       active)
{
  GimpViewable *viewable = gimp_row_get_viewable (GIMP_ROW (row));

  GIMP_ROW_FILTER_CLASS (parent_class)->active_toggled (row, active);

  if (viewable)
    {
      GimpDrawable *drawable =
        gimp_drawable_filter_get_drawable (GIMP_DRAWABLE_FILTER (viewable));

      gimp_drawable_update (drawable, 0, 0, -1, -1);
      gimp_image_flush (gimp_item_get_image (GIMP_ITEM (drawable)));
    }
}
