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
   dbbrowser_utils.c
   0.08  26th sept 97  by Thomas NOEL <thomas@minet.net> 
*/

#include "dbbrowser_utils.h"

GtkWidget*
gimp_db_browser(void (* apply_callback) ( gchar     *selected_proc_name,
					  gchar     *selected_scheme_proc_name,
					  gchar     *selected_proc_blurb,
					  gchar     *selected_proc_help,
					  gchar     *selected_proc_author,
					  gchar     *selected_proc_copyright,
					  gchar     *selected_proc_date,
					  int        selected_proc_type,
					  int        selected_nparams,
					  int        selected_nreturn_vals,
					  GParamDef *selected_params,
					  GParamDef *selected_return_vals ) )
  /* create the dialog box */
  /* console_entry != NULL => called from the script-fu-console */
 
{
  
  dbbrowser_t* dbbrowser;
  
  GtkWidget *button;
  GtkWidget *hbox,*searchhbox,*vbox;
  GtkWidget *label;
  GtkWidget *list_scroll;
  
  dbbrowser = (gpointer)malloc(sizeof(dbbrowser_t));
  
  dbbrowser->apply_callback = apply_callback;
  
  /* the dialog box */

  dbbrowser->dlg = gtk_dialog_new ();
  
  gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), "DB Browser (init)");
  gtk_window_position (GTK_WINDOW (dbbrowser->dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->dlg), "destroy",
                      (GtkSignalFunc) dialog_close_callback,
                      dbbrowser);
  
  /* hbox : left=list ; right=description */
  
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->vbox), 
		      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);
  
  /* left = vbox : the list and the search entry */
  
  vbox = gtk_vbox_new( FALSE, 0 );
  gtk_container_border_width (GTK_CONTAINER (vbox), 3); 
  gtk_box_pack_start (GTK_BOX (hbox), 
		      vbox, FALSE, TRUE, 0);
  gtk_widget_show(vbox);
  
  /* list : list in a scrolled_win */
  
  list_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (list_scroll),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (vbox), 
		      list_scroll, TRUE, TRUE, 0);
  gtk_widget_set_usize(list_scroll, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_widget_show(list_scroll);
  
  dbbrowser->list = gtk_list_new();
  gtk_list_set_selection_mode (GTK_LIST (dbbrowser->list), 
			       GTK_SELECTION_BROWSE);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->list), "button_press_event",
		      (GtkSignalFunc) dialog_list_button, dbbrowser);
  gtk_container_add (GTK_CONTAINER (list_scroll), dbbrowser->list);
  gtk_widget_show(dbbrowser->list);

  /* search entry */

  searchhbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      searchhbox, FALSE, TRUE, 0);
  gtk_widget_show(searchhbox);

  label = gtk_label_new("Search :");
  gtk_misc_set_alignment( GTK_MISC(label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      label, TRUE, TRUE, 0);
  gtk_widget_show(label);

  dbbrowser->search_entry = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      dbbrowser->search_entry, TRUE, TRUE, 0);
  gtk_widget_show(dbbrowser->search_entry);

  /* right = description */

  dbbrowser->descr_scroll = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dbbrowser->descr_scroll),
				  GTK_POLICY_ALWAYS, 
				  GTK_POLICY_ALWAYS
				  );
  gtk_box_pack_start (GTK_BOX (hbox), 
		      dbbrowser->descr_scroll, TRUE, TRUE, 0);
  gtk_widget_set_usize (dbbrowser->descr_scroll, DBL_WIDTH - DBL_LIST_WIDTH, 0);
  gtk_widget_show (dbbrowser->descr_scroll);

  /* buttons in dlg->action_aera */

  gtk_container_border_width (GTK_CONTAINER (GTK_DIALOG(dbbrowser->dlg)->action_area), 0);

  dbbrowser->name_button = gtk_button_new_with_label ("Search by name");
  GTK_WIDGET_SET_FLAGS (dbbrowser->name_button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->name_button), "clicked",
                      (GtkSignalFunc) dialog_search_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		    dbbrowser->name_button , TRUE, TRUE, 0);
  gtk_widget_show(dbbrowser->name_button);

  dbbrowser->blurb_button = gtk_button_new_with_label ("Search by blurb");
  GTK_WIDGET_SET_FLAGS (dbbrowser->blurb_button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->blurb_button), "clicked",
                      (GtkSignalFunc) dialog_search_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		    dbbrowser->blurb_button , TRUE, TRUE, 0);
  gtk_widget_show(dbbrowser->blurb_button);

  if (apply_callback) {
    button = gtk_button_new_with_label ("Apply");
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) dialog_apply_callback, dbbrowser );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_widget_show (button);
  }

  button = gtk_button_new_with_label ("Close");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) dialog_close_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /* now build the list */

  gtk_widget_show (dbbrowser->list); 
  gtk_widget_show (dbbrowser->dlg);

  /* initialize the "return" value (for "apply") */

  dbbrowser->descr_table = NULL;
  dbbrowser->selected_proc_name = NULL;
  dbbrowser->selected_scheme_proc_name = NULL;
  dbbrowser->selected_proc_blurb = NULL;
  dbbrowser->selected_proc_help = NULL;
  dbbrowser->selected_proc_author = NULL;
  dbbrowser->selected_proc_copyright = NULL;
  dbbrowser->selected_proc_date = NULL;
  dbbrowser->selected_proc_type = 0;
  dbbrowser->selected_nparams = 0;
  dbbrowser->selected_nreturn_vals = 0;
  dbbrowser->selected_params = NULL;
  dbbrowser->selected_return_vals = NULL;

  /* first search (all procedures) */
  dialog_search_callback( NULL, (gpointer)dbbrowser );

  return dbbrowser->dlg;
}


