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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "plug-in/gimppluginmanager.h"
#include "plug-in/gimppluginmanager-help-domain.h"
#include "plug-in/gimppluginmanager-history.h"
#include "plug-in/gimppluginmanager-locale-domain.h"
#include "plug-in/gimppluginmanager-menu-branch.h"
#include "plug-in/gimppluginprocedure.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     plug_in_actions_menu_branch_added    (GimpPlugInManager   *manager,
                                                      GFile               *file,
                                                      const gchar         *menu_path,
                                                      const gchar         *menu_label,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_register_procedure   (GimpPDB             *pdb,
                                                      GimpProcedure       *procedure,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_unregister_procedure (GimpPDB             *pdb,
                                                      GimpProcedure       *procedure,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_menu_path_added      (GimpPlugInProcedure *proc,
                                                      const gchar         *menu_path,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_add_proc             (GimpActionGroup     *group,
                                                      GimpPlugInProcedure *proc);

static void     plug_in_actions_history_changed      (GimpPlugInManager   *manager,
                                                      GimpActionGroup     *group);
static gboolean plug_in_actions_check_translation    (const gchar         *original,
                                                      const gchar         *translated);
static void     plug_in_actions_build_path           (GimpActionGroup     *group,
                                                      const gchar         *original,
                                                      const gchar         *translated);


/*  private variables  */

static const GimpActionEntry plug_in_actions[] =
{
  { "plug-in-menu",                NULL, NC_("plug-in-action",
                                             "Filte_rs")          },
  { "plug-in-recent-menu",         NULL, NC_("plug-in-action",
                                             "Recently Used")     },
  { "plug-in-blur-menu",           NULL, NC_("plug-in-action",
                                             "_Blur")             },
  { "plug-in-noise-menu",          NULL, NC_("plug-in-action",
                                             "_Noise")            },
  { "plug-in-edge-detect-menu",    NULL, NC_("plug-in-action",
                                             "Edge-De_tect")      },
  { "plug-in-enhance-menu",        NULL, NC_("plug-in-action",
                                             "En_hance")          },
  { "plug-in-combine-menu",        NULL, NC_("plug-in-action",
                                             "C_ombine")          },
  { "plug-in-generic-menu",        NULL, NC_("plug-in-action",
                                             "_Generic")          },
  { "plug-in-light-shadow-menu",   NULL, NC_("plug-in-action",
                                             "_Light and Shadow") },
  { "plug-in-distorts-menu",       NULL, NC_("plug-in-action",
                                             "_Distorts")         },
  { "plug-in-artistic-menu",       NULL, NC_("plug-in-action",
                                             "_Artistic")         },
  { "plug-in-decor-menu",          NULL, NC_("plug-in-action",
                                             "_Decor")            },
  { "plug-in-map-menu",            NULL, NC_("plug-in-action",
                                             "_Map")              },
  { "plug-in-render-menu",         NULL, NC_("plug-in-action",
                                             "_Render")           },
  { "plug-in-render-clouds-menu",  NULL, NC_("plug-in-action",
                                             "_Clouds")           },
  { "plug-in-render-fractals-menu", NULL, NC_("plug-in-action",
                                             "_Fractals")         },
  { "plug-in-render-nature-menu",  NULL, NC_("plug-in-action",
                                             "_Nature")           },
  { "plug-in-render-noise-menu",   NULL, NC_("plug-in-action",
                                             "N_oise")            },
  { "plug-in-render-pattern-menu", NULL, NC_("plug-in-action",
                                             "_Pattern")          },
  { "plug-in-web-menu",            NULL, NC_("plug-in-action",
                                             "_Web")              },
  { "plug-in-animation-menu",      NULL, NC_("plug-in-action",
                                             "An_imation")        },

  { "plug-in-reset-all", GIMP_STOCK_RESET,
    NC_("plug-in-action", "Reset all _Filters"), NULL,
    NC_("plug-in-action", "Reset all plug-ins to their default settings"),
    G_CALLBACK (plug_in_reset_all_cmd_callback),
    GIMP_HELP_FILTER_RESET_ALL }
};

static const GimpEnumActionEntry plug_in_repeat_actions[] =
{
  { "plug-in-repeat", "system-run",
    NC_("plug-in-action", "Re_peat Last"), "<primary>F",
    NC_("plug-in-action",
        "Rerun the last used plug-in using the same settings"),
    GIMP_RUN_WITH_LAST_VALS, FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "plug-in-reshow", GIMP_STOCK_RESHOW_FILTER,
    NC_("plug-in-action", "R_e-Show Last"), "<primary><shift>F",
    NC_("plug-in-action", "Show the last used plug-in dialog again"),
    GIMP_RUN_INTERACTIVE, FALSE,
    GIMP_HELP_FILTER_RESHOW }
};


/*  public functions  */

void
plug_in_actions_setup (GimpActionGroup *group)
{
  GimpPlugInManager     *manager = group->gimp->plug_in_manager;
  GimpPlugInActionEntry *entries;
  GSList                *list;
  gint                   n_entries;
  gint                   i;

  gimp_action_group_add_actions (group, "plug-in-action",
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions));

  gimp_action_group_add_enum_actions (group, "plug-in-action",
                                      plug_in_repeat_actions,
                                      G_N_ELEMENTS (plug_in_repeat_actions),
                                      G_CALLBACK (plug_in_repeat_cmd_callback));

  for (list = gimp_plug_in_manager_get_menu_branches (manager);
       list;
       list = g_slist_next (list))
    {
      GimpPlugInMenuBranch *branch = list->data;

      plug_in_actions_menu_branch_added (manager,
                                         branch->file,
                                         branch->menu_path,
                                         branch->menu_label,
                                         group);
    }

  g_signal_connect_object (manager,
                           "menu-branch-added",
                           G_CALLBACK (plug_in_actions_menu_branch_added),
                           group, 0);

  for (list = manager->plug_in_procedures;
       list;
       list = g_slist_next (list))
    {
      GimpPlugInProcedure *plug_in_proc = list->data;

      if (plug_in_proc->file)
        plug_in_actions_register_procedure (group->gimp->pdb,
                                            GIMP_PROCEDURE (plug_in_proc),
                                            group);
    }

  g_signal_connect_object (group->gimp->pdb, "register-procedure",
                           G_CALLBACK (plug_in_actions_register_procedure),
                           group, 0);
  g_signal_connect_object (group->gimp->pdb, "unregister-procedure",
                           G_CALLBACK (plug_in_actions_unregister_procedure),
                           group, 0);

  n_entries = gimp_plug_in_manager_history_size (manager);

  entries = g_new0 (GimpPlugInActionEntry, n_entries);

  for (i = 0; i < n_entries; i++)
    {
      entries[i].name        = g_strdup_printf ("plug-in-recent-%02d", i + 1);
      entries[i].icon_name   = NULL;
      entries[i].label       = "";
      entries[i].accelerator = "";
      entries[i].tooltip     = NULL;
      entries[i].procedure   = NULL;
      entries[i].help_id     = GIMP_HELP_FILTER_RESHOW;
    }

  gimp_action_group_add_plug_in_actions (group, entries, n_entries,
                                         G_CALLBACK (plug_in_history_cmd_callback));

  for (i = 0; i < n_entries; i++)
    {
      gimp_action_group_set_action_visible (group, entries[i].name, FALSE);
      g_free ((gchar *) entries[i].name);
    }

  g_free (entries);

  g_signal_connect_object (manager, "history-changed",
                           G_CALLBACK (plug_in_actions_history_changed),
                           group, 0);

  plug_in_actions_history_changed (manager, group);
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage         *image    = action_data_get_image (data);
  GimpPlugInManager *manager  = group->gimp->plug_in_manager;
  GimpDrawable      *drawable = NULL;
  GSList            *list;
  gint               i;

  if (image)
    drawable = gimp_image_get_active_drawable (image);

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if ((proc->menu_label || proc->menu_paths) &&
          ! proc->file_proc                      &&
          proc->image_types_val)
        {
          gboolean sensitive = gimp_plug_in_procedure_get_sensitive (proc,
                                                                     drawable);

          gimp_action_group_set_action_sensitive (group,
                                                  gimp_object_get_name (proc),
                                                  sensitive);
        }
    }

  if (manager->history &&
      gimp_plug_in_procedure_get_sensitive (manager->history->data, drawable))
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", TRUE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", TRUE);
    }
  else
    {
      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", FALSE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", FALSE);
    }

  for (list = manager->history, i = 0; list; list = list->next, i++)
    {
      GimpPlugInProcedure *proc = list->data;
      gchar               *name = g_strdup_printf ("plug-in-recent-%02d",
                                                   i + 1);
      gboolean             sensitive;

      sensitive = gimp_plug_in_procedure_get_sensitive (proc, drawable);

      gimp_action_group_set_action_sensitive (group, name, sensitive);

      g_free (name);
    }
}


