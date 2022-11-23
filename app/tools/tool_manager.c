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

#include "libligmaconfig/ligmaconfig.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmatoolgroup.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"
#include "core/ligmatoolpreset.h"

#include "display/ligmadisplay.h"

#include "widgets/ligmacairo-wilber.h"

#include "ligmatool.h"
#include "ligmatoolcontrol.h"
#include "ligmatransformgridtool.h"
#include "tool_manager.h"


typedef struct _LigmaToolManager LigmaToolManager;

struct _LigmaToolManager
{
  Ligma          *ligma;

  LigmaTool      *active_tool;
  GSList        *tool_stack;

  LigmaToolGroup *active_tool_group;

  LigmaImage     *image;

  GQuark         image_clean_handler_id;
  GQuark         image_dirty_handler_id;
  GQuark         image_saving_handler_id;
};


/*  local function prototypes  */

static LigmaToolManager * tool_manager_get                       (Ligma            *ligma);

static void              tool_manager_select_tool               (LigmaToolManager *tool_manager,
                                                                 LigmaTool        *tool);

static void              tool_manager_set_active_tool_group     (LigmaToolManager *tool_manager,
                                                                 LigmaToolGroup   *tool_group);

static void              tool_manager_tool_changed              (LigmaContext     *user_context,
                                                                 LigmaToolInfo    *tool_info,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_preset_changed            (LigmaContext     *user_context,
                                                                 LigmaToolPreset  *preset,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_image_clean_dirty         (LigmaImage       *image,
                                                                 LigmaDirtyMask    dirty_mask,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_image_saving              (LigmaImage       *image,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_tool_ancestry_changed     (LigmaToolInfo    *tool_info,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_group_active_tool_changed (LigmaToolGroup   *tool_group,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_image_changed             (LigmaContext     *context,
                                                                 LigmaImage       *image,
                                                                 LigmaToolManager *tool_manager);
static void              tool_manager_selected_layers_changed   (LigmaImage       *image,
                                                                 LigmaToolManager *tool_manager);

static void              tool_manager_cast_spell                (LigmaToolInfo    *tool_info);


static GQuark tool_manager_quark = 0;


/*  public functions  */

void
tool_manager_init (Ligma *ligma)
{
  LigmaToolManager *tool_manager;
  LigmaContext     *user_context;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (tool_manager_quark == 0);

  tool_manager_quark = g_quark_from_static_string ("ligma-tool-manager");

  tool_manager = g_slice_new0 (LigmaToolManager);

  tool_manager->ligma = ligma;

  g_object_set_qdata (G_OBJECT (ligma), tool_manager_quark, tool_manager);

  tool_manager->image_clean_handler_id =
    ligma_container_add_handler (ligma->images, "clean",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_dirty_handler_id =
    ligma_container_add_handler (ligma->images, "dirty",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_saving_handler_id =
    ligma_container_add_handler (ligma->images, "saving",
                                G_CALLBACK (tool_manager_image_saving),
                                tool_manager);

  user_context = ligma_get_user_context (ligma);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_manager_tool_changed),
                    tool_manager);
  g_signal_connect (user_context, "tool-preset-changed",
                    G_CALLBACK (tool_manager_preset_changed),
                    tool_manager);
  g_signal_connect (user_context, "image-changed",
                    G_CALLBACK (tool_manager_image_changed),
                    tool_manager);

  tool_manager_image_changed (user_context,
                              ligma_context_get_image (user_context),
                              tool_manager);
  tool_manager_selected_layers_changed (ligma_context_get_image (user_context),
                                        tool_manager);

  tool_manager_tool_changed (user_context,
                             ligma_context_get_tool (user_context),
                             tool_manager);
}

void
tool_manager_exit (Ligma *ligma)
{
  LigmaToolManager *tool_manager;
  LigmaContext     *user_context;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  g_return_if_fail (tool_manager != NULL);

  user_context = ligma_get_user_context (ligma);

  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_tool_changed,
                                        tool_manager);
  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_preset_changed,
                                        tool_manager);
  g_signal_handlers_disconnect_by_func (user_context,
                                        tool_manager_image_changed,
                                        tool_manager);

  ligma_container_remove_handler (ligma->images,
                                 tool_manager->image_clean_handler_id);
  ligma_container_remove_handler (ligma->images,
                                 tool_manager->image_dirty_handler_id);
  ligma_container_remove_handler (ligma->images,
                                 tool_manager->image_saving_handler_id);

  if (tool_manager->active_tool)
    {
      g_signal_handlers_disconnect_by_func (
        tool_manager->active_tool->tool_info,
        tool_manager_tool_ancestry_changed,
        tool_manager);

      g_clear_object (&tool_manager->active_tool);
    }

  tool_manager_set_active_tool_group (tool_manager, NULL);

  g_slice_free (LigmaToolManager, tool_manager);

  g_object_set_qdata (G_OBJECT (ligma), tool_manager_quark, NULL);
}

LigmaTool *
tool_manager_get_active (Ligma *ligma)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  tool_manager = tool_manager_get (ligma);

  return tool_manager->active_tool;
}

void
tool_manager_push_tool (Ligma     *ligma,
                        LigmaTool *tool)
{
  LigmaToolManager *tool_manager;
  LigmaDisplay     *focus_display = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      focus_display = tool_manager->active_tool->focus_display;

      tool_manager->tool_stack = g_slist_prepend (tool_manager->tool_stack,
                                                  tool_manager->active_tool);

      g_object_ref (tool_manager->tool_stack->data);
    }

  tool_manager_select_tool (tool_manager, tool);

  if (focus_display)
    tool_manager_focus_display_active (ligma, focus_display);
}

void
tool_manager_pop_tool (Ligma *ligma)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->tool_stack)
    {
      LigmaTool *tool = tool_manager->tool_stack->data;

      tool_manager->tool_stack = g_slist_remove (tool_manager->tool_stack,
                                                 tool);

      tool_manager_select_tool (tool_manager, tool);

      g_object_unref (tool);
    }
}

