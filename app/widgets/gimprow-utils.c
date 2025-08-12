/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimprow-utils.c
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

#include "core/gimpcontext.h"
#include "core/gimpdrawablefilter.h"
#include "core/gimpsettings.h"

#include "gimpcontainerview.h"
#include "gimpdeviceinfo.h"
#include "gimprow.h"
#include "gimprow-utils.h"
#include "gimprowdeviceinfo.h"
#include "gimprowdrawablefilter.h"
#include "gimprowseparator.h"
#include "gimprowsettings.h"


GType
gimp_row_type_from_viewable (GimpViewable *viewable)
{
  GType row_type = GIMP_TYPE_ROW;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (viewable), G_TYPE_NONE);

  if (GIMP_IS_DRAWABLE_FILTER (viewable))
    {
      row_type = GIMP_TYPE_ROW_DRAWABLE_FILTER;
    }
  else if (GIMP_IS_FILTER (viewable))
    {
      row_type = GIMP_TYPE_ROW_FILTER;
    }
  else if (GIMP_IS_SETTINGS (viewable))
    {
      if (gimp_object_get_name (viewable))
        row_type = GIMP_TYPE_ROW_SETTINGS;
      else
        row_type = GIMP_TYPE_ROW_SEPARATOR;
    }
  else if (GIMP_IS_DEVICE_INFO (viewable))
    {
      row_type = GIMP_TYPE_ROW_DEVICE_INFO;
    }

#if 0
  g_printerr ("viewable/row type: %s/%s\n",
              g_type_name (G_OBJECT_TYPE (viewable)),
              g_type_name (row_type));
#endif

  return row_type;
}

GtkWidget *
gimp_row_create_for_context (gpointer item,
                             gpointer context)
{
  g_return_val_if_fail (GIMP_IS_VIEWABLE (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  return gimp_row_new (context, item, GIMP_VIEW_SIZE_MEDIUM, 1);
}

GtkWidget *
gimp_row_create_for_container_view (gpointer item,
                                    gpointer container_view)
{
  GimpContext *context;
  gint         view_size;
  gint         view_border_width;

  g_return_val_if_fail (GIMP_IS_VIEWABLE (item), NULL);
  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (container_view), NULL);

  context   = gimp_container_view_get_context (container_view);
  view_size = gimp_container_view_get_view_size (container_view,
                                                 &view_border_width);

  return gimp_row_new (context, item, view_size, view_border_width);
}
