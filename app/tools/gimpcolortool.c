/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-pick-color.h"
#include "core/gimpitem.h"
#include "core/gimpmarshal.h"

#include "display/gimpdisplay.h"

#include "gimpcoloroptions.h"
#include "gimpcolortool.h"
#include "gimptoolcontrol.h"


enum
{
  PICKED,
  LAST_SIGNAL
};


/*  local function prototypes  */

static void 	  gimp_color_tool_class_init  (GimpColorToolClass *klass);
static void       gimp_color_tool_init        (GimpColorTool      *color_tool);
static void       gimp_color_tool_finalize    (GObject            *object);

static void       gimp_color_tool_button_press   (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_button_release (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_motion         (GimpTool        *tool,
						  GimpCoords      *coords,
						  guint32          time,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);
static void       gimp_color_tool_cursor_update  (GimpTool        *tool,
						  GimpCoords      *coords,
						  GdkModifierType  state,
						  GimpDisplay     *gdisp);

static void       gimp_color_tool_draw           (GimpDrawTool    *draw_tool);

static gboolean   gimp_color_tool_real_pick      (GimpColorTool   *color_tool,
                                                  gint             x,
                                                  gint             y,
                                                  GimpImageType   *sample_type,
                                                  GimpRGB         *color,
                                                  gint            *color_index);
static void       gimp_color_tool_pick           (GimpColorTool   *tool,
                                                  GimpColorPickState  pick_state,
                                                  gint             x,
                                                  gint             y);


static guint gimp_color_tool_signals[LAST_SIGNAL] = { 0 };

static GimpDrawToolClass *parent_class = NULL;


GtkType
gimp_color_tool_get_type (void)
{
  static GtkType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpColorToolClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_color_tool_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpColorTool),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_color_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_DRAW_TOOL,
					  "GimpColorTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_color_tool_class_init (GimpColorToolClass *klass)
{
  GObjectClass      *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class   = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_class   = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_color_tool_signals[PICKED] =
    g_signal_new ("picked",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpColorToolClass, picked),
		  NULL, NULL,
		  gimp_marshal_VOID__ENUM_ENUM_BOXED_INT,
		  G_TYPE_NONE, 4,
                  GIMP_TYPE_COLOR_PICK_STATE,
                  GIMP_TYPE_IMAGE_TYPE,
                  GIMP_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE,
                  G_TYPE_INT);

  object_class->finalize     = gimp_color_tool_finalize;

  tool_class->button_press   = gimp_color_tool_button_press;
  tool_class->button_release = gimp_color_tool_button_release;
  tool_class->motion         = gimp_color_tool_motion;
  tool_class->cursor_update  = gimp_color_tool_cursor_update;

  draw_class->draw	     = gimp_color_tool_draw;

  klass->pick                = gimp_color_tool_real_pick;
  klass->picked              = NULL;
}

static void
gimp_color_tool_finalize (GObject *object)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (object);

  if (color_tool->options)
    {
      g_object_unref (color_tool->options);
      color_tool->options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_tool_init (GimpColorTool *color_tool)
{
  color_tool->enabled   = FALSE;
  color_tool->center_x  = 0;
  color_tool->center_y  = 0;
  color_tool->pick_mode = GIMP_COLOR_PICK_MODE_NONE;
  color_tool->options   = NULL;
}

static void
gimp_color_tool_button_press (GimpTool        *tool,
			      GimpCoords      *coords,
			      guint32          time,
			      GdkModifierType  state,
			      GimpDisplay     *gdisp)
{
  GimpColorTool *color_tool;

  /*  Make the tool active and set it's gdisplay & drawable  */
  tool->gdisp    = gdisp;
  tool->drawable = gimp_image_active_drawable (gdisp->gimage);
  gimp_tool_control_activate (tool->control);

  color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      gint off_x, off_y;

      /*  Keep the coordinates of the target  */
      gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

      color_tool->center_x = coords->x - off_x;
      color_tool->center_y = coords->y - off_y;

      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), gdisp);

      gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_NEW,
                            coords->x, coords->y);
    }
}

static void
gimp_color_tool_button_release (GimpTool        *tool,
				GimpCoords      *coords,
				guint32          time,
				GdkModifierType  state,
				GimpDisplay     *gdisp)
{
  if (GIMP_COLOR_TOOL (tool)->enabled)
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);
}

static void
gimp_color_tool_motion (GimpTool        *tool,
                        GimpCoords      *coords,
                        guint32          time,
                        GdkModifierType  state,
                        GimpDisplay     *gdisp)
{
  GimpColorTool *color_tool;
  gint           off_x, off_y;

  color_tool = GIMP_COLOR_TOOL (tool);

  if (! color_tool->enabled)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_item_offsets (GIMP_ITEM (tool->drawable), &off_x, &off_y);

  color_tool->center_x = coords->x - off_x;
  color_tool->center_y = coords->y - off_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_color_tool_pick (color_tool, GIMP_COLOR_PICK_STATE_UPDATE,
                        coords->x, coords->y);
}

