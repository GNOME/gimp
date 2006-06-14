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

#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel-select.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"
#include "core/gimpundostack.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpbycolorselecttool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static GObject * gimp_by_color_select_tool_constructor (GType                  type,
                                                        guint                  n_params,
                                                        GObjectConstructParam *params);

static void   gimp_by_color_select_tool_dispose        (GObject         *object);
static void   gimp_by_color_select_tool_button_press   (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_button_release (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        guint32          time,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_oper_update    (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        GdkModifierType  state,
                                                        gboolean         proximity,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_cursor_update  (GimpTool        *tool,
                                                        GimpCoords      *coords,
                                                        GdkModifierType  state,
                                                        GimpDisplay     *display);
static void   gimp_by_color_select_tool_select           (GimpByColorSelectTool *by_color_sel);
static void   gimp_by_color_select_tool_threshold_notify (GimpSelectionOptions  *options,
                                                          GParamSpec            *pspec,
                                                          GimpByColorSelectTool *by_color_sel);

G_DEFINE_TYPE (GimpByColorSelectTool, gimp_by_color_select_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_by_color_select_tool_parent_class


void
gimp_by_color_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_BY_COLOR_SELECT_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-by-color-select-tool",
                _("Select by Color"),
                _("Select regions by color"),
                N_("_By Color Select"), "<shift>O",
                NULL, GIMP_HELP_TOOL_BY_COLOR_SELECT,
                GIMP_STOCK_TOOL_BY_COLOR_SELECT,
                data);
}

static void
gimp_by_color_select_tool_class_init (GimpByColorSelectToolClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpToolClass *tool_class   = GIMP_TOOL_CLASS (klass);

  object_class->constructor  = gimp_by_color_select_tool_constructor;
  object_class->dispose      = gimp_by_color_select_tool_dispose;

  tool_class->button_press   = gimp_by_color_select_tool_button_press;
  tool_class->button_release = gimp_by_color_select_tool_button_release;
  tool_class->oper_update    = gimp_by_color_select_tool_oper_update;
  tool_class->cursor_update  = gimp_by_color_select_tool_cursor_update;
}

static void
gimp_by_color_select_tool_init (GimpByColorSelectTool *by_color_select)
{
  GimpTool *tool = GIMP_TOOL (by_color_select);

  gimp_tool_control_set_cursor (tool->control,      GIMP_CURSOR_MOUSE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_HAND);

  by_color_select->x    = 0;
  by_color_select->y    = 0;

  by_color_select->undo = NULL;
}

static GObject *
gimp_by_color_select_tool_constructor (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject  *object;
  GObject  *options;
  GimpTool *tool;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool = GIMP_TOOL (object);

  tool->display = NULL;

  options = G_OBJECT (tool->tool_info->tool_options);
  g_signal_connect_object (options, "notify::threshold",
                           G_CALLBACK (gimp_by_color_select_tool_threshold_notify),
                           GIMP_BY_COLOR_SELECT_TOOL (tool), 0);

  return object;
}

static void
gimp_by_color_select_tool_dispose (GObject *object)
{
  GimpTool          *tool      = GIMP_TOOL (object);
  GObject           *options;

  options = G_OBJECT (tool->tool_info->tool_options);

  tool->display = NULL;

  g_signal_handlers_disconnect_by_func
    (options,
     G_CALLBACK (gimp_by_color_select_tool_threshold_notify),
     GIMP_BY_COLOR_SELECT_TOOL (object));
}

static void
gimp_by_color_select_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  GimpByColorSelectTool *by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);
  GimpSelectionOptions  *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  tool->drawable = gimp_image_active_drawable (display->image);

  gimp_tool_control_activate (tool->control);
  tool->display = display;

  by_color_sel->x = coords->x;
  by_color_sel->y = coords->y;

  if (! options->sample_merged)
    {
      gint off_x, off_y;

      gimp_item_offsets (GIMP_ITEM (gimp_image_active_drawable (display->image)),
                         &off_x, &off_y);

      by_color_sel->x -= off_x;
      by_color_sel->y -= off_y;
    }

  by_color_sel->undo = NULL;
}

