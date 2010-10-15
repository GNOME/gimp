/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpwidgets.c
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpwidgets
 * @title: GimpWidgets
 * @short_description: A collection of convenient widget constructors,
 *                     standard callbacks and helper functions.
 *
 * A collection of convenient widget constructors, standard callbacks
 * and helper functions.
 **/


/**
 * gimp_radio_group_new:
 * @in_frame:    %TRUE if you want a #GtkFrame around the radio button group.
 * @frame_title: The title of the Frame or %NULL if you don't want a title.
 * @...:         A %NULL-terminated @va_list describing the radio buttons.
 *
 * Convenience function to create a group of radio buttons embedded into
 * a #GtkFrame or #GtkVBox.
 *
 * Returns: A #GtkFrame or #GtkVBox (depending on @in_frame).
 **/
GtkWidget *
gimp_radio_group_new (gboolean            in_frame,
                      const gchar        *frame_title,

                      /* specify radio buttons as va_list:
                       *  const gchar    *label,
                       *  GCallback       callback,
                       *  gpointer        callback_data,
                       *  gpointer        item_data,
                       *  GtkWidget     **widget_ptr,
                       *  gboolean        active,
                       */

                      ...)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  const gchar  *label;
  GCallback     callback;
  gpointer      callback_data;
  gpointer      item_data;
  GtkWidget   **widget_ptr;
  gboolean      active;

  va_list args;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  group = NULL;

  /*  create the radio buttons  */
  va_start (args, frame_title);
  label = va_arg (args, const gchar *);
  while (label)
    {
      callback      = va_arg (args, GCallback);
      callback_data = va_arg (args, gpointer);
      item_data     = va_arg (args, gpointer);
      widget_ptr    = va_arg (args, GtkWidget **);
      active        = va_arg (args, gboolean);

      if (label != (gpointer) 1)
        button = gtk_radio_button_new_with_mnemonic (group, label);
      else
        button = gtk_radio_button_new (group);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      if (item_data)
        {
          g_object_set_data (G_OBJECT (button), "gimp-item-data", item_data);

          /*  backward compatibility  */
          g_object_set_data (G_OBJECT (button), "user_data", item_data);
        }

      if (widget_ptr)
        *widget_ptr = button;

      if (active)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "toggled",
                        callback,
                        callback_data);

      gtk_widget_show (button);

      label = va_arg (args, const gchar *);
    }
  va_end (args);

  if (in_frame)
    {
      GtkWidget *frame;

      frame = gimp_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      return frame;
    }

  return vbox;
}

/**
 * gimp_radio_group_new2:
 * @in_frame:              %TRUE if you want a #GtkFrame around the
 *                         radio button group.
 * @frame_title:           The title of the Frame or %NULL if you don't want
 *                         a title.
 * @radio_button_callback: The callback each button's "toggled" signal will
 *                         be connected with.
 * @radio_button_callback_data:
 *                         The data which will be passed to g_signal_connect().
 * @initial:               The @item_data of the initially pressed radio button.
 * @...:                   A %NULL-terminated @va_list describing
 *                         the radio buttons.
 *
 * Convenience function to create a group of radio buttons embedded into
 * a #GtkFrame or #GtkVBox.
 *
 * Returns: A #GtkFrame or #GtkVBox (depending on @in_frame).
 **/
GtkWidget *
gimp_radio_group_new2 (gboolean         in_frame,
                       const gchar     *frame_title,
                       GCallback        radio_button_callback,
                       gpointer         callback_data,
                       gpointer         initial, /* item_data */

                       /* specify radio buttons as va_list:
                        *  const gchar *label,
                        *  gpointer     item_data,
                        *  GtkWidget  **widget_ptr,
                        */

                       ...)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  const gchar *label;
  gpointer     item_data;
  GtkWidget  **widget_ptr;

  va_list args;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  group = NULL;

  /*  create the radio buttons  */
  va_start (args, initial);
  label = va_arg (args, const gchar *);

  while (label)
    {
      item_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);

      if (label != (gpointer) 1)
        button = gtk_radio_button_new_with_mnemonic (group, label);
      else
        button = gtk_radio_button_new (group);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      if (item_data)
        {
          g_object_set_data (G_OBJECT (button), "gimp-item-data", item_data);

          /*  backward compatibility  */
          g_object_set_data (G_OBJECT (button), "user_data", item_data);
        }

      if (widget_ptr)
        *widget_ptr = button;

      if (initial == item_data)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "toggled",
                        radio_button_callback,
                        callback_data);

      gtk_widget_show (button);

      label = va_arg (args, const gchar *);
    }
  va_end (args);

  if (in_frame)
    {
      GtkWidget *frame;

      frame = gimp_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      return frame;
    }

  return vbox;
}

