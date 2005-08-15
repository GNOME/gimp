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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
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
#include "gimpnewrectselectoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


static void gimp_new_rect_select_tool_class_init     (GimpNewRectSelectToolClass *klass);
static void gimp_new_rect_select_tool_init           (GimpNewRectSelectTool      *rect_tool);
static void gimp_new_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *rectangle_iface);

static GObject * gimp_new_rect_select_tool_constructor (GType                       type,
                                                        guint                       n_params,
                                                        GObjectConstructParam      *params);
static void gimp_new_rect_select_tool_finalize       (GObject         *object);
static void gimp_new_rect_select_tool_control        (GimpTool        *tool,
                                                      GimpToolAction   action,
                                                      GimpDisplay     *gdisp);
static void gimp_new_rect_select_tool_oper_update    (GimpTool        *tool,
                                                      GimpCoords      *coords,
                                                      GdkModifierType  state,
                                                      GimpDisplay     *gdisp);
static gboolean gimp_new_rect_select_tool_execute    (GimpRectangleTool          *rect_tool,
                                                      gint                        x,
                                                      gint                        y,
                                                      gint                        w,
                                                      gint                        h);

void        gimp_new_rect_select_tool_set_controls   (GimpRectangleTool *tool,
                                                      GtkWidget         *controls);
