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

#include "base/boundary.h"

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimplayer-floating-sel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimptoolinfo.h"
#include "core/gimp-utils.h"
#include "core/gimpundo.h"
#include "core/gimpundostack.h"

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


static void     gimp_new_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface);

static GObject *
                gimp_new_rect_select_tool_constructor       (GType              type,
                                                             guint              n_params,
                                                             GObjectConstructParam *params);
static void     gimp_new_rect_select_tool_finalize          (GObject           *object);
static void     gimp_new_rect_select_tool_control           (GimpTool          *tool,
                                                             GimpToolAction     action,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_button_press      (GimpTool          *tool,
                                                             GimpCoords        *coords,
                                                             guint32            time,
                                                             GdkModifierType    state,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_button_release    (GimpTool          *tool,
                                                             GimpCoords        *coords,
                                                             guint32            time,
                                                             GdkModifierType    state,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_modifier_key      (GimpTool          *tool,
                                                             GdkModifierType    key,
                                                             gboolean           press,
                                                             GdkModifierType    state,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_oper_update       (GimpTool          *tool,
                                                             GimpCoords        *coords,
                                                             GdkModifierType    state,
                                                             gboolean           proximity,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_cursor_update     (GimpTool          *tool,
                                                             GimpCoords        *coords,
                                                             GdkModifierType    state,
                                                             GimpDisplay       *display);
static void     gimp_new_rect_select_tool_rect_select       (GimpRectangleTool *rect_tool,
                                                             gint               x,
                                                             gint               y,
                                                             gint               w,
                                                             gint               h);
static gboolean gimp_new_rect_select_tool_execute           (GimpRectangleTool *rect_tool,
                                                             gint               x,
                                                             gint               y,
                                                             gint               w,
                                                             gint               h);
static void     gimp_new_rect_select_tool_cancel            (GimpRectangleTool *rect_tool);
static gboolean gimp_new_rect_select_tool_rectangle_changed (GimpRectangleTool *rect_tool);
static void     gimp_new_rect_select_tool_real_rect_select  (GimpNewRectSelectTool *rect_select,
                                                             gint                   x,
                                                             gint                   y,
                                                             gint                   w,
                                                             gint                   h);


G_DEFINE_TYPE_WITH_CODE (GimpNewRectSelectTool, gimp_new_rect_select_tool,
                         GIMP_TYPE_SELECTION_TOOL,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RECTANGLE_TOOL,
                                                gimp_new_rect_select_tool_rectangle_tool_iface_init))

#define parent_class gimp_new_rect_select_tool_parent_class


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
                N_("_New Rect Select"), "R",
                NULL, GIMP_HELP_TOOL_RECT_SELECT,
                GIMP_STOCK_TOOL_RECT_SELECT,
                data);
}

static void
gimp_new_rect_select_tool_class_init (GimpNewRectSelectToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  object_class->constructor  = gimp_new_rect_select_tool_constructor;
  object_class->dispose      = gimp_rectangle_tool_dispose;
  object_class->finalize     = gimp_new_rect_select_tool_finalize;
  object_class->set_property = gimp_rectangle_tool_set_property;
  object_class->get_property = gimp_rectangle_tool_get_property;

  gimp_rectangle_tool_install_properties (object_class);

  tool_class->initialize     = gimp_rectangle_tool_initialize;
  tool_class->control        = gimp_new_rect_select_tool_control;
  tool_class->button_press   = gimp_new_rect_select_tool_button_press;
  tool_class->button_release = gimp_new_rect_select_tool_button_release;
  tool_class->motion         = gimp_rectangle_tool_motion;
  tool_class->key_press      = gimp_rectangle_tool_key_press;
  tool_class->modifier_key   = gimp_new_rect_select_tool_modifier_key;
  tool_class->oper_update    = gimp_new_rect_select_tool_oper_update;
  tool_class->cursor_update  = gimp_new_rect_select_tool_cursor_update;

  draw_tool_class->draw      = gimp_rectangle_tool_draw;

  klass->rect_select         = gimp_new_rect_select_tool_real_rect_select;
}

static void
gimp_new_rect_select_tool_init (GimpNewRectSelectTool *new_rect_select_tool)
{
  GimpTool          *tool      = GIMP_TOOL (new_rect_select_tool);
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (new_rect_select_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_RECT_SELECT);
  gimp_rectangle_tool_set_constrain (rect_tool, TRUE);
}

static void
gimp_new_rect_select_tool_rectangle_tool_iface_init (GimpRectangleToolInterface *iface)
{
  iface->execute           = gimp_new_rect_select_tool_execute;
  iface->cancel            = gimp_new_rect_select_tool_cancel;
  iface->rectangle_changed = gimp_new_rect_select_tool_rectangle_changed;
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
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_new_rect_select_tool_control (GimpTool       *tool,
                                   GimpToolAction  action,
                                   GimpDisplay    *display)
{
  gimp_rectangle_tool_control (tool, action, display);

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_new_rect_select_tool_button_press (GimpTool        *tool,
                                        GimpCoords      *coords,
                                        guint32          time,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  guint function;

/*   /\* if we have an existing rectangle in the current display, then */
/*      we have already "executed", and need to undo at this point, unless */
/*      the user has done something in the meantime *\/ */
/*   if (tool->display && display == tool->display) */
/*     { */
/*       GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (tool); */
/*       GimpImage             *image       = display->image; */
/*       GimpUndo              *undo; */

/*       undo = gimp_undo_stack_peek (image->undo_stack); */
/*       if (undo && rect_select->undo == undo) */
/*         { */
/*           gimp_image_undo (image); */
/*           rect_select->undo = NULL; */
/*         } */
/*     } */

  if (tool->display && display != tool->display)
    gimp_rectangle_tool_cancel (GIMP_RECTANGLE_TOOL (tool));

  g_object_get (tool, "function", &function, NULL);

  if (function == RECT_INACTIVE)
    {
      GimpDisplay *old_display;
      gboolean     edit_started;

      old_display = tool->display;
      tool->display = display;
      gimp_tool_control_activate (tool->control);

      edit_started = gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (tool),
                                                     coords);

      if (gimp_tool_control_is_active (tool->control))
        gimp_tool_control_halt (tool->control);
      tool->display = old_display;

      if (edit_started)
        return;
    }

  gimp_rectangle_tool_button_press (tool, coords, time, state, display);
}

static void
gimp_new_rect_select_tool_button_release (GimpTool        *tool,
                                          GimpCoords      *coords,
                                          guint32          time,
                                          GdkModifierType  state,
                                          GimpDisplay     *display)
{
  gimp_tool_pop_status (tool, display);
  gimp_tool_push_status (tool, display,
                         _("Click or press enter to create the selection."));

  gimp_rectangle_tool_button_release (tool, coords, time, state, display);
}

static void
gimp_new_rect_select_tool_modifier_key (GimpTool        *tool,
                                        GdkModifierType  key,
                                        gboolean         press,
                                        GdkModifierType  state,
                                        GimpDisplay     *display)
{
  gimp_rectangle_tool_modifier_key (tool, key, press, state, display);

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press, state,
                                                display);
}

static void
gimp_new_rect_select_tool_oper_update (GimpTool        *tool,
                                       GimpCoords      *coords,
                                       GdkModifierType  state,
                                       gboolean         proximity,
                                       GimpDisplay     *display)
{
  guint function;

  gimp_rectangle_tool_oper_update (tool, coords, state, proximity, display);

  g_object_get (tool, "function", &function, NULL);

  if (function == RECT_INACTIVE)
    GIMP_SELECTION_TOOL (tool)->allow_move = TRUE;
  else
    GIMP_SELECTION_TOOL (tool)->allow_move = FALSE;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
}

static void
gimp_new_rect_select_tool_cursor_update (GimpTool        *tool,
                                         GimpCoords      *coords,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  gimp_rectangle_tool_cursor_update (tool, coords, state, display);

  GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords, state, display);
}


/*
 * This function is called if the user clicks and releases the left
 * button without moving it.  There are four things we might want
 * to do here:
 * 1) If there is an existing rectangle and we are inside it, we
 *    convert it into a selection.
 * 2) If there is an existing rectangle and we are outside it, we
 *    clear it.
 * 3) If there is no rectangle and we are inside the selection, we
 *    create a rectangle from the selection bounds.
 * 4) If there is no rectangle and we are outside the selection,
 *    we clear the selection.
 */
static void
gimp_new_rect_select_tool_rect_select (GimpRectangleTool *rectangle,
                                       gint               x,
                                       gint               y,
                                       gint               w,
                                       gint               h)
{
  GimpTool              *tool        = GIMP_TOOL (rectangle);
  GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (rectangle);
  GimpSelectionOptions  *options;
  GimpImage             *image;
  gboolean               rectangle_exists;
  gboolean               auto_shrink;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_pop_status (tool, tool->display);

  image = tool->display->image;

  rectangle_exists = (x <= image->width && y <= image->height &&
                      x + w >= 0 && y + h >= 0 &&
                      w > 0 && h > 0);

  g_object_get (options,
                "auto-shrink", &auto_shrink,
                NULL);

  if (auto_shrink)
    {
      /* FIXME */
    }

  /* if rectangle exists, turn it into a selection */
  if (rectangle_exists)
    GIMP_NEW_RECT_SELECT_TOOL_GET_CLASS (rect_select)->rect_select (rect_select,
                                                                    x, y, w, h);
}

static void
gimp_new_rect_select_tool_real_rect_select (GimpNewRectSelectTool *rect_select,
                                            gint                   x,
                                            gint                   y,
                                            gint                   w,
                                            gint                   h)
{
  GimpTool             *tool = GIMP_TOOL (rect_select);
  GimpSelectionOptions *options;

  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  gimp_channel_select_rectangle (gimp_image_get_mask (tool->display->image),
                                 x, y, w, h,
                                 options->operation,
                                 options->feather,
                                 options->feather_radius,
                                 options->feather_radius);
}

static gboolean
gimp_new_rect_select_tool_execute (GimpRectangleTool *rectangle,
                                   gint               x,
                                   gint               y,
                                   gint               w,
                                   gint               h)
{
  GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (rectangle);

  /*  don't keep the undo step across separate rectangles  */
  rect_select->undo = NULL;

  if (w == 0 && h == 0)
    {
      GimpImage   *image     = GIMP_TOOL (rectangle)->display->image;
      GimpChannel *selection = gimp_image_get_mask (image);;
      gint         pressx, pressy;

      g_object_get (rectangle,
                    "pressx", &pressx,
                    "pressy", &pressy,
                    NULL);

      /*  if the click was inside the marching ants  */
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (selection),
                                        pressx, pressy) > 127)
        {
          const BoundSeg *segs_in;
          const BoundSeg *segs_out;
          gint            n_segs_in;
          gint            n_segs_out;

          if (gimp_channel_boundary (selection,
                                     &segs_in, &segs_out,
                                     &n_segs_in, &n_segs_out,
                                     0, 0, 0, 0))
            {
              gint x1 = 0;
              gint y1 = 0;
              gint x2 = 0;
              gint y2 = 0;

              if (n_segs_in > 0)
                {
                  gint i;

                  x1 = segs_in[0].x1;
                  x2 = segs_in[0].x1;
                  y1 = segs_in[0].y1;
                  y2 = segs_in[0].y1;

                  for (i = 1; i < n_segs_in; i++)
                    {
                      x1 = MIN (x1, segs_in[i].x1);
                      x2 = MAX (x2, segs_in[i].x1);
                      y1 = MIN (y1, segs_in[i].y1);
                      y2 = MAX (y2, segs_in[i].y1);
                    }
                }

              g_object_set (rectangle,
                            "x1", x1,
                            "y1", y1,
                            "x2", x2,
                            "y2", y2,
                            NULL);
            }
          else
            {
              g_object_set (rectangle,
                            "x1", 0,
                            "y1", 0,
                            "x2", image->width,
                            "y2", image->height,
                            NULL);
            }

          g_object_set (rectangle, "function", RECT_MOVING, NULL);

          return FALSE;
        }
      else
        {
          /* otherwise clear the selection */
          gimp_channel_clear (selection, NULL, TRUE);
        }
    }

  return TRUE;
}

