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

#ifndef __ERROR_PORT_H__
#define __ERROR_PORT_H__

void         error_port_init                  (scheme *sc);
void         error_port_redirect_output       (scheme *sc);
gboolean     error_port_is_redirect_output    (scheme *sc);

port        *error_port_get_port_rep          (scheme *sc);
const gchar *error_port_take_string_and_close (scheme *sc);

#endif /* __ERROR_PORT_H__ */