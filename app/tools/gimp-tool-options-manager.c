/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-tool-options-manager.c
 * Copyright (C) 2018 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimp-tool-options-manager.h"


typedef struct _GimpToolOptionsManager GimpToolOptionsManager;

struct _GimpToolOptionsManager
{
  Gimp                *gimp;
  GimpPaintOptions    *global_paint_options;
  GimpContextPropMask  global_props;

  GimpToolInfo        *active_tool;
};


/*  local function prototypes  */

static GimpContextPropMask
              tool_options_manager_get_global_props
                                                 (GimpCoreConfig         *config);

static void   tool_options_manager_global_notify (GimpCoreConfig         *config,
                                                  const GParamSpec       *pspec,
                                                  GimpToolOptionsManager *manager);
static void   tool_options_manager_paint_options_notify
                                                 (GimpPaintOptions       *src,
                                                  const GParamSpec       *pspec,
                                                  GimpPaintOptions       *dest);

static void   tool_options_manager_copy_paint_props
                                                 (GimpPaintOptions       *src,
                                                  GimpPaintOptions       *dest,
                                                  GimpContextPropMask     prop_mask);

static void   tool_options_manager_tool_changed  (GimpContext            *user_context,
                                                  GimpToolInfo           *tool_info,
                                                  GimpToolOptionsManager *manager);


static GQuark manager_quark = 0;


/*  public functions  */

void
gimp_tool_options_manager_init (Gimp *gimp)
{
  GimpToolOptionsManager *manager;
  GimpContext            *user_context;
  GList                  *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (manager_quark == 0);

  manager_quark = g_quark_from_static_string ("gimp-tool-options-manager");

  manager = g_slice_new0 (GimpToolOptionsManager);

  g_object_set_qdata (G_OBJECT (gimp), manager_quark, manager);

  manager->gimp = gimp;

  manager->global_paint_options =
    g_object_new (GIMP_TYPE_PAINT_OPTIONS,
                  "gimp", gimp,
                  "name", "tool-options-manager-global-paint-options",
                  NULL);

  manager->global_props = tool_options_manager_get_global_props (gimp->config);

  user_context = gimp_get_user_context (gimp);

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      /*  the global props that are actually used by the tool are
       *  always shared with the user context by undefining them...
       */
      gimp_context_define_properties (GIMP_CONTEXT (tool_info->tool_options),
                                      manager->global_props &
                                      tool_info->context_props,
                                      FALSE);

      /*  ...and setting the user context as parent
       */
      gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options),
                               user_context);

      /*  make sure paint tools also share their brush, dynamics,
       *  gradient properties if the resp. context properties are
       *  global
       */
      if (GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          g_signal_connect (tool_info->tool_options, "notify",
                            G_CALLBACK (tool_options_manager_paint_options_notify),
                            manager->global_paint_options);

          g_signal_connect (manager->global_paint_options, "notify",
                            G_CALLBACK (tool_options_manager_paint_options_notify),
                            tool_info->tool_options);

          tool_options_manager_copy_paint_props (manager->global_paint_options,
                                                 GIMP_PAINT_OPTIONS (tool_info->tool_options),
                                                 tool_info->context_props &
                                                 manager->global_props);
        }
    }

  g_signal_connect (gimp->config, "notify::global-brush",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-dynamics",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-pattern",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-palette",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-gradient",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-font",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);
  g_signal_connect (gimp->config, "notify::global-expand",
                    G_CALLBACK (tool_options_manager_global_notify),
                    manager);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_options_manager_tool_changed),
                    manager);

  tool_options_manager_tool_changed (user_context,
                                     gimp_context_get_tool (user_context),
                                     manager);
}

