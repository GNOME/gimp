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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


/* 
   dbbrowser_utils.h
   0.08  26th sept 97  by Thomas NOEL <thomas@minet.net> 
*/

/* configuration */

#define DBL_LIST_WIDTH 220
#define DBL_WIDTH DBL_LIST_WIDTH+400
#define DBL_HEIGHT 250

/* end of configuration */

#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "dbbrowser.h"

typedef struct {

  GtkWidget* dlg;

  GtkWidget* search_entry;
  GtkWidget* name_button;
  GtkWidget* blurb_button;

  GtkWidget* descr_scroll;
  GtkWidget* descr_table;

  GtkWidget* list;

  /* the currently selected procedure */
  gchar *selected_proc_name;
  gchar *selected_scheme_proc_name;
  gchar *selected_proc_blurb;
  gchar *selected_proc_help;
  gchar *selected_proc_author;
  gchar *selected_proc_copyright;
  gchar *selected_proc_date;
  int selected_proc_type;
  int selected_nparams;
  int selected_nreturn_vals;
  GParamDef *selected_params;
  GParamDef *selected_return_vals; 

  void (*apply_callback) ( gchar *selected_proc_name,
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
			   GParamDef *selected_return_vals );

} dbbrowser_t;

/* local functions */

static void
dialog_apply_callback(GtkWidget *, gpointer );

static gint
dialog_list_button (GtkWidget      *widget,
		    GdkEventButton *event,
		    gpointer        data);

static void
dialog_search_callback(GtkWidget *, 
		       gpointer);

static void
dialog_select(dbbrowser_t *dbbrowser, 
	      gchar *proc_name);

static void
dialog_close_callback(GtkWidget *, 
		      gpointer);

static void 
dialog_selection_free_filename (GtkWidget *widget,
				gpointer   client_data);

static void      
convert_string (gchar *str);

static gchar* 
GParamType2char(GParamType t);

