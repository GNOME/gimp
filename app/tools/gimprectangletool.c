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
#define ARROW_VELOCITY 25


#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) \
  (gimp_rectangle_tool_get_private ((GimpRectangleTool *) (obj)))


typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  gint                   pressx;     /*  x where button pressed         */
  gint                   pressy;     /*  y where button pressed         */

  gint                   x1, y1;     /*  upper left hand coordinate     */
  gint                   x2, y2;     /*  lower right hand coords        */

  guint                  function;   /*  moving or resizing             */

  GimpRectangleConstraint constraint; /* how to constrain rectangle     */

  /* Internal state */
  gint                   startx;     /*  starting x coord               */
  gint                   starty;     /*  starting y coord               */

  gint                   lastx;      /*  previous x coord               */
  gint                   lasty;      /*  previous y coord               */

  gint                   dx1, dy1;   /*  display coords                 */
  gint                   dx2, dy2;   /*                                 */

  gint                   dcw, dch;   /*  width and height of edges      */

  gint                   saved_x1;   /*  for saving in case action is canceled */
  gint                   saved_y1;
  gint                   saved_x2;
  gint                   saved_y2;
  gdouble                saved_center_x;
  gdouble                saved_center_y;

  GimpRectangleGuide     guide; /* synced with options->guide, only exists for drawing */
};


static void gimp_rectangle_tool_iface_base_init     (GimpRectangleToolInterface *iface);

static GimpRectangleToolPrivate *
            gimp_rectangle_tool_get_private         (GimpRectangleTool *tool);

void        gimp_rectangle_tool_set_function        (GimpRectangleTool *tool,
                                                     guint              function);
guint       gimp_rectangle_tool_get_function        (GimpRectangleTool *tool);

GimpRectangleConstraint
            gimp_rectangle_tool_get_constraint      (GimpRectangleTool *tool);

