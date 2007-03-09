/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimphintbox.h
 * Copyright (C) 2006 Sven Neumann <sven@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_HINT_BOX_H__
#define __GIMP_HINT_BOX_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_HINT_BOX  (gimp_hint_box_get_type ())

GType       gimp_hint_box_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_hint_box_new      (const gchar *hint);


G_END_DECLS

#endif /* __GIMP_HINT_BOX_H__ */