static gint
dialog_list_button (GtkWidget      *widget,
		    GdkEventButton *event,
		    gpointer        data)
{
  dbbrowser_t *dbbrowser = data;
  GtkWidget *event_widget;
  gchar *proc_name;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (dbbrowser != NULL, FALSE);
  
  event_widget = gtk_get_event_widget ((GdkEvent*) event);
  if (GTK_IS_LIST_ITEM (event_widget))
    {
      switch (event->type)
	{
	case GDK_BUTTON_PRESS:
	  proc_name = gtk_object_get_user_data (GTK_OBJECT (event_widget));
	  /* gtk_widget_grab_focus (fs->selection_entry); */
	  dialog_select (dbbrowser, proc_name);
	  break;

	case GDK_2BUTTON_PRESS:
	  if (dbbrowser->apply_callback) {}
	  /* gtk_button_clicked (GTK_BUTTON (fs->ok_button)); */
	  break;
	  
	default:
	  break;
	}
    }
  
  return FALSE;
}

static void 
dialog_select (dbbrowser_t *dbbrowser, 
	       gchar       *proc_name)
  /* update the description box (right) */
{
  GtkWidget* label, *old_table;
  gint i,row=0;
  
  if (dbbrowser->selected_proc_name) 
    g_free(dbbrowser->selected_proc_name);
  dbbrowser->selected_proc_name = g_strdup(proc_name);

  if (dbbrowser->selected_scheme_proc_name)
    g_free(dbbrowser->selected_scheme_proc_name);
  dbbrowser->selected_scheme_proc_name = 
    g_strdup(proc_name);
  convert_string(dbbrowser->selected_scheme_proc_name);

  if (dbbrowser->selected_proc_blurb) g_free(dbbrowser->selected_proc_blurb);
  if (dbbrowser->selected_proc_help) g_free(dbbrowser->selected_proc_help);
  if (dbbrowser->selected_proc_author) g_free(dbbrowser->selected_proc_author);
  if (dbbrowser->selected_proc_copyright) g_free(dbbrowser->selected_proc_copyright);
  if (dbbrowser->selected_proc_date) g_free(dbbrowser->selected_proc_date);
  if (dbbrowser->selected_params) g_free(dbbrowser->selected_params);
  if (dbbrowser->selected_return_vals) g_free(dbbrowser->selected_return_vals);

  gimp_query_procedure (proc_name, 
			&(dbbrowser->selected_proc_blurb), 
			&(dbbrowser->selected_proc_help), 
			&(dbbrowser->selected_proc_author),
			&(dbbrowser->selected_proc_copyright), 
			&(dbbrowser->selected_proc_date), 
			&(dbbrowser->selected_proc_type), 
			&(dbbrowser->selected_nparams),
			&(dbbrowser->selected_nreturn_vals), 
			&(dbbrowser->selected_params), 
			&(dbbrowser->selected_return_vals));

  /* save the "old" table */
  old_table = dbbrowser->descr_table;

  dbbrowser->descr_table = gtk_table_new( 
       10 + dbbrowser->selected_nparams + dbbrowser->selected_nreturn_vals ,
       5 , FALSE );

  gtk_table_set_col_spacings( GTK_TABLE(dbbrowser->descr_table), 3);

  /* show the name */

  label = gtk_label_new("Name :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show(label);

  label = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(label),dbbrowser->selected_scheme_proc_name);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;

  /* show the description */

  label = gtk_label_new("Blurb :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  label = gtk_label_new(dbbrowser->selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;

  label = gtk_hseparator_new(); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 4, row, row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show(label);
  row++;

  /* in parameters */
  if (dbbrowser->selected_nparams) 
    {
      label = gtk_label_new("In :");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
	   0, 1, row, row+(dbbrowser->selected_nparams), 
	   GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show(label);
      for (i=0;i<(dbbrowser->selected_nparams);i++) 
	{
	  /* name */
	  label = gtk_label_new((dbbrowser->selected_params[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    1, 2, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);

	  /* type */
	  label = gtk_label_new(GParamType2char((dbbrowser->selected_params[i]).type));
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    2, 3, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);

	  /* description */
	  label = gtk_label_new((dbbrowser->selected_params[i]).description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.0);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);
	  
	  row++;
	}
    }
  
  if ((dbbrowser->selected_nparams) && 
      (dbbrowser->selected_nreturn_vals)) {
    label = gtk_hseparator_new(); /* ok, not really a label ... :) */
    gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		      0, 4, row, row+1,
		      GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show( label );
    row++;
  }

  /* out parameters */
  if (dbbrowser->selected_nreturn_vals)
    {
      label = gtk_label_new("Out :");
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			0, 1, row, row+(dbbrowser->selected_nreturn_vals), 
			GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show(label);
      for (i=0;i<(dbbrowser->selected_nreturn_vals);i++) 
	{
	  /* name */
	  label = gtk_label_new((dbbrowser->selected_return_vals[i]).name);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    1, 2, row, row+1, 
			    GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);

	  /* type */
	  label = gtk_label_new(GParamType2char((dbbrowser->selected_return_vals[i]).type));
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    2, 3, row, row+1, 
			    GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);

	  /* description */
	  label = gtk_label_new((dbbrowser->selected_return_vals[i]).description);
	  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5); 
	  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			    3, 4, row, row+1, 
			    GTK_FILL, GTK_FILL, 0, 0);
	  gtk_widget_show(label);
	  row++;

	}
    }

  /* show the author & the copyright */

  if ((dbbrowser->selected_nparams) || 
      (dbbrowser->selected_nreturn_vals)) {
    label = gtk_hseparator_new(); /* ok, not really a label ... :) */
    gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		      0, 4, row, row+1,
		      GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show( label );
    row++;
  }

  label = gtk_label_new("Author :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, 
		    GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  label = gtk_label_new(dbbrowser->selected_proc_author);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4,  row, row+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;
  
  label = gtk_label_new("Date :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, 
		    GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  label = gtk_label_new(dbbrowser->selected_proc_date);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4,  row, row+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;

  label = gtk_label_new("Copyright :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, 
		    GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  label = gtk_label_new(dbbrowser->selected_proc_copyright);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4,  row, row+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;

  /* TODO
  label = gtk_label_new("Help :");
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, 
		    GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

     insert help here
     but gtk_text seems bugggggggyyy */

  if (old_table) gtk_widget_destroy(old_table);

  gtk_container_add (GTK_CONTAINER (dbbrowser->descr_scroll), 
		     dbbrowser->descr_table );
  gtk_widget_show(dbbrowser->descr_table);
}

static void
dialog_close_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;
  
  if (dbbrowser->apply_callback) {
    /* we are called by another application : just kill the dialog box */
    gtk_widget_hide(dbbrowser->dlg);
    gtk_widget_destroy(dbbrowser->dlg);
  } else {
    /* we are in the plug_in : kill the gtk application */
    gtk_widget_destroy(dbbrowser->dlg);
    gtk_main_quit ();
  }
}

static void 
dialog_apply_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;

  (dbbrowser->apply_callback)( dbbrowser->selected_proc_name,
			       dbbrowser->selected_scheme_proc_name,
			       dbbrowser->selected_proc_blurb,
			       dbbrowser->selected_proc_help,
			       dbbrowser->selected_proc_author,
			       dbbrowser->selected_proc_copyright,
			       dbbrowser->selected_proc_date,
			       dbbrowser->selected_proc_type,
			       dbbrowser->selected_nparams,
			       dbbrowser->selected_nreturn_vals,
			       dbbrowser->selected_params,
			       dbbrowser->selected_return_vals );
}


