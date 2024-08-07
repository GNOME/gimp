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

#include <string.h>

#include <gtk/gtk.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "menus-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginprocedure.h"

#include "widgets/gimpuimanager.h"

#include "plug-in-menus.h"

#include "gimp-log.h"
#include "gimp-intl.h"


typedef struct _PlugInMenuEntry PlugInMenuEntry;

struct _PlugInMenuEntry
{
  GimpPlugInProcedure *proc;
  const gchar         *menu_path;
};


/*  local function prototypes  */

static void    plug_in_menus_register_procedure   (GimpPDB             *pdb,
                                                   GimpProcedure       *procedure,
                                                   GimpUIManager       *manager);
static void    plug_in_menus_unregister_procedure (GimpPDB             *pdb,
                                                   GimpProcedure       *procedure,
                                                   GimpUIManager       *manager);
static void    plug_in_menus_menu_path_added      (GimpPlugInProcedure *plug_in_proc,
                                                   const gchar         *menu_path,
                                                   GimpUIManager       *manager);
static void    plug_in_menus_add_proc             (GimpUIManager       *manager,
                                                   const gchar         *ui_path,
                                                   GimpPlugInProcedure *proc,
                                                   const gchar         *menu_path);
static void    plug_in_menus_tree_insert          (GTree               *entries,
                                                   const gchar     *    path,
                                                   PlugInMenuEntry     *entry);
static gboolean   plug_in_menus_tree_traverse     (gpointer             key,
                                                   PlugInMenuEntry     *entry,
                                                   GimpUIManager       *manager);
static void    plug_in_menu_entry_free            (PlugInMenuEntry     *entry);


/*  public functions  */

void
plug_in_menus_setup (GimpUIManager *manager,
                     const gchar   *ui_path)
{
  GimpPlugInManager *plug_in_manager;
  GTree             *menu_entries;
  GSList            *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  plug_in_manager = manager->gimp->plug_in_manager;

  menu_entries = g_tree_new_full ((GCompareDataFunc) strcmp, NULL,
                                  g_free,
                                  (GDestroyNotify) plug_in_menu_entry_free);

  for (list = plug_in_manager->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      GimpPlugInProcedure *plug_in_proc = list->data;

      if (! plug_in_proc->file)
        continue;

      g_signal_connect_object (plug_in_proc, "menu-path-added",
                               G_CALLBACK (plug_in_menus_menu_path_added),
                               manager, 0);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
          GList *path;

          for (path = plug_in_proc->menu_paths; path; path = g_list_next (path))
            {
              if (g_str_has_prefix (path->data, manager->name))
                {
                  PlugInMenuEntry *entry = g_slice_new0 (PlugInMenuEntry);
                  gchar           *menu;

                  entry->proc      = plug_in_proc;
                  entry->menu_path = path->data;

                  menu = g_strconcat (path->data, "/",
                                      plug_in_proc->menu_label,
                                      NULL);

                  plug_in_menus_tree_insert (menu_entries, menu, entry);
                  g_free (menu);
                }
            }
        }
    }

  g_object_set_data (G_OBJECT (manager), "ui-path", (gpointer) ui_path);

  g_tree_foreach (menu_entries,
                  (GTraverseFunc) plug_in_menus_tree_traverse,
                  manager);

  g_object_set_data (G_OBJECT (manager), "ui-path", NULL);

  g_tree_destroy (menu_entries);

  g_signal_connect_object (manager->gimp->pdb, "register-procedure",
                           G_CALLBACK (plug_in_menus_register_procedure),
                           manager, 0);
  g_signal_connect_object (manager->gimp->pdb, "unregister-procedure",
                           G_CALLBACK (plug_in_menus_unregister_procedure),
                           manager, 0);
}


/*  private functions  */

static void
plug_in_menus_register_procedure (GimpPDB       *pdb,
                                  GimpProcedure *procedure,
                                  GimpUIManager *manager)
{
  if (GIMP_IS_PLUG_IN_PROCEDURE (procedure))
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      g_signal_connect_object (plug_in_proc, "menu-path-added",
                               G_CALLBACK (plug_in_menus_menu_path_added),
                               manager, 0);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
          GList *list;

          GIMP_LOG (MENUS, "register procedure: %s", gimp_object_get_name (procedure));

          for (list = plug_in_proc->menu_paths; list; list = g_list_next (list))
            plug_in_menus_menu_path_added (plug_in_proc, list->data, manager);
        }
    }
}

