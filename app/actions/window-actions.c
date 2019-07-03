/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "window-actions.h"
#include "window-commands.h"

#include "gimp-intl.h"


/*  private functions  */

static void   window_actions_display_opened (GdkDisplayManager *manager,
                                             GdkDisplay        *display,
                                             GimpActionGroup   *group);
static void   window_actions_display_closed (GdkDisplay        *display,
                                             gboolean           is_error,
                                             GimpActionGroup   *group);


/*  public functions  */

void
window_actions_setup (GimpActionGroup *group,
                      const gchar     *move_to_screen_help_id)
{
  GdkDisplayManager *manager = gdk_display_manager_get ();
  GSList            *displays;
  GSList            *list;

  g_object_set_data_full (G_OBJECT (group), "move-to-screen-help-id",
                          g_strdup (move_to_screen_help_id),
                          (GDestroyNotify) g_free);

  g_object_set_data_full (G_OBJECT (group), "display-table",
                          g_hash_table_new_full (g_str_hash,
                                                 g_str_equal,
                                                 g_free, NULL),
                          (GDestroyNotify) g_hash_table_unref);

  displays = gdk_display_manager_list_displays (manager);

  /*  present displays in the order in which they were opened  */
  displays = g_slist_reverse (displays);

  for (list = displays; list; list = g_slist_next (list))
    {
      window_actions_display_opened (manager, list->data, group);
    }

  g_slist_free (displays);

  g_signal_connect_object (manager, "display-opened",
                           G_CALLBACK (window_actions_display_opened),
                           G_OBJECT (group), 0);
}

void
window_actions_update (GimpActionGroup *group,
                       GtkWidget       *window)
{
  const gchar *group_name;
  gint         show_menu = FALSE;
  gchar       *name;

  group_name = gimp_action_group_get_name (group);

#define SET_ACTIVE(action,active) \
        gimp_action_group_set_action_active (group, action, (active) != 0)
#define SET_VISIBLE(action,active) \
        gimp_action_group_set_action_visible (group, action, (active) != 0)

  if (GTK_IS_WINDOW (window))
    {
      GdkScreen  *screen;
      gchar      *screen_name;

#ifndef GIMP_UNSTABLE
      {
        GdkDisplay *display;

        display = gtk_widget_get_display (window);
        show_menu = (gdk_display_get_n_screens (display) > 1);
      }
#else
      show_menu = TRUE;
#endif /* !GIMP_UNSTABLE */

      if (! show_menu)
        {
          GdkDisplayManager *manager = gdk_display_manager_get ();
          GSList            *displays;

          displays = gdk_display_manager_list_displays (manager);
          show_menu = (displays->next != NULL);
          g_slist_free (displays);
        }

      screen = gtk_widget_get_screen (window);

      screen_name = gdk_screen_make_display_name (screen);
      name = g_strdup_printf ("%s-move-to-screen-%s", group_name, screen_name);
      g_free (screen_name);

      SET_ACTIVE (name, TRUE);
      g_free (name);
    }

  name = g_strdup_printf ("%s-move-to-screen-menu", group_name);
  SET_VISIBLE (name, show_menu);
  g_free (name);

#undef SET_ACTIVE
#undef SET_VISIBLE
}


/*  private functions  */

