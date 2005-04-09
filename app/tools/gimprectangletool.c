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
#include "core/gimpimage-crop.h"
#include "core/gimptoolinfo.h"
#include "core/gimpunit.h"
#include "core/gimp-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpselectiontool.h"
#include "gimpselectionoptions.h"
#include "gimprectangletool.h"
#include "gimprectangleoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  speed of key movement  */
#define ARROW_VELOCITY 25

static void     gimp_rectangle_tool_class_init     (GimpRectangleToolClass *klass);
static void     gimp_rectangle_tool_init           (GimpRectangleTool      *rectangle_tool);

static GObject * gimp_rectangle_tool_constructor   (GType              type,
                                                    guint              n_params,
                                                    GObjectConstructParam *params);
static void     gimp_rectangle_tool_finalize       (GObject           *object);
static gboolean gimp_rectangle_tool_initialize     (GimpTool          *tool,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_control        (GimpTool          *tool,
                                                    GimpToolAction     action,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_button_press   (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_button_release (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_motion         (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    guint32            time,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static gboolean gimp_rectangle_tool_key_press      (GimpTool          *tool,
                                                    GdkEventKey       *kevent,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_modifier_key   (GimpTool          *tool,
                                                    GdkModifierType    key,
                                                    gboolean           press,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_oper_update    (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);
static void     gimp_rectangle_tool_cursor_update  (GimpTool          *tool,
                                                    GimpCoords        *coords,
                                                    GdkModifierType    state,
                                                    GimpDisplay       *gdisp);

static void     gimp_rectangle_tool_draw           (GimpDrawTool      *draw_tool);

/*  Rectangle helper functions  */
static void     rectangle_recalc                   (GimpRectangleTool *rectangle);
static void     rectangle_tool_start               (GimpRectangleTool *rectangle);

/*  Rectangle dialog functions  */
static void     rectangle_info_update              (GimpRectangleTool *rectangle);
static void     rectangle_response                 (GtkWidget         *widget,
                                                    gint               response_id,
                                                    GimpRectangleTool *rectangle);

static void     rectangle_selection_callback       (GtkWidget         *widget,
                                                    GimpRectangleTool *rectangle);
static void     rectangle_automatic_callback       (GtkWidget         *widget,
                                                    GimpRectangleTool *rectangle);

static void     rectangle_dimensions_changed       (GtkWidget         *widget,
                                                    GimpRectangleTool *rectangle);

static gboolean rectangle_tool_execute             (GimpRectangleTool *rect_tool,
                                                    gint               x,
                                                    gint               y,
                                                    gint               w,
                                                    gint               h);

static gboolean gimp_rectangle_tool_real_execute   (GimpRectangleTool *rect_tool,
                                                    gint               x,
                                                    gint               y,
                                                    gint               w,
                                                    gint               h);
static void  gimp_rectangle_tool_update_options    (GimpRectangleTool *rectangle,
                                                    GimpDisplay       *gdisp);
static GtkWidget * gimp_rectangle_controls         (GimpRectangleTool *rectangle);
static void  gimp_rectangle_tool_zero_controls     (GimpRectangleTool *rectangle);

static GimpDrawToolClass *parent_class = NULL;


/*  public functions  */

GType
gimp_rectangle_tool_get_type (void)
{
  static GType tool_type = 0;

  if (! tool_type)
    {
      static const GTypeInfo tool_info =
      {
        sizeof (GimpRectangleToolClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_rectangle_tool_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpRectangleTool),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_rectangle_tool_init,
      };

      tool_type = g_type_register_static (GIMP_TYPE_SELECTION_TOOL,
                                          "GimpRectangleTool",
                                          &tool_info, 0);
    }

  return tool_type;
}

static void
gimp_rectangle_tool_class_init (GimpRectangleToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_rectangle_tool_constructor;
  object_class->finalize     = gimp_rectangle_tool_finalize;

  tool_class->initialize     = gimp_rectangle_tool_initialize;
  tool_class->control        = gimp_rectangle_tool_control;
  tool_class->button_press   = gimp_rectangle_tool_button_press;
  tool_class->button_release = gimp_rectangle_tool_button_release;
  tool_class->motion         = gimp_rectangle_tool_motion;
  tool_class->key_press      = gimp_rectangle_tool_key_press;
  tool_class->modifier_key   = gimp_rectangle_tool_modifier_key;
  tool_class->oper_update    = gimp_rectangle_tool_oper_update;
  tool_class->cursor_update  = gimp_rectangle_tool_cursor_update;

  draw_tool_class->draw      = gimp_rectangle_tool_draw;
  klass->execute             = gimp_rectangle_tool_real_execute;
}

static void
gimp_rectangle_tool_init (GimpRectangleTool *rectangle)
{
  GimpTool     *tool                = GIMP_TOOL (rectangle);

  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control, GIMP_DIRTY_IMAGE_SIZE);

  rectangle->selection_tool   = FALSE;
  rectangle->controls         = NULL;
  rectangle->dimensions_entry = NULL;
}

static GObject *
gimp_rectangle_tool_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject           *object;
  GimpTool          *tool;
  GimpRectangleTool *rectangle;
  GtkContainer *controls_container;
  GObject           *options;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  tool      = GIMP_TOOL (object);
  rectangle = GIMP_RECTANGLE_TOOL (object);

  tool->gdisp = NULL;

  g_assert (GIMP_IS_TOOL_INFO (tool->tool_info));

  options = G_OBJECT (tool->tool_info->tool_options);

  controls_container = GTK_CONTAINER (g_object_get_data (options,
                                                         "controls-container"));

  rectangle->controls = gimp_rectangle_controls (rectangle);
  gtk_container_add (controls_container, rectangle->controls);

  return object;
}

static void
gimp_rectangle_tool_finalize (GObject *object)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);

  if (rectangle->controls)
    {
      gtk_widget_destroy (rectangle->controls);
      rectangle->controls = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gimp_rectangle_tool_initialize (GimpTool    *tool,
                                GimpDisplay *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpSizeEntry     *entry;

  entry = GIMP_SIZE_ENTRY (rectangle->dimensions_entry);

  if (gdisp != tool->gdisp)
    {
      gint     width  = gimp_image_get_width (gdisp->gimage);
      gint     height = gimp_image_get_height (gdisp->gimage);
      GimpUnit unit;
      gdouble  xres;
      gdouble  yres;

      gimp_size_entry_set_refval_boundaries (entry, 0, 0, height);
      gimp_size_entry_set_refval_boundaries (entry, 1, 0, width);
      gimp_size_entry_set_refval_boundaries (entry, 2, 0, width);
      gimp_size_entry_set_refval_boundaries (entry, 3, 0, height);

      gimp_size_entry_set_size (entry, 0, 0, height);
      gimp_size_entry_set_size (entry, 1, 0, width);
      gimp_size_entry_set_size (entry, 2, 0, width);
      gimp_size_entry_set_size (entry, 3, 0, height);

      gimp_image_get_resolution (gdisp->gimage, &xres, &yres);

      gimp_size_entry_set_resolution (entry, 0, yres, TRUE);
      gimp_size_entry_set_resolution (entry, 1, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 2, xres, TRUE);
      gimp_size_entry_set_resolution (entry, 3, yres, TRUE);

      unit = gimp_display_shell_get_unit (GIMP_DISPLAY_SHELL (gdisp->shell));
      gimp_size_entry_set_unit (entry, unit);
    }

  return TRUE;
}

static void
gimp_rectangle_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);

  switch (action)
    {
    case PAUSE:
      break;

    case RESUME:
      rectangle_recalc (rectangle);
      break;

    case HALT:
      rectangle_response (NULL, GTK_RESPONSE_CANCEL, rectangle);
      break;

    default:
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, gdisp);
}

static void
gimp_rectangle_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
  GimpRectangleTool     *rectangle  = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool          *draw_tool  = GIMP_DRAW_TOOL (tool);
  GimpRectangleOptions  *options;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  if (gdisp != tool->gdisp)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      rectangle->function = RECT_CREATING;
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->gdisp = gdisp;

      rectangle->x2 = rectangle->x1 = ROUND (coords->x);
      rectangle->y2 = rectangle->y1 = ROUND (coords->y);

      rectangle_tool_start (rectangle);
    }

  rectangle->pressx = rectangle->lastx = rectangle->startx = ROUND (coords->x);
  rectangle->pressy = rectangle->lasty = rectangle->starty = ROUND (coords->y);

  gimp_tool_control_activate (tool->control);
}

static void
gimp_rectangle_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *gdisp)
{
  GimpRectangleTool     *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleOptions  *options;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  gimp_tool_control_halt (tool->control);
  gimp_tool_pop_status (tool);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if ( (rectangle->lastx == rectangle->pressx) &&
           (rectangle->lasty == rectangle->pressy))
        {
          rectangle_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE, rectangle);
        }

      rectangle_info_update (rectangle);
    }
}

