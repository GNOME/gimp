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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimplist.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimptoolpreset.h"

#include "paint/gimppaintoptions.h"

#include "display/gimpdisplay.h"

#include "gimptool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"


typedef struct _GimpToolManager GimpToolManager;

struct _GimpToolManager
{
  GimpTool         *active_tool;
  GimpPaintOptions *shared_paint_options;
  GSList           *tool_stack;

  GQuark            image_clean_handler_id;
  GQuark            image_dirty_handler_id;
};


/*  local function prototypes  */

static GimpToolManager * tool_manager_get     (Gimp            *gimp);
static void              tool_manager_set     (Gimp            *gimp,
                                               GimpToolManager *tool_manager);
static void   tool_manager_select_tool        (Gimp            *gimp,
                                               GimpTool        *tool);
static void   tool_manager_tool_changed       (GimpContext     *user_context,
                                               GimpToolInfo    *tool_info,
                                               GimpToolManager *tool_manager);
static void   tool_manager_preset_changed     (GimpContext     *user_context,
                                               GimpToolPreset  *preset,
                                               GimpToolManager *tool_manager);
static void   tool_manager_image_clean_dirty  (GimpImage       *image,
                                               GimpDirtyMask    dirty_mask,
                                               GimpToolManager *tool_manager);

static void   tool_manager_connect_options    (GimpToolManager *tool_manager,
                                               GimpContext     *user_context,
                                               GimpToolInfo    *tool_info);
static void   tool_manager_disconnect_options (GimpToolManager *tool_manager,
                                               GimpContext     *user_context,
                                               GimpToolInfo    *tool_info);


/*  public functions  */

void
tool_manager_init (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = g_slice_new0 (GimpToolManager);

  tool_manager->active_tool            = NULL;
  tool_manager->tool_stack             = NULL;
  tool_manager->image_clean_handler_id = 0;
  tool_manager->image_dirty_handler_id = 0;

  tool_manager_set (gimp, tool_manager);

  tool_manager->image_clean_handler_id =
    gimp_container_add_handler (gimp->images, "clean",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_dirty_handler_id =
    gimp_container_add_handler (gimp->images, "dirty",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  user_context = gimp_get_user_context (gimp);

  tool_manager->shared_paint_options = g_object_new (GIMP_TYPE_PAINT_OPTIONS,
                                                     "gimp", gimp,
                                                     "name", "tool-manager-shared-paint-options",
                                                     NULL);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_manager_tool_changed),
                    tool_manager);
  g_signal_connect (user_context, "tool-preset-changed",
                    G_CALLBACK (tool_manager_preset_changed),
                    tool_manager);
}

void
tool_manager_exit (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);
  tool_manager_set (gimp, NULL);

  user_context = gimp_get_user_context (gimp);

  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_tool_changed,
                                        tool_manager);
  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_preset_changed,
                                        tool_manager);

  gimp_container_remove_handler (gimp->images,
                                 tool_manager->image_clean_handler_id);
  gimp_container_remove_handler (gimp->images,
                                 tool_manager->image_dirty_handler_id);

  g_clear_object (&tool_manager->active_tool);
  g_clear_object (&tool_manager->shared_paint_options);

  g_slice_free (GimpToolManager, tool_manager);
}

GimpTool *
tool_manager_get_active (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  return tool_manager->active_tool;
}

void
tool_manager_push_tool (Gimp     *gimp,
                        GimpTool *tool)
{
  GimpToolManager *tool_manager;
  GimpDisplay     *focus_display = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      focus_display = tool_manager->active_tool->focus_display;

      tool_manager->tool_stack = g_slist_prepend (tool_manager->tool_stack,
                                                  tool_manager->active_tool);

      g_object_ref (tool_manager->tool_stack->data);
    }

  tool_manager_select_tool (gimp, tool);

  if (focus_display)
    tool_manager_focus_display_active (gimp, focus_display);
}

void
tool_manager_pop_tool (Gimp *gimp)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->tool_stack)
    {
      GimpTool *tool = tool_manager->tool_stack->data;

      tool_manager->tool_stack = g_slist_remove (tool_manager->tool_stack,
                                                 tool);

      tool_manager_select_tool (gimp, tool);

      g_object_unref (tool);
    }
}

