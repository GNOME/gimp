/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpchainbutton.h"
#include "gimppixmap.h"
#include "gimpsizeentry.h"
#include "gimpunitmenu.h"
#include "gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


/*
 *  Widget Constructors
 */

/**
 * gimp_option_menu_new:
 * @menu_only: %TRUE if the function should return a #GtkMenu only.
 * @...:       A %NULL-terminated @va_list describing the menu items.
 *
 * Convenience function to create a #GtkOptionMenu or a #GtkMenu.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 **/
GtkWidget *
gimp_option_menu_new (gboolean            menu_only,

		      /* specify menu items as va_list:
		       *  const gchar    *label,
		       *  GCallback       callback,
		       *  gpointer        callback_data,
		       *  gpointer        item_data,
		       *  GtkWidget     **widget_ptr,
		       *  gboolean        active
		       */

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  const gchar    *label;
  GCallback       callback;
  gpointer        callback_data;
  gpointer        item_data;
  GtkWidget     **widget_ptr;
  gboolean        active;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;

  va_start (args, menu_only);
  label = va_arg (args, const gchar *);

  for (i = 0; label; i++)
    {
      callback      = va_arg (args, GCallback);
      callback_data = va_arg (args, gpointer);
      item_data     = va_arg (args, gpointer);
      widget_ptr    = va_arg (args, GtkWidget **);
      active        = va_arg (args, gboolean);

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  g_signal_connect (menuitem, "activate",
			    callback,
			    callback_data);

	  if (item_data)
            {
              g_object_set_data (G_OBJECT (menuitem), "gimp-item-data",
                                 item_data);

              /*  backward compat  */
              g_object_set_data (G_OBJECT (menuitem), "user_data", item_data);
            }
	}
      else
	{
	  menuitem = gtk_menu_item_new ();

	  gtk_widget_set_sensitive (menuitem, FALSE);
	}

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (active)
	initial_index = i;

      label = va_arg (args, const gchar *);
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
 * @menu_only:          %TRUE if the function should return a #GtkMenu only.
 * @menu_item_callback: The callback each menu item's "activate" signal will
 *                      be connected with.
 * @menu_item_callback_data:
 *                      The data which will be passed to g_signal_connect().
 * @initial:            The @item_data of the initially selected menu item.
 * @...:                A %NULL-terminated @va_list describing the menu items.
 *
 * Convenience function to create a #GtkOptionMenu or a #GtkMenu.
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 **/
GtkWidget *
gimp_option_menu_new2 (gboolean         menu_only,
		       GCallback        menu_item_callback,
		       gpointer         callback_data,
		       gpointer         initial, /* item_data */

		       /* specify menu items as va_list:
			*  const gchar *label,
			*  gpointer     item_data,
			*  GtkWidget  **widget_ptr,
			*/

		       ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  const gchar  *label;
  gpointer      item_data;
  GtkWidget   **widget_ptr;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;

  va_start (args, initial);
  label = va_arg (args, const gchar *);

  for (i = 0; label; i++)
    {
      item_data  = va_arg (args, gpointer);
      widget_ptr = va_arg (args, GtkWidget **);

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  g_signal_connect (menuitem, "activate",
			    menu_item_callback,
			    callback_data);

	  if (item_data)
            {
              g_object_set_data (G_OBJECT (menuitem), "gimp-item-data",
                                 item_data);

              /*  backward compat  */
              g_object_set_data (G_OBJECT (menuitem), "user_data", item_data);
            }
	}
      else
	{
	  menuitem = gtk_menu_item_new ();

	  gtk_widget_set_sensitive (menuitem, FALSE);
	}

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (item_data == initial)
	initial_index = i;

      label = va_arg (args, const gchar *);
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
 * gimp_int_option_menu_new:
 * @menu_only:          %TRUE if the function should return a #GtkMenu only.
 * @menu_item_callback: The callback each menu item's "activate" signal will
 *                      be connected with.
 * @menu_item_callback_data:
 *                      The data which will be passed to g_signal_connect().
 * @initial:            The @item_data of the initially selected menu item.
 * @...:                A %NULL-terminated @va_list describing the menu items.
 *
 * Convenience function to create a #GtkOptionMenu or a #GtkMenu. This
 * function does the same thing as the deprecated function
 * gimp_option_menu_new2(), but it takes integers as @item_data
 * instead of pointers, since that is a very common case (mapping an
 * enum to a menu).
 *
 * Returns: A #GtkOptionMenu or a #GtkMenu (depending on @menu_only).
 **/
GtkWidget *
gimp_int_option_menu_new (gboolean         menu_only,
		          GCallback        menu_item_callback,
		          gpointer         callback_data,
		          gint             initial, /* item_data */

		          /* specify menu items as va_list:
			   *  const gchar *label,
			   *  gint         item_data,
			   *  GtkWidget  **widget_ptr,
			   */

		          ...)
{
  GtkWidget *menu;
  GtkWidget *menuitem;

  /*  menu item variables  */
  const gchar  *label;
  gint          item_data;
  gpointer      item_ptr;
  GtkWidget   **widget_ptr;

  va_list args;
  gint    i;
  gint    initial_index;

  menu = gtk_menu_new ();

  /*  create the menu items  */
  initial_index = 0;

  va_start (args, initial);
  label = va_arg (args, const gchar *);

  for (i = 0; label; i++)
    {
      item_data  = va_arg (args, gint);
      widget_ptr = va_arg (args, GtkWidget **);

      item_ptr = GINT_TO_POINTER (item_data);

      if (strcmp (label, "---"))
	{
	  menuitem = gtk_menu_item_new_with_label (label);

	  g_signal_connect (menuitem, "activate",
			    menu_item_callback,
			    callback_data);

	  if (item_data)
            {
              g_object_set_data (G_OBJECT (menuitem), "gimp-item-data",
                                 item_ptr);

              /*  backward compat  */
              g_object_set_data (G_OBJECT (menuitem), "user_data", item_ptr);
            }
	}
      else
	{
	  menuitem = gtk_menu_item_new ();

	  gtk_widget_set_sensitive (menuitem, FALSE);
	}

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

      if (widget_ptr)
	*widget_ptr = menuitem;

      gtk_widget_show (menuitem);

      /*  remember the initial menu item  */
      if (item_data == initial)
	initial_index = i;

      label = va_arg (args, const gchar *);
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
 * @item_data:   The @item_data of the menu item you want to select.
 *
 * Iterates over all entries in a #GtkOptionMenu and selects the one
 * with the matching @item_data. Probably only makes sense to use with
 * a #GtkOptionMenu that was created using gimp_option_menu_new() or
 * gimp_option_menu_new2().
 **/
void
gimp_option_menu_set_history (GtkOptionMenu *option_menu,
			      gpointer       item_data)
{
  GtkWidget *menu_item;
  GList     *list;
  gint       history = 0;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  for (list = GTK_MENU_SHELL (option_menu->menu)->children;
       list;
       list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child) &&
	  g_object_get_data (G_OBJECT (menu_item),
                             "gimp-item-data") == item_data)
	{
	  break;
	}

      history++;
    }

  if (list)
    gtk_option_menu_set_history (option_menu, history);
}

/**
 * gimp_int_option_menu_set_history:
 * @option_menu: A #GtkOptionMenu as returned by gimp_int_option_menu_new().
 * @item_data:   The @item_data of the menu item you want to select.
 *
 * Iterates over all entries in a #GtkOptionMenu and selects the one with the
 * matching @item_data. Probably only makes sense to use with a #GtkOptionMenu
 * that was created using gimp_int_option_menu_new(). This function does the
 * same thing as gimp_option_menu_set_history(), but takes integers as
 * @item_data instead of pointers.
 **/
void
gimp_int_option_menu_set_history (GtkOptionMenu *option_menu,
			          gint           item_data)
{
  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  gimp_option_menu_set_history (option_menu, GINT_TO_POINTER (item_data));
}

/**
 * gimp_option_menu_set_sensitive:
 * @option_menu: a #GtkOptionMenu as returned by gimp_option_menu_new() or
 *            gimp_option_menu_new2().
 * @callback: a function called for each item in the menu to determine the
 *            the sensitivity state.
 * @callback_data: data to pass to the @callback function.
 *
 * Calls the given @callback for each item in the menu and passes it the
 * item_data and the @callback_data. The menu item's sensitivity is set
 * according to the return value of this function.
 **/
void
gimp_option_menu_set_sensitive (GtkOptionMenu                     *option_menu,
                                GimpOptionMenuSensitivityCallback  callback,
                                gpointer                           callback_data)
{
  GtkWidget *menu_item;
  GList     *list;
  gpointer   item_data;
  gboolean   sensitive;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (callback != NULL);

  for (list = GTK_MENU_SHELL (option_menu->menu)->children;
       list;
       list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child))
        {
          item_data = g_object_get_data (G_OBJECT (menu_item),
                                         "gimp-item-data");
          sensitive = callback (item_data, callback_data);
          gtk_widget_set_sensitive (menu_item, sensitive);
	}
    }
}

