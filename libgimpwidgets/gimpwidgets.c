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

#include "gimpchainbutton.h"
#include "gimphelpui.h"
#include "gimppixmap.h"
#include "gimpunitmenu.h"
#include "gimpwidgets.h"
#include "gimpmath.h"

#include "libgimp-intl.h"

/*
 *  Widget Constructors
 */

/**
 * gimp_option_menu_new:
 * @menu_only: #TRUE if the function should return a #GtkMenu only.
 * @...: A #NULL terminated @va_list describing the menu items.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 *
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

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      callback,
			      data);

	  if (user_data)
	    gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);
	}
      else
	{
	  menuitem = gtk_menu_item_new ();
	}

      gtk_menu_append (GTK_MENU (menu), menuitem);

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

/**
 * gimp_option_menu_new2:
 * @menu_only: #TRUE if the function should return a #GtkMenu only.
 * @menu_item_callback: The callback each menu item's "activate" signal will
 *                      be connected with.
 * @data: The data which will be passed to gtk_signal_connect().
 * @initial: The @user_data of the initially selected menu item.
 * @...: A #NULL terminated @va_list describing the menu items.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 *
 */
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

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  gtk_signal_connect (GTK_OBJECT (menuitem), "activate",
			      menu_item_callback,
			      data);

	  if (user_data)
	    gtk_object_set_user_data (GTK_OBJECT (menuitem), user_data);
	}
      else
	{
	  menuitem = gtk_menu_item_new ();
	}

      gtk_menu_append (GTK_MENU (menu), menuitem);

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

/**
 * gimp_option_menu_set_history:
 * @option_menu: A #GtkOptionMenu as returned by gimp_option_menu_new() or
 *               gimp_option_menu_new2().
 * @user_data: The @user_data of the menu item you want to select.
 *
 */
void
gimp_option_menu_set_history (GtkOptionMenu *option_menu,
			      gpointer       user_data)
{
  GtkWidget *menu_item;
  GList     *list;
  gint       history = 0;

  g_return_if_fail (option_menu);
  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  for (list = GTK_MENU_SHELL (option_menu->menu)->children;
       list;
       list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child) &&
	  gtk_object_get_user_data (GTK_OBJECT (menu_item)) == user_data)
	{
	  break;
	}

      history++;
    }

  if (list)
    gtk_option_menu_set_history (option_menu, history);
}

/**
 * gimp_radio_group_new:
 * @in_frame: #TRUE if you want a #GtkFrame around the radio button group.
 * @frame_title: The title of the Frame or #NULL if you don't want a title.
 * @...: A #NULL terminated @va_list describing the radio buttons.
 *
 * Returns: A #GtkFrame or #GtkVbox (depending on @in_frame).
 *
 */
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
    {
      GtkWidget *frame;

      frame = gtk_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      return frame;
    }

  return vbox;
}

/**
 * gimp_radio_group_new2:
 * @in_frame: #TRUE if you want a #GtkFrame around the radio button group.
 * @frame_title: The title of the Frame or #NULL if you don't want a title.
 * @radio_button_callback: The callback each button's "toggled" signal will
 *                         be connected with.
 * @data: The data which will be passed to gtk_signal_connect().
 * @initial: The @user_data of the initially pressed radio button.
 * @...: A #NULL terminated @va_list describing the radio buttons.
 *
 * Returns: A #GtkFrame or #GtkVbox (depending on @in_frame).
 *
 */
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
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  gchar      *label;
  gpointer    user_data;
  GtkWidget **widget_ptr;

  va_list args;

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

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
    {
      GtkWidget *frame;

      frame = gtk_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      return frame;
    }

  return vbox;
}

/**
 * gimp_spin_button_new:
 * @adjustment: Returns the spinbutton's #GtkAdjustment.
 * @value: The initial value of the spinbutton.
 * @lower: The lower boundary.
 * @upper: The uppper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size: The spinbutton's page size.
 * @climb_rate: The spinbutton's climb rate.
 * @digits: The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a subsequent
 * gtk_spin_button_new() and does some more initialisation stuff like
 * setting a standard minimun horizontal size.
 *
 * Returns: A #GtkSpinbutton and it's #GtkAdjustment.
 *
 */
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

static void
gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
						    GtkAdjustment *other_adj)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (other_adj), adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (other_adj), adjustment);
}

