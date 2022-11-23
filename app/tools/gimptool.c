/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmaimage.h"
#include "core/ligmaprogress.h"
#include "core/ligmatoolinfo.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-cursor.h"
#include "display/ligmastatusbar.h"

#include "ligmatool.h"
#include "ligmatool-progress.h"
#include "ligmatoolcontrol.h"

#include "ligma-log.h"
#include "ligma-intl.h"


/* #define DEBUG_ACTIVE_STATE 1 */


enum
{
  PROP_0,
  PROP_TOOL_INFO
};


static void            ligma_tool_constructed         (GObject          *object);
static void            ligma_tool_dispose             (GObject          *object);
static void            ligma_tool_finalize            (GObject          *object);
static void            ligma_tool_set_property        (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void            ligma_tool_get_property        (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static gboolean        ligma_tool_real_has_display    (LigmaTool         *tool,
                                                      LigmaDisplay      *display);
static LigmaDisplay   * ligma_tool_real_has_image      (LigmaTool         *tool,
                                                      LigmaImage        *image);
static gboolean        ligma_tool_real_initialize     (LigmaTool         *tool,
                                                      LigmaDisplay      *display,
                                                      GError          **error);
static void            ligma_tool_real_control        (LigmaTool         *tool,
                                                      LigmaToolAction    action,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_button_press   (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      LigmaButtonPressType press_type,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_button_release (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      LigmaButtonReleaseType release_type,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_motion         (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      guint32           time,
                                                      GdkModifierType   state,
                                                      LigmaDisplay      *display);
static gboolean        ligma_tool_real_key_press      (LigmaTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      LigmaDisplay      *display);
static gboolean        ligma_tool_real_key_release    (LigmaTool         *tool,
                                                      GdkEventKey      *kevent,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_modifier_key   (LigmaTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      LigmaDisplay      *display);
static void       ligma_tool_real_active_modifier_key (LigmaTool         *tool,
                                                      GdkModifierType   key,
                                                      gboolean          press,
                                                      GdkModifierType   state,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_oper_update    (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      GdkModifierType   state,
                                                      gboolean          proximity,
                                                      LigmaDisplay      *display);
static void            ligma_tool_real_cursor_update  (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      GdkModifierType   state,
                                                      LigmaDisplay      *display);
static const gchar   * ligma_tool_real_can_undo       (LigmaTool         *tool,
                                                      LigmaDisplay      *display);
static const gchar   * ligma_tool_real_can_redo       (LigmaTool         *tool,
                                                      LigmaDisplay      *display);
static gboolean        ligma_tool_real_undo           (LigmaTool         *tool,
                                                      LigmaDisplay      *display);
static gboolean        ligma_tool_real_redo           (LigmaTool         *tool,
                                                      LigmaDisplay      *display);
static LigmaUIManager * ligma_tool_real_get_popup      (LigmaTool         *tool,
                                                      const LigmaCoords *coords,
                                                      GdkModifierType   state,
                                                      LigmaDisplay      *display,
                                                      const gchar     **ui_path);
static void            ligma_tool_real_options_notify (LigmaTool         *tool,
                                                      LigmaToolOptions  *options,
                                                      const GParamSpec *pspec);

static void            ligma_tool_options_notify      (LigmaToolOptions  *options,
                                                      const GParamSpec *pspec,
                                                      LigmaTool         *tool);
static void            ligma_tool_clear_status        (LigmaTool         *tool);
static void            ligma_tool_release             (LigmaTool         *tool);


G_DEFINE_TYPE_WITH_CODE (LigmaTool, ligma_tool, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROGRESS,
                                                ligma_tool_progress_iface_init))

#define parent_class ligma_tool_parent_class

static gint global_tool_ID = 1;


static void
ligma_tool_class_init (LigmaToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_tool_constructed;
  object_class->dispose      = ligma_tool_dispose;
  object_class->finalize     = ligma_tool_finalize;
  object_class->set_property = ligma_tool_set_property;
  object_class->get_property = ligma_tool_get_property;

  klass->has_display         = ligma_tool_real_has_display;
  klass->has_image           = ligma_tool_real_has_image;
  klass->initialize          = ligma_tool_real_initialize;
  klass->control             = ligma_tool_real_control;
  klass->button_press        = ligma_tool_real_button_press;
  klass->button_release      = ligma_tool_real_button_release;
  klass->motion              = ligma_tool_real_motion;
  klass->key_press           = ligma_tool_real_key_press;
  klass->key_release         = ligma_tool_real_key_release;
  klass->modifier_key        = ligma_tool_real_modifier_key;
  klass->active_modifier_key = ligma_tool_real_active_modifier_key;
  klass->oper_update         = ligma_tool_real_oper_update;
  klass->cursor_update       = ligma_tool_real_cursor_update;
  klass->can_undo            = ligma_tool_real_can_undo;
  klass->can_redo            = ligma_tool_real_can_redo;
  klass->undo                = ligma_tool_real_undo;
  klass->redo                = ligma_tool_real_redo;
  klass->get_popup           = ligma_tool_real_get_popup;
  klass->options_notify      = ligma_tool_real_options_notify;

  g_object_class_install_property (object_class, PROP_TOOL_INFO,
                                   g_param_spec_object ("tool-info",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TOOL_INFO,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_tool_init (LigmaTool *tool)
{
  tool->tool_info             = NULL;
  tool->ID                    = global_tool_ID++;
  tool->control               = g_object_new (LIGMA_TYPE_TOOL_CONTROL, NULL);
  tool->display               = NULL;
  tool->drawables             = NULL;
  tool->focus_display         = NULL;
  tool->modifier_state        = 0;
  tool->active_modifier_state = 0;
  tool->button_press_state    = 0;
}

static void
ligma_tool_constructed (GObject *object)
{
  LigmaTool *tool = LIGMA_TOOL (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_TOOL_INFO (tool->tool_info));

  g_signal_connect_object (ligma_tool_get_options (tool), "notify",
                           G_CALLBACK (ligma_tool_options_notify),
                           tool, 0);
}

static void
ligma_tool_dispose (GObject *object)
{
  LigmaTool *tool = LIGMA_TOOL (object);

  ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, tool->display);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_tool_finalize (GObject *object)
{
  LigmaTool *tool = LIGMA_TOOL (object);

  g_clear_object (&tool->tool_info);
  g_clear_object (&tool->control);
  g_clear_pointer (&tool->label,     g_free);
  g_clear_pointer (&tool->undo_desc, g_free);
  g_clear_pointer (&tool->icon_name, g_free);
  g_clear_pointer (&tool->help_id,   g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaTool *tool = LIGMA_TOOL (object);

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
ligma_tool_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  LigmaTool *tool = LIGMA_TOOL (object);

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
ligma_tool_real_has_display (LigmaTool    *tool,
                            LigmaDisplay *display)
{
  return (display == tool->display ||
          g_list_find (tool->status_displays, display));
}

static LigmaDisplay *
ligma_tool_real_has_image (LigmaTool  *tool,
                          LigmaImage *image)
{
  if (tool->display)
    {
      if (image && ligma_display_get_image (tool->display) == image)
        return tool->display;

      /*  NULL image means any display  */
      if (! image)
        return tool->display;
    }

  return NULL;
}

static gboolean
ligma_tool_real_initialize (LigmaTool     *tool,
                           LigmaDisplay  *display,
                           GError      **error)
{
  return TRUE;
}

static void
ligma_tool_real_control (LigmaTool       *tool,
                        LigmaToolAction  action,
                        LigmaDisplay    *display)
{
  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      tool->display   = NULL;
      g_list_free (tool->drawables);
      tool->drawables = NULL;
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      break;
    }
}

static void
ligma_tool_real_button_press (LigmaTool            *tool,
                             const LigmaCoords    *coords,
                             guint32              time,
                             GdkModifierType      state,
                             LigmaButtonPressType  press_type,
                             LigmaDisplay         *display)
{
  if (press_type == LIGMA_BUTTON_PRESS_NORMAL)
    {
      LigmaImage *image = ligma_display_get_image (display);

      tool->display   = display;
      g_list_free (tool->drawables);
      tool->drawables = ligma_image_get_selected_drawables (image);

      ligma_tool_control_activate (tool->control);
    }
}

static void
ligma_tool_real_button_release (LigmaTool              *tool,
                               const LigmaCoords      *coords,
                               guint32                time,
                               GdkModifierType        state,
                               LigmaButtonReleaseType  release_type,
                               LigmaDisplay           *display)
{
  ligma_tool_control_halt (tool->control);
}

static void
ligma_tool_real_motion (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       guint32           time,
                       GdkModifierType   state,
                       LigmaDisplay      *display)
{
}

static gboolean
ligma_tool_real_key_press (LigmaTool    *tool,
                          GdkEventKey *kevent,
                          LigmaDisplay *display)
{
  return FALSE;
}

static gboolean
ligma_tool_real_key_release (LigmaTool    *tool,
                            GdkEventKey *kevent,
                            LigmaDisplay *display)
{
  return FALSE;
}

static void
ligma_tool_real_modifier_key (LigmaTool        *tool,
                             GdkModifierType  key,
                             gboolean         press,
                             GdkModifierType  state,
                             LigmaDisplay     *display)
{
}

static void
ligma_tool_real_active_modifier_key (LigmaTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    LigmaDisplay     *display)
{
}

static void
ligma_tool_real_oper_update (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            GdkModifierType   state,
                            gboolean          proximity,
                            LigmaDisplay      *display)
{
}

static void
ligma_tool_real_cursor_update (LigmaTool         *tool,
                              const LigmaCoords *coords,
                              GdkModifierType   state,
                              LigmaDisplay      *display)
{
  ligma_tool_set_cursor (tool, display,
                        ligma_tool_control_get_cursor (tool->control),
                        ligma_tool_control_get_tool_cursor (tool->control),
                        ligma_tool_control_get_cursor_modifier (tool->control));
}

static const gchar *
ligma_tool_real_can_undo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  return NULL;
}

static const gchar *
ligma_tool_real_can_redo (LigmaTool    *tool,
                         LigmaDisplay *display)
{
  return NULL;
}

static gboolean
ligma_tool_real_undo (LigmaTool    *tool,
                     LigmaDisplay *display)
{
  return FALSE;
}

static gboolean
ligma_tool_real_redo (LigmaTool    *tool,
                     LigmaDisplay *display)
{
  return FALSE;
}

static LigmaUIManager *
ligma_tool_real_get_popup (LigmaTool         *tool,
                          const LigmaCoords *coords,
                          GdkModifierType   state,
                          LigmaDisplay      *display,
                          const gchar     **ui_path)
{
  *ui_path = NULL;

  return NULL;
}

static void
ligma_tool_real_options_notify (LigmaTool         *tool,
                               LigmaToolOptions  *options,
                               const GParamSpec *pspec)
{
}


/*  public functions  */

LigmaToolOptions *
ligma_tool_get_options (LigmaTool *tool)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);
  g_return_val_if_fail (LIGMA_IS_TOOL_INFO (tool->tool_info), NULL);

  return tool->tool_info->tool_options;
}

void
ligma_tool_set_label (LigmaTool    *tool,
                     const gchar *label)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  g_free (tool->label);
  tool->label = g_strdup (label);
}

const gchar *
ligma_tool_get_label (LigmaTool *tool)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);

  if (tool->label)
    return tool->label;

  return tool->tool_info->label;
}

void
ligma_tool_set_undo_desc (LigmaTool    *tool,
                         const gchar *undo_desc)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  g_free (tool->undo_desc);
  tool->undo_desc = g_strdup (undo_desc);
}

const gchar *
ligma_tool_get_undo_desc (LigmaTool *tool)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);

  if (tool->undo_desc)
    return tool->undo_desc;

  return tool->tool_info->label;
}

void
ligma_tool_set_icon_name (LigmaTool    *tool,
                         const gchar *icon_name)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  g_free (tool->icon_name);
  tool->icon_name = g_strdup (icon_name);
}