static void
gimp_rectangle_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleOptions  *options;
  gint               x1, y1, x2, y2;
  gint               curx, cury;
  gint               inc_x, inc_y;
  gint               min_x, min_y, max_x, max_y;
  gboolean           fixed_width;
  gboolean           fixed_height;
  gboolean           fixed_aspect;
  gboolean           fixed_center;

  /*  This is the only case when the motion events should be ignored--
      we're just waiting for the button release event to execute  */
  if (rectangle->function == RECT_EXECUTING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  x1 = rectangle->startx;
  y1 = rectangle->starty;
  x2 = curx;
  y2 = cury;

  if (rectangle->function == RECT_CREATING)
    {
      if (x1 > x2 && y1 > y2)
        rectangle->function = RECT_RESIZING_UPPER_LEFT;
      else if (x1 > x2 && y1 < y2)
        rectangle->function = RECT_RESIZING_LOWER_LEFT;
      else if (x1 < x2 && y1 > y2)
        rectangle->function = RECT_RESIZING_UPPER_RIGHT;
      else if (x1 < x2 && y1 < y2)
        rectangle->function = RECT_RESIZING_LOWER_RIGHT;
    }

  inc_x = (x2 - x1);
  inc_y = (y2 - y1);

  /*  If there have been no changes... return  */
  if (rectangle->lastx == x2 && rectangle->lasty == y2)
    return;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);
  fixed_width  = options->fixed_width;
  fixed_height = options->fixed_height;
  fixed_aspect = options->fixed_aspect;
  fixed_center = options->fixed_center;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  switch (rectangle->function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = rectangle->x1 + inc_x;
      if (fixed_width)
        {
          x2 = x1 + options->width;
          if (x1 < 0)
            {
              x1 = 0;
              x2 = options->width;
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - options->width;
            }
        }
      else if (fixed_center)
        {
          x2 = x1 + 2 * (options->center_x - x1);
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * options->center_x;
            }
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - options->center_x);
            }
        }
      else
        {
          x2 = MAX (x1, rectangle->x2);
        }
      rectangle->startx = curx;
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = rectangle->x2 + inc_x;
      if (fixed_width)
        {
          x1 = x2 - options->width;
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - options->width;
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = options->width;
            }
        }
      else if (fixed_center)
        {
          x1 = x2 - 2 * (x2 - options->center_x);
          if (x2 > max_x)
            {
              x2 = max_x;
              x1 = max_x - 2 * (max_x - options->center_x);
            }
          if (x1 < 0)
            {
              x1 = 0;
              x2 = 2 * options->center_x;
            }
        }
      else
        {
          x1 = MIN (rectangle->x1, x2);
        }
      rectangle->startx = curx;
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = rectangle->x1;
      x2 = rectangle->x2;
      rectangle->startx = curx;
      break;

    case RECT_MOVING:
      if (fixed_width &&
          (rectangle->x1 + inc_x < 0 ||
           rectangle->x2 + inc_x > max_x))
        {
          x1 = rectangle->x1;
          x2 = rectangle->x2;
        }
      else
        {
          x1 = rectangle->x1 + inc_x;
          x2 = rectangle->x2 + inc_x;
        }
      rectangle->startx = curx;
      break;
    }

  switch (rectangle->function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      y1 = rectangle->y1 + inc_y;
      if (fixed_height)
        {
          y2 = y1 + options->height;
          if (y1 < 0)
            {
              y1 = 0;
              y2 = options->height;
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - options->height;
            }
        }
      else if (fixed_center)
        {
          y2 = y1 + 2 * (options->center_y - y1);
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * options->center_y;
            }
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - options->center_y);
            }
        }
      else
        {
          y2 = MAX (y1, rectangle->y2);
        }
      rectangle->starty = cury;
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = rectangle->y2 + inc_y;
      if (fixed_height)
        {
          y1 = y2 - options->height;
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - options->height;
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = options->height;
            }
        }
      else if (fixed_center)
        {
          y1 = y2 - 2 * (y2 - options->center_y);
          if (y2 > max_y)
            {
              y2 = max_y;
              y1 = max_y - 2 * (max_y - options->center_y);
            }
          if (y1 < 0)
            {
              y1 = 0;
              y2 = 2 * options->center_y;
            }
        }
      else
        {
          y1 = MIN (rectangle->y1, y2);
        }
      rectangle->starty = cury;
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = rectangle->y1;
      y2 = rectangle->y2;
      rectangle->starty = cury;
      break;

    case RECT_MOVING:
      if (fixed_height &&
          (rectangle->y1 + inc_y < 0 ||
          rectangle->y2 + inc_y > max_y))
        {
          y1 = rectangle->y1;
          y2 = rectangle->y2;
        }
      else
        {
          y1 = rectangle->y1 + inc_y;
          y2 = rectangle->y2 + inc_y;
        }
      rectangle->starty = cury;
      break;
    }

  /*  make sure that the coords are in bounds  */
  rectangle->x1 = MIN (x1, x2);
  rectangle->y1 = MIN (y1, y2);
  rectangle->x2 = MAX (x1, x2);
  rectangle->y2 = MAX (y1, y2);

  rectangle->lastx = curx;
  rectangle->lasty = cury;

  if (fixed_aspect)
    {
      gdouble aspect = options->aspect;

      if (aspect > max_y)
        aspect = max_y;
      if (aspect < 1.0 / max_x)
        aspect = 1.0 / max_x;

      switch (rectangle->function)
        {
        case RECT_RESIZING_UPPER_LEFT:
        case RECT_RESIZING_LEFT:
          rectangle->y2 = rectangle->y1 + aspect * (rectangle->x2 - rectangle->x1);
          if (rectangle->y2 > max_y)
            {
              rectangle->y2 = max_y;
              rectangle->x2 = rectangle->x1 + (rectangle->y2 - rectangle->y1) / aspect;
            }
          break;

        case RECT_RESIZING_UPPER_RIGHT:
        case RECT_RESIZING_RIGHT:
          rectangle->y2 = rectangle->y1 + aspect * (rectangle->x2 - rectangle->x1);
          if (rectangle->y2 > max_y)
            {
              rectangle->y2 = max_y;
              rectangle->x1 = rectangle->x2 - (rectangle->y2 - rectangle->y1) / aspect;
            }
          break;

        case RECT_RESIZING_LOWER_LEFT:
          rectangle->y1 = rectangle->y2 - aspect * (rectangle->x2 - rectangle->x1);
          if (rectangle->y1 < 0)
            {
              rectangle->y1 = 0;
              rectangle->x2 = rectangle->x1 + (rectangle->y2 - rectangle->y1) / aspect;
            }
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          rectangle->y1 = rectangle->y2 - aspect * (rectangle->x2 - rectangle->x1);
          if (rectangle->y1 < 0)
            {
              rectangle->y1 = 0;
              rectangle->x1 = rectangle->x2 - (rectangle->y2 - rectangle->y1) / aspect;
            }
          break;

        case RECT_RESIZING_TOP:
          rectangle->x2 = rectangle->x1 + (rectangle->y2 - rectangle->y1) / aspect;
          if (rectangle->x2 > max_x)
            {
              rectangle->x2 = max_x;
              rectangle->y2 = rectangle->y1 + aspect * (rectangle->x2 - rectangle->x1);
            }
          break;

        case RECT_RESIZING_BOTTOM:
          rectangle->x2 = rectangle->x1 + (rectangle->y2 - rectangle->y1) / aspect;
          if (rectangle->x2 > max_x)
            {
              rectangle->x2 = max_x;
              rectangle->y1 = rectangle->y2 - aspect * (rectangle->x2 - rectangle->x1);
            }
          break;

        default:
          break;
        }
    }

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  rectangle_recalc (rectangle);

  switch (rectangle->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x,
                                          rectangle->y1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x,
                                          rectangle->y1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x,
                                          rectangle->y2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x,
                                          rectangle->y2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, rectangle->y1 - coords->y, 0, 0);
      break;

    case RECT_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, rectangle->y2 - coords->y, 0, 0);
      break;

    case RECT_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x,
                                          rectangle->y1 - coords->y,
                                          rectangle->x2 - rectangle->x1,
                                          rectangle->y2 - rectangle->y1);
      break;

    default:
      break;
    }

  gimp_rectangle_tool_update_options (rectangle, gdisp);

  if (rectangle->function == RECT_CREATING      ||
      rectangle->function == RECT_RESIZING_UPPER_LEFT ||
      rectangle->function == RECT_RESIZING_LOWER_RIGHT)
    {
      gimp_tool_pop_status (tool);

      gimp_tool_push_status_coords (tool,
                                    _("Rectangle: "),
                                    rectangle->x2 - rectangle->x1,
                                    " x ",
                                    rectangle->y2 - rectangle->y1);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  gint               inc_x, inc_y;
  gint               min_x, min_y;
  gint               max_x, max_y;

  if (gdisp != tool->gdisp)
    return FALSE;

  inc_x = inc_y = 0;

  switch (kevent->keyval)
    {
    case GDK_Up:
      inc_y = -1;
      break;
    case GDK_Left:
      inc_x = -1;
      break;
    case GDK_Right:
      inc_x = 1;
      break;
    case GDK_Down:
      inc_y = 1;
      break;

    case GDK_KP_Enter:
    case GDK_Return:
      rectangle_response (NULL, GIMP_RECTANGLE_MODE_EXECUTE, rectangle);
      return TRUE;

    case GDK_Escape:
      rectangle_response (NULL, GTK_RESPONSE_CANCEL, rectangle);
      return TRUE;

    default:
      return FALSE;
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & GDK_SHIFT_MASK)
    {
      inc_y *= ARROW_VELOCITY;
      inc_x *= ARROW_VELOCITY;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  min_x = min_y = 0;
  max_x = gdisp->gimage->width;
  max_y = gdisp->gimage->height;

  rectangle->x1 += inc_x;
  rectangle->x2 += inc_x;
  rectangle->y1 += inc_y;
  rectangle->y2 += inc_y;

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  return TRUE;
}

static void
gimp_rectangle_tool_modifier_key (GimpTool        *tool,
                                  GdkModifierType  key,
                                  gboolean         press,
                                  GdkModifierType  state,
                                  GimpDisplay     *gdisp)
{
}

static void
gimp_rectangle_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 GimpDisplay     *gdisp)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool      *draw_tool = GIMP_DRAW_TOOL (tool);
  gboolean           inside_x;
  gboolean           inside_y;

  if (tool->gdisp != gdisp)
    {
      if (rectangle->selection_tool)
        GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, gdisp);

      return;
    }

  inside_x = coords->x > rectangle->x1 && coords->x < rectangle->x2;
  inside_y = coords->y > rectangle->y1 && coords->y < rectangle->y2;

  if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                rectangle->x1, rectangle->y1,
                                rectangle->dcw, rectangle->dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      rectangle->function = RECT_RESIZING_UPPER_LEFT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x,
                                          rectangle->y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     rectangle->x2, rectangle->y2,
                                     rectangle->dcw, rectangle->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      rectangle->function = RECT_RESIZING_LOWER_RIGHT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x,
                                          rectangle->y2 - coords->y,
                                          0, 0);
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      rectangle->x2, rectangle->y1,
                                      rectangle->dcw, rectangle->dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      rectangle->function = RECT_RESIZING_UPPER_RIGHT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x,
                                          rectangle->y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, gdisp,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     rectangle->x1, rectangle->y2,
                                     rectangle->dcw, rectangle->dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      rectangle->function = RECT_RESIZING_LOWER_LEFT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x,
                                          rectangle->y2 - coords->y,
                                          0, 0);
    }
  else if ( (fabs (coords->x - rectangle->x1) < rectangle->dcw) && inside_y)
    {
      rectangle->function = RECT_RESIZING_LEFT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x1 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->x - rectangle->x2) < rectangle->dcw) && inside_y)
    {
      rectangle->function = RECT_RESIZING_RIGHT;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          rectangle->x2 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->y - rectangle->y1) < rectangle->dch) && inside_x)
    {
      rectangle->function = RECT_RESIZING_TOP;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, rectangle->y1 - coords->y, 0, 0);

    }
  else if ( (fabs (coords->y - rectangle->y2) < rectangle->dch) && inside_x)
    {
      rectangle->function = RECT_RESIZING_BOTTOM;

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, rectangle->y2 - coords->y, 0, 0);

    }
  else if (inside_x && inside_y)
    {
      rectangle->function = RECT_MOVING;
    }
  /*  otherwise, the new function will be creating, since we want
   *  to start a new rectangle
   */
  else
    {
      rectangle->function = RECT_CREATING;

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }

}