/**
 * gimp_scale_entry_new:
 * @table: The #GtkTable the widgets will be attached to.
 * @column: The column to start with.
 * @row: The row to attach the widgets.
 * @text: The text for the #GtkLabel which will appear left of the #GtkHScale.
 * @scale_usize: The minimum horizontal size of the #GtkHScale.
 * @spinbutton_usize: The minimum horizontal size of the #GtkSpinButton.
 * @value: The initial value.
 * @lower: The lower boundary.
 * @upper: The upper boundary.
 * @step_increment: The step increment.
 * @page_increment: The page increment.
 * @digits: The number of decimal digits.
 * @constrain: #TRUE if the range of possible values of the #GtkSpinButton
 *             should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == #FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == #FALSE.
 * @tooltip: A tooltip message for the scale and the spinbutton.
 * @help_data: The widgets' help_data (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Note that if you pass a @tooltip or @private_tip to this function you'll
 * have to initialize GIMP's help system with gimp_help_init() before using it.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 *
 */
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
		      gboolean  constrain,
		      gfloat    unconstrained_lower,
		      gfloat    unconstrained_upper,
		      gchar    *tooltip,
		      gchar    *help_data)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label,
                    column, column + 1, row, row + 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  if (! constrain &&
      unconstrained_lower <= lower &&
      unconstrained_upper >= upper)
    {
      GtkObject *constrained_adj;

      constrained_adj = gtk_adjustment_new (value, lower, upper,
					    step_increment, page_increment,
					    0.0);

      spinbutton = gimp_spin_button_new (&adjustment, value,
					 unconstrained_lower,
					 unconstrained_upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      gtk_signal_connect
	(GTK_OBJECT (constrained_adj), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 adjustment);

      gtk_signal_connect
	(GTK_OBJECT (adjustment), "value_changed",
	 GTK_SIGNAL_FUNC (gimp_scale_entry_unconstrained_adjustment_callback),
	 constrained_adj);

      return_adj = adjustment;

      adjustment = constrained_adj;
    }
  else
    {
      spinbutton = gimp_spin_button_new (&adjustment, value, lower, upper,
					 step_increment, page_increment, 0.0,
					 1.0, digits);

      return_adj = adjustment;
    }
    
  if (spinbutton_usize > 0)
    gtk_widget_set_usize (spinbutton, spinbutton_usize, -1);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
  if (scale_usize > 0)
    gtk_widget_set_usize (scale, scale_usize, -1);
  gtk_scale_set_digits (GTK_SCALE (scale), digits);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);
  gtk_table_attach (GTK_TABLE (table), scale,
		    column + 1, column + 2, row, row + 1,
		    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (scale);

  gtk_table_attach (GTK_TABLE (table), spinbutton,
		    column + 2, column + 3, row, row + 1,
		    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (spinbutton);

  if (tooltip || help_data)
    {
      gimp_help_set_help_data (scale, tooltip, help_data);
      gimp_help_set_help_data (spinbutton, tooltip, help_data);
    }

  gtk_object_set_data (GTK_OBJECT (return_adj), "label", label);
  gtk_object_set_data (GTK_OBJECT (return_adj), "scale", scale);
  gtk_object_set_data (GTK_OBJECT (return_adj), "spinbutton", spinbutton);

  return return_adj;
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

/**
 * gimp_random_seed_new:
 * @seed: A pointer to the variable which stores the random seed.
 * @use_time: A pointer to the variable which stores the @use_time
 *            toggle boolean.
 * @time_true: The value to write to @use_time if the toggle button is checked.
 * @time_false: The value to write to @use_time if the toggle button is
 *              unchecked.
 *
 * Note that this widget automatically sets tooltips with
 * gimp_help_set_help_data(), so you'll have to initialize GIMP's help
 * system with gimp_help_init() before using it.
 *
 * Returns: A #GtkHBox containing a #GtkSpinButton for the random seed and
 *          a #GtkToggleButton for toggling the @use_time behaviour.
 *
 */
GtkWidget *
gimp_random_seed_new (gint       *seed,
		      gint       *use_time,
		      gint        time_true,
		      gint        time_false)
{
  GtkWidget *hbox;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *button;

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

  button = gtk_toggle_button_new_with_label (_("Time"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
                      GTK_SIGNAL_FUNC (gimp_random_seed_toggle_update),
                      use_time);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Seed random number generator from the current "
                             "time - this guarantees a reasonable "
                             "randomization"), NULL);

  gtk_object_set_data (GTK_OBJECT (button), "time_true",
		       (gpointer) time_true);
  gtk_object_set_data (GTK_OBJECT (button), "time_false",
		       (gpointer) time_false);

  gtk_object_set_data (GTK_OBJECT (button), "inverse_sensitive",
		       spinbutton);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                *use_time == time_true);

  gtk_object_set_data (GTK_OBJECT (hbox), "spinbutton", spinbutton);
  gtk_object_set_data (GTK_OBJECT (hbox), "togglebutton", button);

  return hbox;
}

