/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpquerybox.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org> 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gimpquerybox.h"

#include "gimpdialog.h"
#include "gimppixmap.h"
#include "gimpsizeentry.h"
#include "gimpwidgets.h"

#include "config.h"
#include "libgimp-intl.h"

#include "pixmaps/eek.xpm"

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
  GtkSignalFunc  callback;
  gpointer       data;
};

static QueryBox * create_query_box             (gchar         *title,
						GimpHelpFunc   help_func,
						gchar         *help_data,
						GtkSignalFunc  ok_callback,
						GtkSignalFunc  cancel_callback,
						gchar         *message,
						gchar         *ok_button,
						gchar         *cancel_button,
						GtkObject     *object,
						gchar         *signal,
						GtkSignalFunc  callback,
						gpointer       data);

static QueryBox * query_box_disconnect         (gpointer       data);

static void       string_query_box_ok_callback (GtkWidget     *widget,
						gpointer       data);
static void       int_query_box_ok_callback    (GtkWidget     *widget,
						gpointer       data);
static void       double_query_box_ok_callback (GtkWidget     *widget,
						gpointer       data);
static void       size_query_box_ok_callback   (GtkWidget     *widget,
						gpointer       data);

static void       query_box_cancel_callback    (GtkWidget     *widget,
						gpointer       data);

static void       boolean_query_box_true_callback  (GtkWidget     *widget,
						    gpointer       data);
static void       boolean_query_box_false_callback (GtkWidget     *widget,
						    gpointer       data);

/*  create a generic query box without any entry widget  */
static QueryBox *
create_query_box (gchar         *title,
		  GimpHelpFunc   help_func,
		  gchar         *help_data,
		  GtkSignalFunc  ok_callback,
		  GtkSignalFunc  cancel_callback,
		  gchar         *message,
		  gchar         *ok_button,
		  gchar         *cancel_button,
		  GtkObject     *object,
		  gchar         *signal,
		  GtkSignalFunc  callback,
		  gpointer       data)
{
  QueryBox  *query_box;
  GtkWidget *qbox;
  GtkWidget *vbox = NULL;
  GtkWidget *label;

  query_box = g_new (QueryBox, 1);

  qbox = gimp_dialog_new (title, "query_box",
			  help_func, help_data,
			  GTK_WIN_POS_MOUSE,
			  FALSE, TRUE, FALSE,

			  ok_button, ok_callback,
			  query_box, NULL, NULL, TRUE, FALSE,
			  cancel_button, cancel_callback,
			  query_box, NULL, NULL, FALSE, TRUE,

			  NULL);

  /*  if we are associated with an object, connect to the provided signal  */
  if (object && GTK_IS_OBJECT (object) && signal)
    gtk_signal_connect (GTK_OBJECT (object), signal,
			GTK_SIGNAL_FUNC (query_box_cancel_callback),
			query_box);
  else
    object = NULL;

  if (message)
    {
      vbox = gtk_vbox_new (FALSE, 2);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      gtk_container_add (GTK_CONTAINER (GTK_DIALOG (qbox)->vbox), vbox);
      gtk_widget_show (vbox);

      label = gtk_label_new (message);
      gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);
    }

  query_box->qbox     = qbox;
  query_box->vbox     = vbox;
  query_box->entry    = NULL;
  query_box->object   = object;
  query_box->callback = callback;
  query_box->data     = data;

  return query_box;
}

/**
 * gimp_query_string_box:
 * @title: The query box dialog's title.
 * @help_func: The help function to show this dialog's help page.
 * @help_data: A string pointing to this dialog's html help page.
 * @message: A string which will be shown above the dialog's entry widget.
 * @initial: The initial value.
 * @object: The object this query box is associated with.
 * @signal: The object's signal which will cause the query box to be closed.
 * @callback: The function which will be called when the user selects "OK".
 * @data: The callback's user data.
 *
 * Returns: A pointer to the new #GtkDialog.
 *
 */
GtkWidget *
gimp_query_string_box (gchar                   *title,
		       GimpHelpFunc             help_func,
		       gchar                   *help_data,
		       gchar                   *message,
		       gchar                   *initial,
		       GtkObject               *object,
		       gchar                   *signal,
		       GimpQueryStringCallback  callback,
		       gpointer                 data)
{
  QueryBox  *query_box;
  GtkWidget *entry;

  query_box = create_query_box (title, help_func, help_data,
				string_query_box_ok_callback,
				query_box_cancel_callback,
				message,
				_("OK"), _("Cancel"),
				object, signal,
				callback, data);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (query_box->vbox), entry, FALSE, FALSE, 0);
  if (initial)
    gtk_entry_set_text (GTK_ENTRY (entry), initial);
  gtk_widget_grab_focus (entry);
  gtk_widget_show (entry);

  query_box->entry = entry;

  return query_box->qbox;
}

