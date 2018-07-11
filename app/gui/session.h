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

#ifndef __SESSION_H__
#define __SESSION_H__


void       session_init    (Gimp       *gimp);
void       session_exit    (Gimp       *gimp);

void       session_restore (Gimp       *gimp,
                            GdkMonitor *monitor);
void       session_save    (Gimp       *gimp,
                            gboolean    always_save);

gboolean   session_clear   (Gimp       *gimp,
                            GError    **error);


#endif  /*  __SESSION_H__  */