typedef struct
{
  GimpChainButton *chainbutton;
  gboolean         chain_constrains_ratio;
  gdouble          orig_x;
  gdouble          orig_y;
  gdouble          last_x;
  gdouble          last_y;
} GimpCoordinatesData;

static void
gimp_coordinates_callback (GtkWidget *widget,
			   gpointer   data)
{
  GimpCoordinatesData *gcd;
  gdouble new_x;
  gdouble new_y;

  gcd = (GimpCoordinatesData *) data;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (gcd->chainbutton))
    {
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "value_changed");

      if (gcd->chain_constrains_ratio)
	{
	  if ((gcd->orig_x != 0) && (gcd->orig_y != 0))
	    {
	      if (new_x != gcd->last_x)
		{
		  gcd->last_x = new_x;
		  gcd->last_y = new_y = (new_x * gcd->orig_y) / gcd->orig_x;
		  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1,
					      new_y);
		}
	      else if (new_y != gcd->last_y)
		{
		  gcd->last_y = new_y;
		  gcd->last_x = new_x = (new_y * gcd->orig_x) / gcd->orig_y;
		  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0,
					      new_x);
		}
	    }
	}
      else
	{
	  if (new_x != gcd->last_x)
	    {
	      gcd->last_y = new_y = gcd->last_x = new_x;
	      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, new_x);
	    }
	  else if (new_y != gcd->last_y)
	    {
	      gcd->last_x = new_x = gcd->last_y = new_y;
	      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, new_y);
	    }
	}
    }
  else
    {
      if (new_x != gcd->last_x)
        gcd->last_x = new_x;
      if (new_y != gcd->last_y)
        gcd->last_y = new_y;
    }     
}

/**
 * gimp_coordinates_new:
 * @unit: The initial unit of the #GimpUnitMenu.
 * @unit_format: The unit format string as passed to gimp_size_entry_new().
 * @menu_show_pixels: #TRUE if the #GimpUnitMenu should contain an item for
 *                    GIMP_UNIT_PIXEL.
 * @menu_show_percent: #TRUE if the #GimpUnitMenu should contain an item for
 *                     GIMP_UNIT_PERCENT.
 * @spinbutton_usize: The horizontal usize of the #GimpSizeEntry's
 *                    #GtkSpinButton's.
 * @update_policy: The update policy for the #GimpSizeEntry.
 * @chainbutton_active: #TRUE if the attached #GimpChainButton should be
 *                      active.
 * @chain_constrains_ratio: #TRUE if the chainbutton should constrain the
 *                          fields' aspect ratio. If #FALSE, the values will
 *                          be constrained.
 * @xlabel: The label for the X coordinate.
 * @x: The initial value of the X coordinate.
 * @xres: The horizontal resolution in DPI.
 * @lower_boundary_x: The lower boundary of the X coordinate.
 * @upper_boundary_x: The upper boundary of the X coordinate.
 * @xsize_0: The X value which will be treated as 0%.
 * @xsize_100: The X value which will be treated as 100%.
 * @ylabel: The label for the Y coordinate.
 * @y: The initial value of the Y coordinate.
 * @yres: The vertical resolution in DPI.
 * @lower_boundary_y: The lower boundary of the Y coordinate.
 * @upper_boundary_y: The upper boundary of the Y coordinate.
 * @ysize_0: The Y value which will be treated as 0%.
 * @ysize_100: The Y value which will be treated as 100%.
 *
 * Returns: A #GimpSizeEntry with two fields for x/y coordinates/sizes with
 *          a #GimpChainButton attached to constrain either the two fields'
 *          values or the ratio between them.
 *
 */
