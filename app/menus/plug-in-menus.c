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

#include "gui-types.h"

#include "core/gimp.h"

#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-def.h"
#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-in-run.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "plug-in-commands.h"
#include "plug-in-menus.h"

#include "gimp-intl.h"


typedef struct _PlugInMenuEntry PlugInMenuEntry;

struct _PlugInMenuEntry
{
  PlugInProcDef *proc_def;
  const gchar   *locale_domain;
  const gchar   *help_domain;
};


/*  local function prototypes  */

static gboolean plug_in_menu_tree_traverse_func (gpointer         foo,
                                                 PlugInMenuEntry *menu_entry,
                                                 GimpItemFactory *item_factory);


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
plug_in_menus_create (GimpItemFactory *item_factory,
                      GSList          *proc_defs)
{
  GSList *procs;
  GTree  *menu_entries;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));
  g_return_if_fail (proc_defs != NULL);

  menu_entries = g_tree_new_full ((GCompareDataFunc) g_utf8_collate, NULL,
                                  NULL, g_free);

  for (procs = proc_defs; procs; procs = procs->next)
    {
      PlugInProcDef *proc_def = procs->data;

      if (proc_def->prog         &&
          proc_def->menu_path    &&
          ! proc_def->extensions &&
          ! proc_def->prefixes   &&
          ! proc_def->magics)
        {
          PlugInMenuEntry *menu_entry;
          const gchar     *progname;
          const gchar     *locale_domain;
          const gchar     *help_domain;

          progname = plug_in_proc_def_get_progname (proc_def);

          locale_domain = plug_ins_locale_domain (item_factory->gimp,
                                                  progname, NULL);
          help_domain = plug_ins_help_domain (item_factory->gimp,
                                              progname, NULL);

          menu_entry = g_new0 (PlugInMenuEntry, 1);

          menu_entry->proc_def      = proc_def;
          menu_entry->locale_domain = locale_domain;
          menu_entry->help_domain   = help_domain;

          g_tree_insert (menu_entries,
                         dgettext (locale_domain, proc_def->menu_path),
                         menu_entry);
        }
    }

  g_tree_foreach (menu_entries,
                  (GTraverseFunc) plug_in_menu_tree_traverse_func,
                  item_factory);
  g_tree_destroy (menu_entries);
}

void
plug_in_menus_create_entry (GimpItemFactory *item_factory,
                            PlugInProcDef   *proc_def,
                            const gchar     *locale_domain,
                            const gchar     *help_domain)
{
  GimpItemFactoryEntry  entry;
  gchar                *help_id;

  g_return_if_fail (item_factory == NULL ||
                    GIMP_IS_ITEM_FACTORY (item_factory));
  g_return_if_fail (proc_def != NULL);

  help_id = plug_in_proc_def_get_help_id (proc_def, help_domain);

  entry.entry.path            = strstr (proc_def->menu_path, "/");
  entry.entry.accelerator     = proc_def->accelerator;
  entry.entry.callback        = plug_in_run_cmd_callback;
  entry.entry.callback_action = 0;
  entry.entry.item_type       = NULL;
  entry.quark_string          = NULL;
  entry.help_id               = help_id;
  entry.description           = NULL;

  if (item_factory)
    {
      gchar *factory_path;

      factory_path = GTK_ITEM_FACTORY (item_factory)->path;

      if (! strncmp (proc_def->menu_path,
                     factory_path, strlen (factory_path)))
        {
          gimp_item_factory_create_item (item_factory,
                                         &entry,
                                         locale_domain,
                                         &proc_def->db_info, 2, FALSE);
        }
    }
  else
    {
      GList *list;

      for (list = gimp_item_factories_from_path (proc_def->menu_path);
           list;
           list = g_list_next (list))
        {
          item_factory = list->data;

          gimp_item_factory_create_item (item_factory,
                                         &entry,
                                         locale_domain,
                                         &proc_def->db_info, 2, FALSE);
        }
    }

  g_free (help_id);
}

void
plug_in_menus_delete_entry (const gchar *menu_path)
{
  GList *list;
  gchar *path;

  g_return_if_fail (menu_path != NULL);

  path = gimp_strip_uline (menu_path);

  for (list = gimp_item_factories_from_path (menu_path);
       list;
       list = g_list_next (list))
    {
      GtkItemFactory *item_factory = list->data;

      gtk_item_factory_delete_item (GTK_ITEM_FACTORY (item_factory), path);
    }

  g_free (path);
}