/**
 * gimp_int_option_menu_set_sensitive:
 * @option_menu: a #GtkOptionMenu as returned by gimp_option_menu_new() or
 *            gimp_option_menu_new2().
 * @callback: a function called for each item in the menu to determine the
 *            the sensitivity state.
 * @callback_data: data to pass to the @callback function.
 *
 * Calls the given @callback for each item in the menu and passes it the
 * item_data and the @callback_data. The menu item's sensitivity is set
 * according to the return value of this function. This function does the
 * same thing as gimp_option_menu_set_sensitive(), but takes integers as
 * @item_data instead of pointers.
 **/
void
gimp_int_option_menu_set_sensitive (GtkOptionMenu                        *option_menu,
                                    GimpIntOptionMenuSensitivityCallback  callback,
                                    gpointer                              callback_data)
{
  GtkWidget *menu_item;
  GList     *list;
  gint       item_data;
  gboolean   sensitive;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (callback != NULL);

  for (list = GTK_MENU_SHELL (option_menu->menu)->children;
       list;
       list = g_list_next (list))
    {
      menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (GTK_BIN (menu_item)->child))
        {
          item_data = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item),
                                                          "gimp-item-data"));
          sensitive = callback (item_data, callback_data);
          gtk_widget_set_sensitive (menu_item, sensitive);
	}
    }
}

