/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpenummenu.c
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimpenummenu.h"

#include "gimp-intl.h"


static void  gimp_enum_menu_class_init  (GimpEnumMenuClass *klass);
static void  gimp_enum_menu_init        (GimpEnumMenu      *enum_menu);
static void  gimp_enum_menu_finalize    (GObject           *object);


static GtkMenuClass *parent_class = NULL;


GType
gimp_enum_menu_get_type (void)
{
  static GType enum_menu_type = 0;

  if (!enum_menu_type)
    {
      static const GTypeInfo enum_menu_info =
      {
        sizeof (GimpEnumMenuClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_enum_menu_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpEnumMenu),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_enum_menu_init,
      };

      enum_menu_type = g_type_register_static (GTK_TYPE_MENU,
                                               "GimpEnumMenu",
                                               &enum_menu_info, 0);
    }

  return enum_menu_type;
}

static void
gimp_enum_menu_class_init (GimpEnumMenuClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_enum_menu_finalize;
}

static void
gimp_enum_menu_init (GimpEnumMenu *enum_menu)
{
  enum_menu->enum_class = NULL;
}

static void
gimp_enum_menu_finalize (GObject *object)
{
  GimpEnumMenu *enum_menu = GIMP_ENUM_MENU (object);

  if (enum_menu->enum_class)
    g_type_class_unref (enum_menu->enum_class);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gimp_enum_menu_new:
 * @enum_type: the #GType of an enum.
 * @callback: a callback to connect to the "activate" signal of each
 *            #GtkMenuItem that is created.
 * @callback_data: data to pass to the @callback.
 *
 * Creates a new #GimpEnumMenu, derived from #GtkMenu with menu items
 * for each of the enum values. The enum needs to be registered to the
 * type system and should have translatable value names.
 *
 * To each menu item it's enum value is attached as "gimp-item-data".
 * Therefore you can use gimp_menu_item_update() from libgimpwidgets
 * as @callback function.
 *
 * Return value: a new #GimpEnumMenu.
 **/
GtkWidget *
gimp_enum_menu_new (GType      enum_type,
                    GCallback  callback,
                    gpointer   callback_data)
{
  GEnumClass *enum_class;
  GtkWidget  *menu;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  menu = gimp_enum_menu_new_with_range (enum_type,
                                        enum_class->minimum,
                                        enum_class->maximum,
                                        callback, callback_data);

  g_type_class_unref (enum_class);

  return menu;
}

GtkWidget *
gimp_enum_menu_new_with_range (GType      enum_type,
                               gint       minimum,
                               gint       maximum,
                               GCallback  callback,
                               gpointer   callback_data)
{
  GimpEnumMenu *menu;
  GtkWidget    *menu_item;
  GEnumValue   *value;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  menu = g_object_new (GIMP_TYPE_ENUM_MENU, NULL);

  menu->enum_class = g_type_class_ref (enum_type);

  for (value = menu->enum_class->values; value->value_name; value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      menu_item = gtk_image_menu_item_new_with_mnemonic (gettext (value->value_name));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);

      g_object_set_data (G_OBJECT (menu_item), "gimp-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (menu_item, "activate",
                          callback,
                          callback_data);
    }

  return GTK_WIDGET (menu);
}

GtkWidget *
gimp_enum_menu_new_with_values (GType      enum_type,
                                GCallback  callback,
                                gpointer   callback_data,
                                gint       n_values,
                                ...)
{
  GtkWidget *menu;
  va_list    args;

  va_start (args, n_values);

  menu = gimp_enum_menu_new_with_values_valist (enum_type,
                                                callback,
                                                callback_data,
                                                n_values,
                                                args);

  va_end (args);

  return menu;
}

