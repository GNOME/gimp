/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpui.h
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef __GIMP_UI_H__
#define __GIMP_UI_H__

#include <gtk/gtk.h>

#include "gimphelp.h"

#include "libgimp/gimpchainbutton.h"
#include "libgimp/gimpdialog.h"
#include "libgimp/gimpfileselection.h"
#include "libgimp/gimppixmap.h"
#include "libgimp/gimppatheditor.h"
#include "libgimp/gimpquerybox.h"
#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpwidgets.h"

/*  a simple message box  */

void gimp_message_box   (gchar        *message,
			 GtkCallback   callback,
			 gpointer      data);

void gimp_dialog_hide   (GtkWidget    *dialog);

#endif /* __GIMP_UI_H__ */