GtkWidget * gimp_new_rect_select_tool_get_controls   (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_entry      (GimpRectangleTool *tool,
                                                      GtkWidget         *entry);
GtkWidget * gimp_new_rect_select_tool_get_entry      (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_startx     (GimpRectangleTool *tool,
                                                      gint               startx);
gint        gimp_new_rect_select_tool_get_startx     (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_starty     (GimpRectangleTool *tool,
                                                      gint               starty);
gint        gimp_new_rect_select_tool_get_starty     (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_lastx      (GimpRectangleTool *tool,
                                                      gint               lastx);
gint        gimp_new_rect_select_tool_get_lastx      (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_lasty      (GimpRectangleTool *tool,
                                                      gint               lasty);
gint        gimp_new_rect_select_tool_get_lasty      (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_pressx     (GimpRectangleTool *tool,
                                                      gint               pressx);
gint        gimp_new_rect_select_tool_get_pressx     (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_pressy     (GimpRectangleTool *tool,
                                                      gint               pressy);
gint        gimp_new_rect_select_tool_get_pressy     (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_x1         (GimpRectangleTool *tool,
                                                      gint               x1);
gint        gimp_new_rect_select_tool_get_x1         (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_y1         (GimpRectangleTool *tool,
                                                      gint               y1);
gint        gimp_new_rect_select_tool_get_y1         (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_x2         (GimpRectangleTool *tool,
                                                      gint               x2);
gint        gimp_new_rect_select_tool_get_x2         (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_y2         (GimpRectangleTool *tool,
                                                      gint               y2);
gint        gimp_new_rect_select_tool_get_y2         (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_function   (GimpRectangleTool *tool,
                                                      guint              function);
guint       gimp_new_rect_select_tool_get_function   (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_dx1        (GimpRectangleTool *tool,
                                                      gint               dx1);
gint        gimp_new_rect_select_tool_get_dx1        (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_dy1        (GimpRectangleTool *tool,
                                                      gint               dy1);
gint        gimp_new_rect_select_tool_get_dy1        (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_dx2        (GimpRectangleTool *tool,
                                                      gint               dx2);
gint        gimp_new_rect_select_tool_get_dx2        (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_dy2        (GimpRectangleTool *tool,
                                                      gint               dy2);
gint        gimp_new_rect_select_tool_get_dy2        (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_dcw        (GimpRectangleTool *tool,
                                                      gint               dcw);
gint        gimp_new_rect_select_tool_get_dcw        (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_dch        (GimpRectangleTool *tool,
                                                      gint               dch);
gint        gimp_new_rect_select_tool_get_dch        (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_origx      (GimpRectangleTool *tool,
                                                      gdouble            origx);
gdouble     gimp_new_rect_select_tool_get_origx      (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_origy      (GimpRectangleTool *tool,
                                                      gdouble            origy);
gdouble     gimp_new_rect_select_tool_get_origy      (GimpRectangleTool *tool);

void        gimp_new_rect_select_tool_set_sizew      (GimpRectangleTool *tool,
                                                      gdouble            sizew);
gdouble     gimp_new_rect_select_tool_get_sizew      (GimpRectangleTool *tool);
void        gimp_new_rect_select_tool_set_sizeh      (GimpRectangleTool *tool,
                                                      gdouble            sizeh);
gdouble     gimp_new_rect_select_tool_get_sizeh      (GimpRectangleTool *tool);

static GimpSelectionToolClass *parent_class = NULL;


/*  public functions  */

void
gimp_new_rect_select_tool_register (GimpToolRegisterCallback  callback,
                                    gpointer                  data)
{
  (* callback) (GIMP_TYPE_NEW_RECT_SELECT_TOOL,
                GIMP_TYPE_NEW_RECT_SELECT_OPTIONS,
                gimp_new_rect_select_options_gui,
                0,
                "gimp-new-rect-select-tool",
                _("New Rect Select"),
                _("Select a Rectangular part of an image"),
                N_("_New Rect Select"), NULL,
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
      static const GInterfaceInfo rectangle_info =
      {
        (GInterfaceInitFunc) gimp_new_rect_select_tool_rectangle_tool_iface_init,           /* interface_init */
        NULL,           /* interface_finalize */
        NULL            /* interface_data */
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
                                          "GimpNewRectSelectTool",
                                          &tool_info, 0);
      g_type_add_interface_static (tool_type,
                                   GIMP_TYPE_RECTANGLE_TOOL,
                                   &rectangle_info);
    }

  return tool_type;
}

static void
gimp_new_rect_select_tool_class_init (GimpNewRectSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_new_rect_select_tool_constructor;
  object_class->finalize     = gimp_new_rect_select_tool_finalize;

  tool_class->initialize     = gimp_rectangle_tool_initialize;
  tool_class->control        = gimp_new_rect_select_tool_control;
  tool_class->button_press   = gimp_rectangle_tool_button_press;
  tool_class->button_release = gimp_rectangle_tool_button_release;
  tool_class->motion         = gimp_rectangle_tool_motion;
  tool_class->key_press      = gimp_rectangle_tool_key_press;
  tool_class->modifier_key   = gimp_rectangle_tool_modifier_key;
  tool_class->oper_update    = gimp_new_rect_select_tool_oper_update;
  tool_class->cursor_update  = gimp_rectangle_tool_cursor_update;

  draw_tool_class->draw      = gimp_rectangle_tool_draw;
}

static void
gimp_new_rect_select_tool_init (GimpNewRectSelectTool *new_rect_select_tool)
{
  GimpTool          *tool      = GIMP_TOOL (new_rect_select_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RECT_SELECT);
}

static void
gimp_new_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *rectangle_iface)
{
  rectangle_iface->set_controls = gimp_new_rect_select_tool_set_controls;
  rectangle_iface->get_controls = gimp_new_rect_select_tool_get_controls;
  rectangle_iface->set_entry    = gimp_new_rect_select_tool_set_entry;
  rectangle_iface->get_entry    = gimp_new_rect_select_tool_get_entry;
  rectangle_iface->set_startx   = gimp_new_rect_select_tool_set_startx;
  rectangle_iface->get_startx   = gimp_new_rect_select_tool_get_startx;
  rectangle_iface->set_starty   = gimp_new_rect_select_tool_set_starty;
  rectangle_iface->get_starty   = gimp_new_rect_select_tool_get_starty;
  rectangle_iface->set_lastx    = gimp_new_rect_select_tool_set_lastx;
  rectangle_iface->get_lastx    = gimp_new_rect_select_tool_get_lastx;
  rectangle_iface->set_lasty    = gimp_new_rect_select_tool_set_lasty;
  rectangle_iface->get_lasty    = gimp_new_rect_select_tool_get_lasty;
  rectangle_iface->set_pressx   = gimp_new_rect_select_tool_set_pressx;
  rectangle_iface->get_pressx   = gimp_new_rect_select_tool_get_pressx;
  rectangle_iface->set_pressy   = gimp_new_rect_select_tool_set_pressy;
  rectangle_iface->get_pressy   = gimp_new_rect_select_tool_get_pressy;
  rectangle_iface->set_x1       = gimp_new_rect_select_tool_set_x1;
  rectangle_iface->get_x1       = gimp_new_rect_select_tool_get_x1;
  rectangle_iface->set_y1       = gimp_new_rect_select_tool_set_y1;
  rectangle_iface->get_y1       = gimp_new_rect_select_tool_get_y1;
  rectangle_iface->set_x2       = gimp_new_rect_select_tool_set_x2;
  rectangle_iface->get_x2       = gimp_new_rect_select_tool_get_x2;
  rectangle_iface->set_y2       = gimp_new_rect_select_tool_set_y2;
  rectangle_iface->get_y2       = gimp_new_rect_select_tool_get_y2;
  rectangle_iface->set_function = gimp_new_rect_select_tool_set_function;
  rectangle_iface->get_function = gimp_new_rect_select_tool_get_function;
  rectangle_iface->set_dx1      = gimp_new_rect_select_tool_set_dx1;
  rectangle_iface->get_dx1      = gimp_new_rect_select_tool_get_dx1;
  rectangle_iface->set_dy1      = gimp_new_rect_select_tool_set_dy1;
  rectangle_iface->get_dy1      = gimp_new_rect_select_tool_get_dy1;
  rectangle_iface->set_dx2      = gimp_new_rect_select_tool_set_dx2;
  rectangle_iface->get_dx2      = gimp_new_rect_select_tool_get_dx2;
  rectangle_iface->set_dy2      = gimp_new_rect_select_tool_set_dy2;
  rectangle_iface->get_dy2      = gimp_new_rect_select_tool_get_dy2;
  rectangle_iface->set_dcw      = gimp_new_rect_select_tool_set_dcw;
  rectangle_iface->get_dcw      = gimp_new_rect_select_tool_get_dcw;
  rectangle_iface->set_dch      = gimp_new_rect_select_tool_set_dch;
  rectangle_iface->get_dch      = gimp_new_rect_select_tool_get_dch;
  rectangle_iface->set_origx    = gimp_new_rect_select_tool_set_origx;
  rectangle_iface->get_origx    = gimp_new_rect_select_tool_get_origx;
  rectangle_iface->set_origy    = gimp_new_rect_select_tool_set_origy;
  rectangle_iface->get_origy    = gimp_new_rect_select_tool_get_origy;
  rectangle_iface->set_sizew    = gimp_new_rect_select_tool_set_sizew;
  rectangle_iface->get_sizew    = gimp_new_rect_select_tool_get_sizew;
  rectangle_iface->set_sizeh    = gimp_new_rect_select_tool_set_sizeh;
  rectangle_iface->get_sizeh    = gimp_new_rect_select_tool_get_sizeh;
  rectangle_iface->execute      = gimp_new_rect_select_tool_execute;
}

static GObject *
gimp_new_rect_select_tool_constructor (GType                  type,
                                       guint                  n_params,
                                       GObjectConstructParam *params)
{
  GObject *object;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  gimp_rectangle_tool_constructor (object);

  return object;
}

static void
gimp_new_rect_select_tool_finalize (GObject *object)
{
  gimp_rectangle_tool_finalize (object);
  
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_new_rect_select_tool_control (GimpTool       *tool,
                                   GimpToolAction  action,
                                   GimpDisplay    *gdisp)
{
  gimp_rectangle_tool_control (tool, action, gdisp);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_new_rect_select_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       GimpDisplay     *gdisp)
{
  gimp_rectangle_tool_oper_update (tool, coords, state, gdisp);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);
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
  gint                  x1, y1;
  gint                  x2, y2;

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


  val = gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection_mask),
                                      gimp_rectangle_tool_get_pressx (rectangle), gimp_rectangle_tool_get_pressy (rectangle));

  selected = (val > 127);

  /* if point clicked is inside selection, set rectangle to  */
  /* selection bounds.                                       */
  if (selected)
    {
      if (! gimp_channel_bounds (selection_mask,
                                 &x1, &y1,
                                 &x2, &y2))
        {
          gimp_rectangle_tool_set_x1 (rectangle, x1);
          gimp_rectangle_tool_set_y1 (rectangle, y1);
          gimp_rectangle_tool_set_x2 (rectangle, x2);
          gimp_rectangle_tool_set_y2 (rectangle, y2);
        }
      else
        {
          gimp_rectangle_tool_set_x1 (rectangle, 0);
          gimp_rectangle_tool_set_y1 (rectangle, 0);
          gimp_rectangle_tool_set_x2 (rectangle, gimage->width);
          gimp_rectangle_tool_set_y2 (rectangle, gimage->height);
        }

      return FALSE;
    }

  /* otherwise clear the selection */
  gimp_channel_clear (selection_mask, NULL, TRUE);

  return TRUE;
}

void
gimp_new_rect_select_tool_set_controls (GimpRectangleTool *tool,
                                        GtkWidget         *controls)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->controls = controls;
}

GtkWidget *
gimp_new_rect_select_tool_get_controls (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->controls;
}

void
gimp_new_rect_select_tool_set_entry (GimpRectangleTool *tool,
                                     GtkWidget         *entry)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dimensions_entry = entry;
}

GtkWidget *
gimp_new_rect_select_tool_get_entry (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dimensions_entry;
}

void
gimp_new_rect_select_tool_set_startx (GimpRectangleTool *tool,
                                      gint               startx)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->startx = startx;
}

gint
gimp_new_rect_select_tool_get_startx (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->startx;
}

void
gimp_new_rect_select_tool_set_starty (GimpRectangleTool *tool,
                                      gint               starty)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->starty = starty;
}

gint
gimp_new_rect_select_tool_get_starty (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->starty;
}

void
gimp_new_rect_select_tool_set_lastx (GimpRectangleTool *tool,
                                     gint               lastx)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->lastx = lastx;
}

gint
gimp_new_rect_select_tool_get_lastx (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->lastx;
}

void
gimp_new_rect_select_tool_set_lasty (GimpRectangleTool *tool,
                                     gint               lasty)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->lasty = lasty;
}

gint
gimp_new_rect_select_tool_get_lasty (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->lasty;
}

void
gimp_new_rect_select_tool_set_pressx (GimpRectangleTool *tool,
                                      gint               pressx)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->pressx = pressx;
}

gint
gimp_new_rect_select_tool_get_pressx (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->pressx;
}

void
gimp_new_rect_select_tool_set_pressy (GimpRectangleTool *tool,
                                      gint               pressy)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->pressy = pressy;
}

gint
gimp_new_rect_select_tool_get_pressy (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->pressy;
}

void
gimp_new_rect_select_tool_set_x1 (GimpRectangleTool *tool,
                                  gint               x1)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->x1 = x1;
}

gint
gimp_new_rect_select_tool_get_x1 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->x1;
}

void
gimp_new_rect_select_tool_set_y1 (GimpRectangleTool *tool,
                                  gint               y1)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->y1 = y1;
}

gint
gimp_new_rect_select_tool_get_y1 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->y1;
}

void
gimp_new_rect_select_tool_set_x2 (GimpRectangleTool *tool,
                                  gint               x2)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->x2 = x2;
}

gint
gimp_new_rect_select_tool_get_x2 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->x2;
}

void
gimp_new_rect_select_tool_set_y2 (GimpRectangleTool *tool,
                                  gint               y2)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->y2 = y2;
}

gint
gimp_new_rect_select_tool_get_y2 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->y2;
}

void
gimp_new_rect_select_tool_set_function (GimpRectangleTool *tool,
                                        guint              function)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->function = function;
}

