/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpoldwidgets.c
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

/* FIXME: #undef GTK_DISABLE_DEPRECATED */
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#undef GIMP_DISABLE_DEPRECATED
#include "gimpoldwidgets.h"
#include "gimppixmap.h"
#include "gimpunitmenu.h"
#include "gimp3migration.h"


/**
 * SECTION: gimpoldwidgets
 * @title: GimpOldWidgets
 * @short_description: Old API that is still available but declared
 *                     as deprecated.
 * @see_also: #GimpIntComboBox
 *
 * These functions are not defined if you #define GIMP_DISABLE_DEPRECATED.
 **/


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
  GList *children;
  GList *list;
  gint   history = 0;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));

  children = gtk_container_get_children (GTK_CONTAINER (option_menu->menu));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (gtk_bin_get_child (GTK_BIN (menu_item))) &&
          g_object_get_data (G_OBJECT (menu_item),
                             "gimp-item-data") == item_data)
        {
          break;
        }

      history++;
    }

  if (list)
    gtk_option_menu_set_history (option_menu, history);

  g_list_free (children);
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
  GList *children;
  GList *list;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (callback != NULL);

  children = gtk_container_get_children (GTK_CONTAINER (option_menu->menu));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (gtk_bin_get_child (GTK_BIN (menu_item))))
        {
          gpointer item_data;
          gboolean sensitive;

          item_data = g_object_get_data (G_OBJECT (menu_item),
                                         "gimp-item-data");
          sensitive = callback (item_data, callback_data);
          gtk_widget_set_sensitive (menu_item, sensitive);
        }
    }

  g_list_free (children);
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
  GList *children;
  GList *list;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (callback != NULL);

  children = gtk_container_get_children (GTK_CONTAINER (option_menu->menu));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *menu_item = GTK_WIDGET (list->data);

      if (GTK_IS_LABEL (gtk_bin_get_child (GTK_BIN (menu_item))))
        {
          gint     item_data;
          gboolean sensitive;

          item_data = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menu_item),
                                                          "gimp-item-data"));
          sensitive = callback (item_data, callback_data);
          gtk_widget_set_sensitive (menu_item, sensitive);
        }
    }

  g_list_free (children);
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
  gint *item_val = (gint *) data;

  *item_val = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-item-data"));
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

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
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
 *
 * Deprecated: use #GimpUnitComboBox instead.
 **/
void
gimp_unit_menu_update (GtkWidget *widget,
                       gpointer   data)
{
  GimpUnit  *val = (GimpUnit *) data;
  GtkWidget *spinbutton;
  gint       digits;

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