static void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *gdisp)
{
  GimpRectangleTool     *rectangle   = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleOptions  *options;
  GimpCursorType         cursor      = GDK_CROSSHAIR;
  GimpCursorModifier     modifier    = GIMP_CURSOR_MODIFIER_NONE;
  GimpToolCursorType     tool_cursor;

  tool_cursor = gimp_tool_control_get_tool_cursor (tool->control);

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  if (tool->gdisp == gdisp && ! (state & GDK_BUTTON1_MASK))
    {
      switch (rectangle->function)
        {
        case RECT_CREATING:
          cursor = GDK_CROSSHAIR;
          break;
        case RECT_MOVING:
          cursor = GDK_FLEUR;
          break;
        case RECT_RESIZING_UPPER_LEFT:
          cursor = GDK_TOP_LEFT_CORNER;
          break;
        case RECT_RESIZING_UPPER_RIGHT:
          cursor = GDK_TOP_RIGHT_CORNER;
          break;
        case RECT_RESIZING_LOWER_LEFT:
          cursor = GDK_BOTTOM_LEFT_CORNER;
          break;
        case RECT_RESIZING_LOWER_RIGHT:
          cursor = GDK_BOTTOM_RIGHT_CORNER;
          break;
        case RECT_RESIZING_LEFT:
          cursor = GDK_LEFT_SIDE;
          break;
        case RECT_RESIZING_RIGHT:
          cursor = GDK_RIGHT_SIDE;
          break;
        case RECT_RESIZING_TOP:
          cursor = GDK_TOP_SIDE;
          break;
        case RECT_RESIZING_BOTTOM:
          cursor = GDK_BOTTOM_SIDE;
          break;

        default:
          break;
        }
    }

  gimp_tool_control_set_cursor (tool->control, cursor);
  gimp_tool_control_set_tool_cursor (tool->control, tool_cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);

  gimp_tool_set_cursor (tool, gdisp,
                        gimp_tool_control_get_cursor (tool->control),
                        gimp_tool_control_get_tool_cursor (tool->control),
                        gimp_tool_control_get_cursor_modifier (tool->control));
}