static void
window_actions_display_opened (GdkDisplayManager *manager,
                               GdkDisplay        *display,
                               GimpActionGroup   *group)
{
  GimpRadioActionEntry *entries;
  GHashTable           *displays;
  const gchar          *display_name;
  const gchar          *help_id;
  const gchar          *group_name;
  GSList               *radio_group;
  gint                  count;
  gint                  n_screens;
  gint                  i;

  displays = g_object_get_data (G_OBJECT (group), "display-table");

  display_name = gdk_display_get_name (display);

  count = GPOINTER_TO_INT (g_hash_table_lookup (displays,
                                                display_name));

  g_hash_table_insert (displays, g_strdup (display_name),
                       GINT_TO_POINTER (count + 1));

  /*  don't add the same display twice  */
  if (count > 0)
    return;

  help_id = g_object_get_data (G_OBJECT (group), "change-to-screen-help-id");

  group_name = gimp_action_group_get_name (group);

  n_screens = gdk_display_get_n_screens (display);

  entries = g_new0 (GimpRadioActionEntry, n_screens);

  for (i = 0; i < n_screens; i++)
    {
      GdkScreen *screen = gdk_display_get_screen (display, i);
      gchar     *screen_name;

      screen_name = gdk_screen_make_display_name (screen);

      entries[i].name        = g_strdup_printf ("%s-move-to-screen-%s",
                                                group_name, screen_name);
      entries[i].icon_name   = GIMP_ICON_WINDOW_MOVE_TO_SCREEN;
      entries[i].label       = g_strdup_printf (_("Screen %s"), screen_name);
      entries[i].accelerator = NULL;
      entries[i].tooltip     = g_strdup_printf (_("Move this window to "
                                                  "screen %s"), screen_name);
      entries[i].value       = g_quark_from_string (screen_name);
      entries[i].help_id     = help_id;

      g_free (screen_name);
    }

  radio_group = g_object_get_data (G_OBJECT (group),
                                   "change-to-screen-radio-group");
  radio_group = gimp_action_group_add_radio_actions (group, NULL,
                                                     entries, n_screens,
                                                     radio_group, 0,
                                                     window_move_to_screen_cmd_callback);
  g_object_set_data (G_OBJECT (group), "change-to-screen-radio-group",
                     radio_group);

  for (i = 0; i < n_screens; i++)
    {
      GdkScreen  *screen = gdk_display_get_screen (display, i);
      GimpAction *action;

      action = gimp_action_group_get_action (group, entries[i].name);

      if (action)
        g_object_set_data (G_OBJECT (action), "screen", screen);

      g_free ((gchar *) entries[i].name);
      g_free ((gchar *) entries[i].tooltip);
      g_free ((gchar *) entries[i].label);
    }

  g_free (entries);

  g_signal_connect_object (display, "closed",
                           G_CALLBACK (window_actions_display_closed),
                           G_OBJECT (group), 0);
}

static void
window_actions_display_closed (GdkDisplay      *display,
                               gboolean         is_error,
                               GimpActionGroup *group)
{
  GHashTable  *displays;
  const gchar *display_name;
  const gchar *group_name;
  gint         count;
  gint         n_screens;
  gint         i;

  displays = g_object_get_data (G_OBJECT (group), "display-table");

  display_name = gdk_display_get_name (display);

  count = GPOINTER_TO_INT (g_hash_table_lookup (displays,
                                                display_name));

  /*  don't remove the same display twice  */
  if (count > 1)
    {
      g_hash_table_insert (displays, g_strdup (display_name),
                           GINT_TO_POINTER (count - 1));
      return;
    }

  g_hash_table_remove (displays, display_name);

  group_name = gimp_action_group_get_name (group);

  n_screens = gdk_display_get_n_screens (display);

  for (i = 0; i < n_screens; i++)
    {
      GdkScreen  *screen = gdk_display_get_screen (display, i);
      GimpAction *action;
      gchar      *screen_name;
      gchar      *action_name;

      screen_name = gdk_screen_make_display_name (screen);
      action_name = g_strdup_printf ("%s-move-to-screen-%s",
                                     group_name, screen_name);
      g_free (screen_name);

      action = gimp_action_group_get_action (group, action_name);

      if (action)
        {
          GSList *radio_group;

          radio_group = gtk_radio_action_get_group (GTK_RADIO_ACTION (action));
          if (radio_group->data == (gpointer) action)
            radio_group = radio_group->next;

          gimp_action_group_remove_action (group, action);

          g_object_set_data (G_OBJECT (group), "change-to-screen-radio-group",
                             radio_group);
        }

      g_free (action_name);
    }
}
