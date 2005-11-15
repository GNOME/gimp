/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "window-actions.h"

#include "gimp-intl.h"


void
window_actions_setup (GimpActionGroup *group,
                      const gchar     *move_to_screen_help_id,
                      GCallback        move_to_screen_callback)
{
  GdkDisplay           *display;
  GimpRadioActionEntry *entries;
  const gchar          *group_name;
  gint                  n_entries;
  gint                  i;

  display   = gdk_display_get_default ();
  n_entries = gdk_display_get_n_screens (display);

  group_name = gtk_action_group_get_name (GTK_ACTION_GROUP (group));

  entries = g_new0 (GimpRadioActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      GdkScreen *screen;
      gchar     *screen_name;

      screen      = gdk_display_get_screen (display, i);
      screen_name = gdk_screen_make_display_name (screen);

      entries[i].name        = g_strdup_printf ("%s-move-to-screen-%02d",
                                                group_name, i);
      entries[i].stock_id    = GIMP_STOCK_MOVE_TO_SCREEN;
      entries[i].label       = g_strdup_printf (_("Screen %d (%s)"),
                                                i, screen_name);
      entries[i].accelerator = NULL;
      entries[i].tooltip     = NULL;
      entries[i].value       = i;
      entries[i].help_id     = move_to_screen_help_id;

      g_free (screen_name);
    }

  gimp_action_group_add_radio_actions (group, entries, n_entries, NULL, 0,
                                       G_CALLBACK (move_to_screen_callback));

  for (i = 0; i < n_entries; i++)
    {
      g_free ((gchar *) entries[i].name);
      g_free ((gchar *) entries[i].label);
    }

  g_free (entries);
}

void
window_actions_update (GimpActionGroup *group,
                       GtkWidget       *window)
{
  const gchar *group_name;
  gint         n_screens = 1;
  gchar       *name;

  group_name = gtk_action_group_get_name (GTK_ACTION_GROUP (group));

#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active) != 0)
#define SET_VISIBLE(action,active) \
        gimp_action_group_set_action_visible (group, action, (active) != 0)

  if (GTK_IS_WINDOW (window))
    {
      GdkDisplay  *display;
      GdkScreen   *screen;
      gint         cur_screen;

      display = gtk_widget_get_display (window);
      screen  = gtk_widget_get_screen (window);

      n_screens  = gdk_display_get_n_screens (display);
      cur_screen = gdk_screen_get_number (screen);

      name = g_strdup_printf ("%s-move-to-screen-%02d",
                              group_name, cur_screen);
      SET_ACTIVE (name, TRUE);
      g_free (name);
    }

  name = g_strdup_printf ("%s-move-to-screen-menu", group_name);
  SET_VISIBLE (name, n_screens > 1);
  g_free (name);

#undef SET_ACTIVE
#undef SET_VISIBLE
}