/**
 * gimp_radio_group_new:
 * @in_frame:    %TRUE if you want a #GtkFrame around the radio button group.
 * @frame_title: The title of the Frame or %NULL if you don't want a title.
 * @...:         A %NULL-terminated @va_list describing the radio buttons.
 *
 * Convenience function to create a group of radio buttons embedded into
 * a #GtkFrame or #GtkVbox.
 *
 * Returns: A #GtkFrame or #GtkVbox (depending on @in_frame).
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

  vbox = gtk_vbox_new (FALSE, 1);

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
	button = gtk_radio_button_new_with_label (group, label);
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

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      frame = gtk_frame_new (frame_title);
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
 * a #GtkFrame or #GtkVbox.
 *
 * Returns: A #GtkFrame or #GtkVbox (depending on @in_frame).
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

  vbox = gtk_vbox_new (FALSE, 1);

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

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      frame = gtk_frame_new (frame_title);
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
 * a #GtkFrame or #GtkVbox. This function does the same thing as
 * gimp_radio_group_new2(), but it takes integers as @item_data instead of
 * pointers, since that is a very common case (mapping an enum to a radio
 * group).
 *
 * Returns: A #GtkFrame or #GtkVbox (depending on @in_frame).
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

  vbox = gtk_vbox_new (FALSE, 1);

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

      gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);

      frame = gtk_frame_new (frame_title);
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
 * @upper:          The uppper boundary.
 * @step_increment: The spinbutton's step increment.
 * @page_increment: The spinbutton's page increment (mouse button 2).
 * @page_size:      The spinbutton's page size.
 * @climb_rate:     The spinbutton's climb rate.
 * @digits:         The spinbutton's number of decimal digits.
 *
 * This function is a shortcut for gtk_adjustment_new() and a subsequent
 * gtk_spin_button_new() and does some more initialisation stuff like
 * setting a standard minimum horizontal size.
 *
 * Returns: A #GtkSpinbutton and it's #GtkAdjustment.
 **/
GtkWidget *
gimp_spin_button_new (GtkObject **adjustment,  /* return value */
		      gdouble     value,
		      gdouble     lower,
		      gdouble     upper,
		      gdouble     step_increment,
		      gdouble     page_increment,
		      gdouble     page_size,
		      gdouble     climb_rate,
		      guint       digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
				    step_increment, page_increment, page_size);

  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (*adjustment),
				    climb_rate, digits);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  return spinbutton;
}