const gchar *
ligma_tool_get_icon_name (LigmaTool *tool)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);

  if (tool->icon_name)
    return tool->icon_name;

  return ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool->tool_info));
}

void
ligma_tool_set_help_id (LigmaTool    *tool,
                       const gchar *help_id)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  g_free (tool->help_id);
  tool->help_id = g_strdup (help_id);
}

const gchar *
ligma_tool_get_help_id (LigmaTool *tool)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);

  if (tool->help_id)
    return tool->help_id;

  return tool->tool_info->help_id;
}

gboolean
ligma_tool_has_display (LigmaTool    *tool,
                       LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  return LIGMA_TOOL_GET_CLASS (tool)->has_display (tool, display);
}

LigmaDisplay *
ligma_tool_has_image (LigmaTool  *tool,
                     LigmaImage *image)
{
  LigmaDisplay *display;

  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);
  g_return_val_if_fail (image == NULL || LIGMA_IS_IMAGE (image), NULL);

  display = LIGMA_TOOL_GET_CLASS (tool)->has_image (tool, image);

  /*  check status displays last because they don't affect the tool
   *  itself (unlike tool->display or draw_tool->display)
   */
  if (! display && tool->status_displays)
    {
      GList *list;

      for (list = tool->status_displays; list; list = g_list_next (list))
        {
          LigmaDisplay *status_display = list->data;

          if (ligma_display_get_image (status_display) == image)
            return status_display;
        }

      /*  NULL image means any display  */
      if (! image)
        return tool->status_displays->data;
    }

  return display;
}