gboolean
tool_manager_initialize_active (Ligma        *ligma,
                                LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      LigmaTool *tool = tool_manager->active_tool;

      if (ligma_tool_initialize (tool, display))
        {
          LigmaImage *image = ligma_display_get_image (display);

          g_list_free (tool->drawables);
          tool->drawables = ligma_image_get_selected_drawables (image);

          return TRUE;
        }
    }

  return FALSE;
}

void
tool_manager_control_active (Ligma           *ligma,
                             LigmaToolAction  action,
                             LigmaDisplay    *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      LigmaTool *tool = tool_manager->active_tool;

      if (display && ligma_tool_has_display (tool, display))
        {
          ligma_tool_control (tool, action, display);
        }
      else if (action == LIGMA_TOOL_ACTION_HALT)
        {
          if (ligma_tool_control_is_active (tool->control))
            ligma_tool_control_halt (tool->control);
        }
    }
}

void
tool_manager_button_press_active (Ligma                *ligma,
                                  const LigmaCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  LigmaButtonPressType  press_type,
                                  LigmaDisplay         *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      ligma_tool_button_press (tool_manager->active_tool,
                              coords, time, state, press_type,
                              display);
    }
}

void
tool_manager_button_release_active (Ligma             *ligma,
                                    const LigmaCoords *coords,
                                    guint32           time,
                                    GdkModifierType   state,
                                    LigmaDisplay      *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      ligma_tool_button_release (tool_manager->active_tool,
                                coords, time, state,
                                display);
    }
}

void
tool_manager_motion_active (Ligma             *ligma,
                            const LigmaCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            LigmaDisplay      *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      ligma_tool_motion (tool_manager->active_tool,
                        coords, time, state,
                        display);
    }
}

gboolean
tool_manager_key_press_active (Ligma        *ligma,
                               GdkEventKey *kevent,
                               LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_key_press (tool_manager->active_tool,
                                  kevent,
                                  display);
    }

  return FALSE;
}

gboolean
tool_manager_key_release_active (Ligma        *ligma,
                                 GdkEventKey *kevent,
                                 LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_key_release (tool_manager->active_tool,
                                    kevent,
                                    display);
    }

  return FALSE;
}

void
tool_manager_focus_display_active (Ligma        *ligma,
                                   LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool &&
      ! ligma_tool_control_is_active (tool_manager->active_tool->control))
    {
      ligma_tool_set_focus_display (tool_manager->active_tool,
                                   display);
    }
}

void
tool_manager_modifier_state_active (Ligma            *ligma,
                                    GdkModifierType  state,
                                    LigmaDisplay     *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool &&
      ! ligma_tool_control_is_active (tool_manager->active_tool->control))
    {
      ligma_tool_set_modifier_state (tool_manager->active_tool,
                                    state,
                                    display);
    }
}