static void
gimp_scale_entry_unconstrained_adjustment_callback (GtkAdjustment *adjustment,
						    GtkAdjustment *other_adj)
{
  g_signal_handlers_block_by_func (other_adj,
				   gimp_scale_entry_unconstrained_adjustment_callback,
				   adjustment);

  gtk_adjustment_set_value (other_adj, adjustment->value);

  g_signal_handlers_unblock_by_func (other_adj,
				     gimp_scale_entry_unconstrained_adjustment_callback,
				     adjustment);
}

static GtkObject *
gimp_scale_entry_new_internal (gboolean     color_scale,
                               GtkTable    *table,
                               gint         column,
                               gint         row,
                               const gchar *text,
                               gint         scale_width,
                               gint         spinbutton_width,
                               gdouble      value,
                               gdouble      lower,
                               gdouble      upper,
                               gdouble      step_increment,
                               gdouble      page_increment,
                               guint        digits,
                               gboolean     constrain,
                               gdouble      unconstrained_lower,
                               gdouble      unconstrained_upper,
                               const gchar *tooltip,
                               const gchar *help_id)
{
  GtkWidget *label;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  GtkObject *adjustment;
  GtkObject *return_adj;

  label = gtk_label_new_with_mnemonic (text);
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

      g_signal_connect
	(G_OBJECT (constrained_adj), "value_changed",
	 G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
	 adjustment);

      g_signal_connect
	(G_OBJECT (adjustment), "value_changed",
	 G_CALLBACK (gimp_scale_entry_unconstrained_adjustment_callback),
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

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);

  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), spinbutton_width);
      else
        gtk_widget_set_size_request (spinbutton, spinbutton_width, -1);
    }

  if (color_scale)
    {
      scale = gimp_color_scale_new (GTK_ORIENTATION_HORIZONTAL,
                                    GIMP_COLOR_SELECTOR_VALUE);

      gtk_range_set_adjustment (GTK_RANGE (scale),
                                GTK_ADJUSTMENT (adjustment));
    }
  else
    {
      scale = gtk_hscale_new (GTK_ADJUSTMENT (adjustment));
    }

  if (scale_width > 0)
    gtk_widget_set_size_request (scale, scale_width, -1);
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

  if (tooltip || help_id)
    {
      gimp_help_set_help_data (scale, tooltip, help_id);
      gimp_help_set_help_data (spinbutton, tooltip, help_id);
    }

  g_object_set_data (G_OBJECT (return_adj), "label", label);
  g_object_set_data (G_OBJECT (return_adj), "scale", scale);
  g_object_set_data (G_OBJECT (return_adj), "spinbutton", spinbutton);

  return return_adj;
}

/**
 * gimp_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_width:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_width:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @constrain:           %TRUE if the range of possible values of the
 *                       #GtkSpinButton should be the same as of the #GtkHScale.
 * @unconstrained_lower: The spinbutton's lower boundary
 *                       if @constrain == %FALSE.
 * @unconstrained_upper: The spinbutton's upper boundary
 *                       if @constrain == %FALSE.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_id:             The widgets' help_id (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GtkHScale and a #GtkSpinButton and
 * attaches them to a 3-column #GtkTable.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
gimp_scale_entry_new (GtkTable    *table,
		      gint         column,
		      gint         row,
		      const gchar *text,
		      gint         scale_width,
		      gint         spinbutton_width,
		      gdouble      value,
		      gdouble      lower,
		      gdouble      upper,
		      gdouble      step_increment,
		      gdouble      page_increment,
		      guint        digits,
		      gboolean     constrain,
		      gdouble      unconstrained_lower,
		      gdouble      unconstrained_upper,
		      const gchar *tooltip,
		      const gchar *help_id)
{
  return gimp_scale_entry_new_internal (FALSE,
                                        table, column, row,
                                        text, scale_width, spinbutton_width,
                                        value, lower, upper,
                                        step_increment, page_increment,
                                        digits,
                                        constrain,
                                        unconstrained_lower,
                                        unconstrained_upper,
                                        tooltip, help_id);
}

/**
 * gimp_color_scale_entry_new:
 * @table:               The #GtkTable the widgets will be attached to.
 * @column:              The column to start with.
 * @row:                 The row to attach the widgets.
 * @text:                The text for the #GtkLabel which will appear
 *                       left of the #GtkHScale.
 * @scale_width:         The minimum horizontal size of the #GtkHScale.
 * @spinbutton_width:    The minimum horizontal size of the #GtkSpinButton.
 * @value:               The initial value.
 * @lower:               The lower boundary.
 * @upper:               The upper boundary.
 * @step_increment:      The step increment.
 * @page_increment:      The page increment.
 * @digits:              The number of decimal digits.
 * @tooltip:             A tooltip message for the scale and the spinbutton.
 * @help_id:             The widgets' help_id (see gimp_help_set_help_data()).
 *
 * This function creates a #GtkLabel, a #GimpColorScale and a
 * #GtkSpinButton and attaches them to a 3-column #GtkTable.
 *
 * Returns: The #GtkSpinButton's #GtkAdjustment.
 **/
