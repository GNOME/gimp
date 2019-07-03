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
#include "plug-in/gimppluginmanager-locale-domain.h"
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

static gboolean plug_in_actions_check_translation    (const gchar         *original,
                                                      const gchar         *translated);
static void     plug_in_actions_build_path           (GimpActionGroup     *group,
                                                      const gchar         *original,
                                                      const gchar         *translated);


/*  private variables  */

static const GimpActionEntry plug_in_actions[] =
{
  { "plug-in-reset-all", GIMP_ICON_RESET,
    NC_("plug-in-action", "Reset all _Filters"), NULL,
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
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
  GimpImage         *image    = action_data_get_image (data);
  GimpPlugInManager *manager  = group->gimp->plug_in_manager;
  GimpDrawable      *drawable = NULL;
  GSList            *list;

  if (image)
    drawable = gimp_image_get_active_drawable (image);

  for (list = manager->plug_in_procedures; list; list = g_slist_next (list))
    {
      GimpPlugInProcedure *proc = list->data;

      if ((proc->menu_label || proc->menu_paths) &&
          ! proc->file_proc                      &&
          proc->image_types_val)
        {
          GimpProcedure *procedure = GIMP_PROCEDURE (proc);
          gboolean       sensitive;
          const gchar   *tooltip;

          sensitive = gimp_procedure_get_sensitive (procedure,
                                                    GIMP_OBJECT (drawable),
                                                    &tooltip);

          gimp_action_group_set_action_sensitive (group,
                                                  gimp_object_get_name (proc),
                                                  sensitive);

          if (sensitive || ! drawable || ! tooltip)
            tooltip = gimp_procedure_get_blurb (procedure);

          gimp_action_group_set_action_tooltip (group,
                                                gimp_object_get_name (proc),
                                                tooltip);
        }
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
  GimpProcedureActionEntry  entry;
  const gchar              *locale_domain;
  gchar                    *path_original    = NULL;
  gchar                    *path_translated  = NULL;

  locale_domain = gimp_plug_in_procedure_get_locale_domain (proc);

  if (! proc->menu_label)
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
  entry.icon_name   = gimp_viewable_get_icon_name (GIMP_VIEWABLE (proc));
  entry.label       = gimp_procedure_get_menu_label (GIMP_PROCEDURE (proc));
  entry.accelerator = NULL;
  entry.tooltip     = gimp_procedure_get_blurb (GIMP_PROCEDURE (proc));
  entry.procedure   = GIMP_PROCEDURE (proc);
  entry.help_id     = gimp_procedure_get_help_id (GIMP_PROCEDURE (proc));

  gimp_action_group_add_procedure_actions (group, &entry, 1,
                                           plug_in_run_cmd_callback);

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
      const gchar  *tooltip;

      if (image)
        drawable = gimp_image_get_active_drawable (image);

      sensitive = gimp_procedure_get_sensitive (GIMP_PROCEDURE (proc),
                                                GIMP_OBJECT (drawable),
                                                &tooltip);

      gimp_action_group_set_action_sensitive (group,
                                              gimp_object_get_name (proc),
                                              sensitive);

      if (! sensitive && drawable && tooltip)
        gimp_action_group_set_action_tooltip (group,
                                              gimp_object_get_name (proc),
                                              tooltip);
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
      GimpAction *action;
      gchar      *label;

      label = p2 + 1;

#if 0
      g_print ("adding plug-in submenu '%s' (%s)\n",
               copy_original, label);
#endif

      action = gimp_action_impl_new (copy_original, label, NULL, NULL, NULL);
      gimp_action_group_add_action (group, action);
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
