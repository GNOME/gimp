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

#include <string.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimpchannel.h"
#include "core/gimpimage.h"
#include "core/gimpimage-crop.h"
#include "core/gimppickable.h"
#include "core/gimpmarshal.h"

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

#define MAX_HANDLE_SIZE  50
#define MIN_HANDLE_SIZE   6

#define SQRT5   2.236067977

typedef enum
{
  CLAMPED_NONE   = 0,
  CLAMPED_LEFT   = (1 << 0),
  CLAMPED_RIGHT  = (1 << 1),
  CLAMPED_TOP    = (1 << 2),
  CLAMPED_BOTTOM = (1 << 3)
} ClampedSide;

typedef enum
{
  SIDE_TO_RESIZE_NONE,
  SIDE_TO_RESIZE_LEFT,
  SIDE_TO_RESIZE_RIGHT,
  SIDE_TO_RESIZE_TOP,
  SIDE_TO_RESIZE_BOTTOM,
  SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY,
  SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY,
} SideToResize;


#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) \
  (gimp_rectangle_tool_get_private (GIMP_RECTANGLE_TOOL (obj)))


typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  /* The following members are "constants", that is, variables that are setup
   * during gimp_rectangle_tool_button_press and then only read.
   */

  /* Holds coordinate where button was pressed when rectangle adjustment was
   * initiated.
   */
  gint                    pressx;
  gint                    pressy;

  /* Holds the coordinate that should be used as the "other side" when
   * fixed-center is turned off.
   */
  gint                    other_side_x;
  gint                    other_side_y;

  /* Holds the coordinate to be used as center when fixed-center is used. */
  gint                    center_x_on_fixed_center;
  gint                    center_y_on_fixed_center;

  /* The rest of the members are internal state variables, that is, variables
   * that might change during the manipulation session of the rectangle. Make
   * sure these variables are in consistent states.
   */

  gint                    x1, y1;     /*  upper left hand coordinate     */
  gint                    x2, y2;     /*  lower right hand coords        */

  guint                   function;   /*  moving or resizing             */

  GimpRectangleConstraint constraint; /* how to constrain rectangle     */

  gint                    lastx;      /*  previous x coord               */
  gint                    lasty;      /*  previous y coord               */

  gint                    handle_w;   /*  handle width                   */
  gint                    handle_h;   /*  handle height                  */

                                      /*  Top and bottom side handle
                                       *  width.
                                       */
  gint                    top_and_bottom_handle_w;
                                      /*  Left and right side handle
                                       *  height.
                                       */
  gint                    left_and_right_handle_h;

  gint                    saved_x1;   /*  for saving in case action      */
  gint                    saved_y1;   /*  is canceled                    */
  gint                    saved_x2;
  gint                    saved_y2;
  gdouble                 saved_center_x;
  gdouble                 saved_center_y;

  gint                    suppress_updates;

  GimpRectangleGuide      guide; /* synced with options->guide, only exists for drawing */
};


static void gimp_rectangle_tool_iface_base_init     (GimpRectangleToolInterface *iface);

static GimpRectangleToolPrivate *
                gimp_rectangle_tool_get_private     (GimpRectangleTool *rectangle);

GimpRectangleConstraint
                gimp_rectangle_tool_get_constraint  (GimpRectangleTool *rectangle);

/*  Rectangle helper functions  */
static void     gimp_rectangle_tool_start           (GimpRectangleTool *rectangle,
                                                     GimpDisplay       *display);
static void     gimp_rectangle_tool_halt            (GimpRectangleTool *rectangle);
static void     gimp_rectangle_tool_draw_guides     (GimpDrawTool      *draw_tool);

/*  Rectangle dialog functions  */
static void     gimp_rectangle_tool_update_options  (GimpRectangleTool *rectangle,
                                                     GimpDisplay       *display);

static void     gimp_rectangle_tool_options_notify  (GimpRectangleOptions *options,
                                                     GParamSpec         *pspec,
                                                     GimpRectangleTool  *rectangle);

static void     gimp_rectangle_tool_check_function  (GimpRectangleTool *rectangle);

static void gimp_rectangle_tool_rectangle_changed   (GimpRectangleTool *rectangle);

static void     gimp_rectangle_tool_auto_shrink     (GimpRectangleTool *rectangle);

static GtkAnchorType gimp_rectangle_tool_get_anchor (GimpRectangleToolPrivate *private);
static void     gimp_rectangle_tool_set_highlight   (GimpRectangleTool *rectangle);

static void gimp_rectangle_tool_get_other_side      (GimpRectangleTool  *rectangle_tool,
                                                     const gchar       **other_x,
                                                     const gchar       **other_y);
static void gimp_rectangle_tool_get_other_side_coord
                                                    (GimpRectangleTool  *rectangle_tool,
                                                     gint               *other_side_x,
                                                     gint               *other_side_y);
static void gimp_rectangle_tool_set_other_side_coord
                                                    (GimpRectangleTool  *rectangle_tool,
                                                     gint                other_side_x,
                                                     gint                other_side_y);

static void gimp_rectangle_tool_apply_coord         (GimpRectangleTool      *rectangle_tool,
                                                     gint                    coord_x,
                                                     gint                    coord_y);

static void gimp_rectangle_tool_clamp               (GimpRectangleTool      *rectangle_tool,
                                                     ClampedSide            *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean                symmetrically);
static void gimp_rectangle_tool_clamp_width         (GimpRectangleTool      *rectangle_tool,
                                                     ClampedSide            *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean                symmetrically);
static void gimp_rectangle_tool_clamp_height        (GimpRectangleTool      *rectangle_tool,
                                                     ClampedSide            *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean                symmetrically);

static void gimp_rectangle_tool_keep_inside         (GimpRectangleTool      *rectangle_tool,
                                                     GimpRectangleConstraint constraint);
static void gimp_rectangle_tool_keep_inside_horizontally
                                                    (GimpRectangleTool      *rectangle_tool,
                                                     GimpRectangleConstraint constraint);
static void gimp_rectangle_tool_keep_inside_vertically
                                                    (GimpRectangleTool      *rectangle_tool,
                                                     GimpRectangleConstraint constraint);

static void gimp_rectangle_tool_apply_fixed_width   (GimpRectangleTool      *rectangle_tool,
                                                     GimpRectangleConstraint constraint);
static void gimp_rectangle_tool_apply_fixed_height  (GimpRectangleTool      *rectangle_tool,
                                                     GimpRectangleConstraint constraint);

static void gimp_rectangle_tool_apply_aspect        (GimpRectangleTool      *rectangle_tool,
                                                     gdouble                 aspect,
                                                     gint                    clamped_sides);

static void gimp_rectangle_tool_update_with_coord   (GimpRectangleTool       *rectangle_tool,
                                                     gint                    new_x,
                                                     gint                    new_y);

static void gimp_rectangle_tool_get_constraints     (GimpRectangleTool      *rectangle_tool,
                                                     gint                   *min_x,
                                                     gint                   *min_y,
                                                     gint                   *max_x,
                                                     gint                   *max_y,
                                                     GimpRectangleConstraint constraint);

