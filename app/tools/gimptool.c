/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis and others
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "tools-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpimage.h"
#include "core/gimplinklayer.h"
#include "core/gimpprogress.h"
#include "core/gimptoolinfo.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-cursor.h"
#include "display/gimpstatusbar.h"

#include "path/gimpvectorlayer.h"

#include "text/gimptextlayer.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpsourcetool.h"
#include "gimptool.h"
#include "gimptool-progress.h"
#include "gimptoolcontrol.h"
#include "gimptools-utils.h"

#include "gimp-log.h"
#include "gimp-intl.h"


/* #define DEBUG_ACTIVE_STATE 1 */


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void            gimp_tool_constructed         (GObject          *object);
static void            gimp_tool_dispose             (GObject          *object);
static void            gimp_tool_finalize            (GObject          *object);
static void            gimp_tool_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void            gimp_tool_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean        gimp_tool_real_has_display    (GimpTool         *tool,
                                                      GimpDisplay      *display);
static GimpDisplay   * gimp_tool_real_has_image      (GimpTool         *tool,
                                                      GimpImage        *image);
static gboolean        gimp_tool_real_initialize     (GimpTool         *tool,
                                                      GimpDisplay      *display,
                                                      GError          **error);
static void            gimp_tool_real_control        (GimpTool         *tool,
                                                      GimpToolAction    action,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_button_press   (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      GimpButtonPressType press_type,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_button_release (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      GimpButtonReleaseType release_type,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_motion         (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      GimpDisplay      *display);
static gboolean        gimp_tool_real_key_press      (GimpTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      GimpDisplay      *display);
static gboolean        gimp_tool_real_key_release    (GimpTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_modifier_key   (GimpTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      GimpDisplay      *display);
static void       gimp_tool_real_active_modifier_key (GimpTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_oper_update    (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      GdkModifierType   state,
                                                      gboolean          proximity,
                                                      GimpDisplay      *display);
static void            gimp_tool_real_cursor_update  (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      GdkModifierType   state,
                                                      GimpDisplay      *display);
static const gchar   * gimp_tool_real_can_undo       (GimpTool         *tool,
                                                      GimpDisplay      *display);
static const gchar   * gimp_tool_real_can_redo       (GimpTool         *tool,
                                                      GimpDisplay      *display);
static gboolean        gimp_tool_real_undo           (GimpTool         *tool,
                                                      GimpDisplay      *display);
static gboolean        gimp_tool_real_redo           (GimpTool         *tool,
                                                      GimpDisplay      *display);
static GimpUIManager * gimp_tool_real_get_popup      (GimpTool         *tool,
                                                      const GimpCoords *coords,
                                                      GdkModifierType   state,
                                                      GimpDisplay      *display,
                                                      const gchar     **ui_path);
static void            gimp_tool_real_options_notify (GimpTool         *tool,
                                                      GimpToolOptions  *options,
                                                      const GParamSpec *pspec);

static void            gimp_tool_options_notify      (GimpToolOptions  *options,
                                                      const GParamSpec *pspec,
                                                      GimpTool         *tool);
static void            gimp_tool_clear_status        (GimpTool         *tool);
static void            gimp_tool_release             (GimpTool         *tool);


G_DEFINE_TYPE_WITH_CODE (GimpTool, gimp_tool, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROGRESS,
                                                gimp_tool_progress_iface_init))

#define parent_class gimp_tool_parent_class

static gint global_tool_ID = 1;


static void
gimp_tool_class_init (GimpToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_tool_constructed;
  object_class->dispose      = gimp_tool_dispose;
  object_class->finalize     = gimp_tool_finalize;
  object_class->set_property = gimp_tool_set_property;
  object_class->get_property = gimp_tool_get_property;

  klass->has_display         = gimp_tool_real_has_display;
  klass->has_image           = gimp_tool_real_has_image;
  klass->initialize          = gimp_tool_real_initialize;
  klass->control             = gimp_tool_real_control;
  klass->button_press        = gimp_tool_real_button_press;
  klass->button_release      = gimp_tool_real_button_release;
  klass->motion              = gimp_tool_real_motion;
  klass->key_press           = gimp_tool_real_key_press;
  klass->key_release         = gimp_tool_real_key_release;
  klass->modifier_key        = gimp_tool_real_modifier_key;
  klass->active_modifier_key = gimp_tool_real_active_modifier_key;
  klass->oper_update         = gimp_tool_real_oper_update;
  klass->cursor_update       = gimp_tool_real_cursor_update;
  klass->can_undo            = gimp_tool_real_can_undo;
  klass->can_redo            = gimp_tool_real_can_redo;
  klass->undo                = gimp_tool_real_undo;
  klass->redo                = gimp_tool_real_redo;
  klass->get_popup           = gimp_tool_real_get_popup;
  klass->options_notify      = gimp_tool_real_options_notify;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_TOOL_INFO,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  klass->is_destructive = TRUE;
}

static void
gimp_tool_init (GimpTool *tool)
{
  tool->tool_info             = NULL;
  tool->ID                    = global_tool_ID++;
  tool->control               = g_object_new (GIMP_TYPE_TOOL_CONTROL, NULL);
  tool->display               = NULL;
  tool->drawables             = NULL;
  tool->focus_display         = NULL;
  tool->modifier_state        = 0;
  tool->active_modifier_state = 0;
  tool->button_press_state    = 0;
}

static void
gimp_tool_constructed (GObject *object)
{
  GimpTool *tool = GIMP_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  g_signal_connect_object (gimp_tool_get_options (tool), "notify",
                           G_CALLBACK (gimp_tool_options_notify),
                           tool, 0);
}

static void
gimp_tool_dispose (GObject *object)
{
  GimpTool *tool = GIMP_TOOL (object);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, tool->display);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_tool_finalize (GObject *object)
{
  GimpTool *tool = GIMP_TOOL (object);

  g_clear_object (&tool->tool_info);
  g_clear_object (&tool->control);
  g_clear_pointer (&tool->label,     g_free);
  g_clear_pointer (&tool->undo_desc, g_free);
  g_clear_pointer (&tool->icon_name, g_free);
  g_clear_pointer (&tool->help_id,   g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpTool *tool = GIMP_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      tool->tool_info = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpTool *tool = GIMP_TOOL (object);

  switch (property_id)
    {
    case PROP_TOOL_INFO:
      g_value_set_object (value, tool->tool_info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  standard member functions  */

static gboolean
gimp_tool_real_has_display (GimpTool    *tool,
                            GimpDisplay *display)
{
  return (display == tool->display ||
          g_list_find (tool->status_displays, display));
}

static GimpDisplay *
gimp_tool_real_has_image (GimpTool  *tool,
                          GimpImage *image)
{
  if (tool->display)
    {
      if (image && gimp_display_get_image (tool->display) == image)
        return tool->display;

      /*  NULL image means any display  */
      if (! image)
        return tool->display;
    }

  return NULL;
}

static gboolean
gimp_tool_real_initialize (GimpTool     *tool,
                           GimpDisplay  *display,
                           GError      **error)
{
  return TRUE;
}

static void
gimp_tool_real_control (GimpTool       *tool,
                        GimpToolAction  action,
                        GimpDisplay    *display)
{
  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      tool->display   = NULL;
      g_list_free (tool->drawables);
      tool->drawables = NULL;
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      break;
    }
}

static void
gimp_tool_real_button_press (GimpTool            *tool,
                             const GimpCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             GimpButtonPressType  press_type,
                             GimpDisplay         *display)
{
  if (press_type == GIMP_BUTTON_PRESS_NORMAL)
    {
      GimpImage *image = gimp_display_get_image (display);

      tool->display   = display;
      g_list_free (tool->drawables);
      tool->drawables = gimp_image_get_selected_drawables (image);

      gimp_tool_control_activate (tool->control);
    }
}

static void
gimp_tool_real_button_release (GimpTool              *tool,
                               const GimpCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               GimpButtonReleaseType  release_type,
                               GimpDisplay           *display)
{
  gimp_tool_control_halt (tool->control);
}

static void
gimp_tool_real_motion (GimpTool         *tool,
                       const GimpCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       GimpDisplay      *display)
{
}

static gboolean
gimp_tool_real_key_press (GimpTool    *tool,
                          GdkEventKey *kevent,
                          GimpDisplay *display)
{
  return FALSE;
}

static gboolean
gimp_tool_real_key_release (GimpTool    *tool,
                            GdkEventKey *kevent,
                            GimpDisplay *display)
{
  return FALSE;
}

static void
gimp_tool_real_modifier_key (GimpTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
}

static void
gimp_tool_real_active_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
}

static void
gimp_tool_real_oper_update (GimpTool         *tool,
                            const GimpCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            GimpDisplay      *display)
{
}

static void
gimp_tool_real_cursor_update (GimpTool         *tool,
                              const GimpCoords *coords,
                              GdkModifierType   state,
                              GimpDisplay      *display)
{
  gimp_tool_set_cursor (tool, display,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        gimp_tool_control_get_cursor_modifier (tool->control));
}

static const gchar *
gimp_tool_real_can_undo (GimpTool    *tool,
                         GimpDisplay *display)
{
  return NULL;
}

static const gchar *
gimp_tool_real_can_redo (GimpTool    *tool,
                         GimpDisplay *display)
{
  return NULL;
}

static gboolean
gimp_tool_real_undo (GimpTool    *tool,
                     GimpDisplay *display)
{
  return FALSE;
}

static gboolean
gimp_tool_real_redo (GimpTool    *tool,
                     GimpDisplay *display)
{
  return FALSE;
}

static GimpUIManager *
gimp_tool_real_get_popup (GimpTool         *tool,
                          const GimpCoords *coords,
                          GdkModifierType   state,
                          GimpDisplay      *display,
                          const gchar     **ui_path)
{
  *ui_path = NULL;

  return NULL;
}

static void
gimp_tool_real_options_notify (GimpTool         *tool,
                               GimpToolOptions  *options,
                               const GParamSpec *pspec)
{
}


/*  public functions  */

GimpToolOptions *
gimp_tool_get_options (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (GIMP_IS_TOOL_INFO (tool->tool_info), NULL);

  return tool->tool_info->tool_options;
}

void
gimp_tool_set_label (GimpTool    *tool,
                     const gchar *label)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_free (tool->label);
  tool->label = g_strdup (label);
}

const gchar *
gimp_tool_get_label (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  if (tool->label)
    return tool->label;

  return tool->tool_info->label;
}

void
gimp_tool_set_undo_desc (GimpTool    *tool,
                         const gchar *undo_desc)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_free (tool->undo_desc);
  tool->undo_desc = g_strdup (undo_desc);
}

const gchar *
gimp_tool_get_undo_desc (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  if (tool->undo_desc)
    return tool->undo_desc;

  return tool->tool_info->label;
}

void
gimp_tool_set_icon_name (GimpTool    *tool,
                         const gchar *icon_name)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_free (tool->icon_name);
  tool->icon_name = g_strdup (icon_name);
}

const gchar *
gimp_tool_get_icon_name (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  if (tool->icon_name)
    return tool->icon_name;

  return gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool->tool_info));
}

void
gimp_tool_set_help_id (GimpTool    *tool,
                       const gchar *help_id)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_free (tool->help_id);
  tool->help_id = g_strdup (help_id);
}