gboolean
ligma_tool_initialize (LigmaTool    *tool,
                      LigmaDisplay *display)
{
  GError *error = NULL;

  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  if (! LIGMA_TOOL_GET_CLASS (tool)->initialize (tool, display, &error))
    {
      if (error)
        {
          ligma_tool_message_literal (tool, display, error->message);
          g_clear_error (&error);
        }

      return FALSE;
    }

  return TRUE;
}

void
ligma_tool_control (LigmaTool       *tool,
                   LigmaToolAction  action,
                   LigmaDisplay    *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  g_object_ref (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
      if (! ligma_tool_control_is_paused (tool->control))
        LIGMA_TOOL_GET_CLASS (tool)->control (tool, action, display);

      ligma_tool_control_pause (tool->control);
      break;

    case LIGMA_TOOL_ACTION_RESUME:
      if (ligma_tool_control_is_paused (tool->control))
        {
          ligma_tool_control_resume (tool->control);

          if (! ligma_tool_control_is_paused (tool->control))
            LIGMA_TOOL_GET_CLASS (tool)->control (tool, action, display);
        }
      else
        {
          g_warning ("ligma_tool_control: unable to RESUME tool with "
                     "tool->control->paused_count == 0");
        }
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_tool_release (tool);

      LIGMA_TOOL_GET_CLASS (tool)->control (tool, action, display);

      /*  always HALT after COMMIT here and not in each tool individually;
       *  some tools interact with their subclasses (e.g. filter tool
       *  and operation tool), and it's essential that COMMIT runs
       *  through the entire class hierarchy before HALT
       */
      action = LIGMA_TOOL_ACTION_HALT;

      /* pass through */
    case LIGMA_TOOL_ACTION_HALT:
      ligma_tool_release (tool);

      LIGMA_TOOL_GET_CLASS (tool)->control (tool, action, display);

      if (ligma_tool_control_is_active (tool->control))
        ligma_tool_control_halt (tool->control);

      ligma_tool_clear_status (tool);
      break;
    }

  g_object_unref (tool);
}

