/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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


/* 
   dbbrowser.h
   0.08  26th sept 97  by Thomas NOEL <thomas@minet.net> 
*/

#include "gtk/gtk.h"
#include "libgimp/gimp.h"

GtkWidget*
gimp_db_browser (void (* apply_callback) 
		 ( gchar *selected_proc_name,
		   gchar *selected_scheme_proc_name,
		   gchar *selected_proc_blurb,
		   gchar *selected_proc_help,
		   gchar *selected_proc_author,
		   gchar *selected_proc_copyright,
		   gchar *selected_proc_date,
		   int selected_proc_type,
		   int selected_nparams,
		   int selected_nreturn_vals,
		   GParamDef *selected_params,
		   GParamDef *selected_return_vals ));