void
tool_manager_active_modifier_state_active (Ligma            *ligma,
                                           GdkModifierType  state,
                                           LigmaDisplay     *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      ligma_tool_set_active_modifier_state (tool_manager->active_tool,
                                           state,
                                           display);
    }
}

void
tool_manager_oper_update_active (Ligma             *ligma,
                                 const LigmaCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 LigmaDisplay      *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool &&
      ! ligma_tool_control_is_active (tool_manager->active_tool->control))
    {
      ligma_tool_oper_update (tool_manager->active_tool,
                             coords, state, proximity,
                             display);
    }
}

void
tool_manager_cursor_update_active (Ligma             *ligma,
                                   const LigmaCoords *coords,
                                   GdkModifierType   state,
                                   LigmaDisplay      *display)
{
  LigmaToolManager *tool_manager;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool &&
      ! ligma_tool_control_is_active (tool_manager->active_tool->control))
    {
      ligma_tool_cursor_update (tool_manager->active_tool,
                               coords, state,
                               display);
    }
}

const gchar *
tool_manager_can_undo_active (Ligma        *ligma,
                              LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_can_undo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

const gchar *
tool_manager_can_redo_active (Ligma        *ligma,
                              LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_can_redo (tool_manager->active_tool,
                                 display);
    }

  return NULL;
}

gboolean
tool_manager_undo_active (Ligma        *ligma,
                          LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_undo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

gboolean
tool_manager_redo_active (Ligma        *ligma,
                          LigmaDisplay *display)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_redo (tool_manager->active_tool,
                             display);
    }

  return FALSE;
}

LigmaUIManager *
tool_manager_get_popup_active (Ligma             *ligma,
                               const LigmaCoords *coords,
                               GdkModifierType   state,
                               LigmaDisplay      *display,
                               const gchar     **ui_path)
{
  LigmaToolManager *tool_manager;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  tool_manager = tool_manager_get (ligma);

  if (tool_manager->active_tool)
    {
      return ligma_tool_get_popup (tool_manager->active_tool,
                                  coords, state,
                                  display,
                                  ui_path);
    }

  return NULL;
}


/*  private functions  */

static LigmaToolManager *
tool_manager_get (Ligma *ligma)
{
  return g_object_get_qdata (G_OBJECT (ligma), tool_manager_quark);
}

static void
tool_manager_select_tool (LigmaToolManager *tool_manager,
                          LigmaTool        *tool)
{
  Ligma *ligma = tool_manager->ligma;

  /*  reset the previously selected tool, but only if it is not only
   *  temporarily pushed to the tool stack
   */
  if (tool_manager->active_tool)
    {
      if (! tool_manager->tool_stack ||
          tool_manager->active_tool != tool_manager->tool_stack->data)
        {
          LigmaTool    *active_tool = tool_manager->active_tool;
          LigmaDisplay *display;

          /*  NULL image returns any display (if there is any)  */
          display = ligma_tool_has_image (active_tool, NULL);

          tool_manager_control_active (ligma, LIGMA_TOOL_ACTION_HALT, display);
          tool_manager_focus_display_active (ligma, NULL);
        }
    }

  g_set_object (&tool_manager->active_tool, tool);
}

static void
tool_manager_set_active_tool_group (LigmaToolManager *tool_manager,
                                    LigmaToolGroup   *tool_group)
{
  if (tool_group != tool_manager->active_tool_group)
    {
      if (tool_manager->active_tool_group)
        {
          g_signal_handlers_disconnect_by_func (
            tool_manager->active_tool_group,
            tool_manager_group_active_tool_changed,
            tool_manager);
        }

      g_set_weak_pointer (&tool_manager->active_tool_group, tool_group);

      if (tool_manager->active_tool_group)
        {
          g_signal_connect (
            tool_manager->active_tool_group, "active-tool-changed",
            G_CALLBACK (tool_manager_group_active_tool_changed),
            tool_manager);
        }
    }
}