const gchar *
gimp_tool_get_help_id (GimpTool *tool)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);

  if (tool->help_id)
    return tool->help_id;

  return tool->tool_info->help_id;
}

gboolean
gimp_tool_has_display (GimpTool    *tool,
                       GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->has_display (tool, display);
}

GimpDisplay *
gimp_tool_has_image (GimpTool  *tool,
                     GimpImage *image)
{
  GimpDisplay *display;

  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (image == NULL || GIMP_IS_IMAGE (image), NULL);

  display = GIMP_TOOL_GET_CLASS (tool)->has_image (tool, image);

  /*  check status displays last because they don't affect the tool
   *  itself (unlike tool->display or draw_tool->display)
   */
  if (! display && tool->status_displays)
    {
      GList *list;

      for (list = tool->status_displays; list; list = g_list_next (list))
        {
          GimpDisplay *status_display = list->data;

          if (gimp_display_get_image (status_display) == image)
            return status_display;
        }

      /*  NULL image means any display  */
      if (! image)
        return tool->status_displays->data;
    }

  return display;
}

gboolean
gimp_tool_initialize (GimpTool    *tool,
                      GimpDisplay *display)
{
  GError *error = NULL;

  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (! GIMP_TOOL_GET_CLASS (tool)->initialize (tool, display, &error))
    {
      if (error)
        {
          gimp_tool_message_literal (tool, display, error->message);
          g_clear_error (&error);
        }

      return FALSE;
    }

  return TRUE;
}

