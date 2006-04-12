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

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "pdb/gimppluginprocedure.h"

#include "plug-in/plug-in-help-domain.h"
#include "plug-in/plug-in-locale-domain.h"
#include "plug-in/plug-in-menu-branch.h"

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

static const GimpActionEntry plug_in_actions[] =
{
  { "plug-in-menu",                   NULL, N_("Filte_rs")          },
  { "plug-in-recent-menu",            NULL, N_("Recently Used")     },
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
    N_("Reset all _Filters"), NULL,
    N_("Set all plug-in to their default settings"),
    G_CALLBACK (plug_in_reset_all_cmd_callback),
    GIMP_HELP_FILTER_RESET_ALL }
};

static const GimpEnumActionEntry plug_in_repeat_actions[] =
{
  { "plug-in-repeat", GTK_STOCK_EXECUTE,
    N_("Re_peat Last"), "<control>F",
    N_("Rerun the last used plug-in using the same settings"),
    0, FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "plug-in-reshow", GIMP_STOCK_RESHOW_FILTER,
    N_("R_e-Show Last"), "<control><shift>F",
    N_("Show the last used plug-in dialog again"),
    0, FALSE,
    GIMP_HELP_FILTER_RESHOW }
};


/*  public functions  */

void
plug_in_actions_setup (GimpActionGroup *group)
{
  GimpEnumActionEntry *entries;
  GSList              *list;
  gint                 n_entries;
  gint                 i;

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

  for (list = group->gimp->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->prog         &&
          proc->menu_paths   &&
          ! proc->extensions &&
          ! proc->prefixes   &&
          ! proc->magics)
        {
          plug_in_actions_add_proc (group, proc);
        }
    }

  n_entries = group->gimp->config->plug_in_history_size;

  entries = g_new0 (GimpEnumActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name           = g_strdup_printf ("plug-in-recent-%02d",
                                                   i + 1);
      entries[i].stock_id       = GIMP_STOCK_RESHOW_FILTER;
      entries[i].label          = "";
      entries[i].tooltip        = NULL;
      entries[i].value          = i;
      entries[i].value_variable = FALSE;
      entries[i].help_id        = GIMP_HELP_FILTER_RESHOW;
      entries[i].accelerator    = "";
    }

  gimp_action_group_add_enum_actions (group, entries, n_entries,
                                      G_CALLBACK (plug_in_repeat_cmd_callback));

  for (i = 0; i < n_entries; i++)
    {
      gimp_action_group_set_action_visible (group, entries[i].name, FALSE);
      g_free ((gchar *) entries[i].name);
    }

  g_free (entries);

  g_signal_connect_object (group->gimp, "last-plug-ins-changed",
                           G_CALLBACK (plug_in_actions_last_changed),
                           group, 0);

  plug_in_actions_last_changed (group->gimp, group);
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage     *image = action_data_get_image (data);
  GimpImageType  type   = -1;
  GSList        *list;
  gint           i;

  if (image)
    {
      GimpDrawable *drawable = gimp_image_active_drawable (image);

      if (drawable)
        type = gimp_drawable_type (drawable);
    }

  for (list = group->gimp->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->menu_paths      &&
          proc->image_types_val &&
          ! proc->extensions    &&
          ! proc->prefixes      &&
          ! proc->magics)
        {
          gboolean sensitive = gimp_plug_in_procedure_get_sensitive (proc, type);

          gimp_action_group_set_action_sensitive (group,
                                                  GIMP_OBJECT (proc)->name,
                                                  sensitive);
        }
    }

  if (group->gimp->last_plug_ins &&
      gimp_plug_in_procedure_get_sensitive (group->gimp->last_plug_ins->data, type))
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", TRUE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", TRUE);
    }
  else
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", FALSE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", FALSE);
    }

  for (list = group->gimp->last_plug_ins, i = 0; list; list = list->next, i++)
    {
      GimpPlugInProcedure *proc = list->data;
      gchar               *name     = g_strdup_printf ("plug-in-recent-%02d",
                                                       i + 1);
      gboolean             sensitive;

      sensitive = gimp_plug_in_procedure_get_sensitive (proc, type);

      gimp_action_group_set_action_sensitive (group, name, sensitive);

      g_free (name);
    }
}