GtkObject *
gimp_color_scale_entry_new (GtkTable    *table,
                            gint         column,
                            gint         row,
                            const gchar *text,
                            gint         scale_width,
                            gint         spinbutton_width,
                            gdouble      value,
                            gdouble      lower,
                            gdouble      upper,
                            gdouble      step_increment,
                            gdouble      page_increment,
                            guint        digits,
                            const gchar *tooltip,
                            const gchar *help_id)
{
  return gimp_scale_entry_new_internal (TRUE,
                                        table, column, row,
                                        text, scale_width, spinbutton_width,
                                        value, lower, upper,
                                        step_increment, page_increment,
                                        digits,
                                        TRUE, 0.0, 0.0,
                                        tooltip, help_id);
}

/**
 * gimp_scale_entry_set_sensitive:
 * @adjustment: a #GtkAdjustment returned by gimp_scale_entry_new()
 * @sensitive:  a boolean value with the same semantics as the @sensitive
 *              parameter of gtk_widget_set_sensitive()
 *
 * Sets the sensitivity of the scale_entry's #GtkLabel, #GtkHScale and
 * #GtkSpinbutton.
 **/
void
gimp_scale_entry_set_sensitive (GtkObject *adjustment,
                                gboolean   sensitive)
{
  GtkWidget *widget;

  g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

  widget = GIMP_SCALE_ENTRY_LABEL (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);

  widget = GIMP_SCALE_ENTRY_SCALE (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);

  widget = GIMP_SCALE_ENTRY_SPINBUTTON (adjustment);
  if (widget)
    gtk_widget_set_sensitive (widget, sensitive);
}

