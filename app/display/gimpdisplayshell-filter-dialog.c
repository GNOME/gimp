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

#include "actionarea.h"
#include "gdisplay.h"
#include "gdisplay_color.h"
#include "gdisplay_color_ui.h"
#include "gimpimageP.h"
#include "libgimp/parasite.h"
#include "libgimp/gimpintl.h"
#include <gtk/gtk.h>

typedef struct _ColorDisplayDialog ColorDisplayDialog;

struct _ColorDisplayDialog
{
  GtkWidget *shell;
  GtkWidget *src;
  GtkWidget *dest;
};

static ColorDisplayDialog cdd = { NULL, NULL, NULL };

typedef void (*ButtonCallback) (GtkWidget *, gpointer);

typedef struct _ButtonInfo ButtonInfo;

struct _ButtonInfo
{
  const gchar    *label;
  ButtonCallback  callback;
};

static void color_display_ok_callback        (GtkWidget *, gpointer);
static void color_display_cancel_callback    (GtkWidget *, gpointer);
static gint color_display_delete_callback    (GtkWidget *, gpointer);
static gint color_display_destroy_callback   (GtkWidget *, gpointer);
static void color_display_add_callback       (GtkWidget *, gpointer);
static void color_display_remove_callback    (GtkWidget *, gpointer);
static void color_display_up_callback        (GtkWidget *, gpointer);
static void color_display_down_callback      (GtkWidget *, gpointer);
static void color_display_configure_callback (GtkWidget *, gpointer);

static void src_list_populate (const char *name,
			       gpointer    user_data);

static void
make_dialog (void)
{
  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkWidget *vbbox;
  char *titles[2];
  int i;

  static ActionAreaItem action_items[] =
  {
    { N_("OK"), color_display_ok_callback, NULL, NULL },
    { N_("Cancel"), color_display_cancel_callback, NULL, NULL }
  };

  static ButtonInfo buttons[] = 
  {
    { N_("Add"), color_display_add_callback },
    { N_("Remove"), color_display_remove_callback },
    { N_("Up"), color_display_up_callback },
    { N_("Down"), color_display_down_callback },
    { N_("Configure"), color_display_configure_callback }
  };

  cdd.shell = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (cdd.shell), "display_color", "Gimp");
  gtk_window_set_title (GTK_WINDOW (cdd.shell), _("Color Display Filters"));

  gtk_signal_connect (GTK_OBJECT (cdd.shell), "delete_event",
		      GTK_SIGNAL_FUNC (color_display_delete_callback),
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
  gtk_clist_column_titles_passive (GTK_CLIST (cdd.src));
  gtk_clist_set_auto_sort (GTK_CLIST (cdd.src), TRUE);
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd.src);

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
  gtk_clist_column_titles_passive (GTK_CLIST (cdd.dest));
  gtk_container_add (GTK_CONTAINER (scrolled_win), cdd.dest);

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

  action_items[0].user_data = cdd.shell;
  action_items[1].user_data = cdd.shell;
  build_action_area (GTK_DIALOG (cdd.shell), action_items, 2, 0);
}

static void
color_display_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  gtk_widget_hide (GTK_WIDGET (data));
}

static void
color_display_cancel_callback (GtkWidget *widget,
			       gpointer   data)
{
  gtk_widget_hide (GTK_WIDGET (data));
}

static gint
color_display_delete_callback (GtkWidget *widget,
			       gpointer   data)
{
  color_display_cancel_callback (widget, data);
  return TRUE;
}

static gint
color_display_destroy_callback (GtkWidget *widget,
				gpointer   data)
{
  g_free (data);
  return FALSE;
}

static void
color_display_add_callback (GtkWidget *widget,
			    gpointer   data)
{
}

static void
color_display_remove_callback (GtkWidget *widget,
			       gpointer   data)
{
}

static void
color_display_up_callback (GtkWidget *widget,
			   gpointer   data)
{
}

static void
color_display_down_callback (GtkWidget *widget,
			     gpointer   data)
{
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

  gimp_color_display_foreach (src_list_populate, cdd.src);

  gtk_widget_show (cdd.shell);
}

static void
src_list_populate (const char *name,
		   gpointer    user_data)
{
  gtk_clist_append (GTK_CLIST (user_data), &name);
}