static void
plug_in_menus_unregister_procedure (GimpPDB       *pdb,
                                    GimpProcedure *procedure,
                                    GimpUIManager *manager)
{
  if (GIMP_IS_PLUG_IN_PROCEDURE (procedure))
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      g_signal_handlers_disconnect_by_func (plug_in_proc,
                                            plug_in_menus_menu_path_added,
                                            manager);

      if (plug_in_proc->menu_label && ! plug_in_proc->file_proc)
        {
          GIMP_LOG (MENUS, "unregister procedure: %s", gimp_object_get_name (procedure));

          gimp_ui_manager_remove_ui (manager, gimp_object_get_name (plug_in_proc));
        }
    }
}

static void
plug_in_menus_menu_path_added (GimpPlugInProcedure *plug_in_proc,
                               const gchar         *menu_path,
                               GimpUIManager       *manager)
{
  GIMP_LOG (MENUS, "menu path added: %s (%s)",
           gimp_object_get_name (plug_in_proc), menu_path);

  if (g_str_has_prefix (menu_path, manager->name))
    {
      if (! strcmp (manager->name, "<Image>"))
        {
          plug_in_menus_add_proc (manager, "/image-menubar", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Layers>"))
        {
          plug_in_menus_add_proc (manager, "/layers-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Channels>"))
        {
          plug_in_menus_add_proc (manager, "/channels-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Paths>"))
        {
          plug_in_menus_add_proc (manager, "/paths-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Colormap>"))
        {
          plug_in_menus_add_proc (manager, "/colormap-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Brushes>"))
        {
          plug_in_menus_add_proc (manager, "/brushes-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Dynamics>"))
        {
          plug_in_menus_add_proc (manager, "/dynamics-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<MyPaintBrushes>"))
        {
          plug_in_menus_add_proc (manager, "/mypaint-brushes-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Gradients>"))
        {
          plug_in_menus_add_proc (manager, "/gradients-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Palettes>"))
        {
          plug_in_menus_add_proc (manager, "/palettes-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Patterns>"))
        {
          plug_in_menus_add_proc (manager, "/patterns-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<ToolPresets>"))
        {
          plug_in_menus_add_proc (manager, "/tool-presets-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Fonts>"))
        {
          plug_in_menus_add_proc (manager, "/fonts-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
      else if (! strcmp (manager->name, "<Buffers>"))
        {
          plug_in_menus_add_proc (manager, "/buffers-popup", plug_in_proc,
                                  menu_path + strlen (manager->name));
        }
    }
}

static void
plug_in_menus_add_proc (GimpUIManager       *manager,
                        const gchar         *ui_path,
                        GimpPlugInProcedure *proc,
                        const gchar         *menu_path)
{
  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  if (! proc->menu_label)
    return;

  GIMP_LOG (MENUS, "adding menu item for '%s' (@ %s)",
            gimp_object_get_name (proc), menu_path);

  gimp_ui_manager_add_ui (manager, menu_path,
                          gimp_object_get_name (proc),
                          FALSE);
}

static void
plug_in_menus_tree_insert (GTree           *entries,
                           const gchar     *path,
                           PlugInMenuEntry *entry)
{
  gchar *strip = gimp_strip_uline (path);
  gchar *key;

  /* Append the procedure name to the menu path in order to get a unique
   * key even if two procedures are installed to the same menu entry.
   */
  key = g_strconcat (strip, gimp_object_get_name (entry->proc), NULL);

  g_tree_insert (entries, g_utf8_collate_key (key, -1), entry);

  g_free (key);
  g_free (strip);
}

static gboolean
plug_in_menus_tree_traverse (gpointer         key,
                             PlugInMenuEntry *entry,
                             GimpUIManager   *manager)
{
  const gchar *ui_path = g_object_get_data (G_OBJECT (manager), "ui-path");

  g_return_val_if_fail (g_str_has_prefix (entry->menu_path, manager->name), FALSE);

  plug_in_menus_add_proc (manager, ui_path, entry->proc,
                          entry->menu_path + strlen (manager->name));

  return FALSE;
}

static void
plug_in_menu_entry_free (PlugInMenuEntry *entry)
{
  g_slice_free (PlugInMenuEntry, entry);
}