static void
gimp_random_seed_update (GtkWidget *widget,
		         gpointer   data)
{
  GtkWidget *w = data;

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (w),
                             (guint) g_random_int ());
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
gimp_random_seed_new (guint *seed, gboolean *random_seed)
{
  GtkWidget *hbox;
  GtkWidget *toggle;
  GtkWidget *spinbutton;
  GtkObject *adj;
  GtkWidget *button;

  hbox = gtk_hbox_new (FALSE, 4);

  /* If we're being asked to generate a random seed, generate one. */
  /* I'm not sure this should be here
  if (*random_seed)
    {
      *seed = g_random_int ();
    }
  */

  spinbutton = gimp_spin_button_new (&adj, *seed,
                                     0, (guint32) -1 , 1, 10, 0, 1, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  g_signal_connect (adj, "value_changed",
                    G_CALLBACK (gimp_uint_adjustment_update),
                    seed);
  gtk_widget_show (spinbutton);

  gimp_help_set_help_data (spinbutton,
                           _("Use this value for random number generator "
                             "seed - this allows you to repeat a "
                             "given \"random\" operation"), NULL);

  button = gtk_button_new_with_mnemonic (_("_New seed"));
  gtk_misc_set_padding (GTK_MISC (GTK_BIN (button)->child), 2, 0);
  /* Send spinbutton as data so that we can change the value in
   * gimp_random_seed_update() */
  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_random_seed_update),
                    spinbutton);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gimp_help_set_help_data (button,
                           _("Seed random number generator with a generated random number"),
                           NULL);

  toggle = gtk_check_button_new_with_label (_("Randomize"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), *random_seed);
  gtk_box_pack_start (GTK_BOX (hbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);
  
  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (gimp_toggle_button_update),
                    random_seed);

  g_object_set_data (G_OBJECT (hbox), "spinbutton", spinbutton);
  g_object_set_data (G_OBJECT (hbox), "button", button);
  g_object_set_data (G_OBJECT (hbox), "toggle", toggle);

  /* Set sensitivity data for the toggle, this stuff makes 
   * gimp_toggle_button_sensitive_update work */
  g_object_set_data (G_OBJECT (toggle), "inverse_sensitive", spinbutton);
  g_object_set_data (G_OBJECT (spinbutton), "inverse_sensitive", button);
  // g_object_set_data (G_OBJECT (button), "inverse_sensitive", adj);

  /* Initialise sensitivity */
  gimp_toggle_button_update (toggle, random_seed);

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
			   GimpCoordinatesData *gcd)
{
  gdouble new_x;
  gdouble new_y;

  new_x = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 0);
  new_y = gimp_size_entry_get_refval (GIMP_SIZE_ENTRY (widget), 1);

  if (gimp_chain_button_get_active (gcd->chainbutton))
    {
      if (gcd->chain_constrains_ratio)
	{
	  if ((gcd->orig_x != 0) && (gcd->orig_y != 0))
	    {
	      if (new_x != gcd->last_x)
		{
		  gcd->last_x = new_x;
		  gcd->last_y = new_y = (new_x * gcd->orig_y) / gcd->orig_x;

                  g_signal_stop_emission_by_name (widget, "value_changed");
		  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1,
					      new_y);
		}
	      else if (new_y != gcd->last_y)
		{
		  gcd->last_y = new_y;
		  gcd->last_x = new_x = (new_y * gcd->orig_x) / gcd->orig_y;

                  g_signal_stop_emission_by_name (widget, "value_changed");
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

              g_signal_stop_emission_by_name (widget, "value_changed");
	      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (widget), 1, new_x);
	    }
	  else if (new_y != gcd->last_y)
	    {
	      gcd->last_x = new_x = gcd->last_y = new_y;

              g_signal_stop_emission_by_name (widget, "value_changed");
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
  GimpCoordinatesData *gcd;
  GtkObject *adjustment;
  GtkWidget *spinbutton;
  GtkWidget *sizeentry;
  GtkWidget *chainbutton;

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);

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

  g_signal_connect_swapped (sizeentry, "destroy",
			    G_CALLBACK (g_free),
			    gcd);

  g_signal_connect (sizeentry, "value_changed",
		    G_CALLBACK (gimp_coordinates_callback),
		    gcd);

  g_object_set_data (G_OBJECT (sizeentry), "chainbutton", chainbutton);

  return sizeentry;
}


/**
 * gimp_pixmap_button_new:
 * @xpm_data: The XPM data which will be passed to gimp_pixmap_new().
 * @text:     An optional text which will appear right of the pixmap.
 *
 * Convenience function that creates a #GtkButton with a #GimpPixmap
 * and an optional #GtkLabel.
 *
 * Returns: The new #GtkButton.
 **/
GtkWidget *
gimp_pixmap_button_new (gchar       **xpm_data,
			const gchar  *text)
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

      label = gtk_label_new_with_mnemonic (text);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
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
 * If you attached a pointer to a #GtkWidget with g_object_set_data() and
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
 **/
void
gimp_toggle_button_sensitive_update (GtkToggleButton *toggle_button)
{
  GtkWidget *set_sensitive;
  gboolean   active;

  active = gtk_toggle_button_get_active (toggle_button);

  set_sensitive =
    g_object_get_data (G_OBJECT (toggle_button), "set_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, active);
      set_sensitive =
        g_object_get_data (G_OBJECT (set_sensitive), "set_sensitive");
    }

  set_sensitive =
    g_object_get_data (G_OBJECT (toggle_button), "inverse_sensitive");
  while (set_sensitive)
    {
      gtk_widget_set_sensitive (set_sensitive, ! active);
      set_sensitive =
        g_object_get_data (G_OBJECT (set_sensitive), "inverse_sensitive");
    }
}