void
ligma_tool_button_press (LigmaTool            *tool,
                        const LigmaCoords    *coords,
                        guint32              time,
                        GdkModifierType      state,
                        LigmaButtonPressType  press_type,
                        LigmaDisplay         *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  LIGMA_TOOL_GET_CLASS (tool)->button_press (tool, coords, time, state,
                                            press_type, display);

  if (press_type == LIGMA_BUTTON_PRESS_NORMAL &&
      ligma_tool_control_is_active (tool->control))
    {
      tool->button_press_state    = state;
      tool->active_modifier_state = state;

      tool->last_pointer_coords = *coords;
      tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
      tool->last_pointer_state  = state;

      if (ligma_tool_control_get_wants_click (tool->control))
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
ligma_tool_check_click_distance (LigmaTool         *tool,
                                const LigmaCoords *coords,
                                guint32           time,
                                LigmaDisplay      *display)
{
  LigmaDisplayShell *shell;
  gint              double_click_time;
  gint              double_click_distance;

  if (! tool->in_click_distance)
    return FALSE;

  shell = ligma_display_get_shell (display);

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
      LigmaDisplayShell *shell = ligma_display_get_shell (display);
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
ligma_tool_button_release (LigmaTool         *tool,
                          const LigmaCoords *coords,
                          guint32           time,
                          GdkModifierType   state,
                          LigmaDisplay      *display)
{
  LigmaButtonReleaseType release_type = LIGMA_BUTTON_RELEASE_NORMAL;
  LigmaCoords            my_coords;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == TRUE);

  g_object_ref (tool);

  tool->last_pointer_state = 0;

  my_coords = *coords;

  if (state & GDK_BUTTON3_MASK)
    {
      release_type = LIGMA_BUTTON_RELEASE_CANCEL;
    }
  else if (ligma_tool_control_get_wants_click (tool->control))
    {
      if (ligma_tool_check_click_distance (tool, coords, time, display))
        {
          release_type = LIGMA_BUTTON_RELEASE_CLICK;
          my_coords    = tool->button_press_coords;

          if (tool->got_motion_event)
            {
              /*  if there has been a motion() since button_press(),
               *  synthesize a motion() back to the recorded press
               *  coordinates
               */
              LIGMA_TOOL_GET_CLASS (tool)->motion (tool, &my_coords, time,
                                                  state & GDK_BUTTON1_MASK,
                                                  display);
            }
        }
      else if (! tool->got_motion_event)
        {
          release_type = LIGMA_BUTTON_RELEASE_NO_MOTION;
        }
    }

  LIGMA_TOOL_GET_CLASS (tool)->button_release (tool, &my_coords, time, state,
                                              release_type, display);

  g_warn_if_fail (ligma_tool_control_is_active (tool->control) == FALSE);

  if (tool->active_modifier_state                            != 0 &&
      ligma_tool_control_get_active_modifiers (tool->control) !=
      LIGMA_TOOL_ACTIVE_MODIFIERS_SAME)
    {
      ligma_tool_control_activate (tool->control);

      ligma_tool_set_active_modifier_state (tool, 0, display);

      ligma_tool_control_halt (tool->control);
    }

  tool->button_press_state    = 0;
  tool->active_modifier_state = 0;

  g_object_unref (tool);
}

void
ligma_tool_motion (LigmaTool         *tool,
                  const LigmaCoords *coords,
                  guint32           time,
                  GdkModifierType   state,
                  LigmaDisplay      *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == TRUE);

  tool->got_motion_event = TRUE;

  tool->last_pointer_coords = *coords;
  tool->last_pointer_time   = time - g_get_monotonic_time () / 1000;
  tool->last_pointer_state  = state;

  LIGMA_TOOL_GET_CLASS (tool)->motion (tool, coords, time, state, display);
}

void
ligma_tool_set_focus_display (LigmaTool    *tool,
                             LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (display == NULL || LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == FALSE);

  LIGMA_LOG (TOOL_FOCUS, "tool: %p  focus_display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  if (display != tool->focus_display)
    {
      if (tool->focus_display)
        {
          if (tool->active_modifier_state != 0)
            {
              ligma_tool_control_activate (tool->control);

              ligma_tool_set_active_modifier_state (tool, 0, tool->focus_display);

              ligma_tool_control_halt (tool->control);
            }

          if (tool->modifier_state != 0)
            ligma_tool_set_modifier_state (tool, 0, tool->focus_display);
        }

      tool->focus_display = display;
    }
}

gboolean
ligma_tool_key_press (LigmaTool    *tool,
                     GdkEventKey *kevent,
                     LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (ligma_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return LIGMA_TOOL_GET_CLASS (tool)->key_press (tool, kevent, display);
}

gboolean
ligma_tool_key_release (LigmaTool    *tool,
                       GdkEventKey *kevent,
                       LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (display == tool->focus_display, FALSE);
  g_return_val_if_fail (ligma_tool_control_is_active (tool->control) == FALSE,
                        FALSE);

  return LIGMA_TOOL_GET_CLASS (tool)->key_release (tool, kevent, display);
}

static void
ligma_tool_modifier_key (LigmaTool        *tool,
                        GdkModifierType  key,
                        gboolean         press,
                        GdkModifierType  state,
                        LigmaDisplay     *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  LIGMA_TOOL_GET_CLASS (tool)->modifier_key (tool, key, press, state, display);
}

static void
ligma_tool_active_modifier_key (LigmaTool        *tool,
                               GdkModifierType  key,
                               gboolean         press,
                               GdkModifierType  state,
                               LigmaDisplay     *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (display == tool->focus_display);

  LIGMA_TOOL_GET_CLASS (tool)->active_modifier_key (tool, key, press, state,
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
ligma_tool_set_modifier_state (LigmaTool        *tool,
                              GdkModifierType  state,
                              LigmaDisplay     *display)
{
  gboolean press;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == FALSE);

  LIGMA_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  if (state_changed (tool->modifier_state, state, GDK_SHIFT_MASK, &press))
    {
      ligma_tool_modifier_key (tool, GDK_SHIFT_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_CONTROL_MASK, &press))
    {
      ligma_tool_modifier_key (tool, GDK_CONTROL_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD1_MASK, &press))
    {
      ligma_tool_modifier_key (tool, GDK_MOD1_MASK,
                              press, state,
                              display);
    }

  if (state_changed (tool->modifier_state, state, GDK_MOD2_MASK, &press))
    {
      ligma_tool_modifier_key (tool, GDK_MOD2_MASK,
                              press, state,
                              display);
    }

  tool->modifier_state = state;
}

void
ligma_tool_set_active_modifier_state (LigmaTool        *tool,
                                     GdkModifierType  state,
                                     LigmaDisplay     *display)
{
  LigmaToolActiveModifiers active_modifiers;
  gboolean                press;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == TRUE);

  LIGMA_LOG (TOOL_FOCUS, "tool: %p  display: %p  tool->focus_display: %p",
            tool, display, tool->focus_display);

  g_return_if_fail (display == tool->focus_display);

  active_modifiers = ligma_tool_control_get_active_modifiers (tool->control);

  if (state_changed (tool->active_modifier_state, state, GDK_SHIFT_MASK,
                     &press))
    {
#ifdef DEBUG_ACTIVE_STATE
      g_printerr ("%s: SHIFT %s\n", G_STRFUNC,
                  press ? "pressed" : "released");
#endif

      switch (active_modifiers)
        {
        case LIGMA_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SAME:
          ligma_tool_modifier_key (tool, GDK_SHIFT_MASK,
                                  press, state,
                                  display);
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_SHIFT_MASK))
            {
              tool->button_press_state &= ~GDK_SHIFT_MASK;
            }
          else
            {
              ligma_tool_active_modifier_key (tool, GDK_SHIFT_MASK,
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
        case LIGMA_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SAME:
          ligma_tool_modifier_key (tool, GDK_CONTROL_MASK,
                                  press, state,
                                  display);
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_CONTROL_MASK))
            {
              tool->button_press_state &= ~GDK_CONTROL_MASK;
            }
          else
            {
              ligma_tool_active_modifier_key (tool, GDK_CONTROL_MASK,
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
        case LIGMA_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SAME:
          ligma_tool_modifier_key (tool, GDK_MOD1_MASK,
                                  press, state,
                                  display);
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD1_MASK))
            {
              tool->button_press_state &= ~GDK_MOD1_MASK;
            }
          else
            {
              ligma_tool_active_modifier_key (tool, GDK_MOD1_MASK,
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
        case LIGMA_TOOL_ACTIVE_MODIFIERS_OFF:
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SAME:
          ligma_tool_modifier_key (tool, GDK_MOD2_MASK,
                                  press, state,
                                  display);
          break;

        case LIGMA_TOOL_ACTIVE_MODIFIERS_SEPARATE:
          if (! press && (tool->button_press_state & GDK_MOD2_MASK))
            {
              tool->button_press_state &= ~GDK_MOD2_MASK;
            }
          else
            {
              ligma_tool_active_modifier_key (tool, GDK_MOD2_MASK,
                                             press, state,
                                             display);
            }
          break;
        }
    }

  tool->active_modifier_state = state;

  if (active_modifiers == LIGMA_TOOL_ACTIVE_MODIFIERS_SAME)
    tool->modifier_state = state;
}

void
ligma_tool_oper_update (LigmaTool         *tool,
                       const LigmaCoords *coords,
                       GdkModifierType   state,
                       gboolean          proximity,
                       LigmaDisplay      *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == FALSE);

  LIGMA_TOOL_GET_CLASS (tool)->oper_update (tool, coords, state, proximity,
                                           display);

  if (G_UNLIKELY (ligma_image_is_empty (ligma_display_get_image (display)) &&
                  ! ligma_tool_control_get_handle_empty_image (tool->control)))
    {
      ligma_tool_replace_status (tool, display,
                                "%s",
                                _("Can't work on an empty image, "
                                  "add a layer first"));
    }
}

void
ligma_tool_cursor_update (LigmaTool         *tool,
                         const LigmaCoords *coords,
                         GdkModifierType   state,
                         LigmaDisplay      *display)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (coords != NULL);
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (ligma_tool_control_is_active (tool->control) == FALSE);

  LIGMA_TOOL_GET_CLASS (tool)->cursor_update (tool, coords, state, display);
}

const gchar *
ligma_tool_can_undo (LigmaTool    *tool,
                    LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return LIGMA_TOOL_GET_CLASS (tool)->can_undo (tool, display);

  return NULL;
}

const gchar *
ligma_tool_can_redo (LigmaTool    *tool,
                    LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);

  if (display == tool->display)
    return LIGMA_TOOL_GET_CLASS (tool)->can_redo (tool, display);

  return NULL;
}

