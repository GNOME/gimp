/* base64.h - encode and decode base64 encoding according to RFC 1521
 *
 * Copyright (C) 2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef BASE64_H
#define BASE64_H

#include <glib.h>

G_BEGIN_DECLS

gssize base64_decode (const gchar *src_b64,
                      gsize        src_size,
                      gchar       *dest,
                      gsize        dest_size,
                      gboolean     ignore_errors);

gssize base64_encode (const gchar *src,
                      gsize        src_size,
                      gchar       *dest_b64,
                      gsize        dest_size,
                      gint         columns);

G_END_DECLS

#endif /* BASE64_H */