GtkWidget *
gimp_enum_menu_new_with_values_valist (GType      enum_type,
                                       GCallback  callback,
                                       gpointer   callback_data,
                                       gint       n_values,
                                       va_list    args)
{
  GimpEnumMenu *menu;
  GtkWidget    *menu_item;
  GEnumValue   *value;
  gint          i;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (n_values > 1, NULL);

  menu = g_object_new (GIMP_TYPE_ENUM_MENU, NULL);

  menu->enum_class = g_type_class_ref (enum_type);

  for (i = 0; i < n_values; i++)
    {
      value = g_enum_get_value (menu->enum_class, va_arg (args, gint));

      if (value)
        {
          menu_item =
            gtk_image_menu_item_new_with_mnemonic (gettext (value->value_name));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
          gtk_widget_show (menu_item);

          g_object_set_data (G_OBJECT (menu_item), "gimp-item-data",
                             GINT_TO_POINTER (value->value));

          if (callback)
            g_signal_connect (menu_item, "activate",
                              callback,
                              callback_data);
        }
    }

  return GTK_WIDGET (menu);
}

/**
 * gimp_enum_menu_set_stock_prefix:
 * @enum_menu: a #GimpEnumMenu
 * @stock_prefix: the prefix of the group of stock ids to use.
 *
 * Adds stock icons to the items in @enum_menu.  The stock_id for each
 * icon is created by appending the enum_value's nick to the given
 * @stock_prefix.  If no such stock_id exists, no icon is displayed.
 **/
void
gimp_enum_menu_set_stock_prefix (GimpEnumMenu *enum_menu,
                                 const gchar  *stock_prefix)
{
  GtkWidget  *image;
  GEnumValue *enum_value;
  GList      *list;
  gchar      *stock_id;
  gint        value;

  g_return_if_fail (GIMP_IS_ENUM_MENU (enum_menu));
  g_return_if_fail (stock_prefix != NULL);

  for (list = GTK_MENU_SHELL (enum_menu)->children; list; list = list->next)
    {
      GtkImageMenuItem *item = list->data;

      value = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                  "gimp-item-data"));

      enum_value = g_enum_get_value (enum_menu->enum_class, value);
      if (!enum_value)
        continue;

      stock_id = g_strconcat (stock_prefix, "-", enum_value->value_nick, NULL);
      image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
      g_free (stock_id);

      gtk_image_menu_item_set_image (item, image);
      gtk_widget_show (image);
    }
}


/**
 * gimp_enum_option_menu_new:
 * @enum_type: the #GType of an enum.
 * @callback: a callback to connect to the "activate" signal of each
 *            #GtkMenuItem that is created.
 * @callback_data: data to pass to the @callback.
 *
 * This function calls gimp_enum_menu_new() for you and attaches
 * the resulting @GtkMenu to a newly created #GtkOptionMenu.
 *
 * Return value: a new #GtkOptionMenu.
 **/
GtkWidget *
gimp_enum_option_menu_new (GType      enum_type,
                           GCallback  callback,
                           gpointer   callback_data)
{
  GtkWidget *option_menu;
  GtkWidget *menu;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  menu = gimp_enum_menu_new (enum_type, callback, callback_data);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  return option_menu;
}

GtkWidget *
gimp_enum_option_menu_new_with_range (GType      enum_type,
                                      gint       minimum,
                                      gint       maximum,
                                      GCallback  callback,
                                      gpointer   callback_data)
{
  GtkWidget *option_menu;
  GtkWidget *menu;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  menu = gimp_enum_menu_new_with_range (enum_type,
                                        minimum, maximum,
                                        callback, callback_data);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

  return option_menu;
}

GtkWidget *
gimp_enum_option_menu_new_with_values (GType      enum_type,
                                       GCallback  callback,
                                       gpointer   callback_data,
                                       gint       n_values,
                                       ...)
{
  GtkWidget *option_menu;
  va_list    args;

  va_start (args, n_values);

  option_menu = gimp_enum_option_menu_new_with_values_valist (enum_type,
                                                              callback,
                                                              callback_data,
                                                              n_values, args);

  va_end (args);

  return option_menu;
}

GtkWidget *
gimp_enum_option_menu_new_with_values_valist (GType      enum_type,
                                              GCallback  callback,
                                              gpointer   callback_data,
                                              gint       n_values,
                                              va_list    args)
{
  GtkWidget *option_menu = NULL;
  GtkWidget *menu;

  menu = gimp_enum_menu_new_with_values_valist (enum_type,
                                                callback, callback_data,
                                                n_values, args);

  if (menu)
    {
      option_menu = gtk_option_menu_new ();
      gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);
    }

  return option_menu;
}