static void gimp_rectangle_tool_handle_general_clamping
                                                    (GimpRectangleTool      *rectangle_tool);


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
                                           g_param_spec_enum ("constraint",
                                                              NULL, NULL,
                                                              GIMP_TYPE_RECTANGLE_CONSTRAINT,
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
  g_slice_free (GimpRectangleToolPrivate, private);
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
      private = g_slice_new0 (GimpRectangleToolPrivate);

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
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT:
      gimp_rectangle_tool_set_constraint (rectangle, g_value_get_enum (value));
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
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT:
      g_value_set_enum (value, gimp_rectangle_tool_get_constraint (rectangle));
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
  GimpRectangleTool           *rectangle;
  GimpDrawTool                *draw_tool;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gint                         x, y;
  gint                         snap_x, snap_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle       = GIMP_RECTANGLE_TOOL (tool);
  draw_tool       = GIMP_DRAW_TOOL (tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  x = ROUND (coords->x);
  y = ROUND (coords->y);

  gimp_draw_tool_pause (draw_tool);

  if (display != tool->display)
    {
      if (gimp_draw_tool_is_active (draw_tool))
        gimp_draw_tool_stop (draw_tool);

      gimp_rectangle_tool_set_function (rectangle, RECT_CREATING);

      g_object_set (rectangle,
                    "x1", x,
                    "y1", y,
                    "x2", x,
                    "y2", y,
                    NULL);

      gimp_rectangle_tool_start (rectangle, display);
    }

  /* save existing shape in case of cancellation */
  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  private->saved_center_x = options_private->center_x;
  private->saved_center_y = options_private->center_y;

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

  private->lastx  = x;
  private->lasty  = y;

  /* If the rectangle is being modified we want the center on fixed_center to be
   * at the center of the currently existing rectangle, otherwise we want the
   * point where the user clicked to be the center on fixed_center.
   */
  if (private->function == RECT_CREATING)
    {
      private->center_x_on_fixed_center = private->pressx;
      private->center_y_on_fixed_center = private->pressy;
    }
  else
    {
      private->center_x_on_fixed_center = options_private->center_x;
      private->center_y_on_fixed_center = options_private->center_y;
    }

  /* When the user toggles modifier keys, we want to keep track of what
   * coordinates the "other side" should have. If we are creating a rectangle,
   * use the current mouse coordinates as the coordinate of the "other side",
   * otherwise use the immidiate "other side" for that.
   */
  if (private->function == RECT_CREATING)
    {
      private->other_side_x = private->pressx;
      private->other_side_y = private->pressy;
    }
  else
    {
      gint other_side_x = 0;
      gint other_side_y = 0;

      gimp_rectangle_tool_get_other_side_coord (rectangle,
                                                &other_side_x,
                                                &other_side_y);
      private->other_side_x = other_side_x;
      private->other_side_y = other_side_y;
    }

  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_resume (draw_tool);
}

void
gimp_rectangle_tool_button_release (GimpTool              *tool,
                                    GimpCoords            *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_tool_control_halt (tool->control);

  if (private->function == RECT_EXECUTING)
    gimp_tool_pop_status (tool, display);

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_NORMAL:
      gimp_rectangle_tool_rectangle_changed (rectangle);
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
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
      break;

    case GIMP_BUTTON_RELEASE_CLICK:

      /* When a dead area is clicked, don't execute. */
      if (private->function == RECT_DEAD)
        break;

      if (gimp_rectangle_tool_execute (rectangle))
        gimp_rectangle_tool_halt (rectangle);
      break;

    case GIMP_BUTTON_RELEASE_NO_MOTION:
      break;
    }

  /* We must update this. */
  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

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
  GimpRectangleTool           *rectangle;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gint                         current_x;
  gint                         current_y;
  gint                         snap_x, snap_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rectangle       = GIMP_RECTANGLE_TOOL (tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  /*  This is the only case when the motion events should be ignored --
   *  we're just waiting for the button release event to execute.
   */
  if (private->function == RECT_EXECUTING)
    return;

  current_x = ROUND (coords->x);
  current_y = ROUND (coords->y);

  /* Handle snapping. */
  gimp_tool_control_get_snap_offsets (tool->control,
                                      &snap_x, &snap_y, NULL, NULL);
  current_x += snap_x;
  current_y += snap_y;

  /*  If there have been no changes... return  */
  if (private->lastx == current_x && private->lasty == current_y)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));


  /* This is the core rectangle shape updating function: */
  gimp_rectangle_tool_update_with_coord (rectangle,
                                         current_x,
                                         current_y);

  /*  recalculate the coordinates for rectangle_draw based on the new values  */
  gimp_rectangle_tool_configure (rectangle);

  if (private->function != RECT_MOVING &&
      private->function != RECT_EXECUTING)
    {
      gint w, h;

      gimp_tool_pop_status (tool, display);

      w = private->x2 - private->x1;
      h = private->y2 - private->y1;

      if (w > 0 && h > 0)
        gimp_tool_push_status_coords (tool, display,
                                      _("Rectangle: "), w, " × ", h, NULL);
    }

  if (private->function == RECT_CREATING)
    {
      GimpRectangleFunction function = RECT_CREATING;
      gint                  dx = current_x - private->lastx;
      gint                  dy = current_y - private->lasty;

      /* When the user starts to move the cursor, set the current function to
       * one of the corner-grabbed functions, depending on in what direction
       * the user starts dragging the rectangle.
       */
      if (dx < 0)
        {
          function = dy < 0 ?
            RECT_RESIZING_UPPER_LEFT :
            RECT_RESIZING_LOWER_LEFT;
        }
      else if (dx > 0)
        {
          function = dy < 0 ?
            RECT_RESIZING_UPPER_RIGHT :
            RECT_RESIZING_LOWER_RIGHT;
        }
      else if (dy < 0)
        {
          function = dx < 0 ?
            RECT_RESIZING_UPPER_LEFT :
            RECT_RESIZING_UPPER_RIGHT;
        }
      else if (dy > 0)
        {
          function = dx < 0 ?
            RECT_RESIZING_LOWER_LEFT :
            RECT_RESIZING_LOWER_RIGHT;
        }

      gimp_rectangle_tool_set_function (rectangle, function);
    }

  gimp_rectangle_tool_update_options (rectangle, display);

  private->lastx = current_x;
  private->lasty = current_y;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void

gimp_rectangle_tool_active_modifier_key (GimpTool        *tool,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpDrawTool                *draw_tool;
  GimpRectangleTool           *rectangle;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleToolPrivate    *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  draw_tool       = GIMP_DRAW_TOOL (tool);
  rectangle       = GIMP_RECTANGLE_TOOL (tool);
  private         = gimp_rectangle_tool_get_private (rectangle);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  gimp_draw_tool_pause (draw_tool);

  if (key == GDK_SHIFT_MASK)
    {
      /* Here we want to handle manualy when to update the rectangle, so we
       * don't want gimp_rectangle_tool_options_notify to do anything.
       */
      g_signal_handlers_block_by_func (options,
                                       gimp_rectangle_tool_options_notify,
                                       rectangle);

      g_object_set (options,
                    "fixed-aspect", ! options_private->fixed_aspect,
                    NULL);

      g_signal_handlers_unblock_by_func (options,
                                         gimp_rectangle_tool_options_notify,
                                         rectangle);

      /* Only change the shape if the mouse is still down (i.e. the user is
       * still editing the rectangle.
       */
      if (state & GDK_BUTTON1_MASK)
        {
          gimp_rectangle_tool_update_with_coord (rectangle,
                                                 private->lastx,
                                                 private->lasty);

          gimp_rectangle_tool_configure (rectangle);

          gimp_rectangle_tool_rectangle_changed (rectangle);
        }
    }

  if (key == GDK_CONTROL_MASK)
    {

      g_object_set (options,
                    "fixed-center", ! options_private->fixed_center,
                    NULL);

      if (options_private->fixed_center)
        {
          g_object_set (options,
                        "center-x", (gdouble) private->center_x_on_fixed_center,
                        "center-y", (gdouble) private->center_y_on_fixed_center,
                        NULL);

          gimp_rectangle_tool_update_with_coord (rectangle,
                                                 private->lastx,
                                                 private->lasty);

          gimp_rectangle_tool_configure (rectangle);


          gimp_rectangle_tool_rectangle_changed (rectangle);

        }
      else if (state & GDK_BUTTON1_MASK)
        {
          /* If we are leaving fixed_center mode we want to set the "other side"
           * where it should be. Don't do anything if we came here by a mouse-click
           * though, since then the user has confirmed the shape and we don't want
           * to modify it afterwards.
           */
          gimp_rectangle_tool_set_other_side_coord (rectangle,
                                                    private->other_side_x,
                                                    private->other_side_y);

          gimp_rectangle_tool_configure (rectangle);

          gimp_rectangle_tool_rectangle_changed (rectangle);

        }

    }

  gimp_draw_tool_resume (draw_tool);
}