/*  Rectangle helper functions  */
static void     rectangle_tool_start                (GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_draw_guides     (GimpDrawTool          *draw_tool);

/*  Rectangle dialog functions  */
static void     gimp_rectangle_tool_update_options  (GimpRectangleTool     *rectangle,
                                                     GimpDisplay           *display);

static void     gimp_rectangle_tool_notify_x          (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_y          (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_width      (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_height     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_aspect     (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_highlight  (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_notify_guide      (GimpRectangleOptions  *options,
                                                       GParamSpec            *pspec,
                                                       GimpRectangleTool     *rectangle);
static void     gimp_rectangle_tool_check_function    (GimpRectangleTool     *rectangle,
                                                       gint                   curx,
                                                       gint                   cury);
gboolean        gimp_rectangle_tool_constraint_violated (GimpRectangleTool   *rectangle,
                                                         gint                 x1,
                                                         gint                 y1,
                                                         gint                 x2,
                                                         gint                 y2,
                                                         gdouble             *alpha,
                                                         gdouble             *beta);
static void     gimp_rectangle_tool_auto_shrink_notify (GimpRectangleOptions *options,
                                                        GParamSpec           *pspec,
                                                        GimpRectangleTool    *rectangle);

static void     gimp_rectangle_tool_auto_shrink       (GimpRectangleTool     *rectangle);

static guint gimp_rectangle_tool_signals[LAST_SIGNAL] = { 0 };


GType
gimp_rectangle_tool_interface_get_type (void)
{
  static GType iface_type = 0;

  if (!iface_type)
    {
      static const GTypeInfo iface_info =
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
gimp_rectangle_tool_set_function (GimpRectangleTool *tool,
                                  guint              function)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->function = function;

  g_object_notify (G_OBJECT (tool), "function");
}

guint
gimp_rectangle_tool_get_function (GimpRectangleTool *tool)
{
  GimpRectangleToolPrivate *private;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), 0);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  return private->function;
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
      gimp_rectangle_tool_set_function (rectangle, g_value_get_uint (value));
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
      g_value_set_uint (value, gimp_rectangle_tool_get_function (rectangle));
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
  GimpTool          *tool      = GIMP_TOOL (object);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);
  GObject           *options;

  tool->display = NULL;

  options = G_OBJECT (gimp_tool_get_options (tool));

  g_signal_connect_object (options, "notify::x0",
                           G_CALLBACK (gimp_rectangle_tool_notify_x),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::y0",
                           G_CALLBACK (gimp_rectangle_tool_notify_y),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::width",
                           G_CALLBACK (gimp_rectangle_tool_notify_width),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::height",
                           G_CALLBACK (gimp_rectangle_tool_notify_height),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::fixed-aspect",
                           G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::aspect-numerator",
                           G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::aspect-denominator",
                           G_CALLBACK (gimp_rectangle_tool_notify_aspect),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::highlight",
                           G_CALLBACK (gimp_rectangle_tool_notify_highlight),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::guide",
                           G_CALLBACK (gimp_rectangle_tool_notify_guide),
                           rectangle, 0);
  g_signal_connect_object (options, "notify::auto-shrink",
                           G_CALLBACK (gimp_rectangle_tool_auto_shrink_notify),
                           rectangle, 0);

  gimp_rectangle_tool_set_constraint (rectangle, GIMP_RECTANGLE_CONSTRAIN_NONE);
}

void
gimp_rectangle_tool_dispose (GObject *object)
{
  GimpTool          *tool      = GIMP_TOOL (object);
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (object);
  GObject           *options   = G_OBJECT (gimp_tool_get_options (tool));

  g_signal_handlers_disconnect_matched (options, G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, rectangle);
}

gboolean
gimp_rectangle_tool_initialize (GimpTool    *tool,
                                GimpDisplay *display)
{
  GimpRectangleToolPrivate *private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  GObject                  *options = G_OBJECT (gimp_tool_get_options (tool));

  g_object_get (options,
                "guide",            &private->guide,
                NULL);

  return TRUE;
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
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);
  guint                     function;
  GimpRectangleToolPrivate *private;
  gint                      x         = ROUND (coords->x);
  gint                      y         = ROUND (coords->y);
  GimpRectangleOptions     *options;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      function = RECT_CREATING;
      g_object_set (rectangle, "function", function,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

      tool->display = display;
      rectangle_tool_start (rectangle);
    }

  /* save existing shape in case of cancellation */
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  g_object_get (rectangle,
                "x1", &private->saved_x1,
                "y1", &private->saved_y1,
                "x2", &private->saved_x2,
                "y2", &private->saved_y2,
                NULL);
  g_object_get (options,
                "center-x", &private->saved_center_x,
                "center-y", &private->saved_center_y,
                NULL);

  g_object_get (rectangle,
                "function", &function,
                NULL);

  if (function == RECT_CREATING)
    {
      g_object_set (options,
                    "center-x", (gdouble) x,
                    "center-y", (gdouble) y,
                    NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      g_object_set (rectangle,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }

  private->pressx = x;
  private->pressy = y;

  private->startx = x;
  private->starty = y;
  private->lastx = x;
  private->lasty = y;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
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
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  guint                     function;
  GimpRectangleOptions     *options;
  gboolean                  auto_shrink;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  g_object_get (options,
                "auto-shrink", &auto_shrink,
                NULL);

  if (auto_shrink)
    gimp_rectangle_tool_auto_shrink (rectangle);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));
  gimp_tool_control_halt (tool->control);
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  g_object_get (rectangle, "function", &function, NULL);

  if (function == RECT_EXECUTING)
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

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

      g_object_set (rectangle,
                    "x1", private->saved_x1,
                    "y1", private->saved_y1,
                    "x2", private->saved_x2,
                    "y2", private->saved_y2,
                    NULL);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
    }
}

void
gimp_rectangle_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  guint                     function;
  gint                      x1, y1, x2, y2;
  gint                      curx, cury;
  gint                      inc_x, inc_y;
  gint                      rx1, ry1, rx2, ry2;
  gboolean                  fixed_width;
  gboolean                  fixed_height;
  gboolean                  fixed_aspect;
  gboolean                  fixed_center;
  gdouble                   width, height;
  gdouble                   center_x, center_y;
  gdouble                   alpha;
  gdouble                   beta;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  /*  This is the only case when the motion events should be ignored --
   *  we're just waiting for the button release event to execute.
   */
  g_object_get (rectangle, "function", &function, NULL);

  if (function == RECT_EXECUTING)
    return;

  curx = ROUND (coords->x);
  cury = ROUND (coords->y);

  /* fix function, startx, starty if user has "flipped" the rectangle */
  gimp_rectangle_tool_check_function (rectangle, curx, cury);

  inc_x = curx - private->startx;
  inc_y = cury - private->starty;

  /*  If there have been no changes... return  */
  if (private->lastx == curx && private->lasty == cury)
    return;

  g_object_get (options,
                "fixed-width",      &fixed_width,
                "fixed-height",     &fixed_height,
                "fixed-aspect",     &fixed_aspect,
                "fixed-center",     &fixed_center,
                NULL);

  g_object_get (options,
                "width", &width,
                "height", &height,
                "center-x", &center_x,
                "center-y", &center_y,
                NULL);

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);
  x1 = rx1;
  y1 = ry1;
  x2 = rx2;
  y2 = ry2;

  switch (function)
    {
    case RECT_INACTIVE:
      g_warning ("function is RECT_INACTIVE while mouse is moving");
      break;

    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      x1 = rx1 + inc_x;
      if (fixed_width)
        x2 = x1 + width;
      else if (fixed_center)
        x2 = x1 + 2 * (center_x - x1);
      else
        x2 = MAX (x1, rx2);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      x2 = rx2 + inc_x;
      if (fixed_width)
        x1 = x2 - width;
      else if (fixed_center)
        x1 = x2 - 2 * (x2 - center_x);
      else
        x1 = MIN (rx1, x2);
      break;

    case RECT_RESIZING_BOTTOM:
    case RECT_RESIZING_TOP:
      x1 = rx1;
      x2 = rx2;
      break;

    case RECT_MOVING:
      x1 = rx1 + inc_x;
      x2 = rx2 + inc_x;
      break;
    }

  switch (function)
    {
    case RECT_CREATING:
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      y1 = ry1 + inc_y;
      if (fixed_height)
        y2 = y1 + height;
      else if (fixed_center)
        y2 = y1 + 2 * (center_y - y1);
      else
        y2 = MAX (y1, ry2);
      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      y2 = ry2 + inc_y;
      if (fixed_height)
        y1 = y2 - height;
      else if (fixed_center)
        y1 = y2 - 2 * (y2 - center_y);
      else
        y1 = MIN (ry1, y2);
      break;

    case RECT_RESIZING_RIGHT:
    case RECT_RESIZING_LEFT:
      y1 = ry1;
      y2 = ry2;
      break;

    case RECT_MOVING:
      y1 = ry1 + inc_y;
      y2 = ry2 + inc_y;
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

      switch (function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          /*
           * The same basically happens for each corner, just with a
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
          x1 = rx2 - (ry2 - cury) * aspect + .5;
          y1 = ry2 - (rx2 - curx) / aspect + .5;
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
          x2 = rx1 + (ry2 - cury) * aspect + .5;
          y1 = ry2 - (curx - rx1) / aspect + .5;
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
          x1 = rx2 - (cury - ry1) * aspect + .5;
          y2 = ry1 + (rx2 - curx) / aspect + .5;
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
          x2 = rx1 + (cury - ry1) * aspect + .5;
          y2 = ry1 + (curx - rx1) / aspect + .5;
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
          x2 = rx1 + (ry2 - y1) * aspect + .5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_LEFT:
          /* When resizing the left hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = ry1 + (rx2 - x1) / aspect + .5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
          break;

        case RECT_RESIZING_BOTTOM:
          x2 = rx1 + (y2 - ry1) * aspect + .5;
          if (fixed_center)
            x1 = x2 - 2 * (x2 - center_x);
          break;

        case RECT_RESIZING_RIGHT:
          /* When resizing the right hand delimiter then the aspect
           * dictates the height of the result, any inc_y is redundant
           * and not relevant to the result
           */
          y2 = ry1 + (x2 - rx1) / aspect + 0.5;
          if (fixed_center)
            y1 = y2 - 2 * (y2 - center_y);
          break;

        default:
          break;
        }
    }

  private->lastx = curx;
  private->lasty = cury;

  /*
   * Check to see whether the new rectangle obeys the boundary constraints, if any.
   * If not, see whether we can downscale the mouse movement and call this
   * motion handler again recursively.  The reason for the recursive call is
   * to avoid leaving the rectangle edge hanging some pixels away from the
   * constraining boundary if the user moves the pointer quickly.
   */
  if (gimp_rectangle_tool_constraint_violated (rectangle, x1, y1, x2, y2, &alpha, &beta))
    {
      GimpCoords new_coords;

      inc_x *= alpha;
      inc_y *= beta;

      if (inc_x != 0 || inc_y != 0)
        {
          new_coords.x = private->startx + inc_x;
          new_coords.y = private->starty + inc_y;

          gimp_rectangle_tool_motion (tool, &new_coords, time, state, display);
        }
      return;
    }

  /* set startx, starty according to function, to keep rect on cursor */
  switch (function)
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

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  /*  make sure that the coords are in bounds  */
  g_object_set (rectangle,
                "x1", MIN (x1, x2),
                "y1", MIN (y1, y2),
                "x2", MAX (x1, x2),
                "y2", MAX (y1, y2),
                NULL);

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  gimp_rectangle_tool_configure (rectangle);

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  switch (function)
    {
    case RECT_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x,
                                          ry1 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x,
                                          ry2 - coords->y,
                                          0, 0);
      break;

    case RECT_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx2 - coords->x, 0, 0, 0);
      break;

    case RECT_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, ry1 - coords->y, 0, 0);
      break;

    case RECT_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, ry2 - coords->y, 0, 0);
      break;

    case RECT_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          rx1 - coords->x,
                                          ry1 - coords->y,
                                          rx2 - rx1,
                                          ry2 - ry1);
      break;

    default:
      break;
    }

  gimp_rectangle_tool_update_options (rectangle, display);

  if (function != RECT_MOVING && function != RECT_EXECUTING)
    {
      gint w, h;

      gimp_tool_pop_status (tool, display);

      w = rx2 - rx1;
      h = ry2 - ry1;

      if (w > 0 && h > 0)
        gimp_tool_push_status_coords (tool, display,
                                      _("Rectangle: "), w, " × ", h);
    }

  if (function == RECT_CREATING)
    {
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
  guint                     new_function;
  gint                      x1, y1, x2, y2;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  g_object_get (rectangle,
                "function", &function,
                "x1",       &x1,
                "y1",       &y1,
                "x2",       &x2,
                "y2",       &y2,
                NULL);

  new_function = function;

  switch (function)
    {
    case RECT_RESIZING_LEFT:
      if (curx > x2)
        {
          new_function = RECT_RESIZING_RIGHT;
          private->startx = x2;
        }
      break;

    case RECT_RESIZING_RIGHT:
      if (curx < x1)
        {
          new_function = RECT_RESIZING_LEFT;
          private->startx = x1;
        }
      break;

    case RECT_RESIZING_TOP:
      if (cury > y2)
        {
          new_function = RECT_RESIZING_BOTTOM;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_BOTTOM:
      if (cury < y1)
        {
          new_function = RECT_RESIZING_TOP;
          private->starty = y1;
        }
      break;

    case RECT_RESIZING_UPPER_LEFT:
      if (curx > x2 && cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = x2;
          private->starty = y2;
        }
      else if (curx > x2)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = x2;
        }
      else if (cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_UPPER_RIGHT:
      if (curx < x1 && cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->startx = x1;
          private->starty = y2;
        }
      else if (curx < x1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->startx = x1;
        }
      else if (cury > y2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->starty = y2;
        }
      break;

    case RECT_RESIZING_LOWER_LEFT:
      if (curx > x2 && cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->startx = x2;
          private->starty = y1;
        }
      else if (curx > x2)
        {
          new_function = RECT_RESIZING_LOWER_RIGHT;
          private->startx = x2;
        }
      else if (cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->starty = y1;
        }
      break;

    case RECT_RESIZING_LOWER_RIGHT:
      if (curx < x1 && cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_LEFT;
          private->startx = x1;
          private->starty = y1;
        }
      else if (curx < x1)
        {
          new_function = RECT_RESIZING_LOWER_LEFT;
          private->startx = x1;
        }
      else if (cury < y1)
        {
          new_function = RECT_RESIZING_UPPER_RIGHT;
          private->starty = y1;
        }
      break;

    default:
      break;
    }

  if (new_function != function)
    g_object_set (rectangle, "function", new_function, NULL);

}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpRectangleTool *rectangle = GIMP_RECTANGLE_TOOL (tool);
  gint               inc_x, inc_y;
  gint               min_x, min_y;
  gint               max_x, max_y;
  gint               x1, y1, x2, y2;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

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

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  g_object_set (rectangle,
                "x1", x1 + inc_x,
                "y1", y1 + inc_y,
                "x2", x2 + inc_x,
                "y2", y2 + inc_y,
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
  GimpRectangleTool        *rectangle = GIMP_RECTANGLE_TOOL (tool);
  GimpRectangleToolPrivate *private;
  GimpDrawTool             *draw_tool = GIMP_DRAW_TOOL (tool);
  gint                      x1, y1, x2, y2;
  gboolean                  inside_x;
  gboolean                  inside_y;
  gdouble                   handle_w, handle_h;
  GimpDisplayShell         *shell;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (tool->display != display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  handle_w = private->dcw / SCALEFACTOR_X (shell);
  handle_h = private->dch / SCALEFACTOR_Y (shell);

  inside_x = coords->x > x1 && coords->x < x2;
  inside_y = coords->y > y1 && coords->y < y2;

  if (gimp_draw_tool_on_handle (draw_tool, display,
                                coords->x, coords->y,
                                GIMP_HANDLE_SQUARE,
                                x1, y1,
                                private->dcw, private->dch,
                                GTK_ANCHOR_NORTH_WEST,
                                FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x2, y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_EAST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if  (gimp_draw_tool_on_handle (draw_tool, display,
                                      coords->x, coords->y,
                                      GIMP_HANDLE_SQUARE,
                                      x2, y1,
                                      private->dcw, private->dch,
                                      GTK_ANCHOR_NORTH_EAST,
                                      FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_UPPER_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x,
                                          y1 - coords->y,
                                          0, 0);
    }
  else if (gimp_draw_tool_on_handle (draw_tool, display,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     x1, y2,
                                     private->dcw, private->dch,
                                     GTK_ANCHOR_SOUTH_WEST,
                                     FALSE))
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LOWER_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x,
                                          y2 - coords->y,
                                          0, 0);
    }
  else if ( (fabs (coords->x - x1) < handle_w) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_LEFT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x1 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->x - x2) < handle_w) && inside_y)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_RIGHT, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          x2 - coords->x, 0, 0, 0);

    }
  else if ( (fabs (coords->y - y1) < handle_h) && inside_x)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_TOP, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, y1 - coords->y, 0, 0);

    }
  else if ( (fabs (coords->y - y2) < handle_h) && inside_x)
    {
      g_object_set (rectangle, "function", RECT_RESIZING_BOTTOM, NULL);

      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, y2 - coords->y, 0, 0);

    }
  else if (inside_x && inside_y)
    {
      g_object_set (rectangle, "function", RECT_MOVING, NULL);
    }
  /*  otherwise, the new function will be creating, since we want
   *  to start a new rectangle
   */
  else
    {
      g_object_set (rectangle, "function", RECT_CREATING, NULL);

      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
    }
}

