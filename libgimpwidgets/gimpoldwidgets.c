/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoldwidgets.c
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

#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#undef GIMP_DISABLE_DEPRECATED
#include "gimpoldwidgets.h"


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