gboolean
tool_manager_initialize_active (Gimp        *gimp,
                                GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      GimpTool *tool = tool_manager->active_tool;

      if (gimp_tool_initialize (tool, display))
        {
          GimpImage *image = gimp_display_get_image (display);

          tool->drawable = gimp_image_get_active_drawable (image);

          return TRUE;
        }
    }

  return FALSE;
}

void
tool_manager_control_active (Gimp           *gimp,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      GimpTool *tool = tool_manager->active_tool;

      if (display && gimp_tool_has_display (tool, display))
        {
          gimp_tool_control (tool, action, display);
        }
      else if (action == GIMP_TOOL_ACTION_HALT)
        {
          if (gimp_tool_control_is_active (tool->control))
            gimp_tool_control_halt (tool->control);
        }
    }
}

void
tool_manager_button_press_active (Gimp                *gimp,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type,
                                  GimpDisplay         *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_button_press (tool_manager->active_tool,
                              coords, time, state, press_type,
                              display);
    }
}

void
tool_manager_button_release_active (Gimp             *gimp,
                                    const GimpCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    GimpDisplay      *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_button_release (tool_manager->active_tool,
                                coords, time, state,
                                display);
    }
}

void
tool_manager_motion_active (Gimp             *gimp,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            GimpDisplay      *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_motion (tool_manager->active_tool,
                        coords, time, state,
                        display);
    }
}

gboolean
tool_manager_key_press_active (Gimp        *gimp,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_key_press (tool_manager->active_tool,
                                  kevent,
                                  display);
    }

  return FALSE;
}

gboolean
tool_manager_key_release_active (Gimp        *gimp,
                                 GdkEventKey *kevent,
                                 GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_key_release (tool_manager->active_tool,
                                    kevent,
                                    display);
    }

  return FALSE;
}

void
tool_manager_focus_display_active (Gimp        *gimp,
                                   GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_set_focus_display (tool_manager->active_tool,
                                   display);
    }
}

void
tool_manager_modifier_state_active (Gimp            *gimp,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_set_modifier_state (tool_manager->active_tool,
                                    state,
                                    display);
    }
}

void
tool_manager_active_modifier_state_active (Gimp            *gimp,
                                           GdkModifierType  state,
                                           GimpDisplay     *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_set_active_modifier_state (tool_manager->active_tool,
                                           state,
                                           display);
    }
}

void
tool_manager_oper_update_active (Gimp             *gimp,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_oper_update (tool_manager->active_tool,
                             coords, state, proximity,
                             display);
    }
}

void
tool_manager_cursor_update_active (Gimp             *gimp,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      gimp_tool_cursor_update (tool_manager->active_tool,
                               coords, state,
                               display);
    }
}