static void swap_ints (gint *i,
                       gint *j)
{
  gint tmp;

  tmp = *i;
  *i = *j;
  *j = tmp;
}

/* gimp_rectangle_tool_check_function() is needed to deal with
 * situations where the user drags a corner or edge across one of the
 * existing edges, thereby changing its function.  Ugh.
 */
static void
gimp_rectangle_tool_check_function (GimpRectangleTool *rectangle)

{
  GimpRectangleToolPrivate *private;
  GimpRectangleFunction     function;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  function = private->function;

  if (private->x2 < private->x1)
    {
      swap_ints (&private->x1, &private->x2);
      switch (function)
        {
          case RECT_RESIZING_UPPER_LEFT:
            function = RECT_RESIZING_UPPER_RIGHT;
            break;
          case RECT_RESIZING_UPPER_RIGHT:
            function = RECT_RESIZING_UPPER_LEFT;
            break;
          case RECT_RESIZING_LOWER_LEFT:
            function = RECT_RESIZING_LOWER_RIGHT;
            break;
          case RECT_RESIZING_LOWER_RIGHT:
            function = RECT_RESIZING_LOWER_LEFT;
            break;
          case RECT_RESIZING_LEFT:
            function = RECT_RESIZING_RIGHT;
            break;
          case RECT_RESIZING_RIGHT:
            function = RECT_RESIZING_LEFT;
            break;
          /* avoid annoying warnings about unhandled enums */
          default:
            break;
        }
    }

  if (private->y2 < private->y1)
    {
      swap_ints (&private->y1, &private->y2);
      switch (function)
        {
          case RECT_RESIZING_UPPER_LEFT:
           function = RECT_RESIZING_LOWER_LEFT;
            break;
          case RECT_RESIZING_UPPER_RIGHT:
            function = RECT_RESIZING_LOWER_RIGHT;
            break;
          case RECT_RESIZING_LOWER_LEFT:
            function = RECT_RESIZING_UPPER_LEFT;
            break;
          case RECT_RESIZING_LOWER_RIGHT:
            function = RECT_RESIZING_UPPER_RIGHT;
            break;
          case RECT_RESIZING_TOP:
            function = RECT_RESIZING_BOTTOM;
            break;
          case RECT_RESIZING_BOTTOM:
            function = RECT_RESIZING_TOP;
            break;
          default:
            break;
        }
    }

  gimp_rectangle_tool_set_function (rectangle, function);
}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  gint                      dx = 0;
  gint                      dy = 0;
  gint                      new_x = 0;
  gint                      new_y = 0;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

  if (display != tool->display)
    return FALSE;

  rectangle = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  switch (kevent->keyval)
    {
    case GDK_Up:
      dy = -1;
      break;
    case GDK_Left:
      dx = -1;
      break;
    case GDK_Right:
      dx = 1;
      break;
    case GDK_Down:
      dy = 1;
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
      return FALSE;
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & GDK_SHIFT_MASK)
    {
      dx *= ARROW_VELOCITY;
      dy *= ARROW_VELOCITY;
    }

  gimp_tool_control_set_snap_offsets (GIMP_TOOL (rectangle)->control,
                                      0, 0, 0, 0);

  /*  Resize the rectangle if the mouse is over a handle, otherwise move it  */
  switch (private->function)
    {
    case RECT_MOVING:
    case RECT_RESIZING_UPPER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case RECT_RESIZING_UPPER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case RECT_RESIZING_LOWER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case RECT_RESIZING_LOWER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case RECT_RESIZING_LEFT:
      new_x = private->x1 + dx;
      private->lastx = new_x;
      break;
    case RECT_RESIZING_RIGHT:
      new_x = private->x2 + dx;
      private->lastx = new_x;
      break;
    case RECT_RESIZING_TOP:
      new_y = private->y1 + dy;
      private->lasty = new_y;
      break;
    case RECT_RESIZING_BOTTOM:
      new_y = private->y2 + dy;
      private->lasty = new_y;
      break;

    default:
      return TRUE;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_rectangle_tool_update_with_coord (rectangle,
                                         new_x,
                                         new_y);

  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

/*   g_object_set (rectangle, */
/*                 "x1", private->x1, */
/*                 "y1", private->y1, */
/*                 "x2", private->x2, */
/*                 "y2", private->y2, */
/*                 NULL); */

  gimp_rectangle_tool_configure (rectangle);

  gimp_rectangle_tool_update_options (rectangle, tool->display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_rectangle_tool_rectangle_changed (rectangle);

  /*  Evil hack to suppress oper updates. We do this because we don't
   *  want the rectangle tool to change function while the rectangle
   *  is being resized or moved using the keyboard.
   */
  private->suppress_updates = 2;

  return TRUE;
}

void
gimp_rectangle_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpRectangleToolPrivate *private;
  GimpDrawTool             *draw_tool;
  gint                      function;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);

  if (tool->display != display)
    return;

  if (private->suppress_updates)
    {
      private->suppress_updates--;
      return;
    }

  if (coords->x > private->x1 && coords->x < private->x2 &&
      coords->y > private->y1 && coords->y < private->y2)
    {
      GimpDisplayShell *shell = GIMP_DISPLAY_SHELL (tool->display->shell);
      gint              w     = private->x2 - private->x1;
      gint              h     = private->y2 - private->y1;
      gint              tw    = w * shell->scale_x;
      gint              th    = h * shell->scale_y;

      if (gimp_draw_tool_on_handle (draw_tool, display,
                                    coords->x, coords->y,
                                    GIMP_HANDLE_SQUARE,
                                    private->x1, private->y1,
                                    private->handle_w, private->handle_h,
                                    GTK_ANCHOR_NORTH_WEST,
                                    FALSE))
        {
          function = RECT_RESIZING_UPPER_LEFT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x2, private->y2,
                                         private->handle_w, private->handle_h,
                                         GTK_ANCHOR_SOUTH_EAST,
                                         FALSE))
        {
          function = RECT_RESIZING_LOWER_RIGHT;
        }
      else if  (gimp_draw_tool_on_handle (draw_tool, display,
                                          coords->x, coords->y,
                                          GIMP_HANDLE_SQUARE,
                                          private->x2, private->y1,
                                          private->handle_w, private->handle_h,
                                          GTK_ANCHOR_NORTH_EAST,
                                          FALSE))
        {
          function = RECT_RESIZING_UPPER_RIGHT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x1, private->y2,
                                         private->handle_w, private->handle_h,
                                         GTK_ANCHOR_SOUTH_WEST,
                                         FALSE))
        {
          function = RECT_RESIZING_LOWER_LEFT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x1, private->y1 + h / 2,
                                         private->handle_w, private->left_and_right_handle_h,
                                         GTK_ANCHOR_WEST,
                                         FALSE))
        {
          function = RECT_RESIZING_LEFT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x2, private->y1 + h / 2,
                                         private->handle_w, private->left_and_right_handle_h,
                                         GTK_ANCHOR_EAST,
                                         FALSE))
        {
          function = RECT_RESIZING_RIGHT;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x1 + w / 2, private->y1,
                                         private->top_and_bottom_handle_w, private->handle_h,
                                         GTK_ANCHOR_NORTH,
                                         FALSE))
        {
          function = RECT_RESIZING_TOP;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x1 + w / 2, private->y2,
                                         private->top_and_bottom_handle_w, private->handle_h,
                                         GTK_ANCHOR_SOUTH,
                                         FALSE))
        {
          function = RECT_RESIZING_BOTTOM;
        }
      else if (gimp_draw_tool_on_handle (draw_tool, display,
                                         coords->x, coords->y,
                                         GIMP_HANDLE_SQUARE,
                                         private->x1 + w / 2,
                                         private->y1 + h / 2,
                                         tw - private->handle_w * 2,
                                         th - private->handle_h * 2,
                                         GTK_ANCHOR_CENTER,
                                         FALSE))
        {
          function = RECT_MOVING;
        }
      else
        {
          /* FIXME: This is currently the only measure done to make this area
           * dead. In the final code the concrete rectangle tools will have to
           * be written to handle this state.
           */
          function = RECT_DEAD;
        }
    }
  else
    {
      /*  otherwise, the new function will be creating, since we want
       *  to start a new rectangle
       */
      function = RECT_CREATING;
    }

  gimp_rectangle_tool_set_function (GIMP_RECTANGLE_TOOL (tool), function);
}