static void
gimp_rectangle_tool_draw (GimpDrawTool *draw)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (draw);
  GimpTool          *tool      = GIMP_TOOL (draw);
  GimpDisplayShell  *shell     = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpCanvas        *canvas    = GIMP_CANVAS (shell->canvas);

  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rectangle->dx1, rectangle->dy1,
                         rectangle->dx2, rectangle->dy1);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rectangle->dx1, rectangle->dy1,
                         rectangle->dx1, rectangle->dy2);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rectangle->dx2, rectangle->dy2,
                         rectangle->dx1, rectangle->dy2);
  gimp_canvas_draw_line (canvas, GIMP_CANVAS_STYLE_XOR,
                         rectangle->dx2, rectangle->dy2,
                         rectangle->dx2, rectangle->dy1);
}

static void
rectangle_recalc (GimpRectangleTool *rectangle)
{
  GimpTool              *tool     = GIMP_TOOL (rectangle);
  GimpDisplayShell      *shell    = GIMP_DISPLAY_SHELL (tool->gdisp->shell);
  GimpRectangleOptions  *options;

  options = GIMP_RECTANGLE_OPTIONS (tool->tool_info->tool_options);

  if (! tool->gdisp)
    return;

  if (options->highlight)
    {
      GdkRectangle rect;

      rect.x      = rectangle->x1;
      rect.y      = rectangle->y1;
      rect.width  = rectangle->x2 - rectangle->x1;
      rect.height = rectangle->y2 - rectangle->y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   rectangle->x1, rectangle->y1,
                                   &rectangle->dx1, &rectangle->dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   rectangle->x2, rectangle->y2,
                                   &rectangle->dx2, &rectangle->dy2,
                                   FALSE);

#define SRW 10
#define SRH 10

  rectangle->dcw = ((rectangle->dx2 - rectangle->dx1) < SRW) ?
    (rectangle->dx2 - rectangle->dx1) : SRW;

  rectangle->dch = ((rectangle->dy2 - rectangle->dy1) < SRH) ?
    (rectangle->dy2 - rectangle->dy1) : SRH;

#undef SRW
#undef SRH

  rectangle_info_update (rectangle);
}

