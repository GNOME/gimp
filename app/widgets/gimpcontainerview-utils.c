/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "gimpcontainereditor.h"
#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdockable.h"


/*  public functions  */

GimpContainerView *
gimp_container_view_get_by_dockable (GimpDockable *dockable)
{
  GtkBin *bin;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  bin = GTK_BIN (dockable);

  if (bin->child)
    {
      if (GIMP_IS_CONTAINER_EDITOR (bin->child))
        {
          return GIMP_CONTAINER_EDITOR (bin->child)->view;
        }
      else if (GIMP_IS_CONTAINER_VIEW (bin->child))
        {
          return GIMP_CONTAINER_VIEW (bin->child);
        }
    }

  return NULL;
}
