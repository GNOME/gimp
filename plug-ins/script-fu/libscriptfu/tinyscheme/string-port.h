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

#ifndef __STRING_PORT_H__
#define __STRING_PORT_H__

pointer string_port_open_output_string (scheme      *sc,
                                        pointer      scheme_string);
pointer string_port_open_input_string  (scheme      *sc,
                                        pointer      scheme_string,
                                        int          prop);
void    string_port_dispose            (scheme      *sc,
                                        pointer      port);
void    string_port_dispose_struct     (scheme      *sc,
                                        port        *port);
void    string_port_init_static_port   (port        *port,
                                        const gchar *command);

gint    string_port_inbyte             (port        *pt);
void    string_port_backbyte           (port        *pt);
void    string_port_put_bytes          (scheme      *sc,
                                        port        *port,
                                        const gchar *bytes,
                                        guint        byte_count);

pointer string_port_get_output_string  (scheme      *sc,
                                        port        *port);

port   *string_port_open_output_port   (scheme      *sc);

#endif /* __STRING_PORT_H__ */