/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpThrobberAction
 * Copyright (C) 2005  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gimpthrobberaction.h"
#include "gimpthrobber.h"



static void gimp_throbber_action_class_init    (GimpThrobberActionClass *klass);

static void gimp_throbber_action_connect_proxy (GtkAction  *action,
                                                GtkWidget  *proxy);
static void gimp_throbber_action_sync_property (GtkAction  *action,
                                                GParamSpec *pspec,
                                                GtkWidget  *proxy);


static GtkActionClass *parent_class = NULL;


GType
gimp_throbber_action_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GtkActionClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_throbber_action_class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GtkAction),
        0, /* n_preallocs */
        (GInstanceInitFunc) NULL
      };

      type = g_type_register_static (GTK_TYPE_ACTION,
                                     "GimpThrobberAction",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_throbber_action_class_init (GimpThrobberActionClass *klass)
{
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  action_class->connect_proxy     = gimp_throbber_action_connect_proxy;
  action_class->toolbar_item_type = GIMP_TYPE_THROBBER;
}

static void
gimp_throbber_action_connect_proxy (GtkAction *action,
                                    GtkWidget *proxy)
{

  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  if (GIMP_IS_THROBBER (proxy))
    {
      GParamSpec *pspec;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (action),
                                            "icon-name");

      gimp_throbber_action_sync_property (action, pspec, proxy);
      g_signal_connect_object (action, "notify::icon-name",
                               G_CALLBACK (gimp_throbber_action_sync_property),
                               proxy, 0);

      g_signal_connect_object (proxy, "clicked",
                               G_CALLBACK (gtk_action_activate), action,
                               G_CONNECT_SWAPPED);
    }
}

static void
gimp_throbber_action_sync_property (GtkAction  *action,
                                    GParamSpec *pspec,
                                    GtkWidget  *proxy)
{
  const gchar *property = g_param_spec_get_name (pspec);
  GValue       value    = G_VALUE_INIT;

  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_object_get_property (G_OBJECT (action), property, &value);

  g_object_set_property (G_OBJECT (proxy), property, &value);
  g_value_unset (&value);
}

GtkAction *
gimp_throbber_action_new (const gchar *name,
                          const gchar *label,
                          const gchar *tooltip,
                          const gchar *icon_name)
{
  return g_object_new (GIMP_TYPE_THROBBER_ACTION,
                       "name",      name,
                       "label",     label,
                       "tooltip",   tooltip,
                       "icon-name", icon_name,
                       NULL);
}
