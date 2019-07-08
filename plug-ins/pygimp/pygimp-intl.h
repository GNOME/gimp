/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * pygimp-intl.h
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

#ifndef __PYGIMP_INTL_H__
#define __PYGIMP_INTL_H__

#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to pygimp-intl.h"
#endif

#include <libintl.h>


#define  _(String) dgettext (GETTEXT_PACKAGE "-python", String)
#define Q_(String) g_dpgettext (GETTEXT_PACKAGE "-python", String, 0)
#define C_(Context,String) g_dpgettext (GETTEXT_PACKAGE "-python", Context "\004" String, strlen (Context) + 1)

#undef gettext
#define gettext(String) dgettext (GETTEXT_PACKAGE "-python", String)

#undef ngettext
#define ngettext(String1, String2, number) dngettext (GETTEXT_PACKAGE "-python", String1, String2, number)

#ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#else
#    define N_(String) (String)
#endif


#endif /* __PYGIMP_INTL_H__ */