const gchar *
tool_manager_can_undo_active (Gimp        *gimp,
                              GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_can_undo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

const gchar *
tool_manager_can_redo_active (Gimp        *gimp,
                              GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_can_redo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

gboolean
tool_manager_undo_active (Gimp        *gimp,
                          GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_undo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

gboolean
tool_manager_redo_active (Gimp        *gimp,
                          GimpDisplay *display)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_redo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

GimpUIManager *
tool_manager_get_popup_active (Gimp             *gimp,
                               const GimpCoords *coords,
                               GdkModifierType   state,
                               GimpDisplay      *display,
                               const gchar     **ui_path)
{
  GimpToolManager *tool_manager;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  tool_manager = tool_manager_get (gimp);

  if (tool_manager->active_tool)
    {
      return gimp_tool_get_popup (tool_manager->active_tool,
                                  coords, state,
                                  display,
                                  ui_path);
    }

  return NULL;
}


/*  private functions  */

static GQuark tool_manager_quark = 0;

static GimpToolManager *
tool_manager_get (Gimp *gimp)
{
  if (! tool_manager_quark)
    tool_manager_quark = g_quark_from_static_string ("gimp-tool-manager");

  return g_object_get_qdata (G_OBJECT (gimp), tool_manager_quark);
}

static void
tool_manager_set (Gimp            *gimp,
                  GimpToolManager *tool_manager)
{
  if (! tool_manager_quark)
    tool_manager_quark = g_quark_from_static_string ("gimp-tool-manager");

  g_object_set_qdata (G_OBJECT (gimp), tool_manager_quark, tool_manager);
}

static void
tool_manager_select_tool (Gimp     *gimp,
                          GimpTool *tool)
{
  GimpToolManager *tool_manager;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_TOOL (tool));

  tool_manager = tool_manager_get (gimp);

  /*  reset the previously selected tool, but only if it is not only
   *  temporarily pushed to the tool stack
   */
  if (tool_manager->active_tool)
    {
      if (! tool_manager->tool_stack ||
          tool_manager->active_tool != tool_manager->tool_stack->data)
        {
          GimpTool    *active_tool = tool_manager->active_tool;
          GimpDisplay *display;

          /*  NULL image returns any display (if there is any)  */
          display = gimp_tool_has_image (active_tool, NULL);

          tool_manager_control_active (gimp, GIMP_TOOL_ACTION_HALT, display);
          tool_manager_focus_display_active (gimp, NULL);
        }

      g_object_unref (tool_manager->active_tool);
    }

  tool_manager->active_tool = g_object_ref (tool);
}

static void
tool_manager_tool_changed (GimpContext     *user_context,
                           GimpToolInfo    *tool_info,
                           GimpToolManager *tool_manager)
{
  GimpTool *new_tool = NULL;

  if (! tool_info)
    return;

  if (! g_type_is_a (tool_info->tool_type, GIMP_TYPE_TOOL))
    {
      g_warning ("%s: tool_info->tool_type is no GimpTool subclass",
                 G_STRFUNC);
      return;
    }

  /* FIXME: gimp_busy HACK */
  if (user_context->gimp->busy)
    {
      /*  there may be contexts waiting for the user_context's "tool-changed"
       *  signal, so stop emitting it.
       */
      g_signal_stop_emission_by_name (user_context, "tool-changed");

      if (G_TYPE_FROM_INSTANCE (tool_manager->active_tool) !=
          tool_info->tool_type)
        {
          g_signal_handlers_block_by_func (user_context,
                                           tool_manager_tool_changed,
                                           tool_manager);

          /*  explicitly set the current tool  */
          gimp_context_set_tool (user_context,
                                 tool_manager->active_tool->tool_info);

          g_signal_handlers_unblock_by_func (user_context,
                                             tool_manager_tool_changed,
                                             tool_manager);
        }

      return;
    }

  if (tool_manager->active_tool)
    {
      GimpTool    *active_tool = tool_manager->active_tool;
      GimpDisplay *display;

      /*  NULL image returns any display (if there is any)  */
      display = gimp_tool_has_image (active_tool, NULL);

      /*  commit the old tool's operation before creating the new tool
       *  because creating a tool might mess with the old tool's
       *  options (old and new tool might be the same)
       */
      if (display)
        tool_manager_control_active (user_context->gimp, GIMP_TOOL_ACTION_COMMIT,
                                     display);

      /*  disconnect the old tool's context  */
      if (active_tool->tool_info)
        tool_manager_disconnect_options (tool_manager, user_context,
                                         active_tool->tool_info);
    }

  new_tool = g_object_new (tool_info->tool_type,
                           "tool-info", tool_info,
                           NULL);

  /*  connect the new tool's context  */
  tool_manager_connect_options (tool_manager, user_context, tool_info);

  tool_manager_select_tool (user_context->gimp, new_tool);

  g_object_unref (new_tool);
}

static void
tool_manager_preset_changed (GimpContext     *user_context,
                             GimpToolPreset  *preset,
                             GimpToolManager *tool_manager)
{
  GimpToolInfo *preset_tool;
  gchar        *options_name;
  gboolean      tool_change = FALSE;

  if (! preset || user_context->gimp->busy)
    return;

  preset_tool = gimp_context_get_tool (GIMP_CONTEXT (preset->tool_options));

  if (preset_tool != gimp_context_get_tool (user_context))
    tool_change = TRUE;

  if (! tool_change)
    tool_manager_disconnect_options (tool_manager, user_context, preset_tool);

  /*  save the name, we don't want to overwrite it  */
  options_name = g_strdup (gimp_object_get_name (preset_tool->tool_options));

  gimp_config_copy (GIMP_CONFIG (preset->tool_options),
                    GIMP_CONFIG (preset_tool->tool_options), 0);

  /*  restore the saved name  */
  gimp_object_take_name (GIMP_OBJECT (preset_tool->tool_options), options_name);

  if (tool_change)
    gimp_context_set_tool (user_context, preset_tool);
  else
    tool_manager_connect_options (tool_manager, user_context, preset_tool);

  gimp_context_copy_properties (GIMP_CONTEXT (preset->tool_options),
                                user_context,
                                gimp_tool_preset_get_prop_mask (preset));

  if (GIMP_IS_PAINT_OPTIONS (preset->tool_options))
    {
      GimpToolOptions *src  = preset->tool_options;
      GimpToolOptions *dest = tool_manager->active_tool->tool_info->tool_options;

      /*  copy various data objects' additional tool options again
       *  manually, they might have been overwritten by e.g. the "link
       *  brush stuff to brush defaults" logic in
       *  gimptooloptions-gui.c
       */
      if (preset->use_brush)
        gimp_paint_options_copy_brush_props (GIMP_PAINT_OPTIONS (src),
                                             GIMP_PAINT_OPTIONS (dest));

      if (preset->use_dynamics)
        gimp_paint_options_copy_dynamics_props (GIMP_PAINT_OPTIONS (src),
                                                GIMP_PAINT_OPTIONS (dest));

      if (preset->use_gradient)
        gimp_paint_options_copy_gradient_props (GIMP_PAINT_OPTIONS (src),
                                                GIMP_PAINT_OPTIONS (dest));
    }
}

static void
tool_manager_image_clean_dirty (GimpImage       *image,
                                GimpDirtyMask    dirty_mask,
                                GimpToolManager *tool_manager)
{
  GimpTool *tool = tool_manager->active_tool;

  if (tool &&
      ! gimp_tool_control_get_preserve (tool->control) &&
      (gimp_tool_control_get_dirty_mask (tool->control) & dirty_mask))
    {
      GimpDisplay *display = gimp_tool_has_image (tool, image);

      if (display)
        tool_manager_control_active (image->gimp, GIMP_TOOL_ACTION_HALT,
                                     display);
    }
}

static void
tool_manager_connect_options (GimpToolManager *tool_manager,
                              GimpContext     *user_context,
                              GimpToolInfo    *tool_info)
{
  if (tool_info->context_props)
    {
      GimpCoreConfig      *config       = user_context->gimp->config;
      GimpContextPropMask  global_props = 0;

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

      gimp_context_copy_properties (GIMP_CONTEXT (tool_info->tool_options),
                                    user_context,
                                    tool_info->context_props & ~global_props);
      gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options),
                               user_context);

      if (GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          if (config->global_brush)
            gimp_paint_options_copy_brush_props (tool_manager->shared_paint_options,
                                                 GIMP_PAINT_OPTIONS (tool_info->tool_options));

          if (config->global_dynamics)
            gimp_paint_options_copy_dynamics_props (tool_manager->shared_paint_options,
                                                    GIMP_PAINT_OPTIONS (tool_info->tool_options));

          if (config->global_gradient)
            gimp_paint_options_copy_gradient_props (tool_manager->shared_paint_options,
                                                    GIMP_PAINT_OPTIONS (tool_info->tool_options));
        }
    }
}

static void
tool_manager_disconnect_options (GimpToolManager *tool_manager,
                                 GimpContext     *user_context,
                                 GimpToolInfo    *tool_info)
{
  if (tool_info->context_props)
    {
      if (GIMP_IS_PAINT_OPTIONS (tool_info->tool_options))
        {
          /* Storing is unconditional, because the user may turn on
           * brush sharing mid use
           */
          gimp_paint_options_copy_brush_props (GIMP_PAINT_OPTIONS (tool_info->tool_options),
                                               tool_manager->shared_paint_options);

          gimp_paint_options_copy_dynamics_props (GIMP_PAINT_OPTIONS (tool_info->tool_options),
                                                  tool_manager->shared_paint_options);

          gimp_paint_options_copy_gradient_props (GIMP_PAINT_OPTIONS (tool_info->tool_options),
                                                  tool_manager->shared_paint_options);
        }

      gimp_context_set_parent (GIMP_CONTEXT (tool_info->tool_options), NULL);
    }
}
