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
#include "core/gimpdrawable.h"

#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-ins.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"

#include "file-commands.h"
#include "file-save-menu.h"
#include "menus.h"

#include "gimp-intl.h"


GimpItemFactoryEntry file_save_menu_entries[] =
{
  { { N_("/By Extension"), NULL,
      file_save_by_extension_cmd_callback, 0 },
    NULL,
    GIMP_HELP_FILE_SAVE_BY_EXTENSION, NULL },

  MENU_SEPARATOR ("/---")
};

gint n_file_save_menu_entries = G_N_ELEMENTS (file_save_menu_entries);


void
file_save_menu_setup (GimpItemFactory *factory)
{
  GSList *list;

  for (list = factory->gimp->save_procs; list; list = g_slist_next (list))
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

      locale_domain = plug_ins_locale_domain (factory->gimp, progname, NULL);
      help_path     = plug_ins_help_path (factory->gimp, progname);

      help_id = plug_in_proc_def_get_help_id (file_proc);

      if (help_path)
        help_page = g_strconcat (help_path, ":", help_id, NULL);
      else
        help_page = g_strconcat ("filters/", help_id, NULL);

      g_free (help_id);

      entry.entry.path            = strstr (file_proc->menu_path, "/");
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_save_type_cmd_callback;
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

void
file_save_menu_update (GtkItemFactory *item_factory,
                       gpointer        data)
{
  GimpDrawable    *drawable;
  PlugInImageType  plug_in_image_type = 0;
  GSList          *procs;

  drawable = GIMP_DRAWABLE (data);

  switch (gimp_drawable_type (drawable))
    {
    case GIMP_RGB_IMAGE:
      plug_in_image_type = PLUG_IN_RGB_IMAGE;
      break;
    case GIMP_RGBA_IMAGE:
      plug_in_image_type = PLUG_IN_RGBA_IMAGE;
      break;
    case GIMP_GRAY_IMAGE:
      plug_in_image_type = PLUG_IN_GRAY_IMAGE;
      break;
    case GIMP_GRAYA_IMAGE:
      plug_in_image_type = PLUG_IN_GRAYA_IMAGE;
      break;
    case GIMP_INDEXED_IMAGE:
      plug_in_image_type = PLUG_IN_INDEXED_IMAGE;
      break;
    case GIMP_INDEXEDA_IMAGE:
      plug_in_image_type = PLUG_IN_INDEXEDA_IMAGE;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  for (procs = GIMP_ITEM_FACTORY (item_factory)->gimp->save_procs;
       procs;
       procs = g_slist_next (procs))
    {
      PlugInProcDef *file_proc;

      file_proc = (PlugInProcDef *) procs->data;

      if (file_proc->db_info.proc_type != GIMP_EXTENSION)
        {
          gimp_item_factory_set_sensitive (item_factory,
                                           file_proc->menu_path,
                                           (file_proc->image_types_val &
                                            plug_in_image_type));
        }
    }
}