gboolean
ligma_tool_undo (LigmaTool    *tool,
                LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  if (ligma_tool_can_undo (tool, display))
    return LIGMA_TOOL_GET_CLASS (tool)->undo (tool, display);

  return FALSE;
}

gboolean
ligma_tool_redo (LigmaTool    *tool,
                LigmaDisplay *display)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), FALSE);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), FALSE);

  if (ligma_tool_can_redo (tool, display))
    return LIGMA_TOOL_GET_CLASS (tool)->redo (tool, display);

  return FALSE;
}

LigmaUIManager *
ligma_tool_get_popup (LigmaTool         *tool,
                     const LigmaCoords *coords,
                     GdkModifierType   state,
                     LigmaDisplay      *display,
                     const gchar     **ui_path)
{
  g_return_val_if_fail (LIGMA_IS_TOOL (tool), NULL);
  g_return_val_if_fail (coords != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_DISPLAY (display), NULL);
  g_return_val_if_fail (ui_path != NULL, NULL);

  return LIGMA_TOOL_GET_CLASS (tool)->get_popup (tool, coords, state, display,
                                                ui_path);
}

/*
 * ligma_tool_push_status:
 * @tool:
 * @display:
 * @format:
 *
 * Push a new status message for the context of the @tool, in the shell
 * associated to @display. Note that the message will replace any
 * message for the same context (i.e. same tool) while also ensuring it
 * is in the front of the message list (except for any progress or
 * temporary messages which might be going on and are always front).
 * This is the main difference with ligma_tool_replace_status() which
 * does not necessarily try to make it a front message.
 *
 * In particular, it means you don't have to call ligma_tool_pop_status()
 * first before calling ligma_tool_push_status(). Even more, you should
 * not pop the status if you were planning to push a new status for the
 * same context because you are triggering non necessary redraws of the
 * status bar.
 */
