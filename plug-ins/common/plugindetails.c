/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for the GIMP.
 *
 * Copyright (C) 1999 Andy Thomas  alt@picnic.demon.co.uk
 *
 * Note some portions of th UI comes from the dbbrowser plugin.
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define DBL_LIST_WIDTH  250
#define DBL_WIDTH       (DBL_LIST_WIDTH + 300)
#define DBL_HEIGHT      200

static gchar *proc_type_str[] =
{
  N_("Internal GIMP procedure"),
  N_("GIMP Plug-In"),
  N_("GIMP Extension"),
  N_("Temporary Procedure")
};


/* Declare some local functions.
 */
static void   query      (void);
static void   run        (gchar   *name,
                          gint     nparams,
                          GParam  *param,
                          gint    *nreturn_vals,
                          GParam **return_vals);

static GtkWidget * gimp_plugin_desc (void);

static gint procedure_clist_select_callback (GtkWidget      *widget,
					     gint            row, 
					     gint            column, 
					     GdkEventButton *bevent,
					     gpointer        data);
static gint procedure_ctree_select_callback (GtkWidget      *widget,
					     GtkWidget      *row, 
					     gint            column, 
					     gpointer        data);

GPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};


MAIN ()

static void
query (void)
{
  static GParamDef args[] =
  {
    { PARAM_INT32, "run_mode", "Interactive, [non-interactive]" }
  };
  static gint nargs = sizeof (args) / sizeof (args[0]);

  gimp_install_procedure ("plug_in_details",
                          "Displays plugin details",
                          "Helps browse the plugin menus system. You can search for plugin names, sort by name or menu location and you can view a tree representation of the plugin menus. Can also be of help to find where new plugins have installed themselves in the menuing system",
                          "Andy Thomas",
                          "Andy Thomas",
                          "1999",
			  N_("<Toolbox>/Xtns/Plugin Details..."),
			  "",
                          PROC_EXTENSION,
			  nargs, 0,
                          args, NULL);
}

static void
run (gchar   *name,
     gint     nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam **return_vals)
{
  static GParam  values[2];
  GRunModeType   run_mode;
  GtkWidget     *plugin_dialog;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals  = values;
  values[0].type          = PARAM_STATUS;
  values[0].data.d_status = STATUS_CALLING_ERROR;

  INIT_I18N_UI();

  if (strcmp (name, "plug_in_details") == 0)
    {
      *nreturn_vals = 1;

      values[0].data.d_status = STATUS_SUCCESS;

      plugin_dialog = gimp_plugin_desc ();

      gtk_main ();
      gdk_flush ();
    }
}


typedef struct  
{
  GtkWidget *dlg;
  GtkWidget *clist;
  GtkWidget *ctree;
  GtkWidget *search_entry;
  GtkWidget *descr_scroll;
  GtkWidget *name_button;
  GtkWidget *blurb_button;
  GtkWidget *scrolled_win;
  GtkWidget *ctree_scrolled_win;
  GtkWidget *info_table;
  GtkWidget *paned;
  GtkWidget *left_paned;
  GtkWidget *info_align;
  gint       num_plugins;
  gint       ctree_row;
  gint       clist_row;
  gint       c1size;
  gboolean   details_showing;
} PDesc;

PDesc *plugindesc = NULL;

typedef struct
{
  gchar *menu;
  gchar *accel;
  gchar *prog;
  gchar *types;
  gchar *realname;
  gint  instime;
} PInfo;

static void
dialog_close_callback (GtkWidget *widget, 
		       gpointer   data)
     /* end of the dialog */
{
  PDesc *pdesc = data;
  gtk_widget_destroy (pdesc->dlg);
  gtk_main_quit ();
}

/* Bit of a fiddle but sorta has the effect I want... */

