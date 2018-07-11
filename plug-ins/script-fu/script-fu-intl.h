/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * script-fu-intl.h
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

#ifndef __SCRIPT_FU_INTL_H__
#define __SCRIPT_FU_INTL_H__

#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to script-fu-intl.h"
#endif

#include <glib/gi18n.h>


#ifndef HAVE_BIND_TEXTDOMAIN_CODESET
#    define bind_textdomain_codeset(Domain, Codeset) (Domain)
#endif

#define INIT_I18N()	G_STMT_START{			          \
  bindtextdomain (GETTEXT_PACKAGE"-script-fu",                    \
                  gimp_locale_directory ());                      \
  bind_textdomain_codeset (GETTEXT_PACKAGE"-script-fu", "UTF-8"); \
  textdomain (GETTEXT_PACKAGE"-script-fu");                       \
}G_STMT_END


#endif /* __SCRIPT_FU_INTL_H__ */