/**
 * gimp_toggle_button_update:
 * @widget: A #GtkToggleButton.
 * @data:   A pointer to a #gint variable which will store the value of
 *          gtk_toggle_button_get_active().
 *
 * Note that this function calls gimp_toggle_button_sensitive_update().
 **/
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
 * @data:   A pointer to a #gint variable which will store the value of
 *          GPOINTER_TO_INT (g_object_get_data (@widget, "gimp-item-data")).
 *
 * Note that this function calls gimp_toggle_button_sensitive_update().
 **/
void
gimp_radio_button_update (GtkWidget *widget,
			  gpointer   data)
{
  gint *toggle_val;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      toggle_val = (gint *) data;

      *toggle_val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
							"gimp-item-data"));
    }

  gimp_toggle_button_sensitive_update (GTK_TOGGLE_BUTTON (widget));
}

/**
 * gimp_menu_item_update:
 * @widget: A #GtkMenuItem.
 * @data:   A pointer to a #gint variable which will store the value of
 *          GPOINTER_TO_INT (g_object_get_data (@widget, "gimp-item-data")).
 **/
void
gimp_menu_item_update (GtkWidget *widget,
		       gpointer   data)
{
  gint *item_val;

  item_val = (gint *) data;

  *item_val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
						  "gimp-item-data"));
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
  gint *val;

  val = (gint *) data;
  *val = RINT (adjustment->value);
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
  guint *val;

  val = (guint *) data;
  *val = (guint) (adjustment->value + 0.5);
}

/**
 * gimp_float_adjustment_update:
 * @adjustment: A #GtkAdjustment.
 * @data:       A pointer to a #gfloat varaiable which will store the
 *              @adjustment's value.
 **/
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
 * @data:       A pointer to a #gdouble variable which will store the
 *              @adjustment's value.
 **/
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
 * @data:   A pointer to a #GimpUnit variable which will store the unit menu's
 *          value.
 *
 * This callback can set the number of decimal digits of an arbitrary number
 * of #GtkSpinButton's. To use this functionality, attach the spinbuttons
 * as list of data pointers attached with g_object_set_data() with the
 * "set_digits" key.
 *
 * See gimp_toggle_button_sensitive_update() for a description of how
 * to set up the list.
 **/
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

  digits += gimp_unit_menu_get_pixel_digits (GIMP_UNIT_MENU (widget));

  spinbutton = g_object_get_data (G_OBJECT (widget), "set_digits");
  while (spinbutton)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (spinbutton), digits);
      spinbutton = g_object_get_data (G_OBJECT (spinbutton), "set_digits");
    }
}


/*
 *  Helper Functions
 */

/**
 * gimp_table_attach_aligned:
 * @table:      The #GtkTable the widgets will be attached to.
 * @column:     The column to start with.
 * @row:        The row to attach the widgets.
 * @label_text: The text for the #GtkLabel which will be attached left of
 *              the widget.
 * @xalign:     The horizontal alignment of the #GtkLabel.
 * @yalign:     The vertival alignment of the #GtkLabel.
 * @widget:     The #GtkWidget to attach right of the label.
 * @colspan:    The number of columns the widget will use.
 * @left_align: %TRUE if the widget should be left-aligned.
 *
 * Note that the @label_text can be %NULL and that the widget will be
 * attached starting at (@column + 1) in this case, too.
 *
 * Returns: The created #GtkLabel.
 **/
GtkWidget *
gimp_table_attach_aligned (GtkTable    *table,
			   gint         column,
			   gint         row,
			   const gchar *label_text,
			   gfloat       xalign,
			   gfloat       yalign,
			   GtkWidget   *widget,
			   gint         colspan,
			   gboolean     left_align)
{
  GtkWidget *label = NULL;

  if (label_text)
    {
      label = gtk_label_new_with_mnemonic (label_text);
      gtk_misc_set_alignment (GTK_MISC (label), xalign, yalign);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
      gtk_table_attach (table, label,
			column, column + 1,
			row, row + 1,
			GTK_FILL, GTK_FILL, 0, 0);
      gtk_widget_show (label);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), widget);
    }

  if (left_align)
    {
      GtkWidget *hbox = gtk_hbox_new (FALSE, 0);

      gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);

      widget = hbox;
    }

  gtk_table_attach (table, widget,
		    column + 1, column + 1 + colspan,
		    row, row + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);

  gtk_widget_show (widget);

  return label;
}