static void
details_callback (GtkWidget *widget, 
		  gpointer   data)
{
  /* Show or hide the details window */ 
  PDesc *pdesc = data;
  GtkLabel *lab = GTK_LABEL (GTK_BIN (widget)->child);

  /* This is a lame hack: 
     We add the description on the right on the first details_callback.
     Otherwise the window reacts quite weird on resizes */
  if (pdesc->descr_scroll == NULL)
    {
      pdesc->descr_scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pdesc->descr_scroll),
				      GTK_POLICY_ALWAYS, 
				      GTK_POLICY_ALWAYS);
      gtk_widget_set_usize (pdesc->descr_scroll, DBL_WIDTH - DBL_LIST_WIDTH, -1);
      gtk_paned_pack2 (GTK_PANED (pdesc->paned), pdesc->descr_scroll,
		       FALSE, TRUE);
      gtk_clist_select_row (GTK_CLIST(pdesc->clist), pdesc->clist_row, -1);
    }

  if (pdesc->details_showing == FALSE)
    {
      GTK_PANED (pdesc->paned)->child1_resize = FALSE;
      gtk_paned_set_handle_size (GTK_PANED (pdesc->paned), 10);
      gtk_paned_set_gutter_size (GTK_PANED (pdesc->paned), 6);
      gtk_label_set_text (lab, _("Details <<"));
      gtk_widget_show (pdesc->descr_scroll);
      pdesc->details_showing = TRUE;
    }
  else
    {
      GtkWidget *p = GTK_WIDGET (pdesc->paned)->parent;
      GTK_PANED (pdesc->paned)->child1_resize = TRUE;
      GTK_PANED (pdesc->paned)->child2_resize = TRUE;
      gtk_paned_set_handle_size (GTK_PANED (pdesc->paned), 0);
      gtk_paned_set_gutter_size (GTK_PANED (pdesc->paned), 0);
      gtk_label_set_text (lab, _("Details >>"));
      gtk_widget_hide (pdesc->descr_scroll);
      gtk_paned_set_position (GTK_PANED (pdesc->paned),
			      p->allocation.width);/*plugindesc->c1size);*/
      pdesc->details_showing = FALSE;
    }
}

static gchar *
format_menu_path (gchar *s)
{
  gchar ** str_array;
  gchar *newstr = NULL;

  if (!s)
    return s;

  str_array = g_strsplit (s,"/",0);

  newstr = g_strjoinv ("->", str_array);

  g_strfreev (str_array);

  return newstr;
}