/**
 * gimp_int_radio_group_new:
 * @in_frame:              %TRUE if you want a #GtkFrame around the
 *                         radio button group.
 * @frame_title:           The title of the Frame or %NULL if you don't want
 *                         a title.
 * @radio_button_callback: The callback each button's "toggled" signal will
 *                         be connected with.
 * @radio_button_callback_data:
 *                         The data which will be passed to g_signal_connect().
 * @initial:               The @item_data of the initially pressed radio button.
 * @...:                   A %NULL-terminated @va_list describing
 *                         the radio buttons.
 *
 * Convenience function to create a group of radio buttons embedded into
 * a #GtkFrame or #GtkVBox. This function does the same thing as
 * gimp_radio_group_new2(), but it takes integers as @item_data instead of
 * pointers, since that is a very common case (mapping an enum to a radio
 * group).
 *
 * Returns: A #GtkFrame or #GtkVBox (depending on @in_frame).
 **/
GtkWidget *
gimp_int_radio_group_new (gboolean         in_frame,
                          const gchar     *frame_title,
                          GCallback        radio_button_callback,
                          gpointer         callback_data,
                          gint             initial, /* item_data */

                          /* specify radio buttons as va_list:
                           *  const gchar *label,
                           *  gint         item_data,
                           *  GtkWidget  **widget_ptr,
                           */

                          ...)
{
  GtkWidget *vbox;
  GtkWidget *button;
  GSList    *group;

  /*  radio button variables  */
  const gchar *label;
  gint         item_data;
  gpointer     item_ptr;
  GtkWidget  **widget_ptr;

  va_list args;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  group = NULL;

  /*  create the radio buttons  */
  va_start (args, initial);
  label = va_arg (args, const gchar *);

  while (label)
    {
      item_data  = va_arg (args, gint);
      widget_ptr = va_arg (args, GtkWidget **);

      item_ptr = GINT_TO_POINTER (item_data);

      if (label != GINT_TO_POINTER (1))
        button = gtk_radio_button_new_with_mnemonic (group, label);
      else
        button = gtk_radio_button_new (group);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

      if (item_data)
        {
          g_object_set_data (G_OBJECT (button), "gimp-item-data", item_ptr);

          /*  backward compatibility  */
          g_object_set_data (G_OBJECT (button), "user_data", item_ptr);
        }

      if (widget_ptr)
        *widget_ptr = button;

      if (initial == item_data)
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

      g_signal_connect (button, "toggled",
                        radio_button_callback,
                        callback_data);

      gtk_widget_show (button);

      label = va_arg (args, const gchar *);
    }
  va_end (args);

  if (in_frame)
    {
      GtkWidget *frame;

      frame = gimp_frame_new (frame_title);
      gtk_container_add (GTK_CONTAINER (frame), vbox);
      gtk_widget_show (vbox);

      return frame;
    }

  return vbox;
}

/**
 * gimp_radio_group_set_active:
 * @radio_button: Pointer to a #GtkRadioButton.
 * @item_data: The @item_data of the radio button you want to select.
 *
 * Calls gtk_toggle_button_set_active() with the radio button that was
 * created with a matching @item_data.
 **/
void
gimp_radio_group_set_active (GtkRadioButton *radio_button,
                             gpointer        item_data)
{
  GtkWidget *button;
  GSList    *group;

  g_return_if_fail (GTK_IS_RADIO_BUTTON (radio_button));

  for (group = gtk_radio_button_get_group (radio_button);
       group;
       group = g_slist_next (group))
    {
      button = GTK_WIDGET (group->data);

      if (g_object_get_data (G_OBJECT (button), "gimp-item-data") == item_data)
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
          return;
        }
    }
}

/**
 * gimp_int_radio_group_set_active:
 * @radio_button: Pointer to a #GtkRadioButton.
 * @item_data: The @item_data of the radio button you want to select.
 *
 * Calls gtk_toggle_button_set_active() with the radio button that was created
 * with a matching @item_data. This function does the same thing as
 * gimp_radio_group_set_active(), but takes integers as @item_data instead
 * of pointers.
 **/
void
gimp_int_radio_group_set_active (GtkRadioButton *radio_button,
                                 gint            item_data)
{
  g_return_if_fail (GTK_IS_RADIO_BUTTON (radio_button));

  gimp_radio_group_set_active (radio_button, GINT_TO_POINTER (item_data));
}

