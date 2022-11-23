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

#ifndef __LIGMA_TREE_HANDLER_H__
#define __LIGMA_TREE_HANDLER_H__


#include "core/ligmaobject.h"


#define LIGMA_TYPE_TREE_HANDLER            (ligma_tree_handler_get_type ())
#define LIGMA_TREE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_TREE_HANDLER, LigmaTreeHandler))
#define LIGMA_TREE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_TREE_HANDLER, LigmaTreeHandlerClass))
#define LIGMA_IS_TREE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_TREE_HANDLER))
#define LIGMA_IS_TREE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_TREE_HANDLER))
#define LIGMA_TREE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_TREE_HANDLER, LigmaTreeHandlerClass))


typedef struct _LigmaTreeHandlerClass LigmaTreeHandlerClass;

struct _LigmaTreeHandler
{
  LigmaObject     parent_instance;

  LigmaContainer *container;

  gchar         *signal_name;
  GCallback      callback;
  gpointer       user_data;
};

struct _LigmaTreeHandlerClass
{
  LigmaObjectClass  parent_class;
};


GType             ligma_tree_handler_get_type   (void) G_GNUC_CONST;

LigmaTreeHandler * ligma_tree_handler_connect    (LigmaContainer   *container,
                                                const gchar     *signal_name,
                                                GCallback        callback,
                                                gpointer         user_data);
void              ligma_tree_handler_disconnect (LigmaTreeHandler *handler);


#endif /* __LIGMA_TREE_HANDLER_H__ */