static void
rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool         *tool  = GIMP_TOOL (rectangle);

  rectangle_recalc (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->gdisp);
}


static void
rectangle_info_update (GimpRectangleTool *rectangle)
{
  rectangle->orig_vals[0] = rectangle->x1;
  rectangle->orig_vals[1] = rectangle->y1;
  rectangle->size_vals[0] = rectangle->x2 - rectangle->x1;
  rectangle->size_vals[1] = rectangle->y2 - rectangle->y1;

}

static void
rectangle_response (GtkWidget         *widget,
                    gint               response_id,
                    GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);
  gboolean  finish;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  finish = rectangle_tool_execute (GIMP_RECTANGLE_TOOL (tool),
                                   rectangle->x1, rectangle->y1,
                                   rectangle->x2 - rectangle->x1,
                                   rectangle->y2 - rectangle->y1);

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));

  if (finish)
    {
      gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->gdisp->shell),
                                        NULL);

      if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
        gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

      if (gimp_tool_control_is_active (GIMP_TOOL (rectangle)->control))
        gimp_tool_control_halt (GIMP_TOOL (rectangle)->control);

      gimp_image_flush (tool->gdisp->gimage);

      tool->gdisp    = NULL;
      tool->drawable = NULL;

    }
}

static void
rectangle_selection_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpRectangleOptions *options;
  GimpDisplay          *gdisp;

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  if (! gimp_channel_bounds (gimp_image_get_mask (gdisp->gimage),
                             &rectangle->x1, &rectangle->y1,
                             &rectangle->x2, &rectangle->y2))
    {
      rectangle->x1 = rectangle->y1 = 0;
      rectangle->x2 = gdisp->gimage->width;
      rectangle->y2 = gdisp->gimage->height;
    }

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
}

