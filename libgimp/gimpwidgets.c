/* LIBGIMP - The GIMP Library                                                   
 * Copyright (C) 1995-1999 Peter Mattis and Spencer Kimball                
 *
 * gimpwidgets.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.             
 *                                                                              
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU            
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "gimphelpui.h"
#include "gimpunitmenu.h"
#include "gimpwidgets.h"
#include "gimpmath.h"

#include "libgimp-intl.h"

/*
 *  Forward declarations
 */

static void gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button);


/*
 *  Widget Constructors
 */

GtkWidget *
gimp_option_menu_new (gboolean            menu_only,

		      /* specify menu items as va_list:
		       *  gchar          *label,
		       *  GtkSignalFunc   callback,
		       *  gpointer        data,
		       *  gpointer        user_data,
		       *  GtkWidget     **widget_ptr,
		       *  gboolean        active
		       */

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  gchar          *label;
  GtkSignalFunc   callback;
  gpointer        data;
  gpointer        user_data;
  GtkWidget     **widget_ptr;
  gboolean        active;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;
  va_start (args, menu_only);
  label = va_arg (args, gchar*);
  for (i = 0; label; i++)
    {
      callback   = va_arg (args, GtkSignalFunc);
      data       = va_arg (args, gpointer);
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);
      active     = va_arg (args, gboolean);

      if (label != (gpointer) 1)
	menuitem = gtk_menu_item_new_with_label (label);
      else
	menuitem = gtk_menu_item_new ();

      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  callback,
			  data);

      if (user_data)
	gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (active)
	initial_index = i;

      label = va_arg (args, gchar*);
    }
  va_end (args);

  if (! menu_only)
    {
      GtkWidget *optionmenu;

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      /*  select the initial menu item  */
      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), initial_index);

      return optionmenu;
    }

  return menu;
}

GtkWidget *
gimp_option_menu_new2 (gboolean        menu_only,
		       GtkSignalFunc   menu_item_callback,
		       gpointer        data,
		       gpointer        initial, /* user_data */

		       /* specify menu items as va_list:
			*  gchar      *label,
			*  gpointer    user_data,
			*  GtkWidget **widget_ptr,
			*/

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  gchar      *label;
  gpointer    user_data;
  GtkWidget **widget_ptr;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;
  va_start (args, initial);
  label = va_arg (args, gchar*);
  for (i = 0; label; i++)
    {
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);

      if (label != (gpointer) 1)
	menuitem = gtk_menu_item_new_with_label (label);
      else
	menuitem = gtk_menu_item_new ();

      gtk_menu_append (GTK_MENU (menu), menuitem);
      gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			  menu_item_callback,
			  data);

      if (user_data)
	gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (user_data == initial)
	initial_index = i;

      label = va_arg (args, gchar*);
    }
  va_end (args);

  if (! menu_only)
    {
      GtkWidget *optionmenu;

      optionmenu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), menu);

      /*  select the initial menu item  */
      gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu), initial_index);

      return optionmenu;
    }

  return menu;
}

GtkWidget *
gimp_radio_group_new (gboolean            in_frame,
		      gchar              *frame_title,

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
      widget_ptr = va_arg (args, GtkWidget **);
      active     = va_arg (args, gboolean);

      if (label != (gpointer) 1)
	button = gtk_radio_button_new_with_label (group, label);
      else
	button = gtk_radio_button_new (group);

      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      if (user_data)
	gtk_object_set_user_data (GTK_OBJECT (button), user_data);

      if (widget_ptr)
	*widget_ptr = button;

      if (active)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  callback,
			  data);

      gtk_widget_show (button);

      label = va_arg (args, gchar*);
    }
  va_end (args);

  if (in_frame)
    return frame;

  return vbox;
}

GtkWidget *
gimp_radio_group_new2 (gboolean        in_frame,
		       gchar          *frame_title,
		       GtkSignalFunc   radio_button_callback,
		       gpointer        data,
		       gpointer        initial, /* user_data */

		       /* specify radio buttons as va_list:
			*  gchar      *label,
			*  gpointer    user_data,
			*  GtkWidget **widget_ptr,
			*/

		       ...)
{
  GtkWidget *vbox;
  GtkWidget *frame = NULL;
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  gchar      *label;
  gpointer    user_data;
  GtkWidget **widget_ptr;

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
  va_start (args, initial);
  label = va_arg (args, gchar*);
  while (label)
    {
      user_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);

      if (label != (gpointer) 1)
	button = gtk_radio_button_new_with_label (group, label);
      else
	button = gtk_radio_button_new (group);

      group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      if (user_data)
	gtk_object_set_user_data (GTK_OBJECT (button), user_data);

      if (widget_ptr)
	*widget_ptr = button;

      if (initial == user_data)
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      gtk_signal_connect (GTK_OBJECT (button), "toggled",
			  radio_button_callback,
			  data);

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

GtkObject *
gimp_scale_entry_new (GtkTable *table,
		      gint      column,
		      gint      row,
		      gchar    *text,
		      gint      scale_usize,
		      gint      spinbutton_usize,
		      gfloat    value,
		      gfloat    lower,
		      gfloat    upper,
		      gfloat    step_increment,
		      gfloat    page_increment,
		      guint     digits,
		      gchar    *tooltip,
		      gchar    *private_tip)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  spinbutton = gimp_spin_button_new (&adjustment, value, lower, upper,
				     step_increment, page_increment, 0.0,
				     1.0, digits);
  if (spinbutton_usize > 0)
    gtk_widget_set_usize (spinbutton, spinbutton_usize, -1);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  if (scale_usize > 0)
    gtk_widget_set_usize (scale, scale_usize, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
		    column + 1, column + 2, row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
		    column + 2, column + 3, row, row + 1,
		    GTK_SHRINK, GTK_FILL, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip || private_tip)
    {
      gimp_help_set_help_data (scale, tooltip, private_tip);
      gimp_help_set_help_data (spinbutton, tooltip, private_tip);
    }

  return adjustment;
}

