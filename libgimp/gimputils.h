/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimputils.h
 *
 * Copyright (C) 2000 Sven Neumann <sven@gimp.org>
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

#ifndef __GIMPUTILS_H__
#define __GIMPUTILS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


/*  
 *  Right now all you find here are the g_strescape and g_strcompress 
 *  function out of glib-1.3. We need their functionality, but don't 
 *  want to rely on that version being installed 
 */

#if (defined (GLIB_CHECK_VERSION) && GLIB_CHECK_VERSION (1,3,1))
#define gimp_strescape(string, exceptions) g_strescape (string, exceptions)
#define gimp_strcompress(string)           g_strcompress (string)
#else
gchar * gimp_strescape   (const gchar *source, 
			  const gchar *exceptions);
gchar * gimp_strcompress (const gchar *source);
#endif  /* GLIB <= 1.3 */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMPUTILS_H__ */



