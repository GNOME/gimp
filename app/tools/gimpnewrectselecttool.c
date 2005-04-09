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
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimp-utils.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimpnewrectselecttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void gimp_new_rect_select_tool_class_init     (GimpNewRectSelectToolClass *klass);
static void gimp_new_rect_select_tool_init           (GimpNewRectSelectTool      *rect_tool);

static void gimp_new_rect_select_tool_button_press   (GimpTool                   *tool,
                                                      GimpCoords                 *coords,
                                                      guint32                     time,
                                                      GdkModifierType             state,
                                                      GimpDisplay                *gdisp);
static void gimp_new_rect_select_tool_modifier_key   (GimpTool                   *tool,
                                                      GdkModifierType             key,
                                                      gboolean                    press,
                                                      GdkModifierType             state,
                                                      GimpDisplay                *gdisp);
static void gimp_new_rect_select_tool_cursor_update  (GimpTool                   *tool,
                                                      GimpCoords                 *coords,
                                                      GdkModifierType             state,
                                                      GimpDisplay                *gdisp);

static gboolean gimp_new_rect_select_tool_execute    (GimpRectangleTool          *rect_tool,
                                                      gint                        x,
                                                      gint                        y,
                                                      gint                        w,
                                                      gint                        h);

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_new_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_NEW_RECT_SELECT_TOOL,
                GIMP_TYPE_RECTANGLE_OPTIONS,
                gimp_rectangle_options_gui,
                0,
                "gimp-new-rect-select-tool",
                _("New Rect Select"),
                _("Select a Rectangular part of an image"),
                N_("_New Rect Select"), "<shift>C",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_STOCK_TOOL_RECT_SELECT,
                data);
}

GType
gimp_new_rect_select_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpNewRectSelectToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_new_rect_select_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpNewRectSelectTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_new_rect_select_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_RECTANGLE_TOOL,
                                          "GimpNewRectSelectTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_new_rect_select_tool_class_init (GimpNewRectSelectToolClass *klass)
{
  GimpToolClass          *tool_class  = GIMP_TOOL_CLASS (klass);
  GimpRectangleToolClass *rect_class  = GIMP_RECTANGLE_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  tool_class->button_press   = gimp_new_rect_select_tool_button_press;
  tool_class->modifier_key   = gimp_new_rect_select_tool_modifier_key;
  tool_class->cursor_update  = gimp_new_rect_select_tool_cursor_update;

  rect_class->execute  = gimp_new_rect_select_tool_execute;
}

static void
gimp_new_rect_select_tool_init (GimpNewRectSelectTool *new_rect_select_tool)
{
  GimpTool          *tool      = GIMP_TOOL (new_rect_select_tool);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (new_rect_select_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RECT_SELECT);

  rectangle->selection_tool = TRUE;
}

static void
gimp_new_rect_select_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *gdisp)
{
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords,
                                                time, state, gdisp);

  if (gimp_selection_tool_start_edit (sel_tool, coords))
    {
      gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
      return;
    }

  switch (sel_tool->op)
    {
    case SELECTION_ADD:
      gimp_tool_push_status (tool, _("Selection: ADD"));
      break;
    case SELECTION_SUBTRACT:
      gimp_tool_push_status (tool, _("Selection: SUBTRACT"));
      break;
    case SELECTION_INTERSECT:
      gimp_tool_push_status (tool, _("Selection: INTERSECT"));
      break;
    case SELECTION_REPLACE:
      gimp_tool_push_status (tool, _("Selection: REPLACE"));
      break;
    default:
      break;
    }
}

static void
gimp_new_rect_select_tool_modifier_key (GimpTool        *tool,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state,
                                        GimpDisplay     *gdisp)
{
}

static void
gimp_new_rect_select_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *gdisp)
{
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RECT_SELECT);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, gdisp);
}

/*
 * This function is called if the user clicks and releases the left
 * button without moving it.  There are five things we might want
 * to do here:
 * 1) If there is a floating selection, we anchor it.
 * 2) If there is an existing rectangle and we are inside it, we
 *    convert it into a selection.
 * 3) If there is an existing rectangle and we are outside it, we
 *    clear it.
 * 4) If there is no rectangle and we are inside the selection, we
 *    create a rectangle from the selection bounds.
 * 5) If there is no rectangle and we are outside the selection,
 *    we clear the selection.
 */
static gboolean
gimp_new_rect_select_tool_execute (GimpRectangleTool  *rectangle,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  GimpTool             *tool     = GIMP_TOOL (rectangle);
  GimpSelectionOptions *options;
  GimpImage            *gimage;
  gboolean              rectangle_exists;
  gboolean              selected;
  gint                  val;
  GimpChannel          *selection_mask;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimage = tool->gdisp->gimage;
  selection_mask = gimp_image_get_mask (gimage);

  rectangle_exists = (w > 0 && h > 0);

  /*  If there is a floating selection, anchor it  */
  if (gimp_image_floating_sel (gimage))
    {
      floating_sel_anchor (gimp_image_floating_sel (gimage));
      return FALSE;
    }

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    {
      GimpChannel  *selection_mask = gimp_image_get_mask (gimage);

      gimp_channel_select_rectangle (selection_mask,
                                     x, y, w, h,
                                     options->operation,
                                     options->feather,
                                     options->feather_radius,
                                     options->feather_radius);
      return TRUE;
    }


  val = gimp_channel_value (selection_mask,
                            rectangle->pressx, rectangle->pressy);

  selected = (val > 127);

  /* if point clicked is inside selection, set rectangle to  */
  /* selection bounds.                                       */
  if (selected)
    {
      if (! gimp_channel_bounds (selection_mask,
                                 &rectangle->x1, &rectangle->y1,
                                 &rectangle->x2, &rectangle->y2))
        {
          rectangle->x1 = 0;
          rectangle->y1 = 0;
          rectangle->x2 = gimage->width;
          rectangle->y2 = gimage->height;
        }

      return FALSE;
    }

  /* otherwise clear the selection */
  gimp_channel_clear (selection_mask, NULL, TRUE);

  return TRUE;
}

