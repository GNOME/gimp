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
#include "core/gimpcontext.h"
#include "core/gimpimage.h"

#include "pdb/procedural_db.h"

#include "plug-in/plug-ins.h"
#include "plug-in/plug-in-proc.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "plug-in-actions.h"
#include "plug-in-commands.h"

#include "gimp-intl.h"


typedef struct _PlugInActionEntry PlugInActionEntry;

struct _PlugInActionEntry
{
  PlugInProcDef *proc_def;
  const gchar   *locale_domain;
  const gchar   *help_domain;
};


/*  local function prototypes  */

static gboolean plug_in_actions_tree_traverse_func (gpointer           foo,
                                                    PlugInActionEntry *entry,
                                                    GimpActionGroup   *group);


static GimpActionEntry plug_in_actions[] =
{
  { "plug-in-menu",                NULL, N_("Filte_rs")      },
  { "plug-in-blur-menu",           NULL, N_("_Blur")          },
  { "plug-in-colors-menu",         NULL, N_("_Colors")        },
  { "plug-in-colors-map-menu",     NULL, N_("Ma_p")           },
  { "plug-in-noise-menu",          NULL, N_("_Noise")         },
  { "plug-in-edge-detect-menu",    NULL, N_("Edge-De_tect")   },
  { "plug-in-enhance-menu",        NULL, N_("En_hance")       },
  { "plug-in-generic-menu",        NULL, N_("_Generic")       },
  { "plug-in-glass-effects-menu",  NULL, N_("Gla_ss Effects") },
  { "plug-in-light-effects-menu",  NULL, N_("_Light Effects") },
  { "plug-in-distorts-menu",       NULL, N_("_Distorts")      },
  { "plug-in-artistic-menu",       NULL, N_("_Artistic")      },
  { "plug-in-map-menu",            NULL, N_("_Map")           },
  { "plug-in-render-menu",         NULL, N_("_Render")        },
  { "plug-in-render-clouds-menu",  NULL, N_("_Clouds")        },
  { "plug-in-render-nature-menu",  NULL, N_("_Nature")        },
  { "plug-in-render-pattern-menu", NULL, N_("_Pattern")       },
  { "plug-in-web-menu",            NULL, N_("_Web")           },
  { "plug-in-animation-menu",      NULL, N_("An_imation")     },
  { "plug-in-combine-menu",        NULL, N_("C_ombine")       },
  { "plug-in-toys-menu",           NULL, N_("To_ys")          }
};

static GimpEnumActionEntry plug_in_repeat_actions[] =
{
  { "plug-in-repeat", GTK_STOCK_EXECUTE,
    N_("Repeat Last"), "<control>F", NULL,
    FALSE,
    GIMP_HELP_FILTER_REPEAT },

  { "plug-in-reshow", GIMP_STOCK_RESHOW_FILTER,
    N_("Re-Show Last"), "<control><shift>F", NULL,
    TRUE,
    GIMP_HELP_FILTER_RESHOW }
};


/*  public functions  */

void
plug_in_actions_setup (GimpActionGroup *group,
                       gpointer         data)
{
  GSList *procs;
  GTree  *action_entries;

  gimp_action_group_add_actions (group,
                                 plug_in_actions,
                                 G_N_ELEMENTS (plug_in_actions),
                                 data);

  gimp_action_group_add_enum_actions (group,
                                      plug_in_repeat_actions,
                                      G_N_ELEMENTS (plug_in_repeat_actions),
                                      G_CALLBACK (plug_in_repeat_cmd_callback),
                                      data);

  action_entries = g_tree_new_full ((GCompareDataFunc) g_utf8_collate, NULL,
                                    g_free, g_free);

  for (procs = group->gimp->plug_in_proc_defs;
       procs;
       procs = procs->next)
    {
      PlugInProcDef *proc_def = procs->data;

      if (proc_def->prog         &&
          proc_def->menu_path    &&
          ! proc_def->extensions &&
          ! proc_def->prefixes   &&
          ! proc_def->magics)
        {
          PlugInActionEntry *entry;
          const gchar       *progname;
          const gchar       *locale_domain;
          const gchar       *help_domain;
          gchar             *key;

          progname = plug_in_proc_def_get_progname (proc_def);

          locale_domain = plug_ins_locale_domain (group->gimp,
                                                  progname, NULL);
          help_domain = plug_ins_help_domain (group->gimp,
                                              progname, NULL);

          entry = g_new0 (PlugInActionEntry, 1);

          entry->proc_def      = proc_def;
          entry->locale_domain = locale_domain;
          entry->help_domain   = help_domain;

          key = gimp_strip_uline (dgettext (locale_domain,
                                            proc_def->menu_path));
          g_tree_insert (action_entries, key, entry);
        }
    }

  g_tree_foreach (action_entries,
                  (GTraverseFunc) plug_in_actions_tree_traverse_func,
                  group);
  g_tree_destroy (action_entries);
}

void
plug_in_actions_update (GimpActionGroup *group,
                        gpointer         data)
{
}

void
plug_in_actions_add_proc (GimpActionGroup *group,
                          PlugInProcDef   *proc_def,
                          const gchar     *locale_domain,
                          const gchar     *help_domain)
{
  GimpActionEntry  entry;
  gchar           *help_id;

  g_return_if_fail (GIMP_IS_ACTION_GROUP (group));
  g_return_if_fail (proc_def != NULL);

  help_id = plug_in_proc_def_get_help_id (proc_def, help_domain);

  entry.name        = proc_def->db_info.name;
  entry.stock_id    = NULL;
  entry.label       = strrchr (proc_def->menu_path, '/') + 1;
  entry.accelerator = proc_def->accelerator;
  entry.tooltip     = NULL;
  entry.callback    = G_CALLBACK (plug_in_run_cmd_callback);
  entry.help_id     = help_id;

  gimp_action_group_add_actions (group, &entry, 1, &proc_def->db_info);

  g_free (help_id);
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
    gtk_action_group_remove_action (GTK_ACTION_GROUP (group), action);
}


/*  private functions  */

static gboolean
plug_in_actions_tree_traverse_func (gpointer           foo,
                                    PlugInActionEntry *entry,
                                    GimpActionGroup   *group)
{
  plug_in_actions_add_proc (group,
                            entry->proc_def,
                            entry->locale_domain,
                            entry->help_domain);

  return FALSE;
}