/**
 * gimp_spin_button_new:
 * @adjustment:     Returns the spinbutton's #GtkAdjustment.
 * @value:          The initial value of the spinbutton.
 * @lower:          The lower boundary.
 * @upper:          The upper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size:      Ignored, spin buttons must always have a zero page size.
 * @climb_rate:     The spinbutton's climb rate.
 * @digits:         The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a
 * subsequent gtk_spin_button_new(). It also calls
 * gtk_spin_button_set_numeric() so that non-numeric text cannot be
 * entered.
 *
 * Deprecated: 2.10: Use gtk_spin_button_new() instead.
 *
 * Returns: A #GtkSpinButton and its #GtkAdjustment.
 **/
GtkWidget *
gimp_spin_button_new (GtkAdjustment **adjustment,  /* return value */
                      gdouble         value,
                      gdouble         lower,
                      gdouble         upper,
                      gdouble         step_increment,
                      gdouble         page_increment,
                      gdouble         page_size,
                      gdouble         climb_rate,
                      guint           digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
                                    step_increment, page_increment, 0);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
                                    climb_rate, digits);

  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  return spinbutton;
}

static void
gimp_random_seed_update (GtkWidget *widget,
                         gpointer   data)
{
  GtkWidget *spinbutton = data;

  /* Generate a new seed if the "New Seed" button was clicked or
   * of the "Randomize" toggle is activated
   */
  if (! GTK_IS_TOGGLE_BUTTON (widget) ||
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (spinbutton),
                                 (guint) g_random_int ());
    }
}

/**
 * gimp_random_seed_new:
 * @seed:        A pointer to the variable which stores the random seed.
 * @random_seed: A pointer to a boolean indicating whether seed should be
 *               initialised randomly or not.
 *
 * Creates a widget that allows the user to control how the random number
 * generator is initialized.
 *
 * Returns: A #GtkHBox containing a #GtkSpinButton for the seed and
 *          a #GtkButton for setting a random seed.
 **/
GtkWidget *
gimp_random_seed_new (guint    *seed,
                      gboolean *random_seed)
{
  GtkWidget     *hbox;
  GtkWidget     *toggle;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;
  GtkWidget     *button;

  g_return_val_if_fail (seed != NULL, NULL);
  g_return_val_if_fail (random_seed != NULL, NULL);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);

  /* If we're being asked to generate a random seed, generate one. */
  if (*random_seed)
    *seed = g_random_int ();

  adj = (GtkAdjustment *)
    gtk_adjustment_new (*seed, 0, (guint32) -1, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adj, 1.0, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (gimp_uint_adjustment_update),
                    seed);

  gimp_help_set_help_data (spinbutton,
                           _("Use this value for random number generator "
                             "seed - this allows you to repeat a "
                             "given \"random\" operation"), NULL);

  button = gtk_button_new_with_mnemonic (_("_New Seed"));
  gtk_misc_set_padding (GTK_MISC (gtk_bin_get_child (GTK_BIN (button))), 2, 0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Send spinbutton as data so that we can change the value in
   * gimp_random_seed_update()
   */
  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_random_seed_update),
                    spinbutton);

  gimp_help_set_help_data (button,
                           _("Seed random number generator with a generated "
                             "random number"),
                           NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("_Randomize"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *random_seed);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    random_seed);

  /* Need to create a new seed when the "Randomize" toggle is activated  */
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_random_seed_update),
                    spinbutton);

  g_object_set_data (G_OBJECT (hbox), "spinbutton", spinbutton);
  g_object_set_data (G_OBJECT (hbox), "button", button);
  g_object_set_data (G_OBJECT (hbox), "toggle", toggle);

  g_object_bind_property (toggle,     "active",
                          spinbutton, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  g_object_bind_property (toggle, "active",
                          button, "sensitive",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);

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
gimp_coordinates_callback (GtkWidget           *widget,
                           GimpCoordinatesData *data)
{
  gdouble new_x;
  gdouble new_y;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (data->chainbutton))
    {
      if (data->chain_constrains_ratio)
        {
          if ((data->orig_x != 0) && (data->orig_y != 0))
            {
              g_signal_handlers_block_by_func (widget,
                                               gimp_coordinates_callback,
                                               data);

              if (ROUND (new_x) != ROUND (data->last_x))
                {
                  data->last_x = new_x;
                  new_y = (new_x * data->orig_y) / data->orig_x;

                  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1,
                                              new_y);
                  data->last_y
                    = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);
                }
              else if (ROUND (new_y) != ROUND (data->last_y))
                {
                  data->last_y = new_y;
                  new_x = (new_y * data->orig_x) / data->orig_y;

                  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0,
                                              new_x);
                  data->last_x
                    = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
                }

              g_signal_handlers_unblock_by_func (widget,
                                                 gimp_coordinates_callback,
                                                 data);
            }
        }
      else
        {
          if (new_x != data->last_x)
            {
              gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, new_x);
              data->last_y = data->last_x
                = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);
            }
          else if (new_y != data->last_y)
            {
              gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 0, new_y);
              data->last_x = data->last_y
                = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
            }
        }
    }
  else
    {
      if (new_x != data->last_x)
        data->last_x = new_x;
      if (new_y != data->last_y)
        data->last_y = new_y;
    }
}

