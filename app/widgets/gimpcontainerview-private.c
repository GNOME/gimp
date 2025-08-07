/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcontainerview-private.c
 * Copyright (C) 2001-2025 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"

#include "widgets-types.h"

#include "core/gimptreehandler.h"

#include "gimpcontainerview.h"
#include "gimpcontainerview-private.h"


static void
gimp_container_view_private_dispose (GimpContainerView        *view,
                                     GimpContainerViewPrivate *private)
{
  if (private->container)
    gimp_container_view_set_container (view, NULL);

  if (private->context)
    gimp_container_view_set_context (view, NULL);
}

static void
gimp_container_view_private_finalize (GimpContainerViewPrivate *private)
{
  g_clear_pointer (&private->item_hash,
                   g_hash_table_destroy);
  g_clear_pointer (&private->name_changed_handler,
                   gimp_tree_handler_disconnect);
  g_clear_pointer (&private->expanded_changed_handler,
                   gimp_tree_handler_disconnect);

  g_slice_free (GimpContainerViewPrivate, private);
}

GimpContainerViewPrivate *
_gimp_container_view_get_private (GimpContainerView *view)
{
  GimpContainerViewPrivate *private;

  static GQuark private_key = 0;

  g_return_val_if_fail (GIMP_IS_CONTAINER_VIEW (view), NULL);

  if (! private_key)
    private_key = g_quark_from_static_string ("gimp-container-view-private");

  private = g_object_get_qdata ((GObject *) view, private_key);

  if (! private)
    {
      GimpContainerViewInterface *view_iface;

      view_iface = GIMP_CONTAINER_VIEW_GET_IFACE (view);

      private = g_slice_new0 (GimpContainerViewPrivate);

      private->view_border_width = 1;

      if (! GIMP_CONTAINER_VIEW_GET_IFACE (view)->use_list_model)
        private->item_hash = g_hash_table_new_full (g_direct_hash,
                                                    g_direct_equal,
                                                    NULL,
                                                    view_iface->insert_data_free);

      g_object_set_qdata_full ((GObject *) view, private_key, private,
                               (GDestroyNotify)
                               gimp_container_view_private_finalize);

      g_signal_connect (view, "destroy",
                        G_CALLBACK (gimp_container_view_private_dispose),
                        private);
    }

  return private;
}