void
gimp_tool_options_manager_exit (Gimp *gimp)
{
  GimpToolOptionsManager *manager;
  GimpContext            *user_context;
  GList                  *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  manager = g_object_get_qdata (G_OBJECT (gimp), manager_quark);

  g_return_if_fail (manager != NULL);

  user_context = gimp_get_user_context (gimp);

  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_options_manager_tool_changed,
                                        manager);

  g_signal_handlers_disconnect_by_func (gimp->config,
                                        tool_options_manager_global_notify,
                                        manager);

  for (list = gimp_get_tool_info_iter (gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options), NULL);

      if (GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          g_signal_handlers_disconnect_by_func (tool_info->tool_options,
                                                tool_options_manager_paint_options_notify,
                                                manager->global_paint_options);

          g_signal_handlers_disconnect_by_func (manager->global_paint_options,
                                                tool_options_manager_paint_options_notify,
                                                tool_info->tool_options);
        }
    }

  g_clear_object (&manager->global_paint_options);

  g_slice_free (GimpToolOptionsManager, manager);

  g_object_set_qdata (G_OBJECT (gimp), manager_quark, NULL);
}


/*  private functions  */

static GimpContextPropMask
tool_options_manager_get_global_props (GimpCoreConfig *config)
{
  GimpContextPropMask global_props = 0;

  /*  FG and BG are always shared between all tools  */
  global_props |= GIMP_CONTEXT_PROP_MASK_FOREGROUND;
  global_props |= GIMP_CONTEXT_PROP_MASK_BACKGROUND;

  if (config->global_brush)
    global_props |= GIMP_CONTEXT_PROP_MASK_BRUSH;
  if (config->global_dynamics)
    global_props |= GIMP_CONTEXT_PROP_MASK_DYNAMICS;
  if (config->global_pattern)
    global_props |= GIMP_CONTEXT_PROP_MASK_PATTERN;
  if (config->global_palette)
    global_props |= GIMP_CONTEXT_PROP_MASK_PALETTE;
  if (config->global_gradient)
    global_props |= GIMP_CONTEXT_PROP_MASK_GRADIENT;
  if (config->global_font)
    global_props |= GIMP_CONTEXT_PROP_MASK_FONT;
  if (config->global_expand)
    global_props |= GIMP_CONTEXT_PROP_MASK_EXPAND;

  return global_props;
}

static void
tool_options_manager_global_notify (GimpCoreConfig         *config,
                                    const GParamSpec       *pspec,
                                    GimpToolOptionsManager *manager)
{
  GimpContextPropMask  global_props;
  GimpContextPropMask  enabled_global_props;
  GimpContextPropMask  disabled_global_props;
  GList               *list;

  global_props = tool_options_manager_get_global_props (config);

  enabled_global_props  = global_props & ~manager->global_props;
  disabled_global_props = manager->global_props & ~global_props;

  /*  copy the newly enabled global props to all tool options, and
   *  disconnect the newly disabled ones from the user context
   */
  for (list = gimp_get_tool_info_iter (manager->gimp);
       list;
       list = g_list_next (list))
    {
      GimpToolInfo *tool_info = list->data;

      /*  don't change the active tool, it is always fully connected
       *  to the user_context anyway because we set its
       *  defined/undefined context props in tool_changed()
       */
      if (tool_info == manager->active_tool)
        continue;

      /*  defining the newly disabled ones disconnects them from the
       *  parent user context
       */
      gimp_context_define_properties (GIMP_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props &
                                      disabled_global_props,
                                      TRUE);

      /*  undefining the newly enabled ones copies the value from the
       *  parent user context
       */
      gimp_context_define_properties (GIMP_CONTEXT (tool_info->tool_options),
                                      tool_info->context_props &
                                      enabled_global_props,
                                      FALSE);

      if (GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
        tool_options_manager_copy_paint_props (manager->global_paint_options,
                                               GIMP_PAINT_OPTIONS (tool_info->tool_options),
                                               tool_info->context_props &
                                               enabled_global_props);
    }

  manager->global_props = global_props;
}

static void
tool_options_manager_paint_options_notify (GimpPaintOptions *src,
                                           const GParamSpec *pspec,
                                           GimpPaintOptions *dest)
{
  Gimp                   *gimp   = GIMP_CONTEXT (src)->gimp;
  GimpCoreConfig         *config = gimp->config;
  GimpToolOptionsManager *manager;
  GimpToolInfo           *tool_info;
  GimpContextPropMask     prop_mask = 0;
  gboolean                active    = FALSE;

  manager = g_object_get_qdata (G_OBJECT (gimp), manager_quark);

  /*  one of the options is the global one, the other is the tool's,
   *  get the tool_info from the tool's options
   */
  if (manager->global_paint_options == src)
    tool_info = gimp_context_get_tool (GIMP_CONTEXT (dest));
  else
    tool_info = gimp_context_get_tool (GIMP_CONTEXT (src));

  if (tool_info == manager->active_tool)
    active = TRUE;

  if ((active || config->global_brush) &&
      tool_info->context_props & GIMP_CONTEXT_PROP_MASK_BRUSH)
    {
      prop_mask |= GIMP_CONTEXT_PROP_MASK_BRUSH;
    }

  if ((active || config->global_dynamics) &&
      tool_info->context_props & GIMP_CONTEXT_PROP_MASK_DYNAMICS)
    {
      prop_mask |= GIMP_CONTEXT_PROP_MASK_DYNAMICS;
    }

  if ((active || config->global_gradient) &&
      tool_info->context_props & GIMP_CONTEXT_PROP_MASK_GRADIENT)
    {
      prop_mask |= GIMP_CONTEXT_PROP_MASK_GRADIENT;
    }

  if ((active || config->global_expand) &&
      tool_info->context_props & GIMP_CONTEXT_PROP_MASK_EXPAND)
    {
      prop_mask |= GIMP_CONTEXT_PROP_MASK_EXPAND;
    }

  if (gimp_paint_options_is_prop (pspec->name, prop_mask))
    {
      GValue value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);

      g_object_get_property (G_OBJECT (src), pspec->name, &value);

      g_signal_handlers_block_by_func (dest,
                                       tool_options_manager_paint_options_notify,
                                       src);

      g_object_set_property (G_OBJECT (dest), pspec->name, &value);

      g_signal_handlers_unblock_by_func (dest,
                                         tool_options_manager_paint_options_notify,
                                         src);

      g_value_unset (&value);
    }
}