static void
gimp_color_tool_cursor_update (GimpTool        *tool,
			       GimpCoords      *coords,
			       GdkModifierType  state,
			       GimpDisplay     *gdisp)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (tool);

  if (color_tool->enabled)
    {
      GimpCursorType     cursor   = GIMP_CURSOR_BAD;
      GimpCursorModifier modifier = GIMP_CURSOR_MODIFIER_NONE;

      if (coords->x > 0 && coords->x < gdisp->gimage->width  &&
          coords->y > 0 && coords->y < gdisp->gimage->height &&

          (color_tool->options->sample_merged ||
           gimp_image_coords_in_active_drawable (gdisp->gimage, coords)))
        {
          cursor = GIMP_CURSOR_COLOR_PICKER;
        }

      switch (color_tool->pick_mode)
        {
        case GIMP_COLOR_PICK_MODE_NONE:
          modifier = GIMP_CURSOR_MODIFIER_NONE;
          break;
        case GIMP_COLOR_PICK_MODE_FOREGROUND:
          modifier = GIMP_CURSOR_MODIFIER_FOREGROUND;
          break;
        case GIMP_COLOR_PICK_MODE_BACKGROUND:
          modifier = GIMP_CURSOR_MODIFIER_BACKGROUND;
          break;
        }

      gimp_tool_set_cursor (tool, gdisp,
                            cursor, GIMP_TOOL_CURSOR_COLOR_PICKER, modifier);

      return;  /*  don't chain up  */
    }

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

static void
gimp_color_tool_draw (GimpDrawTool *draw_tool)
{
  GimpColorTool *color_tool = GIMP_COLOR_TOOL (draw_tool);

  if (! color_tool->enabled)
    return;

  if (color_tool->options->sample_average)
    {
      gdouble radius = color_tool->options->average_radius;

      gimp_draw_tool_draw_rectangle (draw_tool,
                                     FALSE,
                                     color_tool->center_x - radius,
                                     color_tool->center_y - radius,
                                     2 * radius + 1,
                                     2 * radius + 1,
                                     TRUE);
    }
}

static gboolean
gimp_color_tool_real_pick (GimpColorTool *color_tool,
                           gint           x,
                           gint           y,
                           GimpImageType *sample_type,
                           GimpRGB       *color,
                           gint          *color_index)
{
  GimpTool *tool = GIMP_TOOL (color_tool);

  g_return_val_if_fail (tool->gdisp != NULL, FALSE);
  g_return_val_if_fail (tool->drawable != NULL, FALSE);

  return gimp_image_pick_color (tool->gdisp->gimage, tool->drawable, x, y,
                                color_tool->options->sample_merged,
                                color_tool->options->sample_average,
                                color_tool->options->average_radius,
                                sample_type,
                                color,
                                color_index);
}

static void
gimp_color_tool_pick (GimpColorTool      *tool,
                      GimpColorPickState  pick_state,
		      gint                x,
		      gint                y)
{
  GimpColorToolClass *klass;
  GimpImageType       sample_type;
  GimpRGB             color;
  gint                color_index;

  klass = GIMP_COLOR_TOOL_GET_CLASS (tool);

  if (klass->pick &&
      klass->pick (tool, x, y, &sample_type, &color, &color_index))
    {
      g_signal_emit (tool, gimp_color_tool_signals[PICKED], 0,
                     pick_state, sample_type, &color, color_index);
    }
}


void
gimp_color_tool_enable (GimpColorTool    *color_tool,
                        GimpColorOptions *options)
{
  GimpTool *tool;

  g_return_if_fail (GIMP_IS_COLOR_TOOL (color_tool));
  g_return_if_fail (GIMP_IS_COLOR_OPTIONS (options));

  tool = GIMP_TOOL (color_tool);
  if (gimp_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to enable GimpColorTool while it is active.");
      return;
    }

  if (color_tool->options)
    {
      g_object_unref (color_tool->options);
      color_tool->options = NULL;
    }
  color_tool->options = g_object_ref (options);

  color_tool->enabled = TRUE;
}

void
gimp_color_tool_disable (GimpColorTool *color_tool)
{
  GimpTool *tool;

  g_return_if_fail (GIMP_IS_COLOR_TOOL (color_tool));

  tool = GIMP_TOOL (color_tool);
  if (gimp_tool_control_is_active (tool->control))
    {
      g_warning ("Trying to disable GimpColorTool while it is active.");
      return;
    }

  if (color_tool->options)
    {
      g_object_unref (color_tool->options);
      color_tool->options = NULL;
    }

  color_tool->enabled = FALSE;
}

gboolean
gimp_color_tool_is_enabled (GimpColorTool *color_tool)
{
  return color_tool->enabled;
}
