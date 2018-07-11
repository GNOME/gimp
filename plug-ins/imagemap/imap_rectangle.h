/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#ifndef _IMAP_RECTANGLE_H
#define _IMAP_RECTANGLE_H

#include "imap_object.h"

typedef struct {
   Object_t obj;
   gint x;
   gint y;
   gint width;
   gint height;
} Rectangle_t;

#define ObjectToRectangle(obj) ((Rectangle_t*) (obj))

Object_t* create_rectangle(gint x, gint y, gint width, gint height);
ObjectFactory_t *get_rectangle_factory(guint state);

#endif /* _IMAP_RECTANGLE_H */