void
gimp_tool_control (GimpTool       *tool,
                   GimpToolAction  action,
                   GimpDisplay    *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  g_object_ref (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      if (! gimp_tool_control_is_paused (tool->control))
        GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);

      gimp_tool_control_pause (tool->control);
      break;

    case GIMP_TOOL_ACTION_RESUME:
      if (gimp_tool_control_is_paused (tool->control))
        {
          gimp_tool_control_resume (tool->control);

          if (! gimp_tool_control_is_paused (tool->control))
            GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);
        }
      else
        {
          g_warning ("gimp_tool_control: unable to RESUME tool with "
                     "tool->control->paused_count == 0");
        }
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      gimp_tool_release (tool);

      GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);

      /*  always HALT after COMMIT here and not in each tool individually;
       *  some tools interact with their subclasses (e.g. filter tool
       *  and operation tool), and it's essential that COMMIT runs
       *  through the entire class hierarchy before HALT
       */
      action = GIMP_TOOL_ACTION_HALT;

      /* pass through */
    case GIMP_TOOL_ACTION_HALT:
      gimp_tool_release (tool);

      GIMP_TOOL_GET_CLASS (tool)->control (tool, action, display);

      if (gimp_tool_control_is_active (tool->control))
        gimp_tool_control_halt (tool->control);

      gimp_tool_clear_status (tool);
      break;
    }

  g_object_unref (tool);
}

