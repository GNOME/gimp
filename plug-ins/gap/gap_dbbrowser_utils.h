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
   gap_dbbrowser_utils.h (original code dbbrowser_utils.h by Thomas NOEL <thomas@minet.net>
   
   09. dec .1998   hof: update for GIMP 1.1
   20. dec .1997   hof: GAP variant of DBbrowser
                        removed apply_callback
                        added constraint_procedure, 
                        added 2 buttons
                        added return type

   0.08  26th sept 97  by Thomas NOEL <thomas@minet.net> 
*/

#ifndef _GAP_DB_BROWSER_UTILS_H
#define _GAP_DB_BROWSER_UTILS_H

/* configuration */

#define DBL_LIST_WIDTH 220
#define DBL_WIDTH DBL_LIST_WIDTH+400
#define DBL_HEIGHT 250

/* end of configuration */

#include <stdlib.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
# include "gap_filter.h"

typedef struct {
  gchar *label;
  gchar *func;
} ListEntry_t;

typedef struct {

  GtkWidget* dlg;

  GtkWidget* search_entry;
  GtkWidget* name_button;
  GtkWidget* blurb_button;
  GtkWidget* app_const_button;
  GtkWidget* app_vary_button;

  GtkWidget* descr_scroll;
  GtkWidget* descr_table;

  GtkWidget* clist;
  GtkWidget* scrolled_win;

  /* the currently selected procedure */
  gchar           *selected_proc_name;
  gchar           *selected_scheme_proc_name;
  gchar           *selected_proc_blurb;
  gchar           *selected_proc_help;
  gchar           *selected_proc_author;
  gchar           *selected_proc_copyright;
  gchar           *selected_proc_date;
  GimpPDBProcType  selected_proc_type;
  gint             selected_nparams;
  gint             selected_nreturn_vals;
  GimpParamDef    *selected_params;
  GimpParamDef    *selected_return_vals; 

  t_constraint_func      constraint_func;
  t_constraint_func      constraint_func_sel1;
  t_constraint_func      constraint_func_sel2;
  t_gap_db_browse_result *result;
  
  gint codegen_flag;
} dbbrowser_t;

/* local functions */

static void
dialog_button_1_callback(GtkWidget *, gpointer );

static void
dialog_button_2_callback(GtkWidget *, gpointer );

static void
dialog_button_3_callback(GtkWidget *, gpointer );

static gint
procedure_select_callback (GtkWidget *widget,
			   gint row, 
			   gint column, 
			   GdkEventButton * bevent,
			   gpointer data);

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
convert_string (gchar *str);

static gchar* 
GParamType2char(GimpPDBArgType t);

#endif
