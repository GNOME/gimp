/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * script-fu-intl.h
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
#ifndef __SCRIPT_FU_INTL_H__
#define __SCRIPT_FU_INTL_H__

#include "libgimp/gimpintl.h"

#ifdef HAVE_LC_MESSAGES
#define INIT_I18N()	G_STMT_START{			\
  setlocale(LC_MESSAGES, ""); 				\
  bindtextdomain("gimp-libgimp", LOCALEDIR);            \
  bindtextdomain("gimp-script-fu", LOCALEDIR);	        \
  textdomain("gimp-script-fu");			        \
  			}G_STMT_END
#else
#define INIT_I18N()	G_STMT_START{			\
  bindtextdomain("gimp-script-fu", LOCALEDIR);	        \
  textdomain("gimp-script-fu");                         \
  			}G_STMT_END
#endif

#define INIT_I18N_UI()	G_STMT_START{	\
  gtk_set_locale();			\
  setlocale (LC_NUMERIC, "C");		\
  INIT_I18N();				\
			}G_STMT_END

#endif /* __SCRIPT_FU_INTL_H__ */
