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
   gap_dbbrowser_utils.c (original code dbbrowser_utils.c by Thomas NOEL <thomas@minet.net>

   27. jan .1999   hof: update for GIMP 1.1.1 (show help)
   09. dec .1998   hof: update for GIMP 1.1
   12. jan .1998   hof: added "Gen Code" button
   
   23. dec .1997   hof: GAP variant of DBbrowser
                        removed apply_callback
                        added constraint_procedure, 
                        added 2 buttons
                        added return type
   
 
   0.08  26th sept 97  by Thomas NOEL <thomas@minet.net> )
*/

#include "config.h"
#include "libgimp/stdplugins-intl.h"
#include "gap_dbbrowser_utils.h"

extern int gap_debug;

int 
gap_db_browser_dialog(char *title_txt,
                      char *button_1_txt,
                      char *button_2_txt,
                      t_constraint_func        constraint_func,
                      t_constraint_func        constraint_func_sel1,
                      t_constraint_func        constraint_func_sel2,
                      t_gap_db_browse_result  *result,
		      gint                     init_gtk_flag)
  /* create the dialog box */
{
  gchar **l_argsv;
  gint    l_argsc;
  
  dbbrowser_t* dbbrowser;
  
  GtkWidget *button;
  GtkWidget *hbox,*searchhbox,*vbox;
  GtkWidget *label;
  
  l_argsc = 1;
  l_argsv = g_new (gchar *, 1);
  l_argsv[0] = g_strdup ( _("GAP Animated Filter apply"));

  if (init_gtk_flag)
  {
    /* gtk init (should be called only once in a plugin-process) */
    gtk_init (&l_argsc, &l_argsv);
  }

  dbbrowser = (gpointer)g_malloc(sizeof(dbbrowser_t));

  /* store pointers to gap constraint procedures */  
  dbbrowser->constraint_func      = constraint_func;
  dbbrowser->constraint_func_sel1 = constraint_func_sel1;
  dbbrowser->constraint_func_sel2 = constraint_func_sel2;
  dbbrowser->result  = result;
  dbbrowser->codegen_flag  = 0;   /* default: no code generation */
 
  /* the dialog box */
  
  dbbrowser->dlg = gtk_dialog_new ();
  
  gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), _("Animated Filter apply (init)"));
  gtk_window_position (GTK_WINDOW (dbbrowser->dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->dlg), "destroy",
                      (GtkSignalFunc) dialog_close_callback,
                      dbbrowser);
  
  /* label (fill 1. table_row) */
  label = gtk_label_new(title_txt);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

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
  
  dbbrowser->clist = gtk_clist_new(1);
  dbbrowser->scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (dbbrowser->scrolled_win),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_clist_set_selection_mode (GTK_CLIST (dbbrowser->clist),
			        GTK_SELECTION_BROWSE);

  gtk_widget_set_usize(dbbrowser->clist, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->clist), "select_row",
		      (GtkSignalFunc) procedure_select_callback,
		      dbbrowser);
  gtk_box_pack_start (GTK_BOX (vbox), dbbrowser->scrolled_win, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (dbbrowser->scrolled_win), dbbrowser->clist);
  gtk_widget_show(dbbrowser->clist);
  gtk_widget_show(dbbrowser->scrolled_win);

  /* search entry */

  searchhbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (vbox),
		      searchhbox, FALSE, TRUE, 0);
  gtk_widget_show(searchhbox);

  label = gtk_label_new( _("Search:"));
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


  if (gap_debug) {
    button = gtk_button_new_with_label ( _("Gen Code by name"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) dialog_button_3_callback, dbbrowser );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_widget_show (button);
  }

  dbbrowser->name_button = gtk_button_new_with_label ( _("Search by Name"));
  GTK_WIDGET_SET_FLAGS (dbbrowser->name_button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->name_button), "clicked",
                      (GtkSignalFunc) dialog_search_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		    dbbrowser->name_button , TRUE, TRUE, 0);
  gtk_widget_show(dbbrowser->name_button);

  dbbrowser->blurb_button = gtk_button_new_with_label ( _("Search by Blurb"));
  GTK_WIDGET_SET_FLAGS (dbbrowser->blurb_button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (dbbrowser->blurb_button), "clicked",
                      (GtkSignalFunc) dialog_search_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		    dbbrowser->blurb_button , TRUE, TRUE, 0);
  gtk_widget_show(dbbrowser->blurb_button);

  if (button_1_txt) {
    dbbrowser->app_const_button = gtk_button_new_with_label (button_1_txt);
    GTK_WIDGET_SET_FLAGS (dbbrowser->app_const_button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (dbbrowser->app_const_button), "clicked",
			(GtkSignalFunc) dialog_button_1_callback, dbbrowser );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
			dbbrowser->app_const_button, TRUE, TRUE, 0);
    gtk_widget_show (dbbrowser->app_const_button);
  } else dbbrowser->app_const_button = NULL;

  if (button_2_txt) {
    dbbrowser->app_vary_button = gtk_button_new_with_label (button_2_txt);
    GTK_WIDGET_SET_FLAGS (dbbrowser->app_vary_button, GTK_CAN_DEFAULT);
    gtk_signal_connect (GTK_OBJECT (dbbrowser->app_vary_button), "clicked",
			(GtkSignalFunc) dialog_button_2_callback, dbbrowser );
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
			dbbrowser->app_vary_button, TRUE, TRUE, 0);
    gtk_widget_show (dbbrowser->app_vary_button);
  } else dbbrowser->app_vary_button = NULL;

  button = gtk_button_new_with_label ( _("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) dialog_close_callback, dbbrowser);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dbbrowser->dlg)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_show (button);


  /* now build the list */

  gtk_widget_show (dbbrowser->clist); 
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

  dbbrowser->result->selected_proc_name[0] = '\0';
  dbbrowser->result->button_nr = -1;

  /* first search (all procedures) */
  dialog_search_callback( NULL, (gpointer)dbbrowser );
  gtk_main ();
  gdk_flush ();

  return  (dbbrowser->result->button_nr);
}

static gint
procedure_select_callback (GtkWidget *widget,
			   gint row, 
			   gint column, 
			   GdkEventButton * bevent,
			   gpointer data)
{
  dbbrowser_t *dbbrowser = data;
  gchar *func;

  g_return_val_if_fail (widget != NULL, FALSE);
  /*  g_return_val_if_fail (bevent != NULL, FALSE);  */
  g_return_val_if_fail (dbbrowser != NULL, FALSE);

  if ((func = (gchar *) (gtk_clist_get_row_data (GTK_CLIST (widget), row))))
      dialog_select (dbbrowser, func);
  return FALSE;
}


static void 
dialog_select (dbbrowser_t *dbbrowser, 
	       gchar       *proc_name)
  /* update the description box (right) */
{
  GtkWidget *label;
  GtkWidget *old_table;
  GtkWidget *help;
  GtkWidget *text = NULL;
  GtkWidget *vscrollbar;
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

  label = gtk_label_new( _("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    0, 1, row, row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show(label);

  label = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(label),dbbrowser->selected_scheme_proc_name);
  
  gtk_entry_set_editable(GTK_ENTRY(label), FALSE);
  gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		    1, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  row++;

  /* show the description */

  label = gtk_label_new( _("Blurb:"));
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
      label = gtk_label_new( _("In:"));
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
      label = gtk_label_new( _("Out:"));
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

  if ((dbbrowser->selected_nparams) || 
      (dbbrowser->selected_nreturn_vals)) {
    label = gtk_hseparator_new(); /* ok, not really a label ... :) */
    gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
		      0, 4, row, row+1,
		      GTK_FILL, GTK_FILL, 3, 6);
    gtk_widget_show( label );
    row++;
  }

  /* show the help */
  if ((dbbrowser->selected_proc_help) && (strlen(dbbrowser->selected_proc_help) > 1))
    {
      label = gtk_label_new( _("Help:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			0, 1, row, row+1, 
			GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show(label);
      
      help = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacing (GTK_TABLE (help), 0, 2);
      gtk_table_set_col_spacing (GTK_TABLE (help), 0, 2);
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), help,
			1, 4, row, row+1, GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show (help);
      row++;
      
      text = gtk_text_new (NULL, NULL);
      gtk_text_set_editable (GTK_TEXT (text), FALSE);
      gtk_text_set_word_wrap(GTK_TEXT(text), TRUE);
      gtk_widget_set_usize (text, -1, 60);
      gtk_table_attach (GTK_TABLE (help), text, 0, 1, 0, 1,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (text);
      
      vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text)->vadj);
      gtk_table_attach (GTK_TABLE (help), vscrollbar, 1, 2, 0, 1,
			GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (vscrollbar);
            
      label = gtk_hseparator_new(); /* ok, not really a label ... :) */
      gtk_table_attach (GTK_TABLE (dbbrowser->descr_table), label,
			0, 4, row, row+1, GTK_FILL, GTK_FILL, 3, 6);
      gtk_widget_show(label);
      row++;
    }

  /* show the author & the copyright */

  label = gtk_label_new( _("Author:"));
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
  
  label = gtk_label_new( _("Date:"));
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

  label = gtk_label_new( _("Copyright:"));
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


  if (old_table) gtk_widget_destroy(old_table);
 
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (dbbrowser->descr_scroll), 
					 dbbrowser->descr_table);

  /* now after the table is added to a window add the text */
  if (text != NULL)
    {
      gtk_widget_realize (text);
      gtk_text_freeze (GTK_TEXT (text));
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
		       dbbrowser->selected_proc_help, -1);
      gtk_text_thaw (GTK_TEXT (text));
    }
  
   		     
  gtk_widget_show(dbbrowser->descr_table);

  /* call constraint functions to check sensibility for the apply buttons */ 
  if(dbbrowser->app_const_button != NULL)
  {
     if(0 != (dbbrowser->constraint_func_sel1)(dbbrowser->selected_proc_name))
     { 
       gtk_widget_set_sensitive (dbbrowser->app_const_button, TRUE);
     }
     else 
     {
       gtk_widget_set_sensitive (dbbrowser->app_const_button, FALSE);
     }
  }
  if(dbbrowser->app_vary_button != NULL)
  {
     if(0 != (dbbrowser->constraint_func_sel2)(dbbrowser->selected_proc_name))
     { 
        gtk_widget_set_sensitive (dbbrowser->app_vary_button, TRUE);
     }
     else
     {
        gtk_widget_set_sensitive (dbbrowser->app_vary_button, FALSE);
     }
  }
}

