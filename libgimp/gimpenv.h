/* gimpenv.h
 *
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
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

#ifndef __GIMPENV_H__
#define __GIMPENV_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */

gchar* gimp_directory        (void);
gchar* gimp_personal_rc_file (gchar *basename);
gchar* gimp_data_directory   (void);
gchar* gimp_gtkrc            (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /*  __GIMPENV_H__  */