void
plug_in_actions_add_proc (GimpActionGroup     *group,
                          GimpPlugInProcedure *proc)
{
  GimpPlugInActionEntry  entry;
  const gchar           *progname;
  const gchar           *locale_domain;
  const gchar           *help_domain;
  const gchar           *label;
  const gchar           *tooltip          = NULL;
  gchar                 *path_original    = NULL;
  gchar                 *path_translated  = NULL;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  progname = gimp_plug_in_procedure_get_progname (proc);

  locale_domain = plug_in_locale_domain (group->gimp, progname, NULL);
  help_domain   = plug_in_help_domain (group->gimp, progname, NULL);

  if (proc->menu_label)
    {
      label = dgettext (locale_domain, proc->menu_label);
    }
  else
    {
      gchar *p1, *p2;

      path_original   = proc->menu_paths->data;
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

      label = p2 + 1;
    }

  if (GIMP_PROCEDURE (proc)->blurb)
    tooltip = dgettext (locale_domain, GIMP_PROCEDURE (proc)->blurb);

  entry.name        = GIMP_OBJECT (proc)->name;
  entry.stock_id    = gimp_plug_in_procedure_get_stock_id (proc);
  entry.label       = label;
  entry.accelerator = NULL;
  entry.tooltip     = tooltip;
  entry.procedure   = proc;
  entry.help_id     = gimp_plug_in_procedure_get_help_id (proc, help_domain);

#if 0
  g_print ("adding plug-in action '%s' (%s)\n",
           GIMP_OBJECT (proc)->name, label);
#endif

  gimp_action_group_add_plug_in_actions (group, &entry, 1,
                                         G_CALLBACK (plug_in_run_cmd_callback));

  g_free ((gchar *) entry.help_id);

  if (proc->menu_label)
    {
      GList *list;

      for (list = proc->menu_paths; list; list = g_list_next (list))
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
plug_in_actions_add_path (GimpActionGroup     *group,
                          GimpPlugInProcedure *proc,
                          const gchar         *menu_path)
{
  const gchar *progname;
  const gchar *locale_domain;
  const gchar *path_translated;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));
  g_return_if_fail (menu_path != NULL);

  progname = gimp_plug_in_procedure_get_progname (proc);

  locale_domain = plug_in_locale_domain (group->gimp, progname, NULL);

  path_translated = dgettext (locale_domain, menu_path);

  if (plug_in_actions_check_translation (menu_path, path_translated))
    plug_in_actions_build_path (group, menu_path, path_translated);
  else
    plug_in_actions_build_path (group, menu_path, menu_path);
}

void
plug_in_actions_remove_proc (GimpActionGroup     *group,
                             GimpPlugInProcedure *proc)
{
  GtkAction *action;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (proc));

  action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                        GIMP_OBJECT (proc)->name);

  if (action)
    {
#if 0
      g_print ("removing plug-in action '%s'\n",
               GIMP_OBJECT (proc)->name);
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

  locale_domain = plug_in_locale_domain (group->gimp, progname, NULL);

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
  GSList      *list;
  const gchar *progname;
  const gchar *domain;
  gint         i;

  if (gimp->last_plug_ins)
    {
      GimpPlugInProcedure *proc = gimp->last_plug_ins->data;
      gchar               *label;
      gchar               *repeat;
      gchar               *reshow;

      progname = gimp_plug_in_procedure_get_progname (proc);
      domain   = plug_in_locale_domain (gimp, progname, NULL);

      label = gimp_plug_in_procedure_get_label (proc, domain);

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

  for (list = gimp->last_plug_ins, i = 0; list; list = list->next, i++)
    {
      GtkAction           *action;
      GimpPlugInProcedure *proc = list->data;
      gchar               *name = g_strdup_printf ("plug-in-recent-%02d",
                                                   i + 1);
      gchar               *label;

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), name);
      g_free (name);

      progname = gimp_plug_in_procedure_get_progname (proc);
      domain   = plug_in_locale_domain (gimp, progname, NULL);

      label = gimp_plug_in_procedure_get_label (proc, domain);

      g_object_set (action,
                    "label",    label,
                    "visible",  TRUE,
                    "stock-id", gimp_plug_in_procedure_get_stock_id (proc),
                    NULL);

      g_free (label);
    }

  for (; i < gimp->config->plug_in_history_size; i++)
    {
      GtkAction *action;
      gchar     *name = g_strdup_printf ("plug-in-recent-%02d", i + 1);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), name);
      g_free (name);

      g_object_set (action,
                    "visible", FALSE,
                    NULL);
    }

  /* update sensitivity of the actions */
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
