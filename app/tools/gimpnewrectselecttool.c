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

#ifdef __GNUC__
#warning FIXME #include "dialogs/dialogs-types.h"
#endif
#include "dialogs/dialogs-types.h"
#include "dialogs/info-dialog.h"

#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimpnewrectselecttool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void gimp_new_rect_select_tool_class_init     (GimpNewRectSelectToolClass *klass);
static void gimp_new_rect_select_tool_init           (GimpNewRectSelectTool      *rect_tool);

static void gimp_new_rect_select_tool_modifier_key   (GimpTool                   *tool,
                                                      GdkModifierType             key,
                                                      gboolean                    press,
                                                      GdkModifierType             state,
                                                      GimpDisplay                *gdisp);
static void gimp_new_rect_select_tool_cursor_update  (GimpTool                   *tool,
                                                      GimpCoords                 *coords,
                                                      GdkModifierType             state,
                                                      GimpDisplay                *gdisp);

static void gimp_new_rect_select_tool_execute        (GimpRectangleTool          *rect_tool,
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

static void
gimp_new_rect_select_tool_execute (GimpRectangleTool  *rect_tool,
                                   gint                x,
                                   gint                y,
                                   gint                w,
                                   gint                h)
{
  GimpTool             *tool     = GIMP_TOOL (rect_tool);
  GimpSelectionTool    *sel_tool = GIMP_SELECTION_TOOL (rect_tool);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_rectangle (gimp_image_get_mask (tool->gdisp->gimage),
                                 x, y, w, h,
                                 options->operation,
                                 options->feather,
                                 options->feather_radius,
                                 options->feather_radius);
}