static void
gimp_coordinates_data_free (GimpCoordinatesData *data)
{
  g_slice_free (GimpCoordinatesData, data);
}

static void
gimp_coordinates_chainbutton_toggled (GimpChainButton *button,
                                      GimpSizeEntry   *entry)
{
  if (gimp_chain_button_get_active (button))
    {
      GimpCoordinatesData *data;

      data = g_object_get_data (G_OBJECT (entry), "coordinates-data");

      data->orig_x = gimp_size_entry_get_refval (entry, 0);
      data->orig_y = gimp_size_entry_get_refval (entry, 1);
    }
}

/**
 * gimp_coordinates_new:
 * @unit:                   The initial unit of the #GimpUnitMenu.
 * @unit_format:            A printf-like unit-format string as is used with
 *                          gimp_unit_menu_new().
 * @menu_show_pixels:       %TRUE if the #GimpUnitMenu should contain an item
 *                          for GIMP_UNIT_PIXEL.
 * @menu_show_percent:      %TRUE if the #GimpUnitMenu should contain an item
 *                          for GIMP_UNIT_PERCENT.
 * @spinbutton_width:       The horizontal size of the #GimpSizeEntry's
 *                           #GtkSpinButton's.
 * @update_policy:          The update policy for the #GimpSizeEntry.
 * @chainbutton_active:     %TRUE if the attached #GimpChainButton should be
 *                          active.
 * @chain_constrains_ratio: %TRUE if the chainbutton should constrain the
 *                          fields' aspect ratio. If %FALSE, the values will
 *                          be constrained.
 * @xlabel:                 The label for the X coordinate.
 * @x:                      The initial value of the X coordinate.
 * @xres:                   The horizontal resolution in DPI.
 * @lower_boundary_x:       The lower boundary of the X coordinate.
 * @upper_boundary_x:       The upper boundary of the X coordinate.
 * @xsize_0:                The X value which will be treated as 0%.
 * @xsize_100:              The X value which will be treated as 100%.
 * @ylabel:                 The label for the Y coordinate.
 * @y:                      The initial value of the Y coordinate.
 * @yres:                   The vertical resolution in DPI.
 * @lower_boundary_y:       The lower boundary of the Y coordinate.
 * @upper_boundary_y:       The upper boundary of the Y coordinate.
 * @ysize_0:                The Y value which will be treated as 0%.
 * @ysize_100:              The Y value which will be treated as 100%.
 *
 * Convenience function that creates a #GimpSizeEntry with two fields for x/y
 * coordinates/sizes with a #GimpChainButton attached to constrain either the
 * two fields' values or the ratio between them.
 *
 * Returns: The new #GimpSizeEntry.
 **/
