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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"

#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-ins.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "file-commands.h"
#include "file-open-actions.h"

#include "gimp-intl.h"


static GimpActionEntry file_open_actions[] =
{
  { "file-open-popup", NULL, N_("File Open Menu"), NULL, NULL, NULL,
    GIMP_HELP_FILE_OPEN },

  { "file-open-automatic", NULL,
    N_("Automatic"), NULL, NULL,
    G_CALLBACK (file_type_cmd_callback),
    GIMP_HELP_FILE_OPEN_BY_EXTENSION }
};


void
file_open_actions_setup (GimpActionGroup *group)
{
  GSList *list;

  gimp_action_group_add_actions (group,
                                 file_open_actions,
                                 G_N_ELEMENTS (file_open_actions));

  for (list = group->gimp->load_procs; list; list = g_slist_next (list))
    {
      PlugInProcDef   *file_proc     = list->data;
      const gchar     *locale_domain = NULL;
      const gchar     *stock_id      = NULL;
      gchar           *help_id;
      GimpActionEntry  entry;
      gboolean         is_xcf;
      GtkAction       *action;

      if (! file_proc->menu_path)
        continue;

      is_xcf = (strcmp (file_proc->db_info.name, "gimp_xcf_load") == 0);

      if (is_xcf)
        {
          stock_id = GIMP_STOCK_WILBER;
          help_id  = g_strdup (GIMP_HELP_FILE_OPEN_XCF);
        }
      else
        {
          const gchar *progname;
          const gchar *help_domain;

          progname = plug_in_proc_def_get_progname (file_proc);

          locale_domain = plug_ins_locale_domain (group->gimp, progname, NULL);
          help_domain   = plug_ins_help_domain (group->gimp, progname, NULL);

          help_id = plug_in_proc_def_get_help_id (file_proc, help_domain);
        }

      entry.name     = file_proc->db_info.name;
      entry.stock_id = stock_id;
      entry.label    = strstr (file_proc->menu_path, "/") + 1;
      entry.tooltip  = NULL;
      entry.callback = G_CALLBACK (file_type_cmd_callback);
      entry.help_id  = help_id;

      gimp_action_group_add_actions (group, &entry, 1);

      g_free (help_id);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                            file_proc->db_info.name);

      g_object_set_data (G_OBJECT (action), "file-proc", file_proc);
    }
}

void
file_open_actions_update (GimpActionGroup *group,
                          gpointer         data)
{
}