GtkWidget *
gimp_coordinates_new (GimpUnit         unit,
		      gchar           *unit_format,
		      gboolean         menu_show_pixels,
		      gboolean         menu_show_percent,
		      gint             spinbutton_usize,
		      GimpSizeEntryUpdatePolicy  update_policy,

		      gboolean         chainbutton_active,
		      gboolean         chain_constrains_ratio,

		      gchar           *xlabel,
		      gdouble          x,
		      gdouble          xres,
		      gdouble          lower_boundary_x,
		      gdouble          upper_boundary_x,
		      gdouble          xsize_0,   /* % */
		      gdouble          xsize_100, /* % */

		      gchar           *ylabel,
		      gdouble          y,
		      gdouble          yres,
		      gdouble          lower_boundary_y,
		      gdouble          upper_boundary_y,
		      gdouble          ysize_0,   /* % */
		      gdouble          ysize_100  /* % */)
{
  GimpCoordinatesData *gcd;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *sizeentry;
  GtkWidget *chainbutton;

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  sizeentry = gimp_size_entry_new (1, unit, unit_format,
				   menu_show_pixels,
				   menu_show_percent,
				   FALSE,
				   spinbutton_usize,
				   update_policy);
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 0, 4);  
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 2, 4);  
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry), GIMP_UNIT_PIXEL);

  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 0, xres, TRUE);
  gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (sizeentry), 1, yres, TRUE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 0,
					 lower_boundary_x, upper_boundary_x);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (sizeentry), 1,
					 lower_boundary_y, upper_boundary_y);

  if (menu_show_percent)
    {
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 0,
				xsize_0, xsize_100);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (sizeentry), 1,
				ysize_0, ysize_100);
    }

  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 0, x);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (sizeentry), 1, y);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry), xlabel, 0, 0, 1.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry), ylabel, 1, 0, 1.0);

  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  if (chainbutton_active)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainbutton), TRUE);
  gtk_table_attach (GTK_TABLE (sizeentry), chainbutton, 2, 3, 0, 2,
		    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chainbutton);

  gcd = g_new (GimpCoordinatesData, 1);
  gcd->chainbutton            = GIMP_CHAIN_BUTTON (chainbutton);
  gcd->chain_constrains_ratio = chain_constrains_ratio;
  gcd->orig_x                 = x;
  gcd->orig_y                 = y;
  gcd->last_x                 = x;
  gcd->last_y                 = y;

  gtk_signal_connect_object (GTK_OBJECT (sizeentry), "destroy",
			     GTK_SIGNAL_FUNC (g_free),
			     (GtkObject *) gcd);

  gtk_signal_connect (GTK_OBJECT (sizeentry), "value_changed", 
                      GTK_SIGNAL_FUNC (gimp_coordinates_callback),
                      gcd);

  gtk_object_set_data (GTK_OBJECT (sizeentry), "chainbutton", chainbutton);

  return sizeentry;
}

/**
 * gimp_pixmap_button_new:
 * @xpm_data: The XPM data which will be passed to gimp_pixmap_new().
 * @text: An optional text which will appear right of the pixmap.
 *
 * Returns: A #GtkButton with a #GimpPixmap and an optional #GtkLabel.
 *
 */
GtkWidget *
gimp_pixmap_button_new (gchar **xpm_data,
			gchar  *text)
{
  GtkWidget *button;
  GtkWidget *pixmap;

  button = gtk_button_new ();
  pixmap = gimp_pixmap_new (xpm_data);

  if (text)
    {
      GtkWidget *abox;
      GtkWidget *hbox;
      GtkWidget *label;

      abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (button), abox);
      gtk_widget_show (abox);

      hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (abox), hbox);
      gtk_widget_show (hbox);

      gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 4);
      gtk_widget_show (pixmap);

      label = gtk_label_new (text);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 4);
      gtk_widget_show (label);
    }
  else
    {
      gtk_container_add (GTK_CONTAINER (button), pixmap);
      gtk_widget_show (pixmap);
    }


  return button;
}

/*
 *  Standard Callbacks
 */

/**
 * gimp_toggle_button_sensitive_update:
 * @toggle_button: The #GtkToggleButton the "set_sensitive" and
 *                 "inverse_sensitive" lists are attached to.
 *
 * If you attached a pointer to a #GtkWidget with gtk_object_set_data() and
 * the "set_sensitive" key to the #GtkToggleButton, the sensitive state of
 * the attached widget will be set according to the toggle button's
 * "active" state.
 *
 * You can attach an arbitrary list of widgets by attaching another
 * "set_sensitive" data pointer to the first widget (and so on...).
 *
 * This function can also set the sensitive state according to the toggle
 * button's inverse "active" state by attaching widgets with the
 * "inverse_sensitive" key.
 *
 */
void
gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button)
{
  GtkWidget *set_sensitive;
  gboolean   active;

  active = gtk_toggle_button_get_active (toggle_button);

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "set_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "set_sensitive");
    }

  set_sensitive =
    gtk_object_get_data (GTK_OBJECT (toggle_button), "inverse_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, ! active);
      set_sensitive =
        gtk_object_get_data (GTK_OBJECT (set_sensitive), "inverse_sensitive");
    }
}