GtkWidget *
gimp_coordinates_new (GimpUnit         unit,
                      const gchar     *unit_format,
                      gboolean         menu_show_pixels,
                      gboolean         menu_show_percent,
                      gint             spinbutton_width,
                      GimpSizeEntryUpdatePolicy  update_policy,

                      gboolean         chainbutton_active,
                      gboolean         chain_constrains_ratio,

                      const gchar     *xlabel,
                      gdouble          x,
                      gdouble          xres,
                      gdouble          lower_boundary_x,
                      gdouble          upper_boundary_x,
                      gdouble          xsize_0,   /* % */
                      gdouble          xsize_100, /* % */

                      const gchar     *ylabel,
                      gdouble          y,
                      gdouble          yres,
                      gdouble          lower_boundary_y,
                      gdouble          upper_boundary_y,
                      gdouble          ysize_0,   /* % */
                      gdouble          ysize_100  /* % */)
{
  GimpCoordinatesData *data;
  GtkAdjustment       *adjustment;
  GtkWidget           *spinbutton;
  GtkWidget           *sizeentry;
  GtkWidget           *chainbutton;

  adjustment = (GtkAdjustment *) gtk_adjustment_new (1, 0, 1, 1, 10, 0);
  spinbutton = gtk_spin_button_new (adjustment, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), spinbutton_width);
      else
        gtk_widget_set_size_request (spinbutton, spinbutton_width, -1);
    }

  sizeentry = gimp_size_entry_new (1, unit, unit_format,
                                   menu_show_pixels,
                                   menu_show_percent,
                                   FALSE,
                                   spinbutton_width,
                                   update_policy);
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 0, 4);
  gtk_table_set_col_spacing (GTK_TABLE (sizeentry), 2, 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (sizeentry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (sizeentry), spinbutton, 1, 2, 0, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_set_unit (GIMP_SIZE_ENTRY (sizeentry),
                            (update_policy == GIMP_SIZE_ENTRY_UPDATE_RESOLUTION) ||
                            (menu_show_pixels == FALSE) ?
                            GIMP_UNIT_INCH : GIMP_UNIT_PIXEL);

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

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
                                xlabel, 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (sizeentry),
                                ylabel, 1, 0, 0.0);

  chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

  if (chainbutton_active)
    gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chainbutton), TRUE);

  gtk_table_attach (GTK_TABLE (sizeentry), chainbutton, 2, 3, 0, 2,
                    GTK_SHRINK | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (chainbutton);

  data = g_slice_new (GimpCoordinatesData);

  data->chainbutton            = GIMP_CHAIN_BUTTON (chainbutton);
  data->chain_constrains_ratio = chain_constrains_ratio;
  data->orig_x                 = x;
  data->orig_y                 = y;
  data->last_x                 = x;
  data->last_y                 = y;

  g_object_set_data_full (G_OBJECT (sizeentry), "coordinates-data",
                          data,
                          (GDestroyNotify) gimp_coordinates_data_free);

  g_signal_connect (sizeentry, "value-changed",
                    G_CALLBACK (gimp_coordinates_callback),
                    data);

  g_object_set_data (G_OBJECT (sizeentry), "chainbutton", chainbutton);

  g_signal_connect (chainbutton, "toggled",
                    G_CALLBACK (gimp_coordinates_chainbutton_toggled),
                    sizeentry);

  return sizeentry;
}


/*
 *  Standard Callbacks
 */

/**
 * gimp_toggle_button_update:
 * @widget: A #GtkToggleButton.
 * @data:   A pointer to a #gint variable which will store the value of
 *          gtk_toggle_button_get_active().
 **/
void
gimp_toggle_button_update (GtkWidget *widget,
                           gpointer   data)
{
  gint *toggle_val = (gint *) data;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

/**
 * gimp_radio_button_update:
 * @widget: A #GtkRadioButton.
 * @data:   A pointer to a #gint variable which will store the value of
 *          GPOINTER_TO_INT (g_object_get_data (@widget, "gimp-item-data")).
 **/
void
gimp_radio_button_update (GtkWidget *widget,
                          gpointer   data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      gint *toggle_val = (gint *) data;

      *toggle_val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                        "gimp-item-data"));
    }
}

/**
 * gimp_int_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gint variable which will store the
 *              @adjustment's value.
 *
 * Note that the #GtkAdjustment's value (which is a #gdouble) will be
 * rounded with RINT().
 **/
void
gimp_int_adjustment_update (GtkAdjustment *adjustment,
                            gpointer       data)
{
  gint *val = (gint *) data;

  *val = RINT (gtk_adjustment_get_value (adjustment));
}

/**
 * gimp_uint_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #guint variable which will store the
 *              @adjustment's value.
 *
 * Note that the #GtkAdjustment's value (which is a #gdouble) will be rounded
 * with (#guint) (value + 0.5).
 **/
void
gimp_uint_adjustment_update (GtkAdjustment *adjustment,
                             gpointer       data)
{
  guint *val = (guint *) data;

  *val = (guint) (gtk_adjustment_get_value (adjustment) + 0.5);
}

/**
 * gimp_float_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gfloat variable which will store the
 *              @adjustment's value.
 **/
void
gimp_float_adjustment_update (GtkAdjustment *adjustment,
                              gpointer       data)
{
  gfloat *val = (gfloat *) data;

  *val = gtk_adjustment_get_value (adjustment);

}

/**
 * gimp_double_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gdouble variable which will store the
 *              @adjustment's value.
 **/
void
gimp_double_adjustment_update (GtkAdjustment *adjustment,
                               gpointer       data)
{
  gdouble *val = (gdouble *) data;

  *val = gtk_adjustment_get_value (adjustment);
}
