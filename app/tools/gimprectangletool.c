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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimppickable.h"
#include "core/gimpmarshal.h"

#include "display/gimpcanvas.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


enum
{
  RECTANGLE_CHANGED,
  LAST_SIGNAL
};

/*  speed of key movement  */
#define ARROW_VELOCITY   25

#define HANDLE_SIZE      50
#define MIN_HANDLE_SIZE   6


#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) \
  (gimp_rectangle_tool_get_private (GIMP_RECTANGLE_TOOL (obj)))


typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  gint                    pressx;     /*  x where button pressed         */
  gint                    pressy;     /*  y where button pressed         */

  gint                    x1, y1;     /*  upper left hand coordinate     */
  gint                    x2, y2;     /*  lower right hand coords        */

  guint                   function;   /*  moving or resizing             */

  GimpRectangleConstraint constraint; /* how to constrain rectangle     */

  /* Internal state */
  gint                    startx;     /*  starting x coord               */
  gint                    starty;     /*  starting y coord               */

  gint                    lastx;      /*  previous x coord               */
  gint                    lasty;      /*  previous y coord               */

  gint                    dcw, dch;   /*  width and height of edges      */

  gint                    saved_x1;   /*  for saving in case action      */
  gint                    saved_y1;   /*  is canceled                    */
  gint                    saved_x2;
  gint                    saved_y2;
  gdouble                 saved_center_x;
  gdouble                 saved_center_y;

  GimpRectangleGuide      guide; /* synced with options->guide, only exists for drawing */
};


static void gimp_rectangle_tool_iface_base_init     (GimpRectangleToolInterface *iface);

static GimpRectangleToolPrivate *
                gimp_rectangle_tool_get_private     (GimpRectangleTool *rectangle);

GimpRectangleConstraint
                gimp_rectangle_tool_get_constraint  (GimpRectangleTool *rectangle);

/*  Rectangle helper functions  */
static void     gimp_rectangle_tool_start           (GimpRectangleTool *rectangle);
static void     gimp_rectangle_tool_halt            (GimpRectangleTool *rectangle);
static void     gimp_rectangle_tool_draw_guides     (GimpDrawTool      *draw_tool);

/*  Rectangle dialog functions  */
static void     gimp_rectangle_tool_update_options  (GimpRectangleTool *rectangle,
                                                     GimpDisplay       *display);

static void     gimp_rectangle_tool_options_notify  (GimpRectangleOptions *options,
                                                     GParamSpec         *pspec,
                                                     GimpRectangleTool  *rectangle);

static void     gimp_rectangle_tool_check_function  (GimpRectangleTool *rectangle,
                                                     gint               curx,
                                                     gint               cury);
static gboolean
            gimp_rectangle_tool_constraint_violated (GimpRectangleTool *rectangle,
                                                     gint               x1,
                                                     gint               y1,
                                                     gint               x2,
                                                     gint               y2,
                                                     gdouble           *alpha,
                                                     gdouble           *beta);

static void     gimp_rectangle_tool_auto_shrink     (GimpRectangleTool *rectangle);


static guint gimp_rectangle_tool_signals[LAST_SIGNAL] = { 0 };


GType
gimp_rectangle_tool_interface_get_type (void)
{
  static GType iface_type = 0;

  if (! iface_type)
    {
      const GTypeInfo iface_info =
      {
        sizeof (GimpRectangleToolInterface),
        (GBaseInitFunc)     gimp_rectangle_tool_iface_base_init,
        (GBaseFinalizeFunc) NULL,
      };

      iface_type = g_type_register_static (G_TYPE_INTERFACE,
                                           "GimpRectangleToolInterface",
                                           &iface_info, 0);

      g_type_interface_add_prerequisite (iface_type, GIMP_TYPE_DRAW_TOOL);
    }

  return iface_type;
}

static void
gimp_rectangle_tool_iface_base_init (GimpRectangleToolInterface *iface)
{
  static gboolean initialized = FALSE;

  if (! initialized)
    {
      gimp_rectangle_tool_signals[RECTANGLE_CHANGED] =
        g_signal_new ("rectangle-changed",
                      G_TYPE_FROM_INTERFACE (iface),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpRectangleToolInterface,
                                       rectangle_changed),
                      NULL, NULL,
                      gimp_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("x1",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("y1",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("x2",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_int ("y2",
                                                             NULL, NULL,
                                                             -GIMP_MAX_IMAGE_SIZE,
                                                             GIMP_MAX_IMAGE_SIZE,
                                                             0,
                                                             GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_uint ("function",
                                                              NULL, NULL,
                                                              RECT_INACTIVE,
                                                              RECT_EXECUTING,
                                                              RECT_INACTIVE,
                                                              GIMP_PARAM_READWRITE));

      g_object_interface_install_property (iface,
                                           g_param_spec_uint ("constraint",
                                                              NULL, NULL,
                                                              GIMP_RECTANGLE_CONSTRAIN_NONE,
                                                              GIMP_RECTANGLE_CONSTRAIN_DRAWABLE,
                                                              GIMP_RECTANGLE_CONSTRAIN_NONE,
                                                              GIMP_PARAM_READWRITE));

      iface->execute           = NULL;
      iface->cancel            = NULL;
      iface->rectangle_changed = NULL;

      initialized = TRUE;
    }
}

static void
gimp_rectangle_tool_private_finalize (GimpRectangleToolPrivate *private)
{
  g_free (private);
}

static GimpRectangleToolPrivate *
gimp_rectangle_tool_get_private (GimpRectangleTool *tool)
{
  static GQuark private_key = 0;

  GimpRectangleToolPrivate *private;

  if (G_UNLIKELY (private_key == 0))
    private_key = g_quark_from_static_string ("gimp-rectangle-tool-private");

  private = g_object_get_qdata (G_OBJECT (tool), private_key);

  if (! private)
    {
      private = g_new0 (GimpRectangleToolPrivate, 1);

      g_object_set_qdata_full (G_OBJECT (tool), private_key, private,
                               (GDestroyNotify)
                               gimp_rectangle_tool_private_finalize);
    }

  return private;
}

/**
 * gimp_rectangle_tool_install_properties:
 * @klass: the class structure for a type deriving from #GObject
 *
 * Installs the necessary properties for a class implementing
 * #GimpToolOptions. A #GimpRectangleToolProp property is installed
 * for each property, using the values from the #GimpRectangleToolProp
 * enumeration. The caller must make sure itself that the enumeration
 * values don't collide with some other property values they
 * are using (that's what %GIMP_RECTANGLE_TOOL_PROP_LAST is good for).
 **/
void
gimp_rectangle_tool_install_properties (GObjectClass *klass)
{
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_X1,
                                    "x1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_Y1,
                                    "y1");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_X2,
                                    "x2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_Y2,
                                    "y2");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_FUNCTION,
                                    "function");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT,
                                    "constraint");
}

