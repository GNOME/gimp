/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball 
 *
 * stdplugins-intl.h
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

#ifndef __STDPLUGINS_INTL_H__
#define __STDPLUGINS_INTL_H__

#include "gimpintl.h"


#define INIT_I18N()	G_STMT_START{                               \
  bindtextdomain(GETTEXT_PACKAGE"-libgimp", LOCALEDIR);             \
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libgimp", "UTF-8");     \
  bindtextdomain(GETTEXT_PACKAGE"-std-plugins", LOCALEDIR);         \
  bind_textdomain_codeset (GETTEXT_PACKAGE"-std-plugins", "UTF-8"); \
  textdomain(GETTEXT_PACKAGE"-std-plugins");		            \
  setlocale (LC_NUMERIC, "C");                                      \
}G_STMT_END

#define INIT_I18N_UI()	G_STMT_START{	\
  gtk_set_locale();			\
  INIT_I18N();				\
}G_STMT_END


#endif /* __STDPLUGINS_INTL_H__ */
