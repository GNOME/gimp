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
#include "gdisplay.h"
#include "gdisplay_color.h"
#include "gdisplay_color_ui.h"
#include "gimpimageP.h"
#include "gimpui.h"
#include "libgimp/parasite.h"
#include "libgimp/gimpintl.h"
#include <gtk/gtk.h>

typedef struct _ColorDisplayDialog ColorDisplayDialog;

struct _ColorDisplayDialog
{
  GtkWidget *shell;

  GtkWidget *src;
  GtkWidget *dest;

  gint src_row;
  gint dest_row;

  gboolean modified;

  GList *old_nodes;
 
  GDisplay *gdisp;
};

static ColorDisplayDialog cdd = { NULL, NULL, NULL, -1, -1, FALSE, NULL, NULL };

typedef void (*ButtonCallback) (GtkWidget *, gpointer);

typedef struct _ButtonInfo ButtonInfo;

struct _ButtonInfo
{
  const gchar    *label;
  ButtonCallback  callback;
};

static void color_display_ok_callback        (GtkWidget *widget,
					      gpointer   data);
static void color_display_cancel_callback    (GtkWidget *widget,
					      gpointer   data);
static void color_display_add_callback       (GtkWidget *widget,
					      gpointer   data);
static void color_display_remove_callback    (GtkWidget *widget,
					      gpointer   data);
static void color_display_up_callback        (GtkWidget *widget,
					      gpointer   data);
static void color_display_down_callback      (GtkWidget *widget,
					      gpointer   data);
static void color_display_configure_callback (GtkWidget *widget,
					      gpointer   data);

static void src_list_populate  (const char *name,
				gpointer    data);
static void dest_list_populate (GList *node_list);

static void select_src  (GtkWidget       *widget,
			 gint             row,
			 gint             column,
			 GdkEventButton  *event,
			 gpointer         data);
static void select_dest (GtkWidget       *widget,
			 gint             row,
			 gint             column,
			 GdkEventButton  *event,
			 gpointer         data);

#define LIST_WIDTH  180
#define LIST_HEIGHT 180

static void
make_dialog (void)
{
  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkWidget *vbbox;
  char *titles[2];
  int i;

  static ButtonInfo buttons[] = 
  {
    { N_("Add"), color_display_add_callback },
    { N_("Remove"), color_display_remove_callback },
    { N_("Up"), color_display_up_callback },
    { N_("Down"), color_display_down_callback },
    { N_("Configure"), color_display_configure_callback }
  };

  cdd.shell = gimp_dialog_new (_("Color Display Filters"), "display_color",
			       gimp_standard_help_func,
			       "dialogs/display_filters/display_filters.html",
			       GTK_WIN_POS_NONE,
			       FALSE, TRUE, FALSE,

			       _("OK"), color_display_ok_callback,
			       NULL, NULL, TRUE, FALSE,
			       _("Cancel"), color_display_cancel_callback,
			       NULL, NULL, FALSE, TRUE,

			       NULL);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (cdd.shell)->vbox), hbox,
		      TRUE, TRUE, 4);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);

  titles[0] = _("Available Filters");
  titles[1] = NULL;
  cdd.src = gtk_clist_new_with_titles (1, titles);
  gtk_widget_set_usize (cdd.src, LIST_WIDTH, LIST_HEIGHT);
  gtk_clist_column_titles_passive (GTK_CLIST (cdd.src));
  gtk_clist_set_auto_sort (GTK_CLIST (cdd.src), TRUE);
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd.src);

  gtk_signal_connect (GTK_OBJECT (cdd.src), "select_row",
		      GTK_SIGNAL_FUNC (select_src),
		      NULL);

  vbbox = gtk_vbutton_box_new ();
  gtk_vbutton_box_set_layout_default (GTK_BUTTONBOX_START);
  gtk_box_pack_start (GTK_BOX (hbox), vbbox, FALSE, FALSE, 2);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);

  titles[0] = _("Active Filters");
  titles[1] = NULL;
  cdd.dest = gtk_clist_new_with_titles (1, titles);
  gtk_widget_set_usize (cdd.dest, LIST_WIDTH, LIST_HEIGHT);
  gtk_clist_column_titles_passive (GTK_CLIST (cdd.dest));
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd.dest);

  gtk_signal_connect (GTK_OBJECT (cdd.src), "select_row",
		      GTK_SIGNAL_FUNC (select_dest),
		      NULL);

  for (i = 0; i < 5; i++)
    {
       GtkWidget *button;

       button = gtk_button_new_with_label (gettext (buttons[i].label));
       gtk_box_pack_start (GTK_BOX (vbbox), button, FALSE, FALSE, 0);

       gtk_signal_connect (GTK_OBJECT (button), "clicked",
			   GTK_SIGNAL_FUNC (buttons[i].callback),
			   cdd.shell);
    }

  gtk_widget_show_all (hbox);
}

