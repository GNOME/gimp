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

#include "plug-in/plug-in.h"
#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-def.h"
#include "plug-in/plug-in-proc.h"

#include "widgets/gimpitemfactory.h"

#include "plug-in-commands.h"
#include "plug-in-menus.h"

#include "libgimpbase/gimpenv.h"
#include "libgimp/gimpintl.h"


void
plug_in_make_menu (GSList      *plug_in_defs,
                   const gchar *std_plugins_domain)
{
  PlugInDef       *plug_in_def;
  PlugInProcDef   *proc_def;
  PlugInMenuEntry *menu_entry;
  GSList          *domains = NULL;
  GSList          *procs;
  GSList          *tmp;
  GTree           *menu_entries;

#ifdef ENABLE_NLS
  bindtextdomain (std_plugins_domain, gimp_locale_directory ());
  bind_textdomain_codeset (std_plugins_domain, "UTF-8");
  domains = g_slist_append (domains, (gpointer) std_plugins_domain);
#endif

#ifdef ENABLE_NLS
  menu_entries = g_tree_new ((GCompareFunc) strcoll);
#else
  menu_entries = g_tree_new ((GCompareFunc) strcmp);
#endif

  tmp = plug_in_defs;
  while (tmp)
    {
      plug_in_def = tmp->data;
      tmp = tmp->next;

      procs = plug_in_def->proc_defs;

      if (!procs)
	continue;

#ifdef ENABLE_NLS
      {
	gchar    *domain;
	GSList   *list;
	gboolean  found = FALSE;

	if (plug_in_def->locale_domain)
	  {
	    domain = plug_in_def->locale_domain;
	    for (list = domains; list && !found; list = list->next)
	      {
		if (strcmp (domain, (gchar*)(list->data)) == 0)
		  found = TRUE;
	      }
	    if (!found)
	      {
		domains = g_slist_append (domains, domain);
		if (plug_in_def->locale_path)
		  bindtextdomain (domain, plug_in_def->locale_path);
		else
		  bindtextdomain (domain, gimp_locale_directory ());
                bind_textdomain_codeset (domain, "UTF-8");
	      }
	  }
      }
#endif  /*  ENABLE_NLS  */

      while (procs)
	{
	  proc_def = procs->data;
	  procs = procs->next;

	  if (proc_def->prog         &&
              proc_def->menu_path    &&
              ! proc_def->extensions &&
              ! proc_def->prefixes   &&
              ! proc_def->magics)
	    {
	      menu_entry = g_new0 (PlugInMenuEntry, 1);

	      menu_entry->proc_def = proc_def;
	      menu_entry->domain   = (plug_in_def->locale_domain ? 
                                      plug_in_def->locale_domain :
                                      (gchar *) std_plugins_domain);
	      menu_entry->help_path = plug_in_def->help_path;

	      g_tree_insert (menu_entries, 
			     dgettext (menu_entry->domain, 
                                       proc_def->menu_path),
			     menu_entry);
	    }
	}
    }

  g_tree_foreach (menu_entries, 
                  (GTraverseFunc) plug_in_make_menu_entry, 
                  NULL);
  g_tree_destroy (menu_entries);

  g_slist_free (domains);
}

/*  The following function has to be a GTraverseFunction, 
 *  but is also called directly. Please note that it frees the
 *  menu_entry strcuture.                --Sven 
 */ 