void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpRectangleTool *rectangle;
  GimpCursorType     cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier modifier = GIMP_CURSOR_MODIFIER_NONE;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);

  if (tool->display == display)
    {
      guint function;

      g_object_get (rectangle, "function", &function, NULL);

      switch (function)
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

  gimp_tool_control_set_cursor (tool->control, cursor);
  gimp_tool_control_set_cursor_modifier (tool->control, modifier);
}

#define ANCHOR_SIZE 14

void
gimp_rectangle_tool_draw (GimpDrawTool *draw_tool)
{
  GimpRectangleToolPrivate *private;
  gint                      x1, x2, y1, y2;
  guint                     function;
  gboolean                  button1_down;
  GimpTool                 *tool;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (draw_tool));

  tool = GIMP_TOOL (draw_tool);

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (draw_tool);

  g_object_get (GIMP_RECTANGLE_TOOL (draw_tool), "function", &function, NULL);

  if (function == RECT_INACTIVE)
    return;

  x1 = private->x1;
  x2 = private->x2;
  y1 = private->y1;
  y2 = private->y2;

  gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                 x1, y1, x2 - x1, y2 - y1, FALSE);


  button1_down = gimp_tool_control_is_active (tool->control);

  if (button1_down)
    {
      GimpDisplayShell *shell    = GIMP_DISPLAY_SHELL (tool->display->shell);
      gdouble           handle_w;
      gdouble           handle_h;
      gint              X1 = x1;
      gint              Y1 = y1;
      gint              X2 = x2;
      gint              Y2 = y2;
      gboolean          do_it    = TRUE;

      handle_w = private->dcw / SCALEFACTOR_X (shell);
      handle_h = private->dch / SCALEFACTOR_Y (shell);

      switch (private->function)
      {
      case RECT_RESIZING_LEFT:
        X2 = x1 + handle_w / 3;
        break;

      case RECT_RESIZING_RIGHT:
        X1 = x2 - handle_w / 3;
        break;

      case RECT_RESIZING_TOP:
        Y2 = y1 + handle_h / 3;
        break;

      case RECT_RESIZING_BOTTOM:
        Y1 = y2 - handle_h / 3;
        break;

      default:
        do_it = FALSE;
        break;
      }

      if (do_it)
        {
          gimp_draw_tool_draw_rectangle (draw_tool, TRUE,
                                         X1, Y1, X2 - X1, Y2 - Y1,
                                         FALSE);
        }
      else
        {
          GimpCoords coords[3];

          do_it     = TRUE;

          switch (private->function)
            {
            case RECT_RESIZING_UPPER_LEFT:
              coords[0].x = x1;
              coords[0].y = y1;
              coords[1].x = x1;
              coords[1].y = y1 + handle_h;
              coords[2].x = x1 + handle_w;
              coords[2].y = y1;
              break;

            case RECT_RESIZING_UPPER_RIGHT:
              coords[0].x = x2;
              coords[0].y = y1;
              coords[1].x = x2;
              coords[1].y = y1 + handle_h;
              coords[2].x = x2 - handle_w;
              coords[2].y = y1;
              break;

            case RECT_RESIZING_LOWER_LEFT:
              coords[0].x = x1;
              coords[0].y = y2;
              coords[1].x = x1;
              coords[1].y = y2 - handle_h;
              coords[2].x = x1 + handle_w;
              coords[2].y = y2;
              break;

            case RECT_RESIZING_LOWER_RIGHT:
              coords[0].x = x2;
              coords[0].y = y2;
              coords[1].x = x2;
              coords[1].y = y2 - handle_h;
              coords[2].x = x2 - handle_w;
              coords[2].y = y2;
              break;

            default:
              do_it = FALSE;
              break;
            }

          if (do_it)
            {
              gimp_draw_tool_draw_strokes (draw_tool, coords, 3, TRUE, FALSE);
            }
        }
    }
  else
    {
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x1, y1, ANCHOR_SIZE, ANCHOR_SIZE,
                                  GTK_ANCHOR_NORTH_WEST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x2, y1, ANCHOR_SIZE, ANCHOR_SIZE,
                                  GTK_ANCHOR_NORTH_EAST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x1, y2, ANCHOR_SIZE, ANCHOR_SIZE,
                                  GTK_ANCHOR_SOUTH_WEST, FALSE);
      gimp_draw_tool_draw_handle (draw_tool, GIMP_HANDLE_FILLED_SQUARE,
                                  x2, y2, ANCHOR_SIZE, ANCHOR_SIZE,
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
  gboolean                  highlight;
  gint                      x1, y1;
  gint                      x2, y2;
  gint                      dx1, dx2, dy1, dy2, dcw, dch;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (! tool->display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (options, "highlight", &highlight, NULL);

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  if (highlight)
    {
      GdkRectangle rect;

      rect.x      = x1;
      rect.y      = y1;
      rect.width  = x2 - x1;
      rect.height = y2 - y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }

  gimp_display_shell_transform_xy (shell,
                                   x1, y1,
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   x2, y2,
                                   &dx2, &dy2,
                                   FALSE);

#define SRW 20
#define SRH 20

  dcw = ((dx2 - dx1) < SRW) ? (dx2 - dx1) : SRW;
  dch = ((dy2 - dy1) < SRH) ? (dy2 - dy1) : SRH;

#undef SRW
#undef SRH

  private->dx1 = dx1;
  private->dx2 = dx2;
  private->dy1 = dy1;
  private->dy2 = dy2;
  private->dcw = dcw;
  private->dch = dch;
}

static void
rectangle_tool_start (GimpRectangleTool *rectangle)
{
  GimpTool *tool = GIMP_TOOL (rectangle);

  gimp_rectangle_tool_configure (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->display,
                                _("Rectangle: "), 0, " x ", 0);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->display);
}