/*  private functions  */

static void
plug_in_actions_menu_branch_added (GimpPlugInManager *manager,
                                   GFile             *file,
                                   const gchar       *menu_path,
                                   const gchar       *menu_label,
                                   GimpActionGroup   *group)
{
  const gchar *locale_domain;
  const gchar *path_translated;
  const gchar *label_translated;
  gchar       *full;
  gchar       *full_translated;

  locale_domain = gimp_plug_in_manager_get_locale_domain (manager, file, NULL);

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

static void
plug_in_actions_register_procedure (GimpPDB         *pdb,
                                    GimpProcedure   *procedure,
                                    GimpActionGroup *group)
{
  if (GIMP_IS_PLUG_IN_PROCEDURE (procedure))
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      g_signal_connect_object (plug_in_proc, "menu-path-added",
                               G_CALLBACK (plug_in_actions_menu_path_added),
                               group, 0);

      if ((plug_in_proc->menu_label || plug_in_proc->menu_paths) &&
          ! plug_in_proc->file_proc)
        {
#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   gimp_object_get_name (procedure));
#endif

          plug_in_actions_add_proc (group, plug_in_proc);
        }
    }
}

static void
plug_in_actions_unregister_procedure (GimpPDB         *pdb,
                                      GimpProcedure   *procedure,
                                      GimpActionGroup *group)
{
  if (GIMP_IS_PLUG_IN_PROCEDURE (procedure))
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      g_signal_handlers_disconnect_by_func (plug_in_proc,
                                            plug_in_actions_menu_path_added,
                                            group);

      if ((plug_in_proc->menu_label || plug_in_proc->menu_paths) &&
          ! plug_in_proc->file_proc)
        {
          GtkAction *action;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   gimp_object_get_name (procedure));
#endif

          action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                gimp_object_get_name (procedure));

          if (action)
            gtk_action_group_remove_action (GTK_ACTION_GROUP (group), action);
        }
    }
}