static void 
dialog_selection_free_filename (GtkWidget *widget,
				gpointer   client_data)
{
  g_return_if_fail (widget != NULL);

  g_free (gtk_object_get_user_data (GTK_OBJECT (widget)));
  gtk_object_set_user_data (GTK_OBJECT (widget), NULL);
}

static void 
dialog_search_callback (GtkWidget *widget, 
			gpointer   data)
     /* search in the whole db */
{
  char **proc_list;
  int num_procs;
  dbbrowser_t* dbbrowser = data;
  GList *procs_list = NULL;

  if (GTK_LIST(dbbrowser->list)->children)
    gtk_list_remove_items( GTK_LIST(dbbrowser->list), 
			   GTK_LIST(dbbrowser->list)->children);

  /* search */

  if ( widget == (dbbrowser->name_button) ) 
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    "DB Browser (by name - please wait)");
      gimp_query_database (gtk_entry_get_text( GTK_ENTRY(dbbrowser->search_entry) ),
			   ".*", ".*", ".*", ".*", ".*", ".*", 
			   &num_procs, &proc_list);
    }
  else if ( widget == (dbbrowser->blurb_button) )
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    "DB Browser (by blurb - please wait)");
      gimp_query_database (".*", 
			   gtk_entry_get_text( GTK_ENTRY(dbbrowser->search_entry) ),
			   ".*", ".*", ".*", ".*", ".*",
			   &num_procs, &proc_list);
    }
  else {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    "DB Browser (please wait)");
      gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			   &num_procs, &proc_list);
  }

  if (num_procs != 0) {
    GtkWidget* list_item;
    gchar *insert_name, *label_name;
    int i,j,savej;
    
    for (i = 0; i < num_procs ; i++) {

      /* sort the list step by step */
      
      insert_name=g_strdup(proc_list[0]); savej=0;
      for (j = 0; j < num_procs ; j++) {
	if (strcmp(proc_list[j],insert_name)<0) {
	  g_free(insert_name);
	  insert_name=g_strdup(proc_list[j]);
	  savej=j;
	}
      }
      
      proc_list[savej][0]='\255';

      label_name = g_strdup( insert_name );
      convert_string( label_name );

      list_item = gtk_list_item_new_with_label ( label_name );
      gtk_object_set_user_data (GTK_OBJECT (list_item), insert_name );
      gtk_widget_show ( list_item );

      procs_list = g_list_append (procs_list, list_item );

      if (i==0) dialog_select( dbbrowser , insert_name );

      g_free(label_name);
    }
  }
  
  if ( dbbrowser->list ) {
    gtk_container_foreach (GTK_CONTAINER (dbbrowser->list),
			   dialog_selection_free_filename, NULL);
    gtk_list_clear_items (GTK_LIST (dbbrowser->list), 0, -1);
    
    if (procs_list)
      gtk_list_append_items (GTK_LIST (dbbrowser->list), 
			     procs_list);
  }
  
  g_free( proc_list );

  gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			"DB Browser");

}

