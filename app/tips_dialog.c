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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "tips_dialog.h"
#include "gimprc.h"
#include "gimpui.h"

#include "libgimp/gimpenv.h"

#include "libgimp/gimpintl.h"

#include "wilber.h"


#define TIPS_DIR_NAME   "tips"

static void   tips_dialog_destroy (GtkWidget *widget, gpointer data);
static void   tips_show_previous  (GtkWidget *widget, gpointer data);
static void   tips_show_next      (GtkWidget *widget, gpointer data);
static void   tips_toggle_update  (GtkWidget *widget, gpointer data);
static void   read_tips_file      (gchar *filename);

static GtkWidget  *tips_dialog = NULL;
static GtkWidget  *tips_label;
static gchar     **tips_text = NULL;
static gint        tips_count = 0;
static gint        old_show_tips;

void
tips_dialog_create (void)
{
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *preview;
  GtkWidget *button;
  gchar     *temp;
  guchar    *utemp;
  guchar    *src;
  guchar    *dest;
  gint       x;
  gint       y;

  if (tips_count == 0)
    {
      temp = g_strdup_printf ("%s" G_DIR_SEPARATOR_S TIPS_DIR_NAME
                              G_DIR_SEPARATOR_S "%s",
			      gimp_data_directory (),
                              _("gimp_tips.txt"));
      read_tips_file (temp);
      g_free (temp);
    }

  if (last_tip >= tips_count || last_tip < 0)
    last_tip = 0;

  if (!tips_dialog)
    {
      tips_dialog = gtk_window_new (GTK_WINDOW_DIALOG);
      gtk_window_set_wmclass (GTK_WINDOW (tips_dialog), "tip_of_the_day", "Gimp");
      gtk_window_set_title (GTK_WINDOW (tips_dialog), _("GIMP Tip of the Day"));
      gtk_window_set_position (GTK_WINDOW (tips_dialog), GTK_WIN_POS_CENTER);
      gtk_window_set_policy (GTK_WINDOW (tips_dialog), FALSE, TRUE, FALSE);

      gtk_signal_connect (GTK_OBJECT (tips_dialog), "delete_event",
			  GTK_SIGNAL_FUNC (tips_dialog_destroy),
			  NULL);
      gtk_signal_connect (GTK_OBJECT (tips_dialog), "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  &tips_dialog);

      /* destroy the tips window if the mainlevel gtk_main() function is left */
      gtk_quit_add_destroy (1, GTK_OBJECT (tips_dialog));

      vbox = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (tips_dialog), vbox);      
      gtk_widget_show (vbox);

      hbox = gtk_hbox_new (FALSE, 5);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      tips_label = gtk_label_new (tips_text[last_tip]);
      gtk_label_set_justify (GTK_LABEL (tips_label), GTK_JUSTIFY_LEFT);
      gtk_misc_set_alignment (GTK_MISC (tips_label), 0.5, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), tips_label, TRUE, TRUE, 3);
      gtk_widget_show (tips_label);
     
      vbox2 = gtk_vbox_new (FALSE, 0);
      gtk_box_pack_end (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);
      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_box_pack_start (GTK_BOX (vbox2), frame, TRUE, FALSE, 0);

      preview = gtk_preview_new (GTK_PREVIEW_COLOR);
      gtk_preview_size (GTK_PREVIEW (preview), wilber_width, wilber_height);
      utemp = g_new (guchar, wilber_width * 3);
      src = (guchar *)wilber_data;
      for (y = 0; y < wilber_height; y++)
	{
	  dest = utemp;
	  for (x = 0; x < wilber_width; x++)
	    {
	      HEADER_PIXEL (src, dest);
	      dest += 3;
	    }
	  gtk_preview_draw_row (GTK_PREVIEW (preview), utemp, 0, y, wilber_width); 
	}
      g_free(utemp);
      gtk_container_add (GTK_CONTAINER (frame), preview);
      gtk_widget_show (preview);
      gtk_widget_show (frame);
      gtk_widget_show (vbox2);
 
      hbox = gtk_hbox_new (FALSE, 15);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);
      
      button = 
	gtk_check_button_new_with_label (_("Show tip next time GIMP starts"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
				    show_tips);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (tips_toggle_update),
			  (gpointer) &show_tips);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      old_show_tips = show_tips;

      bbox = gtk_hbutton_box_new ();
      gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
      gtk_widget_show (bbox);

      button = gtk_button_new_with_label (_("Close"));
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_window_set_default (GTK_WINDOW (tips_dialog), button);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (tips_dialog_destroy),
			  NULL);
      gtk_container_add (GTK_CONTAINER (bbox), button);
      gtk_widget_show (button);

      bbox = gtk_hbutton_box_new ();
      gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);
      gtk_button_box_set_spacing (GTK_BUTTON_BOX (bbox), 5);
      gtk_box_pack_end (GTK_BOX (hbox), bbox, FALSE, FALSE, 0);
      gtk_widget_show (bbox);

      button = gtk_button_new_with_label (_("Previous Tip"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (tips_show_previous),
			  NULL);
      gtk_container_add (GTK_CONTAINER (bbox), button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label (_("Next Tip"));
      GTK_WIDGET_UNSET_FLAGS (button, GTK_RECEIVES_DEFAULT);
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (tips_show_next),
			  NULL);
      gtk_container_add (GTK_CONTAINER (bbox), button);
      gtk_widget_show (button);

     /*  Connect the "F1" help key  */
      gimp_help_connect_help_accel (tips_dialog,
				    gimp_standard_help_func,
				    "dialogs/tip_of_the_day.html");
    }

  if (!GTK_WIDGET_VISIBLE (tips_dialog))
    {
      gtk_widget_show (tips_dialog);
    }
  else
    {
      gdk_window_raise (tips_dialog->window);
    }
}