static void
gimp_new_rect_select_tool_cancel (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  if (tool->display)
    {
      GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (tool);
      GimpImage             *image       = tool->display->image;
      GimpUndo              *undo;

      /* if we have an existing rectangle in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      undo = gimp_undo_stack_peek (image->undo_stack);

      if (undo && rect_select->undo == undo)
        {
          gimp_image_undo (image);
          rect_select->undo = NULL;
        }
    }
}

static gboolean
gimp_new_rect_select_tool_rectangle_changed (GimpRectangleTool  *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  if (tool->display)
    {
      GimpNewRectSelectTool *rect_select = GIMP_NEW_RECT_SELECT_TOOL (tool);
      GimpImage             *image       = tool->display->image;
      GimpUndo              *undo;
      gint                   x1, y1, x2, y2;

      /* if we have an existing rectangle in the current display, then
       * we have already "executed", and need to undo at this point,
       * unless the user has done something in the meantime
       */
      undo = gimp_undo_stack_peek (image->undo_stack);

      if (undo && rect_select->undo == undo)
        {
          gimp_image_undo (image);
          rect_select->undo = NULL;
        }

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      gimp_new_rect_select_tool_rect_select (rectangle,
                                             x1, y1,
                                             x2 - x1,
                                             y2 - y1);

      /* save the undo that we got when executing */
      rect_select->undo = gimp_undo_stack_peek (image->undo_stack);
    }

  return TRUE;
}
