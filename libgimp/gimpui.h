/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#ifndef __GIMP_UI_H__
#define __GIMP_UI_H__

#include <gtk/gtk.h>

#include <libgimp/gimpchainbutton.h>
#include <libgimp/gimpcolorbutton.h>
#include <libgimp/gimpdialog.h>
#include <libgimp/gimpexport.h>
#include <libgimp/gimpfileselection.h>
#include <libgimp/gimphelpui.h>
#include <libgimp/gimpmenu.h>
#include <libgimp/gimppatheditor.h>
#include <libgimp/gimppixmap.h>
#include <libgimp/gimpquerybox.h>
#include <libgimp/gimpsizeentry.h>
#include <libgimp/gimpunitmenu.h>
#include <libgimp/gimpwidgets.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For information look into the C source or the html documentation */


void gimp_ui_init (gchar    *prog_name,
		   gboolean  preview);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GIMP_UI_H__ */
