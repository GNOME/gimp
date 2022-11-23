/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaTreeHandler
 * Copyright (C) 2009  Michael Natterer <mitch@ligma.org>
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

#include "ligmacontainer.h"
#include "ligmatreehandler.h"
#include "ligmaviewable.h"


static void   ligma_tree_handler_dispose          (GObject         *object);

static void   ligma_tree_handler_freeze           (LigmaTreeHandler *handler,
                                                  LigmaContainer   *container);
static void   ligma_tree_handler_thaw             (LigmaTreeHandler *handler,
                                                  LigmaContainer   *container);

static void   ligma_tree_handler_add_container    (LigmaTreeHandler *handler,
                                                  LigmaContainer   *container);
static void   ligma_tree_handler_add_foreach      (LigmaViewable    *viewable,
                                                  LigmaTreeHandler *handler);
static void   ligma_tree_handler_add              (LigmaTreeHandler *handler,
                                                  LigmaViewable    *viewable,
                                                  LigmaContainer   *container);

static void   ligma_tree_handler_remove_container (LigmaTreeHandler *handler,
                                                  LigmaContainer   *container);
static void   ligma_tree_handler_remove_foreach   (LigmaViewable    *viewable,
                                                  LigmaTreeHandler *handler);
static void   ligma_tree_handler_remove           (LigmaTreeHandler *handler,
                                                  LigmaViewable    *viewable,
                                                  LigmaContainer   *container);


G_DEFINE_TYPE (LigmaTreeHandler, ligma_tree_handler, LIGMA_TYPE_OBJECT)

#define parent_class ligma_tree_handler_parent_class


static void
ligma_tree_handler_class_init (LigmaTreeHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = ligma_tree_handler_dispose;
}

static void
ligma_tree_handler_init (LigmaTreeHandler *handler)
{
}

static void
ligma_tree_handler_dispose (GObject *object)
{
  LigmaTreeHandler *handler = LIGMA_TREE_HANDLER (object);

  if (handler->container)
    {
      g_signal_handlers_disconnect_by_func (handler->container,
                                            ligma_tree_handler_freeze,
                                            handler);
      g_signal_handlers_disconnect_by_func (handler->container,
                                            ligma_tree_handler_thaw,
                                            handler);

      if (! ligma_container_frozen (handler->container))
        ligma_tree_handler_remove_container (handler, handler->container);

      g_clear_object (&handler->container);
      g_clear_pointer (&handler->signal_name, g_free);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


/*  public functions  */

LigmaTreeHandler *
ligma_tree_handler_connect (LigmaContainer *container,
                           const gchar   *signal_name,
                           GCallback      callback,
                           gpointer       user_data)
{
  LigmaTreeHandler *handler;

  g_return_val_if_fail (LIGMA_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (signal_name != NULL, NULL);

  handler = g_object_new (LIGMA_TYPE_TREE_HANDLER, NULL);

  handler->container   = g_object_ref (container);
  handler->signal_name = g_strdup (signal_name);
  handler->callback    = callback;
  handler->user_data   = user_data;

  if (! ligma_container_frozen (container))
    ligma_tree_handler_add_container (handler, container);

  g_signal_connect_object (container, "freeze",
                           G_CALLBACK (ligma_tree_handler_freeze),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_tree_handler_thaw),
                           handler,
                           G_CONNECT_SWAPPED);

  return handler;
}

void
ligma_tree_handler_disconnect (LigmaTreeHandler *handler)
{
  g_return_if_fail (LIGMA_IS_TREE_HANDLER (handler));

  g_object_run_dispose (G_OBJECT (handler));
  g_object_unref (handler);
}


/*  private functions  */

static void
ligma_tree_handler_freeze (LigmaTreeHandler *handler,
                          LigmaContainer   *container)
{
  ligma_tree_handler_remove_container (handler, container);
}

static void
ligma_tree_handler_thaw (LigmaTreeHandler *handler,
                        LigmaContainer   *container)
{
  ligma_tree_handler_add_container (handler, container);
}

static void
ligma_tree_handler_add_container (LigmaTreeHandler *handler,
                                 LigmaContainer   *container)
{
  ligma_container_foreach (container,
                          (GFunc) ligma_tree_handler_add_foreach,
                          handler);

  g_signal_connect_object (container, "add",
                           G_CALLBACK (ligma_tree_handler_add),
                           handler,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_tree_handler_remove),
                           handler,
                           G_CONNECT_SWAPPED);
}

static void
ligma_tree_handler_add_foreach (LigmaViewable    *viewable,
                               LigmaTreeHandler *handler)
{
  ligma_tree_handler_add (handler, viewable, NULL);
}

static void
ligma_tree_handler_add (LigmaTreeHandler *handler,
                       LigmaViewable    *viewable,
                       LigmaContainer   *unused)
{
  LigmaContainer *children = ligma_viewable_get_children (viewable);

  g_signal_connect (viewable,
                    handler->signal_name,
                    handler->callback,
                    handler->user_data);

  if (children)
    ligma_tree_handler_add_container (handler, children);
}

static void
ligma_tree_handler_remove_container (LigmaTreeHandler *handler,
                                    LigmaContainer   *container)
{
  g_signal_handlers_disconnect_by_func (container,
                                        ligma_tree_handler_add,
                                        handler);
  g_signal_handlers_disconnect_by_func (container,
                                        ligma_tree_handler_remove,
                                        handler);

  ligma_container_foreach (container,
                          (GFunc) ligma_tree_handler_remove_foreach,
                          handler);
}

static void
ligma_tree_handler_remove_foreach (LigmaViewable    *viewable,
                                  LigmaTreeHandler *handler)
{
  ligma_tree_handler_remove (handler, viewable, NULL);
}

static void
ligma_tree_handler_remove (LigmaTreeHandler *handler,
                          LigmaViewable    *viewable,
                          LigmaContainer   *unused)
{
  LigmaContainer *children = ligma_viewable_get_children (viewable);

  if (children)
    ligma_tree_handler_remove_container (handler, children);

  g_signal_handlers_disconnect_by_func (viewable,
                                        handler->callback,
                                        handler->user_data);
}