/**
 * gimp_toggle_button_update:
 * @widget: A #GtkToggleButton.
 * @data: A pointer to a #gint variable which will store the value of
 *        gtk_toggle_button_get_active().
 *
 * Note that this function calls gimp_toggle_button_sensitive_update().
 *
 */
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

/**
 * gimp_radio_button_update:
 * @widget: A #GtkRadioButton.
 * @data: A pointer to a #gint variable which will store the value of
 *        (#gint) gtk_object_get_user_data().
 *
 * Note that this function calls gimp_toggle_button_sensitive_update().
 *
 */
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

/**
 * gimp_menu_item_update:
 * @widget: A #GtkMenuItem.
 * @data: A pointer to a #gint variable which will store the value of
 *        (#gint) gtk_object_get_user_data().
 *
 */
void
gimp_menu_item_update (GtkWidget *widget,
		       gpointer   data)
{
  gint *item_val;

  item_val = (gint *) data;

  *item_val = (gint) gtk_object_get_user_data (GTK_OBJECT (widget));
}

/**
 * gimp_int_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data: A pointer to a #gint variable which will store the adjustment's
 *        value.
 *
 * Note that the #GtkAdjustment's value (which is a #gfloat) will be rounded
 * with (#gint) (value + 0.5).
 *
 */
void
gimp_int_adjustment_update (GtkAdjustment *adjustment,
			    gpointer       data)
{
  gint *val;

  val = (gint *) data;
  *val = (gint) (adjustment->value + 0.5);
}

/**
 * gimp_float_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data: A pointer to a #gfloat varaiable which willl store the adjustment's
 *        value.
 *
 */
void
gimp_float_adjustment_update (GtkAdjustment *adjustment,
			      gpointer       data)
{
  gfloat *val;

  val = (gfloat *) data;
  *val = adjustment->value;
}

/**
 * gimp_double_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data: A pointer to a #gdouble variable which will store the adjustment's
 *        value.
 *
 */
void
gimp_double_adjustment_update (GtkAdjustment *adjustment,
			       gpointer       data)
{
  gdouble *val;

  val = (gdouble *) data;
  *val = adjustment->value;
}

/**
 * gimp_unit_menu_update:
 * @widget: A #GimpUnitMenu.
 * @data: A pointer to a #GimpUnit variable which will store the unit menu's
 *        value.
 *
 * This callback can set the number of decimal digits of an arbitrary number
 * of #GtkSpinButton's. To use this functionality, attach the spinbuttons
 * as list of data pointers attached with gtk_object_set_data() with the
 * "set_digits" key.
 *
 * See gimp_toggle_button_sensitive_update() for a description of how
 * to set up the list.
 *
 */
void
gimp_unit_menu_update (GtkWidget *widget,
		       gpointer   data)
{
  GimpUnit  *val;
  GtkWidget *spinbutton;
  gint       digits;

  val = (GimpUnit *) data;
  *val = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  digits = ((*val == GIMP_UNIT_PIXEL) ? 0 :
	    ((*val == GIMP_UNIT_PERCENT) ? 2 :
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

/**
 * gimp_table_attach_aligned:
 * @table: The #GtkTable the widgets will be attached to.
 * @column: The column to start with.
 * @row: The row to attach the eidgets.
 * @label_text: The text for the #GtkLabel which will be attached left of the
 *              widget.
 * @xalign: The horizontal alignment of the #GtkLabel.
 * @yalign: The vertival alignment of the #GtkLabel.
 * @widget: The #GtkWidget to attach right of the label.
 * @colspan: The number of columns the widget will use.
 * @left_align: #TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be #NULL and that the widget will be attached
 * starting at (@column + 1) in this case, too.
 *
 */
void
gimp_table_attach_aligned (GtkTable  *table,
			   gint       column,
			   gint       row,
			   gchar     *label_text,
			   gfloat     xalign,
			   gfloat     yalign,
			   GtkWidget *widget,
			   gint       colspan,
			   gboolean   left_align)
{
  if (label_text)
    {
      GtkWidget *label;

      label = gtk_label_new (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, label,
			column, column + 1,
			row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
    }

  if (left_align)
    {
      GtkWidget *alignment;

      alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
      gtk_container_add (GTK_CONTAINER (alignment), widget);
      gtk_widget_show (widget);

      widget = alignment;
    }

  gtk_table_attach (table, widget,
		    column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (widget);
}