static void
color_display_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  GList *list;

  gtk_widget_hide (GTK_WIDGET (data));

  if (cdd.modified)
    {
      list = cdd.old_nodes;

      while (list)
	{
	  if (!g_list_find (cdd.gdisp->cd_list, list->data))
	    gdisplay_color_detach_destroy (cdd.gdisp, list->data);

	  list = list->next;
	}

      g_list_free (cdd.old_nodes);
    }

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_cancel_callback (GtkWidget *widget,
			       gpointer   data)
{
  GList *list;
  GList *next;

  gtk_widget_hide (GTK_WIDGET (data));
  
  if (cdd.modified)
    {
      list = cdd.gdisp->cd_list;
      cdd.gdisp->cd_list = cdd.old_nodes;

      while (list)
	{
	  next = list->next;

	  if (!g_list_find (cdd.old_nodes, list->data))
	    gdisplay_color_detach_destroy (cdd.gdisp, list->data);

	  list = next;
	}
    }

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_add_callback (GtkWidget *widget,
			    gpointer   data)
{
  gchar *name = NULL;
  ColorDisplayNode *node;
  gint row;

  if (cdd.src_row < 0)
    return;

  gtk_clist_get_text (GTK_CLIST (cdd.src), cdd.src_row, 0, &name);

  if (!name)
    return;

  cdd.modified = TRUE;

  node = gdisplay_color_attach (cdd.gdisp, name);

  row = gtk_clist_append (GTK_CLIST (cdd.dest), &name);
  gtk_clist_set_row_data (GTK_CLIST (cdd.dest), row, node);

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_remove_callback (GtkWidget *widget,
			       gpointer   data)
{
  ColorDisplayNode *node;

  if (cdd.dest_row < 0)
    return;
  
  node = (ColorDisplayNode *) gtk_clist_get_row_data (GTK_CLIST (cdd.dest),
						      cdd.dest_row);

  gtk_clist_remove (GTK_CLIST (cdd.dest), cdd.dest_row);

  cdd.modified = TRUE;

  if (g_list_find (cdd.old_nodes, node))
    gdisplay_color_detach (cdd.gdisp, node);
  else
    gdisplay_color_detach_destroy (cdd.gdisp, node);

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_up_callback (GtkWidget *widget,
			   gpointer   data)
{
  ColorDisplayNode *node;

  if (cdd.dest_row < 0)
    return;

  node = (ColorDisplayNode *) gtk_clist_get_row_data (GTK_CLIST (cdd.dest),
						      cdd.dest_row);

  gtk_clist_row_move (GTK_CLIST (cdd.dest), cdd.dest_row, cdd.dest_row - 1);

  gdisplay_color_reorder_up (cdd.gdisp, node);
  cdd.modified = TRUE;

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_down_callback (GtkWidget *widget,
			     gpointer   data)
{
  ColorDisplayNode *node;

  if (cdd.dest_row < 0)
    return;

  node = (ColorDisplayNode *) gtk_clist_get_row_data (GTK_CLIST (cdd.dest),
						      cdd.dest_row);
  gtk_clist_row_move (GTK_CLIST (cdd.dest), cdd.dest_row, cdd.dest_row + 1);

  gdisplay_color_reorder_down (cdd.gdisp, node);
  cdd.modified = TRUE;

  gdisplay_expose_full (cdd.gdisp);
  gdisplay_flush (cdd.gdisp);
}

static void
color_display_configure_callback (GtkWidget *widget,
				  gpointer   data)
{
}

void
gdisplay_color_ui (GDisplay *gdisp)
{
  if (!cdd.shell)
    make_dialog ();

  gtk_clist_clear (GTK_CLIST (cdd.src));
  gtk_clist_clear (GTK_CLIST (cdd.dest));

  color_display_foreach (src_list_populate, cdd.src);

  if (gdisp)
    {
      cdd.old_nodes = gdisp->cd_list;
      dest_list_populate (gdisp->cd_list);
      gdisp->cd_list = g_list_copy (cdd.old_nodes);
    }

  cdd.gdisp = gdisp;

  cdd.src_row = -1;
  cdd.dest_row = -1;

  /* gtk_clist_set_selectable (GTK_CLIST (cdd.src), 0, TRUE);
  gtk_clist_set_selectable (GTK_CLIST (cdd.dest), 0, TRUE); */

  gtk_widget_show (cdd.shell);
}

static void
src_list_populate (const char *name,
		   gpointer    data)
{
  gtk_clist_append (GTK_CLIST (data), (gchar **) &name);
}

static void
dest_list_populate (GList *node_list)
{
  ColorDisplayNode *node;
  int row;

  while (node_list)
    {
      node = (ColorDisplayNode *) node_list->data;

      row = gtk_clist_append (GTK_CLIST (cdd.dest), &node->cd_name);
      gtk_clist_set_row_data (GTK_CLIST (cdd.dest), row, node);

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
  cdd.src_row = row; 
}

static void
select_dest (GtkWidget       *widget,
	     gint             row,
	     gint             column,
	     GdkEventButton  *event,
	     gpointer         data)
{
  cdd.dest_row = row; 
}
