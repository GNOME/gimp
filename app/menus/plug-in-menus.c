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
#include "plug-in/plug-in-proc.h"

#include "widgets/gimpuimanager.h"

#include "plug-in-menus.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static gboolean plug_in_menus_tree_traverse (gpointer       foo,
                                             PlugInProcDef *proc_def,
                                             GimpUIManager *manager);
static void     plug_in_menus_build_path    (GimpUIManager *manager,
                                             const gchar   *ui_path,
                                             guint          merge_id,
                                             const gchar   *path);


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

  menu_entries = g_tree_new_full ((GCompareDataFunc) g_utf8_collate, NULL,
                                  g_free, NULL);

  for (list = manager->gimp->plug_in_proc_defs;
       list;
       list = g_slist_next (list))
    {
      PlugInProcDef *proc_def = list->data;

      if (proc_def->prog         &&
          proc_def->menu_path    &&
          ! proc_def->extensions &&
          ! proc_def->prefixes   &&
          ! proc_def->magics)
        {
          if ((! strncmp (proc_def->menu_path, "<Toolbox>", 9) &&
               ! strcmp (ui_path, "/toolbox-menubar")) ||
              (! strncmp (proc_def->menu_path, "<Image>", 7) &&
               (! strcmp (ui_path, "/image-menubar") ||
                ! strcmp (ui_path, "/image-popup"))))
            {
              const gchar *progname;
              const gchar *locale_domain;
              gchar       *key;

              progname = plug_in_proc_def_get_progname (proc_def);

              locale_domain = plug_ins_locale_domain (manager->gimp,
                                                      progname, NULL);

              key = gimp_strip_uline (dgettext (locale_domain,
                                                proc_def->menu_path));
              g_tree_insert (menu_entries, key, proc_def);
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
                        PlugInProcDef *proc_def)
{
  gchar *path;
  gchar *p;

  g_return_if_fail (GIMP_IS_UI_MANAGER (manager));
  g_return_if_fail (ui_path != NULL);
  g_return_if_fail (proc_def != NULL);

  path = g_strdup (proc_def->menu_path);

  p = strrchr (path, '/');

  if (p)
    {
      gchar *action_path;
      gchar *merge_key;
      guint  merge_id;

      *p = '\0';

      merge_id = gtk_ui_manager_new_merge_id (GTK_UI_MANAGER (manager));

      plug_in_menus_build_path (manager, ui_path, merge_id, path);

      action_path = g_strdup_printf ("%s%s", ui_path, strchr (path, '/'));

#if 0
      g_print ("adding UI for '%s' (@ %s)\n",
               proc_def->db_info.name, action_path);
#endif

      gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                             action_path,
                             proc_def->db_info.name,
                             proc_def->db_info.name,
                             GTK_UI_MANAGER_MENUITEM,
                             FALSE);

      g_free (action_path);

      merge_key = g_strdup_printf ("%s-merge-id", proc_def->db_info.name);
      g_object_set_data (G_OBJECT (manager), merge_key,
                         GUINT_TO_POINTER (merge_id));
      g_free (merge_key);
    }

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
plug_in_menus_tree_traverse (gpointer       foo,
                             PlugInProcDef *proc_def,
                             GimpUIManager *manager)
{
  const gchar *ui_path = g_object_get_data (G_OBJECT (manager), "ui-path");

  plug_in_menus_add_proc (manager, ui_path, proc_def);

  return FALSE;
}

static void
plug_in_menus_build_path (GimpUIManager *manager,
                          const gchar   *ui_path,
                          guint          merge_id,
                          const gchar   *path)
{
  gchar *action_path;

  action_path = g_strdup_printf ("%s%s", ui_path, strchr (path, '/'));

  if (! gtk_ui_manager_get_widget (GTK_UI_MANAGER (manager), action_path))
    {
      gchar *base_path = g_strdup (path);
      gchar *p;

      p = strrchr (base_path, '/');

      if (p)
        {
          gchar *name;

          *p = '\0';

          plug_in_menus_build_path (manager, ui_path, merge_id, base_path);

          p = strrchr (action_path, '/');
          *p = '\0';

#if 0
          g_print ("adding UI for '%s' (@ %s)\n",
                   path, action_path);
#endif

          name = strrchr (path, '/') + 1;

          gtk_ui_manager_add_ui (GTK_UI_MANAGER (manager), merge_id,
                                 action_path, name, path,
                                 GTK_UI_MANAGER_MENU,
                                 FALSE);
        }

      g_free (base_path);
    }

  g_free (action_path);
}
