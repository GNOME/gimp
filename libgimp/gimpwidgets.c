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

static void
gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
						    GtkAdjustment *other_adj)
{
  gtk_signal_handler_block_by_data (GTK_OBJECT (other_adj), adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  gtk_signal_handler_unblock_by_data (GTK_OBJECT (other_adj), adjustment);
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
		      gboolean  constrain,
		      gfloat    unconstrained_lower,
		      gfloat    unconstrained_upper,
		      gchar    *tooltip,
		      gchar    *private_tip)
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

  if (tooltip || private_tip)
    {
      gimp_help_set_help_data (scale, tooltip, private_tip);
      gimp_help_set_help_data (spinbutton, tooltip, private_tip);
    }

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

GtkWidget *
gimp_random_seed_new (gint       *seed,
		      GtkWidget **seed_spinbutton,
		      gint       *use_time,
		      GtkWidget **time_button,
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

  if (seed_spinbutton)
    *seed_spinbutton = spinbutton;

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

  if (time_button)
    *time_button = button;

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

GtkWidget *
gimp_coordinates_new (GimpUnit         unit,
		      gchar           *unit_format,
		      gboolean         menu_show_pixels,
		      gboolean         menu_show_percent,
		      gint             spinbutton_usize,
		      GimpSizeEntryUpdatePolicy  update_policy,

		      gboolean         chainbutton_active,
		      gboolean         chain_constrains_ratio,
		      /* return value: */
		      GtkWidget      **chainbutton,

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
  GtkWidget *chainb;

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

  chainb = gimp_chain_button_new (GIMP_CHAIN_RIGHT);
  if (chainbutton_active)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainb), TRUE);
  gtk_table_attach (GTK_TABLE (sizeentry), chainb, 2, 3, 0, 2,
		    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chainb);

  if (chainbutton)
    *chainbutton = chainb;

  gcd = g_new (GimpCoordinatesData, 1);
  gcd->chainbutton            = GIMP_CHAIN_BUTTON (chainb);
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

  return sizeentry;
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

/*  add aligned label & widget to a table  */
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