void
gimp_rectangle_tool_set_constraint (GimpRectangleTool       *tool,
                                    GimpRectangleConstraint  constraint)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->constraint = constraint;

  g_object_notify (G_OBJECT (tool), "constraint");
}

GimpRectangleConstraint
gimp_rectangle_tool_get_constraint (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->constraint;
}

void
gimp_rectangle_tool_get_press_coords (GimpRectangleTool *rectangle,
                                      gint              *pressx_ptr,
                                      gint              *pressy_ptr)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  *pressx_ptr = private->pressx;
  *pressy_ptr = private->pressy;
}

void
gimp_rectangle_tool_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  switch (property_id)
    {
    case GIMP_RECTANGLE_TOOL_PROP_X1:
      private->x1 = g_value_get_int (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y1:
      private->y1 = g_value_get_int (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X2:
      private->x2 = g_value_get_int (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y2:
      private->y2 = g_value_get_int (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_FUNCTION:
      private->function = g_value_get_uint (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT:
      gimp_rectangle_tool_set_constraint (rectangle, g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  switch (property_id)
    {
    case GIMP_RECTANGLE_TOOL_PROP_X1:
      g_value_set_int (value, private->x1);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y1:
      g_value_set_int (value, private->y1);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X2:
      g_value_set_int (value, private->x2);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y2:
      g_value_set_int (value, private->y2);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_FUNCTION:
      g_value_set_uint (value, private->function);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT:
      g_value_set_uint (value, gimp_rectangle_tool_get_constraint (rectangle));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_constructor (GObject *object)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (object);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (object);

  g_object_get (options,
                "guide", &private->guide,
                NULL);

  g_signal_connect_object (options, "notify",
                           G_CALLBACK (gimp_rectangle_tool_options_notify),
                           rectangle, 0);
}

void
gimp_rectangle_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      gimp_rectangle_tool_configure (rectangle);
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_rectangle_tool_halt (rectangle);
      break;

    default:
      break;
    }
}

void
gimp_rectangle_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpDrawTool             *draw_tool;
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  gint                      x, y;
  gint                      snap_x, snap_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  x = ROUND (coords->x);
  y = ROUND (coords->y);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      g_object_set (rectangle,
                    "function", RECT_CREATING,
                    "x1",       x,
                    "y1",       y,
                    "x2",       x,
                    "y2",       y,
                    NULL);

      tool->display = display;
      gimp_rectangle_tool_start (rectangle);
    }

  /* save existing shape in case of cancellation */
  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  g_object_get (options,
                "center-x", &private->saved_center_x,
                "center-y", &private->saved_center_y,
                NULL);

  switch (private->function)
    {
    case RECT_CREATING:
      g_object_set (options,
                    "center-x", (gdouble) x,
                    "center-y", (gdouble) y,
                    NULL);

      g_object_set (rectangle,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
      break;

    case RECT_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x1 - coords->x,
                                          private->y1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x2 - coords->x,
                                          private->y1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x1 - coords->x,
                                          private->y2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x2 - coords->x,
                                          private->y2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x1 - coords->x, 0,
                                          0, 0);
      break;

    case RECT_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x2 - coords->x, 0,
                                          0, 0);
      break;

    case RECT_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, private->y1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, private->y2 - coords->y,
                                          0, 0);
      break;

    case RECT_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          private->x1 - coords->x,
                                          private->y1 - coords->y,
                                          private->x2 - private->x1,
                                          private->y2 - private->y1);
      break;

    default:
      break;
    }

  gimp_tool_control_get_snap_offsets (tool->control,
                                      &snap_x, &snap_y, NULL, NULL);

  x += snap_x;
  y += snap_y;

  private->pressx = x;
  private->pressy = y;

  private->startx = x;
  private->starty = y;
  private->lastx  = x;
  private->lasty  = y;

  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void
gimp_rectangle_tool_button_release (GimpTool        *tool,
                                    GimpCoords      *coords,
                                    guint32          time,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  gboolean                  auto_shrink;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  g_object_get (options,
                "auto-shrink", &auto_shrink,
                NULL);

  if (auto_shrink)
    gimp_rectangle_tool_auto_shrink (rectangle);

  gimp_tool_control_halt (tool->control);

  if (private->function == RECT_EXECUTING)
    gimp_tool_pop_status (tool, display);

  if (! (state & GDK_BUTTON3_MASK))
    {
      if (gimp_rectangle_tool_no_movement (rectangle))
        {
          if (gimp_rectangle_tool_execute (rectangle))
            gimp_rectangle_tool_halt (rectangle);
        }
      else
        {
          g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
        }
    }
  else
    {
      g_object_set (options,
                    "center-x", private->saved_center_x,
                    "center-y", private->saved_center_y,
                    NULL);

      g_object_set (rectangle,
                    "x1", private->saved_x1,
                    "y1", private->saved_y1,
                    "x2", private->saved_x2,
                    "y2", private->saved_y2,
                    NULL);
    }

  gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void
gimp_rectangle_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  gint                      x1, y1, x2, y2;
  gint                      curx, cury;
  gint                      snap_x, snap_y;
  gint                      inc_x, inc_y;
  gboolean                  fixed_width;
  gboolean                  fixed_height;
  gboolean                  fixed_aspect;
  gboolean                  fixed_center;
  gdouble                   width, height;
  gdouble                   center_x, center_y;
  gdouble                   alpha;
  gdouble                   beta;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  /*  This is the only case when the motion events should be ignored --
   *  we're just waiting for the button release event to execute.
   */
  if (private->function == RECT_EXECUTING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  gimp_tool_control_get_snap_offsets (tool->control,
                                      &snap_x, &snap_y, NULL, NULL);
  curx += snap_x;
  cury += snap_y;

  /*  If there have been no changes... return  */
  if (private->lastx == curx && private->lasty == cury)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /* fix function, startx, starty if user has "flipped" the rectangle */
  gimp_rectangle_tool_check_function (rectangle, curx, cury);

  inc_x = curx - private->startx;
  inc_y = cury - private->starty;

  g_object_get (options,
                "fixed-width",  &fixed_width,
                "fixed-height", &fixed_height,
                "fixed-aspect", &fixed_aspect,
                "fixed-center", &fixed_center,
                "width",        &width,
                "height",       &height,
                "center-x",     &center_x,
                "center-y",     &center_y,
                NULL);

  x1 = private->x1;
  y1 = private->y1;
  x2 = private->x2;
  y2 = private->y2;

  switch (private->function)
    {
    case RECT_INACTIVE:
      g_warning ("function is RECT_INACTIVE while mouse is moving");
      break;

    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = private->x1 + inc_x;
      if (fixed_width)
        x2 = x1 + width;
      else if (fixed_center)
        x2 = x1 + 2 * (center_x - x1);
      else
        x2 = MAX (x1, private->x2);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = private->x2 + inc_x;
      if (fixed_width)
        x1 = x2 - width;
      else if (fixed_center)
        x1 = x2 - 2 * (x2 - center_x);
      else
        x1 = MIN (private->x1, x2);
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = private->x1;
      x2 = private->x2;
      break;

    case RECT_MOVING:
      x1 = private->x1 + inc_x;
      x2 = private->x2 + inc_x;
      break;
    }

  switch (private->function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      y1 = private->y1 + inc_y;
      if (fixed_height)
        y2 = y1 + height;
      else if (fixed_center)
        y2 = y1 + 2 * (center_y - y1);
      else
        y2 = MAX (y1, private->y2);
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = private->y2 + inc_y;
      if (fixed_height)
        y1 = y2 - height;
      else if (fixed_center)
        y1 = y2 - 2 * (y2 - center_y);
      else
        y1 = MIN (private->y1, y2);
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = private->y1;
      y2 = private->y2;
      break;

    case RECT_MOVING:
      y1 = private->y1 + inc_y;
      y2 = private->y2 + inc_y;
      break;
    }

  if (fixed_aspect)
    {
      gdouble aspect;
      gdouble numerator, denominator;

      g_object_get (options,
                    "aspect-numerator",   &numerator,
                    "aspect-denominator", &denominator,
                    NULL);
      aspect = CLAMP (numerator / denominator,
                      1.0 / display->image->height,
                      display->image->width);

      switch (private->function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          /* The same basically happens for each corner, just with a
           * different fixed corner. To keep within aspect ratio and
           * at the same time keep the cursor on one edge if not the
           * corner itself: - calculate the two positions of the
           * corner in question on the base of the current mouse
           * cursor position and the fixed corner opposite the one
           * selected.  - decide on which egde we are inside the
           * rectangle dimension - if we are on the inside of the
           * vertical edge then we use the x position of the cursor,
           * otherwise we are on the inside (or close enough) of the
           * horizontal edge and then we use the y position of the
           * cursor for the base of our new corner.
           */
          x1 = private->x2 - (private->y2 - cury) * aspect + 0.5;
          y1 = private->y2 - (private->x2 - curx) / aspect + 0.5;
          if ((y1 < cury) && (cury < y2))
            x1 = curx;
          else
            y1 = cury;
          if (fixed_center)
            {
              x2 = x1 + 2 * (center_x - x1);
              y2 = y1 + 2 * (center_y - y1);
            }
          break;

        case RECT_RESIZING_UPPER_RIGHT:
          x2 = private->x1 + (private->y2 - cury) * aspect + 0.5;
          y1 = private->y2 - (curx - private->x1) / aspect + 0.5;
          if ((y1 < cury) && (cury < y2))
            x2 = curx;
          else
            y1 = cury;
          if (fixed_center)
            {
              x1 = x2 - 2 * (x2 - center_x);
              y2 = y1 + 2 * (center_y - y1);
            }
          break;

        case RECT_RESIZING_LOWER_LEFT:
          x1 = private->x2 - (cury - private->y1) * aspect + 0.5;
          y2 = private->y1 + (private->x2 - curx) / aspect + 0.5;
          if ((y1 < cury) && (cury < y2))
            x1 = curx;
          else
            y2 = cury;
          if (fixed_center)
            {
              x2 = x1 + 2 * (center_x - x1);
              y1 = y2 - 2 * (y2 - center_y);
            }
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          x2 = private->x1 + (cury - private->y1) * aspect + 0.5;
          y2 = private->y1 + (curx - private->x1) / aspect + 0.5;
          if ((y1 < cury) && (cury < y2))
            x2 = curx;
          else
            y2 = cury;
          if (fixed_center)
            {
              x1 = x2 - 2 * (x2 - center_x);
              y1 = y2 - 2 * (y2 - center_y);
            }
          break;

        case RECT_RESIZING_TOP:
          x2 = private->x1 + (private->y2 - y1) * aspect + 0.5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_LEFT:
          /* When resizing the left hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = private->y1 + (private->x2 - x1) / aspect + 0.5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
          break;

        case RECT_RESIZING_BOTTOM:
          x2 = private->x1 + (y2 - private->y1) * aspect + 0.5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_RIGHT:
          /* When resizing the right hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = private->y1 + (x2 - private->x1) / aspect + 0.5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
          break;

        default:
          break;
        }
    }

  private->lastx = curx;
  private->lasty = cury;

  /* Check to see whether the new rectangle obeys the boundary
   * constraints, if any.  If not, see whether we can downscale the
   * mouse movement and call this motion handler again recursively.
   * The reason for the recursive call is to avoid leaving the
   * rectangle edge hanging some pixels away from the constraining
   * boundary if the user moves the pointer quickly.
   */
  if (gimp_rectangle_tool_constraint_violated (rectangle, x1, y1, x2, y2,
                                               &alpha, &beta))
    {
      GimpCoords new_coords;

      inc_x *= alpha;
      inc_y *= beta;

      if (inc_x != 0 || inc_y != 0)
        {
          new_coords.x = private->startx - snap_x + inc_x;
          new_coords.y = private->starty - snap_y + inc_y;

          gimp_rectangle_tool_motion (tool, &new_coords, time, state, display);
        }

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

      return;
    }

  /* set startx, starty according to function, to keep rect on cursor */
  switch (private->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
      private->startx = x1;
      private->starty = y1;
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      private->startx = x2;
      private->starty = y1;
      break;

    case RECT_RESIZING_LOWER_LEFT:
      private->startx = x1;
      private->starty = y2;
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      private->startx = x2;
      private->starty = y2;
      break;

    case RECT_RESIZING_TOP:
      private->startx = curx;
      private->starty = y1;
      break;

    case RECT_RESIZING_LEFT:
      private->startx = x1;
      private->starty = cury;
      break;

    case RECT_RESIZING_BOTTOM:
      private->startx = curx;
      private->starty = y2;
      break;

    case RECT_RESIZING_RIGHT:
      private->startx = x2;
      private->starty = cury;
      break;

    case RECT_MOVING:
      private->startx = curx;
      private->starty = cury;
      break;

    default:
      break;
    }

  /*  make sure that the coords are in bounds  */
  g_object_set (rectangle,
                "x1", MIN (x1, x2),
                "y1", MIN (y1, y2),
                "x2", MAX (x1, x2),
                "y2", MAX (y1, y2),
                NULL);

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  gimp_rectangle_tool_configure (rectangle);

  gimp_rectangle_tool_update_options (rectangle, display);

  if (private->function != RECT_MOVING &&
      private->function != RECT_EXECUTING)
    {
      gint w, h;

      gimp_tool_pop_status (tool, display);

      w = private->x2 - private->x1;
      h = private->y2 - private->y1;

      if (w > 0 && h > 0)
        gimp_tool_push_status_coords (tool, display,
                                      _("Rectangle: "), w, " × ", h);
    }

  if (private->function == RECT_CREATING)
    {
      gint function = RECT_CREATING;

      if (inc_x < 0 && inc_y < 0)
        function = RECT_RESIZING_UPPER_LEFT;
      else if (inc_x < 0 && inc_y > 0)
        function = RECT_RESIZING_LOWER_LEFT;
      else if (inc_x > 0 && inc_y < 0)
        function = RECT_RESIZING_UPPER_RIGHT;
      else if (inc_x > 0 && inc_y > 0)
        function = RECT_RESIZING_LOWER_RIGHT;

      g_object_set (rectangle, "function", function, NULL);
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void
gimp_rectangle_tool_active_modifier_key (GimpTool        *tool,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpRectangleTool    *rectangle;
  GimpRectangleOptions *options;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (key == GDK_SHIFT_MASK)
    {
      gboolean fixed_aspect;

      g_object_get (options,
                    "fixed-aspect", &fixed_aspect,
                    NULL);

      g_object_set (options,
                    "fixed-aspect", ! fixed_aspect,
                    NULL);
    }

  if (key == GDK_CONTROL_MASK)
    {
      gboolean fixed_center;

      g_object_get (options,
                    "fixed-center", &fixed_center,
                    NULL);

      g_object_set (options,
                    "fixed-center", ! fixed_center,
                    NULL);

      if (! fixed_center)
        {
          gint    pressx, pressy;
          gdouble center_x, center_y;

          gimp_rectangle_tool_get_press_coords (rectangle, &pressx, &pressy);

          center_x = pressx;
          center_y = pressy;

          g_object_set (options,
                        "center-x", center_x,
                        "center-y", center_y,
                        NULL);
        }
    }
}

/*
 * gimp_rectangle_tool_check_function() is needed to deal with
 * situations where the user drags a corner or edge across one of the
 * existing edges, thereby changing its function.  Ugh.
 */
static void
gimp_rectangle_tool_check_function (GimpRectangleTool *rectangle,
                                    gint               curx,
                                    gint               cury)
{
  GimpRectangleToolPrivate *private;
  guint                     function;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  function = private->function;

  switch (function)
    {
    case RECT_RESIZING_LEFT:
      if (curx > private->x2)
        {
          function = RECT_RESIZING_RIGHT;
          private->startx = private->x2;
        }
      break;

    case RECT_RESIZING_RIGHT:
      if (curx < private->x1)
        {
          function = RECT_RESIZING_LEFT;
          private->startx = private->x1;
        }
      break;

    case RECT_RESIZING_TOP:
      if (cury > private->y2)
        {
          function = RECT_RESIZING_BOTTOM;
          private->starty = private->y2;
        }
      break;

    case RECT_RESIZING_BOTTOM:
      if (cury < private->y1)
        {
          function = RECT_RESIZING_TOP;
          private->starty = private->y1;
        }
      break;

    case RECT_RESIZING_UPPER_LEFT:
      if (curx > private->x2 && cury > private->y2)
        {
          function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = private->x2;
          private->starty = private->y2;
        }
      else if (curx > private->x2)
        {
          function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = private->x2;
        }
      else if (cury > private->y2)
        {
          function = RECT_RESIZING_LOWER_LEFT;
          private->starty = private->y2;
        }
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      if (curx < private->x1 && cury > private->y2)
        {
          function = RECT_RESIZING_LOWER_LEFT;
          private->startx = private->x1;
          private->starty = private->y2;
        }
      else if (curx < private->x1)
        {
          function = RECT_RESIZING_UPPER_LEFT;
          private->startx = private->x1;
        }
      else if (cury > private->y2)
        {
          function = RECT_RESIZING_LOWER_RIGHT;
          private->starty = private->y2;
        }
      break;

    case RECT_RESIZING_LOWER_LEFT:
      if (curx > private->x2 && cury < private->y1)
        {
          function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = private->x2;
          private->starty = private->y1;
        }
      else if (curx > private->x2)
        {
          function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = private->x2;
        }
      else if (cury < private->y1)
        {
          function = RECT_RESIZING_UPPER_LEFT;
          private->starty = private->y1;
        }
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      if (curx < private->x1 && cury < private->y1)
        {
          function = RECT_RESIZING_UPPER_LEFT;
          private->startx = private->x1;
          private->starty = private->y1;
        }
      else if (curx < private->x1)
        {
          function = RECT_RESIZING_LOWER_LEFT;
          private->startx = private->x1;
        }
      else if (cury < private->y1)
        {
          function = RECT_RESIZING_UPPER_RIGHT;
          private->starty = private->y1;
        }
      break;

    default:
      break;
    }

  if (function != private->function)
    g_object_set (rectangle, "function", function, NULL);
}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  gint                      inc_x, inc_y;
  gint                      min_x, min_y;
  gint                      max_x, max_y;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (display != tool->display)
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
      if (gimp_rectangle_tool_execute (rectangle))
        gimp_rectangle_tool_halt (rectangle);
      return TRUE;

    case GDK_Escape:
      gimp_rectangle_tool_cancel (rectangle);
      gimp_rectangle_tool_halt (rectangle);
      return TRUE;

    default:
      g_print ("Key %d pressed\n", kevent->keyval);
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
  max_x = display->image->width;
  max_y = display->image->height;

  g_object_set (rectangle,
                "x1", private->x1 + inc_x,
                "y1", private->y1 + inc_y,
                "x2", private->x2 + inc_x,
                "y2", private->y2 + inc_y,
                NULL);

  gimp_rectangle_tool_configure (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);

  return TRUE;
}

void
gimp_rectangle_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpDrawTool             *draw_tool;
  GimpDisplayShell         *shell;
  gboolean                  inside_x;
  gboolean                  inside_y;
  gdouble                   handle_w;
  gdouble                   handle_h;
  gint                      function;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  if (tool->display != display)
    {
      g_object_set (rectangle, "function", RECT_CREATING, NULL);
      return;
    }

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  handle_w = private->dcw / SCALEFACTOR_X (shell);
  handle_h = private->dch / SCALEFACTOR_Y (shell);

  inside_x = coords->x > private->x1 && coords->x < private->x2;
  inside_y = coords->y > private->y1 && coords->y < private->y2;

  if (gimp_draw_tool_on_handle (draw_tool, display,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                private->x1, private->y1,
                                private->dcw, private->dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      function = RECT_RESIZING_UPPER_LEFT;
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     private->x2, private->y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      function = RECT_RESIZING_LOWER_RIGHT;
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, display,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      private->x2, private->y1,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      function = RECT_RESIZING_UPPER_RIGHT;
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     private->x1, private->y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      function = RECT_RESIZING_LOWER_LEFT;
    }
  else if ((fabs (coords->x - private->x1) < handle_w) && inside_x)
    {
      function = RECT_RESIZING_LEFT;
    }
  else if ((fabs (coords->x - private->x2) < handle_w) && inside_x)
    {
      function = RECT_RESIZING_RIGHT;
    }
  else if ((fabs (coords->y - private->y1) < handle_h) && inside_y)
    {
      function = RECT_RESIZING_TOP;
    }
  else if ((fabs (coords->y - private->y2) < handle_h) && inside_y)
    {
      function = RECT_RESIZING_BOTTOM;
    }
  else if (inside_x && inside_y)
    {
      function = RECT_MOVING;
    }
  else
    {
      /*  otherwise, the new function will be creating, since we want
       *  to start a new rectangle
       */
      function = RECT_CREATING;
    }

  g_object_set (rectangle, "function", function, NULL);
}

void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpCursorType            cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier        modifier = GIMP_CURSOR_MODIFIER_NONE;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (tool->display == display)
    {
      switch (private->function)
        {
        case RECT_CREATING:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
        case RECT_MOVING:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
          break;
        case RECT_RESIZING_UPPER_LEFT:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;
        case RECT_RESIZING_UPPER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;
        case RECT_RESIZING_LOWER_LEFT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;
        case RECT_RESIZING_LOWER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;
        case RECT_RESIZING_LEFT:
          cursor = GIMP_CURSOR_SIDE_LEFT;
          break;
        case RECT_RESIZING_RIGHT:
          cursor = GIMP_CURSOR_SIDE_RIGHT;
          break;
        case RECT_RESIZING_TOP:
          cursor = GIMP_CURSOR_SIDE_TOP;
          break;
        case RECT_RESIZING_BOTTOM:
          cursor = GIMP_CURSOR_SIDE_BOTTOM;
          break;

        default:
          break;
        }
    }

  gimp_tool_control_set_cursor          (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);
}

void
gimp_rectangle_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (draw_tool));

  tool    = GIMP_TOOL (draw_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (draw_tool);

  if (private->function == RECT_INACTIVE)
    return;

  gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                 private->x1,
                                 private->y1,
                                 private->x2 - private->x1,
                                 private->y2 - private->y1,
                                 FALSE);

  if (gimp_tool_control_is_active (tool->control))
    {
      GimpDisplayShell *shell    = GIMP_DISPLAY_SHELL (tool->display->shell);
      gdouble           handle_w = private->dcw / SCALEFACTOR_X (shell);
      gdouble           handle_h = private->dch / SCALEFACTOR_Y (shell);
      gdouble           x1       = private->x1;
      gdouble           x2       = private->x2;
      gdouble           y1       = private->y1;
      gdouble           y2       = private->y2;

      switch (private->function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          gimp_draw_tool_draw_corner (draw_tool,
                                      private->x1, private->y1,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_NORTH_WEST, FALSE);
          break;

        case RECT_RESIZING_UPPER_RIGHT:
          gimp_draw_tool_draw_corner (draw_tool,
                                      private->x2, private->y1,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_NORTH_EAST, FALSE);
          break;

        case RECT_RESIZING_LOWER_LEFT:
          gimp_draw_tool_draw_corner (draw_tool,
                                      private->x1, private->y2,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_SOUTH_WEST, FALSE);
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          gimp_draw_tool_draw_corner (draw_tool,
                                      private->x2, private->y2,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_SOUTH_EAST, FALSE);
          break;

        case RECT_RESIZING_LEFT:
          x1 = x2 = private->x1 + handle_w;
          gimp_draw_tool_draw_line (draw_tool, x1, y1, x2, y2, FALSE);
          break;

        case RECT_RESIZING_RIGHT:
          x1 = x2 = private->x2 - handle_w;
          gimp_draw_tool_draw_line (draw_tool, x1, y1, x2, y2, FALSE);
          break;

        case RECT_RESIZING_TOP:
          y1 = y2 = private->y1 + handle_h;
          gimp_draw_tool_draw_line (draw_tool, x1, y1, x2, y2, FALSE);
          break;

        case RECT_RESIZING_BOTTOM:
          y1 = y2 = private->y2 - handle_h;
          gimp_draw_tool_draw_line (draw_tool, x1, y1, x2, y2, FALSE);
          break;
        }
    }
  else
    {
      gimp_draw_tool_draw_corner (draw_tool,
                                  private->x1, private->y1,
                                  private->dcw, private->dch,
                                  GTK_ANCHOR_NORTH_WEST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool,
                                  private->x2, private->y1,
                                  private->dcw, private->dch,
                                  GTK_ANCHOR_NORTH_EAST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool,
                                  private->x1, private->y2,
                                  private->dcw, private->dch,
                                  GTK_ANCHOR_SOUTH_WEST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool,
                                  private->x2, private->y2,
                                  private->dcw, private->dch,
                                  GTK_ANCHOR_SOUTH_EAST, FALSE);
    }

  gimp_rectangle_tool_draw_guides (draw_tool);
}

static void
gimp_rectangle_tool_draw_guides (GimpDrawTool *draw_tool)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;
  gint                      x1, x2, y1, y2;

  tool    = GIMP_TOOL (draw_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (draw_tool);

  x1 = private->x1;
  x2 = private->x2;
  y1 = private->y1;
  y2 = private->y2;

  switch (private->guide)
    {
    case GIMP_RECTANGLE_GUIDE_NONE:
      break;

    case GIMP_RECTANGLE_GUIDE_CENTER_LINES:
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (y1 + y2) / 2,
                                x2, (y1 + y2) / 2, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (x1 + x2) / 2, y1,
                                (x1 + x2) / 2, y2, FALSE);
      break;

    case GIMP_RECTANGLE_GUIDE_THIRDS:
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (2 * y1 + y2) / 3,
                                x2, (2 * y1 + y2) / 3, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                x1, (y1 + 2 * y2) / 3,
                                x2, (y1 + 2 * y2) / 3, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (2 * x1 + x2) / 3, y1,
                                (2 * x1 + x2) / 3, y2, FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (x1 + 2 * x2) / 3, y1,
                                (x1 + 2 * x2) / 3, y2, FALSE);
      break;

    case GIMP_RECTANGLE_GUIDE_GOLDEN:
      gimp_draw_tool_draw_line (draw_tool,
                                x1,
                                (2 * y1 + (1 + sqrt(5)) * y2) / (3 + sqrt(5)),
                                x2,
                                (2 * y1 + (1 + sqrt(5)) * y2) / (3 + sqrt(5)),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                x1,
                                ((1 + sqrt(5)) * y1 + 2 * y2) / (3 + sqrt(5)),
                                x2,
                                ((1 + sqrt(5)) * y1 + 2 * y2) / (3 + sqrt(5)),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (2 * x1 + (1 + sqrt(5)) * x2) / (3 + sqrt(5)),
                                y1,
                                (2 * x1 + (1 + sqrt(5)) * x2) / (3 + sqrt(5)),
                                y2,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                ((1 + sqrt(5)) * x1 + 2 * x2) / (3 + sqrt(5)),
                                y1,
                                ((1 + sqrt(5)) * x1 + 2 * x2) / (3 + sqrt(5)),
                                y2, FALSE);
      break;
    }
}

