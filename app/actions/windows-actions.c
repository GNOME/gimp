/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "file/file-utils.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "windows-actions.h"
#include "windows-commands.h"

#include "gimp-intl.h"


static void   windows_actions_display_add    (GimpContainer    *container,
                                              GimpDisplay      *display,
                                              GimpActionGroup  *group);
static void   windows_actions_display_remove (GimpContainer    *container,
                                              GimpDisplay      *display,
                                              GimpActionGroup  *group);
static void   windows_actions_image_notify   (GimpDisplay      *display,
                                              const GParamSpec *unused,
                                              GimpActionGroup  *group);


static const GimpActionEntry windows_actions[] =
{
  { "windows-menu",          NULL, N_("_Windows")         },
  { "windows-images-menu",   NULL, N_("_Images")          },
  { "windows-new-dock-menu", NULL, N_("Create New Doc_k") },

  { "windows-show-toolbox", NULL,
    N_("Tool_box"), "<control>B",
    N_("Raise the toolbox"),
    G_CALLBACK (windows_show_toolbox_cmd_callback),
    GIMP_HELP_TOOLBOX }
};


void
windows_actions_setup (GimpActionGroup *group)
{
  GList *list;

  gimp_action_group_add_actions (group,
                                 windows_actions,
                                 G_N_ELEMENTS (windows_actions));

  g_signal_connect_object (group->gimp->displays, "add",
                           G_CALLBACK (windows_actions_display_add),
                           group, 0);
  g_signal_connect_object (group->gimp->displays, "remove",
                           G_CALLBACK (windows_actions_display_remove),
                           group, 0);

  for (list = GIMP_LIST (group->gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      windows_actions_display_add (group->gimp->displays, display, group);
    }
}

void
windows_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
}


/*  private functions  */

static void
windows_actions_display_add (GimpContainer   *container,
                             GimpDisplay     *display,
                             GimpActionGroup *group)
{
  g_signal_connect_object (display, "notify::image",
                           G_CALLBACK (windows_actions_image_notify),
                           group, 0);

  if (display->image)
    windows_actions_image_notify (display, NULL, group);
}

static void
windows_actions_display_remove (GimpContainer   *container,
                                GimpDisplay     *display,
                                GimpActionGroup *group)
{
  GtkAction *action;
  gchar     *action_name = g_strdup_printf ("windows-display-%04d",
                                            gimp_display_get_ID (display));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), action_name);

  if (action)
    gtk_action_group_remove_action (GTK_ACTION_GROUP (group), action);

  g_free (action_name);
}

static void
windows_actions_image_notify (GimpDisplay      *display,
                              const GParamSpec *unused,
                              GimpActionGroup  *group)
{
  if (display->image)
    {
      GtkAction *action;
      gchar     *action_name = g_strdup_printf ("windows-display-%04d",
                                                gimp_display_get_ID (display));

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            action_name);

      if (! action)
        {
          GimpActionEntry entry;

          entry.name        = action_name;
          entry.stock_id    = GIMP_STOCK_IMAGE;
          entry.label       = "";
          entry.accelerator = NULL;
          entry.tooltip     = NULL;
          entry.callback    = G_CALLBACK (windows_show_display_cmd_callback);
          entry.help_id     = NULL;

          gimp_action_group_add_actions (group, &entry, 1);

          action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                action_name);

          g_object_set_data (G_OBJECT (action), "display", display);
        }

      {
        const gchar *uri;
        gchar       *filename;
        gchar       *basename;
        gchar       *escaped;
        gchar       *title;

        uri = gimp_object_get_name (GIMP_OBJECT (display->image));

        filename = file_utils_uri_display_name (uri);
        basename = file_utils_uri_display_basename (uri);

        escaped = gimp_escape_uline (basename);
        g_free (basename);

        title = g_strdup_printf ("%s-%d.%d", escaped,
                                 gimp_image_get_ID (display->image),
                                 display->instance);
        g_free (escaped);

        g_object_set (action,
                      "label",    title,
                      "tooltip",  filename,
                      "viewable", display->image,
                      "context",  gimp_get_user_context (group->gimp),
                      NULL);

        g_free (filename);
        g_free (title);
      }

      g_free (action_name);
    }
  else
    {
      windows_actions_display_remove (group->gimp->displays, display, group);
    }
}
