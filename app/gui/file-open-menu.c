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

#include "gui-types.h"

#include "core/gimp.h"

#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-ins.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "file-commands.h"
#include "file-open-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry file_open_menu_entries[] =
{
  { { N_("/Automatic"), NULL,
      file_open_by_extension_cmd_callback, 0 },
    NULL,
    GIMP_HELP_FILE_OPEN_BY_EXTENSION, NULL },

  MENU_SEPARATOR ("/---")
};

gint n_file_open_menu_entries = G_N_ELEMENTS (file_open_menu_entries);


void
file_open_menu_setup (GimpItemFactory *factory)
{
  GSList *list;

  for (list = factory->gimp->load_procs; list; list = g_slist_next (list))
    {
      PlugInProcDef        *file_proc;
      const gchar          *progname;
      const gchar          *locale_domain;
      const gchar          *help_path;
      GimpItemFactoryEntry  entry;
      gchar                *help_id;
      gchar                *help_page;

      file_proc = (PlugInProcDef *) list->data;

      progname = plug_in_proc_def_get_progname (file_proc);

      locale_domain = plug_ins_locale_domain (factory->gimp,
                                              progname, NULL);
      help_path = plug_ins_help_path (factory->gimp,
                                      progname);

#ifdef __GNUC__
#warning FIXME: fix plug-in menu item help
#endif
      {
        gchar *basename;
        gchar *lowercase_basename;

        basename = g_path_get_basename (file_proc->prog);
        lowercase_basename = g_ascii_strdown (basename, -1);

        help_id = g_strconcat (lowercase_basename, ".html", NULL);

        g_free (lowercase_basename);
      }

      if (help_path)
        help_page = g_strconcat (help_path, ":", help_id, NULL);
      else
        help_page = g_strconcat ("filters/", help_id, NULL);

      g_free (help_id);

      entry.entry.path            = strstr (file_proc->menu_path, "/");
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_open_type_cmd_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.quark_string          = NULL;
      entry.help_id               = help_page;
      entry.description           = NULL;

      gimp_item_factory_create_item (factory,
                                     &entry,
                                     locale_domain,
                                     file_proc, 2,
                                     TRUE, FALSE);

      g_free (help_page);
    }
}