/**
 * gimp_enum_option_menu_set_stock_prefix:
 * @option_menu: a #GtkOptionMenu created using gtk_enum_option_menu_new().
 * @stock_prefix: the prefix of the group of stock ids to use.
 *
 * A convenience function that calls gimp_enum_menu_set_stock_prefix()
 * with the enum menu used by @option_menu.
 **/
void
gimp_enum_option_menu_set_stock_prefix (GtkOptionMenu *option_menu,
                                        const gchar   *stock_prefix)
{
  GtkWidget *enum_menu;

  g_return_if_fail (GTK_IS_OPTION_MENU (option_menu));
  g_return_if_fail (stock_prefix != NULL);

  enum_menu = gtk_option_menu_get_menu (option_menu);

  if (enum_menu && GIMP_IS_ENUM_MENU (enum_menu))
    gimp_enum_menu_set_stock_prefix (GIMP_ENUM_MENU (enum_menu), stock_prefix);
}

/**
 * gimp_enum_radio_box_new:
 * @enum_type: the #GType of an enum.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Creates a new group of #GtkRadioButtons representing the enum values.
 * This is very similar to gimp_enum_menu_new().
 *
 * Return value: a new #GtkVBox holding a group of #GtkRadioButtons.
 **/
GtkWidget *
gimp_enum_radio_box_new (GType       enum_type,
                         GCallback   callback,
                         gpointer    callback_data,
                         GtkWidget **first_button)
{
  GEnumClass *enum_class;
  GtkWidget  *vbox;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  vbox = gimp_enum_radio_box_new_with_range (enum_type,
                                             enum_class->minimum,
                                             enum_class->maximum,
                                             callback, callback_data,
                                             first_button);

  g_type_class_unref (enum_class);

  return vbox;
}

GtkWidget *
gimp_enum_radio_box_new_with_range (GType       enum_type,
                                    gint        minimum,
                                    gint        maximum,
                                    GCallback   callback,
                                    gpointer    callback_data,
                                    GtkWidget **first_button)
{
  GtkWidget  *vbox;
  GtkWidget  *button;
  GEnumClass *enum_class;
  GEnumValue *value;
  GSList     *group = NULL;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  vbox = gtk_vbox_new (FALSE, 1);
  g_object_weak_ref (G_OBJECT (vbox),
                     (GWeakNotify) g_type_class_unref, enum_class);

  if (first_button)
    *first_button = NULL;

  for (value = enum_class->values; value->value_name; value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      button = gtk_radio_button_new_with_mnemonic (group,
                                                   gettext (value->value_name));

      if (first_button && *first_button == NULL)
        *first_button = button;

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (button), "gimp-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (button, "toggled",
                          callback,
                          callback_data);
    }

  return vbox;
}


/**
 * gimp_enum_radio_frame_new:
 * @enum_type: the #GType of an enum.
 * @label_widget: a widget to put into the frame that will hold the radio box.
 * @border_width: the border_width of the vbox inside the frame.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Calls gimp_enum_radio_box_new() and puts the resulting vbox into a
 * #GtkFrame.
 *
 * Return value: a new #GtkFrame holding a group of #GtkRadioButtons.
 **/
GtkWidget *
gimp_enum_radio_frame_new (GType        enum_type,
                           GtkWidget   *label_widget,
                           gint         border_width,
                           GCallback    callback,
                           gpointer     callback_data,
                           GtkWidget  **first_button)
{
  GtkWidget  *frame;
  GtkWidget  *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = gtk_frame_new (NULL);

  if (label_widget)
    {
      gtk_frame_set_label_widget (GTK_FRAME (frame), label_widget);
      gtk_widget_show (label_widget);
    }

  radio_box = gimp_enum_radio_box_new (enum_type,
                                       callback, callback_data,
                                       first_button);

  gtk_container_set_border_width (GTK_CONTAINER (radio_box), border_width);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  return frame;
}