gint
plug_in_make_menu_entry (gpointer         foo,
			 PlugInMenuEntry *menu_entry,
			 gpointer         bar)
{
  GimpItemFactoryEntry  entry;
  gchar                *help_page;
  gchar                *basename;
  gchar                *lowercase_page;

  basename = g_path_get_basename (menu_entry->proc_def->prog);

  if (menu_entry->help_path)
    {
      help_page = g_strconcat (menu_entry->help_path,
			       "@",   /* HACK: locale subdir */
			       basename,
			       ".html",
			       NULL);
    }
  else
    {
      help_page = g_strconcat ("filters/",  /* _not_ G_DIR_SEPARATOR_S */
			       basename,
			       ".html",
			       NULL);
    }

  g_free (basename);

  lowercase_page = g_ascii_strdown (help_page, -1);

  g_free (help_page);

  entry.entry.path            = strstr (menu_entry->proc_def->menu_path, "/");
  entry.entry.accelerator     = menu_entry->proc_def->accelerator;
  entry.entry.callback        = plug_in_run_cmd_callback;
  entry.entry.callback_action = 0;
  entry.entry.item_type       = NULL;
  entry.quark_string          = NULL;
  entry.help_page             = lowercase_page;
  entry.description           = NULL;

  {
    GimpItemFactory *item_factory;

    item_factory = 
      gimp_item_factory_from_path (menu_entry->proc_def->menu_path);

    g_object_set_data (G_OBJECT (item_factory), "textdomain",
                       (gpointer) menu_entry->domain);

    gimp_item_factory_create_item (item_factory,
                                   &entry, 
                                   &menu_entry->proc_def->db_info, 2,
                                   TRUE, FALSE);
  }

  g_free (lowercase_page);

  g_free (menu_entry);

  return FALSE;
}

void
plug_in_delete_menu_entry (const gchar *menu_path)
{
  GimpItemFactory *item_factory;

  item_factory = gimp_item_factory_from_path (menu_path);

  gtk_item_factory_delete_item (GTK_ITEM_FACTORY (item_factory), menu_path);
}

void
plug_in_set_menu_sensitivity (GimpImageType type)
{
  GtkItemFactory *item_factory;
  PlugInProcDef  *proc_def;
  GSList         *tmp;
  gboolean        sensitive = FALSE;

  for (tmp = proc_defs; tmp; tmp = g_slist_next (tmp))
    {
      proc_def = tmp->data;

      if (proc_def->image_types_val && proc_def->menu_path)
	{
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

          item_factory =
            GTK_ITEM_FACTORY (gimp_item_factory_from_path (proc_def->menu_path));

	  gimp_item_factory_set_sensitive (item_factory,
                                           proc_def->menu_path,
                                           sensitive);

          if (last_plug_in && (last_plug_in == &(proc_def->db_info)))
	    {
              gchar *basename;
              gchar *ellipses;
              gchar *repeat;
              gchar *reshow;

              basename = g_path_get_basename (proc_def->menu_path);

              ellipses = strstr (basename, "...");

              if (ellipses && ellipses == (basename + strlen (basename) - 3))
                *ellipses = '\0';

              repeat = g_strdup_printf (_("Repeat \"%s\""), basename);
              reshow = g_strdup_printf (_("Re-show \"%s\""), basename);

              g_free (basename);

              gimp_item_factory_set_label (item_factory,
                                           "/Filters/Repeat Last", repeat);
              gimp_item_factory_set_label (item_factory,
                                           "/Filters/Re-Show Last", reshow);

              g_free (repeat);
              g_free (reshow);

	      gimp_item_factory_set_sensitive (item_factory,
                                               "/Filters/Repeat Last",
                                               sensitive);
	      gimp_item_factory_set_sensitive (item_factory,
                                               "/Filters/Re-Show Last",
                                               sensitive);
	    }
	}
    }

  if (! last_plug_in)
    {
      item_factory =
        GTK_ITEM_FACTORY (gimp_item_factory_from_path ("<Image>"));

      gimp_item_factory_set_label (item_factory,
                                   "/Filters/Repeat Last",
                                   _("Repeat Last"));
      gimp_item_factory_set_label (item_factory,
                                   "/Filters/Re-Show Last",
                                   _("Re-Show Last"));

      gimp_item_factory_set_sensitive (item_factory,
                                       "/Filters/Repeat Last", FALSE);
      gimp_item_factory_set_sensitive (item_factory,
                                       "/Filters/Re-Show Last", FALSE);
    }
}
