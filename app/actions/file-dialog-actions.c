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

#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-ins.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "file-dialog-commands.h"
#include "file-dialog-actions.h"

#include "gimp-intl.h"


void
file_dialog_actions_setup (GimpActionGroup *group,
                           GSList          *file_procs,
                           const gchar     *xcf_proc_name)
{
  GSList *list;

  for (list = file_procs; list; list = g_slist_next (list))
    {
      PlugInProcDef         *file_proc = list->data;
      const gchar           *stock_id  = NULL;
      gchar                 *help_id;
      GimpPlugInActionEntry  entry;
      gboolean               is_xcf;

      if (! file_proc->menu_path)
        continue;

      is_xcf = (strcmp (file_proc->db_info.name, xcf_proc_name) == 0);

      if (is_xcf)
        {
          stock_id = GIMP_STOCK_WILBER;
          help_id  = g_strdup (GIMP_HELP_FILE_SAVE_XCF);
        }
      else
        {
          const gchar *progname;
          const gchar *locale_domain;
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
      entry.proc_def = file_proc;
      entry.help_id  = help_id;

      gimp_action_group_add_plug_in_actions (group, &entry, 1,
                                             G_CALLBACK (file_dialog_type_cmd_callback));

      g_free (help_id);
    }
}