void
gimp_tool_button_press (GimpTool            *tool,
                        const GimpCoords    *coords,
                        guint32              time,
                        GdkModifierType      state,
                        GimpButtonPressType  press_type,
                        GimpDisplay         *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  if (GIMP_TOOL_GET_CLASS (tool)->is_destructive)
    {
      GimpImage *image;
      GList     *drawables;

      image     = gimp_display_get_image (display);
      drawables = gimp_image_get_selected_drawables (image);
      for (GList *iter = drawables; iter; iter = iter->next)
        {
          GimpDrawable *drawable    = iter->data;
          GimpItem     *locked_item = NULL;

          if (gimp_item_is_vector_layer (GIMP_ITEM (drawable)) ||
              gimp_item_is_link_layer (GIMP_ITEM (drawable))   ||
              gimp_item_is_text_layer (GIMP_ITEM (drawable))   ||
              gimp_item_is_content_locked (GIMP_ITEM (drawable), &locked_item))
            {
              gboolean constrain_only;

              /* Allow vector/link or pixel-locked layers to be set as sources */
              constrain_only = (state & gimp_get_constrain_behavior_mask () &&
                                ! (state & gimp_get_extend_selection_mask ()));
              if (! GIMP_IS_SOURCE_TOOL (tool) || ! constrain_only)
                {
                  /* TRANSLATORS: this is a menu path. Make sure you use
                   * the same translations you used for the "Rasterize"
                   * action under the "Layer" menu.
                   */
                  gchar *menu_path = _("Layer > Rasterize");

                  if (gimp_item_is_text_layer (GIMP_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      gimp_tool_message (tool, display, _("Text layers must be rasterized (%s)."), menu_path);
                      gimp_tools_blink_item (display->gimp, GIMP_ITEM (drawable));
                    }
                  else if (gimp_item_is_link_layer (GIMP_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      gimp_tool_message (tool, display, _("Link layers must be rasterized (%s)."), menu_path);
                      gimp_tools_blink_item (display->gimp, GIMP_ITEM (drawable));
                    }
                  else if (gimp_item_is_vector_layer (GIMP_ITEM (drawable)))
                    {
                      /* TRANSLATORS: the string between parentheses will be a menu path. */
                      gimp_tool_message (tool, display, _("Vector layers must be rasterized (%s)."), menu_path);
                      gimp_tools_blink_item (display->gimp, GIMP_ITEM (drawable));
                    }
                  else
                    {
                      gimp_tool_message_literal (tool, display,
                                                 _("The selected item's pixels are locked."));
                      gimp_tools_blink_lock_box (display->gimp, locked_item);
                    }

                  g_list_free (drawables);
                  return;
                }
            }
        }
      g_list_free (drawables);
    }

  GIMP_TOOL_GET_CLASS (tool)->button_press (tool, coords, time, state,
                                            press_type, display);

  if (press_type == GIMP_BUTTON_PRESS_NORMAL &&
      gimp_tool_control_is_active (tool->control))
    {
      tool->button_press_state    = state;
      tool->active_modifier_state = state;

      tool->last_pointer_coords = *coords;
      tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
      tool->last_pointer_state  = state;

      if (gimp_tool_control_get_wants_click (tool->control))
        {
          tool->in_click_distance   = TRUE;
          tool->got_motion_event    = FALSE;
          tool->button_press_coords = *coords;
          tool->button_press_time   = time;
        }
      else
        {
          tool->in_click_distance   = FALSE;
        }
    }
}

static gboolean
gimp_tool_check_click_distance (GimpTool         *tool,
                                const GimpCoords *coords,
                                guint32           time,
                                GimpDisplay      *display)
{
  GimpDisplayShell *shell;
  gint              double_click_time;
  gint              double_click_distance;

  if (! tool->in_click_distance)
    return FALSE;

  shell = gimp_display_get_shell (display);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (shell)),
                "gtk-double-click-time",     &double_click_time,
                "gtk-double-click-distance", &double_click_distance,
                NULL);

  if ((time - tool->button_press_time) > double_click_time)
    {
      tool->in_click_distance = FALSE;
    }
  else
    {
      GimpDisplayShell *shell = gimp_display_get_shell (display);
      gdouble           dx;
      gdouble           dy;

      dx = SCALEX (shell, tool->button_press_coords.x - coords->x);
      dy = SCALEY (shell, tool->button_press_coords.y - coords->y);

      if ((SQR (dx) + SQR (dy)) > SQR (double_click_distance))
        {
          tool->in_click_distance = FALSE;
        }
    }

  return tool->in_click_distance;
}