void
gimp_rectangle_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpRectangleTool        *rectangle;
  GimpRectangleToolPrivate *private;
  GimpCursorType            cursor = GIMP_CURSOR_CROSSHAIR_SMALL;

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
          cursor = GIMP_CURSOR_MOVE;
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
}

void
gimp_rectangle_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (draw_tool));

  tool    = GIMP_TOOL (draw_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (private->function == RECT_INACTIVE)
    return;

  gimp_draw_tool_draw_rectangle (draw_tool, FALSE,
                                 private->x1,
                                 private->y1,
                                 private->x2 - private->x1,
                                 private->y2 - private->y1,
                                 FALSE);

  switch (private->function)
    {
    case RECT_MOVING:
      if (gimp_tool_control_is_active (tool->control))
        break;
      /* else fallthrough */

    case RECT_DEAD:
    case RECT_CREATING:
      gimp_draw_tool_draw_corner (draw_tool, FALSE,
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->handle_h,
                                  GTK_ANCHOR_NORTH_WEST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool, FALSE,
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->handle_h,
                                  GTK_ANCHOR_NORTH_EAST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool, FALSE,
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->handle_h,
                                  GTK_ANCHOR_SOUTH_WEST, FALSE);
      gimp_draw_tool_draw_corner (draw_tool, FALSE,
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->handle_h,
                                  GTK_ANCHOR_SOUTH_EAST, FALSE);
      break;

    case RECT_RESIZING_TOP:
    case RECT_RESIZING_BOTTOM:
      gimp_draw_tool_draw_corner (draw_tool,
                                  ! gimp_tool_control_is_active (tool->control),
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->top_and_bottom_handle_w,
                                  private->handle_h,
                                  gimp_rectangle_tool_get_anchor (private),
                                  FALSE);
      break;

    case RECT_RESIZING_LEFT:
    case RECT_RESIZING_RIGHT:
      gimp_draw_tool_draw_corner (draw_tool,
                                  ! gimp_tool_control_is_active (tool->control),
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->left_and_right_handle_h,
                                  gimp_rectangle_tool_get_anchor (private),
                                  FALSE);
      break;

    default:
      gimp_draw_tool_draw_corner (draw_tool,
                                  ! gimp_tool_control_is_active (tool->control),
                                  private->x1, private->y1,
                                  private->x2, private->y2,
                                  private->handle_w,
                                  private->handle_h,
                                  gimp_rectangle_tool_get_anchor (private),
                                  FALSE);
      break;
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
                                (2 * y1 + (1 + SQRT5) * y2) / (3 + SQRT5),
                                x2,
                                (2 * y1 + (1 + SQRT5) * y2) / (3 + SQRT5),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                x1,
                                ((1 + SQRT5) * y1 + 2 * y2) / (3 + SQRT5),
                                x2,
                                ((1 + SQRT5) * y1 + 2 * y2) / (3 + SQRT5),
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                (2 * x1 + (1 + SQRT5) * x2) / (3 + SQRT5),
                                y1,
                                (2 * x1 + (1 + SQRT5) * x2) / (3 + SQRT5),
                                y2,
                                FALSE);
      gimp_draw_tool_draw_line (draw_tool,
                                ((1 + SQRT5) * x1 + 2 * x2) / (3 + SQRT5),
                                y1,
                                ((1 + SQRT5) * x1 + 2 * x2) / (3 + SQRT5),
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
  gint                      tw,  th;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  if (! tool->display)
    return;

  gimp_rectangle_tool_set_highlight (rectangle);

  shell = GIMP_DISPLAY_SHELL (tool->display->shell);

  gimp_display_shell_transform_xy (shell,
                                   private->x1, private->y1,
                                   &dx1, &dy1,
                                   FALSE);
  gimp_display_shell_transform_xy (shell,
                                   private->x2, private->y2,
                                   &dx2, &dy2,
                                   FALSE);

  tw = dx2 - dx1;
  th = dy2 - dy1;

  private->handle_w = tw / 4;
  private->handle_h = th / 4;

  private->handle_w =
    CLAMP (private->handle_w, MIN_HANDLE_SIZE, MAX_HANDLE_SIZE);
  private->handle_h =
    CLAMP (private->handle_h, MIN_HANDLE_SIZE, MAX_HANDLE_SIZE);

  private->top_and_bottom_handle_w = tw - 3 * private->handle_w;
  private->left_and_right_handle_h = th - 3 * private->handle_h;

  private->top_and_bottom_handle_w =
    CLAMP (private->top_and_bottom_handle_w, MIN_HANDLE_SIZE, G_MAXINT);
  private->left_and_right_handle_h =
    CLAMP (private->left_and_right_handle_h, MIN_HANDLE_SIZE, G_MAXINT);
}

static void
gimp_rectangle_tool_start (GimpRectangleTool *rectangle,
                           GimpDisplay       *display)
{
  GimpTool                    *tool = GIMP_TOOL (rectangle);
  GimpRectangleOptionsPrivate *options_private;

  options_private =
    GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (gimp_tool_get_options (tool));

  tool->display = display;
  gimp_rectangle_tool_configure (rectangle);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->display,
                                _("Rectangle: "), 0, " x ", 0, NULL);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->display);

  if (options_private->auto_shrink_button)
    {
      g_signal_connect_swapped (options_private->auto_shrink_button, "clicked",
                                G_CALLBACK (gimp_rectangle_tool_auto_shrink),
                                rectangle);

      gtk_widget_set_sensitive (options_private->auto_shrink_button, TRUE);
    }
}

static void
gimp_rectangle_tool_halt (GimpRectangleTool *rectangle)
{
  GimpTool                    *tool = GIMP_TOOL (rectangle);
  GimpRectangleOptionsPrivate *options_private;

  options_private =
    GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (gimp_tool_get_options (tool));

  if (tool->display)
    gimp_display_shell_set_highlight (GIMP_DISPLAY_SHELL (tool->display->shell),
                                      NULL);

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rectangle)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (rectangle));

  if (gimp_tool_control_is_active (tool->control))
    gimp_tool_control_halt (tool->control);

  tool->display  = NULL;
  tool->drawable = NULL;

  gimp_rectangle_tool_set_function (rectangle, RECT_INACTIVE);

  if (options_private->auto_shrink_button)
    {
      gtk_widget_set_sensitive (options_private->auto_shrink_button, FALSE);

      g_signal_handlers_disconnect_by_func (options_private->auto_shrink_button,
                                            gimp_rectangle_tool_auto_shrink,
                                            rectangle);
    }
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
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gdouble                      x;
  gdouble                      y;
  gdouble                      width;
  gdouble                      height;
  gdouble                      center_x, center_y;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

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

  if (! options_private->fixed_width)
    g_object_set (options,
                  "width",  width,
                  NULL);

  if (! options_private->fixed_height)
    g_object_set (options,
                  "height", height,
                  NULL);

  g_object_set (options,
                "center-x", center_x,
                "center-y", center_y,
                NULL);

  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_options_notify,
                                     rectangle);

}