/**
 * gimp_query_int_box:
 * @title: The query box dialog's title.
 * @help_func: The help function to show this dialog's help page.
 * @help_data: A string pointing to this dialog's html help page.
 * @message: A string which will be shown above the dialog's entry widget.
 * @initial: The initial value.
 * @lower: The lower boundary of the range of possible values.
 * @upper: The upper boundray of the range of possible values.
 * @object: The object this query box is associated with.
 * @signal: The object's signal which will cause the query box to be closed.
 * @callback: The function which will be called when the user selects "OK".
 * @data: The callback's user data.
 *
 * Returns: A pointer to the new #GtkDialog.
 *
 */
GtkWidget *
gimp_query_int_box (gchar                *title,
		    GimpHelpFunc          help_func,
		    gchar                *help_data,
		    gchar                *message,
		    gint                  initial,
		    gint                  lower,
		    gint                  upper,
		    GtkObject            *object,
		    gchar                *signal,
		    GimpQueryIntCallback  callback,
		    gpointer              data)
{
  QueryBox  *query_box;
  GtkWidget *spinbutton;
  GtkObject *adjustment;

  query_box = create_query_box (title, help_func, help_data,
				int_query_box_ok_callback,
				query_box_cancel_callback,
				message,
				_("OK"), _("Cancel"),
				object, signal,
				callback, data);

  spinbutton = gimp_spin_button_new (&adjustment,
				     initial, lower, upper, 1, 10, 0,
				     1, 0);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

/**
 * gimp_query_double_box:
 * @title: The query box dialog's title.
 * @help_func: The help function to show this dialog's help page.
 * @help_data: A string pointing to this dialog's html help page.
 * @message: A string which will be shown above the dialog's entry widget.
 * @initial: The initial value.
 * @lower: The lower boundary of the range of possible values.
 * @upper: The upper boundray of the range of possible values.
 * @digits: The number of decimal digits the #GtkSpinButton will provide.
 * @object: The object this query box is associated with.
 * @signal: The object's signal which will cause the query box to be closed.
 * @callback: The function which will be called when the user selects "OK".
 * @data: The callback's user data.
 *
 * Returns: A pointer to the new #GtkDialog.
 *
 */
GtkWidget *
gimp_query_double_box (gchar                   *title,
		       GimpHelpFunc             help_func,
		       gchar                   *help_data,
		       gchar                   *message,
		       gdouble                  initial,
		       gdouble                  lower,
		       gdouble                  upper,
		       gint                     digits,
		       GtkObject               *object,
		       gchar                   *signal,
		       GimpQueryDoubleCallback  callback,
		       gpointer                 data)
{
  QueryBox  *query_box;
  GtkWidget *spinbutton;
  GtkObject *adjustment;

  query_box = create_query_box (title, help_func, help_data,
				double_query_box_ok_callback,
				query_box_cancel_callback,
				message,
				_("OK"), _("Cancel"),
				object, signal,
				callback, data);

  spinbutton = gimp_spin_button_new (&adjustment,
				     initial, lower, upper, 1, 10, 0,
				     1, digits);
  gtk_box_pack_start (GTK_BOX (query_box->vbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_grab_focus (spinbutton);
  gtk_widget_show (spinbutton);

  query_box->entry = spinbutton;

  return query_box->qbox;
}

/**
 * gimp_query_size_box:
 * @title: The query box dialog's title.
 * @help_func: The help function to show this dialog's help page.
 * @help_data: A string pointing to this dialog's html help page.
 * @message: A string which will be shown above the dialog's entry widget.
 * @initial: The initial value.
 * @lower: The lower boundary of the range of possible values.
 * @upper: The upper boundray of the range of possible values.
 * @digits: The number of decimal digits the #GimpSizeEntry provide in
 *          "pixel" mode.
 * @unit: The unit initially shown by the #GimpUnitMenu.
 * @resolution: The resolution (in dpi) which will be used for pixel/unit
 *              calculations.
 * @dot_for_dot: #TRUE if the #GimpUnitMenu's initial unit should be "pixels".
 * @object: The object this query box is associated with.
 * @signal: The object's signal which will cause the query box to be closed.
 * @callback: The function which will be called when the user selects "OK".
 * @data: The callback's user data.
 *
 * Returns: A pointer to the new #GtkDialog.
 *
 */
GtkWidget *
gimp_query_size_box (gchar                 *title,
		     GimpHelpFunc           help_func,
		     gchar                 *help_data,
		     gchar                 *message,
		     gdouble                initial,
		     gdouble                lower,
		     gdouble                upper,
		     gint                   digits,
		     GimpUnit               unit,
		     gdouble                resolution,
		     gboolean               dot_for_dot,
		     GtkObject             *object,
		     gchar                 *signal,
		     GimpQuerySizeCallback  callback,
		     gpointer               data)
{
  QueryBox  *query_box;
  GtkWidget *sizeentry;

  query_box = create_query_box (title, help_func, help_data,
				size_query_box_ok_callback,
				query_box_cancel_callback,
				message,
				_("OK"), _("Cancel"),
				object, signal,
				callback, data);

  sizeentry = gimp_size_entry_new (1, unit, "%p", TRUE, FALSE, FALSE, 100,
				   GIMP_SIZE_ENTRY_UPDATE_SIZE);
  if (dot_for_dot)
    gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), GIMP_UNIT_PIXEL);
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

/**
 * gimp_query_boolean_box:
 * @title: The query box dialog's title.
 * @help_func: The help function to show this dialog's help page.
 * @help_data: A string pointing to this dialog's html help page.
 * @eek: #TRUE if you want the "Eek" wilber to appear left of the dialog's
 *       message.
 * @message: A string which will be shown in the query box.
 * @true_button: The string to be shown in the dialog's left button.
 * @false_button: The string to be shown in the dialog's right button.
 * @object: The object this query box is associated with.
 * @signal: The object's signal which will cause the query box to be closed.
 * @callback: The function which will be called when the user clicks one
 *            of the buttons.
 * @data: The callback's user data.
 *
 * Returns: A pointer to the new #GtkDialog.
 *
 */
GtkWidget *
gimp_query_boolean_box (gchar                    *title,
			GimpHelpFunc              help_func,
			gchar                    *help_data,
			gboolean                  eek,
			gchar                    *message,
			gchar                    *true_button,
			gchar                    *false_button,
			GtkObject                *object,
			gchar                    *signal,
			GimpQueryBooleanCallback  callback,
			gpointer                  data)
{
  QueryBox  *query_box;
  GtkWidget *hbox;
  GtkWidget *pixmap;
  GtkWidget *label;

  query_box = create_query_box (title, help_func, help_data,
				boolean_query_box_true_callback,
				boolean_query_box_false_callback,
				eek ? NULL : message,
				true_button, false_button,
				object, signal,
				callback, data);

  if (!eek)
    return query_box->qbox;

  hbox = gtk_hbox_new (FALSE, 10);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (query_box->qbox)->vbox), hbox);
  gtk_widget_show (hbox);

  pixmap = gimp_pixmap_new (eek_xpm);
  gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
  gtk_widget_show (pixmap);

  label = gtk_label_new (message);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return query_box->qbox;
}