void
gimp_rectangle_tool_configure (GimpRectangleTool *rectangle)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  GimpDisplayShell         *shell;
  gint                      dx1, dx2;
  gint                      dy1, dy2;
  gboolean                  highlight;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (! tool->display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (options, "highlight", &highlight, NULL);

  if (highlight)
    {
      GdkRectangle rect;

      rect.x      = private->x1;
      rect.y      = private->y1;
      rect.width  = private->x2 - private->x1;
      rect.height = private->y2 - private->y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   private->x1, private->y1,
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   private->x2, private->y2,
                                   &dx2, &dy2,
                                   FALSE);

  private->dcw = CLAMP ((dx2 - dx1) / 3, MIN_HANDLE_SIZE, HANDLE_SIZE);
  private->dch = CLAMP ((dy2 - dy1) / 3, MIN_HANDLE_SIZE, HANDLE_SIZE);
}

static void
gimp_rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_rectangle_tool_configure (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->display,
                                _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->display);
}

static void
gimp_rectangle_tool_halt (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->display->shell),
                                    NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  tool->display  = NULL;
  tool->drawable = NULL;

  g_object_set (rectangle, "function", RECT_INACTIVE, NULL);
}

gboolean
gimp_rectangle_tool_execute (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *iface;
  gboolean                    retval = FALSE;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  if (iface->execute)
    {
      GimpRectangleToolPrivate *private;

      private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      retval = iface->execute (rectangle,
                               private->x1,
                               private->y1,
                               private->x2 - private->x1,
                               private->y2 - private->y1);

      gimp_rectangle_tool_configure (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }

  return retval;
}

void
gimp_rectangle_tool_cancel (GimpRectangleTool *rectangle)
{
  GimpRectangleToolInterface *iface;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rectangle);

  if (iface->cancel)
    iface->cancel (rectangle);
}