static void
plug_in_actions_menu_path_added (GimpPlugInProcedure *plug_in_proc,
                                 const gchar         *menu_path,
                                 GimpActionGroup     *group)
{
  const gchar *locale_domain;
  const gchar *path_translated;

#if 0
  g_print ("%s: %s (%s)\n", G_STRFUNC,
           gimp_object_get_name (plug_in_proc), menu_path);
#endif

  locale_domain = gimp_plug_in_procedure_get_locale_domain (plug_in_proc);

  path_translated = dgettext (locale_domain, menu_path);

  if (plug_in_actions_check_translation (menu_path, path_translated))
    plug_in_actions_build_path (group, menu_path, path_translated);
  else
    plug_in_actions_build_path (group, menu_path, menu_path);
}

static void
plug_in_actions_add_proc (GimpActionGroup     *group,
                          GimpPlugInProcedure *proc)
{
  GimpPlugInActionEntry  entry;
  const gchar           *locale_domain;
  const gchar           *label;
  gchar                 *path_original    = NULL;
  gchar                 *path_translated  = NULL;

  locale_domain = gimp_plug_in_procedure_get_locale_domain (proc);

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

      if (p1 && p2)
        {
          *p1 = '\0';
          *p2 = '\0';

          label = p2 + 1;
        }
      else
        {
          g_warning ("bad menu path for procedure \"%s\": \"%s\"",
                     gimp_object_get_name (proc), path_original);

          g_free (path_original);
          g_free (path_translated);

          return;
        }
    }

  entry.name        = gimp_object_get_name (proc);
  entry.icon_name   = gimp_plug_in_procedure_get_icon_name (proc);
  entry.label       = label;
  entry.accelerator = NULL;
  entry.tooltip     = gimp_plug_in_procedure_get_blurb (proc);
  entry.procedure   = proc;
  entry.help_id     = gimp_plug_in_procedure_get_help_id (proc);