GtkWidget *
gimp_enum_radio_frame_new_with_range (GType        enum_type,
                                      gint         minimum,
                                      gint         maximum,
                                      GtkWidget   *label_widget,
                                      gint         border_width,
                                      GCallback    callback,
                                      gpointer     callback_data,
                                      GtkWidget  **first_button)
{
  GtkWidget  *frame;
  GtkWidget  *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = gtk_frame_new (NULL);

  if (label_widget)
    {
      gtk_frame_set_label_widget (GTK_FRAME (frame), label_widget);
      gtk_widget_show (label_widget);
    }

  radio_box = gimp_enum_radio_box_new_with_range (enum_type,
                                                  minimum,
                                                  maximum,
                                                  callback, callback_data,
                                                  first_button);

  gtk_container_set_border_width (GTK_CONTAINER (radio_box), border_width);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  return frame;
}


/**
 * gimp_enum_stock_box_new:
 * @enum_type: the #GType of an enum.
 * @stock_prefix: the prefix of the group of stock ids to use.
 * @icon_size:
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Creates a horizontal box of radio buttons with stock icons.  The
 * stock_id for each icon is created by appending the enum_value's
 * nick to the given @stock_prefix.
 *
 * Return value: a new #GtkHbox holding a group of #GtkRadioButtons.
 **/
GtkWidget *
gimp_enum_stock_box_new (GType         enum_type,
                         const gchar  *stock_prefix,
                         GtkIconSize   icon_size,
                         GCallback     callback,
                         gpointer      callback_data,
                         GtkWidget   **first_button)
{
  GEnumClass *enum_class;
  GtkWidget  *box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  box = gimp_enum_stock_box_new_with_range (enum_type,
                                            enum_class->minimum,
                                            enum_class->maximum,
                                            stock_prefix, icon_size,
                                            callback, callback_data,
                                            first_button);

  g_type_class_unref (enum_class);

  return box;
}

GtkWidget *
gimp_enum_stock_box_new_with_range (GType         enum_type,
                                    gint          minimum,
                                    gint          maximum,
                                    const gchar  *stock_prefix,
                                    GtkIconSize   icon_size,
                                    GCallback     callback,
                                    gpointer      callback_data,
                                    GtkWidget   **first_button)
{
  GtkWidget  *hbox;
  GtkWidget  *button;
  GtkWidget  *image;
  GEnumClass *enum_class;
  GEnumValue *value;
  gchar      *stock_id;
  GSList     *group = NULL;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (stock_prefix != NULL, NULL);

  enum_class = g_type_class_ref (enum_type);

  hbox = gtk_hbox_new (FALSE, 2);
  g_object_weak_ref (G_OBJECT (hbox),
                     (GWeakNotify) g_type_class_unref, enum_class);

  if (first_button)
    *first_button = NULL;

  for (value = enum_class->values; value->value_name; value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      button = gtk_radio_button_new (group);

      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      if (first_button && *first_button == NULL)
        *first_button = button;

      stock_id = g_strconcat (stock_prefix, "-", value->value_nick, NULL);

      image = gtk_image_new_from_stock (stock_id, icon_size);

      g_free (stock_id);

      if (image)
        {
          gtk_container_add (GTK_CONTAINER (button), image);
          gtk_widget_show (image);
        }

      if (value->value_name)
        gimp_help_set_help_data (button, gettext (value->value_name), NULL);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (button), "gimp-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (button, "toggled",
                          callback,
                          callback_data);
    }

  return hbox;
}

void
gimp_enum_stock_box_set_child_padding (GtkWidget *stock_box,
                                       gint       xpad,
                                       gint       ypad)
{
  GList *list;

  g_return_if_fail (GTK_IS_CONTAINER (stock_box));

  for (list = gtk_container_get_children (GTK_CONTAINER (stock_box));
       list;
       list = g_list_next (list))
    {
      GtkBin  *bin  = list->data;
      GtkMisc *misc = bin->child;

      gtk_misc_set_padding (misc,
                            xpad < 0 ? misc->xpad : xpad,
                            ypad < 0 ? misc->ypad : ypad);
    }
}