static void
gimp_rectangle_tool_synthesize_motion (GimpRectangleTool *rectangle,
                                       gint               function,
                                       gint               new_x,
                                       gint               new_y)
{
  GimpRectangleToolPrivate *private;
  GimpRectangleFunction     old_function;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  old_function = private->function;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

  gimp_rectangle_tool_set_function (rectangle, function);

  gimp_rectangle_tool_update_with_coord (rectangle,
                                         new_x,
                                         new_y);

  /* We must update this. */
  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

  gimp_rectangle_tool_update_options (rectangle,
                                      GIMP_TOOL (rectangle)->display);

  gimp_rectangle_tool_set_function (rectangle, old_function);

  gimp_rectangle_tool_configure (rectangle);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));

  gimp_rectangle_tool_rectangle_changed (rectangle);
}

static void
gimp_rectangle_tool_options_notify (GimpRectangleOptions *options,
                                    GParamSpec           *pspec,
                                    GimpRectangleTool    *rectangle)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptionsPrivate *options_private;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);


  if (! strcmp (pspec->name, "guide"))
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rectangle));

      private->guide = options_private->guide;

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rectangle));

      return;
    }

  if (! GIMP_TOOL (rectangle)->display)
    return;

  if (! strcmp (pspec->name, "x0"))
    {
      if (private->x1 != options_private->x0)
        gimp_rectangle_tool_synthesize_motion (rectangle,
                                               RECT_MOVING,
                                               options_private->x0,
                                               private->y1);
    }
  else if (! strcmp (pspec->name, "y0"))
    {
      if (private->y1 != options_private->y0)
        gimp_rectangle_tool_synthesize_motion (rectangle,
                                               RECT_MOVING,
                                               private->x1,
                                               options_private->y0);
    }
  else if (! strcmp (pspec->name, "width"))
    {
      /* Calculate x2, y2 that will create a rectangle of given width, for the
       * current options.
       */
      gint x2;

      if (private->x2 - private->x1 != options_private->width)
        {
          if (options_private->fixed_center)
            {
              x2 = private->center_x_on_fixed_center +
                options_private->width / 2;
            }
          else
            {
              x2 = private->x1 + options_private->width;
            }

          gimp_rectangle_tool_synthesize_motion (rectangle,
                                                 RECT_RESIZING_RIGHT,
                                                 x2,
                                                 private->y2);
        }
    }
  else if (! strcmp (pspec->name, "height"))
    {
      /* Calculate x2, y2 that will create a rectangle of given height, for the
       * current options.
       */
      gint y2;

      if (private->y2 - private->y1 != options_private->height)
        {
          if (options_private->fixed_center)
            {
              y2 = private->center_y_on_fixed_center +
                options_private->height / 2;
            }
          else
            {
              y2 = private->y1 + options_private->height;
            }

          gimp_rectangle_tool_synthesize_motion (rectangle,
                                                 RECT_RESIZING_BOTTOM,
                                                 private->x2,
                                                 y2);
        }
    }
  else if (! strcmp (pspec->name, "fixed-aspect")     ||
           ! strcmp (pspec->name, "aspect-numerator") ||
           ! strcmp (pspec->name, "aspect-denominator"))
    {
      if (options_private->fixed_aspect)
        {
          gimp_rectangle_tool_synthesize_motion (rectangle,
                                                 RECT_RESIZING_LOWER_RIGHT,
                                                 private->x2,
                                                 private->y2);
        }
    }
  else if (! strcmp (pspec->name, "highlight"))
    {
      gimp_rectangle_tool_set_highlight (rectangle);
    }
}

GimpRectangleFunction
gimp_rectangle_tool_get_function (GimpRectangleTool *rectangle)
{
  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (rectangle), RECT_INACTIVE);

  return GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle)->function;
}

