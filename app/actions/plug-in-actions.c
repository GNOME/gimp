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
#include "plug-in/gimppluginmanager-menu-branch.h"
#include "plug-in/gimppluginprocedure.h"

#include "widgets/gimpaction.h"
#include "widgets/gimpactiongroup.h"
#include "widgets/gimpactionimpl.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void     plug_in_actions_register_procedure   (GimpPDB             *pdb,
                                                      GimpProcedure       *procedure,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_unregister_procedure (GimpPDB             *pdb,
                                                      GimpProcedure       *procedure,
                                                      GimpActionGroup     *group);
static void     plug_in_actions_add_proc             (GimpActionGroup     *group,
                                                      GimpPlugInProcedure *proc);


/*  private variables  */

static const GimpActionEntry plug_in_actions[] =
{
  { "plug-in-reset-all", GIMP_ICON_RESET,
    NC_("plug-in-action", "Reset all _Filters"), NULL, { NULL },
    NC_("plug-in-action", "Reset all plug-ins to their default settings"),
    plug_in_reset_all_cmd_callback,
    GIMP_HELP_FILTER_RESET_ALL }
};


/*  public functions  */

void
plug_in_actions_setup (GimpActionGroup *group)
{
  GimpPlugInManager *manager = group->gimp->plug_in_manager;
  GSList            *list;

  gimp_action_group_add_actions (group, "plug-in-action",
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions));

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
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage         *image    = action_data_get_image (data);
  GimpPlugInManager *manager  = group->gimp->plug_in_manager;
  GSList            *list;

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if (proc->menu_label && ! proc->file_proc)
        {
          GimpProcedure *procedure = GIMP_PROCEDURE (proc);
          gboolean       sensitive;
          const gchar   *tooltip;
          const gchar   *reason = NULL;

          sensitive = gimp_procedure_get_sensitive (procedure,
                                                    GIMP_OBJECT (image),
                                                    &reason);

          gimp_action_group_set_action_sensitive (group,
                                                  gimp_object_get_name (proc),
                                                  sensitive, reason);

          tooltip = gimp_procedure_get_blurb (procedure);
          if (tooltip)
            gimp_action_group_set_action_tooltip (group,
                                                  gimp_object_get_name (proc),
                                                  tooltip);
        }
    }
}


/*  private functions  */

static void
plug_in_actions_register_procedure (GimpPDB         *pdb,
                                    GimpProcedure   *procedure,
                                    GimpActionGroup *group)
{
  if (GIMP_IS_PLUG_IN_PROCEDURE (procedure))
    {
      GimpPlugInProcedure *plug_in_proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      if (plug_in_proc->menu_label &&
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

      if (plug_in_proc->menu_label &&
          ! plug_in_proc->file_proc)
        {
          GimpAction *action;

#if 0
          g_print ("%s: %s\n", G_STRFUNC,
                   gimp_object_get_name (procedure));
#endif

          action = gimp_action_group_get_action (group,
                                                 gimp_object_get_name (procedure));

          if (action)
            gimp_action_group_remove_action (group, action);
        }
    }
}

static void
plug_in_actions_add_proc (GimpActionGroup     *group,
                          GimpPlugInProcedure *proc)
{
  GimpProcedureActionEntry  entry;

  entry.name        = gimp_object_get_name (proc);
  entry.icon_name   = gimp_viewable_get_icon_name (GIMP_VIEWABLE (proc));
  entry.label       = gimp_procedure_get_menu_label (GIMP_PROCEDURE (proc));
  entry.accelerator = NULL;
  entry.tooltip     = gimp_procedure_get_blurb (GIMP_PROCEDURE (proc));
  entry.procedure   = GIMP_PROCEDURE (proc);
  entry.help_id     = gimp_procedure_get_help_id (GIMP_PROCEDURE (proc));

  gimp_action_group_add_procedure_actions (group, &entry, 1,
                                           plug_in_run_cmd_callback);

  if (proc->image_types_val)
    {
      GimpContext  *context  = gimp_get_user_context (group->gimp);
      GimpImage    *image    = gimp_context_get_image (context);
      gboolean      sensitive;
      const gchar  *tooltip;
      const gchar  *reason;

      sensitive = gimp_procedure_get_sensitive (GIMP_PROCEDURE (proc),
                                                GIMP_OBJECT (image),
                                                &reason);

      gimp_action_group_set_action_sensitive (group,
                                              gimp_object_get_name (proc),
                                              sensitive, reason);

      tooltip = gimp_procedure_get_blurb (GIMP_PROCEDURE (proc));
      if (tooltip)
        gimp_action_group_set_action_tooltip (group,
                                              gimp_object_get_name (proc),
                                              tooltip);
    }
}
