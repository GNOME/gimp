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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "menus-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-locale-domain.h"
#include "plug-in/gimppluginprocedure.h"

#include "widgets/gimpuimanager.h"

#include "plug-in-menus.h"

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
static gchar * plug_in_menus_build_path           (GimpUIManager       *manager,
                                                   const gchar         *ui_path,
                                                   guint                merge_id,
                                                   const gchar         *menu_path,
                                                   gboolean             for_menu);
static void    plug_in_menu_entry_free            (PlugInMenuEntry     *entry);


/*  public functions  */

void
plug_in_menus_setup (GimpUIManager *manager,
                     const gchar   *ui_path)
{
  GimpPlugInManager *plug_in_manager;
  GTree             *menu_entries;
  GSList            *list;
  guint              merge_id;
  gint               i;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  plug_in_manager = manager->gimp->plug_in_manager;

  merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));

  for (i = 0; i < manager->gimp->config->plug_in_history_size; i++)
    {
      gchar *action_name;
      gchar *action_path;

      action_name = g_strdup_printf ("plug-in-recent-%02d", i + 1);
      action_path = g_strdup_printf ("%s/Filters/Recently Used/Plug-Ins",
                                     ui_path);

      gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                             action_path, action_name, action_name,
                             GTK_UI_MANAGER_MENUITEM,
                             FALSE);

      g_free (action_name);
      g_free (action_path);
    }

  menu_entries = g_tree_new_full ((GCompareDataFunc) strcmp, NULL,
                                  g_free,
                                  (GDestroyNotify) plug_in_menu_entry_free);

  for (list = plug_in_manager->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      GimpPlugInProcedure *plug_in_proc = list->data;

      if (! plug_in_proc->prog)
        continue;

      g_signal_connect_object (plug_in_proc, "menu-path-added",
                               G_CALLBACK (plug_in_menus_menu_path_added),
                               manager, 0);

      if (plug_in_proc->menu_paths &&
          ! plug_in_proc->file_proc)
        {
          GList *path;

          for (path = plug_in_proc->menu_paths; path; path = g_list_next (path))
            {
              if (g_str_has_prefix (path->data, manager->name))
                {
                  PlugInMenuEntry *entry = g_slice_new0 (PlugInMenuEntry);
                  const gchar     *progname;
                  const gchar     *locale_domain;

                  entry->proc      = plug_in_proc;
                  entry->menu_path = path->data;

                  progname = gimp_plug_in_procedure_get_progname (plug_in_proc);

                  locale_domain =
                    gimp_plug_in_manager_get_locale_domain (plug_in_manager,
                                                            progname, NULL);

                  if (plug_in_proc->menu_label)
                    {
                      gchar *menu;

                      menu = g_strconcat (dgettext (locale_domain,
                                                    path->data),
                                          "/",
                                          dgettext (locale_domain,
                                                    plug_in_proc->menu_label),
                                          NULL);

                      plug_in_menus_tree_insert (menu_entries, menu, entry);
                      g_free (menu);
                    }
                  else
                    {
                      plug_in_menus_tree_insert (menu_entries,
                                                 dgettext (locale_domain,
                                                           path->data),
                                                 entry);
                    }
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

      if ((plug_in_proc->menu_label || plug_in_proc->menu_paths) &&
          ! plug_in_proc->file_proc)
        {
          GList *list;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   gimp_object_get_name (GIMP_OBJECT (procedure)));
#endif

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

      if ((plug_in_proc->menu_label || plug_in_proc->menu_paths) &&
          ! plug_in_proc->file_proc)
        {
          GList *list;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   gimp_object_get_name (GIMP_OBJECT (procedure)));
#endif

          for (list = plug_in_proc->menu_paths; list; list = g_list_next (list))
            {
              if (g_str_has_prefix (list->data, manager->name))
                {
                  gchar *merge_key;
                  guint  merge_id;

                  merge_key = g_strdup_printf ("%s-merge-id",
                                               GIMP_OBJECT (plug_in_proc)->name);
                  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                                  merge_key));
                  g_free (merge_key);

                  if (merge_id)
                    gtk_ui_manager_remove_ui (GTK_UI_MANAGER (manager),
                                              merge_id);

                  break;
                }
            }
        }
    }
}