static void
tool_options_manager_copy_paint_props (GimpPaintOptions    *src,
                                       GimpPaintOptions    *dest,
                                       GimpContextPropMask  prop_mask)
{
  g_signal_handlers_block_by_func (dest,
                                   tool_options_manager_paint_options_notify,
                                   src);

  gimp_paint_options_copy_props (src, dest, prop_mask);

  g_signal_handlers_unblock_by_func (dest,
                                     tool_options_manager_paint_options_notify,
                                     src);
}

static void
tool_options_manager_tool_changed (GimpContext            *user_context,
                                   GimpToolInfo           *tool_info,
                                   GimpToolOptionsManager *manager)
{
  if (tool_info == manager->active_tool)
    return;

  /*  FIXME: gimp_busy HACK
   *  the tool manager will stop the emission, so simply return
   */
  if (manager->gimp->busy)
    return;

  if (manager->active_tool)
    {
      GimpToolInfo *active = manager->active_tool;

      /*  disconnect the old active tool from all context properties
       *  it uses, but are not currently global
       */
      gimp_context_define_properties (GIMP_CONTEXT (active->tool_options),
                                      active->context_props &
                                      ~manager->global_props,
                                      TRUE);
    }

  manager->active_tool = tool_info;

  if (manager->active_tool)
    {
      GimpToolInfo *active = manager->active_tool;

      /*  make sure the tool options GUI always exists, this call
       *  creates it if needed, so tools always have their option GUI
       *  available even if the tool options dockable is not open, see
       *  for example issue #3435
       */
      gimp_tools_get_tool_options_gui (active->tool_options);

      /*  copy the new tool's context properties that are not
       *  currently global to the user context, so they get used by
       *  everything
       */
      gimp_context_copy_properties (GIMP_CONTEXT (active->tool_options),
                                    gimp_get_user_context (manager->gimp),
                                    active->context_props &
                                    ~manager->global_props);

      if (GIMP_IS_PAINT_OPTIONS (active->tool_options))
        tool_options_manager_copy_paint_props (GIMP_PAINT_OPTIONS (active->tool_options),
                                               manager->global_paint_options,
                                               active->context_props &
                                               ~manager->global_props);

      /*  then, undefine these properties so the tool syncs with the
       *  user context automatically
       */
      gimp_context_define_properties (GIMP_CONTEXT (active->tool_options),
                                      active->context_props &
                                      ~manager->global_props,
                                      FALSE);
    }
}
