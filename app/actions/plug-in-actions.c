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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-proc-def.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     plug_in_actions_last_changed      (Gimp            *gimp,
                                                   GimpActionGroup *group);
static gboolean plug_in_actions_check_translation (const gchar     *original,
                                                   const gchar     *translated);
static void     plug_in_actions_build_path        (GimpActionGroup *group,
                                                   const gchar     *original,
                                                   const gchar     *translated);


/*  private variables  */

static GimpActionEntry plug_in_actions[] =
{
  { "plug-in-menu",                   NULL, N_("Filte_rs")          },
  { "plug-in-blur-menu",              NULL, N_("_Blur")             },
  { "plug-in-noise-menu",             NULL, N_("_Noise")            },
  { "plug-in-edge-detect-menu",       NULL, N_("Edge-De_tect")      },
  { "plug-in-enhance-menu",           NULL, N_("En_hance")          },
  { "plug-in-combine-menu",           NULL, N_("C_ombine")          },
  { "plug-in-generic-menu",           NULL, N_("_Generic")          },
  { "plug-in-light-shadow-menu",      NULL, N_("_Light and Shadow") },
  { "plug-in-distorts-menu",          NULL, N_("_Distorts")         },
  { "plug-in-artistic-menu",          NULL, N_("_Artistic")         },
  { "plug-in-map-menu",               NULL, N_("_Map")              },
  { "plug-in-render-menu",            NULL, N_("_Render")           },
  { "plug-in-render-clouds-menu",     NULL, N_("_Clouds")           },
  { "plug-in-render-nature-menu",     NULL, N_("_Nature")           },
  { "plug-in-render-pattern-menu",    NULL, N_("_Pattern")          },
  { "plug-in-web-menu",               NULL, N_("_Web")              },
  { "plug-in-animation-menu",         NULL, N_("An_imation")        },

  { "plug-in-reset-all", GIMP_STOCK_RESET,
    N_("Reset all _Filters"), NULL, NULL,
    G_CALLBACK (plug_in_reset_all_cmd_callback),
    GIMP_HELP_FILTER_RESET_ALL }
};

static GimpEnumActionEntry plug_in_repeat_actions[] =
{
  { "plug-in-repeat", GTK_STOCK_EXECUTE,
    N_("Re_peat Last"), "<control>F", NULL,
    FALSE, FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "plug-in-reshow", GIMP_STOCK_RESHOW_FILTER,
    N_("R_e-Show Last"), "<control><shift>F", NULL,
    TRUE, FALSE,
    GIMP_HELP_FILTER_RESHOW }
};


/*  public functions  */

void
plug_in_actions_setup (GimpActionGroup *group)
{
  GSList *list;

  gimp_action_group_add_actions (group,
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions));

  gimp_action_group_add_enum_actions (group,
                                      plug_in_repeat_actions,
                                      G_N_ELEMENTS (plug_in_repeat_actions),
                                      G_CALLBACK (plug_in_repeat_cmd_callback));

  for (list = group->gimp->plug_in_menu_branches;
       list;
       list = g_slist_next (list))
    {
      PlugInMenuBranch *branch = list->data;

      plug_in_actions_add_branch (group,
                                  branch->prog_name,
                                  branch->menu_path,
                                  branch->menu_label);
    }

  for (list = group->gimp->plug_in_proc_defs;
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
          plug_in_actions_add_proc (group, proc_def);
        }
    }

  g_signal_connect_object (group->gimp, "last-plug-in-changed",
                           G_CALLBACK (plug_in_actions_last_changed),
                           group, 0);

  plug_in_actions_last_changed (group->gimp, group);
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage     *gimage = action_data_get_image (data);
  GimpImageType  type   = -1;
  GSList        *list;

  if (gimage)
    {
      GimpDrawable *drawable = gimp_image_active_drawable (gimage);

      if (drawable)
        type = gimp_drawable_type (drawable);
    }

  for (list = group->gimp->plug_in_proc_defs;
       list;
       list = g_slist_next (list))
    {
      PlugInProcDef *proc_def = list->data;

      if (proc_def->menu_paths      &&
          proc_def->image_types_val &&
          ! proc_def->extensions    &&
          ! proc_def->prefixes      &&
          ! proc_def->magics)
        {
          gboolean sensitive = plug_in_proc_def_get_sensitive (proc_def, type);

          gimp_action_group_set_action_sensitive (group,
                                                  proc_def->db_info.name,
                                                  sensitive);
	}
    }

  if (group->gimp->last_plug_in &&
      plug_in_proc_def_get_sensitive (group->gimp->last_plug_in, type))
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", TRUE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", TRUE);
    }
  else
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", FALSE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", FALSE);
    }
}