void
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
      gint x1, y1, x2, y2;

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      retval = iface->execute (rectangle, x1, y1, x2 - x1, y2 - y1);

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
  GimpRectangleOptions *options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle);
  gdouble               x;
  gdouble               y;
  gdouble               width;
  gdouble               height;
  gdouble               center_x, center_y;
  gint                  x1, y1, x2, y2;
  gboolean              fixed_width;
  gboolean              fixed_height;

  g_object_get (rectangle,
                "x1", &x1,
                "y1", &y1,
                "x2", &x2,
                "y2", &y2,
                NULL);

  g_object_get (options,
                "fixed-width",      &fixed_width,
                "fixed-height",     &fixed_height,
                NULL);

  x = x1;
  y = y1;
  width  = x2 - x1;
  height = y2 - y1;

  center_x = (x1 + x2) / 2.0;
  center_y = (y1 + y2) / 2.0;

  /* need to block "notify" handlers for the options */
  g_signal_handlers_block_matched (options, G_SIGNAL_MATCH_DATA,
                                   0, 0, NULL, NULL, rectangle);

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

  g_signal_handlers_unblock_matched (options, G_SIGNAL_MATCH_DATA,
                                     0, 0, NULL, NULL, rectangle);


  g_object_set (options,
                "center-x", center_x,
                "center-y", center_y,
                NULL);

}