void
gimp_tool_button_release (GimpTool         *tool,
                          const GimpCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          GimpDisplay      *display)
{
  GimpButtonReleaseType release_type = GIMP_BUTTON_RELEASE_NORMAL;
  GimpCoords            my_coords;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == TRUE);

  g_object_ref (tool);

  tool->last_pointer_state = 0;

  my_coords = *coords;

  if (state & GDK_BUTTON3_MASK)
    {
      release_type = GIMP_BUTTON_RELEASE_CANCEL;
    }
  else if (gimp_tool_control_get_wants_click (tool->control))
    {
      if (gimp_tool_check_click_distance (tool, coords, time, display))
        {
          release_type = GIMP_BUTTON_RELEASE_CLICK;
          my_coords    = tool->button_press_coords;

          if (tool->got_motion_event)
            {
              /*  if there has been a motion() since button_press(),
               *  synthesize a motion() back to the recorded press
               *  coordinates
               */
              GIMP_TOOL_GET_CLASS (tool)->motion (tool, &my_coords, time,
                                                  state & GDK_BUTTON1_MASK,
                                                  display);
            }
        }
      else if (! tool->got_motion_event)
        {
          release_type = GIMP_BUTTON_RELEASE_NO_MOTION;
        }
    }

  GIMP_TOOL_GET_CLASS (tool)->button_release (tool, &my_coords, time, state,
                                              release_type, display);

  g_warn_if_fail (gimp_tool_control_is_active (tool->control) == FALSE);

  if (tool->active_modifier_state                            != 0 &&
      gimp_tool_control_get_active_modifiers (tool->control) !=
      GIMP_TOOL_ACTIVE_MODIFIERS_SAME)
    {
      gimp_tool_control_activate (tool->control);

      gimp_tool_set_active_modifier_state (tool, 0, display);

      gimp_tool_control_halt (tool->control);
    }

  tool->button_press_state    = 0;
  tool->active_modifier_state = 0;

  g_object_unref (tool);
}