void
ligma_tool_push_status (LigmaTool    *tool,
                       LigmaDisplay *display,
                       const gchar *format,
                       ...)
{
  LigmaDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = ligma_display_get_shell (display);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool->tool_info));

  va_start (args, format);

  ligma_statusbar_push_valist (ligma_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
ligma_tool_push_status_coords (LigmaTool            *tool,
                              LigmaDisplay         *display,
                              LigmaCursorPrecision  precision,
                              const gchar         *title,
                              gdouble              x,
                              const gchar         *separator,
                              gdouble              y,
                              const gchar         *help)
{
  LigmaDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  shell = ligma_display_get_shell (display);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool->tool_info));

  ligma_statusbar_push_coords (ligma_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              precision, title, x, separator, y,
                              help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
ligma_tool_push_status_length (LigmaTool            *tool,
                              LigmaDisplay         *display,
                              const gchar         *title,
                              LigmaOrientationType  axis,
                              gdouble              value,
                              const gchar         *help)
{
  LigmaDisplayShell *shell;
  const gchar      *icon_name;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  shell = ligma_display_get_shell (display);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool->tool_info));

  ligma_statusbar_push_length (ligma_display_shell_get_statusbar (shell),
                              G_OBJECT_TYPE_NAME (tool), icon_name,
                              title, axis, value, help);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

