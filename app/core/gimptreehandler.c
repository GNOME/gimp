/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpTreeHandler
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gimpcontainer.h"
#include "gimptreehandler.h"
#include "gimpviewable.h"


static void   gimp_tree_handler_dispose          (GObject         *object);

static void   gimp_tree_handler_freeze           (GimpTreeHandler *handler,
                                                  GimpContainer   *container);
static void   gimp_tree_handler_thaw             (GimpTreeHandler *handler,
                                                  GimpContainer   *container);

static void   gimp_tree_handler_add_container    (GimpTreeHandler *handler,
                                                  GimpContainer   *container);
static void   gimp_tree_handler_add_foreach      (GimpViewable    *viewable,
                                                  GimpTreeHandler *handler);
static void   gimp_tree_handler_add              (GimpTreeHandler *handler,
                                                  GimpViewable    *viewable,
                                                  GimpContainer   *container);

static void   gimp_tree_handler_remove_container (GimpTreeHandler *handler,
                                                  GimpContainer   *container);
static void   gimp_tree_handler_remove_foreach   (GimpViewable    *viewable,
                                                  GimpTreeHandler *handler);
static void   gimp_tree_handler_remove           (GimpTreeHandler *handler,
                                                  GimpViewable    *viewable,
                                                  GimpContainer   *container);


G_DEFINE_TYPE (GimpTreeHandler, gimp_tree_handler, GIMP_TYPE_OBJECT)

#define parent_class gimp_tree_handler_parent_class


static void
gimp_tree_handler_class_init (GimpTreeHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = gimp_tree_handler_dispose;
}

static void
gimp_tree_handler_init (GimpTreeHandler *handler)
{
}

static void
gimp_tree_handler_dispose (GObject *object)
{
  GimpTreeHandler *handler = GIMP_TREE_HANDLER (object);

  if (handler->container)
    {
      g_signal_handlers_disconnect_by_func (handler->container,
                                            gimp_tree_handler_freeze,
                                            handler);
      g_signal_handlers_disconnect_by_func (handler->container,
                                            gimp_tree_handler_thaw,
                                            handler);

      if (! gimp_container_frozen (handler->container))
        gimp_tree_handler_remove_container (handler, handler->container);

      g_clear_object (&handler->container);
      g_clear_pointer (&handler->signal_name, g_free);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

GimpTreeHandler *
gimp_tree_handler_connect (GimpContainer *container,
                           const gchar   *signal_name,
                           GCallback      callback,
                           gpointer       user_data)
{
  GimpTreeHandler *handler;

  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (signal_name != NULL, NULL);

  handler = g_object_new (GIMP_TYPE_TREE_HANDLER, NULL);

  handler->container   = g_object_ref (container);
  handler->signal_name = g_strdup (signal_name);
  handler->callback    = callback;
  handler->user_data   = user_data;

  if (! gimp_container_frozen (container))
    gimp_tree_handler_add_container (handler, container);

  g_signal_connect_object (container, "freeze",
                           G_CALLBACK (gimp_tree_handler_freeze),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (gimp_tree_handler_thaw),
                           handler,
                           G_CONNECT_SWAPPED);

  return handler;
}

void
gimp_tree_handler_disconnect (GimpTreeHandler *handler)
{
  g_return_if_fail (GIMP_IS_TREE_HANDLER (handler));

  g_object_run_dispose (G_OBJECT (handler));
  g_object_unref (handler);
}


/*  private functions  */

static void
gimp_tree_handler_freeze (GimpTreeHandler *handler,
                          GimpContainer   *container)
{
  gimp_tree_handler_remove_container (handler, container);
}

static void
gimp_tree_handler_thaw (GimpTreeHandler *handler,
                        GimpContainer   *container)
{
  gimp_tree_handler_add_container (handler, container);
}

static void
gimp_tree_handler_add_container (GimpTreeHandler *handler,
                                 GimpContainer   *container)
{
  gimp_container_foreach (container,
                          (GFunc) gimp_tree_handler_add_foreach,
                          handler);

  g_signal_connect_object (container, "add",
                           G_CALLBACK (gimp_tree_handler_add),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (gimp_tree_handler_remove),
                           handler,
                           G_CONNECT_SWAPPED);
}

static void
gimp_tree_handler_add_foreach (GimpViewable    *viewable,
                               GimpTreeHandler *handler)
{
  gimp_tree_handler_add (handler, viewable, NULL);
}

static void
gimp_tree_handler_add (GimpTreeHandler *handler,
                       GimpViewable    *viewable,
                       GimpContainer   *unused)
{
  GimpContainer *children = gimp_viewable_get_children (viewable);

  g_signal_connect (viewable,
                    handler->signal_name,
                    handler->callback,
                    handler->user_data);

  if (children)
    gimp_tree_handler_add_container (handler, children);
}

static void
gimp_tree_handler_remove_container (GimpTreeHandler *handler,
                                    GimpContainer   *container)
{
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_tree_handler_add,
                                        handler);
  g_signal_handlers_disconnect_by_func (container,
                                        gimp_tree_handler_remove,
                                        handler);

  gimp_container_foreach (container,
                          (GFunc) gimp_tree_handler_remove_foreach,
                          handler);
}

static void
gimp_tree_handler_remove_foreach (GimpViewable    *viewable,
                                  GimpTreeHandler *handler)
{
  gimp_tree_handler_remove (handler, viewable, NULL);
}

static void
gimp_tree_handler_remove (GimpTreeHandler *handler,
                          GimpViewable    *viewable,
                          GimpContainer   *unused)
{
  GimpContainer *children = gimp_viewable_get_children (viewable);

  if (children)
    gimp_tree_handler_remove_container (handler, children);

  g_signal_handlers_disconnect_by_func (viewable,
                                        handler->callback,
                                        handler->user_data);
}