void
gimp_tool_motion (GimpTool         *tool,
                  const GimpCoords *coords,
                  guint32           time,
                  GdkModifierType   state,
                  GimpDisplay      *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == TRUE);

  tool->got_motion_event = TRUE;

  tool->last_pointer_coords = *coords;
  tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
  tool->last_pointer_state  = state;

  GIMP_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, display);
}

void
gimp_tool_set_focus_display (GimpTool    *tool,
                             GimpDisplay *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (display == NULL || GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == FALSE);

  GIMP_LOG (TOOL_FOCUS, "tool: %p  focus_display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  if (display != tool->focus_display)
    {
      if (tool->focus_display)
        {
          if (tool->active_modifier_state != 0)
            {
              gimp_tool_control_activate (tool->control);

              gimp_tool_set_active_modifier_state (tool, 0, tool->focus_display);

              gimp_tool_control_halt (tool->control);
            }

          if (tool->modifier_state != 0)
            gimp_tool_set_modifier_state (tool, 0, tool->focus_display);
        }

      tool->focus_display = display;
    }
}

gboolean
gimp_tool_key_press (GimpTool    *tool,
                     GdkEventKey *kevent,
                     GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (gimp_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->key_press (tool, kevent, display);
}

gboolean
gimp_tool_key_release (GimpTool    *tool,
                       GdkEventKey *kevent,
                       GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (gimp_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return GIMP_TOOL_GET_CLASS (tool)->key_release (tool, kevent, display);
}

static void
gimp_tool_modifier_key (GimpTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  GIMP_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, display);
}

static void
gimp_tool_active_modifier_key (GimpTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  GIMP_TOOL_GET_CLASS (tool)->active_modifier_key (tool, key, press, state,
                                                   display);
}

static gboolean
state_changed (GdkModifierType  old_state,
               GdkModifierType  new_state,
               GdkModifierType  modifier,
               gboolean        *press)
{
  if ((old_state & modifier) != (new_state & modifier))
    {
      *press = (new_state & modifier) ? TRUE : FALSE;

      return TRUE;
    }

  return FALSE;
}

void
gimp_tool_set_modifier_state (GimpTool        *tool,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  gboolean press;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == FALSE);

  GIMP_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  if (state_changed (tool->modifier_state, state, GDK_SHIFT_MASK, &press))
    {
      gimp_tool_modifier_key (tool, GDK_SHIFT_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_CONTROL_MASK, &press))
    {
      gimp_tool_modifier_key (tool, GDK_CONTROL_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD1_MASK, &press))
    {
      gimp_tool_modifier_key (tool, GDK_MOD1_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD2_MASK, &press))
    {
      gimp_tool_modifier_key (tool, GDK_MOD2_MASK,
                              press, state,
                              display);
    }

  tool->modifier_state = state;
}

void
gimp_tool_set_active_modifier_state (GimpTool        *tool,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpToolActiveModifiers active_modifiers;
  gboolean                press;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == TRUE);

  GIMP_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  active_modifiers = gimp_tool_control_get_active_modifiers (tool->control);

  if (state_changed (tool->active_modifier_state, state, GDK_SHIFT_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: SHIFT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case GIMP_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SAME:
          gimp_tool_modifier_key (tool, GDK_SHIFT_MASK,
                                  press, state,
                                  display);
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_SHIFT_MASK))
            {
              tool->button_press_state &= ~GDK_SHIFT_MASK;
            }
          else
            {
              gimp_tool_active_modifier_key (tool, GDK_SHIFT_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_CONTROL_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: CONTROL %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case GIMP_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SAME:
          gimp_tool_modifier_key (tool, GDK_CONTROL_MASK,
                                  press, state,
                                  display);
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_CONTROL_MASK))
            {
              tool->button_press_state &= ~GDK_CONTROL_MASK;
            }
          else
            {
              gimp_tool_active_modifier_key (tool, GDK_CONTROL_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_MOD1_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: ALT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case GIMP_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SAME:
          gimp_tool_modifier_key (tool, GDK_MOD1_MASK,
                                  press, state,
                                  display);
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD1_MASK))
            {
              tool->button_press_state &= ~GDK_MOD1_MASK;
            }
          else
            {
              gimp_tool_active_modifier_key (tool, GDK_MOD1_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  if (state_changed (tool->active_modifier_state, state, GDK_MOD2_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: MOD2 %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case GIMP_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SAME:
          gimp_tool_modifier_key (tool, GDK_MOD2_MASK,
                                  press, state,
                                  display);
          break;

        case GIMP_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD2_MASK))
            {
              tool->button_press_state &= ~GDK_MOD2_MASK;
            }
          else
            {
              gimp_tool_active_modifier_key (tool, GDK_MOD2_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  tool->active_modifier_state = state;

  if (active_modifiers == GIMP_TOOL_ACTIVE_MODIFIERS_SAME)
    tool->modifier_state = state;
}

void
gimp_tool_oper_update (GimpTool         *tool,
                       const GimpCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity,
                       GimpDisplay      *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == FALSE);

  GIMP_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, proximity,
                                           display);

  if (G_UNLIKELY (gimp_image_is_empty (gimp_display_get_image (display)) &&
                  ! gimp_tool_control_get_handle_empty_image (tool->control)))
    {
      gimp_tool_replace_status (tool, display,
                                "%s",
                                _("Can't work on an empty image, "
                                  "add a layer first"));
    }
}

void
gimp_tool_cursor_update (GimpTool         *tool,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         GimpDisplay      *display)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_tool_control_is_active (tool->control) == FALSE);

  GIMP_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, display);
}