static void
rectangle_automatic_callback (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpRectangleOptions *options;
  GimpDisplay          *gdisp;
  gint                  offset_x, offset_y;
  gint                  width, height;
  gint                  x1, y1, x2, y2;
  gint                  shrunk_x1;
  gint                  shrunk_y1;
  gint                  shrunk_x2;
  gint                  shrunk_y2;

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);
  gdisp   = GIMP_TOOL (rectangle)->gdisp;

  width    = gdisp->gimage->width;
  height   = gdisp->gimage->height;
  offset_x = 0;
  offset_y = 0;

  x1 = rectangle->x1 - offset_x  > 0      ? rectangle->x1 - offset_x : 0;
  x2 = rectangle->x2 - offset_x  < width  ? rectangle->x2 - offset_x : width;
  y1 = rectangle->y1 - offset_y  > 0      ? rectangle->y1 - offset_y : 0;
  y2 = rectangle->y2 - offset_y  < height ? rectangle->y2 - offset_y : height;

  if (gimp_image_crop_auto_shrink (gdisp->gimage,
                                   x1, y1, x2, y2,
                                   FALSE,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      rectangle->x1 = offset_x + shrunk_x1;
      rectangle->x2 = offset_x + shrunk_x2;
      rectangle->y1 = offset_y + shrunk_y1;
      rectangle->y2 = offset_y + shrunk_y2;

      rectangle_recalc (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }
}

static void
rectangle_dimensions_changed (GtkWidget         *widget,
                              GimpRectangleTool *rectangle)
{
  GimpSizeEntry  *entry = GIMP_SIZE_ENTRY (widget);
  gdouble         x1, y1, x2, y2;
  GimpCoords      coords;

  if (! GIMP_TOOL (rectangle)->gdisp)
    return;

  x1 = gimp_size_entry_get_refval (entry, 2);
  y1 = gimp_size_entry_get_refval (entry, 3);
  x2 = gimp_size_entry_get_refval (entry, 1);
  y2 = gimp_size_entry_get_refval (entry, 0);

  if (x1 != rectangle->x1)
    {
      rectangle->function = RECT_RESIZING_LEFT;
      coords.x = x1;
      coords.y = y1;
      rectangle->startx = rectangle->x1;
      rectangle->starty = rectangle->y1;
    }
  else if (y1 != rectangle->y1)
    {
      rectangle->function = RECT_RESIZING_TOP;
      coords.x = x1;
      coords.y = y1;
      rectangle->startx = rectangle->x1;
      rectangle->starty = rectangle->y1;
    }
  else if (x2 != rectangle->x2)
    {
      rectangle->function = RECT_RESIZING_RIGHT;
      coords.x = x2;
      coords.y = y2;
      rectangle->startx = rectangle->x2;
      rectangle->starty = rectangle->y2;
    }
  else if (y2 != rectangle->y2)
    {
      rectangle->function = RECT_RESIZING_BOTTOM;
      coords.x = x2;
      coords.y = y2;
      rectangle->startx = rectangle->x2;
      rectangle->starty = rectangle->y2;
    }
  else
    return;

  /* use the motion handler to handle this, to avoid duplicating
     a bunch of code */
  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->gdisp);
}