#if 0
  g_print ("adding plug-in action '%s' (%s)\n",
           gimp_object_get_name (proc), label);
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

  if ((proc->menu_label || proc->menu_paths) &&
      ! proc->file_proc                      &&
      proc->image_types_val)
    {
      GimpContext  *context  = gimp_get_user_context (group->gimp);
      GimpImage    *image    = gimp_context_get_image (context);
      GimpDrawable *drawable = NULL;
      gboolean      sensitive;

      if (image)
        drawable = gimp_image_get_active_drawable (image);

      sensitive = gimp_plug_in_procedure_get_sensitive (proc, drawable);

      gimp_action_group_set_action_sensitive (group,
                                              gimp_object_get_name (proc),
                                              sensitive);
    }
}

static void
plug_in_actions_history_changed (GimpPlugInManager *manager,
                                 GimpActionGroup   *group)
{
  GimpPlugInProcedure *proc;
  gint                 i;

  proc = gimp_plug_in_manager_history_nth (manager, 0);

  if (proc)
    {
      GtkAction   *actual_action;
      const gchar *label;
      gchar       *repeat;
      gchar       *reshow;
      gboolean     sensitive = FALSE;

      label = gimp_plug_in_procedure_get_label (proc);

      /*  copy the sensitivity of the plug-in procedure's actual action
       *  instead of calling plug_in_actions_update() because doing the
       *  latter would set the sensitivity of this image's action on
       *  all images' actions. See bug #517683.
       */
      actual_action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                   gimp_object_get_name (proc));
      if (actual_action)
        sensitive = gtk_action_get_sensitive (actual_action);

      repeat = g_strdup_printf (_("Re_peat \"%s\""),  label);
      reshow = g_strdup_printf (_("R_e-Show \"%s\""), label);

      gimp_action_group_set_action_label (group, "plug-in-repeat", repeat);
      gimp_action_group_set_action_label (group, "plug-in-reshow", reshow);

      gimp_action_group_set_action_sensitive (group,
                                              "plug-in-repeat", sensitive);
      gimp_action_group_set_action_sensitive (group,
                                              "plug-in-reshow", sensitive);

      g_free (repeat);
      g_free (reshow);
    }
  else
    {
      gimp_action_group_set_action_label (group, "plug-in-repeat",
                                          _("Repeat Last"));
      gimp_action_group_set_action_label (group, "plug-in-reshow",
                                          _("Re-Show Last"));

      gimp_action_group_set_action_sensitive (group, "plug-in-repeat", FALSE);
      gimp_action_group_set_action_sensitive (group, "plug-in-reshow", FALSE);
    }

  for (i = 0; i < gimp_plug_in_manager_history_length (manager); i++)
    {
      GtkAction   *action;
      GtkAction   *actual_action;
      const gchar *label;
      gchar       *name;
      gboolean     sensitive = FALSE;

      name = g_strdup_printf ("plug-in-recent-%02d", i + 1);
      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), name);
      g_free (name);

      proc = gimp_plug_in_manager_history_nth (manager, i);

      if (proc->menu_label)
        {
          label = dgettext (gimp_plug_in_procedure_get_locale_domain (proc),
                            proc->menu_label);
        }
      else
        {
          label = gimp_plug_in_procedure_get_label (proc);
        }

      /*  see comment above  */
      actual_action = gtk_action_group_get_action (GTK_ACTION_GROUP (group),
                                                   gimp_object_get_name (proc));
      if (actual_action)
        sensitive = gtk_action_get_sensitive (actual_action);

      g_object_set (action,
                    "visible",   TRUE,
                    "sensitive", sensitive,
                    "procedure", proc,
                    "label",     label,
                    "icon-name", gimp_plug_in_procedure_get_icon_name (proc),
                    "tooltip",   gimp_plug_in_procedure_get_blurb (proc),
                    NULL);
    }

  for (; i < gimp_plug_in_manager_history_size (manager); i++)
    {
      GtkAction *action;
      gchar     *name = g_strdup_printf ("plug-in-recent-%02d", i + 1);

      action = gtk_action_group_get_action (GTK_ACTION_GROUP (group), name);
      g_free (name);

      g_object_set (action,
                    "visible",   FALSE,
                    "procedure", NULL,
                    NULL);
    }
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
      GtkAction *action;
      gchar     *label;

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

      /* recursively call ourselves with the last part of the path removed */
      plug_in_actions_build_path (group, copy_original, copy_translated);
    }

  g_free (copy_original);
  g_free (copy_translated);
}