static gint
procedure_general_select_callback (PDesc *pdesc,
				   PInfo *pinfo)
{
  gchar *selected_proc_blurb;
  gchar *selected_proc_help; 
  gchar *selected_proc_author;
  gchar *selected_proc_copyright;
  gchar *selected_proc_date;
  gint   selected_proc_type; 
  gint   selected_nparams;
  gint   selected_nreturn_vals;
  GParamDef *selected_params;
  GParamDef *selected_return_vals;
  GtkWidget *label;
  GtkWidget *help;
  GtkWidget *text;
  GtkWidget *vscrollbar;
  GtkWidget *old_table;
  GtkWidget *old_align;
  gint       table_row = 0;
  gchar     *str;

  g_return_val_if_fail (pdesc != NULL, FALSE);
  g_return_val_if_fail (pinfo != NULL, FALSE);

  if (pdesc->descr_scroll == NULL)
    return FALSE;

  selected_proc_blurb = NULL;
  selected_proc_help = NULL;
  selected_proc_author = NULL;
  selected_proc_copyright = NULL;
  selected_proc_date = NULL;
  selected_proc_type = 0;
  selected_nparams = 0;
  selected_nreturn_vals = 0;
  selected_params = NULL;
  selected_return_vals = NULL;

  gimp_query_procedure (pinfo->realname, 
			&selected_proc_blurb, 
			&selected_proc_help, 
			&selected_proc_author,
			&selected_proc_copyright, 
			&selected_proc_date, 
			&selected_proc_type, 
			&selected_nparams,
			&selected_nreturn_vals, 
			&selected_params, 
			&selected_return_vals);   

  old_table = pdesc->info_table;
  old_align = pdesc->info_align;

  pdesc->info_table = gtk_table_new (10, 5, FALSE);
  pdesc->info_align = gtk_alignment_new (0.5, 0.5, 0, 0);

  gtk_table_set_col_spacings (GTK_TABLE (pdesc->info_table), 3);

  /* Number of plugins */

  str = g_strdup_printf (_(" Number of Plugin Interfaces: %d"),
			 pdesc->num_plugins);
  label = gtk_label_new (str);
  g_free (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* menu path */

  label = gtk_label_new (_("Menu Path:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show(label);

  label = gtk_label_new (format_menu_path (pinfo->menu));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* show the name */

  label = gtk_label_new (_("Name:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show(label);

  label = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (label), pinfo->realname);
  gtk_entry_set_editable (GTK_ENTRY (label), FALSE);
  gtk_table_attach (GTK_TABLE(pdesc->info_table), label,
		    1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* show the description */

  label = gtk_label_new (_("Blurb:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 0);
  gtk_widget_show (label);

  label = gtk_label_new (selected_proc_blurb);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show(label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* show the help */
  if ((selected_proc_help) && (strlen(selected_proc_help) > 1))
    {
      label = gtk_label_new (_("Help:"));
      gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
      gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
			0, 1, table_row, table_row+1, 
			GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show (label);
      
      help = gtk_table_new (2, 2, FALSE);
      gtk_table_set_row_spacing (GTK_TABLE (help), 0, 2);
      gtk_table_set_col_spacing (GTK_TABLE (help), 0, 2);
      gtk_table_attach (GTK_TABLE (pdesc->info_table), help,
			1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 0);
      gtk_widget_show (help);
      table_row++;
      
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
            
      label = gtk_hseparator_new (); /* ok, not really a label ... :) */
      gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
			0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
      gtk_widget_show(label);

      gtk_text_freeze (GTK_TEXT (text));
      gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
		       selected_proc_help, -1);
      gtk_text_thaw (GTK_TEXT (text));

      table_row++;
    }

  /* show the type */

  label = gtk_label_new (_("Type:"));
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5); 
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 1, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show(label);

  label = gtk_label_new (gettext (proc_type_str[selected_proc_type]));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    1, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  table_row++;

  label = gtk_hseparator_new (); /* ok, not really a label ... :) */
  gtk_table_attach (GTK_TABLE (pdesc->info_table), label,
		    0, 4, table_row, table_row+1, GTK_FILL, GTK_FILL, 3, 6);
  gtk_widget_show (label);
  table_row++;

  /* Remove old and replace with new */

  if (old_table)
    gtk_widget_destroy (old_table);

  if (old_align)
    gtk_widget_destroy (old_align);

  gtk_container_add (GTK_CONTAINER (pdesc->info_align),pdesc->info_table);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (pdesc->descr_scroll), 
					 pdesc->info_align);

  gtk_widget_show (pdesc->info_table);
  gtk_widget_show (pdesc->info_align);

  if (selected_proc_blurb)
    g_free (selected_proc_blurb);
  if (selected_proc_help)
    g_free (selected_proc_help); 
  if (selected_proc_author)
    g_free (selected_proc_author);
  if (selected_proc_copyright)
    g_free (selected_proc_copyright);
  if (selected_proc_date)
    g_free (selected_proc_date);
  if (selected_params)
    g_free (selected_params);
  if (selected_return_vals)
    g_free (selected_return_vals);
  
  return FALSE;
}

static void 
expand_to (PDesc        *pdesc,
	   GtkCTreeNode *parent)
{
  if(parent)
    {
      expand_to (pdesc, (GTK_CTREE_ROW (parent))->parent);
      gtk_ctree_expand (GTK_CTREE (pdesc->ctree), parent);      
    }
}

static gint
procedure_clist_select_callback (GtkWidget      *widget,
				 gint            row, 
				 gint            column, 
				 GdkEventButton *bevent,
				 gpointer        data)
{
  PInfo *pinfo;
  GtkCTreeNode * found_node; 
  PDesc *pdesc = data;

  g_return_val_if_fail (pdesc != NULL, FALSE);

  pinfo = (PInfo *) gtk_clist_get_row_data (GTK_CLIST (widget), row);

  if(!pinfo)
    return FALSE;

  /* Must select the correct one in the ctree structure */

  found_node = gtk_ctree_find_by_row_data (GTK_CTREE (pdesc->ctree),
					   NULL, pinfo);
  
  if (found_node)
    {
      GtkCTreeRow   *ctr;
      GtkCTreeNode  *parent;
      gint sel_row;

      /* Make sure this is expanded */

      ctr = GTK_CTREE_ROW (found_node);

      parent = GTK_CTREE_NODE (ctr->parent);

      expand_to (pdesc, parent);

      sel_row = gtk_clist_find_row_from_data (GTK_CLIST (pdesc->ctree), pinfo);

      gtk_widget_hide (pdesc->ctree); 
      gtk_widget_show (pdesc->ctree); 

      gtk_signal_handler_block_by_func (GTK_OBJECT(pdesc->ctree),
					(GtkSignalFunc)procedure_ctree_select_callback, pdesc);

      gtk_clist_select_row (GTK_CLIST (pdesc->ctree), sel_row, -1);  
      gtk_ctree_select (GTK_CTREE (pdesc->ctree), found_node);

      gtk_clist_moveto (GTK_CLIST (pdesc->ctree),
			sel_row,
			0,
			0.5, 0.5);

      gtk_signal_handler_unblock_by_func (GTK_OBJECT (pdesc->ctree),
					  (GtkSignalFunc)procedure_ctree_select_callback, pdesc);

      pdesc->ctree_row = sel_row;
    }
  else
    {
      g_warning ("Failed to find node in ctree");
    }

  return procedure_general_select_callback (pdesc, pinfo);
}

/* This was an attempt to get around a problem in gtk 
 * where the scroll windows that contain ctree and clist
 * refuse to respond to the moveto funcions unless the
 * widgets are visible.
 * Still did not work 100% even after this.
 * Note the scrollbars are in the correct position but the
 * widget needs to be redrawn at the correct location.
 */

static gint
page_select_callback (GtkNotebook     *notebook,
		      GtkNotebookPage *page,
		      guint            page_num,
		      gpointer         data)
{
  PDesc *pdesc = data;

  if (page_num == 0)
    {
      gtk_clist_select_row (GTK_CLIST (pdesc->clist), pdesc->clist_row, -1);  
      gtk_clist_moveto (GTK_CLIST (pdesc->clist),
			pdesc->clist_row,
			0,
			0.5, 0.0);
    }
  else
    {
      gtk_clist_select_row (GTK_CLIST (pdesc->ctree), pdesc->ctree_row, -1);  
      gtk_clist_moveto (GTK_CLIST (pdesc->ctree),
			pdesc->ctree_row,
			0,
			0.5, 0.0);
    }

  return FALSE;
}

static gint
procedure_ctree_select_callback (GtkWidget *widget,
				 GtkWidget *row, 
				 gint       column, 
				 gpointer   data)
{
  PInfo *pinfo;
  PDesc *pdesc;
  gboolean is_leaf;
  gint sel_row;

  /* row is not a leaf the we have no interest in it */

  gtk_ctree_get_node_info (GTK_CTREE (widget),
			   GTK_CTREE_NODE (row),
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   NULL,
			   &is_leaf,
			   NULL);

  if (!is_leaf)
    return FALSE;

  pdesc = data;

  pinfo = (PInfo *) gtk_ctree_node_get_row_data (GTK_CTREE (widget),
						 GTK_CTREE_NODE (row));

  /* Must set clist to this one */
  /* Block signals */

  gtk_signal_handler_block_by_func (GTK_OBJECT (pdesc->clist),
				    (GtkSignalFunc)procedure_clist_select_callback, pdesc);

  sel_row = gtk_clist_find_row_from_data (GTK_CLIST (pdesc->clist), pinfo);
  gtk_clist_select_row (GTK_CLIST (pdesc->clist), sel_row, -1);  
  gtk_clist_moveto (GTK_CLIST (pdesc->clist),
		    sel_row,
		    0,
		    0.5, 0.5);

  gtk_signal_handler_unblock_by_func (GTK_OBJECT (pdesc->clist),
				      (GtkSignalFunc)procedure_clist_select_callback, pdesc);

  pdesc->clist_row = sel_row;
  
  return procedure_general_select_callback (pdesc, pinfo);
}

static void
pinfo_free (gpointer p)
{
  PInfo *pinfo = p;

  g_free (pinfo->menu);
  g_free (pinfo->accel);
  g_free (pinfo->prog);
  g_free (pinfo->types);
  g_free (pinfo->realname);
  g_free (pinfo);
}

static GtkCTreeNode*
get_parent (PDesc       *pdesc,
	    GHashTable  *ghash,
	    gchar       *mpath)
{
  GtkCTreeNode *parent;
  GtkCTreeNode *last_parent;
  gchar *tmp_ptr;
  gchar *str_ptr;
  gchar *leaf_ptr;
  gchar *labels[3];
  
  if (mpath == NULL)
    return NULL; /* Parent is root */

  parent = g_hash_table_lookup (ghash, mpath);

  if (parent)
    {
      /* found node */
      return parent;
    }

  /* Next one up */
  tmp_ptr = g_strdup (mpath);

  str_ptr = strrchr (tmp_ptr,'/');

  if (str_ptr == NULL)
    {
      /*       printf("Root node for %s\n",mpath); */
      leaf_ptr = mpath;
      tmp_ptr = "<root>";
      last_parent = NULL;
    }
  else
    {
      leaf_ptr = g_strdup(str_ptr+1);

      *str_ptr = '\000';

      last_parent = get_parent (pdesc, ghash, tmp_ptr);
    }

  labels[0] = g_strdup (leaf_ptr); 
  labels[1] = g_strdup (""); 
  labels[2] = g_strdup (""); 

  /*   printf("get_parent::creating node %s under %s\n",leaf_ptr,tmp_ptr); */

  parent = gtk_ctree_insert_node (GTK_CTREE (pdesc->ctree),
				  last_parent,
				  NULL,
				  labels,
				  4,
				  NULL,
				  NULL,
				  NULL,
				  NULL,
				  FALSE,
				  FALSE);

  g_hash_table_insert (ghash, mpath, parent); 

  return parent;
}

static void
insert_into_ctree (PDesc      *pdesc,
		   gchar      *name,
		   gchar      *xtimestr,
		   gchar      *menu_str,
		   gchar      *types_str,
		   GHashTable *ghash,
		   PInfo      *pinfo)
{
  gchar *labels[3];
  gchar *str_ptr;
  gchar *tmp_ptr;
  gchar *leaf_ptr;
  GtkCTreeNode *parent = NULL;
  GtkCTreeNode *leaf_node = NULL;

  /* Find all nodes */
  /* Last one is the leaf part */

  tmp_ptr = g_strdup (menu_str);

  str_ptr = strrchr (tmp_ptr, '/');

  if (str_ptr == NULL)
    return; /* No node */

  leaf_ptr = g_strdup (str_ptr + 1);

  *str_ptr = '\000';

  /*   printf("inserting %s...\n",menu_str); */

  parent = get_parent (pdesc, ghash, tmp_ptr);

  /* Last was a leaf */
  /*   printf("found leaf %s parent = %p\n",leaf_ptr,parent); */
  labels[0] = g_strdup (name);
  labels[1] = g_strdup (xtimestr);
  labels[2] = g_strdup (types_str);

  leaf_node = gtk_ctree_insert_node (GTK_CTREE (pdesc->ctree),
				     parent,
				     NULL,
				     labels,
				     4,
				     NULL,
				     NULL,
				     NULL,
				     NULL,
				     TRUE,
				     FALSE);

  gtk_ctree_node_set_row_data (GTK_CTREE (pdesc->ctree), leaf_node, pinfo);
}

static void
get_plugin_info (PDesc *pdesc,
		 gchar *search_text)
{
  GParam *return_vals;
  gint nreturn_vals;
  gint  row_count = 0;
  gchar **menu_strs;
  gchar **accel_strs;
  gchar **prog_strs;
  gchar **types_strs;
  gchar **realname_strs;
  gint  *time_ints;

  GHashTable* ghash = g_hash_table_new (g_str_hash, g_str_equal);

  if (!search_text)
    search_text = "";

  return_vals = gimp_run_procedure ("gimp_plugins_query",
                                    &nreturn_vals,
				    PARAM_STRING,search_text,
                                    PARAM_END);

  if (return_vals[0].data.d_status == STATUS_SUCCESS)
    {
      int loop;
      pdesc->num_plugins = return_vals[1].data.d_int32;
      menu_strs = return_vals[2].data.d_stringarray;
      accel_strs = return_vals[4].data.d_stringarray;
      prog_strs = return_vals[6].data.d_stringarray;
      types_strs = return_vals[8].data.d_stringarray;
      time_ints = return_vals[10].data.d_int32array;
      realname_strs = return_vals[12].data.d_stringarray;

      for (loop = 0; loop < return_vals[1].data.d_int32; loop++)
	{
	  PInfo *pinfo;
	  gchar *labels[4];
	  gchar *name;
	  gchar xtimestr[50];
	  struct tm * x;
	  time_t tx;
	  int ret;

	  name = strrchr (menu_strs[loop], '/');

	  if (name)
	    name = name + 1;
	  else
	    name = menu_strs[loop];

	  pinfo = g_new0 (PInfo, 1);

	  tx = time_ints[loop];
	  if (tx)
	    {
	      x = localtime (&tx);
	      ret = strftime (xtimestr, sizeof (xtimestr), "%c", x);
	      xtimestr[ret] = 0;
	    }
	  else
	    strcpy (xtimestr,"");

	  pinfo->menu = g_strdup (menu_strs[loop]);
	  pinfo->accel = g_strdup (accel_strs[loop]);
	  pinfo->prog = g_strdup (prog_strs[loop]);
	  pinfo->types = g_strdup (types_strs[loop]);
	  pinfo->instime = time_ints[loop];
	  pinfo->realname = g_strdup (realname_strs[loop]);

	  labels[0] = g_strdup (name);
	  labels[1] = g_strdup (xtimestr);
	  labels[2] = g_strdup (menu_strs[loop]);
	  labels[3] = g_strdup (types_strs[loop]);

	  gtk_clist_insert (GTK_CLIST (pdesc->clist), row_count, labels);

	  gtk_clist_set_row_data_full (GTK_CLIST (pdesc->clist), row_count,
				       pinfo, pinfo_free);

	  row_count++;

	  /* Now do the tree view.... */
	  insert_into_ctree (pdesc,
			     name,
			     xtimestr,
			     menu_strs[loop],
			     types_strs[loop],
			     ghash,
			     pinfo);
	}
    }

  gimp_destroy_params (return_vals, nreturn_vals);
}

static void 
dialog_search_callback (GtkWidget *widget, 
			gpointer   data)
{
  PDesc *pdesc = data;
  gchar *search_text = NULL;
 
  if (widget != NULL)
    {
      /* The result of a button press... read entry data */
      search_text = gtk_entry_get_text (GTK_ENTRY (plugindesc->search_entry));
    }

  gtk_clist_freeze (GTK_CLIST (pdesc->ctree));
  gtk_clist_clear (GTK_CLIST (pdesc->ctree));
  gtk_clist_freeze (GTK_CLIST (pdesc->clist));
  gtk_clist_clear (GTK_CLIST (pdesc->clist));

  get_plugin_info (pdesc, search_text);

  gtk_clist_columns_autosize (GTK_CLIST (plugindesc->clist));

  gtk_clist_sort (GTK_CLIST (pdesc->clist));
  gtk_clist_thaw (GTK_CLIST (pdesc->clist));
  gtk_ctree_sort_recursive (GTK_CTREE (pdesc->ctree), NULL);
  gtk_clist_thaw (GTK_CLIST (pdesc->ctree));
}

static gint
date_sort (GtkCList      *clist,
	   gconstpointer  ptr1,
	   gconstpointer  ptr2)
{
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;

  /* Get the data for the row */
  PInfo *row1_pinfo = row1->data;
  PInfo *row2_pinfo = row2->data;

  /* Want to sort on the date field */

  if (row2_pinfo->instime < row1_pinfo->instime)
    {
      return -1;
    }

  if (row2_pinfo->instime > row1_pinfo->instime)
    {
      return 1;
    }

  return 0;
}

static void 
clist_click_column (GtkCList *clist, 
		    gint      column, 
		    gpointer  data)
{
  if (column == 1)
    {
      gtk_clist_set_compare_func (clist, date_sort);
    }
  else
    {
      gtk_clist_set_compare_func (clist, NULL); /* Set back to default */
    }

  if (column == clist->sort_column)
    {
      if (clist->sort_type == GTK_SORT_ASCENDING)
	clist->sort_type = GTK_SORT_DESCENDING;
      else
	clist->sort_type = GTK_SORT_ASCENDING;
    }
  else
    gtk_clist_set_sort_column (clist, column);
  
  gtk_clist_sort (clist);
}


static GtkWidget *
gimp_plugin_desc (void)
{
  GtkWidget  *button;
  GtkWidget  *hbox, *searchhbox, *vbox;
  GtkWidget  *label, *notebook, *swindow;
  gchar     **argv;
  gint        argc;
  gchar      *clabels[4];

  argc    = 1;
  argv    = g_new (gchar *, 1);
  argv[0] = g_strdup ("plugindetails");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());

  plugindesc = g_new0 (PDesc, 1);

  /* the dialog box */
  plugindesc->dlg =
    gimp_dialog_new (_("Plugin Descriptions"), "plugindetails",
		     gimp_plugin_help_func, "filters/plugindetails.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, TRUE,

		     _("Search by Name"), dialog_search_callback,
		     plugindesc, NULL, NULL, FALSE, FALSE,
		     _("Close"), dialog_close_callback,
		     plugindesc, NULL, NULL, TRUE, TRUE,

		     NULL);

  plugindesc->details_showing = FALSE;

  gtk_signal_connect (GTK_OBJECT (plugindesc->dlg), "destroy",
                      GTK_SIGNAL_FUNC (dialog_close_callback),
		      plugindesc);

  /* hbox : left=notebook ; right=description */
  
  plugindesc->paned = hbox = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plugindesc->dlg)->vbox), 
		      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  gtk_paned_set_handle_size (GTK_PANED (hbox), 0);
  gtk_paned_set_gutter_size (GTK_PANED (hbox), 0);

  /* left = vbox : the list and the search entry */
  
  plugindesc->left_paned = vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_border_width (GTK_CONTAINER (vbox), 3); 
  gtk_paned_pack1 (GTK_PANED (hbox), vbox, FALSE, FALSE);
  gtk_widget_show (vbox);

  /* left = notebook */

  notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  /* list : list in a scrolled_win */
  
  clabels[0] = g_strdup (_("Name")); 
  clabels[1] = g_strdup (_("Ins Date")); 
  clabels[2] = g_strdup (_("Menu Path")); 
  clabels[3] = g_strdup (_("Image Types")); 
  plugindesc->clist = gtk_clist_new_with_titles (4, clabels); 

  gtk_signal_connect (GTK_OBJECT (plugindesc->clist), "click_column",
		      (GtkSignalFunc) clist_click_column, NULL);
  gtk_clist_column_titles_show (GTK_CLIST (plugindesc->clist));
  swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_clist_set_selection_mode (GTK_CLIST (plugindesc->clist),
			        GTK_SELECTION_BROWSE);

  gtk_widget_set_usize (plugindesc->clist, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_signal_connect (GTK_OBJECT (plugindesc->clist), "select_row",
		      (GtkSignalFunc) procedure_clist_select_callback,
		      plugindesc);
  
  label = gtk_label_new (_("List View"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), plugindesc->clist);
  gtk_widget_show (plugindesc->clist);
  gtk_widget_show (swindow);

  /* notebook->ctree */
  clabels[0] = g_strdup (_("Menu Path/Name")); 
  clabels[1] = g_strdup (_("Ins Date")); 
  clabels[2] = g_strdup (_("Image Types")); 
  plugindesc->ctree = gtk_ctree_new_with_titles (3, 0, clabels);  
  plugindesc->ctree_scrolled_win =
    swindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize(plugindesc->ctree, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_signal_connect (GTK_OBJECT (plugindesc->ctree), "tree_select_row",
		      (GtkSignalFunc) procedure_ctree_select_callback,
		      plugindesc);
  label = gtk_label_new (_("Tree View"));
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 1, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (plugindesc->ctree), 2, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), swindow, label);
  gtk_container_add (GTK_CONTAINER (swindow), plugindesc->ctree);
  gtk_widget_show (plugindesc->ctree);
  gtk_widget_show (swindow);

  gtk_signal_connect_after (GTK_OBJECT (notebook), "switch_page",
			    (GtkSignalFunc) page_select_callback,
			    plugindesc);
  
  gtk_widget_show (notebook);

  /* search entry & details button */

  searchhbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox),
		      searchhbox, FALSE, FALSE, 0);
  gtk_widget_show (searchhbox);

  label = gtk_label_new (_("Search:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      label, FALSE, FALSE, 0);
  gtk_widget_show(label);

  plugindesc->search_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      plugindesc->search_entry, TRUE, TRUE, 0);
  gtk_widget_show (plugindesc->search_entry);

  button = gtk_button_new_with_label (_("Details >>"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_widget_show (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) details_callback, plugindesc);
  gtk_box_pack_start (GTK_BOX (searchhbox), 
		      button, FALSE, FALSE, 0);

  /* right = description */
  /* the right description is build on first click of the Details button */
 
  /* now build the list */
  dialog_search_callback (NULL, (gpointer) plugindesc);

  gtk_widget_show (plugindesc->clist); 
  gtk_widget_show (plugindesc->dlg);
  
  gtk_clist_select_row (GTK_CLIST (plugindesc->clist), 0, 0);
  gtk_clist_moveto (GTK_CLIST (plugindesc->clist), 0, 0, 0.0, 0.0);

  plugindesc->c1size = GTK_PANED (plugindesc->paned)->child1_size;
  GTK_PANED (plugindesc->paned)->child1_resize = TRUE;

  return plugindesc->dlg;
}