static void
gimp_by_color_select_tool_button_release (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  GimpByColorSelectTool *by_color_sel = GIMP_BY_COLOR_SELECT_TOOL (tool);
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions  *options;
  GimpDrawable          *drawable;

  options  = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);
  drawable = gimp_image_active_drawable (display->image);

  gimp_tool_control_halt (tool->control);

  /*  First take care of the case where the user "cancels" the action  */
  if (state & GDK_BUTTON3_MASK)
    return;

  by_color_sel->op = sel_tool->op;

  gimp_by_color_select_tool_select (by_color_sel);
}

static void
gimp_by_color_select_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       gboolean         proximity,
                                       GimpDisplay     *display)
{
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (tool);
  GimpSelectionOptions *options;

  if (gimp_tool_control_is_active (tool->control))
    return;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if ((state & GDK_CONTROL_MASK) && (state & GDK_SHIFT_MASK))
    {
      sel_tool->op = SELECTION_INTERSECT; /* intersect with selection */
    }
  else if (state & GDK_SHIFT_MASK)
    {
      sel_tool->op = SELECTION_ADD;   /* add to the selection */
    }
  else if (state & GDK_CONTROL_MASK)
    {
      sel_tool->op = SELECTION_SUBTRACT;   /* subtract from the selection */
    }
  else
    {
      sel_tool->op = options->operation;
    }
}

static void
gimp_by_color_select_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpSelectionOptions *options;
  GimpCursorModifier    modifier = GIMP_CURSOR_MODIFIER_NONE;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (! gimp_image_coords_in_active_pickable (display->image, coords,
                                              options->sample_merged, FALSE))
    modifier = GIMP_CURSOR_MODIFIER_BAD;

  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}


static void
gimp_by_color_select_tool_threshold_notify (GimpSelectionOptions  *options,
                                            GParamSpec            *pspec,
                                            GimpByColorSelectTool *by_color_sel)
{
  GimpTool    *tool     = GIMP_TOOL (by_color_sel);
  GimpDisplay *display;
  GimpImage   *image;
  GimpUndo    *undo     = by_color_sel->undo;

  display = tool->display;

  if (!display)
    return;

  image = display->image;

  /* don't do anything unless we have already done something */
  if (!undo)
    return;

  if (undo && undo == gimp_undo_stack_peek (image->undo_stack))
    {
      gimp_image_undo (image);
      by_color_sel->undo = NULL;
    }

  gimp_by_color_select_tool_select (by_color_sel);
}

static void
gimp_by_color_select_tool_select (GimpByColorSelectTool *by_color_sel)
{
  GimpTool              *tool      = GIMP_TOOL (by_color_sel);
  GimpDisplay           *display;
  GimpDrawable          *drawable;
  GimpSelectionOptions  *options;
  GimpImage             *image;

  display = tool->display;
  image = display->image;
  drawable = gimp_image_active_drawable (image);
  options  = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (by_color_sel->x >= 0 &&
      by_color_sel->y >= 0 &&
      by_color_sel->x < gimp_item_width  (GIMP_ITEM (drawable)) &&
      by_color_sel->y < gimp_item_height (GIMP_ITEM (drawable)))
    {
      GimpPickable *pickable;
      guchar       *col;

      if (options->sample_merged)
        pickable = GIMP_PICKABLE (image->projection);
      else
        pickable = GIMP_PICKABLE (drawable);

      gimp_pickable_flush (pickable);

      col = gimp_pickable_get_color_at (pickable,
                                        by_color_sel->x,
                                        by_color_sel->y);

      if (col)
        {
          GimpRGB color;

          gimp_rgba_set_uchar (&color, col[0], col[1], col[2], col[3]);
          g_free (col);

          gimp_channel_select_by_color (gimp_image_get_mask (image),
                                        drawable,
                                        options->sample_merged,
                                        &color,
                                        options->threshold,
                                        options->select_transparent,
                                        by_color_sel->op,
                                        options->antialias,
                                        options->feather,
                                        options->feather_radius,
                                        options->feather_radius);


          by_color_sel->undo = gimp_undo_stack_peek (image->undo_stack);

          gimp_image_flush (image);
        }
    }
}