/*
 * ligma_tool_replace_status:
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
 * See also ligma_tool_push_status().
 *
 * Therefore you should not call ligma_tool_pop_status() first before
 * calling ligma_tool_replace_status() as it would not be the same
 * behavior and could trigger unnecessary redraws of the status bar.
 */
void
ligma_tool_replace_status (LigmaTool    *tool,
                          LigmaDisplay *display,
                          const gchar *format,
                          ...)
{
  LigmaDisplayShell *shell;
  const gchar      *icon_name;
  va_list           args;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  shell = ligma_display_get_shell (display);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool->tool_info));

  va_start (args, format);

  ligma_statusbar_replace_valist (ligma_display_shell_get_statusbar (shell),
                                 G_OBJECT_TYPE_NAME (tool), icon_name,
                                 format, args);

  va_end (args);

  tool->status_displays = g_list_remove (tool->status_displays, display);
  tool->status_displays = g_list_prepend (tool->status_displays, display);
}

void
ligma_tool_pop_status (LigmaTool    *tool,
                      LigmaDisplay *display)
{
  LigmaDisplayShell *shell;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  shell = ligma_display_get_shell (display);

  ligma_statusbar_pop (ligma_display_shell_get_statusbar (shell),
                      G_OBJECT_TYPE_NAME (tool));

  tool->status_displays = g_list_remove (tool->status_displays, display);
}