void
plug_in_actions_add_proc (GimpActionGroup *group,
                          PlugInProcDef   *proc_def)
{
  GimpPlugInActionEntry  entry;
  const gchar           *progname;
  const gchar           *locale_domain;
  const gchar           *help_domain;
  const gchar           *label_translated;
  gchar                 *path_original    = NULL;
  gchar                 *path_translated  = NULL;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (proc_def != NULL);

  progname = plug_in_proc_def_get_progname (proc_def);

  locale_domain = plug_ins_locale_domain (group->gimp, progname, NULL);
  help_domain   = plug_ins_help_domain (group->gimp, progname, NULL);

  if (proc_def->menu_label)
    {
      label_translated = dgettext (locale_domain, proc_def->menu_label);
    }
  else
    {
      gchar *p1, *p2;

      path_original   = proc_def->menu_paths->data;
      path_translated = dgettext (locale_domain, path_original);

      path_original = g_strdup (path_original);

      if (plug_in_actions_check_translation (path_original, path_translated))
        path_translated = g_strdup (path_translated);
      else
        path_translated = g_strdup (path_original);

      p1 = strrchr (path_original, '/');
      p2 = strrchr (path_translated, '/');

      *p1 = '\0';
      *p2 = '\0';

      label_translated = p2 + 1;
    }

  entry.name        = proc_def->db_info.name;
  entry.stock_id    = plug_in_proc_def_get_stock_id (proc_def);
  entry.label       = label_translated;
  entry.accelerator = NULL;
  entry.tooltip     = NULL;
  entry.proc_def    = proc_def;
  entry.help_id     = plug_in_proc_def_get_help_id (proc_def, help_domain);

#if 0
  g_print ("adding plug-in action '%s' (%s)\n",
           proc_def->db_info.name, label_translated);
#endif

  gimp_action_group_add_plug_in_actions (group, &entry, 1,
                                         G_CALLBACK (plug_in_run_cmd_callback));

  g_free ((gchar *) entry.help_id);

  if (proc_def->menu_label)
    {
      GList *list;

      for (list = proc_def->menu_paths; list; list = g_list_next (list))
        {
          const gchar *original   = list->data;
          const gchar *translated = dgettext (locale_domain, original);

          if (plug_in_actions_check_translation (original, translated))
            plug_in_actions_build_path (group, original, translated);
          else
            plug_in_actions_build_path (group, original, original);
        }
    }
  else
    {
      plug_in_actions_build_path (group, path_original, path_translated);

      g_free (path_original);
      g_free (path_translated);
    }
}

void
plug_in_actions_add_path (GimpActionGroup *group,
                          PlugInProcDef   *proc_def,
                          const gchar     *menu_path)
{
  const gchar *progname;
  const gchar *locale_domain;
  const gchar *path_translated;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (proc_def != NULL);
  g_return_if_fail (menu_path != NULL);

  progname = plug_in_proc_def_get_progname (proc_def);

  locale_domain = plug_ins_locale_domain (group->gimp, progname, NULL);

  path_translated = dgettext (locale_domain, menu_path);

  if (plug_in_actions_check_translation (menu_path, path_translated))
    plug_in_actions_build_path (group, menu_path, path_translated);
  else
    plug_in_actions_build_path (group, menu_path, menu_path);
}

void
plug_in_actions_remove_proc (GimpActionGroup *group,
                             PlugInProcDef   *proc_def)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (proc_def != NULL);

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        proc_def->db_info.name);

  if (action)
    {
#if 0
      g_print ("removing plug-in action '%s'\n",
               proc_def->db_info.name);
#endif

      gtk_action_group_remove_action (GTK_ACTION_GROUP (group), action);
    }
}

void
plug_in_actions_add_branch (GimpActionGroup *group,
                            const gchar     *progname,
                            const gchar     *menu_path,
                            const gchar     *menu_label)
{
  const gchar *locale_domain;
  const gchar *path_translated;
  const gchar *label_translated;
  gchar       *full;
  gchar       *full_translated;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (menu_label != NULL);

  locale_domain = plug_ins_locale_domain (group->gimp, progname, NULL);

  path_translated  = dgettext (locale_domain, menu_path);
  label_translated = dgettext (locale_domain, menu_label);

  full            = g_strconcat (menu_path,       "/", menu_label,       NULL);
  full_translated = g_strconcat (path_translated, "/", label_translated, NULL);

  if (plug_in_actions_check_translation (full, full_translated))
    plug_in_actions_build_path (group, full, full_translated);
  else
    plug_in_actions_build_path (group, full, full);

  g_free (full_translated);
  g_free (full);
}