const gchar *
gimp_tool_can_undo (GimpTool    *tool,
                    GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return GIMP_TOOL_GET_CLASS (tool)->can_undo (tool, display);

  return NULL;
}

const gchar *
gimp_tool_can_redo (GimpTool    *tool,
                    GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return GIMP_TOOL_GET_CLASS (tool)->can_redo (tool, display);

  return NULL;
}

gboolean
gimp_tool_undo (GimpTool    *tool,
                GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (gimp_tool_can_undo (tool, display))
    return GIMP_TOOL_GET_CLASS (tool)->undo (tool, display);

  return FALSE;
}

gboolean
gimp_tool_redo (GimpTool    *tool,
                GimpDisplay *display)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), FALSE);

  if (gimp_tool_can_redo (tool, display))
    return GIMP_TOOL_GET_CLASS (tool)->redo (tool, display);

  return FALSE;
}

GimpUIManager *
gimp_tool_get_popup (GimpTool         *tool,
                     const GimpCoords *coords,
                     GdkModifierType   state,
                     GimpDisplay      *display,
                     const gchar     **ui_path)
{
  g_return_val_if_fail (GIMP_IS_TOOL (tool), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  return GIMP_TOOL_GET_CLASS (tool)->get_popup (tool, coords, state, display,
                                                ui_path);
}

/*
 * gimp_tool_push_status:
 * @tool:
 * @display:
 * @format:
 *
 * Push a new status message for the context of the @tool, in the shell
 * associated to @display. Note that the message will replace any
 * message for the same context (i.e. same tool) while also ensuring it
 * is in the front of the message list (except for any progress or
 * temporary messages which might be going on and are always front).
 * This is the main difference with gimp_tool_replace_status() which
 * does not necessarily try to make it a front message.
 *
 * In particular, it means you don't have to call gimp_tool_pop_status()
 * first before calling gimp_tool_push_status(). Even more, you should
 * not pop the status if you were planning to push a new status for the
 * same context because you are triggering non necessary redraws of the
 * status bar.
 */
void
gimp_tool_push_status (GimpTool    *tool,
                       GimpDisplay *display,
                       const gchar *format,
                       ...)
{
  GimpDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = gimp_display_get_shell (display);

  icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool->tool_info));

  va_start (args, format);

  gimp_statusbar_push_valist (gimp_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_push_status_coords (GimpTool            *tool,
                              GimpDisplay         *display,
                              GimpCursorPrecision  precision,
                              const gchar         *title,
                              gdouble              x,
                              const gchar         *separator,
                              gdouble              y,
                              const gchar         *help)
{
  GimpDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = gimp_display_get_shell (display);

  icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool->tool_info));

  gimp_statusbar_push_coords (gimp_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              precision, title, x, separator, y,
                              help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_push_status_length (GimpTool            *tool,
                              GimpDisplay         *display,
                              const gchar         *title,
                              GimpOrientationType  axis,
                              gdouble              value,
                              const gchar         *help)
{
  GimpDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = gimp_display_get_shell (display);

  icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool->tool_info));

  gimp_statusbar_push_length (gimp_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              title, axis, value, help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

/*
 * gimp_tool_replace_status:
 * @tool:
 * @display:
 * @format:
 *
 * Push a new status message for the context of the @tool, in the shell
 * associated to @display. If any message for the same context (i.e.
 * same tool) is already lined up, it will be replaced without changing
 * the appearance order. In other words, it doesn't try to prioritize
 * this new status message. Yet if no message for the same context
 * existed already, the new status would end up front (except for any
 * progress or temporary messages which might be going on and are always
 * front).
 * See also gimp_tool_push_status().
 *
 * Therefore you should not call gimp_tool_pop_status() first before
 * calling gimp_tool_replace_status() as it would not be the same
 * behavior and could trigger unnecessary redraws of the status bar.
 */
void
gimp_tool_replace_status (GimpTool    *tool,
                          GimpDisplay *display,
                          const gchar *format,
                          ...)
{
  GimpDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = gimp_display_get_shell (display);

  icon_name = gimp_viewable_get_icon_name (GIMP_VIEWABLE (tool->tool_info));

  va_start (args, format);

  gimp_statusbar_replace_valist (gimp_display_shell_get_statusbar (shell),
                                 G_OBJECT_TYPE_NAME (tool), icon_name,
                                 format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
gimp_tool_pop_status (GimpTool    *tool,
                      GimpDisplay *display)
{
  GimpDisplayShell *shell;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  shell = gimp_display_get_shell (display);

  gimp_statusbar_pop (gimp_display_shell_get_statusbar (shell),
                      G_OBJECT_TYPE_NAME (tool));

  tool->status_displays = g_list_remove (tool->status_displays, display);
}

void
gimp_tool_message (GimpTool    *tool,
                   GimpDisplay *display,
                   const gchar *format,
                   ...)
{
  va_list args;

  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  gimp_message_valist (display->gimp, G_OBJECT (display),
                       GIMP_MESSAGE_WARNING, format, args);

  va_end (args);
}

void
gimp_tool_message_literal (GimpTool    *tool,
                           GimpDisplay *display,
                           const gchar *message)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (message != NULL);

  gimp_message_literal (display->gimp, G_OBJECT (display),
                        GIMP_MESSAGE_WARNING, message);
}

void
gimp_tool_set_cursor (GimpTool           *tool,
                      GimpDisplay        *display,
                      GimpCursorType      cursor,
                      GimpToolCursorType  tool_cursor,
                      GimpCursorModifier  modifier)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  gimp_display_shell_set_cursor (gimp_display_get_shell (display),
                                 cursor, tool_cursor, modifier);
}


/*  private functions  */

static void
gimp_tool_options_notify (GimpToolOptions  *options,
                          const GParamSpec *pspec,
                          GimpTool         *tool)
{
  GIMP_TOOL_GET_CLASS (tool)->options_notify (tool, options, pspec);
}

static void
gimp_tool_clear_status (GimpTool *tool)
{
  g_return_if_fail (GIMP_IS_TOOL (tool));

  while (tool->status_displays)
    gimp_tool_pop_status (tool, tool->status_displays->data);
}

static void
gimp_tool_release (GimpTool *tool)
{
  if (tool->last_pointer_state &&
      gimp_tool_control_is_active (tool->control))
    {
      gimp_tool_button_release (
        tool,
        &tool->last_pointer_coords,
        tool->last_pointer_time + g_get_monotonic_time () / 1000,
        tool->last_pointer_state,
        tool->display);
    }
}