void
gimp_rectangle_tool_set_function (GimpRectangleTool     *rectangle,
                                  GimpRectangleFunction  function)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (rectangle));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle);

  /* redraw the tool when the function changes */
  /* FIXME: should also update the cursor      */
  if (private->function != function)
    {
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (rectangle);

      gimp_draw_tool_pause (draw_tool);

      private->function = function;

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_rectangle_tool_rectangle_changed (GimpRectangleTool *rectangle)
{
  g_signal_emit (rectangle,
                 gimp_rectangle_tool_signals[RECTANGLE_CHANGED], 0);
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

static GtkAnchorType
gimp_rectangle_tool_get_anchor (GimpRectangleToolPrivate *private)
{
  switch (private->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
      return GTK_ANCHOR_NORTH_WEST;

    case RECT_RESIZING_UPPER_RIGHT:
      return GTK_ANCHOR_NORTH_EAST;

    case RECT_RESIZING_LOWER_LEFT:
      return GTK_ANCHOR_SOUTH_WEST;

    case RECT_RESIZING_LOWER_RIGHT:
      return GTK_ANCHOR_SOUTH_EAST;

    case RECT_RESIZING_LEFT:
      return GTK_ANCHOR_WEST;

    case RECT_RESIZING_RIGHT:
      return GTK_ANCHOR_EAST;

    case RECT_RESIZING_TOP:
      return GTK_ANCHOR_NORTH;

    case RECT_RESIZING_BOTTOM:
      return GTK_ANCHOR_SOUTH;

    default:
      return GTK_ANCHOR_CENTER;
    }
}

static void
gimp_rectangle_tool_set_highlight (GimpRectangleTool *rectangle)
{
  GimpTool             *tool      = GIMP_TOOL (rectangle);
  GimpRectangleOptions *options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  GimpDisplayShell     *shell     = GIMP_DISPLAY_SHELL (tool->display->shell);
  gboolean              highlight = FALSE;

  g_object_get (options, "highlight", &highlight, NULL);

  if (highlight)
    {
      GimpRectangleToolPrivate *private;
      GdkRectangle              rect;

      private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

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

/**
 * gimp_rectangle_tool_get_other_side:
 * @rectangle_tool: A #GimpRectangleTool.
 * @other_x:        Pointer to where to set the other-x string.
 * @other_y:        Pointer to where to set the other-y string.
 *
 * Calculates what property variables that hold the coordinates of the opposite
 * side (either the opposite corner or literally the opposite side), based on
 * the current function. The opposite of a corner needs two coordinates, the
 * opposite of a side only needs one.
 */
static void
gimp_rectangle_tool_get_other_side (GimpRectangleTool *rectangle_tool,
                                    const gchar      **other_x,
                                    const gchar      **other_y)
{
  GimpRectangleToolPrivate *private;

  private = gimp_rectangle_tool_get_private (GIMP_RECTANGLE_TOOL (rectangle_tool));

  switch (private->function)
    {
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      *other_x = "x1";
      break;

    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      *other_x = "x2";
      break;

    case RECT_RESIZING_TOP:
    case RECT_RESIZING_BOTTOM:
    default:
      *other_x = NULL;
      break;
    }

  switch (private->function)
    {
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_BOTTOM:
      *other_y = "y1";
      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_TOP:
      *other_y = "y2";
      break;

    case RECT_RESIZING_LEFT:
    case RECT_RESIZING_RIGHT:
    default:
      *other_y = NULL;
      break;
    }
}

static void
gimp_rectangle_tool_get_other_side_coord (GimpRectangleTool *rectangle_tool,
                                          gint              *other_side_x,
                                          gint              *other_side_y)
{
  const gchar *other_x = NULL;
  const gchar *other_y = NULL;

  gimp_rectangle_tool_get_other_side (GIMP_RECTANGLE_TOOL (rectangle_tool),
                                      &other_x,
                                      &other_y);
  if (other_x)
    g_object_get (rectangle_tool,
                  other_x, other_side_x,
                  NULL);
  if (other_y)
    g_object_get (rectangle_tool,
                  other_y, other_side_y,
                  NULL);
}

static void
gimp_rectangle_tool_set_other_side_coord (GimpRectangleTool *rectangle_tool,
                                          gint               other_side_x,
                                          gint               other_side_y)
{
  const gchar *other_x = NULL;
  const gchar *other_y = NULL;

  gimp_rectangle_tool_get_other_side (GIMP_RECTANGLE_TOOL (rectangle_tool),
                                      &other_x,
                                      &other_y);
  if (other_x)
    g_object_set (rectangle_tool,
                  other_x, other_side_x,
                  NULL);
  if (other_y)
    g_object_set (rectangle_tool,
                  other_y, other_side_y,
                  NULL);
}

/**
 * gimp_rectangle_tool_apply_coord:
 * @param:     A #GimpRectangleTool.
 * @coord_x:   X of coord.
 * @coord_y:   Y of coord.
 *
 * Adjust the rectangle to the new position specified by passed coordinate,
 * taking fixed_center into account, which means it expands the rectagle around
 * the center point.
 */
static void
gimp_rectangle_tool_apply_coord (GimpRectangleTool *rectangle_tool,
                                 gint               coord_x,
                                 gint               coord_y)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  if (private->function == RECT_INACTIVE)
    g_warning ("function is RECT_INACTIVE while mouse is moving");

  if (private->function == RECT_MOVING)
    {
      /* Preserve width and height while moving the grab-point to where the
       * cursor is.
       */
      gint w = private->x2 - private->x1;
      gint h = private->y2 - private->y1;

      private->x1 = coord_x;
      private->y1 = coord_y;

      private->x2 = private->x1 + w;
      private->y2 = private->y1 + h;

      /* We are done already. */
      return;
    }

  switch (private->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LEFT:
      private->x1 = coord_x;

      if (options_private->fixed_center)
        private->x2 = 2 * private->center_x_on_fixed_center - private->x1;

      break;

    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_RIGHT:
      private->x2 = coord_x;

      if (options_private->fixed_center)
        private->x1 = 2 * private->center_x_on_fixed_center - private->x2;

      break;
    }

  switch (private->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:
      private->y1 = coord_y;

      if (options_private->fixed_center)
        private->y2 = 2 * private->center_y_on_fixed_center - private->y1;

      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:
      private->y2 = coord_y;

      if (options_private->fixed_center)
        private->y1 = 2 * private->center_y_on_fixed_center - private->y2;

      break;
    }
}

/**
 * gimp_rectangle_tool_clamp_width:
 * @rectangle_tool: A #GimpRectangleTool.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps rectangle inside specified bounds, providing information of where
 * clamping was done. Can also clamp symmetrically.
 */
static void
gimp_rectangle_tool_clamp (GimpRectangleTool       *rectangle_tool,
                           ClampedSide             *clamped_sides,
                           GimpRectangleConstraint  constraint,
                           gboolean                 symmetrically)
{
  gimp_rectangle_tool_clamp_width (rectangle_tool,
                                   clamped_sides,
                                   constraint,
                                   symmetrically);

  gimp_rectangle_tool_clamp_height (rectangle_tool,
                                    clamped_sides,
                                    constraint,
                                    symmetrically);
}

/**
 * gimp_rectangle_tool_clamp_width:
 * @rectangle_tool: A #GimpRectangleTool.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps height of rectangle. Set symmetrically to true when using for
 * fixed_center:ed rectangles, since that will clamp symmetrically which is just
 * what is needed.
 *
 * When this function constrains, it puts what it constrains in
 * @constraint. This information is essential when an aspect ratio is to be
 * applied.
 */
static void
gimp_rectangle_tool_clamp_width (GimpRectangleTool       *rectangle_tool,
                                 ClampedSide             *clamped_sides,
                                 GimpRectangleConstraint  constraint,
                                 gboolean                 symmetrically)
{
  GimpRectangleToolPrivate *private;
  gint                      min_x;
  gint                      max_x;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);

  gimp_rectangle_tool_get_constraints (rectangle_tool,
                                       &min_x,
                                       NULL,
                                       &max_x,
                                       NULL,
                                       constraint);
  if (private->x1 < min_x)
    {
      gint dx = min_x - private->x1;

      private->x1 += dx;

      if (symmetrically)
        {
          private->x2 -= dx;

          if (private->x2 < min_x)
            private->x1 = private->x2 = min_x;
        }

      if (clamped_sides != NULL)
        *clamped_sides |= CLAMPED_LEFT;
    }

  if (private->x2 > max_x)
    {
      gint dx = max_x - private->x2;

      private->x2 += dx;

      if (symmetrically)
        {
          private->x1 -= dx;

          if (private->x1 > max_x)
            private->x1 = max_x;
        }

      if (clamped_sides != NULL)
        *clamped_sides |= CLAMPED_RIGHT;
    }
}

/**
 * gimp_rectangle_tool_clamp_height:
 * @rectangle_tool: A #GimpRectangleTool.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps height of rectangle. Set symmetrically to true when using for
 * fixed_center:ed rectangles, since that will clamp symmetrically which is just
 * what is needed.
 *
 * When this function constrains, it puts what it constrains in
 * @constraint. This information is essential when an aspect ratio is to be
 * applied.
 */
static void
gimp_rectangle_tool_clamp_height (GimpRectangleTool       *rectangle_tool,
                                  ClampedSide             *clamped_sides,
                                  GimpRectangleConstraint  constraint,
                                  gboolean                 symmetrically)
{
  GimpRectangleToolPrivate *private;
  gint                      min_y;
  gint                      max_y;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);

  gimp_rectangle_tool_get_constraints (rectangle_tool,
                                             NULL,
                                             &min_y,
                                             NULL,
                                             &max_y,
                                             constraint);
  if (private->y1 < min_y)
    {
      gint dy = min_y - private->y1;

      private->y1 += dy;

      if (symmetrically)
        {
          private->y2 -= dy;

          if (private->y2 < min_y)
            private->y1 = private->y2 = min_y;
        }

      if (clamped_sides != NULL)
        *clamped_sides |= CLAMPED_TOP;
    }

  if (private->y2 > max_y)
    {
      gint dy = max_y - private->y2;

      private->y2 += dy;

      if (symmetrically)
        {
          private->y1 -= dy;

          if (private->y1 > max_y)
            private->y1 = private->y2 = max_y;
        }

      if (clamped_sides != NULL)
        *clamped_sides |= CLAMPED_BOTTOM;
    }
}

/**
 * gimp_rectangle_tool_keep_inside:
 * @rectangle_tool: A #GimpRectangleTool.
 *
 * If the rectangle is outside of the canvas, move it into it. If the rectangle is
 * larger than the canvas in any direction, make it fill the canvas in that direction.
 */
static void
gimp_rectangle_tool_keep_inside (GimpRectangleTool      *rectangle_tool,
                                 GimpRectangleConstraint constraint)
{
  gimp_rectangle_tool_keep_inside_horizontally (rectangle_tool,
                                                constraint);

  gimp_rectangle_tool_keep_inside_vertically (rectangle_tool,
                                              constraint);
}

/**
 * gimp_rectangle_tool_keep_inside_horizontally:
 * @rectangle_tool: A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint horizontally, move it
 * inside. If it is too big to fit inside, make it just as big as the width
 * limit.
 */
static void
gimp_rectangle_tool_keep_inside_horizontally (GimpRectangleTool      *rectangle_tool,
                                              GimpRectangleConstraint constraint)
{
  GimpRectangleToolPrivate    *private;
  gint                         min_x;
  gint                         max_x;

  gimp_rectangle_tool_get_constraints (rectangle_tool,
                                             &min_x,
                                             NULL,
                                             &max_x,
                                             NULL,
                                             constraint);

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);

  if (max_x - min_x < private->x2 - private->x1)
    {
      private->x1 = min_x;
      private->x2 = max_x;
    }
  else
    {
      if (private->x1 < min_x)
        {
          gint dx = min_x - private->x1;

          private->x1 += dx;
          private->x2 += dx;
        }
      if (private->x2 > max_x)
        {
          gint dx = max_x - private->x2;

          private->x1 += dx;
          private->x2 += dx;
        }
    }
}

