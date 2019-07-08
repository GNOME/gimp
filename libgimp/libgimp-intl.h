/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * libgimp-intl.h
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
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __LIBGIMP_INTL_H__
#define __LIBGIMP_INTL_H__

#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to libgimp-intl.h"
#endif

#include <libintl.h>


#define  _(String) dgettext (GETTEXT_PACKAGE "-libgimp", String)
#define Q_(String) g_dpgettext (GETTEXT_PACKAGE "-libgimp", String, 0)
#define C_(Context,String) g_dpgettext (GETTEXT_PACKAGE "-libgimp", Context "\004" String, strlen (Context) + 1)

#undef gettext
#define gettext(String) dgettext (GETTEXT_PACKAGE "-libgimp", String)

#undef ngettext
#define ngettext(String1, String2, number) dngettext (GETTEXT_PACKAGE "-libgimp", String1, String2, number)

#define N_(String) (String)
#define NC_(Context,String) (String)


#endif /* __LIBGIMP_INTL_H__ */
