/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpenumwidgets.c
 * Copyright (C) 2002-2004  Sven Neumann <sven@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgetstypes.h"

#include "gimpenumwidgets.h"
#include "gimpframe.h"
#include "gimphelpui.h"


/**
 * gimp_enum_radio_box_new:
 * @enum_type: the #GType of an enum.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Creates a new group of #GtkRadioButtons representing the enum
 * values.  A group of radiobuttons is a good way to represent enums
 * with up to three or four values. Often it is better to use a
 * #GimpEnumComboBox instead.
 *
 * Return value: a new #GtkVBox holding a group of #GtkRadioButtons.
 *
 * Since: GIMP 2.4
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

/**
 * gimp_enum_radio_box_new_with_range:
 * @minimum:
 * @maximum:
 * @enum_type: the #GType of an enum.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Just like gimp_enum_radio_box_new(), this function creates a group
 * of radio buttons, but it allows to limit the range of available
 * enum values.
 *
 * Return value: a new #GtkVBox holding a group of #GtkRadioButtons.
 *
 * Since: GIMP 2.4
 **/
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
      const gchar *desc;

      if (value->value < minimum || value->value > maximum)
        continue;

      desc = gimp_enum_value_get_desc (enum_class, value);

      button = gtk_radio_button_new_with_mnemonic (group, desc);

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
 * @label_widget: a widget to use as label for the frame that will
 *                hold the radio box.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Calls gimp_enum_radio_box_new() and puts the resulting vbox into a
 * #GtkFrame.
 *
 * Return value: a new #GtkFrame holding a group of #GtkRadioButtons.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_enum_radio_frame_new (GType       enum_type,
                           GtkWidget  *label_widget,
                           GCallback   callback,
                           gpointer    callback_data,
                           GtkWidget **first_button)
{
  GtkWidget *frame;
  GtkWidget *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = gimp_frame_new (NULL);

  if (label_widget)
    {
      gtk_frame_set_label_widget (GTK_FRAME (frame), label_widget);
      gtk_widget_show (label_widget);
    }

  radio_box = gimp_enum_radio_box_new (enum_type,
                                       callback, callback_data,
                                       first_button);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  return frame;
}

/**
 * gimp_enum_radio_frame_new_with_range:
 * @enum_type: the #GType of an enum.
 * @minimum:
 * @maximum:
 * @label_widget: a widget to put into the frame that will hold the radio box.
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Calls gimp_enum_radio_box_new_with_range() and puts the resulting
 * vbox into a #GtkFrame.
 *
 * Return value: a new #GtkFrame holding a group of #GtkRadioButtons.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_enum_radio_frame_new_with_range (GType       enum_type,
                                      gint        minimum,
                                      gint        maximum,
                                      GtkWidget  *label_widget,
                                      GCallback   callback,
                                      gpointer    callback_data,
                                      GtkWidget **first_button)
{
  GtkWidget *frame;
  GtkWidget *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = gimp_frame_new (NULL);

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
 *
 * Since: GIMP 2.4
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

/**
 * gimp_enum_stock_box_new_with_range:
 * @enum_type: the #GType of an enum.
 * @minimum:
 * @maximum:
 * @stock_prefix: the prefix of the group of stock ids to use.
 * @icon_size:
 * @callback: a callback to connect to the "toggled" signal of each
 *            #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @first_button: returns the first button in the created group.
 *
 * Just like gimp_enum_stock_box_new(), this function creates a group
 * of radio buttons, but it allows to limit the range of available
 * enum values.
 *
 * Return value: a new #GtkHbox holding a group of #GtkRadioButtons.
 *
 * Since: GIMP 2.4
 **/
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

      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
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

      gimp_help_set_help_data (button,
                               gimp_enum_value_get_desc (enum_class, value),
                               NULL);

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

/**
 * gimp_enum_stock_box_set_child_padding:
 * @stock_box: a stock box widget
 * @xpad: horizontal padding
 * @ypad: vertical padding
 *
 * Sets the padding of all buttons in a box created by
 * gimp_enum_stock_box_new().
 *
 * Since: GIMP 2.4
 **/
void
gimp_enum_stock_box_set_child_padding (GtkWidget *stock_box,
                                       gint       xpad,
                                       gint       ypad)
{
  GList *children;
  GList *list;

  g_return_if_fail (GTK_IS_CONTAINER (stock_box));

  children = gtk_container_get_children (GTK_CONTAINER (stock_box));

  for (list = children; list; list = g_list_next (list))
    {
      GtkBin *bin = list->data;

      if (GTK_IS_MISC (bin->child))
        {
          GtkMisc *misc = GTK_MISC (bin->child);

          gtk_misc_set_padding (misc,
                                xpad < 0 ? misc->xpad : xpad,
                                ypad < 0 ? misc->ypad : ypad);
        }
    }

  g_list_free (children);
}