void
plug_in_menus_update (GimpItemFactory *item_factory,
                      GimpImageType    type)
{
  GSList   *list;
  gchar    *factory_path;
  gint      factory_path_len;
  gboolean  is_image_factory = FALSE;

  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));

  factory_path     = GTK_ITEM_FACTORY (item_factory)->path;
  factory_path_len = strlen (factory_path);

  if (! strcmp (factory_path, "<Image>"))
    is_image_factory = TRUE;

  for (list = item_factory->gimp->plug_in_proc_defs;
       list;
       list = g_slist_next (list))
    {
      PlugInProcDef *proc_def = list->data;

      if (proc_def->menu_path && proc_def->image_types_val)
        {
          gboolean sensitive;

          switch (type)
            {
            case GIMP_RGB_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_RGB_IMAGE;
              break;
            case GIMP_RGBA_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_RGBA_IMAGE;
              break;
            case GIMP_GRAY_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_GRAY_IMAGE;
              break;
            case GIMP_GRAYA_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_GRAYA_IMAGE;
              break;
            case GIMP_INDEXED_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_INDEXED_IMAGE;
              break;
            case GIMP_INDEXEDA_IMAGE:
              sensitive = proc_def->image_types_val & PLUG_IN_INDEXEDA_IMAGE;
              break;
            default:
              sensitive = FALSE;
              break;
            }

          if (! strncmp (proc_def->menu_path, factory_path, factory_path_len))
            {
              gchar *menu_path;

              menu_path = gimp_strip_uline (proc_def->menu_path);

              gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                               menu_path,
                                               sensitive);

              g_free (menu_path);
            }

          if (is_image_factory                 &&
              item_factory->gimp->last_plug_in &&
              item_factory->gimp->last_plug_in == &proc_def->db_info)
            {
              const gchar *progname;
              const gchar *path;
              gchar       *stripped;
              gchar       *basename;
              gchar       *ellipses;
              gchar       *repeat;
              gchar       *reshow;

              progname = plug_in_proc_def_get_progname (proc_def);

              path = dgettext (plug_ins_locale_domain (item_factory->gimp,
                                                       progname, NULL),
                               proc_def->menu_path);

              stripped = gimp_strip_uline (path);
              basename = g_path_get_basename (stripped);

              g_free (stripped);

              ellipses = strstr (basename, "...");

              if (ellipses && ellipses == (basename + strlen (basename) - 3))
                *ellipses = '\0';

              repeat = g_strdup_printf (_("Re_peat \"%s\""), basename);
              reshow = g_strdup_printf (_("R_e-show \"%s\""), basename);

              g_free (basename);

              gimp_item_factory_set_label (GTK_ITEM_FACTORY (item_factory),
                                           "/Filters/Repeat Last", repeat);
              gimp_item_factory_set_label (GTK_ITEM_FACTORY (item_factory),
                                           "/Filters/Re-Show Last", reshow);

              g_free (repeat);
              g_free (reshow);

	      gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                               "/Filters/Repeat Last",
                                               sensitive);
	      gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                               "/Filters/Re-Show Last",
                                               sensitive);
	    }
	}
    }

  if (is_image_factory && ! item_factory->gimp->last_plug_in)
    {
      gimp_item_factory_set_label (GTK_ITEM_FACTORY (item_factory),
                                   "/Filters/Repeat Last",
                                   _("Repeat Last"));
      gimp_item_factory_set_label (GTK_ITEM_FACTORY (item_factory),
                                   "/Filters/Re-Show Last",
                                   _("Re-Show Last"));

      gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                       "/Filters/Repeat Last", FALSE);
      gimp_item_factory_set_sensitive (GTK_ITEM_FACTORY (item_factory),
                                       "/Filters/Re-Show Last", FALSE);
    }
}


/*  private functions  */

static gboolean
plug_in_menu_tree_traverse_func (gpointer         foo,
                                 PlugInMenuEntry *menu_entry,
                                 GimpItemFactory *item_factory)
{
  plug_in_menus_create_entry (item_factory,
                              menu_entry->proc_def,
                              menu_entry->locale_domain,
                              menu_entry->help_domain);

  return FALSE;
}
