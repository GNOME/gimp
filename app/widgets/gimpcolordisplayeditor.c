/* The GIMP -- an image manipulation program
 * Copyright (C) 1999 Manish Singh
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

#include "config.h"

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "core/gimpimage.h"

#include "widgets/gimpeditor.h"

#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-filter.h"
#include "gimpdisplayshell-filter-dialog.h"

#include "libgimp/gimpintl.h"


#define LIST_WIDTH  150
#define LIST_HEIGHT 100

#define UPDATE_DISPLAY(shell) G_STMT_START      \
{	                                        \
  gimp_display_shell_expose_full (shell);       \
  gimp_display_shell_flush (shell);             \
} G_STMT_END  


typedef struct _ColorDisplayDialog ColorDisplayDialog;

struct _ColorDisplayDialog
{
  GimpDisplayShell *shell;

  GtkWidget *dialog;

  GtkWidget *src;
  GtkWidget *dest;

  gint src_row;
  gint dest_row;

  gboolean modified;

  GList *old_nodes;
  GList *conf_nodes;

  GtkWidget *add_button;
  GtkWidget *remove_button;
  GtkWidget *up_button;
  GtkWidget *down_button;
  GtkWidget *configure_button;
};


static void   make_dialog                      (ColorDisplayDialog *cdd);

static void   color_display_ok_callback        (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_cancel_callback    (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_add_callback       (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_remove_callback    (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_up_callback        (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_down_callback      (GtkWidget          *widget,
                                                gpointer            data);
static void   color_display_configure_callback (GtkWidget          *widget,
                                                gpointer            data);

static void   src_list_populate                (const char         *name,
                                                gpointer            data);
static void   dest_list_populate               (GList              *node_list,
                                                GtkWidget          *dest);

static void   select_src                       (GtkWidget          *widget,
                                                gint                row,
                                                gint                column,
                                                GdkEventButton     *event,
                                                gpointer            data);
static void   unselect_src                     (GtkWidget          *widget,
                                                gint                row,
                                                gint                column,
                                                GdkEventButton     *event,
                                                gpointer            data);
static void   select_dest                      (GtkWidget          *widget,
                                                gint                row,
                                                gint                column,
                                                GdkEventButton     *event,
                                                gpointer            data);
static void   unselect_dest                    (GtkWidget          *widget,
                                                gint                row,
                                                gint                column,
                                                GdkEventButton     *event,
                                                gpointer            data);

static void   color_display_update_up_and_down (ColorDisplayDialog *cdd);


static void
make_dialog (ColorDisplayDialog *cdd)
{
  GtkWidget *hbox;
  GtkWidget *editor;
  GtkWidget *scrolled_win;
  GtkWidget *vbox;
  GtkWidget *image;
  gchar     *titles[2];

  cdd->dialog = gimp_dialog_new (_("Color Display Filters"), "display_filters",
                                 gimp_standard_help_func,
                                 "dialogs/display_filters/display_filters.html",
                                 GTK_WIN_POS_NONE,
                                 FALSE, TRUE, FALSE,

                                 GTK_STOCK_CANCEL, color_display_cancel_callback,
                                 cdd, NULL, NULL, FALSE, TRUE,

                                 GTK_STOCK_OK, color_display_ok_callback,
                                 cdd, NULL, NULL, TRUE, FALSE,

                                 NULL);

  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cdd->dialog)->vbox), hbox,
		      TRUE, TRUE, 0);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  titles[0] = _("Available Filters");
  titles[1] = NULL;
  cdd->src = gtk_clist_new_with_titles (1, titles);
  gtk_widget_set_size_request (cdd->src, LIST_WIDTH, LIST_HEIGHT);
  gtk_clist_column_titles_passive (GTK_CLIST (cdd->src));
  gtk_clist_set_auto_sort (GTK_CLIST (cdd->src), TRUE);
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd->src);
  gtk_widget_show (cdd->src);

  g_signal_connect (G_OBJECT (cdd->src), "select_row",
                    G_CALLBACK (select_src),
                    cdd);
  g_signal_connect (G_OBJECT (cdd->src), "unselect_row",
                    G_CALLBACK (unselect_src),
                    cdd);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  cdd->add_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), cdd->add_button, FALSE, FALSE, 16);
  gtk_widget_set_sensitive (cdd->add_button, FALSE);
  gtk_widget_show (cdd->add_button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (cdd->add_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (cdd->add_button,
                           _("Add the selected filter to the list of "
                             "active filters."), NULL);

  g_signal_connect (G_OBJECT (cdd->add_button), "clicked",
                    G_CALLBACK (color_display_add_callback),
                    cdd);

  cdd->remove_button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (vbox), cdd->remove_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (cdd->remove_button, FALSE);
  gtk_widget_show (cdd->remove_button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (cdd->remove_button), image);
  gtk_widget_show (image);

  gimp_help_set_help_data (cdd->remove_button,
                           _("Remove the selected filter from the list of "
                             "active filters."), NULL);

  g_signal_connect (G_OBJECT (cdd->remove_button), "clicked",
                    G_CALLBACK (color_display_remove_callback),
                    cdd);

  editor = gimp_editor_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  cdd->up_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_GO_UP,
                            _("Move the selected filter up"),
                            NULL,
                            G_CALLBACK (color_display_up_callback),
                            NULL,
                            cdd);

  cdd->down_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_GO_DOWN,
                            _("Move the selected filter down"),
                            NULL,
                            G_CALLBACK (color_display_down_callback),
                            NULL,
                            cdd);

  cdd->configure_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GIMP_STOCK_EDIT,
                            _("Configure the selected filter"),
                            NULL,
                            G_CALLBACK (color_display_configure_callback),
                            NULL,
                            cdd);

  gtk_widget_set_sensitive (cdd->up_button,        FALSE);
  gtk_widget_set_sensitive (cdd->down_button,      FALSE);
  gtk_widget_set_sensitive (cdd->configure_button, FALSE);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (editor), scrolled_win);
  gtk_widget_show (scrolled_win);

  titles[0] = _("Active Filters");
  titles[1] = NULL;
  cdd->dest = gtk_clist_new_with_titles (1, titles);
  gtk_widget_set_size_request (cdd->dest, LIST_WIDTH, LIST_HEIGHT);
  gtk_clist_column_titles_passive (GTK_CLIST (cdd->dest));
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd->dest);
  gtk_widget_show (cdd->dest);

  g_signal_connect (G_OBJECT (cdd->dest), "select_row",
                    G_CALLBACK (select_dest),
                    cdd);
  g_signal_connect (G_OBJECT (cdd->dest), "unselect_row",
                    G_CALLBACK (unselect_dest),
                    cdd);

  gtk_widget_show (hbox);
}

static void
color_display_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  GList              *list;

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
  shell->filters_dialog = NULL;

  if (cdd->modified)
    {
      for (list = cdd->old_nodes; list; list = g_list_next (list))
	{
	  if (! g_list_find (shell->filters, list->data))
	    gimp_display_shell_filter_detach_destroy (shell, list->data);
	}

      g_list_free (cdd->old_nodes);

      UPDATE_DISPLAY (shell);
    }
}

static void
color_display_cancel_callback (GtkWidget *widget,
			       gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  GList *list;
  GList *next;

  gtk_widget_destroy (GTK_WIDGET (cdd->dialog));
  shell->filters_dialog = NULL;
  
  if (cdd->modified)
    {
      list = shell->filters;
      shell->filters = cdd->old_nodes;

      while (list)
	{
	  next = list->next;

	  if (! g_list_find (cdd->old_nodes, list->data))
	    gimp_display_shell_filter_detach_destroy (shell, list->data);

	  list = next;
	}

      UPDATE_DISPLAY (shell);
    }
}

static void 
color_display_update_up_and_down (ColorDisplayDialog *cdd)
{
  gtk_widget_set_sensitive (cdd->up_button, cdd->dest_row > 0);
  gtk_widget_set_sensitive (cdd->down_button,
                            cdd->dest_row >= 0 &&
                            cdd->dest_row < GTK_CLIST (cdd->dest)->rows - 1);
}

static void
color_display_add_callback (GtkWidget *widget,
			    gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  gchar              *name  = NULL;
  ColorDisplayNode   *node;
  gint                row;

  if (cdd->src_row < 0)
    return;

  gtk_clist_get_text (GTK_CLIST (cdd->src), cdd->src_row, 0, &name);

  if (!name)
    return;

  cdd->modified = TRUE;

  node = gimp_display_shell_filter_attach (shell, name);

  row = gtk_clist_append (GTK_CLIST (cdd->dest), &name);
  gtk_clist_set_row_data (GTK_CLIST (cdd->dest), row, node);

  color_display_update_up_and_down (cdd);

  UPDATE_DISPLAY (shell);
}

static void
color_display_remove_callback (GtkWidget *widget,
			       gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  ColorDisplayNode   *node;

  if (cdd->dest_row < 0)
    return;
  
  node = gtk_clist_get_row_data (GTK_CLIST (cdd->dest), cdd->dest_row);
  gtk_clist_remove (GTK_CLIST (cdd->dest), cdd->dest_row);

  cdd->modified = TRUE;

  if (g_list_find (cdd->old_nodes, node))
    gimp_display_shell_filter_detach (shell, node);
  else
    gimp_display_shell_filter_detach_destroy (shell, node);

  cdd->dest_row = -1;

  color_display_update_up_and_down (cdd);
  
  UPDATE_DISPLAY (shell);
}

static void
color_display_up_callback (GtkWidget *widget,
			   gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  ColorDisplayNode   *node;

  if (cdd->dest_row < 1)
    return;

  node = gtk_clist_get_row_data (GTK_CLIST (cdd->dest), cdd->dest_row);
  gtk_clist_row_move (GTK_CLIST (cdd->dest), cdd->dest_row, cdd->dest_row - 1);

  gimp_display_shell_filter_reorder_up (shell, node);
  cdd->modified = TRUE;

  cdd->dest_row--;

  color_display_update_up_and_down (cdd);

  UPDATE_DISPLAY (shell);
}

static void
color_display_down_callback (GtkWidget *widget,
			     gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  ColorDisplayNode   *node;

  if (cdd->dest_row < 0)
    return;

  if (cdd->dest_row >= GTK_CLIST (cdd->dest)->rows -1)
    return;

  node = gtk_clist_get_row_data (GTK_CLIST (cdd->dest), cdd->dest_row);
  gtk_clist_row_move (GTK_CLIST (cdd->dest), cdd->dest_row, cdd->dest_row + 1);

  gimp_display_shell_filter_reorder_down (shell, node);
  cdd->modified = TRUE;

  cdd->dest_row++;

  color_display_update_up_and_down (cdd);

  UPDATE_DISPLAY (shell);
}

static void
color_display_configure_callback (GtkWidget *widget,
				  gpointer   data)
{
  ColorDisplayDialog *cdd   = data;
  GimpDisplayShell   *shell = cdd->shell;
  ColorDisplayNode   *node;

  if (cdd->dest_row < 0)
    return;

  node = gtk_clist_get_row_data (GTK_CLIST (cdd->dest), cdd->dest_row);

  if (! g_list_find (cdd->conf_nodes, node))
    cdd->conf_nodes = g_list_append (cdd->conf_nodes, node);

  gimp_display_shell_filter_configure (node, NULL, NULL, NULL, NULL);

  cdd->modified = TRUE;

  UPDATE_DISPLAY (shell);
}

void
gimp_display_shell_filter_dialog_new (GimpDisplayShell *shell)
{
  ColorDisplayDialog *cdd;

  cdd = g_new0 (ColorDisplayDialog, 1);

  make_dialog (cdd);

  gtk_clist_clear (GTK_CLIST (cdd->src));
  gtk_clist_clear (GTK_CLIST (cdd->dest));

  color_display_foreach (src_list_populate, cdd->src);

  cdd->old_nodes = shell->filters;
  dest_list_populate (shell->filters, cdd->dest);
  shell->filters = g_list_copy (cdd->old_nodes);

  cdd->shell = shell;

  cdd->src_row = -1;
  cdd->dest_row = -1;

  shell->filters_dialog = cdd->dialog;
}

static void
src_list_populate (const char *name,
		   gpointer    data)
{
  gtk_clist_append (GTK_CLIST (data), (gchar **) &name);
}

static void
dest_list_populate (GList     *node_list,
    		    GtkWidget *dest)
{
  ColorDisplayNode *node;
  int               row;

  while (node_list)
    {
      node = node_list->data;

      row = gtk_clist_append (GTK_CLIST (dest), &node->cd_name);
      gtk_clist_set_row_data (GTK_CLIST (dest), row, node);

      node_list = node_list->next;
    }
}

static void
select_src (GtkWidget       *widget,
	    gint             row,
	    gint             column,
	    GdkEventButton  *event,
	    gpointer         data)
{
  ColorDisplayDialog *cdd = data;

  cdd->src_row = row;

  gtk_widget_set_sensitive (cdd->add_button, TRUE);
}

static void
unselect_src (GtkWidget      *widget,
              gint            row,
              gint            column,
              GdkEventButton *event,
              gpointer        data)
{
  ColorDisplayDialog *cdd = data;

  cdd->src_row = -1;

  gtk_widget_set_sensitive (cdd->add_button, FALSE);
}

static void
select_dest (GtkWidget       *widget,
	     gint             row,
	     gint             column,
	     GdkEventButton  *event,
	     gpointer         data)
{
  ColorDisplayDialog *cdd = data;

  cdd->dest_row = row;

  gtk_widget_set_sensitive (cdd->remove_button,    TRUE);
  gtk_widget_set_sensitive (cdd->configure_button, TRUE);

  color_display_update_up_and_down (cdd);
}

static void
unselect_dest (GtkWidget      *widget,
               gint            row,
               gint            column,
               GdkEventButton *event,
               gpointer        data)
{
  ColorDisplayDialog *cdd = data;

  cdd->dest_row = -1;

  gtk_widget_set_sensitive (cdd->remove_button,    FALSE);
  gtk_widget_set_sensitive (cdd->configure_button, FALSE);

  color_display_update_up_and_down (cdd);
}