guint
gimp_new_rect_select_tool_get_function (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->function;
}

void
gimp_new_rect_select_tool_set_dx1 (GimpRectangleTool *tool,
                                   gint               dx1)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dx1 = dx1;
}

gint
gimp_new_rect_select_tool_get_dx1 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dx1;
}

void
gimp_new_rect_select_tool_set_dy1 (GimpRectangleTool *tool,
                                   gint               dy1)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dy1 = dy1;
}

gint
gimp_new_rect_select_tool_get_dy1 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dy1;
}

void
gimp_new_rect_select_tool_set_dx2 (GimpRectangleTool *tool,
                                   gint               dx2)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dx2 = dx2;
}

gint
gimp_new_rect_select_tool_get_dx2 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dx2;
}

void
gimp_new_rect_select_tool_set_dy2 (GimpRectangleTool *tool,
                                   gint               dy2)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dy2 = dy2;
}

gint
gimp_new_rect_select_tool_get_dy2 (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dy2;
}

void
gimp_new_rect_select_tool_set_dcw (GimpRectangleTool *tool,
                                   gint               dcw)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dcw = dcw;
}

gint
gimp_new_rect_select_tool_get_dcw (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dcw;
}

void
gimp_new_rect_select_tool_set_dch (GimpRectangleTool *tool,
                                   gint               dch)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->dch = dch;
}

gint
gimp_new_rect_select_tool_get_dch (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->dch;
}

void
gimp_new_rect_select_tool_set_origx (GimpRectangleTool *tool,
                                     gdouble            origx)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->origx = origx;
}

gdouble
gimp_new_rect_select_tool_get_origx (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->origx;
}

void
gimp_new_rect_select_tool_set_origy (GimpRectangleTool *tool,
                                     gdouble            origy)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->origy = origy;
}

gdouble
gimp_new_rect_select_tool_get_origy (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->origy;
}

void
gimp_new_rect_select_tool_set_sizew (GimpRectangleTool *tool,
                                     gdouble            sizew)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->sizew = sizew;
}

gdouble
gimp_new_rect_select_tool_get_sizew (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->sizew;
}

void
gimp_new_rect_select_tool_set_sizeh (GimpRectangleTool *tool,
                                     gdouble            sizeh)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  new_rect_select_tool->sizeh = sizeh;
}

gdouble
gimp_new_rect_select_tool_get_sizeh (GimpRectangleTool *tool)
{
  GimpNewRectSelectTool *new_rect_select_tool = GIMP_NEW_RECT_SELECT_TOOL (tool);
  return new_rect_select_tool->sizeh;
}