void
ligma_tool_message (LigmaTool    *tool,
                   LigmaDisplay *display,
                   const gchar *format,
                   ...)
{
  va_list args;

  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (format != NULL);

  va_start (args, format);

  ligma_message_valist (display->ligma, G_OBJECT (display),
                       LIGMA_MESSAGE_WARNING, format, args);

  va_end (args);
}

void
ligma_tool_message_literal (LigmaTool    *tool,
                           LigmaDisplay *display,
                           const gchar *message)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));
  g_return_if_fail (message != NULL);

  ligma_message_literal (display->ligma, G_OBJECT (display),
                        LIGMA_MESSAGE_WARNING, message);
}

void
ligma_tool_set_cursor (LigmaTool           *tool,
                      LigmaDisplay        *display,
                      LigmaCursorType      cursor,
                      LigmaToolCursorType  tool_cursor,
                      LigmaCursorModifier  modifier)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));
  g_return_if_fail (LIGMA_IS_DISPLAY (display));

  ligma_display_shell_set_cursor (ligma_display_get_shell (display),
                                 cursor, tool_cursor, modifier);
}


/*  private functions  */

static void
ligma_tool_options_notify (LigmaToolOptions  *options,
                          const GParamSpec *pspec,
                          LigmaTool         *tool)
{
  LIGMA_TOOL_GET_CLASS (tool)->options_notify (tool, options, pspec);
}

static void
ligma_tool_clear_status (LigmaTool *tool)
{
  g_return_if_fail (LIGMA_IS_TOOL (tool));

  while (tool->status_displays)
    ligma_tool_pop_status (tool, tool->status_displays->data);
}

static void
ligma_tool_release (LigmaTool *tool)
{
  if (tool->last_pointer_state &&
      ligma_tool_control_is_active (tool->control))
    {
      ligma_tool_button_release (
        tool,
        &tool->last_pointer_coords,
        tool->last_pointer_time + g_get_monotonic_time () / 1000,
        tool->last_pointer_state,
        tool->display);
    }
}