static QueryBox *
query_box_disconnect (gpointer  data)
{
  QueryBox *query_box;

  query_box = (QueryBox *) data;

  gtk_widget_set_sensitive (query_box->qbox, FALSE);

  /*  disconnect, if we are connected to some signal  */
  if (query_box->object)
    gtk_signal_disconnect_by_data (query_box->object, query_box);

  return query_box;
}

static void
string_query_box_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  QueryBox *query_box;
  gchar    *string;

  query_box = query_box_disconnect (data);

  /*  Get the entry data  */
  string = g_strdup (gtk_entry_get_text (GTK_ENTRY (query_box->entry)));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, string,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
int_query_box_ok_callback (GtkWidget *widget,
			   gpointer   data)
{
  QueryBox *query_box;
  gint      value;

  query_box = query_box_disconnect (data);

  /*  Get the spinbutton data  */
  value =
    gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, value,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
double_query_box_ok_callback (GtkWidget *widget,
			      gpointer   data)
{
  QueryBox *query_box;
  gdouble   value;

  query_box = query_box_disconnect (data);

  /*  Get the spinbutton data  */
  value =
    gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, value,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
size_query_box_ok_callback (GtkWidget *widget,
			    gpointer   data)
{
  QueryBox *query_box;
  gdouble   size;
  GimpUnit  unit;

  query_box = query_box_disconnect (data);

  /*  Get the sizeentry data  */
  size = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (query_box->entry), 0);
  unit = gimp_size_entry_get_unit (GIMP_SIZE_ENTRY (query_box->entry));

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, size, unit,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
query_box_cancel_callback (GtkWidget *widget,
			   gpointer   data)
{
  QueryBox *query_box;

  query_box = query_box_disconnect (data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
boolean_query_box_true_callback (GtkWidget *widget,
				 gpointer   data)
{
  QueryBox *query_box;

  query_box = query_box_disconnect (data);

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, TRUE,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}

static void
boolean_query_box_false_callback (GtkWidget *widget,
				  gpointer   data)
{
  QueryBox *query_box;

  query_box = query_box_disconnect (data);

  /*  Call the user defined callback  */
  (* query_box->callback) (query_box->qbox, FALSE,
			   query_box->data);

  /*  Destroy the box  */
  gtk_widget_destroy (query_box->qbox);

  g_free (query_box);
}