static void
dialog_close_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;

    /* we are in the plug_in : kill the gtk application */
    gtk_widget_destroy(dbbrowser->dlg);
    gtk_main_quit ();
} 

static void 
dialog_button_1_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;

  if (dbbrowser->selected_proc_name==NULL) return;

  strcpy(dbbrowser->result->selected_proc_name, dbbrowser->selected_proc_name);
  dbbrowser->result->button_nr = 0;
  
  gtk_widget_hide(dbbrowser->dlg);
  gtk_widget_destroy(dbbrowser->dlg);
  gtk_main_quit ();
}

static void 
dialog_button_2_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;

  if (dbbrowser->selected_proc_name==NULL) return;

  strcpy(dbbrowser->result->selected_proc_name, dbbrowser->selected_proc_name);
  dbbrowser->result->button_nr = 1;
  
  gtk_widget_hide(dbbrowser->dlg);
  gtk_widget_destroy(dbbrowser->dlg);
  gtk_main_quit ();
}

static void 
dialog_button_3_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  dbbrowser_t* dbbrowser = data;

  p_remove_codegen_files();      /* remove old versions of generated CODE */
  dbbrowser->codegen_flag = 1;   /* let dialog_search_callback generate */
  dialog_search_callback(dbbrowser->name_button, (gpointer)dbbrowser );
  dbbrowser->codegen_flag = 0;
}