static void
tips_dialog_destroy (GtkWidget *widget,
		     gpointer   data)
{
  GList *update = NULL; /* options that should be updated in .gimprc */
  GList *remove = NULL; /* options that should be commented out */

  gtk_widget_destroy (tips_dialog);

  /* the last-shown-tip is now saved in sessionrc */

  if (show_tips != old_show_tips)
    {
      update = g_list_append (update, "show-tips");
      remove = g_list_append (remove, "dont-show-tips");
      old_show_tips = show_tips;
      save_gimprc (&update, &remove);
    }
  g_list_free (update);
  g_list_free (remove);
}

static void
tips_show_previous (GtkWidget *widget,
		    gpointer  data)
{
  last_tip--;

  if (last_tip < 0)
    last_tip = tips_count - 1;

  gtk_label_set_text (GTK_LABEL (tips_label), tips_text[last_tip]);
}

static void
tips_show_next (GtkWidget *widget,
		gpointer   data)
{
  last_tip++;

  if (last_tip >= tips_count)
    last_tip = 0;

  gtk_label_set_text (GTK_LABEL (tips_label), tips_text[last_tip]);
}

static void
tips_toggle_update (GtkWidget *widget,
		    gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

static void
store_tip (gchar *str)
{
  tips_count++;
  tips_text = g_realloc (tips_text, sizeof (gchar *) * tips_count);
  tips_text[tips_count - 1] = str;
}

static void
read_tips_file (gchar *filename)
{
  FILE *fp;
  gchar *tip = NULL;
  gchar *str = NULL;

  fp = fopen (filename, "rt");
  if (!fp)
    {
      store_tip (_("Your GIMP tips file appears to be missing!\n"
		   "There should be a file called gimp_tips.txt in\n"
		   "the tips subdirectory of the GIMP data directory.\n"
		   "Please check your installation."));
      return;
    }

  str = g_new (char, 1024);
  while (!feof (fp))
    {
      if (!fgets (str, 1024, fp))
	continue;
   
      if (str[0] == '#' || str[0] == '\n')
	{
	  if (tip != NULL)
	    {
	      tip[strlen (tip) - 1] = '\000';
	      store_tip (tip);
	      tip = NULL;
	    }
	}
      else
	{
	  if (tip == NULL)
	    {
	      tip = g_malloc (strlen (str) + 1);
	      strcpy (tip, str);
	    }
	  else
	    {
	      tip = g_realloc (tip, strlen (tip) + strlen (str) + 1);
	      strcat (tip, str);
	    }
	}
    }
  if (tip != NULL)
    store_tip (tip);
  g_free (str);
  fclose (fp);
}
