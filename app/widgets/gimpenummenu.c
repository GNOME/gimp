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

#include "widgets-types.h"

#include "gimpenummenu.h"

#include "libgimp/gimpintl.h"


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

GtkWidget *
gimp_enum_menu_new (GType      enum_type,
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
      menu_item = gtk_menu_item_new_with_label (gettext (value->value_name));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);

      g_object_set_data (G_OBJECT (menu_item), "gimp-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (G_OBJECT (menu_item), "activate",
                          callback,
                          callback_data);
    }

  return GTK_WIDGET (menu);
}

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