/*  private functions  */

static void
plug_in_actions_last_changed (Gimp            *gimp,
                              GimpActionGroup *group)
{
  if (gimp->last_plug_in)
    {
      PlugInProcDef *proc_def = gimp->last_plug_in;
      const gchar   *progname;
      const gchar   *domain;
      gchar         *label;
      gchar         *repeat;
      gchar         *reshow;

      progname = plug_in_proc_def_get_progname (proc_def);
      domain   = plug_ins_locale_domain (gimp, progname, NULL);

      label = plug_in_proc_def_get_label (proc_def, domain);

      repeat = g_strdup_printf (_("Re_peat \"%s\""),  label);
      reshow = g_strdup_printf (_("R_e-Show \"%s\""), label);

      g_free (label);

      gimp_action_group_set_action_label (group, "plug-in-repeat", repeat);
      gimp_action_group_set_action_label (group, "plug-in-reshow", reshow);

      g_free (repeat);
      g_free (reshow);
    }
  else
    {
      gimp_action_group_set_action_label (group, "plug-in-repeat",
                                          _("Repeat Last"));
      gimp_action_group_set_action_label (group, "plug-in-reshow",
                                          _("Re-Show Last"));
    }

  /* update sensitivity of the "plug-in-repeat" and "plug-in-reshow" actions */
  plug_in_actions_update (group, gimp);
}

static gboolean
plug_in_actions_check_translation (const gchar *original,
                                   const gchar *translated)
{
  const gchar *p1;
  const gchar *p2;

  /*  first check if <Prefix> is present and identical in both strings  */
  p1 = strchr (original, '>');
  p2 = strchr (translated, '>');

  if (!p1 || !p2                           ||
      (p1 - original) != (p2 - translated) ||
      strncmp (original, translated, p1 - original))
    {
      g_printerr ("bad translation \"%s\"\n"
                  "for menu path   \"%s\"\n"
                  "(<Prefix> must not be translated)\n\n",
                  translated, original);
      return FALSE;
    }

  p1++;
  p2++;

  /*  then check if either a '/' or nothing follows in *both* strings  */
  if (! ((*p1 == '/' && *p2 == '/') ||
         (*p1 == '\0' && *p2 == '\0')))
    {
      g_printerr ("bad translation \"%s\"\n"
                  "for menu path   \"%s\"\n"
                  "(<Prefix> must be followed by either nothing or '/')\n\n",
                  translated, original);
      return FALSE;
    }

  /*  then check the number of slashes in the remaining string  */
  while (p1 && p2)
    {
      p1 = strchr (p1, '/');
      p2 = strchr (p2, '/');

      if (p1) p1++;
      if (p2) p2++;
    }

  if (p1 || p2)
    {
      g_printerr ("bad translation \"%s\"\n"
                  "for menu path   \"%s\"\n"
                  "(number of '/' must be the same)\n\n",
                  translated, original);
      return FALSE;
    }

  return TRUE;
}

static void
plug_in_actions_build_path (GimpActionGroup *group,
                            const gchar     *path_original,
                            const gchar     *path_translated)
{
  GHashTable *path_table;
  gchar      *copy_original;
  gchar      *copy_translated;
  gchar      *p1, *p2;

  path_table = g_object_get_data (G_OBJECT (group), "plug-in-path-table");

  if (! path_table)
    {
      path_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, NULL);

      g_object_set_data_full (G_OBJECT (group), "plug-in-path-table",
                              path_table,
                              (GDestroyNotify) g_hash_table_destroy);
    }

  copy_original   = gimp_strip_uline (path_original);
  copy_translated = g_strdup (path_translated);

  p1 = strrchr (copy_original, '/');
  p2 = strrchr (copy_translated, '/');

  if (p1 && p2 && ! g_hash_table_lookup (path_table, copy_original))
    {
      gchar     *label;
      GtkAction *action;

      label = p2 + 1;

#if 0
      g_print ("adding plug-in submenu '%s' (%s)\n",
               copy_original, label);
#endif

      action = gtk_action_new (copy_original, label, NULL, NULL);
      gtk_action_group_add_action (GTK_ACTION_GROUP (group), action);
      g_object_unref (action);

      g_hash_table_insert (path_table, g_strdup (copy_original), action);

      *p1 = '\0';
      *p2 = '\0';

      plug_in_actions_build_path (group, copy_original, copy_translated);
    }

  g_free (copy_original);
  g_free (copy_translated);
}