static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rectangle,
                                    GimpDisplay       *display)
{
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  gdouble                   x;
  gdouble                   y;
  gdouble                   width;
  gdouble                   height;
  gdouble                   center_x, center_y;
  gboolean                  fixed_width;
  gboolean                  fixed_height;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle);

  g_object_get (options,
                "fixed-width",  &fixed_width,
                "fixed-height", &fixed_height,
                NULL);

  x      = private->x1;
  y      = private->y1;
  width  = private->x2 - private->x1;
  height = private->y2 - private->y1;

  center_x = (private->x1 + private->x2) / 2.0;
  center_y = (private->y1 + private->y2) / 2.0;

  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_options_notify,
                                   rectangle);

  g_object_set (options,
                "x0", x,
                "y0", y,
                NULL);

  if (! fixed_width)
    g_object_set (options,
                  "width",  width,
                  NULL);

  if (! fixed_height)
    g_object_set (options,
                  "height", height,
                  NULL);

  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_options_notify,
                                     rectangle);

  g_object_set (options,
                "center-x", center_x,
                "center-y", center_y,
                NULL);
}

static void
gimp_rectangle_tool_synthesize_motion (GimpTool   *tool,
                                       gint        function,
                                       gint        startx,
                                       gint        starty,
                                       GimpCoords *coords)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  g_object_set (tool, "function", function, NULL);

  private->startx = startx;
  private->starty = starty;

  gimp_rectangle_tool_motion (tool, coords, 0, 0, tool->display);

  g_signal_emit_by_name (tool, "rectangle-changed", NULL);
}

