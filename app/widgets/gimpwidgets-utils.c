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
#include "gimpui.h"

#include "libgimp/gimpsizeentry.h"
#include "libgimp/gimpintl.h"

/* #include "pixmaps/wilber.xpm" */

/*
 *  Widget Constructors...
 */

GtkWidget *
gimp_option_menu_new (GtkSignalFunc  menu_item_callback,
		      gpointer       initial,  /* user_data */

		      /* specify menu items as va_list:
		       *  gchar     *label,
		       *  gpointer   data,
		       *  gpointer   user_data,
		       */

		      ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;
  GtkWidget *optionmenu;

  /*  menu item variables  */
  gchar     *label;
  gpointer   data;
  gpointer   user_data;

  va_list    args;
  gint       i;
  gint       initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;
  va_start (args, initial);
  label = va_arg (args, gchar*);
  for (i = 0; label; i++)
    {
      data = va_arg (args, gpointer);
      user_data = va_arg (args, gpointer);

      menuitem = gtk_menu_item_new_with_label (label);
      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  menu_item_callback, data);
      gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);
      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (user_data == initial)
	initial_index = i;

      label = va_arg (args, gchar*);
    }
  va_end (args);

  optionmenu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

  /*  select the initial menu item  */
  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), initial_index);

  return optionmenu;
}

GtkWidget *
gimp_radio_group_new (gboolean  in_frame,
		      gchar    *frame_title,

		      /* specify radio buttons as va_list:
		       *  gchar          *label,
		       *  GtkSignalFunc   callback,
		       *  gpointer        data,
		       *  gpointer        user_data,
		       *  GtkWidget     **widget_ptr,
		       *  gboolean        active,
		       */

		      ...)
{
  GtkWidget *vbox;
  GtkWidget *frame = NULL;
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  gchar          *label;
  GtkSignalFunc   callback;
  gpointer        data;
  gpointer        user_data;
  GtkWidget     **widget_ptr;
  gboolean        active;

  va_list args;

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

  if (in_frame)
    {
      frame = gtk_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);
    }

  group = NULL;

  /*  create the radio buttons  */
  va_start (args, frame_title);
  label = va_arg (args, gchar*);
  while (label)
    {
      callback   = va_arg (args, GtkSignalFunc);
      data       = va_arg (args, gpointer);
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, gpointer);
      active     = va_arg (args, gboolean);

      button = gtk_radio_button_new_with_label (group, label);
      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  GTK_SIGNAL_FUNC (callback),
			  data);

      if (user_data)
	gtk_object_set_user_data (GTK_OBJECT (button), user_data);

      if (widget_ptr)
	*widget_ptr = button;

      /*  press the initially active radio button  */
      if (active)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      gtk_widget_show (button);

      label = va_arg (args, gchar*);
    }
  va_end (args);

  if (in_frame)
    return frame;

  return vbox;
}

GtkWidget *
gimp_spin_button_new (GtkObject **adjustment,  /* return value */
		      gfloat      value,
		      gfloat      lower,
		      gfloat      upper,
		      gfloat      step_increment,
		      gfloat      page_increment,
		      gfloat      page_size,
		      gfloat      climb_rate,
		      guint       digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
				    step_increment, page_increment, page_size);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
				    climb_rate, digits);
  gtk_spin_button_set_shadow_type (GTK_SPIN_BUTTON (spinbutton),
				   GTK_SHADOW_NONE);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_widget_set_usize (spinbutton, 75, -1);

  return spinbutton;
}


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

static QueryBox * create_query_box (gchar *, GimpHelpFunc, gchar *,
				    GtkSignalFunc,
				    gchar *, GtkObject *, gchar *,
				    GimpQueryFunc, gpointer);
static void  query_box_cancel_callback    (GtkWidget *, gpointer);
static void  string_query_box_ok_callback (GtkWidget *, gpointer);
static void  int_query_box_ok_callback    (GtkWidget *, gpointer);
static void  double_query_box_ok_callback (GtkWidget *, gpointer);
static void  size_query_box_ok_callback   (GtkWidget *, gpointer);

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
			  query_box, NULL, TRUE, FALSE,
			  _("Cancel"), query_box_cancel_callback,
			  query_box, NULL, FALSE, TRUE,

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
  GtkWidget  *mbox;
  GtkCallback callback;
  gpointer    data;
};

static void  gimp_message_box_close_callback  (GtkWidget *, gpointer);

#define MESSAGE_BOX_MAXIMUM  7  /*  the maximum number of concucrrent
				 *  dialog boxes
				 */

static gint   message_pool     = MESSAGE_BOX_MAXIMUM;
static gchar *message_box_last = NULL;

GtkWidget *
gimp_message_box (gchar       *message,
		  GtkCallback  callback,
		  gpointer     data)
{
  MessageBox *msg_box;
  GtkWidget  *mbox;
  GtkWidget  *vbox;
  GtkWidget  *label;

  static gint repeat_count;

  /* arguably, if message_pool <= 0 we could print to stdout */
  if (!message || message_pool <= 0)
    return NULL;

  if (message_box_last && !strcmp (message_box_last, message))
    {
      repeat_count++;
      if (repeat_count == 3)
        message = "WARNING: message repeated too often, ignoring";
      else if (repeat_count > 3)
        return 0;
    }
  else
    {
      repeat_count = 0;
      if (message_box_last)
	g_free (message_box_last);
      message_box_last = g_strdup (message);
    }

  if (message_pool == 1)
    message = "WARNING: too many messages, close some dialog boxes first";

  msg_box = g_new (MessageBox, 1);

  mbox = gimp_dialog_new (_("GIMP Message"), "gimp_message",
			  NULL, NULL,
			  GTK_WIN_POS_MOUSE,
			  FALSE, FALSE, FALSE,

			  _("OK"), gimp_message_box_close_callback,
			  msg_box, NULL, TRUE, TRUE,

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
  msg_box->callback = callback;
  msg_box->data = data;

  gtk_widget_show (mbox);

  /* allocate message box */
  message_pool--;

  return mbox;
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
  message_pool++;

  g_free (msg_box);
}


/*
 *  Helper Functions...
 */

/*  add aligned label & widget to a two-column table  */
void
gimp_table_attach_aligned (GtkTable  *table,
			   gint       row,
			   gchar     *text,
			   gfloat     xalign,
			   gfloat     yalign,
			   GtkWidget *widget,
			   gboolean   left_adjust)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
  gtk_table_attach (table, GTK_WIDGET (label), 0, 1, row, row + 1,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  if (left_adjust)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 1.0, 0.0, 0.0);
      gtk_table_attach_defaults (table, alignment, 1, 2, row, row + 1);
      gtk_widget_show (alignment);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
    }
  else
    {
      gtk_table_attach_defaults (table, widget, 1, 2, row, row + 1);
    }

  gtk_widget_show (widget);
}
