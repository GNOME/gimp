/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoggleaction.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2008 Sven Neumann <sven@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "gimptoggleaction.h"


static void   gimp_toggle_action_connect_proxy     (GtkAction        *action,
                                                    GtkWidget        *proxy);
static void   gimp_toggle_action_set_proxy_tooltip (GimpToggleAction *action,
                                                    GtkWidget        *proxy);
static void   gimp_toggle_action_tooltip_notify    (GimpToggleAction *action,
                                                    const GParamSpec *pspec,
                                                    gpointer          data);


G_DEFINE_TYPE (GimpToggleAction, gimp_toggle_action, GTK_TYPE_TOGGLE_ACTION)

#define parent_class gimp_toggle_action_parent_class


static void
gimp_toggle_action_class_init (GimpToggleActionClass *klass)
{
  GtkActionClass *action_class = GTK_ACTION_CLASS (klass);

  action_class->connect_proxy = gimp_toggle_action_connect_proxy;
}

static void
gimp_toggle_action_init (GimpToggleAction *action)
{
  g_signal_connect (action, "notify::tooltip",
                    G_CALLBACK (gimp_toggle_action_tooltip_notify),
                    NULL);
}

static void
gimp_toggle_action_connect_proxy (GtkAction *action,
                                  GtkWidget *proxy)
{
  GTK_ACTION_CLASS (parent_class)->connect_proxy (action, proxy);

  gimp_toggle_action_set_proxy_tooltip (GIMP_TOGGLE_ACTION (action), proxy);
}


/*  public functions  */

GtkToggleAction *
gimp_toggle_action_new (const gchar *name,
                        const gchar *label,
                        const gchar *tooltip,
                        const gchar *icon_name)
{
  GtkToggleAction *action;

  action = g_object_new (GIMP_TYPE_TOGGLE_ACTION,
                         "name",      name,
                         "label",     label,
                         "tooltip",   tooltip,
                         "icon-name", icon_name,
                         NULL);

  return action;
}


/*  private functions  */


static void
gimp_toggle_action_set_proxy_tooltip (GimpToggleAction *action,
                                      GtkWidget        *proxy)
{
  const gchar *tooltip = gtk_action_get_tooltip (GTK_ACTION (action));

  if (tooltip)
    gimp_help_set_help_data (proxy, tooltip,
                             g_object_get_qdata (G_OBJECT (proxy),
                                                 GIMP_HELP_ID));
}

static void
gimp_toggle_action_tooltip_notify (GimpToggleAction *action,
                                   const GParamSpec *pspec,
                                   gpointer          data)
{
  GSList *list;

  for (list = gtk_action_get_proxies (GTK_ACTION (action));
       list;
       list = g_slist_next (list))
    {
      gimp_toggle_action_set_proxy_tooltip (action, list->data);
    }
}