/*
 * we handle changes in width by treating them as movement of the right edge
 */
static void
gimp_rectangle_tool_notify_width (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   width;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  /* make sure a rectangle exists */
  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "width", &width,
                NULL);
  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx1 + width;
  coords.y = ry2;

  g_object_set (rectangle,
                "function", RECT_RESIZING_RIGHT,
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in x by treating them as movement of the left edge
 */
static void
gimp_rectangle_tool_notify_x (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   x;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  /* make sure a rectangle exists */
  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "x0", &x,
                NULL);
  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = x;
  coords.y = ry2;

  g_object_set (rectangle,
                "function", RECT_RESIZING_LEFT,
                NULL);
  private->startx = rx1;
  private->starty = ry1;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in y by treating them as movement of the top edge
 */
static void
gimp_rectangle_tool_notify_y (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   y;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  /* make sure a rectangle exists */
  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "y0", &y,
                NULL);
  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx1;
  coords.y = y;

  g_object_set (rectangle,
                "function", RECT_RESIZING_TOP,
                NULL);
  private->startx = rx1;
  private->starty = ry1;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in height by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_height (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   height;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "height", &height,
                NULL);
  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx2;
  coords.y = ry1 + height;

  g_object_set (rectangle,
                "function", RECT_RESIZING_BOTTOM,
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

/*
 * we handle changes in aspect by treating them as movement of the bottom edge
 */
static void
gimp_rectangle_tool_notify_aspect (GimpRectangleOptions *options,
                                   GParamSpec           *pspec,
                                   GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate *private;
  gint                      rx1, rx2, ry1, ry2;
  GimpCoords                coords;
  gdouble                   aspect;
  gdouble                   numerator, denominator;
  gboolean                  fixed_aspect;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (! GIMP_TOOL (rectangle)->display)
    return;

  g_object_get (options,
                "aspect-numerator",   &numerator,
                "aspect-denominator", &denominator,
                "fixed-aspect",       &fixed_aspect,
                NULL);

  if (! fixed_aspect)
    return;

  aspect = numerator / denominator;

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  coords.x = rx2;
  coords.y = ry1 + (rx2 - rx1) / aspect;

  g_object_set (rectangle,
                "function", RECT_RESIZING_BOTTOM,
                NULL);
  private->startx = rx2;
  private->starty = ry2;

  gimp_rectangle_tool_motion (GIMP_TOOL (rectangle), &coords, 0, 0,
                              GIMP_TOOL (rectangle)->display);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

static void
gimp_rectangle_tool_notify_highlight (GimpRectangleOptions *options,
                                      GParamSpec           *pspec,
                                      GimpRectangleTool    *rectangle)
{
  GimpTool         *tool = GIMP_TOOL (rectangle);
  GimpDisplayShell *shell;
  gboolean          highlight;

  if (! tool->display)
    return;

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  g_object_get (options, "highlight", &highlight, NULL);

  if (highlight)
    {
      GdkRectangle rect;
      gint         x1, y1;
      gint         x2, y2;

      g_object_get (rectangle,
                    "x1", &x1,
                    "y1", &y1,
                    "x2", &x2,
                    "y2", &y2,
                    NULL);

      rect.x      = x1;
      rect.y      = y1;
      rect.width  = x2 - x1;
      rect.height = y2 - y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }
  else
    {
      gimp_display_shell_set_highlight (shell, NULL);
    }
}

static void
gimp_rectangle_tool_notify_guide (GimpRectangleOptions *options,
                                  GParamSpec           *pspec,
                                  GimpRectangleTool    *rectangle)
{
  GimpTool                 *tool = GIMP_TOOL (rectangle);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  if (tool->display)
    gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  g_object_get (options,
                "guide", &private->guide,
                NULL);

  if (tool->display)
    gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));
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
gboolean
gimp_rectangle_tool_constraint_violated (GimpRectangleTool *rectangle,
                                         gint               x1,
                                         gint               y1,
                                         gint               x2,
                                         gint               y2,
                                         gdouble           *alpha,
                                         gdouble           *beta)
{
  GimpRectangleConstraint  constraint    = gimp_rectangle_tool_get_constraint (rectangle);
  GimpTool                *tool          = GIMP_TOOL (rectangle);
  GimpImage               *image         = tool->display->image;
  gint                     min_x, min_y;
  gint                     max_x, max_y;

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
      gint rx1;

      g_object_get (rectangle,
                    "x1", &rx1,
                    NULL);

      *alpha = (rx1 - min_x) / (gdouble) (rx1 - x1);
      return TRUE;
    }


  if (y1 < min_y)
    {
      gint ry1;

      g_object_get (rectangle,
                    "y1", &ry1,
                    NULL);

      *beta = (ry1 - min_y) / (gdouble) (ry1 - y1);
      return TRUE;
    }

  if (x2 > max_x)
    {
      gint rx2;

      g_object_get (rectangle,
                    "x2", &rx2,
                    NULL);

      *alpha = (max_x - rx2) / (gdouble) (x2 - rx2);
      return TRUE;
    }

  if (y2 > max_y)
    {
      gint ry2;

      g_object_get (rectangle,
                    "y2", &ry2,
                    NULL);

      *beta = (max_y - ry2) / (gdouble) (y2 - ry2);
      return TRUE;
    }

  return FALSE;
}

static void
gimp_rectangle_tool_auto_shrink_notify (GimpRectangleOptions *options,
                                        GParamSpec           *pspec,
                                        GimpRectangleTool    *rectangle)
{
  gboolean auto_shrink;

  g_object_get (options,
                "auto-shrink", &auto_shrink,
                NULL);

  if (auto_shrink)
    gimp_rectangle_tool_auto_shrink (rectangle);

  g_signal_emit_by_name (rectangle, "rectangle-changed", NULL);
}

static void
gimp_rectangle_tool_auto_shrink (GimpRectangleTool *rectangle)
{
  GimpTool             *tool      = GIMP_TOOL (rectangle);
  GimpDisplay          *display   = tool->display;
  gint                  width;
  gint                  height;
  gint                  offset_x  = 0;
  gint                  offset_y  = 0;
  gint                  rx1, ry1;
  gint                  rx2, ry2;
  gint                  x1, y1;
  gint                  x2, y2;
  gint                  shrunk_x1;
  gint                  shrunk_y1;
  gint                  shrunk_x2;
  gint                  shrunk_y2;
  gboolean              shrink_merged;

  if (! display)
    return;

  width  = display->image->width;
  height = display->image->height;

  g_object_get (gimp_tool_get_options (tool),
                "shrink-merged", &shrink_merged,
                NULL);

  g_object_get (rectangle,
                "x1", &rx1,
                "y1", &ry1,
                "x2", &rx2,
                "y2", &ry2,
                NULL);

  x1 = rx1 - offset_x  > 0      ? rx1 - offset_x : 0;
  x2 = rx2 - offset_x  < width  ? rx2 - offset_x : width;
  y1 = ry1 - offset_y  > 0      ? ry1 - offset_y : 0;
  y2 = ry2 - offset_y  < height ? ry2 - offset_y : height;

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