static void
plug_in_menus_menu_path_added (GimpPlugInProcedure *plug_in_proc,
                               const gchar         *menu_path,
                               GimpUIManager       *manager)
{
#if 0
  g_print ("%s: %s (%s)\n", G_STRFUNC,
           gimp_object_get_name (GIMP_OBJECT (plug_in_proc)), menu_path);
#endif

  if (g_str_has_prefix (menu_path, manager->name))
    {
      if (! strcmp (manager->name, "<Image>"))
        {
          plug_in_menus_add_proc (manager, "/image-menubar",
                                  plug_in_proc, menu_path);
          plug_in_menus_add_proc (manager, "/dummy-menubar/image-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Toolbox>"))
        {
          plug_in_menus_add_proc (manager, "/toolbox-menubar",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Layers>"))
        {
          plug_in_menus_add_proc (manager, "/layers-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Channels>"))
        {
          plug_in_menus_add_proc (manager, "/channels-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Vectors>"))
        {
          plug_in_menus_add_proc (manager, "/vectors-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Colormap>"))
        {
          plug_in_menus_add_proc (manager, "/colormap-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Brushes>"))
        {
          plug_in_menus_add_proc (manager, "/brushes-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Gradients>"))
        {
          plug_in_menus_add_proc (manager, "/gradients-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Palettes>"))
        {
          plug_in_menus_add_proc (manager, "/palettes-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Patterns>"))
        {
          plug_in_menus_add_proc (manager, "/patterns-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Fonts>"))
        {
          plug_in_menus_add_proc (manager, "/fonts-popup",
                                  plug_in_proc, menu_path);
        }
      else if (! strcmp (manager->name, "<Buffers>"))
        {
          plug_in_menus_add_proc (manager, "/buffers-popup",
                                  plug_in_proc, menu_path);
        }
    }
}

static void
plug_in_menus_add_proc (GimpUIManager       *manager,
                        const gchar         *ui_path,
                        GimpPlugInProcedure *proc,
                        const gchar         *menu_path)
{
  gchar *path;
  gchar *merge_key;
  gchar *stripped_path;
  gchar *action_path;
  guint  merge_id;
  guint  menu_merge_id;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  path = g_strdup (menu_path);

  if (! proc->menu_label)
    {
      gchar *p;

      if (! path)
        return;

      p = strrchr (path, '/');
      if (! p)
        {
          g_free (path);
          return;
        }

      *p = '\0';
    }

  merge_key = g_strdup_printf ("%s-merge-id", GIMP_OBJECT (proc)->name);

  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                  merge_key));

  if (! merge_id)
    {
      merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));
      g_object_set_data (G_OBJECT (manager), merge_key,
                         GUINT_TO_POINTER (merge_id));
    }

  g_free (merge_key);

  menu_merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                       "plug-in-menu-merge-id"));

  if (! menu_merge_id)
    {
      menu_merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));
      g_object_set_data (G_OBJECT (manager), "plug-in-menu-merge-id",
                         GUINT_TO_POINTER (menu_merge_id));
    }

  stripped_path = gimp_strip_uline (path);
  action_path = plug_in_menus_build_path (manager, ui_path, menu_merge_id,
                                          stripped_path, FALSE);
  g_free (stripped_path);

  if (! action_path)
    {
      g_free (path);
      return;
    }

#if 0
  g_print ("adding menu item for '%s' (@ %s)\n",
           GIMP_OBJECT (proc)->name, action_path);
#endif

  gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                         action_path,
                         GIMP_OBJECT (proc)->name,
                         GIMP_OBJECT (proc)->name,
                         GTK_UI_MANAGER_MENUITEM,
                         FALSE);

  g_free (action_path);
  g_free (path);
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
  key = g_strconcat (strip,
                     gimp_object_get_name (GIMP_OBJECT (entry->proc)),
                     NULL);

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

  plug_in_menus_add_proc (manager, ui_path, entry->proc, entry->menu_path);

  return FALSE;
}

static gchar *
plug_in_menus_build_path (GimpUIManager *manager,
                          const gchar   *ui_path,
                          guint          merge_id,
                          const gchar   *menu_path,
                          gboolean       for_menu)
{
  gchar *action_path;

  if (! strchr (menu_path, '/'))
    {
      action_path = g_strdup (ui_path);
      goto make_placeholder;
    }

  action_path = g_strdup_printf ("%s%s", ui_path, strchr (menu_path, '/'));

  if (! gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager), action_path))
    {
      gchar *parent_menu_path   = g_strdup (menu_path);
      gchar *parent_action_path = NULL;
      gchar *menu_item_name;

      menu_item_name = strrchr (parent_menu_path, '/');
      *menu_item_name++ = '\0';

      if (menu_item_name)
        parent_action_path = plug_in_menus_build_path (manager,
                                                       ui_path, merge_id,
                                                       parent_menu_path, TRUE);

      if (parent_action_path)
        {
          g_free (action_path);
          action_path = g_strdup_printf ("%s/%s",
                                         parent_action_path, menu_item_name);

          if (! gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager),
                                           action_path))
            {
#if 0
              g_print ("adding menu '%s' at path '%s' for action '%s'\n",
                       menu_item_name, action_path, menu_path);
#endif

              gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                                     parent_action_path, menu_item_name,
                                     menu_path,
                                     GTK_UI_MANAGER_MENU,
                                     FALSE);

              gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                                     action_path, "Menus", NULL,
                                     GTK_UI_MANAGER_PLACEHOLDER,
                                     FALSE);
              gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                                     action_path, "Separator", NULL,
                                     GTK_UI_MANAGER_SEPARATOR,
                                     FALSE);
            }

          g_free (parent_action_path);
        }
      else
        {
          g_free (action_path);
          action_path = NULL;
        }

      g_free (parent_menu_path);
    }

 make_placeholder:

  if (action_path && for_menu)
    {
      gchar *placeholder_path = g_strdup_printf ("%s/%s", action_path, "Menus");

      if (gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager),
                                     placeholder_path))
        {
          g_free (action_path);

          return placeholder_path;
        }

      g_free (placeholder_path);
    }

  return action_path;
}

static void
plug_in_menu_entry_free (PlugInMenuEntry *entry)
{
  g_slice_free (PlugInMenuEntry, entry);
}