static void
tool_manager_tool_changed (LigmaContext     *user_context,
                           LigmaToolInfo    *tool_info,
                           LigmaToolManager *tool_manager)
{
  LigmaTool *new_tool = NULL;

  if (! tool_info)
    return;

  if (! g_type_is_a (tool_info->tool_type, LIGMA_TYPE_TOOL))
    {
      g_warning ("%s: tool_info->tool_type is no LigmaTool subclass",
                 G_STRFUNC);
      return;
    }

  /* FIXME: ligma_busy HACK */
  if (user_context->ligma->busy)
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
          ligma_context_set_tool (user_context,
                                 tool_manager->active_tool->tool_info);

          g_signal_handlers_unblock_by_func (user_context,
                                             tool_manager_tool_changed,
                                             tool_manager);
        }

      return;
    }

  g_return_if_fail (tool_manager->tool_stack == NULL);

  if (tool_manager->active_tool)
    {
      LigmaTool    *active_tool = tool_manager->active_tool;
      LigmaDisplay *display;

      /*  NULL image returns any display (if there is any)  */
      display = ligma_tool_has_image (active_tool, NULL);

      /*  commit the old tool's operation before creating the new tool
       *  because creating a tool might mess with the old tool's
       *  options (old and new tool might be the same)
       */
      if (display)
        tool_manager_control_active (user_context->ligma, LIGMA_TOOL_ACTION_COMMIT,
                                     display);

      g_signal_handlers_disconnect_by_func (active_tool->tool_info,
                                            tool_manager_tool_ancestry_changed,
                                            tool_manager);
    }

  g_signal_connect (tool_info, "ancestry-changed",
                    G_CALLBACK (tool_manager_tool_ancestry_changed),
                    tool_manager);

  tool_manager_tool_ancestry_changed (tool_info, tool_manager);

  new_tool = g_object_new (tool_info->tool_type,
                           "tool-info", tool_info,
                           NULL);

  tool_manager_select_tool (tool_manager, new_tool);

  /* Auto-activate any transform tools */
  if (LIGMA_IS_TRANSFORM_GRID_TOOL (new_tool))
    {
      LigmaDisplay *new_display;

      new_display = ligma_context_get_display (user_context);
      if (new_display && ligma_display_get_image (new_display))
        tool_manager_initialize_active (user_context->ligma, new_display);
    }

  g_object_unref (new_tool);

  /* ??? */
  tool_manager_cast_spell (tool_info);
}

static void
tool_manager_copy_tool_options (GObject *src,
                                GObject *dest)
{
  GList *diff;

  diff = ligma_config_diff (src, dest, G_PARAM_READWRITE);

  if (diff)
    {
      GList *list;

      g_object_freeze_notify (dest);

      for (list = diff; list; list = list->next)
        {
          GParamSpec *prop_spec = list->data;

          if (g_type_is_a (prop_spec->owner_type, LIGMA_TYPE_TOOL_OPTIONS) &&
              ! (prop_spec->flags & G_PARAM_CONSTRUCT_ONLY))
            {
              GValue value = G_VALUE_INIT;

              g_value_init (&value, prop_spec->value_type);

              g_object_get_property (src,  prop_spec->name, &value);
              g_object_set_property (dest, prop_spec->name, &value);

              g_value_unset (&value);
            }
        }

      g_object_thaw_notify (dest);

      g_list_free (diff);
    }
}

static void
tool_manager_preset_changed (LigmaContext     *user_context,
                             LigmaToolPreset  *preset,
                             LigmaToolManager *tool_manager)
{
  LigmaToolInfo *preset_tool;

  if (! preset || user_context->ligma->busy)
    return;

  preset_tool = ligma_context_get_tool (LIGMA_CONTEXT (preset->tool_options));

  /* first, select the preset's tool, even if it's already the active
   * tool
   */
  ligma_context_set_tool (user_context, preset_tool);

  /* then, copy the context properties the preset remembers, possibly
   * changing some tool options due to the "link brush stuff to brush
   * defaults" settings in ligmatooloptions.c
   */
  ligma_context_copy_properties (LIGMA_CONTEXT (preset->tool_options),
                                user_context,
                                ligma_tool_preset_get_prop_mask (preset));

  /* finally, copy all tool options properties, overwriting any
   * changes resulting from setting the context properties above, we
   * really want exactly what is in the preset and nothing else
   */
  tool_manager_copy_tool_options (G_OBJECT (preset->tool_options),
                                  G_OBJECT (preset_tool->tool_options));
}

static void
tool_manager_image_clean_dirty (LigmaImage       *image,
                                LigmaDirtyMask    dirty_mask,
                                LigmaToolManager *tool_manager)
{
  LigmaTool *tool = tool_manager->active_tool;

  if (tool &&
      ! ligma_tool_control_get_preserve (tool->control) &&
      (ligma_tool_control_get_dirty_mask (tool->control) & dirty_mask))
    {
      LigmaDisplay *display = ligma_tool_has_image (tool, image);

      if (display)
        {
          tool_manager_control_active (
            image->ligma,
            ligma_tool_control_get_dirty_action (tool->control),
            display);
        }
    }
}

