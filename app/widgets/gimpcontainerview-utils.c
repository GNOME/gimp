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

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"

#include "gimpcontainereditor.h"
#include "gimpcontainerview.h"
#include "gimpcontainerview-utils.h"
#include "gimpdockable.h"


/*  public functions  */

GimpContainerView *
gimp_container_view_get_by_dockable (GimpDockable *dockable)
{
  GtkWidget *child;

  g_return_val_if_fail (GIMP_IS_DOCKABLE (dockable), NULL);

  child = gtk_bin_get_child (GTK_BIN (dockable));

  if (child)
    {
      if (GIMP_IS_CONTAINER_EDITOR (child))
        {
          return GIMP_CONTAINER_EDITOR (child)->view;
        }
      else if (GIMP_IS_CONTAINER_VIEW (child))
        {
          return GIMP_CONTAINER_VIEW (child);
        }
    }

  return NULL;
}

void
gimp_container_view_remove_active (GimpContainerView *view)
{
  GimpContext   *context;
  GimpContainer *container;

  g_return_if_fail (GIMP_IS_CONTAINER_VIEW (view));

  context   = gimp_container_view_get_context (view);
  container = gimp_container_view_get_container (view);

  if (context && container)
    {
      GType       children_type;
      GimpObject *active;

      children_type = gimp_container_get_children_type (container);

      active = gimp_context_get_by_type (context, children_type);

      if (active)
        {
          GimpObject *new;

          new = gimp_container_get_neighbor_of (container, active);

          if (new)
            gimp_context_set_by_type (context, children_type, new);

          gimp_container_remove (container, active);
        }
    }
}
