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

#include "libgimpconfig/gimpconfig.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"
#include "core/gimptoolpreset.h"

#include "display/gimpdisplay.h"

#include "widgets/gimpcairo-wilber.h"

#include "gimptool.h"
#include "gimptoolcontrol.h"
#include "tool_manager.h"


typedef struct _GimpToolManager GimpToolManager;

struct _GimpToolManager
{
  GimpTool *active_tool;
  GSList   *tool_stack;

  GQuark    image_clean_handler_id;
  GQuark    image_dirty_handler_id;
  GQuark    image_saving_handler_id;
};


/*  local function prototypes  */

static GimpToolManager * tool_manager_get     (Gimp            *gimp);

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
static void   tool_manager_image_saving       (GimpImage       *image,
                                               GimpToolManager *tool_manager);

static void   tool_manager_cast_spell         (GimpToolInfo    *tool_info);


static GQuark tool_manager_quark = 0;


/*  public functions  */

void
tool_manager_init (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (tool_manager_quark == 0);

  tool_manager_quark = g_quark_from_static_string ("gimp-tool-manager");

  tool_manager = g_slice_new0 (GimpToolManager);

  g_object_set_qdata (G_OBJECT (gimp), tool_manager_quark, tool_manager);

  tool_manager->image_clean_handler_id =
    gimp_container_add_handler (gimp->images, "clean",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_dirty_handler_id =
    gimp_container_add_handler (gimp->images, "dirty",
                                G_CALLBACK (tool_manager_image_clean_dirty),
                                tool_manager);

  tool_manager->image_saving_handler_id =
    gimp_container_add_handler (gimp->images, "saving",
                                G_CALLBACK (tool_manager_image_saving),
                                tool_manager);

  user_context = gimp_get_user_context (gimp);

  g_signal_connect (user_context, "tool-changed",
                    G_CALLBACK (tool_manager_tool_changed),
                    tool_manager);
  g_signal_connect (user_context, "tool-preset-changed",
                    G_CALLBACK (tool_manager_preset_changed),
                    tool_manager);

  tool_manager_tool_changed (user_context,
                             gimp_context_get_tool (user_context),
                             tool_manager);
}

void
tool_manager_exit (Gimp *gimp)
{
  GimpToolManager *tool_manager;
  GimpContext     *user_context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  tool_manager = tool_manager_get (gimp);

  g_return_if_fail (tool_manager != NULL);

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
  gimp_container_remove_handler (gimp->images,
                                 tool_manager->image_saving_handler_id);

  g_clear_object (&tool_manager->active_tool);

  g_slice_free (GimpToolManager, tool_manager);

  g_object_set_qdata (G_OBJECT (gimp), tool_manager_quark, NULL);
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

  if (tool_manager->active_tool &&
      ! gimp_tool_control_is_active (tool_manager->active_tool->control))
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

  if (tool_manager->active_tool &&
      ! gimp_tool_control_is_active (tool_manager->active_tool->control))
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

  if (tool_manager->active_tool &&
      ! gimp_tool_control_is_active (tool_manager->active_tool->control))
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

  if (tool_manager->active_tool &&
      ! gimp_tool_control_is_active (tool_manager->active_tool->control))
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

static GimpToolManager *
tool_manager_get (Gimp *gimp)
{
  return g_object_get_qdata (G_OBJECT (gimp), tool_manager_quark);
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
    }

  g_set_object (&tool_manager->active_tool, tool);
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
    }

  new_tool = g_object_new (tool_info->tool_type,
                           "tool-info", tool_info,
                           NULL);

  tool_manager_select_tool (user_context->gimp, new_tool);

  g_object_unref (new_tool);

  /* ??? */
  tool_manager_cast_spell (tool_info);
}

static void
tool_manager_copy_tool_options (GObject *src,
                                GObject *dest)
{
  GList *diff;

  diff = gimp_config_diff (src, dest, G_PARAM_READWRITE);

  if (diff)
    {
      GList *list;

      g_object_freeze_notify (dest);

      for (list = diff; list; list = list->next)
        {
          GParamSpec *prop_spec = list->data;

          if (g_type_is_a (prop_spec->owner_type, GIMP_TYPE_TOOL_OPTIONS) &&
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
tool_manager_preset_changed (GimpContext     *user_context,
                             GimpToolPreset  *preset,
                             GimpToolManager *tool_manager)
{
  GimpToolInfo *preset_tool;

  if (! preset || user_context->gimp->busy)
    return;

  preset_tool = gimp_context_get_tool (GIMP_CONTEXT (preset->tool_options));

  /* first, select the preset's tool, even if it's already the active
   * tool
   */
  gimp_context_set_tool (user_context, preset_tool);

  /* then, copy the context properties the preset remembers, possibly
   * changing some tool options due to the "link brush stuff to brush
   * defaults" settings in gimptooloptions.c
   */
  gimp_context_copy_properties (GIMP_CONTEXT (preset->tool_options),
                                user_context,
                                gimp_tool_preset_get_prop_mask (preset));

  /* finally, copy all tool options properties, overwriting any
   * changes resulting from setting the context properties above, we
   * really want exactly what is in the preset and nothing else
   */
  tool_manager_copy_tool_options (G_OBJECT (preset->tool_options),
                                  G_OBJECT (preset_tool->tool_options));
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
        {
          tool_manager_control_active (
            image->gimp,
            gimp_tool_control_get_dirty_action (tool->control),
            display);
        }
    }
}

static void
tool_manager_image_saving (GimpImage       *image,
                           GimpToolManager *tool_manager)
{
  GimpTool *tool = tool_manager->active_tool;

  if (tool &&
      ! gimp_tool_control_get_preserve (tool->control))
    {
      GimpDisplay *display = gimp_tool_has_image (tool, image);

      if (display)
        tool_manager_control_active (image->gimp, GIMP_TOOL_ACTION_COMMIT,
                                     display);
    }
}

static void
tool_manager_cast_spell (GimpToolInfo *tool_info)
{
  typedef struct
  {
    const gchar *sequence;
    GCallback    func;
  } Spell;

  static const Spell spells[] =
  {
    { .sequence = "gimp-warp-tool\0"
                  "gimp-iscissors-tool\0"
                  "gimp-gradient-tool\0"
                  "gimp-vector-tool\0"
                  "gimp-ellipse-select-tool\0"
                  "gimp-rect-select-tool\0",
      .func     = gimp_cairo_wilber_toggle_pointer_eyes
    }
  };

  static const gchar *spell_progress[G_N_ELEMENTS (spells)];
  const gchar        *tool_name;
  gint                i;

  tool_name = gimp_object_get_name (GIMP_OBJECT (tool_info));

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