static void 
dialog_search_callback (GtkWidget *widget, 
			gpointer   data)
     /* search in the whole db */
{
  char **proc_list;
  int num_procs;
  int i, j;
  int i_added;
  dbbrowser_t* dbbrowser = data;
  gchar *func_name, *label, *query_text;
  GString *query;

  gtk_clist_freeze(GTK_CLIST(dbbrowser->clist));
  gtk_clist_clear(GTK_CLIST(dbbrowser->clist));

  /* search */

  if ( widget == (dbbrowser->name_button) ) 
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("Animated Filter apply (by name - please wait)"));

      query = g_string_new ("");
      query_text = gtk_entry_get_text(GTK_ENTRY(dbbrowser->search_entry));

      while (*query_text)
	{
	  if ((*query_text == '_') || (*query_text == '-'))
	    g_string_append (query, "[-_]");
	  else
	    g_string_append_c (query, *query_text);

	  query_text++;
	}

      gimp_query_database (query->str,
			   ".*", ".*", ".*", ".*", ".*", ".*", 
			   &num_procs, &proc_list);

      g_string_free (query, TRUE);
    }
  else if ( widget == (dbbrowser->blurb_button) )
    {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("Animated Filter apply (by blurb - please wait)"));
      gimp_query_database (".*", 
			   gtk_entry_get_text( GTK_ENTRY(dbbrowser->search_entry) ),
			   ".*", ".*", ".*", ".*", ".*",
			   &num_procs, &proc_list);
    }
  else {
      gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			    _("Animated Filter apply (please wait)"));
      gimp_query_database (".*", ".*", ".*", ".*", ".*", ".*", ".*", 
			   &num_procs, &proc_list);
  }

  i_added = 0;
  for (i = 0; i < num_procs; i++)
  {
    /* the filter constraint_function checks if the
     * PDB-proc has a typical interface to operate on a single drawable.
     * all other PDB-procedures are not listed in the GAP-dbbrowser
     */
    if(0 != (dbbrowser->constraint_func)((char *)proc_list[i]))
    {
        j = 0;
        while((j < i_added) &&
	     (strcmp(gtk_clist_get_row_data(GTK_CLIST(dbbrowser->clist), j),
		     proc_list[i]) < 0))
        {
           j++;
        }
        
        i_added++;
        label = g_strdup(proc_list[i]);
        if((dbbrowser->codegen_flag != 0) && (gap_debug))
        {
          p_gen_forward_iter_ALT(label);
          p_gen_tab_iter_ALT(label);
          p_gen_code_iter_ALT(label);
        }
        convert_string(label);
        gtk_clist_insert (GTK_CLIST (GTK_CLIST(dbbrowser->clist)), j,
                          &label);
        func_name = g_strdup (proc_list[i]);

        gtk_clist_set_row_data_full(GTK_CLIST(dbbrowser->clist), j,
                                    func_name, g_free);
    }
  }
 
  if ( dbbrowser->clist ) {
     ;
  }
  
  g_free( proc_list );

  gtk_window_set_title (GTK_WINDOW (dbbrowser->dlg), 
			_("Animated Filter apply"));

  gtk_clist_thaw(GTK_CLIST(dbbrowser->clist));

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
  case PARAM_PARASITE: return "PARASITE";
  case PARAM_STATUS: return "STATUS";
  case PARAM_END: return "END";
  default: return "UNKNOWN?";
  }
}
