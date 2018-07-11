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

#ifndef __GIMP_TREE_HANDLER_H__
#define __GIMP_TREE_HANDLER_H__


#include "core/gimpobject.h"


#define GIMP_TYPE_TREE_HANDLER            (gimp_tree_handler_get_type ())
#define GIMP_TREE_HANDLER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TREE_HANDLER, GimpTreeHandler))
#define GIMP_TREE_HANDLER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TREE_HANDLER, GimpTreeHandlerClass))
#define GIMP_IS_TREE_HANDLER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TREE_HANDLER))
#define GIMP_IS_TREE_HANDLER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TREE_HANDLER))
#define GIMP_TREE_HANDLER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TREE_HANDLER, GimpTreeHandlerClass))


typedef struct _GimpTreeHandlerClass GimpTreeHandlerClass;

struct _GimpTreeHandler
{
  GimpObject     parent_instance;

  GimpContainer *container;

  gchar         *signal_name;
  GCallback      callback;
  gpointer       user_data;
};

struct _GimpTreeHandlerClass
{
  GimpObjectClass  parent_class;
};


GType             gimp_tree_handler_get_type   (void) G_GNUC_CONST;

GimpTreeHandler * gimp_tree_handler_connect    (GimpContainer   *container,
                                                const gchar     *signal_name,
                                                GCallback        callback,
                                                gpointer         user_data);
void              gimp_tree_handler_disconnect (GimpTreeHandler *handler);


#endif /* __GIMP_TREE_HANDLER_H__ */
