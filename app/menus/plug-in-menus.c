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

#include "string.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "menus-types.h"

#include "core/gimp.h"

#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-def.h"
#include "plug-in/plug-in-proc-def.h"

#include "widgets/gimpuimanager.h"

#include "plug-in-menus.h"

#include "gimp-intl.h"


typedef struct _PlugInMenuEntry PlugInMenuEntry;

struct _PlugInMenuEntry
{
  PlugInProcDef *proc_def;
  const gchar   *menu_path;
};


/*  local function prototypes  */

static gboolean   plug_in_menus_tree_traverse (gpointer         key,
                                               PlugInMenuEntry *entry,
                                               GimpUIManager   *manager);
static gchar    * plug_in_menus_build_path    (GimpUIManager   *manager,
                                               const gchar     *ui_path,
                                               guint            merge_id,
                                               const gchar     *menu_path,
                                               gboolean         for_menu);


/*  public functions  */

void
plug_in_menus_init (Gimp        *gimp,
                    GSList      *plug_in_defs,
                    const gchar *std_plugins_domain)
{
  GSList *domains = NULL;
  GSList *tmp;

  g_return_if_fail (std_plugins_domain != NULL);

  domains = g_slist_append (domains, (gpointer) std_plugins_domain);

  bindtextdomain (std_plugins_domain, gimp_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (std_plugins_domain, "UTF-8");
#endif

  for (tmp = plug_in_defs; tmp; tmp = g_slist_next (tmp))
    {
      PlugInDef   *plug_in_def;
      const gchar *locale_domain;
      const gchar *locale_path;
      GSList      *list;

      plug_in_def = (PlugInDef *) tmp->data;

      if (! plug_in_def->proc_defs)
        continue;

      locale_domain = plug_ins_locale_domain (gimp,
                                              plug_in_def->prog,
                                              &locale_path);

      for (list = domains; list; list = list->next)
        if (! strcmp (locale_domain, (const gchar *) list->data))
          break;

      if (! list)
        {
          domains = g_slist_append (domains, (gpointer) locale_domain);

          bindtextdomain (locale_domain, locale_path);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
          bind_textdomain_codeset (locale_domain, "UTF-8");
#endif
        }
    }

  g_slist_free (domains);
}

void
plug_in_menus_setup (GimpUIManager *manager,
                     const gchar   *ui_path)
{
  GTree  *menu_entries;
  GSList *list;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);

  menu_entries = g_tree_new_full ((GCompareDataFunc) strcmp, NULL,
                                  g_free, g_free);

  for (list = manager->gimp->plug_in_proc_defs;
       list;
       list = g_slist_next (list))
    {
      PlugInProcDef *proc_def = list->data;

      if (proc_def->prog         &&
          proc_def->menu_paths   &&
          ! proc_def->extensions &&
          ! proc_def->prefixes   &&
          ! proc_def->magics)
        {
          GList *path;

          for (path = proc_def->menu_paths; path; path = g_list_next (path))
            {
              if ((! strncmp (path->data, "<Toolbox>", 9) &&
                   ! strcmp (ui_path, "/toolbox-menubar")) ||
                  (! strncmp (path->data, "<Image>", 7) &&
                   (! strcmp (ui_path, "/image-menubar") ||
                    ! strcmp (ui_path, "/dummy-menubar/image-popup"))))
                {
                  PlugInMenuEntry *entry = g_new0 (PlugInMenuEntry, 1);
                  const gchar     *progname;
                  const gchar     *locale_domain;
                  gchar           *key;

                  entry->proc_def  = proc_def;
                  entry->menu_path = path->data;

                  progname = plug_in_proc_def_get_progname (proc_def);

                  locale_domain = plug_ins_locale_domain (manager->gimp,
                                                          progname, NULL);

                  if (proc_def->menu_label)
                    {
                      gchar *menu;
                      gchar *strip;

                      menu = g_strconcat (dgettext (locale_domain,
                                                    path->data),
                                          "/",
                                          dgettext (locale_domain,
                                                    proc_def->menu_label),
                                          NULL);

                      strip = gimp_strip_uline (menu);

                      key = g_utf8_collate_key (strip, -1);

                      g_free (strip);
                      g_free (menu);
                    }
                  else
                    {
                      gchar *strip = gimp_strip_uline (dgettext (locale_domain,
                                                                 path->data));

                      key = g_utf8_collate_key (strip, -1);

                      g_free (strip);
                    }

                  g_tree_insert (menu_entries, key, entry);
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
}

void
plug_in_menus_add_proc (GimpUIManager *manager,
                        const gchar   *ui_path,
                        PlugInProcDef *proc_def,
                        const gchar   *menu_path)
{
  gchar *path;
  gchar *merge_key;
  gchar *action_path;
  guint  merge_id;
  guint  menu_merge_id;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (proc_def != NULL);

  path = g_strdup (menu_path);

  if (! proc_def->menu_label)
    {
      gchar *p = strrchr (path, '/');

      if (! p)
        {
          g_free (path);
          return;
        }

      *p = '\0';
    }

  merge_key = g_strdup_printf ("%s-merge-id", proc_def->db_info.name);

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

  action_path = plug_in_menus_build_path (manager, ui_path, menu_merge_id,
                                          path, FALSE);

  if (! action_path)
    {
      g_free (path);
      return;
    }

#if 0
  g_print ("adding menu item for '%s' (@ %s)\n",
           proc_def->db_info.name, action_path);
#endif

  gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                         action_path,
                         proc_def->db_info.name,
                         proc_def->db_info.name,
                         GTK_UI_MANAGER_MENUITEM,
                         FALSE);

  g_free (action_path);
  g_free (path);
}

void
plug_in_menus_remove_proc (GimpUIManager *manager,
                           PlugInProcDef *proc_def)
{
  gchar *merge_key;
  guint  merge_id;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (proc_def != NULL);

  merge_key = g_strdup_printf ("%s-merge-id", proc_def->db_info.name);
  merge_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (manager),
                                                  merge_key));
  g_free (merge_key);

  if (merge_id)
    gtk_ui_manager_remove_ui (GTK_UI_MANAGER (manager), merge_id);
}


/*  private functions  */

static gboolean
plug_in_menus_tree_traverse (gpointer         key,
                             PlugInMenuEntry *entry,
                             GimpUIManager   *manager)
{
  const gchar *ui_path = g_object_get_data (G_OBJECT (manager), "ui-path");

  plug_in_menus_add_proc (manager, ui_path, entry->proc_def, entry->menu_path);

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
      gchar *parent_menu_path = g_strdup (menu_path);
      gchar *menu_item_name;
      gchar *parent_action_path;

      menu_item_name = strrchr (parent_menu_path, '/');
      *menu_item_name++ = '\0';

      parent_action_path = plug_in_menus_build_path (manager, ui_path, merge_id,
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
      gchar *placeholder_path = g_strdup_printf ("%s/%s",
                                                 action_path, "Menus");

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
