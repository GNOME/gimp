/* LIGMA - The GNU Image Manipulation Program
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmadrawable.h"
#include "core/ligmaimage.h"

#include "plug-in/ligmapluginmanager.h"
#include "plug-in/ligmapluginmanager-help-domain.h"
#include "plug-in/ligmapluginmanager-menu-branch.h"
#include "plug-in/ligmapluginprocedure.h"

#include "widgets/ligmaaction.h"
#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmaactionimpl.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void     plug_in_actions_menu_branch_added    (LigmaPlugInManager   *manager,
                                                      GFile               *file,
                                                      const gchar         *menu_path,
                                                      const gchar         *menu_label,
                                                      LigmaActionGroup     *group);
static void     plug_in_actions_register_procedure   (LigmaPDB             *pdb,
                                                      LigmaProcedure       *procedure,
                                                      LigmaActionGroup     *group);
static void     plug_in_actions_unregister_procedure (LigmaPDB             *pdb,
                                                      LigmaProcedure       *procedure,
                                                      LigmaActionGroup     *group);
static void     plug_in_actions_menu_path_added      (LigmaPlugInProcedure *proc,
                                                      const gchar         *menu_path,
                                                      LigmaActionGroup     *group);
static void     plug_in_actions_add_proc             (LigmaActionGroup     *group,
                                                      LigmaPlugInProcedure *proc);

static void     plug_in_actions_build_path           (LigmaActionGroup     *group,
                                                      const gchar         *translated);


/*  private variables  */

static const LigmaActionEntry plug_in_actions[] =
{
  { "plug-in-reset-all", LIGMA_ICON_RESET,
    NC_("plug-in-action", "Reset all _Filters"), NULL,
    NC_("plug-in-action", "Reset all plug-ins to their default settings"),
    plug_in_reset_all_cmd_callback,
    LIGMA_HELP_FILTER_RESET_ALL }
};


/*  public functions  */

void
plug_in_actions_setup (LigmaActionGroup *group)
{
  LigmaPlugInManager *manager = group->ligma->plug_in_manager;
  GSList            *list;

  ligma_action_group_add_actions (group, "plug-in-action",
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions));

  for (list = ligma_plug_in_manager_get_menu_branches (manager);
       list;
       list = g_slist_next (list))
    {
      LigmaPlugInMenuBranch *branch = list->data;

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
      LigmaPlugInProcedure *plug_in_proc = list->data;

      if (plug_in_proc->file)
        plug_in_actions_register_procedure (group->ligma->pdb,
                                            LIGMA_PROCEDURE (plug_in_proc),
                                            group);
    }

  g_signal_connect_object (group->ligma->pdb, "register-procedure",
                           G_CALLBACK (plug_in_actions_register_procedure),
                           group, 0);
  g_signal_connect_object (group->ligma->pdb, "unregister-procedure",
                           G_CALLBACK (plug_in_actions_unregister_procedure),
                           group, 0);
}

void
plug_in_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
  LigmaImage         *image    = action_data_get_image (data);
  LigmaPlugInManager *manager  = group->ligma->plug_in_manager;
  GSList            *list;

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      LigmaPlugInProcedure *proc = list->data;

      if (proc->menu_label  &&
          ! proc->file_proc &&
          proc->image_types_val)
        {
          LigmaProcedure *procedure = LIGMA_PROCEDURE (proc);
          gboolean       sensitive;
          const gchar   *tooltip;
          const gchar   *reason;

          sensitive = ligma_procedure_get_sensitive (procedure,
                                                    LIGMA_OBJECT (image),
                                                    &reason);

          ligma_action_group_set_action_sensitive (group,
                                                  ligma_object_get_name (proc),
                                                  sensitive, reason);

          tooltip = ligma_procedure_get_blurb (procedure);
          if (tooltip)
            ligma_action_group_set_action_tooltip (group,
                                                  ligma_object_get_name (proc),
                                                  tooltip);
        }
    }
}


/*  private functions  */

static void
plug_in_actions_menu_branch_added (LigmaPlugInManager *manager,
                                   GFile             *file,
                                   const gchar       *menu_path,
                                   const gchar       *menu_label,
                                   LigmaActionGroup   *group)
{
  gchar *full;

  full = g_strconcat (menu_path, "/", menu_label, NULL);
  plug_in_actions_build_path (group, full);

  g_free (full);
}