static void
tool_manager_image_saving (LigmaImage       *image,
                           LigmaToolManager *tool_manager)
{
  LigmaTool *tool = tool_manager->active_tool;

  if (tool &&
      ! ligma_tool_control_get_preserve (tool->control))
    {
      LigmaDisplay *display = ligma_tool_has_image (tool, image);

      if (display)
        tool_manager_control_active (image->ligma, LIGMA_TOOL_ACTION_COMMIT,
                                     display);
    }
}

static void
tool_manager_tool_ancestry_changed (LigmaToolInfo    *tool_info,
                                    LigmaToolManager *tool_manager)
{
  LigmaViewable *parent;

  parent = ligma_viewable_get_parent (LIGMA_VIEWABLE (tool_info));

  if (parent)
    {
      ligma_tool_group_set_active_tool_info (LIGMA_TOOL_GROUP (parent),
                                            tool_info);
    }

  tool_manager_set_active_tool_group (tool_manager, LIGMA_TOOL_GROUP (parent));
}

static void
tool_manager_group_active_tool_changed (LigmaToolGroup   *tool_group,
                                        LigmaToolManager *tool_manager)
{
  ligma_context_set_tool (tool_manager->ligma->user_context,
                         ligma_tool_group_get_active_tool_info (tool_group));
}

static void
tool_manager_image_changed (LigmaContext     *context,
                            LigmaImage       *image,
                            LigmaToolManager *tool_manager)
{
  if (tool_manager->image)
    {
      g_signal_handlers_disconnect_by_func (tool_manager->image,
                                            tool_manager_selected_layers_changed,
                                            tool_manager);
    }

  tool_manager->image = image;

  /* Re-activate transform tools when switching images */
  if (image &&
      LIGMA_IS_TRANSFORM_GRID_TOOL (tool_manager->active_tool))
    {
      ligma_context_set_tool (context,
                             tool_manager->active_tool->tool_info);

      tool_manager_tool_changed (context,
                                 ligma_context_get_tool (context),
                                 tool_manager);
    }

  if (image)
    g_signal_connect (tool_manager->image, "selected-layers-changed",
                      G_CALLBACK (tool_manager_selected_layers_changed),
                      tool_manager);
}

static void
tool_manager_selected_layers_changed (LigmaImage       *image,
                                      LigmaToolManager *tool_manager)
{
  /* Re-activate transform tools when changing selected layers */
  if (image &&
      LIGMA_IS_TRANSFORM_GRID_TOOL (tool_manager->active_tool))
    {
      ligma_context_set_tool (tool_manager->ligma->user_context,
                             tool_manager->active_tool->tool_info);

      tool_manager_tool_changed (tool_manager->ligma->user_context,
                                 ligma_context_get_tool (
                                   tool_manager->ligma->user_context),
                                 tool_manager);
    }
}

static void
tool_manager_cast_spell (LigmaToolInfo *tool_info)
{
  typedef struct
  {
    const gchar *sequence;
    GCallback    func;
  } Spell;

  static const Spell spells[] =
  {
    { .sequence = "ligma-warp-tool\0"
                  "ligma-iscissors-tool\0"
                  "ligma-gradient-tool\0"
                  "ligma-vector-tool\0"
                  "ligma-ellipse-select-tool\0"
                  "ligma-rect-select-tool\0",
      .func     = ligma_cairo_wilber_toggle_pointer_eyes
    }
  };

  static const gchar *spell_progress[G_N_ELEMENTS (spells)];
  const gchar        *tool_name;
  gint                i;

  tool_name = ligma_object_get_name (LIGMA_OBJECT (tool_info));

  for (i = 0; i < G_N_ELEMENTS (spells); i++)
    {
      if (! spell_progress[i])
        spell_progress[i] = spells[i].sequence;

      while (spell_progress[i])
        {
          if (! strcmp (tool_name, spell_progress[i]))
            {
              spell_progress[i] += strlen (spell_progress[i]) + 1;

              if (! *spell_progress[i])
                {
                  spell_progress[i] = NULL;

                  spells[i].func ();
                }

              break;
            }
          else
            {
              if (spell_progress[i] == spells[i].sequence)
                spell_progress[i] = NULL;
              else
                spell_progress[i] = spells[i].sequence;
            }
        }
    }
}