static void
gimp_rectangle_tool_options_notify (GimpRectangleOptions *options,
                                    GParamSpec           *pspec,
                                    GimpRectangleTool    *rectangle)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! strcmp (pspec->name, "guide"))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      g_object_get (options,
                    "guide", &private->guide,
                    NULL);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));

      return;
    }

  if (! tool->display)
    return;

  if (! strcmp (pspec->name, "x0"))
    {
      GimpCoords coords;
      gdouble    x;

      g_object_get (options, "x0", &x, NULL);

      coords.x = x;
      coords.y = private->y1;

      gimp_rectangle_tool_synthesize_motion (tool,
                                             RECT_RESIZING_LEFT,
                                             private->x1,
                                             private->y1,
                                             &coords);
    }
  else if (! strcmp (pspec->name, "y0"))
    {
      GimpCoords coords;
      gdouble    y;

      g_object_get (options, "y0", &y, NULL);

      coords.x = private->x1;
      coords.y = y;

      gimp_rectangle_tool_synthesize_motion (tool,
                                             RECT_RESIZING_TOP,
                                             private->x1,
                                             private->y1,
                                             &coords);
    }
  else if (! strcmp (pspec->name, "width"))
    {
      GimpCoords coords;
      gdouble    width;

      g_object_get (options, "width", &width, NULL);

      coords.x = private->x1 + width;
      coords.y = private->y2;

      gimp_rectangle_tool_synthesize_motion (tool,
                                             RECT_RESIZING_RIGHT,
                                             private->x2,
                                             private->y2,
                                             &coords);
    }
  else if (! strcmp (pspec->name, "height"))
    {
      GimpCoords coords;
      gdouble    height;

      g_object_get (options, "height", &height, NULL);

      coords.x = private->x2;
      coords.y = private->y1 + height;

      gimp_rectangle_tool_synthesize_motion (tool,
                                             RECT_RESIZING_BOTTOM,
                                             private->x2,
                                             private->y2,
                                             &coords);
    }
  else if (! strcmp (pspec->name, "fixed-aspect")     ||
           ! strcmp (pspec->name, "aspect-numerator") ||
           ! strcmp (pspec->name, "aspect-denominator"))
    {
      gdouble  numerator, denominator;
      gboolean fixed_aspect;

      g_object_get (options,
                    "aspect-numerator",   &numerator,
                    "aspect-denominator", &denominator,
                    "fixed-aspect",       &fixed_aspect,
                    NULL);

      if (fixed_aspect)
        {
          GimpCoords coords;
          gdouble    aspect;

          aspect = numerator / denominator;

          coords.x = private->x2;
          coords.y = private->y1 + (private->x2 - private->x1) / aspect;

          gimp_rectangle_tool_synthesize_motion (tool,
                                                 RECT_RESIZING_BOTTOM,
                                                 private->x2,
                                                 private->y2,
                                                 &coords);
        }
    }
  else if (! strcmp (pspec->name, "highlight"))
    {
      GimpDisplayShell *shell;
      gboolean          highlight;

      shell = GIMP_DISPLAY_SHELL (tool->display->shell);

      g_object_get (options, "highlight", &highlight, NULL);

      if (highlight)
        {
          GdkRectangle rect;

          rect.x      = private->x1;
          rect.y      = private->y1;
          rect.width  = private->x2 - private->x1;
          rect.height = private->y2 - private->y1;

          gimp_display_shell_set_highlight (shell, &rect);
        }
      else
        {
          gimp_display_shell_set_highlight (shell, NULL);
        }
    }
}

