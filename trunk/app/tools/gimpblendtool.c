/* GIMP - The GNU Image Manipulation Program
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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpdrawable-blend.h"
#include "core/gimpgradient.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpblendoptions.h"
#include "gimpblendtool.h"
#include "gimptoolcontrol.h"
#include "tools-utils.h"

#include "gimp-intl.h"


#define TARGET_SIZE 15


/*  local function prototypes  */

static gboolean gimp_blend_tool_initialize        (GimpTool              *tool,
                                                   GimpDisplay           *display,
                                                   GError               **error);
static void   gimp_blend_tool_button_press        (GimpTool              *tool,
                                                   GimpCoords            *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_button_release      (GimpTool              *tool,
                                                   GimpCoords            *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpButtonReleaseType  release_type,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_motion              (GimpTool              *tool,
                                                   GimpCoords            *coords,
                                                   guint32                time,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_active_modifier_key (GimpTool              *tool,
                                                   GdkModifierType        key,
                                                   gboolean               press,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);
static void   gimp_blend_tool_cursor_update       (GimpTool              *tool,
                                                   GimpCoords            *coords,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);

static void   gimp_blend_tool_draw                (GimpDrawTool          *draw_tool);

static void   gimp_blend_tool_push_status         (GimpBlendTool         *blend_tool,
                                                   GdkModifierType        state,
                                                   GimpDisplay           *display);


G_DEFINE_TYPE (GimpBlendTool, gimp_blend_tool, GIMP_TYPE_DRAW_TOOL)

#define parent_class gimp_blend_tool_parent_class


void
gimp_blend_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_BLEND_TOOL,
                GIMP_TYPE_BLEND_OPTIONS,
                gimp_blend_options_gui,
                GIMP_CONTEXT_FOREGROUND_MASK |
                GIMP_CONTEXT_BACKGROUND_MASK |
                GIMP_CONTEXT_OPACITY_MASK    |
                GIMP_CONTEXT_PAINT_MODE_MASK |
                GIMP_CONTEXT_GRADIENT_MASK,
                "gimp-blend-tool",
                _("Blend"),
                _("Blend Tool: Fill selected area with a color gradient"),
                N_("Blen_d"), "L",
                NULL, GIMP_HELP_TOOL_BLEND,
                GIMP_STOCK_TOOL_BLEND,
                data);
}

static void
gimp_blend_tool_class_init (GimpBlendToolClass *klass)
{
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  tool_class->initialize          = gimp_blend_tool_initialize;
  tool_class->button_press        = gimp_blend_tool_button_press;
  tool_class->button_release      = gimp_blend_tool_button_release;
  tool_class->motion              = gimp_blend_tool_motion;
  tool_class->active_modifier_key = gimp_blend_tool_active_modifier_key;
  tool_class->cursor_update       = gimp_blend_tool_cursor_update;

  draw_tool_class->draw           = gimp_blend_tool_draw;
}

static void
gimp_blend_tool_init (GimpBlendTool *blend_tool)
{
  GimpTool *tool = GIMP_TOOL (blend_tool);

  gimp_tool_control_set_scroll_lock     (tool->control, TRUE);
  gimp_tool_control_set_cursor          (tool->control,
                                         GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor     (tool->control,
                                         GIMP_TOOL_CURSOR_BLEND);
  gimp_tool_control_set_action_value_1  (tool->control,
                                         "context/context-opacity-set");
  gimp_tool_control_set_action_object_1 (tool->control,
                                         "context/context-gradient-select-set");
}

static gboolean
gimp_blend_tool_initialize (GimpTool     *tool,
                            GimpDisplay  *display,
                            GError      **error)
{
  GimpDrawable *drawable = gimp_image_get_active_drawable (display->image);

  if (! GIMP_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      return FALSE;
    }

  if (gimp_drawable_is_indexed (drawable))
    {
      g_set_error (error, 0, 0,
                   _("Blend does not operate on indexed layers."));
      return FALSE;
    }

  return TRUE;
}

static void
gimp_blend_tool_button_press (GimpTool        *tool,
                              GimpCoords      *coords,
                              guint32          time,
                              GdkModifierType  state,
                              GimpDisplay     *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  blend_tool->start_x = blend_tool->end_x = coords->x;
  blend_tool->start_y = blend_tool->end_y = coords->y;

  blend_tool->last_x = blend_tool->mouse_x = coords->x;
  blend_tool->last_y = blend_tool->mouse_y = coords->y;

  tool->display = display;

  gimp_tool_control_activate (tool->control);

  gimp_blend_tool_push_status (blend_tool, state, display);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
}

static void
gimp_blend_tool_button_release (GimpTool              *tool,
                                GimpCoords            *coords,
                                guint32                time,
                                GdkModifierType        state,
                                GimpButtonReleaseType  release_type,
                                GimpDisplay           *display)
{
  GimpBlendTool    *blend_tool    = GIMP_BLEND_TOOL (tool);
  GimpBlendOptions *options       = GIMP_BLEND_TOOL_GET_OPTIONS (tool);
  GimpPaintOptions *paint_options = GIMP_PAINT_OPTIONS (options);
  GimpContext      *context       = GIMP_CONTEXT (options);
  GimpImage        *image         = display->image;

  gimp_tool_pop_status (tool, display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  if ((release_type != GIMP_BUTTON_RELEASE_CANCEL) &&
      ((blend_tool->start_x != blend_tool->end_x) ||
       (blend_tool->start_y != blend_tool->end_y)))
    {
      GimpDrawable *drawable = gimp_image_get_active_drawable (image);
      GimpProgress *progress;
      gint          off_x;
      gint          off_y;

      progress = gimp_progress_start (GIMP_PROGRESS (display),
                                      _("Blending"), FALSE);

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);

      gimp_drawable_blend (drawable,
                           context,
                           GIMP_CUSTOM_MODE,
                           gimp_context_get_paint_mode (context),
                           options->gradient_type,
                           gimp_context_get_opacity (context),
                           options->offset,
                           paint_options->gradient_options->gradient_repeat,
                           paint_options->gradient_options->gradient_reverse,
                           options->supersample,
                           options->supersample_depth,
                           options->supersample_threshold,
                           options->dither,
                           blend_tool->start_x - off_x,
                           blend_tool->start_y - off_y,
                           blend_tool->end_x - off_x,
                           blend_tool->end_y - off_y,
                           progress);

      if (progress)
        gimp_progress_end (progress);

      gimp_image_flush (image);
    }
}

static void
gimp_blend_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  blend_tool->mouse_x = coords->x;
  blend_tool->mouse_y = coords->y;

  /* Move the whole line if alt is pressed */
  if (state & GDK_MOD1_MASK)
    {
      gdouble dx = blend_tool->last_x - coords->x;
      gdouble dy = blend_tool->last_y - coords->y;

      blend_tool->start_x -= dx;
      blend_tool->start_y -= dy;

      blend_tool->end_x -= dx;
      blend_tool->end_y -= dy;
    }
  else
    {
      blend_tool->end_x = coords->x;
      blend_tool->end_y = coords->y;
    }

  /* Restrict to multiples of 15 degrees if ctrl is pressed */
  if (state & GDK_CONTROL_MASK)
    {
      gimp_tool_motion_constrain (blend_tool->start_x, blend_tool->start_y,
                                  &blend_tool->end_x, &blend_tool->end_y);
    }

  gimp_tool_pop_status (tool, display);
  gimp_blend_tool_push_status (blend_tool, state, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  blend_tool->last_x = coords->x;
  blend_tool->last_y = coords->y;
}

static void
gimp_blend_tool_active_modifier_key (GimpTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (tool);

  if (key == GDK_CONTROL_MASK)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      blend_tool->end_x = blend_tool->mouse_x;
      blend_tool->end_y = blend_tool->mouse_y;

      /* Restrict to multiples of 15 degrees if ctrl is pressed */
      if (press)
        {
          gimp_tool_motion_constrain (blend_tool->start_x, blend_tool->start_y,
                                      &blend_tool->end_x, &blend_tool->end_y);
        }

      gimp_tool_pop_status (tool, display);
      gimp_blend_tool_push_status (blend_tool, state, display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

static void
gimp_blend_tool_cursor_update (GimpTool        *tool,
                               GimpCoords      *coords,
                               GdkModifierType  state,
                               GimpDisplay     *display)
{
  switch (gimp_drawable_type (gimp_image_get_active_drawable (display->image)))
    {
    case GIMP_INDEXED_IMAGE:
    case GIMP_INDEXEDA_IMAGE:
      gimp_tool_control_set_cursor_modifier (tool->control,
                                             GIMP_CURSOR_MODIFIER_BAD);
      break;
    default:
      gimp_tool_control_set_cursor_modifier (tool->control,
                                             GIMP_CURSOR_MODIFIER_NONE);
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}

static void
gimp_blend_tool_draw (GimpDrawTool *draw_tool)
{
  GimpBlendTool *blend_tool = GIMP_BLEND_TOOL (draw_tool);

  /*  Draw start target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->start_x) + 0.5,
                              floor (blend_tool->start_y) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);

  /*  Draw end target  */
  gimp_draw_tool_draw_handle (draw_tool,
                              GIMP_HANDLE_CROSS,
                              floor (blend_tool->end_x) + 0.5,
                              floor (blend_tool->end_y) + 0.5,
                              TARGET_SIZE,
                              TARGET_SIZE,
                              GTK_ANCHOR_CENTER,
                              FALSE);

  /*  Draw the line between the start and end coords  */
  gimp_draw_tool_draw_line (draw_tool,
                            floor (blend_tool->start_x) + 0.5,
                            floor (blend_tool->start_y) + 0.5,
                            floor (blend_tool->end_x) + 0.5,
                            floor (blend_tool->end_y) + 0.5,
                            FALSE);
}

static void
gimp_blend_tool_push_status (GimpBlendTool   *blend_tool,
                             GdkModifierType  state,
                             GimpDisplay     *display)
{
  gchar *status_help;

  status_help = gimp_suggest_modifiers ("",
                                        ((GDK_CONTROL_MASK | GDK_MOD1_MASK)
                                         & ~state),
                                        NULL,
                                        _("%s for constrained angles"),
                                        _("%s to move the whole line"));
  gimp_tool_push_status_coords (GIMP_TOOL (blend_tool), display,
                                _("Blend: "),
                                blend_tool->end_x - blend_tool->start_x,
                                ", ",
                                blend_tool->end_y - blend_tool->start_y,
                                status_help);
  g_free (status_help);
}
