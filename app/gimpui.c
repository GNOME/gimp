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

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

extern gchar *prog_name;

static void  gimp_message_box_close_callback  (GtkWidget *, gpointer);

/*
 *  String, integer, double and size query boxes
 */

typedef struct _QueryBox QueryBox;

struct _QueryBox
{
  GtkWidget     *qbox;
  GtkWidget     *vbox;
  GtkWidget     *entry;
  GtkObject     *object;
  GimpQueryFunc  callback;
  gpointer       data;
};

static QueryBox * create_query_box             (gchar *, GimpHelpFunc, gchar *,
						GtkSignalFunc,
						gchar *, GtkObject *, gchar *,
						GimpQueryFunc, gpointer);
static void       query_box_cancel_callback    (GtkWidget *, gpointer);
static void       string_query_box_ok_callback (GtkWidget *, gpointer);
static void       int_query_box_ok_callback    (GtkWidget *, gpointer);
static void       double_query_box_ok_callback (GtkWidget *, gpointer);
static void       size_query_box_ok_callback   (GtkWidget *, gpointer);

/*  create a generic query box without any entry widget  */
static QueryBox *
create_query_box (gchar         *title,
		  GimpHelpFunc   help_func,
		  gchar         *help_data,
		  GtkSignalFunc  ok_callback,
		  gchar         *message,
		  GtkObject     *object,
		  gchar         *signal,
		  GimpQueryFunc  callback,
		  gpointer       data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox;
  GtkWidget *label;

  query_box = g_new (QueryBox, 1);

  qbox = gimp_dialog_new (title, "query_box",
			  help_func, help_data,
			  GTK_WIN_POS_MOUSE,
			  FALSE, TRUE, FALSE,

			  _("OK"), ok_callback,
			  query_box, NULL, NULL, TRUE, FALSE,
			  _("Cancel"), query_box_cancel_callback,
			  query_box, NULL, NULL, FALSE, TRUE,

			  NULL);

  /*  if we are associated with an object, connect to the provided signal  */
  if (object && GTK_IS_OBJECT (object) && signal)
    gtk_signal_connect (GTK_OBJECT (object), signal,
			(GtkSignalFunc) query_box_cancel_callback,
			query_box);
  else
    object = NULL;

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
  gtk_widget_show (vbox);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  query_box->qbox = qbox;
  query_box->vbox = vbox;
  query_box->entry = NULL;
  query_box->object = object;
  query_box->callback = callback;
  query_box->data = data;

  return query_box;
}

GtkWidget *
gimp_query_string_box (gchar         *title,
		       GimpHelpFunc   help_func,
		       gchar         *help_data,
		       gchar         *message,
		       gchar         *initial,
		       GtkObject     *object,
		       gchar         *signal,
		       GimpQueryFunc  callback,
		       gpointer       data)
{
  QueryBox  *query_box;
  GtkWidget *entry;

  query_box = create_query_box (title, help_func, help_data,
				string_query_box_ok_callback,
				message, object, signal, callback, data);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (query_box->vbox), entry, FALSE, FALSE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);
  gtk_widget_grab_focus (entry);
  gtk_widget_show (entry);

  query_box->entry = entry;

  return query_box->qbox;
}

GtkWidget *
gimp_query_int_box (gchar         *title,
		    GimpHelpFunc   help_func,
		    gchar         *help_data,
		    gchar         *message,
		    gint           initial,
		    gint           lower,
		    gint           upper,
		    GtkObject     *object,
		    gchar         *signal,
		    GimpQueryFunc  callback,
		    gpointer       data)
{
  QueryBox  *query_box;
  GtkAdjustment* adjustment;
  GtkWidget *spinbutton;

  query_box = create_query_box (title, help_func, help_data,
				int_query_box_ok_callback,
				message, object, signal, callback, data);

  adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (initial, lower, upper, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 0);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

GtkWidget *
gimp_query_double_box (gchar         *title,
		       GimpHelpFunc   help_func,
		       gchar         *help_data,
		       gchar         *message,
		       gdouble        initial,
		       gdouble        lower,
		       gdouble        upper,
		       gint           digits,
		       GtkObject     *object,
		       gchar         *signal,
		       GimpQueryFunc  callback,
		       gpointer       data)
{
  QueryBox  *query_box;
  GtkAdjustment* adjustment;
  GtkWidget *spinbutton;

  query_box = create_query_box (title, help_func, help_data,
				double_query_box_ok_callback,
				message, object, signal, callback, data);

  adjustment =
    GTK_ADJUSTMENT (gtk_adjustment_new (initial, lower, upper, 1, 10, 0));
  spinbutton = gtk_spin_button_new (adjustment, 1.0, digits);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

GtkWidget *
gimp_query_size_box (gchar         *title,
		     GimpHelpFunc   help_func,
		     gchar         *help_data,
		     gchar         *message,
		     gdouble        initial,
		     gdouble        lower,
		     gdouble        upper,
		     gint           digits,
		     GUnit          unit,
		     gdouble        resolution,
		     gboolean       dot_for_dot,
		     GtkObject     *object,
		     gchar         *signal,
		     GimpQueryFunc  callback,
		     gpointer       data)
{
  QueryBox  *query_box;
  GtkWidget *sizeentry;

  query_box = create_query_box (title, help_func, help_data,
				size_query_box_ok_callback,
				message, object, signal, callback, data);

  sizeentry = gimp_size_entry_new (1, unit, "%p", TRUE, FALSE, FALSE, 100,
				   GIMP_SIZE_ENTRY_UPDATE_SIZE);
  if (dot_for_dot)
    gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), UNIT_PIXEL);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0,
				  resolution, FALSE);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (sizeentry), 0, digits);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 lower, upper);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, initial);

  gtk_box_pack_start (GTK_BOX (query_box->vbox), sizeentry, FALSE, FALSE, 0);
  gimp_size_entry_grab_focus (GIMP_SIZE_ENTRY (sizeentry));
  gtk_widget_show (sizeentry);

  query_box->entry = sizeentry;

  return query_box->qbox;
}

static void
query_box_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) data;

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
string_query_box_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  QueryBox *query_box;
  gchar    *string;

  query_box = (QueryBox *) data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, query_box->data, (gpointer) string);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
int_query_box_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  QueryBox *query_box;
  gint     *integer_value;

  query_box = (QueryBox *) data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the spinbutton data  */
  integer_value = g_malloc (sizeof (gint));
  *integer_value =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, query_box->data,
			   (gpointer) integer_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
double_query_box_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  QueryBox *query_box;
  gdouble  *double_value;

  query_box = (QueryBox *) data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the spinbutton data  */
  double_value = g_malloc (sizeof (gdouble));
  *double_value =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, query_box->data,
			   (gpointer) double_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
size_query_box_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  QueryBox *query_box;
  gdouble  *double_value;

  query_box = (QueryBox *) data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  /*  Get the sizeentry data  */
  double_value = g_malloc (sizeof (gdouble));
  *double_value =
    gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (query_box->entry), 0);

  /*  Pass the selected unit to the callback  */
  gtk_object_set_data
    (GTK_OBJECT (widget), "size_query_unit",
     (gpointer) gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, query_box->data,
			   (gpointer) double_value);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}


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

static GList *message_boxes    = NULL;

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
	    }
	  return;
	}
    }

  if (g_list_length (message_boxes) == MESSAGE_BOX_MAXIMUM)
    {
      fprintf (stderr, "%s: %s\n", prog_name, message);
      message = _("WARNING:\n"
		  "Too many open message dialogs.\n"
		  "Messages are redirected to stderr.\n");
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
