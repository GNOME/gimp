/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
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
 *
 */

#ifndef _IMAP_POLYGON_H
#define _IMAP_POLYGON_H

#include "imap_object.h"

typedef struct
{
  Object_t obj;
  GList *points;
} Polygon_t;

#define ObjectToPolygon(obj) ((Polygon_t*) (obj))

Object_t        *create_polygon      (GList *points);
ObjectFactory_t *get_polygon_factory (guint  state);

void polygon_insert_point      (GSimpleAction *action,
                                GVariant      *new_state,
                                gpointer       user_data);
void polygon_delete_point      (GSimpleAction *action,
                                GVariant      *new_state,
                                gpointer       user_data);

void polygon_remove_last_point (Polygon_t     *polygon);
void polygon_append_point      (Polygon_t     *polygon,
                                gint           x,
                                gint           y);
GdkPoint *new_point            (gint           x,
                                gint           y);

#endif /* _IMAP_POLYGON_H */