/**
 * gimp_rectangle_tool_keep_inside_vertically:
 * @rectangle_tool: A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint vertically, move it
 * inside. If it is too big to fit inside, make it just as big as the width
 * limit.
 */
static void
gimp_rectangle_tool_keep_inside_vertically (GimpRectangleTool      *rectangle_tool,
                                            GimpRectangleConstraint constraint)
{
  GimpRectangleToolPrivate    *private;
  gint                         min_y;
  gint                         max_y;

  gimp_rectangle_tool_get_constraints (rectangle_tool,
                                             NULL,
                                             &min_y,
                                             NULL,
                                             &max_y,
                                             constraint);

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);

  if (max_y - min_y < private->y2 - private->y1)
    {
      private->y1 = min_y;
      private->y2 = max_y;
    }
  else
    {
      if (private->y1 < min_y)
        {
          gint dy = min_y - private->y1;

          private->y1 += dy;
          private->y2 += dy;
        }
      if (private->y2 > max_y)
        {
          gint dy = max_y - private->y2;

          private->y1 += dy;
          private->y2 += dy;
        }
    }
}

/**
 * gimp_rectangle_tool_apply_fixed_width:
 * @rectangle_tool: A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * Makes the rectangle have a fixed_width, following the constrainment rules
 * of fixed widths as well. Please refer to the rectangle tools spec.
 */
static void
gimp_rectangle_tool_apply_fixed_width (GimpRectangleTool      *rectangle_tool,
                                       GimpRectangleConstraint constraint)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (private->function)
    {
    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_LEFT:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    options_private->width / 2;
      private->x2 = private->x1 + options_private->width;

      break;

    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_RIGHT:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    options_private->width / 2;
      private->x2 = private->x1 + options_private->width;

      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_rectangle_tool_keep_inside_horizontally (rectangle_tool,
                                                constraint);
}

/**
 * gimp_rectangle_tool_apply_fixed_height:
 * @rectangle_tool: A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * Makes the rectangle have a fixed_height, following the constrainment rules
 * of fixed heights as well. Please refer to the rectangle tools spec.
 */
static void
gimp_rectangle_tool_apply_fixed_height (GimpRectangleTool      *rectangle_tool,
                                        GimpRectangleConstraint constraint)

