/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * libgimp-intl.h
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

#ifndef __LIBGIMP_INTL_H__
#define __LIBGIMP_INTL_H__

#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to libgimp-intl.h"
#endif

#include <libintl.h>


#define _(String) dgettext (GETTEXT_PACKAGE "-libgimp", String)
#define Q_(String) g_strip_context ((String), dgettext (GETTEXT_PACKAGE "-libgimp", String))

#undef gettext
#define gettext(String) dgettext (GETTEXT_PACKAGE "-libgimp", String)

#undef ngettext
#define ngettext(String1, String2, number) dngettext (GETTEXT_PACKAGE "-libgimp", String1, String2, number)

#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif


#endif /* __LIBGIMP_INTL_H__ */