static void
gimp_random_seed_toggle_update (GtkWidget *widget,
				gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *toggle_val = (gint) gtk_object_get_data (GTK_OBJECT (widget),
					      "time_true");
  else
    *toggle_val = (gint) gtk_object_get_data (GTK_OBJECT (widget),
					      "time_false");

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));
}

GtkWidget *
gimp_random_seed_new (gint *seed,
		      gint *use_time,
		      gint  time_true,
		      gint  time_false)
{
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *time_button;

  hbox = gtk_hbox_new (FALSE, 4);

  spinbutton = gimp_spin_button_new (&adj, *seed,
                                     0, G_MAXRAND, 1, 10, 0, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (adj), "value_changed",
                      GTK_SIGNAL_FUNC (gimp_int_adjustment_update),
                      seed);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
                           _("If the \"Time\" button is not pressed, "
                             "use this value for random number generator "
                             "seed - this allows you to repeat a "
                             "given \"random\" operation"), NULL);

  time_button = gtk_toggle_button_new_with_label (_("Time"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (time_button)->child), 2, 0);
  gtk_signal_connect (GTK_OBJECT (time_button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_random_seed_toggle_update),
                      use_time);
  gtk_box_pack_end (GTK_BOX (hbox), time_button, FALSE, FALSE, 0);
  gtk_widget_show (time_button);

  gimp_help_set_help_data (time_button,
                           _("Seed random number generator from the current "
                             "time - this guarantees a reasonable "
                             "randomization"), NULL);

  gtk_object_set_data (GTK_OBJECT (time_button), "time_true",
		       (gpointer) time_true);
  gtk_object_set_data (GTK_OBJECT (time_button), "time_false",
		       (gpointer) time_false);

  gtk_object_set_data (GTK_OBJECT (time_button), "inverse_sensitive",
		       spinbutton);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (time_button),
                                *use_time == time_true);

  return hbox;
}

/*
 *  Standard Callbacks
 */

static void
gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button)
{
  GtkWidget *set_sensitive;
  gboolean   active;

  active = gtk_toggle_button_get_active (toggle_button);

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "set_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (set_sensitive), active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "set_sensitive");
    }

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "inverse_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (set_sensitive), ! active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "inverse_sensitive");
    }
}

void
gimp_toggle_button_update (GtkWidget *widget,
			   gpointer   data)
{
  gint *toggle_val;

  toggle_val = (gint *) data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));
}

void
gimp_menu_item_update (GtkWidget *widget,
		       gpointer   data)
{
  gint *item_val;

  item_val = (gint *) data;

  *item_val = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
}

void
gimp_radio_button_update (GtkWidget *widget,
			  gpointer   data)
{
  gint *toggle_val;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      toggle_val = (gint *) data;

      *toggle_val = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
    }

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));
}

void
gimp_int_adjustment_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  gint *val;

  val = (gint *) data;
  *val = (gint) (adjustment->value + 0.5);
}

void
gimp_float_adjustment_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  gfloat *val;

  val = (gfloat *) data;
  *val = adjustment->value;
}

void
gimp_double_adjustment_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  gdouble *val;

  val = (gdouble *) data;
  *val = adjustment->value;
}

void
gimp_unit_menu_update (GtkWidget *widget,
		       gpointer   data)
{
  GUnit     *val;
  GtkWidget *spinbutton;
  gint       digits;

  val = (GUnit *) data;
  *val = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  digits = ((*val == UNIT_PIXEL) ? 0 :
	    ((*val == UNIT_PERCENT) ? 2 :
	     (MIN (6, MAX (3, gimp_unit_get_digits (*val))))));

  spinbutton =
    gtk_object_get_data (GTK_OBJECT (widget), "set_digits");
  while (spinbutton)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
      spinbutton = gtk_object_get_data (GTK_OBJECT (spinbutton), "set_digits");
    }
}

/*
 *  Helper Functions
 */

/*  add aligned label & widget to a two-column table  */
void
gimp_table_attach_aligned (GtkTable  *table,
			   gint       row,
			   gchar     *label_text,
			   gfloat     xalign,
			   gfloat     yalign,
			   GtkWidget *widget,
			   gboolean   left_adjust)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, GTK_WIDGET (label), 0, 1, row, row + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  if (left_adjust)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_table_attach (table, alignment, 1, 2, row, row + 1,
			GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
      gtk_widget_show (alignment);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
    }
  else
    {
      gtk_table_attach_defaults (table, widget, 1, 2, row, row + 1);
    }

  gtk_widget_show (widget);
}