{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  switch (private->function)
    {
    case RECT_RESIZING_UPPER_LEFT:
    case RECT_RESIZING_UPPER_RIGHT:
    case RECT_RESIZING_TOP:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    options_private->height / 2;
      private->y2 = private->y1 + options_private->height;

      break;

    case RECT_RESIZING_LOWER_LEFT:
    case RECT_RESIZING_LOWER_RIGHT:
    case RECT_RESIZING_BOTTOM:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    options_private->height / 2;
      private->y2 = private->y1 + options_private->height;

      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_rectangle_tool_keep_inside_vertically (rectangle_tool,
                                              constraint);
}

/**
 * gimp_rectangle_tool_apply_aspect:
 * @rectangle_tool: A #GimpRectangleTool.
 * @aspect:         The desired aspect.
 * @clamped_sides:  Bitfield of sides that have been clamped.
 *
 * Adjust the rectangle to the desired aspect.
 *
 * Sometimes, a side must not be moved outwards, for example if a the RIGHT side
 * has been clamped previously, we must not move the RIGHT side to the right,
 * since that would violate the constraint again. The clamped_sides bitfield
 * keeps track of sides that have previously been clamped.
 *
 * If fixed_center is used, the function adjusts the aspect by symmetrically
 * adjusting the left and right, or top and bottom side.
 */
static void
gimp_rectangle_tool_apply_aspect (GimpRectangleTool *rectangle_tool,
                                  gdouble            aspect,
                                  gint               clamped_sides)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gint                         current_w;
  gint                         current_h;
  gdouble                      current_aspect;
  SideToResize                 side_to_resize = SIDE_TO_RESIZE_NONE;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  current_w = private->x2 - private->x1;
  current_h = private->y2 - private->y1;

  current_aspect = current_w / (gdouble) current_h;

  /* Do we have to do anything? */
  if (current_aspect == aspect)
    {
      return;
    }

  if (options_private->fixed_center)
    {
      /* We may only adjust the sides symmetrically to get desired aspect. */
      if (current_aspect > aspect)
        {
          /* We prefer to use top and bottom (since that will make the cursor
           * remain on the rectangle edge), unless that is what the user has
           * grabbed.
           */
          switch (private->function)
            {
            case RECT_RESIZING_LEFT:
            case RECT_RESIZING_RIGHT:
            case RECT_RESIZING_UPPER_LEFT:
            case RECT_RESIZING_UPPER_RIGHT:
            case RECT_RESIZING_LOWER_LEFT:
            case RECT_RESIZING_LOWER_RIGHT:
              if (!(clamped_sides & CLAMPED_TOP) &&
                  !(clamped_sides & CLAMPED_BOTTOM))
                {
                  side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
                }
              else
                {
                  side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
                }
              break;

            case RECT_RESIZING_TOP:
            case RECT_RESIZING_BOTTOM:
            default:
              side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
              break;
            }
        }
      else /* (current_aspect < aspect) */
        {
          /* We prefer to use left and right (since that will make the cursor
           * remain on the rectangle edge), unless that is what the user has
           * grabbed.
           */
          switch (private->function)
            {
            case RECT_RESIZING_TOP:
            case RECT_RESIZING_BOTTOM:
            case RECT_RESIZING_UPPER_LEFT:
            case RECT_RESIZING_UPPER_RIGHT:
            case RECT_RESIZING_LOWER_LEFT:
            case RECT_RESIZING_LOWER_RIGHT:
              if (!(clamped_sides & CLAMPED_LEFT) &&
                  !(clamped_sides & CLAMPED_RIGHT))
                {
                  side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
                }
              else
                {
                  side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
                }
              break;

            case RECT_RESIZING_LEFT:
            case RECT_RESIZING_RIGHT:
            default:
              side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
              break;
            }
        }
    }
  else if (current_aspect > aspect)
    {
      /* We can safely pick LEFT or RIGHT, since using those sides will make the
       * rectangle smaller, so we don't need to check for clamped_sides. We may
       * only use TOP and BOTTOM if not those sides have been clamped, since
       * using them will make the rectangle bigger.
       */
      switch (private->function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case RECT_RESIZING_UPPER_RIGHT:
          if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case RECT_RESIZING_LOWER_LEFT:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case RECT_RESIZING_LEFT:
          if (!(clamped_sides & CLAMPED_TOP) && !(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case RECT_RESIZING_RIGHT:
          if (!(clamped_sides & CLAMPED_TOP) && !(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case RECT_RESIZING_BOTTOM:
        case RECT_RESIZING_TOP:
          side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          break;

        case RECT_MOVING:
        default:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;
        }
    }
  else /* (current_aspect < aspect) */
    {
      /* We can safely pick TOP or BOTTOM, since using those sides will make the
       * rectangle smaller, so we don't need to check for clamped_sides. We may
       * only use LEFT and RIGHT if not those sides have been clamped, since
       * using them will make the rectangle bigger.
       */
      switch (private->function)
        {
        case RECT_RESIZING_UPPER_LEFT:
          if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case RECT_RESIZING_UPPER_RIGHT:
          if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case RECT_RESIZING_LOWER_LEFT:
          if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case RECT_RESIZING_LOWER_RIGHT:
          if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case RECT_RESIZING_TOP:
          if (!(clamped_sides & CLAMPED_LEFT) && !(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case RECT_RESIZING_BOTTOM:
          if (!(clamped_sides & CLAMPED_LEFT) && !(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case RECT_RESIZING_LEFT:
        case RECT_RESIZING_RIGHT:
          side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          break;

        case RECT_MOVING:
        default:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;
        }
    }

  /* We now know what side(s) we should resize, so now we just solve the
   * aspect equation for that side(s).
   */
  switch (side_to_resize)
    {
    case SIDE_TO_RESIZE_NONE:
      return;

    case SIDE_TO_RESIZE_LEFT:
      private->x1 = private->x2 - aspect * current_h;
      break;

    case SIDE_TO_RESIZE_RIGHT:
      private->x2 = private->x1 + aspect * current_h;
      break;

    case SIDE_TO_RESIZE_TOP:
      private->y1 = private->y2 - current_w / aspect;
      break;

    case SIDE_TO_RESIZE_BOTTOM:
      private->y2 = private->y1 + current_w / aspect;
      break;

    case SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY:
      {
        gint correct_h = current_w / aspect;

        private->y1 = private->center_y_on_fixed_center - correct_h / 2;
        private->y2 = private->y1 + correct_h;
      }
      break;

    case SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY:
      {
        gint correct_w = current_h * aspect;

        private->x1 = private->center_x_on_fixed_center - correct_w / 2;
        private->x2 = private->x1 + correct_w;
      }
      break;
    }
}

/**
 * gimp_rectangle_tool_update_with_coord:
 * @rectangle_tool: A #GimpRectangleTool.
 * @new_x:          New X-coordinate in the context of the current function.
 * @new_y:          New Y-coordinate in the context of the current function.
 *
 * The core rectangle adjustment function. It updates the rectangle for the
 * passed cursor coordinate, taking current function and tool options into
 * account.  It also updates the current private->function if necessary.
 */
static void
gimp_rectangle_tool_update_with_coord (GimpRectangleTool *rectangle_tool,
                                       gint               new_x,
                                       gint               new_y)
{
  GimpTool                    *tool;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleConstraint      constraint_to_use;

  tool            = GIMP_TOOL (rectangle_tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  /* Move the corner or edge the user currently has grabbed. */
  gimp_rectangle_tool_apply_coord (rectangle_tool,
                                   new_x,
                                   new_y);

  /* Update private->function. The function changes if the user "flips" the
   * rectangle.
   */
  gimp_rectangle_tool_check_function (rectangle_tool);

  /* E.g. the crop tool will set up clamping always to be used, and this
   * function handles that.
   */
  gimp_rectangle_tool_handle_general_clamping (rectangle_tool);

  /* Calculate what constraint to use when needed. */
  constraint_to_use = gimp_rectangle_tool_get_constraint (rectangle_tool);
  if (constraint_to_use == GIMP_RECTANGLE_CONSTRAIN_NONE)
    constraint_to_use = GIMP_RECTANGLE_CONSTRAIN_IMAGE;

  /* fixed_aspect and fixed_width/height are mutually exclusive. */
  if (options_private->fixed_aspect)
    {
      gdouble aspect;

      aspect = CLAMP (options_private->aspect_numerator /
                      options_private->aspect_denominator,
                      1.0 / tool->display->image->height,
                      tool->display->image->width);

      if (private->function != RECT_MOVING)
        {
          ClampedSide clamped_sides = CLAMPED_NONE;

          gimp_rectangle_tool_apply_aspect (rectangle_tool,
                                            aspect,
                                            clamped_sides);

          /* After we have applied aspect, we might have taken the rectangle
           * outside of constraint, so clamp and apply aspect again. We will get
           * the right result this time, since 'clamped_sides' will be setup
           * correctly now.
           */
          gimp_rectangle_tool_clamp (rectangle_tool,
                                     &clamped_sides,
                                     constraint_to_use,
                                     options_private->fixed_center);

          gimp_rectangle_tool_apply_aspect (rectangle_tool,
                                            aspect,
                                            clamped_sides);
        }
      else
        {
          gimp_rectangle_tool_apply_aspect (rectangle_tool,
                                            aspect,
                                            CLAMPED_NONE);

          /* When fixed ratio is used, we always want the rectangle inside the
           * canvas.
           */
          gimp_rectangle_tool_keep_inside (rectangle_tool,
                                           constraint_to_use);
        }
    }
  else /* !options_private->fixed_aspect */
    {
      if (options_private->fixed_width)
        {
          gimp_rectangle_tool_apply_fixed_width (rectangle_tool,
                                                 constraint_to_use);
        }
      if (options_private->fixed_height)
        {
          gimp_rectangle_tool_apply_fixed_height (rectangle_tool,
                                                  constraint_to_use);
        }
    }
}

/**
 * gimp_rectangle_tool_get_constraints:
 * @rectangle_tool: A #GimpRectagnelTool.
 * @min_x:
 * @min_y:
 * @max_x:
 * @max_y:          Pointers of where to put constraints. NULL allowed.
 * @constraint:     Wether to return image or layer constraints.
 *
 * Calculates constraint coordinates for image or layer.
 */
static void
gimp_rectangle_tool_get_constraints (GimpRectangleTool       *rectangle_tool,
                                     gint                    *min_x,
                                     gint                    *min_y,
                                     gint                    *max_x,
                                     gint                    *max_y,
                                     GimpRectangleConstraint  constraint)
{
  GimpTool                    *tool;
  gint                         min_x_dummy;
  gint                         min_y_dummy;
  gint                         max_x_dummy;
  gint                         max_y_dummy;

  if (min_x == NULL)
    min_x = &min_x_dummy;
  if (min_y == NULL)
    min_y = &min_y_dummy;
  if (max_x == NULL)
    max_x = &max_x_dummy;
  if (max_y == NULL)
    max_y = &max_y_dummy;

  tool = GIMP_TOOL (rectangle_tool);

  switch (constraint)
    {
    case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
      *min_x = 0;
      *min_y = 0;
      *max_x = tool->display->image->width;
      *max_y = tool->display->image->height;
      break;

    case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
      {
        GimpItem *item = GIMP_ITEM (tool->drawable);

        gimp_item_offsets (item, min_x, min_y);
        *max_x = *min_x + gimp_item_width (item);
        *max_y = *min_y + gimp_item_height (item);
      }
      break;

    default:
      g_warning ("Invalid rectangle constraint.\n");
      return;
    }
}

/**
 * gimp_rectangle_tool_handle_general_clamping:
 * @rectangle_tool: A #GimpRectangleTool.
 *
 * Make sure that contraints are applied to the rectangle, either by manually
 * doing it, or by looking at the rectangle tool options and concluding it will
 * be done later.
 */
static void
gimp_rectangle_tool_handle_general_clamping (GimpRectangleTool *rectangle_tool)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleConstraint      constraint;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rectangle_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rectangle_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  constraint = gimp_rectangle_tool_get_constraint (rectangle_tool);

  /* fixed_aspect takes care of clamping by it self, so just return in case that
   * is in use. Also return if no constraints should be enforced.
   */
  if (options_private->fixed_aspect ||
      (constraint != GIMP_RECTANGLE_CONSTRAIN_IMAGE &&
       constraint != GIMP_RECTANGLE_CONSTRAIN_DRAWABLE))
    return;

  /* fixed_width and fixed_height takes care of clamping if they are turned on,
   * so we only need to care if they are not on.
   */
  if (!options_private->fixed_width)
    {
      /* Never clamp a moved rect; it causes the rectangle to get "consumed". */
      if (private->function == RECT_MOVING)
        {
          gimp_rectangle_tool_keep_inside_horizontally (rectangle_tool,
                                                        constraint);
        }
      else
        {
          gimp_rectangle_tool_clamp_width (rectangle_tool,
                                           NULL,
                                           constraint,
                                           options_private->fixed_center);
        }
    }

  if (!options_private->fixed_height)
    {
      /* Never clamp a moved rect; it causes the rectangle to get "consumed". */
      if (private->function == RECT_MOVING)
        {
          gimp_rectangle_tool_keep_inside_vertically (rectangle_tool,
                                                      constraint);
        }
      else
        {
          gimp_rectangle_tool_clamp_height (rectangle_tool,
                                            NULL,
                                            constraint,
                                            options_private->fixed_center);
        }
    }
}