static void
plug_in_actions_register_procedure (LigmaPDB         *pdb,
                                    LigmaProcedure   *procedure,
                                    LigmaActionGroup *group)
{
  if (LIGMA_IS_PLUG_IN_PROCEDURE (procedure))
    {
      LigmaPlugInProcedure *plug_in_proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

      g_signal_connect_object (plug_in_proc, "menu-path-added",
                               G_CALLBACK (plug_in_actions_menu_path_added),
                               group, 0);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   ligma_object_get_name (procedure));
#endif

          plug_in_actions_add_proc (group, plug_in_proc);
        }
    }
}

static void
plug_in_actions_unregister_procedure (LigmaPDB         *pdb,
                                      LigmaProcedure   *procedure,
                                      LigmaActionGroup *group)
{
  if (LIGMA_IS_PLUG_IN_PROCEDURE (procedure))
    {
      LigmaPlugInProcedure *plug_in_proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

      g_signal_handlers_disconnect_by_func (plug_in_proc,
                                            plug_in_actions_menu_path_added,
                                            group);

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
          LigmaAction *action;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   ligma_object_get_name (procedure));
#endif

          action = ligma_action_group_get_action (group,
                                                 ligma_object_get_name (procedure));

          if (action)
            ligma_action_group_remove_action (group, action);
        }
    }
}

static void
plug_in_actions_menu_path_added (LigmaPlugInProcedure *plug_in_proc,
                                 const gchar         *menu_path,
                                 LigmaActionGroup     *group)
{
#if 0
  g_print ("%s: %s (%s)\n", G_STRFUNC,
           ligma_object_get_name (plug_in_proc), menu_path);
#endif

  plug_in_actions_build_path (group, menu_path);
}

static void
plug_in_actions_add_proc (LigmaActionGroup     *group,
                          LigmaPlugInProcedure *proc)
{
  LigmaProcedureActionEntry  entry;
  GList                    *list;

  entry.name        = ligma_object_get_name (proc);
  entry.icon_name   = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (proc));
  entry.label       = ligma_procedure_get_menu_label (LIGMA_PROCEDURE (proc));
  entry.accelerator = NULL;
  entry.tooltip     = ligma_procedure_get_blurb (LIGMA_PROCEDURE (proc));
  entry.procedure   = LIGMA_PROCEDURE (proc);
  entry.help_id     = ligma_procedure_get_help_id (LIGMA_PROCEDURE (proc));

  ligma_action_group_add_procedure_actions (group, &entry, 1,
                                           plug_in_run_cmd_callback);

  for (list = proc->menu_paths; list; list = g_list_next (list))
    {
      const gchar *original = list->data;

      plug_in_actions_build_path (group, original);
    }

  if (proc->image_types_val)
    {
      LigmaContext  *context  = ligma_get_user_context (group->ligma);
      LigmaImage    *image    = ligma_context_get_image (context);
      gboolean      sensitive;
      const gchar  *tooltip;
      const gchar  *reason;

      sensitive = ligma_procedure_get_sensitive (LIGMA_PROCEDURE (proc),
                                                LIGMA_OBJECT (image),
                                                &reason);

      ligma_action_group_set_action_sensitive (group,
                                              ligma_object_get_name (proc),
                                              sensitive, reason);

      tooltip = ligma_procedure_get_blurb (LIGMA_PROCEDURE (proc));
      if (tooltip)
        ligma_action_group_set_action_tooltip (group,
                                              ligma_object_get_name (proc),
                                              tooltip);
    }
}

static void
plug_in_actions_build_path (LigmaActionGroup *group,
                            const gchar     *path_translated)
{
  GHashTable *path_table;
  gchar      *copy_translated;
  gchar      *p2;

  path_table = g_object_get_data (G_OBJECT (group), "plug-in-path-table");

  if (! path_table)
    {
      path_table = g_hash_table_new_full (g_str_hash, g_str_equal,
                                          g_free, NULL);

      g_object_set_data_full (G_OBJECT (group), "plug-in-path-table",
                              path_table,
                              (GDestroyNotify) g_hash_table_destroy);
    }

  copy_translated = g_strdup (path_translated);

  p2 = strrchr (copy_translated, '/');

  if (p2 && ! g_hash_table_lookup (path_table, copy_translated))
    {
      LigmaAction *action;
      gchar      *label;

      label = p2 + 1;

#if 0
      g_print ("adding plug-in submenu '%s' (%s)\n",
               copy_translated, label);
#endif

      action = ligma_action_impl_new (copy_translated, label, NULL, NULL, NULL);
      ligma_action_group_add_action (group, action);
      g_object_unref (action);

      g_hash_table_insert (path_table, g_strdup (copy_translated), action);

      *p2 = '\0';

      /* recursively call ourselves with the last part of the path removed */
      plug_in_actions_build_path (group, copy_translated);
    }

  g_free (copy_translated);
}