void
rectangle_width_changed (GimpRectangleTool *rectangle,
                         gint               new_width)
{
  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  rectangle->x2 = rectangle->x1 + new_width;

  rectangle_recalc (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
}

static gboolean
rectangle_tool_execute (GimpRectangleTool *rect_tool,
                        gint               x,
                        gint               y,
                        gint               w,
                        gint               h)
{
  GimpTool             *tool;
  GimpSelectionOptions *options;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool), FALSE);

  tool    = GIMP_TOOL (rect_tool);
  options = GIMP_SELECTION_OPTIONS (tool->tool_info->tool_options);

  if (options->auto_shrink)
    {
      gint off_x = 0;
      gint off_y = 0;
      gint x2, y2;

      if (! gimp_rectangle_intersect (x, y, w, h,
                                      0, 0,
                                      tool->gdisp->gimage->width,
                                      tool->gdisp->gimage->height,
                                      &x, &y, &w, &h))
        {
          return FALSE;
        }

      if (! options->shrink_merged)
        {
          GimpItem *item;
          gint      width, height;

          item = GIMP_ITEM (gimp_image_active_drawable (tool->gdisp->gimage));

          gimp_item_offsets (item, &off_x, &off_y);
          width  = gimp_item_width  (item);
          height = gimp_item_height (item);

          if (! gimp_rectangle_intersect (x, y, w, h,
                                          off_x, off_y, width, height,
                                          &x, &y, &w, &h))
            {
              return FALSE;
            }

          x -= off_x;
          y -= off_y;
        }

      if (gimp_image_crop_auto_shrink (tool->gdisp->gimage,
                                       x, y,
                                       x + w, y + h,
                                       ! options->shrink_merged,
                                       &x, &y,
                                       &x2, &y2))
        {
          w = x2 - x;
          h = y2 - y;
        }

      x += off_x;
      y += off_y;
    }

  return GIMP_RECTANGLE_TOOL_GET_CLASS (rect_tool)->execute (rect_tool, x, y, w, h);
}

