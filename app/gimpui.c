/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpui.c
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
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

#include "gimpui.h"

#include "libgimp/gimpintl.h"

extern gchar *prog_name;

static void  gimp_message_box_close_callback  (GtkWidget *widget,
					       gpointer   data);

/*
 *  Message Boxes...
 */

typedef struct _MessageBox MessageBox;

struct _MessageBox
{
  GtkWidget   *mbox;
  GtkWidget   *repeat_label;
  gchar       *message;
  gint         repeat_count;
  GtkCallback  callback;
  gpointer     data;
};

/*  the maximum number of concucrrent dialog boxes */
#define MESSAGE_BOX_MAXIMUM  4 

static GList *message_boxes = NULL;

void
gimp_message_box (gchar       *message,
		  GtkCallback  callback,
		  gpointer     data)
{
  MessageBox *msg_box;
  GtkWidget  *mbox;
  GtkWidget  *vbox;
  GtkWidget  *label;
  GList      *list;

  if (!message)
    return;

  if (g_list_length (message_boxes) > MESSAGE_BOX_MAXIMUM)
    {
      fprintf (stderr, "%s: %s\n", prog_name, message);
      return;
    }

  for (list = message_boxes; list; list = list->next)
    {
      msg_box = list->data;
      if (strcmp (msg_box->message, message) == 0)
	{
	  msg_box->repeat_count++;
	  if (msg_box->repeat_count > 1)
	    {
	      gchar *text = g_strdup_printf (_("Message repeated %d times"), 
					     msg_box->repeat_count);
	      gtk_label_set_text (GTK_LABEL (msg_box->repeat_label), text);
	      g_free (text);
	      gdk_window_raise (msg_box->mbox->window);
	    }
	  else
	    {
	      GtkWidget *hbox;

	      hbox = gtk_hbox_new (FALSE, 0);
	      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (msg_box->mbox)->action_area), 
				  hbox, TRUE, FALSE, 4);
	      msg_box->repeat_label = gtk_label_new (_("Message repeated once"));
	      gtk_container_add (GTK_CONTAINER (hbox), msg_box->repeat_label);

	      gtk_widget_show (msg_box->repeat_label);
	      gtk_widget_show (hbox);
	      gdk_window_raise (msg_box->mbox->window);
	    }
	  return;
	}
    }

  if (g_list_length (message_boxes) == MESSAGE_BOX_MAXIMUM)
    {
      fprintf (stderr, "%s: %s\n", prog_name, message);
      message = _("WARNING:\n"
		  "Too many open message dialogs.\n"
		  "Messages are redirected to stderr.");
    }
  
  msg_box = g_new0 (MessageBox, 1);

  mbox = gimp_dialog_new (_("GIMP Message"), "gimp_message",
			  NULL, NULL,
			  GTK_WIN_POS_MOUSE,
			  FALSE, FALSE, FALSE,

			  _("OK"), gimp_message_box_close_callback,
			  msg_box, NULL, NULL, TRUE, TRUE,

			  NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (mbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (message);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
  gtk_widget_show (label);

  msg_box->mbox = mbox;
  msg_box->message = g_strdup (message);
  msg_box->callback = callback;
  msg_box->data = data;

  message_boxes = g_list_append (message_boxes, msg_box);

  gtk_widget_show (mbox);
}

static void
gimp_message_box_close_callback (GtkWidget *widget,
				 gpointer   data)
{
  MessageBox *msg_box;

  msg_box = (MessageBox *) data;

  /*  If there is a valid callback, invoke it  */
  if (msg_box->callback)
    (* msg_box->callback) (widget, msg_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (msg_box->mbox);
  
  /* make this box available again */
  message_boxes = g_list_remove (message_boxes, msg_box);

  g_free (msg_box->message);
  g_free (msg_box);
}