/* utils ... */

static void 
convert_string (char *str)
{
  while (*str)
    {
      if (*str == '_') *str = '-';
      str++;
    }
}

static char* 
GParamType2char(GParamType t)
{
  switch (t) {
  case PARAM_INT32: return "INT32";
  case PARAM_INT16: return "INT16";
  case PARAM_INT8: return "INT8";
  case PARAM_FLOAT: return "FLOAT";
  case PARAM_STRING: return "STRING";
  case PARAM_INT32ARRAY: return "INT32ARRAY";
  case PARAM_INT16ARRAY: return "INT16ARRAY";
  case PARAM_INT8ARRAY: return "INT8ARRAY";
  case PARAM_FLOATARRAY: return "FLOATARRAY";
  case PARAM_STRINGARRAY: return "STRINGARRAY";
  case PARAM_COLOR: return "COLOR";
  case PARAM_REGION: return "REGION";
  case PARAM_DISPLAY: return "DISPLAY";
  case PARAM_IMAGE: return "IMAGE";
  case PARAM_LAYER: return "LAYER";
  case PARAM_CHANNEL: return "CHANNEL";
  case PARAM_DRAWABLE: return "DRAWABLE";
  case PARAM_SELECTION: return "SELECTION";
  case PARAM_BOUNDARY: return "BOUNDARY";
  case PARAM_PATH: return "PATH";
  case PARAM_STATUS: return "STATUS";
  case PARAM_END: return "END";
  default: return "UNKNOWN?";
  }
}