static gboolean
gimp_rectangle_tool_real_execute (GimpRectangleTool *rectangle,
                                  gint               x,
                                  gint               y,
                                  gint               w,
                                  gint               h)
{
  GimpTool    *tool             = GIMP_TOOL (rectangle);
  gboolean     rectangle_exists = (rectangle->function != RECT_CREATING);
  GimpChannel *selection_mask;
  gint         val;
  gboolean     selected;
  GimpImage   *gimage;

  gimage = tool->gdisp->gimage;
  selection_mask = gimp_image_get_mask (gimage);
  val = gimp_channel_value (selection_mask,
                            rectangle->pressx, rectangle->pressy);

  selected = (val > 127);

  if (rectangle_exists && selected)
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

  return TRUE;
}


static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rectangle,
                                    GimpDisplay       *gdisp)
{
  GimpDisplayShell *shell;
  gdouble           width;
  gdouble           height;
  gdouble           aspect;
  gdouble           center_x, center_y;
  GimpSizeEntry    *entry;
  GimpRectangleOptions *options;
  shell = GIMP_DISPLAY_SHELL (gdisp->shell);

  entry   = GIMP_SIZE_ENTRY (rectangle->dimensions_entry);
  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);

  width  = rectangle->x2 - rectangle->x1;
  height = rectangle->y2 - rectangle->y1;

  if (width > 0.01)
    aspect = height / width;
  else
    aspect = 0;

  center_x = (rectangle->x1 + rectangle->x2) / 2;
  center_y = (rectangle->y1 + rectangle->y2) / 2;

  g_signal_handlers_block_by_func (rectangle->dimensions_entry,
                                   rectangle_dimensions_changed,
                                   rectangle);

  gimp_size_entry_set_refval (entry, 0, rectangle->y2);
  gimp_size_entry_set_refval (entry, 1, rectangle->x2);
  gimp_size_entry_set_refval (entry, 2, rectangle->x1);
  gimp_size_entry_set_refval (entry, 3, rectangle->y1);

  if (!options->fixed_width)
    g_object_set (options, "width",  width, NULL);

  if (!options->fixed_height)
    g_object_set (options, "height", height, NULL);

  if (!options->fixed_aspect)
    g_object_set (options, "aspect", aspect, NULL);

  if (!options->fixed_center)
    g_object_set (options,
                  "center-x", center_x,
                  "center-y", center_y,
                  NULL);

  g_signal_handlers_unblock_by_func (rectangle->dimensions_entry,
                                     rectangle_dimensions_changed,
                                     rectangle);
}

static GtkWidget *
gimp_rectangle_controls (GimpRectangleTool *rectangle)
{
  GtkWidget            *vbox;
  GtkObject            *adjustment;
  GtkWidget            *spinbutton;
  GimpRectangleOptions *options;
  GtkWidget            *entry;

  options = GIMP_RECTANGLE_OPTIONS (GIMP_TOOL (rectangle)->tool_info->tool_options);

  vbox = gtk_vbox_new (FALSE, 0);

  entry = gimp_size_entry_new (2, GIMP_UNIT_PIXEL, "%a",
                               TRUE, TRUE, FALSE, 4,
                               GIMP_SIZE_ENTRY_UPDATE_SIZE);
  rectangle->dimensions_entry = entry;

  gtk_table_set_row_spacings (GTK_TABLE (entry), 3);
  gtk_table_set_col_spacings (GTK_TABLE (entry), 3);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 1, 2, 2, 3);
  gtk_widget_show (spinbutton);

  spinbutton = gimp_spin_button_new (&adjustment, 1, 0, 1, 1, 10, 1, 1, 2);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 4);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (entry),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_table_attach_defaults (GTK_TABLE (entry), spinbutton, 2, 3, 2, 3);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("1 "), 1, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("2 "), 2, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("X"), 0, 1, 0.5);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (entry), _("Y"), 0, 2, 0.5);

  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

  g_signal_connect (entry, "value-changed",
                    G_CALLBACK (rectangle_dimensions_changed),
                    rectangle);

  gtk_widget_show (entry);

  gimp_rectangle_tool_zero_controls (rectangle);

  gtk_widget_show (vbox);

  return vbox;
}

static void
gimp_rectangle_tool_zero_controls (GimpRectangleTool *rectangle)
{
  GimpSizeEntry    *entry = GIMP_SIZE_ENTRY (rectangle->dimensions_entry);

  gimp_size_entry_set_refval (entry, 0, 0);
  gimp_size_entry_set_refval (entry, 1, 0);
  gimp_size_entry_set_refval (entry, 2, 0);
  gimp_size_entry_set_refval (entry, 3, 0);
}