gboolean
gimp_rectangle_tool_no_movement (GimpRectangleTool *rectangle)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  return (private->lastx == private->pressx &&
          private->lasty == private->pressy);
}

/*
 * check whether the coordinates extend outside the bounds of the image
 * or active drawable, if it is constrained not to.  If it does,
 * caculate how much the current movement needs to be downscaled in
 * order to obey the constraint.
 */
static gboolean
gimp_rectangle_tool_constraint_violated (GimpRectangleTool *rectangle,
                                         gint               x1,
                                         gint               y1,
                                         gint               x2,
                                         gint               y2,
                                         gdouble           *alpha,
                                         gdouble           *beta)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;
  GimpRectangleConstraint   constraint;
  GimpImage                *image;
  gint                      min_x, min_y;
  gint                      max_x, max_y;

  private    = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);
  constraint = gimp_rectangle_tool_get_constraint (rectangle);
  image      = tool->display->image;

  *alpha = *beta = 1;

  switch (constraint)
    {
    case GIMP_RECTANGLE_CONSTRAIN_NONE:
      return FALSE;

    case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
      min_x = 0;
      min_y = 0;
      max_x = image->width;
      max_y = image->height;
      break;

    case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
      {
        GimpItem *item = GIMP_ITEM (tool->drawable);

        gimp_item_offsets (item, &min_x, &min_y);
        max_x = min_x + gimp_item_width (item);
        max_y = min_y + gimp_item_height (item);
      }
      break;

    default:
      g_warning ("Invalid rectangle constraint.\n");
      return FALSE;
    }

  if (x1 < min_x)
    {
      *alpha = (private->x1 - min_x) / (gdouble) (private->x1 - x1);
      return TRUE;
    }

  if (y1 < min_y)
    {
      *beta = (private->y1 - min_y) / (gdouble) (private->y1 - y1);
      return TRUE;
    }

  if (x2 > max_x)
    {
      *alpha = (max_x - private->x2) / (gdouble) (x2 - private->x2);
      return TRUE;
    }

  if (y2 > max_y)
    {
      *beta = (max_y - private->y2) / (gdouble) (y2 - private->y2);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_rectangle_tool_auto_shrink (GimpRectangleTool *rectangle)
{
  GimpTool                 *tool     = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;
  GimpDisplay              *display  = tool->display;
  gint                      width;
  gint                      height;
  gint                      offset_x = 0;
  gint                      offset_y = 0;
  gint                      x1, y1;
  gint                      x2, y2;
  gint                      shrunk_x1;
  gint                      shrunk_y1;
  gint                      shrunk_x2;
  gint                      shrunk_y2;
  gboolean                  shrink_merged;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! display)
    return;

  width  = display->image->width;
  height = display->image->height;

  g_object_get (gimp_tool_get_options (tool),
                "shrink-merged", &shrink_merged,
                NULL);

  x1 = private->x1 - offset_x  > 0      ? private->x1 - offset_x : 0;
  x2 = private->x2 - offset_x  < width  ? private->x2 - offset_x : width;
  y1 = private->y1 - offset_y  > 0      ? private->y1 - offset_y : 0;
  y2 = private->y2 - offset_y  < height ? private->y2 - offset_y : height;

  if (gimp_image_crop_auto_shrink (display->image,
                                   x1, y1, x2, y2,
                                   ! shrink_merged,
                                   &shrunk_x1,
                                   &shrunk_y1,
                                   &shrunk_x2,
                                   &shrunk_y2))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      g_object_set (rectangle,
                    "x1", offset_x + shrunk_x1,
                    "y1", offset_y + shrunk_y1,
                    "x2", offset_x + shrunk_x2,
                    "y2", offset_y + shrunk_y2,
                    NULL);

      gimp_rectangle_tool_configure (rectangle);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
    }
}
