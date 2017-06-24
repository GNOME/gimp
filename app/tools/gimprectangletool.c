/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 2007 Martin Nordholts
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-utils.h"
#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpmarshal.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scroll.h"
#include "display/gimpdisplayshell-transform.h"

#include "gimpdrawtool.h"
#include "gimprectangleoptions.h"
#include "gimprectangletool.h"
#include "gimptoolcontrol.h"

#include "gimp-log.h"

#include "gimp-intl.h"


enum
{
  RECTANGLE_CHANGE_COMPLETE,
  LAST_SIGNAL
};

/*  speed of key movement  */
#define ARROW_VELOCITY   25

#define MAX_HANDLE_SIZE         50
#define MIN_HANDLE_SIZE         15
#define NARROW_MODE_HANDLE_SIZE 15
#define NARROW_MODE_THRESHOLD   45

typedef enum
{
  CLAMPED_NONE   = 0,
  CLAMPED_LEFT   = 1 << 0,
  CLAMPED_RIGHT  = 1 << 1,
  CLAMPED_TOP    = 1 << 2,
  CLAMPED_BOTTOM = 1 << 3
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


#define FEQUAL(a,b)       (fabs ((a) - (b)) < 0.0001)
#define PIXEL_FEQUAL(a,b) (fabs ((a) - (b)) < 0.5)

#define GIMP_RECTANGLE_TOOL_GET_PRIVATE(obj) \
  (gimp_rectangle_tool_get_private (GIMP_RECTANGLE_TOOL (obj)))


typedef struct _GimpRectangleToolPrivate GimpRectangleToolPrivate;

struct _GimpRectangleToolPrivate
{
  /* The following members are "constants", that is, variables that are setup
   * during gimp_rectangle_tool_button_press and then only read.
   */

  /* Whether or not the rectangle currently being rubber-banded was
   * created from scatch.
   */
  gboolean                is_new;

  /* Holds the coordinate that should be used as the "other side" when
   * fixed-center is turned off.
   */
  gdouble                 other_side_x;
  gdouble                 other_side_y;

  /* Holds the coordinate to be used as center when fixed-center is used. */
  gdouble                 center_x_on_fixed_center;
  gdouble                 center_y_on_fixed_center;

  /* True when the rectangle is being adjusted (moved or
   * rubber-banded).
   */
  gboolean                rect_adjusting;


  /* The rest of the members are internal state variables, that is, variables
   * that might change during the manipulation session of the rectangle. Make
   * sure these variables are in consistent states.
   */

  /* Coordinates of upper left and lower right rectangle corners. */
  gdouble                 x1, y1;
  gdouble                 x2, y2;

  /* Integer coordinats of upper left corner and size. We must
   * calculate this separately from the gdouble ones because sometimes
   * we don't want to affect the integer size (e.g. when moving the
   * rectangle), but that will be the case if we always calculate the
   * integer coordinates based on rounded values of the gdouble
   * coordinates even if the gdouble width remains constant.
   *
   * TODO: Change the internal double-representation of the rectangle
   * to x,y width,height instead of x1,y1 x2,y2. That way we don't
   * need to keep a separate representation of the integer version of
   * the rectangle; rounding width an height will yield consistent
   * results and not depend on position of the rectangle.
   */
  gint                    x1_int,    y1_int;
  gint                    width_int, height_int;

  /* What modification state the rectangle is in. What corner are we resizing,
   * or are we moving the rectangle? etc.
   */
  guint                   function;

  /* How to constrain the rectangle. */
  GimpRectangleConstraint constraint;

  /* What precision the rectangle will apear to have externally (it
   * will always be double internally)
   */
  GimpRectanglePrecision  precision;

  /* Previous coordinate applied to the rectangle. */
  gdouble                 lastx;
  gdouble                 lasty;

  /* Width and height of corner handles. */
  gint                    corner_handle_w;
  gint                    corner_handle_h;

  /* Width and height of side handles. */
  gint                    top_and_bottom_handle_w;
  gint                    left_and_right_handle_h;

  /* Whether or not the rectangle is in a 'narrow situation' i.e. it is
   * too small for reasonable sized handle to be inside. In this case
   * we put handles on the outside.
   */
  gboolean                narrow_mode;

  /* For what scale the handle sizes is calculated. We must cache this
   * so that we can differentiate between when the tool is resumed
   * because of zoom level just has changed or because the highlight
   * has just been updated.
   */
  gdouble                 scale_x_used_for_handle_size_calculations;
  gdouble                 scale_y_used_for_handle_size_calculations;

  /* For saving in case of cancelation. */
  gdouble                 saved_x1;
  gdouble                 saved_y1;
  gdouble                 saved_x2;
  gdouble                 saved_y2;

  gint                    suppress_updates;
};


static void          gimp_rectangle_tool_iface_base_init      (GimpRectangleToolInterface *iface);

static GimpRectangleToolPrivate *
                     gimp_rectangle_tool_get_private          (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_start                (GimpRectangleTool        *rect_tool,
                                                               GimpDisplay              *display);
static void          gimp_rectangle_tool_halt                 (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_update_options       (GimpRectangleTool        *rect_tool,
                                                               GimpDisplay              *display);

static void          gimp_rectangle_tool_options_notify       (GimpRectangleOptions     *options,
                                                               GParamSpec               *pspec,
                                                               GimpRectangleTool        *rect_tool);
static void          gimp_rectangle_tool_shell_scrolled       (GimpDisplayShell         *options,
                                                               GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_check_function       (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_rectangle_change_complete
                                                              (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_auto_shrink          (GimpRectangleTool        *rect_tool);

static gboolean      gimp_rectangle_tool_coord_outside        (GimpRectangleTool        *rect_tool,
                                                               const GimpCoords         *coords);

static gboolean      gimp_rectangle_tool_coord_on_handle      (GimpRectangleTool        *rect_tool,
                                                               const GimpCoords         *coords,
                                                               GimpHandleAnchor          anchor);

static GimpHandleAnchor gimp_rectangle_tool_get_anchor        (GimpRectangleToolPrivate *private);

static void          gimp_rectangle_tool_update_highlight     (GimpRectangleTool        *rect_tool);

static gboolean      gimp_rectangle_tool_rect_rubber_banding_func
                                                              (GimpRectangleTool        *rect_tool);
static gboolean      gimp_rectangle_tool_rect_adjusting_func  (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_update_handle_sizes  (GimpRectangleTool        *rect_tool);

static gboolean      gimp_rectangle_tool_scale_has_changed    (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_get_other_side       (GimpRectangleTool        *rect_tool,
                                                               gdouble                 **other_x,
                                                               gdouble                 **other_y);
static void          gimp_rectangle_tool_get_other_side_coord (GimpRectangleTool        *rect_tool,
                                                               gdouble                  *other_side_x,
                                                               gdouble                  *other_side_y);
static void          gimp_rectangle_tool_set_other_side_coord (GimpRectangleTool        *rect_tool,
                                                               gdouble                   other_side_x,
                                                               gdouble                   other_side_y);

static void          gimp_rectangle_tool_apply_coord          (GimpRectangleTool        *rect_tool,
                                                               gdouble                   coord_x,
                                                               gdouble                   coord_y);
static void          gimp_rectangle_tool_setup_snap_offsets   (GimpRectangleTool        *rect_tool,
                                                               const GimpCoords         *coords);

static void          gimp_rectangle_tool_clamp                (GimpRectangleTool        *rect_tool,
                                                               ClampedSide              *clamped_sides,
                                                               GimpRectangleConstraint   constraint,
                                                               gboolean                  symmetrically);
static void          gimp_rectangle_tool_clamp_width          (GimpRectangleTool        *rect_tool,
                                                               ClampedSide              *clamped_sides,
                                                               GimpRectangleConstraint   constraint,
                                                               gboolean                  symmetrically);
static void          gimp_rectangle_tool_clamp_height         (GimpRectangleTool        *rect_tool,
                                                               ClampedSide              *clamped_sides,
                                                               GimpRectangleConstraint   constraint,
                                                               gboolean                  symmetrically);

static void          gimp_rectangle_tool_keep_inside          (GimpRectangleTool        *rect_tool,
                                                               GimpRectangleConstraint   constraint);
static void          gimp_rectangle_tool_keep_inside_horizontally
                                                              (GimpRectangleTool        *rect_tool,
                                                               GimpRectangleConstraint   constraint);
static void          gimp_rectangle_tool_keep_inside_vertically
                                                              (GimpRectangleTool        *rect_tool,
                                                               GimpRectangleConstraint   constraint);

static void          gimp_rectangle_tool_apply_fixed_width    (GimpRectangleTool        *rect_tool,
                                                               GimpRectangleConstraint   constraint,
                                                               gdouble                   width);
static void          gimp_rectangle_tool_apply_fixed_height   (GimpRectangleTool        *rect_tool,
                                                               GimpRectangleConstraint   constraint,
                                                               gdouble                   height);

static void          gimp_rectangle_tool_apply_aspect         (GimpRectangleTool        *rect_tool,
                                                               gdouble                   aspect,
                                                               gint                      clamped_sides);

static void          gimp_rectangle_tool_update_with_coord    (GimpRectangleTool        *rect_tool,
                                                               gdouble                   new_x,
                                                               gdouble                   new_y);
static void          gimp_rectangle_tool_apply_fixed_rule     (GimpRectangleTool        *rect_tool);

static void          gimp_rectangle_tool_get_constraints      (GimpRectangleTool        *rect_tool,
                                                               gint                     *min_x,
                                                               gint                     *min_y,
                                                               gint                     *max_x,
                                                               gint                     *max_y,
                                                               GimpRectangleConstraint   constraint);

static void          gimp_rectangle_tool_handle_general_clamping
                                                              (GimpRectangleTool        *rect_tool);
static void          gimp_rectangle_tool_update_int_rect      (GimpRectangleTool        *rect_tool);
static void          gimp_rectangle_tool_get_public_rect      (GimpRectangleTool        *rect_tool,
                                                               gdouble                  *pub_x1,
                                                               gdouble                  *pub_y1,
                                                               gdouble                  *pub_x2,
                                                               gdouble                  *pub_y2);
static void          gimp_rectangle_tool_adjust_coord         (GimpRectangleTool        *rect_tool,
                                                               gdouble                   coord_x_input,
                                                               gdouble                   coord_y_input,
                                                               gdouble                  *coord_x_output,
                                                               gdouble                  *coord_y_output);


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
      gimp_rectangle_tool_signals[RECTANGLE_CHANGE_COMPLETE] =
        g_signal_new ("rectangle-change-complete",
                      G_TYPE_FROM_INTERFACE (iface),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (GimpRectangleToolInterface,
                                       rectangle_change_complete),
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

      g_object_interface_install_property (iface,
                                           g_param_spec_enum ("precision",
                                                              NULL, NULL,
                                                              GIMP_TYPE_RECTANGLE_PRECISION,
                                                              GIMP_RECTANGLE_PRECISION_INT,
                                                              GIMP_PARAM_READWRITE));
      g_object_interface_install_property (iface,
                                           g_param_spec_boolean ("narrow-mode",
                                                                 NULL, NULL,
                                                                 FALSE,
                                                                 GIMP_PARAM_READWRITE));

      iface->execute                   = NULL;
      iface->cancel                    = NULL;
      iface->rectangle_change_complete = NULL;

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
 * gimp_rectangle_tool_init:
 * @rect_tool:
 *
 * Initializes the GimpRectangleTool.
 **/
void
gimp_rectangle_tool_init (GimpRectangleTool *rect_tool)
{
  /* No need to initialize anything yet. */
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
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_PRECISION,
                                    "precision");
  g_object_class_override_property (klass,
                                    GIMP_RECTANGLE_TOOL_PROP_NARROW_MODE,
                                    "narrow-mode");
}

void
gimp_rectangle_tool_set_constraint (GimpRectangleTool       *tool,
                                    GimpRectangleConstraint  constraint)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  private->constraint = constraint;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_rectangle_tool_clamp (tool,
                             NULL,
                             constraint,
                             FALSE);

  gimp_rectangle_tool_update_highlight (tool);
  gimp_rectangle_tool_update_handle_sizes (tool);

  gimp_rectangle_tool_rectangle_change_complete (tool);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

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

/**
 * gimp_rectangle_tool_pending_size_set:
 * @width_property:  Option property to set to pending rectangle width.
 * @height_property: Option property to set to pending rectangle height.
 *
 * Sets specified rectangle tool options properties to the width and
 * height of the current pending rectangle.
 */
void
gimp_rectangle_tool_pending_size_set (GimpRectangleTool *rect_tool,
                                      GObject           *object,
                                      const gchar       *width_property,
                                      const gchar       *height_property)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool));
  g_return_if_fail (width_property  != NULL);
  g_return_if_fail (height_property != NULL);

  private = gimp_rectangle_tool_get_private (rect_tool);

  g_object_set (object,
                width_property,  MAX (private->x2 - private->x1, 1.0),
                height_property, MAX (private->y2 - private->y1, 1.0),
                NULL);
}

/**
 * gimp_rectangle_tool_constraint_size_set:
 * @width_property:  Option property to set to current constraint width.
 * @height_property: Option property to set to current constraint height.
 *
 * Sets specified rectangle tool options properties to the width and
 * height of the current contraint size.
 */
void
gimp_rectangle_tool_constraint_size_set (GimpRectangleTool *rect_tool,
                                         GObject           *object,
                                         const gchar       *width_property,
                                         const gchar       *height_property)
{
  GimpTool    *tool;
  GimpContext *context;
  GimpImage   *image;
  gdouble      width;
  gdouble      height;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool));

  tool    = GIMP_TOOL (rect_tool);
  context = gimp_get_user_context (tool->tool_info->gimp);
  image   = gimp_context_get_image (context);

  if (! image)
    {
      width  = 1.0;
      height = 1.0;
    }
  else
    {
      GimpRectangleConstraint constraint;

      constraint = gimp_rectangle_tool_get_constraint (rect_tool);

      switch (constraint)
        {
        case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
          {
            GimpItem *item = GIMP_ITEM (gimp_image_get_active_layer (image));

            if (! item)
              {
                width  = 1.0;
                height = 1.0;
              }
            else
              {
                width  = gimp_item_get_width  (item);
                height = gimp_item_get_height (item);
              }
          }
          break;

        case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
        default:
          {
            width  = gimp_image_get_width  (image);
            height = gimp_image_get_height (image);
          }
          break;
        }
    }

  g_object_set (object,
                width_property,  width,
                height_property, height,
                NULL);
}

/**
 * gimp_rectangle_tool_rectangle_is_new:
 * @rect_tool:
 *
 * Returns: %TRUE if the user is creating a new rectangle from
 * scratch, %FALSE if modifying n previously existing rectangle. This
 * function is only meaningful in _motion and _button_release.
 */
gboolean
gimp_rectangle_tool_rectangle_is_new (GimpRectangleTool *rect_tool)
{
  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool), FALSE);

  return GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool)->is_new;
}

/**
 * gimp_rectangle_tool_point_in_rectangle:
 * @rect_tool:
 * @x:         X-coord of point to test (in image coordinates)
 * @y:         Y-coord of point to test (in image coordinates)
 *
 * Returns: %TRUE if the passed point was within the rectangle
 **/
gboolean
gimp_rectangle_tool_point_in_rectangle (GimpRectangleTool *rect_tool,
                                        gdouble            x,
                                        gdouble            y)
{
  gboolean inside = FALSE;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool), FALSE);

  if (GIMP_TOOL (rect_tool)->display)
    {
      gdouble pub_x1, pub_y1, pub_x2, pub_y2;

      gimp_rectangle_tool_get_public_rect (rect_tool,
                                           &pub_x1, &pub_y1, &pub_x2, &pub_y2);

      inside = x >= pub_x1 && x <= pub_x2 &&
               y >= pub_y1 && y <= pub_y2;
    }

  return inside;
}

/**
 * gimp_rectangle_tool_frame_item:
 * @rect_tool: a #GimpRectangleTool interface
 * @item:      a #GimpItem attached to the image on which a
 *             rectangle is being shown.
 *
 * Convenience function to set the corners of the rectangle to
 * match the bounds of the specified item.  The rectangle interface
 * must be active (i.e., showing a rectangle), and the item must be
 * attached to the image on which the rectangle is active.
 **/
void
gimp_rectangle_tool_frame_item (GimpRectangleTool *rect_tool,
                                GimpItem          *item)
{
  GimpDisplay *display;
  gint         offset_x;
  gint         offset_y;
  gint         width;
  gint         height;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool));
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));

  display = GIMP_TOOL (rect_tool)->display;

  g_return_if_fail (GIMP_IS_DISPLAY (display));
  g_return_if_fail (gimp_display_get_image (display) ==
                    gimp_item_get_image (item));

  width  = gimp_item_get_width  (item);
  height = gimp_item_get_height (item);

  gimp_item_get_offset (item, &offset_x, &offset_y);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_tool));

  gimp_rectangle_tool_set_function (rect_tool,
                                    GIMP_RECTANGLE_TOOL_CREATING);

  g_object_set (rect_tool,
                "x1", offset_x,
                "y1", offset_y,
                "x2", offset_x + width,
                "y2", offset_y + height,
                NULL);

  /* kludge to force handle sizes to update.  This call may be
   * harmful if this function is ever moved out of the text tool code.
   */
  gimp_rectangle_tool_set_constraint (rect_tool,
                                      GIMP_RECTANGLE_CONSTRAIN_NONE);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_tool));
}

void
gimp_rectangle_tool_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool        *rect_tool = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

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
      gimp_rectangle_tool_set_constraint (rect_tool, g_value_get_enum (value));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_NARROW_MODE:
      private->narrow_mode = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  gimp_rectangle_tool_update_int_rect (rect_tool);
}

void
gimp_rectangle_tool_get_property (GObject      *object,
                                  guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  GimpRectangleTool        *rect_tool = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (property_id)
    {
    case GIMP_RECTANGLE_TOOL_PROP_X1:
      g_value_set_int (value, private->x1_int);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y1:
      g_value_set_int (value, private->y1_int);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_X2:
      g_value_set_int (value, private->x1_int + private->width_int);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_Y2:
      g_value_set_int (value, private->y1_int + private->height_int);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_CONSTRAINT:
      g_value_set_enum (value, gimp_rectangle_tool_get_constraint (rect_tool));
      break;
    case GIMP_RECTANGLE_TOOL_PROP_PRECISION:
      g_value_set_enum (value, private->precision);
      break;
    case GIMP_RECTANGLE_TOOL_PROP_NARROW_MODE:
      g_value_set_boolean (value, private->narrow_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_rectangle_tool_constructor (GObject *object)
{
  GimpRectangleTool    *rect_tool = GIMP_RECTANGLE_TOOL (object);
  GimpRectangleOptions *options;

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (object);

  g_signal_connect_object (options, "notify",
                           G_CALLBACK (gimp_rectangle_tool_options_notify),
                           rect_tool, 0);
}

void
gimp_rectangle_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpRectangleTool *rect_tool = GIMP_RECTANGLE_TOOL (tool);

  GIMP_LOG (RECTANGLE_TOOL, "action = %s",
            gimp_enum_get_value_name (GIMP_TYPE_TOOL_ACTION, action));

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
      break;

    case GIMP_TOOL_ACTION_RESUME:
      /* When highlightning is on, the shell gets paused/unpaused which means we
       * will get here, but we only want to recalculate handle sizes when the
       * zoom has changed.
       */
      if (gimp_rectangle_tool_scale_has_changed (rect_tool))
        gimp_rectangle_tool_update_handle_sizes (rect_tool);

      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_rectangle_tool_halt (rect_tool);
      break;

    case GIMP_TOOL_ACTION_COMMIT:
      if (gimp_rectangle_tool_execute (rect_tool))
        gimp_rectangle_tool_halt (rect_tool);
      break;
    }
}

void
gimp_rectangle_tool_button_press (GimpTool         *tool,
                                  const GimpCoords *coords,
                                  guint32           time,
                                  GdkModifierType   state,
                                  GimpDisplay      *display)
{
  GimpRectangleTool        *rect_tool;
  GimpDrawTool             *draw_tool;
  GimpRectangleToolPrivate *private;
  gdouble                   snapped_x, snapped_y;
  gint                      snap_x, snap_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rect_tool = GIMP_RECTANGLE_TOOL (tool);
  draw_tool = GIMP_DRAW_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  gimp_draw_tool_pause (draw_tool);

  GIMP_LOG (RECTANGLE_TOOL, "coords->x = %f, coords->y = %f",
            coords->x, coords->y);

  if (display != tool->display)
    {
      if (tool->display)
        gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);

      gimp_rectangle_tool_set_function (rect_tool,
                                        GIMP_RECTANGLE_TOOL_CREATING);

      private->x1 = private->x2 = coords->x;
      private->y1 = private->y2 = coords->y;

      gimp_rectangle_tool_start (rect_tool, display);
    }

  /* save existing shape in case of cancellation */
  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  gimp_rectangle_tool_setup_snap_offsets (rect_tool,
                                          coords);

  gimp_tool_control_get_snap_offsets (tool->control,
                                      &snap_x, &snap_y, NULL, NULL);

  snapped_x = coords->x + snap_x;
  snapped_y = coords->y + snap_y;

  private->lastx = snapped_x;
  private->lasty = snapped_y;

  if (private->function == GIMP_RECTANGLE_TOOL_CREATING)
    {
      /* Remember that this rectangle was created from scratch. */
      private->is_new = TRUE;

      private->x1 = private->x2 = snapped_x;
      private->y1 = private->y2 = snapped_y;

      gimp_rectangle_tool_update_handle_sizes (rect_tool);

      /* Created rectangles should not be started in narrow-mode */
      private->narrow_mode = FALSE;

      /* If the rectangle is being modified we want the center on
       * fixed_center to be at the center of the currently existing
       * rectangle, otherwise we want the point where the user clicked
       * to be the center on fixed_center.
       */
      private->center_x_on_fixed_center = snapped_x;
      private->center_y_on_fixed_center = snapped_y;

      /* When the user toggles modifier keys, we want to keep track of
       * what coordinates the "other side" should have. If we are
       * creating a rectangle, use the current mouse coordinates as
       * the coordinate of the "other side", otherwise use the
       * immidiate "other side" for that.
       */
      private->other_side_x = snapped_x;
      private->other_side_y = snapped_y;

    }
  else
    {
      /* This rectangle was not created from scratch. */
      private->is_new = FALSE;

      private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
      private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

      gimp_rectangle_tool_get_other_side_coord (rect_tool,
                                                &private->other_side_x,
                                                &private->other_side_y);
    }

  gimp_rectangle_tool_update_int_rect (rect_tool);

  /* Is the rectangle being rubber-banded? */
  private->rect_adjusting = gimp_rectangle_tool_rect_adjusting_func (rect_tool);

  gimp_rectangle_tool_update_highlight (rect_tool);

  gimp_draw_tool_resume (draw_tool);
}

void
gimp_rectangle_tool_button_release (GimpTool              *tool,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpRectangleTool        *rect_tool;
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rect_tool = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  GIMP_LOG (RECTANGLE_TOOL, "coords->x = %f, coords->y = %f",
            coords->x, coords->y);

  if (private->function == GIMP_RECTANGLE_TOOL_EXECUTING)
    gimp_tool_pop_status (tool, display);

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_NORMAL:
      gimp_rectangle_tool_rectangle_change_complete (rect_tool);
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
      private->x1 = private->saved_x1;
      private->y1 = private->saved_y1;
      private->x2 = private->saved_x2;
      private->y2 = private->saved_y2;
      gimp_rectangle_tool_update_int_rect (rect_tool);

      /* If the first created rectangle was canceled, halt the tool */
      if (gimp_rectangle_tool_rectangle_is_new (rect_tool))
        gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

      break;

    case GIMP_BUTTON_RELEASE_CLICK:

      /* When a dead area is clicked, don't execute. */
      if (private->function == GIMP_RECTANGLE_TOOL_DEAD)
        break;

      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      break;

    case GIMP_BUTTON_RELEASE_NO_MOTION:
      break;
    }

  /* We must update this. */
  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

  gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);

  /* On button release, we are not rubber-banding the rectangle any longer. */
  private->rect_adjusting = FALSE;

  gimp_rectangle_tool_update_highlight (rect_tool);
  gimp_rectangle_tool_update_handle_sizes (rect_tool);
  gimp_rectangle_tool_update_options (rect_tool, display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

void
gimp_rectangle_tool_motion (GimpTool         *tool,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            GimpDisplay      *display)
{
  GimpRectangleTool        *rect_tool;
  GimpRectangleToolPrivate *private;
  GimpRectangleOptions     *options;
  gdouble                   snapped_x;
  gdouble                   snapped_y;
  gint                      snap_x, snap_y;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  rect_tool = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);

  /* Motion events should be ignored when we're just waiting for the
   * button release event to execute or if the user has grabbed a dead
   * area of the rectangle.
   */
  if (private->function == GIMP_RECTANGLE_TOOL_EXECUTING ||
      private->function == GIMP_RECTANGLE_TOOL_DEAD)
    return;

  GIMP_LOG (RECTANGLE_TOOL, "coords->x = %f, coords->y = %f",
            coords->x, coords->y);

  /* Handle snapping. */
  gimp_tool_control_get_snap_offsets (tool->control,
                                      &snap_x, &snap_y, NULL, NULL);
  snapped_x = coords->x + snap_x;
  snapped_y = coords->y + snap_y;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));


  /* This is the core rectangle shape updating function: */
  gimp_rectangle_tool_update_with_coord (rect_tool,
                                         snapped_x,
                                         snapped_y);

  /* Update the highlight, but only if it is not being adjusted. If it
   * is not being adjusted, the highlight is not shown anyway.
   */
  if (gimp_rectangle_tool_rect_adjusting_func (rect_tool))
    gimp_rectangle_tool_update_highlight (rect_tool);

  if (private->function != GIMP_RECTANGLE_TOOL_MOVING &&
      private->function != GIMP_RECTANGLE_TOOL_EXECUTING)
    {
      gdouble pub_x1, pub_y1, pub_x2, pub_y2;
      gint    w, h;

      gimp_tool_pop_status (tool, display);

      gimp_rectangle_tool_get_public_rect (rect_tool,
                                           &pub_x1, &pub_y1, &pub_x2, &pub_y2);
      w = pub_x2 - pub_x1;
      h = pub_y2 - pub_y1;

      if (w > 0.0 && h > 0.0)
        {
          gchar *aspect_text;

          aspect_text = g_strdup_printf ("  (%.2f:1)", w / (gdouble) h);

          gimp_tool_push_status_coords (tool, display,
                                        gimp_tool_control_get_precision (tool->control),
                                        _("Rectangle: "),
                                        w, " × ", h, aspect_text);
          g_free (aspect_text);
        }
    }

  if (private->function == GIMP_RECTANGLE_TOOL_CREATING)
    {
      GimpRectangleFunction function = GIMP_RECTANGLE_TOOL_CREATING;
      gdouble               dx       = snapped_x - private->lastx;
      gdouble               dy       = snapped_y - private->lasty;

      /* When the user starts to move the cursor, set the current
       * function to one of the corner-grabbed functions, depending on
       * in what direction the user starts dragging the rectangle.
       */
      if (dx < 0)
        {
          function = (dy < 0 ?
                      GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT :
                      GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT);
        }
      else if (dx > 0)
        {
          function = (dy < 0 ?
                      GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT :
                      GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT);
        }
      else if (dy < 0)
        {
          function = (dx < 0 ?
                      GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT :
                      GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT);
        }
      else if (dy > 0)
        {
          function = (dx < 0 ?
                      GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT :
                      GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT);
        }

      gimp_rectangle_tool_set_function (rect_tool, function);

      if (gimp_rectangle_options_fixed_rule_active (options,
                                                    GIMP_RECTANGLE_FIXED_SIZE))
        {
          /* For fixed size, set the function to moving immediately since the
           * rectangle can not be resized anyway.
           */

          /* We fake a coord update to get the right size. */
          gimp_rectangle_tool_update_with_coord (rect_tool,
                                                 snapped_x,
                                                 snapped_y);

          gimp_tool_control_set_snap_offsets (tool->control,
                                              -(private->x2 - private->x1) / 2,
                                              -(private->y2 - private->y1) / 2,
                                              private->x2 - private->x1,
                                              private->y2 - private->y1);

          gimp_rectangle_tool_set_function (rect_tool,
                                            GIMP_RECTANGLE_TOOL_MOVING);
        }
    }

  gimp_rectangle_tool_update_options (rect_tool, display);

  private->lastx = snapped_x;
  private->lasty = snapped_y;

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
  GimpRectangleTool           *rect_tool;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleToolPrivate    *private;
  gboolean                     button1_down;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  draw_tool       = GIMP_DRAW_TOOL (tool);
  rect_tool       = GIMP_RECTANGLE_TOOL (tool);
  private         = gimp_rectangle_tool_get_private (rect_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);
  button1_down    = state & GDK_BUTTON1_MASK;

  gimp_draw_tool_pause (draw_tool);

  if (key == gimp_get_extend_selection_mask ())
    {
      /* Here we want to handle manualy when to update the rectangle, so we
       * don't want gimp_rectangle_tool_options_notify to do anything.
       */
      g_signal_handlers_block_by_func (options,
                                       gimp_rectangle_tool_options_notify,
                                       rect_tool);

      g_object_set (options,
                    "fixed-rule-active", ! options_private->fixed_rule_active,
                    NULL);

      g_signal_handlers_unblock_by_func (options,
                                         gimp_rectangle_tool_options_notify,
                                         rect_tool);

      /* Only change the shape if the mouse is still down (i.e. the user is
       * still editing the rectangle.
       */
      if (button1_down)
        {
          if (!options_private->fixed_rule_active)
            {
              /* Reset anchor point */
              gimp_rectangle_tool_set_other_side_coord (rect_tool,
                                                        private->other_side_x,
                                                        private->other_side_y);
            }

          gimp_rectangle_tool_update_with_coord (rect_tool,
                                                 private->lastx,
                                                 private->lasty);

          gimp_rectangle_tool_update_highlight (rect_tool);
        }
    }

  if (key == gimp_get_toggle_behavior_mask ())
    {
      g_object_set (options,
                    "fixed-center", ! options_private->fixed_center,
                    NULL);

      if (options_private->fixed_center)
        {
          gimp_rectangle_tool_update_with_coord (rect_tool,
                                                 private->lastx,
                                                 private->lasty);

          gimp_rectangle_tool_update_highlight (rect_tool);

          /* Only emit the rectangle-changed signal if the button is
           * not down. If it is down, the signal will and shall be
           * emitted on _button_release instead.
           */
          if (! button1_down)
            {
              gimp_rectangle_tool_rectangle_change_complete (rect_tool);
            }
        }
      else if (button1_down)
        {
          /* If we are leaving fixed_center mode we want to set the
           * "other side" where it should be. Don't do anything if we
           * came here by a mouse-click though, since then the user
           * has confirmed the shape and we don't want to modify it
           * afterwards.
           */
          gimp_rectangle_tool_set_other_side_coord (rect_tool,
                                                    private->other_side_x,
                                                    private->other_side_y);

          gimp_rectangle_tool_update_highlight (rect_tool);
        }
    }

  gimp_draw_tool_resume (draw_tool);

  gimp_rectangle_tool_update_options (rect_tool, tool->display);
}

static void
swap_doubles (gdouble *i,
              gdouble *j)
{
  gdouble tmp;

  tmp = *i;
  *i = *j;
  *j = tmp;
}

/* gimp_rectangle_tool_check_function() is needed to deal with
 * situations where the user drags a corner or edge across one of the
 * existing edges, thereby changing its function.  Ugh.
 */
static void
gimp_rectangle_tool_check_function (GimpRectangleTool *rect_tool)

{
  GimpRectangleToolPrivate *private;
  GimpRectangleFunction     function;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  function = private->function;

  if (private->x2 < private->x1)
    {
      swap_doubles (&private->x1, &private->x2);
      switch (function)
        {
          case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_RIGHT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_LEFT;
            break;
          /* avoid annoying warnings about unhandled enums */
          default:
            break;
        }
    }

  if (private->y2 < private->y1)
    {
      swap_doubles (&private->y1, &private->y2);
      switch (function)
        {
          case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
           function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
            function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
            function = GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM;
            break;
          case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
            function = GIMP_RECTANGLE_TOOL_RESIZING_TOP;
            break;
          default:
            break;
        }
    }

  gimp_rectangle_tool_set_function (rect_tool, function);
}

gboolean
gimp_rectangle_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpRectangleTool        *rect_tool;
  GimpRectangleToolPrivate *private;
  gint                      dx = 0;
  gint                      dy = 0;
  gdouble                   new_x = 0;
  gdouble                   new_y = 0;

  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (tool), FALSE);

  if (display != tool->display)
    return FALSE;

  rect_tool = GIMP_RECTANGLE_TOOL (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
      dy = -1;
      break;
    case GDK_KEY_Left:
      dx = -1;
      break;
    case GDK_KEY_Right:
      dx = 1;
      break;
    case GDK_KEY_Down:
      dy = 1;
      break;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_COMMIT, display);
      return TRUE;

    case GDK_KEY_Escape:
      gimp_rectangle_tool_cancel (rect_tool);
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      return FALSE;
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & gimp_get_extend_selection_mask ())
    {
      dx *= ARROW_VELOCITY;
      dy *= ARROW_VELOCITY;
    }

  gimp_tool_control_set_snap_offsets (GIMP_TOOL (rect_tool)->control,
                                      0, 0, 0, 0);

  /*  Resize the rectangle if the mouse is over a handle, otherwise move it  */
  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_MOVING:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      new_x = private->x1 + dx;
      private->lastx = new_x;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      new_x = private->x2 + dx;
      private->lastx = new_x;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      new_y = private->y1 + dy;
      private->lasty = new_y;
      break;
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      new_y = private->y2 + dy;
      private->lasty = new_y;
      break;

    default:
      return TRUE;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  gimp_rectangle_tool_update_with_coord (rect_tool,
                                         new_x,
                                         new_y);

  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

  gimp_rectangle_tool_update_highlight (rect_tool);
  gimp_rectangle_tool_update_handle_sizes (rect_tool);

  gimp_rectangle_tool_update_options (rect_tool, tool->display);

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  gimp_rectangle_tool_rectangle_change_complete (rect_tool);

  /*  Evil hack to suppress oper updates. We do this because we don't
   *  want the rectangle tool to change function while the rectangle
   *  is being resized or moved using the keyboard.
   */
  private->suppress_updates = 2;

  return TRUE;
}

void
gimp_rectangle_tool_oper_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpRectangleToolPrivate *private;
  GimpRectangleTool        *rect_tool;
  gint                      function;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  rect_tool = GIMP_RECTANGLE_TOOL (tool);

  if (tool->display != display)
    return;

  if (private->suppress_updates)
    {
      private->suppress_updates--;
      return;
    }

  if (! proximity)
    {
      function = GIMP_RECTANGLE_TOOL_DEAD;
    }
  else if (gimp_rectangle_tool_coord_outside (rect_tool, coords))
    {
      /* The cursor is outside of the rectangle, clicking should
       * create a new rectangle.
       */
      function = GIMP_RECTANGLE_TOOL_CREATING;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_NORTH_WEST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH_EAST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT;
    }
  else if  (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                 coords,
                                                 GIMP_HANDLE_ANCHOR_NORTH_EAST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH_WEST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_WEST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_LEFT;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_EAST))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_RIGHT;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_NORTH))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_TOP;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH))
    {
      function = GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM;
    }
  else if (gimp_rectangle_tool_coord_on_handle (rect_tool,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_CENTER))
    {
      function = GIMP_RECTANGLE_TOOL_MOVING;
    }
  else
    {
      function = GIMP_RECTANGLE_TOOL_DEAD;
    }

  gimp_rectangle_tool_set_function (GIMP_RECTANGLE_TOOL (tool), function);
}

void
gimp_rectangle_tool_cursor_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpRectangleToolPrivate *private;
  GimpCursorType            cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  GimpCursorModifier        modifier = GIMP_CURSOR_MODIFIER_NONE;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (tool->display == display)
    {
      switch (private->function)
        {
        case GIMP_RECTANGLE_TOOL_CREATING:
          cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
          break;
        case GIMP_RECTANGLE_TOOL_MOVING:
          cursor   = GIMP_CURSOR_MOVE;
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
          cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
          cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
          cursor = GIMP_CURSOR_SIDE_LEFT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
          cursor = GIMP_CURSOR_SIDE_RIGHT;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
          cursor = GIMP_CURSOR_SIDE_TOP;
          break;
        case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
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
gimp_rectangle_tool_draw (GimpDrawTool    *draw_tool,
                          GimpCanvasGroup *stroke_group)
{
  GimpTool                    *tool;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gdouble                      x1, y1, x2, y2;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (draw_tool));
  g_return_if_fail (stroke_group == NULL || GIMP_IS_CANVAS_GROUP (stroke_group));

  tool            = GIMP_TOOL (draw_tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  gimp_rectangle_tool_get_public_rect (GIMP_RECTANGLE_TOOL (draw_tool),
                                       &x1, &y1, &x2, &y2);

  if (private->function == GIMP_RECTANGLE_TOOL_INACTIVE)
    return;

  if (! stroke_group)
    stroke_group = GIMP_CANVAS_GROUP (gimp_draw_tool_add_stroke_group (draw_tool));

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  gimp_draw_tool_add_rectangle_guides (draw_tool,
                                       options_private->guide,
                                       x1, y1,
                                       x2 - x1,
                                       y2 - y1);

  gimp_draw_tool_add_rectangle (draw_tool, FALSE,
                                x1, y1,
                                x2 - x1,
                                y2 - y1);

  gimp_draw_tool_pop_group (draw_tool);

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_MOVING:

      if (gimp_tool_control_is_active (tool->control))
        {
          /* Mark the center because we snap to it */
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_CROSS,
                                     (x1 + x2) / 2.0,
                                     (y1 + y2) / 2.0,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_TOOL_HANDLE_SIZE_SMALL,
                                     GIMP_HANDLE_ANCHOR_CENTER);
          break;
        }
      else
        {
          /* Fallthrough */
        }

    case GIMP_RECTANGLE_TOOL_DEAD:
    case GIMP_RECTANGLE_TOOL_CREATING:
    case GIMP_RECTANGLE_TOOL_AUTO_SHRINK:
      gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_corner (draw_tool, FALSE, private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->corner_handle_h,
                                 GIMP_HANDLE_ANCHOR_NORTH_WEST);
      gimp_draw_tool_add_corner (draw_tool, FALSE, private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->corner_handle_h,
                                 GIMP_HANDLE_ANCHOR_NORTH_EAST);
      gimp_draw_tool_add_corner (draw_tool, FALSE, private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->corner_handle_h,
                                 GIMP_HANDLE_ANCHOR_SOUTH_WEST);
      gimp_draw_tool_add_corner (draw_tool, FALSE, private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->corner_handle_h,
                                 GIMP_HANDLE_ANCHOR_SOUTH_EAST);

      gimp_draw_tool_pop_group (draw_tool);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_corner (draw_tool,
                                 ! gimp_tool_control_is_active (tool->control),
                                 private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->top_and_bottom_handle_w,
                                 private->corner_handle_h,
                                 gimp_rectangle_tool_get_anchor (private));

      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_pop_group (draw_tool);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_corner (draw_tool,
                                 ! gimp_tool_control_is_active (tool->control),
                                 private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->left_and_right_handle_h,
                                 gimp_rectangle_tool_get_anchor (private));

      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_pop_group (draw_tool);
      break;

    default:
      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_push_group (draw_tool, stroke_group);

      gimp_draw_tool_add_corner (draw_tool,
                                 ! gimp_tool_control_is_active (tool->control),
                                 private->narrow_mode,
                                 x1, y1,
                                 x2, y2,
                                 private->corner_handle_w,
                                 private->corner_handle_h,
                                 gimp_rectangle_tool_get_anchor (private));

      if (gimp_tool_control_is_active (tool->control))
        gimp_draw_tool_pop_group (draw_tool);
      break;
    }
}

static void
gimp_rectangle_tool_update_handle_sizes (GimpRectangleTool *rect_tool)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;
  GimpDisplayShell         *shell;
  gint                      visible_rectangle_width;
  gint                      visible_rectangle_height;
  gint                      rectangle_width;
  gint                      rectangle_height;
  gdouble                   pub_x1, pub_y1;
  gdouble                   pub_x2, pub_y2;

  tool    = GIMP_TOOL (rect_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  if (! (tool && tool->display))
    return;

  shell   = gimp_display_get_shell (tool->display);

  gimp_rectangle_tool_get_public_rect (rect_tool,
                                       &pub_x1, &pub_y1, &pub_x2, &pub_y2);
  {
    /* Calculate rectangles of the selection rectangle and the display shell,
     * with origin at (0, 0) of image, and in screen coordinate scale.
     */
    gint x1 =  pub_x1 * shell->scale_x;
    gint y1 =  pub_y1 * shell->scale_y;
    gint w1 = (pub_x2 - pub_x1) * shell->scale_x;
    gint h1 = (pub_y2 - pub_y1) * shell->scale_y;

    gint x2, y2, w2, h2;

    gimp_display_shell_scroll_get_scaled_viewport (shell, &x2, &y2, &w2, &h2);

    rectangle_width  = w1;
    rectangle_height = h1;

    /* Handle size calculations shall be based on the visible part of the
     * rectangle, so calculate the size for the visible rectangle by
     * intersecting with the viewport rectangle.
     */
    gimp_rectangle_intersect (x1, y1,
                              w1, h1,
                              x2, y2,
                              w2, h2,
                              NULL, NULL,
                              &visible_rectangle_width,
                              &visible_rectangle_height);

    /* Determine if we are in narrow-mode or not. */
    private->narrow_mode = (visible_rectangle_width  < NARROW_MODE_THRESHOLD ||
                            visible_rectangle_height < NARROW_MODE_THRESHOLD);
  }

  if (private->narrow_mode)
    {
      /* Corner handles always have the same (on-screen) size in
       * narrow-mode.
       */
      private->corner_handle_w = NARROW_MODE_HANDLE_SIZE;
      private->corner_handle_h = NARROW_MODE_HANDLE_SIZE;

      private->top_and_bottom_handle_w = CLAMP (rectangle_width,
                                                MIN (rectangle_width - 2,
                                                     NARROW_MODE_HANDLE_SIZE),
                                                G_MAXINT);
      private->left_and_right_handle_h = CLAMP (rectangle_height,
                                                MIN (rectangle_height - 2,
                                                     NARROW_MODE_HANDLE_SIZE),
                                                G_MAXINT);
    }
  else
    {
      /* Calculate and clamp corner handle size. */

      private->corner_handle_w = visible_rectangle_width  / 4;
      private->corner_handle_h = visible_rectangle_height / 4;

      private->corner_handle_w = CLAMP (private->corner_handle_w,
                                        MIN_HANDLE_SIZE,
                                        MAX_HANDLE_SIZE);
      private->corner_handle_h = CLAMP (private->corner_handle_h,
                                        MIN_HANDLE_SIZE,
                                        MAX_HANDLE_SIZE);

      /* Calculate and clamp side handle size. */

      private->top_and_bottom_handle_w = rectangle_width  - 3 * private->corner_handle_w;
      private->left_and_right_handle_h = rectangle_height - 3 * private->corner_handle_h;

      private->top_and_bottom_handle_w = CLAMP (private->top_and_bottom_handle_w,
                                                MIN_HANDLE_SIZE,
                                                G_MAXINT);
      private->left_and_right_handle_h = CLAMP (private->left_and_right_handle_h,
                                                MIN_HANDLE_SIZE,
                                                G_MAXINT);
    }

  /* Keep track of when we need to calculate handle sizes because of a display
   * shell change.
   */
  private->scale_x_used_for_handle_size_calculations = shell->scale_x;
  private->scale_y_used_for_handle_size_calculations = shell->scale_y;
}

/**
 * gimp_rectangle_tool_scale_has_changed:
 * @rect_tool: A #GimpRectangleTool.
 *
 * Returns: %TRUE if the scale that was used to calculate handle sizes
 *          is not the same as the current shell scale.
 */
static gboolean
gimp_rectangle_tool_scale_has_changed (GimpRectangleTool *rect_tool)
{
  GimpTool                 *tool    = GIMP_TOOL (rect_tool);
  GimpRectangleToolPrivate *private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  GimpDisplayShell         *shell;

  if (! tool->display)
    return TRUE;

  shell = gimp_display_get_shell (tool->display);

  return (shell->scale_x != private->scale_x_used_for_handle_size_calculations
          ||
          shell->scale_y != private->scale_y_used_for_handle_size_calculations);
}

static void
gimp_rectangle_tool_start (GimpRectangleTool *rect_tool,
                           GimpDisplay       *display)
{
  GimpTool                    *tool = GIMP_TOOL (rect_tool);
  GimpRectangleOptionsPrivate *options_private;
  GimpImage                   *image;
  gdouble                      xres;
  gdouble                      yres;

  options_private =
    GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (gimp_tool_get_options (tool));

  image = gimp_display_get_image (display);

  tool->display = display;

  g_signal_connect_object (gimp_display_get_shell (tool->display), "scrolled",
                           G_CALLBACK (gimp_rectangle_tool_shell_scrolled),
                           rect_tool, 0);

  gimp_rectangle_tool_update_highlight (rect_tool);
  gimp_rectangle_tool_update_handle_sizes (rect_tool);

  /* initialize the statusbar display */
  gimp_tool_push_status_coords (tool, tool->display,
                                gimp_tool_control_get_precision (tool->control),
                                _("Rectangle: "), 0, " × ", 0, NULL);

  gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), tool->display);

  gimp_image_get_resolution (image, &xres, &yres);

  if (options_private->fixed_width_entry)
    {
      GtkWidget *entry = options_private->fixed_width_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_width (image));
    }

  if (options_private->fixed_height_entry)
    {
      GtkWidget *entry = options_private->fixed_height_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, yres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_height (image));
    }

  if (options_private->x_entry)
    {
      GtkWidget *entry = options_private->x_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_width (image));
    }

  if (options_private->y_entry)
    {
      GtkWidget *entry = options_private->y_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, yres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_height (image));
    }

  if (options_private->width_entry)
    {
      GtkWidget *entry = options_private->width_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, xres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_width (image));
    }

  if (options_private->height_entry)
    {
      GtkWidget *entry = options_private->height_entry;

      gimp_size_entry_set_resolution (GIMP_SIZE_ENTRY (entry), 0, yres, FALSE);
      gimp_size_entry_set_size (GIMP_SIZE_ENTRY (entry), 0,
                                0, gimp_image_get_height (image));
    }

  if (options_private->auto_shrink_button)
    {
      g_signal_connect_swapped (options_private->auto_shrink_button, "clicked",
                                G_CALLBACK (gimp_rectangle_tool_auto_shrink),
                                rect_tool);

      gtk_widget_set_sensitive (options_private->auto_shrink_button, TRUE);
    }
}

static void
gimp_rectangle_tool_halt (GimpRectangleTool *rect_tool)
{
  GimpTool                    *tool = GIMP_TOOL (rect_tool);
  GimpRectangleOptionsPrivate *options_private;

  options_private =
    GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (gimp_tool_get_options (tool));

  if (tool->display)
    {
      GimpDisplayShell *shell = gimp_display_get_shell (tool->display);

      gimp_display_shell_set_highlight (shell, NULL);

      g_signal_handlers_disconnect_by_func (shell,
                                            gimp_rectangle_tool_shell_scrolled,
                                            rect_tool);
    }

  if (gimp_draw_tool_is_active (GIMP_DRAW_TOOL (rect_tool)))
    gimp_draw_tool_stop (GIMP_DRAW_TOOL (rect_tool));

  tool->display  = NULL;
  tool->drawable = NULL;

  gimp_rectangle_tool_set_function (rect_tool, GIMP_RECTANGLE_TOOL_INACTIVE);

  if (options_private->auto_shrink_button)
    {
      gtk_widget_set_sensitive (options_private->auto_shrink_button, FALSE);

      g_signal_handlers_disconnect_by_func (options_private->auto_shrink_button,
                                            gimp_rectangle_tool_auto_shrink,
                                            rect_tool);
    }
}

gboolean
gimp_rectangle_tool_execute (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolInterface *iface;
  gboolean                    retval = FALSE;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rect_tool);

  if (iface->execute)
    {
      gdouble pub_x1, pub_y1;
      gdouble pub_x2, pub_y2;

      gimp_rectangle_tool_get_public_rect (rect_tool,
                                           &pub_x1, &pub_y1, &pub_x2, &pub_y2);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_tool));

      retval = iface->execute (rect_tool,
                               pub_x1,
                               pub_y1,
                               pub_x2 - pub_x1,
                               pub_y2 - pub_y1);

      gimp_rectangle_tool_update_highlight (rect_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_tool));
    }

  return retval;
}

void
gimp_rectangle_tool_cancel (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolInterface *iface;

  iface = GIMP_RECTANGLE_TOOL_GET_INTERFACE (rect_tool);

  if (iface->cancel)
    iface->cancel (rect_tool);
}

static void
gimp_rectangle_tool_update_options (GimpRectangleTool *rect_tool,
                                    GimpDisplay       *display)
{
  GimpRectangleOptions *options;
  gdouble               x1, y1;
  gdouble               x2, y2;
  gdouble               old_x;
  gdouble               old_y;
  gdouble               old_width;
  gdouble               old_height;

  options = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rect_tool);

  gimp_rectangle_tool_get_public_rect (rect_tool, &x1, &y1, &x2, &y2);

  g_signal_handlers_block_by_func (options,
                                   gimp_rectangle_tool_options_notify,
                                   rect_tool);

  g_object_get (options,
                "x",      &old_x,
                "y",      &old_y,
                "width",  &old_width,
                "height", &old_height,
                NULL);

  g_object_freeze_notify (G_OBJECT (options));

  if (! FEQUAL (old_x, x1))
    g_object_set (options, "x", x1, NULL);

  if (! FEQUAL (old_y, y1))
    g_object_set (options, "y", y1, NULL);

  if (! FEQUAL (old_width, x2 - x1))
    g_object_set (options, "width", x2 - x1, NULL);

  if (! FEQUAL (old_height, y2 - y1))
    g_object_set (options, "height", y2 - y1, NULL);

  g_object_thaw_notify (G_OBJECT (options));

  g_signal_handlers_unblock_by_func (options,
                                     gimp_rectangle_tool_options_notify,
                                     rect_tool);
}

static void
gimp_rectangle_tool_synthesize_motion (GimpRectangleTool *rect_tool,
                                       gint               function,
                                       gdouble            new_x,
                                       gdouble            new_y)
{
  GimpTool                 *tool;
  GimpDrawTool             *draw_tool;
  GimpRectangleToolPrivate *private;
  GimpRectangleFunction     old_function;

  tool      = GIMP_TOOL (rect_tool);
  draw_tool = GIMP_DRAW_TOOL (rect_tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  /* We don't want to synthesize motions if the tool control is active
   * since that means the mouse button is down and the rectangle will
   * get updated in _motion anyway. The reason we want to prevent this
   * function from executing is that is emits the
   * rectangle-changed-complete signal which we don't want in the
   * middle of a rectangle change.
   *
   * In addition to that, we don't want to synthesize a motion if
   * there is no pending rectangle because that doesn't make any
   * sense.
   */
  if (gimp_tool_control_is_active (tool->control) ||
      ! tool->display)
    return;

  old_function = private->function;

  gimp_draw_tool_pause (draw_tool);

  gimp_rectangle_tool_set_function (rect_tool, function);

  gimp_rectangle_tool_update_with_coord (rect_tool,
                                         new_x,
                                         new_y);

  /* We must update this. */
  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;

  gimp_rectangle_tool_update_options (rect_tool,
                                      tool->display);

  gimp_rectangle_tool_set_function (rect_tool, old_function);

  gimp_rectangle_tool_update_highlight (rect_tool);
  gimp_rectangle_tool_update_handle_sizes (rect_tool);

  gimp_draw_tool_resume (draw_tool);

  gimp_rectangle_tool_rectangle_change_complete (rect_tool);
}

static void
gimp_rectangle_tool_options_notify (GimpRectangleOptions *options,
                                    GParamSpec           *pspec,
                                    GimpRectangleTool    *rect_tool)
{
  GimpTool                    *tool;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptionsPrivate *options_private;

  tool            = GIMP_TOOL (rect_tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  if (strcmp (pspec->name, "guide") == 0)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_tool));
      gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_tool));
    }
  else if (strcmp  (pspec->name, "x") == 0 &&
           !PIXEL_FEQUAL (private->x1, options_private->x))
    {
      gimp_rectangle_tool_synthesize_motion (rect_tool,
                                             GIMP_RECTANGLE_TOOL_MOVING,
                                             options_private->x,
                                             private->y1);
    }
  else if (strcmp  (pspec->name, "y") == 0 &&
           !PIXEL_FEQUAL (private->y1, options_private->y))
    {
      gimp_rectangle_tool_synthesize_motion (rect_tool,
                                             GIMP_RECTANGLE_TOOL_MOVING,
                                             private->x1,
                                             options_private->y);
    }
  else if (strcmp  (pspec->name, "width") == 0 &&
           !PIXEL_FEQUAL (private->x2 - private->x1, options_private->width))
    {
      /* Calculate x2, y2 that will create a rectangle of given width, for the
       * current options.
       */
      gdouble x2;

      if (options_private->fixed_center)
        {
          x2 = private->center_x_on_fixed_center +
               options_private->width / 2;
        }
      else
        {
          x2 = private->x1 + options_private->width;
        }

      gimp_rectangle_tool_synthesize_motion (rect_tool,
                                             GIMP_RECTANGLE_TOOL_RESIZING_RIGHT,
                                             x2,
                                             private->y2);
    }
  else if (strcmp  (pspec->name, "height") == 0 &&
           !PIXEL_FEQUAL (private->y2 - private->y1, options_private->height))
    {
      /* Calculate x2, y2 that will create a rectangle of given height, for the
       * current options.
       */
      gdouble y2;

      if (options_private->fixed_center)
        {
          y2 = private->center_y_on_fixed_center +
               options_private->height / 2;
        }
      else
        {
          y2 = private->y1 + options_private->height;
        }

      gimp_rectangle_tool_synthesize_motion (rect_tool,
                                             GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM,
                                             private->x2,
                                             y2);
    }
  else if (strcmp (pspec->name, "desired-fixed-size-width") == 0)
    {
      /* We are only interested in when width and height swaps, so
       * it's enough to only check e.g. for width.
       */

      gdouble width  = private->x2 - private->x1;
      gdouble height = private->y2 - private->y1;

      /* Depending on a bunch of conditions, we might want to
       * immedieately switch width and height of the pending
       * rectangle.
       */
      if (options_private->fixed_rule_active                          &&
          tool->display                                       != NULL &&
          tool->button_press_state                            == 0    &&
          tool->active_modifier_state                         == 0    &&
          FEQUAL (options_private->desired_fixed_size_width,  height) &&
          FEQUAL (options_private->desired_fixed_size_height, width))
        {
          gdouble x = private->x1;
          gdouble y = private->y1;

          gimp_rectangle_tool_synthesize_motion (rect_tool,
                                                 GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT,
                                                 private->x2,
                                                 private->y2);

          /* For some reason these needs to be set separately... */
          g_object_set (options,
                        "x", x,
                        NULL);
          g_object_set (options,
                        "y", y,
                        NULL);
        }
    }
  else if (strcmp (pspec->name, "aspect-numerator") == 0)
    {
      /* We are only interested in when numerator and denominator
       * swaps, so it's enough to only check e.g. for numerator.
       */

      double    width             = private->x2 - private->x1;
      double    height            = private->y2 - private->y1;
      gdouble   new_inverse_ratio = options_private->aspect_denominator /
                                    options_private->aspect_numerator;
      gdouble   lower_ratio;
      gdouble   higher_ratio;

      /* The ratio of the Fixed: Aspect ratio rule and the pending
       * rectangle is very rarely exactly the same so use an
       * interval. For small rectangles the below code will
       * automatically yield a more generous accepted ratio interval
       * which is exactly what we want.
       */
      if (width > height && height > 1.0)
        {
          lower_ratio  = width / (height + 1.0);
          higher_ratio = width / (height - 1.0);
        }
      else
        {
          lower_ratio  = (width - 1.0) / height;
          higher_ratio = (width + 1.0) / height;
        }

      /* Depending on a bunch of conditions, we might want to
       * immedieately switch width and height of the pending
       * rectangle.
       */
      if (options_private->fixed_rule_active               &&
          tool->display               != NULL              &&
          tool->button_press_state    == 0                 &&
          tool->active_modifier_state == 0                 &&
          lower_ratio                 <  new_inverse_ratio &&
          higher_ratio                >  new_inverse_ratio)
        {
          gdouble new_x2 = private->x1 + private->y2 - private->y1;
          gdouble new_y2 = private->y1 + private->x2 - private->x1;

          gimp_rectangle_tool_synthesize_motion (rect_tool,
                                                 GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT,
                                                 new_x2,
                                                 new_y2);
        }
    }
  else if (strcmp (pspec->name, "highlight") == 0)
    {
      gimp_rectangle_tool_update_highlight (rect_tool);
    }
}

static void
gimp_rectangle_tool_shell_scrolled (GimpDisplayShell  *shell,
                                    GimpRectangleTool *rect_tool)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (rect_tool);

  gimp_draw_tool_pause (draw_tool);

  gimp_rectangle_tool_update_handle_sizes (rect_tool);

  gimp_draw_tool_resume (draw_tool);
}

GimpRectangleFunction
gimp_rectangle_tool_get_function (GimpRectangleTool *rect_tool)
{
  g_return_val_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool),
                        GIMP_RECTANGLE_TOOL_INACTIVE);

  return GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool)->function;
}

void
gimp_rectangle_tool_set_function (GimpRectangleTool     *rect_tool,
                                  GimpRectangleFunction  function)
{
  GimpRectangleToolPrivate *private;

  g_return_if_fail (GIMP_IS_RECTANGLE_TOOL (rect_tool));

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  /* redraw the tool when the function changes */
  /* FIXME: should also update the cursor      */
  if (private->function != function)
    {
      GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (rect_tool);

      gimp_draw_tool_pause (draw_tool);

      private->function = function;

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_rectangle_tool_rectangle_change_complete (GimpRectangleTool *rect_tool)
{
  g_signal_emit (rect_tool,
                 gimp_rectangle_tool_signals[RECTANGLE_CHANGE_COMPLETE], 0);
}

static void
gimp_rectangle_tool_auto_shrink (GimpRectangleTool *rect_tool)
{
  GimpTool                 *tool    = GIMP_TOOL (rect_tool);
  GimpRectangleToolPrivate *private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  GimpDisplay              *display = tool->display;
  GimpImage                *image;
  GimpPickable             *pickable;
  gint                      offset_x = 0;
  gint                      offset_y = 0;
  gint                      x1, y1;
  gint                      x2, y2;
  gint                      shrunk_x;
  gint                      shrunk_y;
  gint                      shrunk_width;
  gint                      shrunk_height;
  gboolean                  shrink_merged;

  if (! display)
    return;

  image = gimp_display_get_image (display);

  g_object_get (gimp_tool_get_options (tool),
                "shrink-merged", &shrink_merged,
                NULL);

  if (shrink_merged)
    {
      pickable = GIMP_PICKABLE (image);

      x1 = private->x1;
      y1 = private->y1;
      x2 = private->x2;
      y2 = private->y2;
    }
  else
    {
      pickable = GIMP_PICKABLE (gimp_image_get_active_drawable (image));

      if (! pickable)
        return;

      gimp_item_get_offset (GIMP_ITEM (pickable), &offset_x, &offset_y);

      x1 = private->x1 - offset_x;
      y1 = private->y1 - offset_y;
      x2 = private->x2 - offset_x;
      y2 = private->y2 - offset_y;
    }

  switch (gimp_pickable_auto_shrink (pickable,
                                     x1, y1, x2 - x1, y2 - y1,
                                     &shrunk_x,
                                     &shrunk_y,
                                     &shrunk_width,
                                     &shrunk_height))
    {
    case GIMP_AUTO_SHRINK_SHRINK:
      {
        GimpRectangleFunction original_function = private->function;

        gimp_draw_tool_pause (GIMP_DRAW_TOOL (rect_tool));
        private->function = GIMP_RECTANGLE_TOOL_AUTO_SHRINK;

        private->x1 = offset_x + shrunk_x;
        private->y1 = offset_y + shrunk_y;
        private->x2 = offset_x + shrunk_x + shrunk_width;
        private->y2 = offset_y + shrunk_y + shrunk_height;

        gimp_rectangle_tool_update_int_rect (rect_tool);

        gimp_rectangle_tool_rectangle_change_complete (rect_tool);

        gimp_rectangle_tool_update_handle_sizes (rect_tool);
        gimp_rectangle_tool_update_highlight (rect_tool);

        private->function = original_function;
        gimp_draw_tool_resume (GIMP_DRAW_TOOL (rect_tool));
      }
      break;

    default:
      break;
    }

  gimp_rectangle_tool_update_options (rect_tool, tool->display);
}

/**
 * gimp_rectangle_tool_coord_outside:
 *
 * Returns: %TRUE if the coord is outside the rectange bounds
 *          including any outside handles.
 */
static gboolean
gimp_rectangle_tool_coord_outside (GimpRectangleTool *rect_tool,
                                   const GimpCoords  *coord)
{
  GimpRectangleToolPrivate *private;
  GimpDisplayShell         *shell;
  gboolean                  narrow_mode;
  gdouble                   pub_x1, pub_y1, pub_x2, pub_y2;
  gdouble                   x1_b, y1_b, x2_b, y2_b;

  private     = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);
  narrow_mode = private->narrow_mode;
  shell       = gimp_display_get_shell (GIMP_TOOL (rect_tool)->display);

  gimp_rectangle_tool_get_public_rect (rect_tool,
                                       &pub_x1, &pub_y1, &pub_x2, &pub_y2);

  x1_b = pub_x1 - (narrow_mode ? private->corner_handle_w / shell->scale_x : 0);
  x2_b = pub_x2 + (narrow_mode ? private->corner_handle_w / shell->scale_x : 0);
  y1_b = pub_y1 - (narrow_mode ? private->corner_handle_h / shell->scale_y : 0);
  y2_b = pub_y2 + (narrow_mode ? private->corner_handle_h / shell->scale_y : 0);

  return (coord->x < x1_b ||
          coord->x > x2_b ||
          coord->y < y1_b ||
          coord->y > y2_b);
}

/**
 * gimp_rectangle_tool_coord_on_handle:
 *
 * Returns: %TRUE if the coord is on the handle that corresponds to
 *          @anchor.
 */
static gboolean
gimp_rectangle_tool_coord_on_handle (GimpRectangleTool *rect_tool,
                                     const GimpCoords  *coords,
                                     GimpHandleAnchor   anchor)
{
  GimpRectangleToolPrivate *private;
  GimpDisplayShell         *shell;
  GimpDrawTool             *draw_tool;
  GimpTool                 *tool;
  gdouble                   pub_x1, pub_y1, pub_x2, pub_y2;
  gdouble                   rect_w, rect_h;
  gdouble                   handle_x          = 0;
  gdouble                   handle_y          = 0;
  gdouble                   handle_width      = 0;
  gdouble                   handle_height     = 0;
  gint                      narrow_mode_x_dir = 0;
  gint                      narrow_mode_y_dir = 0;

  tool      = GIMP_TOOL (rect_tool);
  draw_tool = GIMP_DRAW_TOOL (tool);
  shell     = gimp_display_get_shell (tool->display);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);

  gimp_rectangle_tool_get_public_rect (rect_tool,
                                       &pub_x1, &pub_y1, &pub_x2, &pub_y2);

  rect_w = pub_x2 - pub_x1;
  rect_h = pub_y2 - pub_y1;

  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      handle_x      = pub_x1;
      handle_y      = pub_y1;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      handle_x      = pub_x2;
      handle_y      = pub_y2;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      handle_x      = pub_x2;
      handle_y      = pub_y1;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      handle_x      = pub_x1;
      handle_y      = pub_y2;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      handle_x      = pub_x1;
      handle_y      = pub_y1 + rect_h / 2;
      handle_width  = private->corner_handle_w;
      handle_height = private->left_and_right_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir =  0;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      handle_x      = pub_x2;
      handle_y      = pub_y1 + rect_h / 2;
      handle_width  = private->corner_handle_w;
      handle_height = private->left_and_right_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir =  0;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      handle_x      = pub_x1 + rect_w / 2;
      handle_y      = pub_y1;
      handle_width  = private->top_and_bottom_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  0;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      handle_x      = pub_x1 + rect_w / 2;
      handle_y      = pub_y2;
      handle_width  = private->top_and_bottom_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  0;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_CENTER:
      handle_x      = pub_x1 + rect_w / 2;
      handle_y      = pub_y1 + rect_h / 2;

      if (private->narrow_mode)
        {
          handle_width  = rect_w * shell->scale_x;
          handle_height = rect_h * shell->scale_y;
        }
      else
        {
          handle_width  = rect_w * shell->scale_x - private->corner_handle_w * 2;
          handle_height = rect_h * shell->scale_y - private->corner_handle_h * 2;
        }

      narrow_mode_x_dir =  0;
      narrow_mode_y_dir =  0;
      break;
    }

  if (private->narrow_mode)
    {
      handle_x += narrow_mode_x_dir * handle_width  / shell->scale_x;
      handle_y += narrow_mode_y_dir * handle_height / shell->scale_y;
    }

  return gimp_draw_tool_on_handle (draw_tool, shell->display,
                                   coords->x, coords->y,
                                   GIMP_HANDLE_SQUARE,
                                   handle_x,     handle_y,
                                   handle_width, handle_height,
                                   anchor);
}

static GimpHandleAnchor
gimp_rectangle_tool_get_anchor (GimpRectangleToolPrivate *private)
{
  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
      return GIMP_HANDLE_ANCHOR_NORTH_WEST;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
      return GIMP_HANDLE_ANCHOR_NORTH_EAST;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
      return GIMP_HANDLE_ANCHOR_SOUTH_WEST;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
      return GIMP_HANDLE_ANCHOR_SOUTH_EAST;

    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      return GIMP_HANDLE_ANCHOR_WEST;

    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      return GIMP_HANDLE_ANCHOR_EAST;

    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      return GIMP_HANDLE_ANCHOR_NORTH;

    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      return GIMP_HANDLE_ANCHOR_SOUTH;

    default:
      return GIMP_HANDLE_ANCHOR_CENTER;
    }
}

static void
gimp_rectangle_tool_update_highlight (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolPrivate *private;
  GimpTool                 *tool;
  GimpRectangleOptions     *options;
  GimpDisplayShell         *shell;
  gboolean                  highlight;

  tool      = GIMP_TOOL (rect_tool);
  options   = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  private   = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);
  highlight = FALSE;

  if (! tool->display)
    return;

  shell = gimp_display_get_shell (tool->display);

  g_object_get (options, "highlight", &highlight, NULL);

  /* Don't show the highlight when the mouse is down. */
  if (! highlight || private->rect_adjusting)
    {
      gimp_display_shell_set_highlight (shell, NULL);
    }
  else
    {
      GdkRectangle rect;
      gdouble      pub_x1, pub_y1;
      gdouble      pub_x2, pub_y2;

      gimp_rectangle_tool_get_public_rect (rect_tool,
                                           &pub_x1, &pub_y1, &pub_x2, &pub_y2);

      rect.x      = pub_x1;
      rect.y      = pub_y1;
      rect.width  = pub_x2 - pub_x1;
      rect.height = pub_y2 - pub_y1;

      gimp_display_shell_set_highlight (shell, &rect);
    }
}

static gboolean
gimp_rectangle_tool_rect_rubber_banding_func (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolPrivate *private;
  gboolean                  rect_rubber_banding_func;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (private->function)
    {
      case GIMP_RECTANGLE_TOOL_CREATING:
      case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
      case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
      case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
      case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
      case GIMP_RECTANGLE_TOOL_AUTO_SHRINK:
        rect_rubber_banding_func = TRUE;
        break;

      case GIMP_RECTANGLE_TOOL_MOVING:
      case GIMP_RECTANGLE_TOOL_INACTIVE:
      case GIMP_RECTANGLE_TOOL_DEAD:
      default:
        rect_rubber_banding_func = FALSE;
        break;
    }

  return rect_rubber_banding_func;
}

/**
 * gimp_rectangle_tool_rect_adjusting_func:
 * @rect_tool:
 *
 * Returns: %TRUE if the current function is a rectangle adjusting
 *          function.
 */
static gboolean
gimp_rectangle_tool_rect_adjusting_func (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  return (gimp_rectangle_tool_rect_rubber_banding_func (rect_tool) ||
          private->function == GIMP_RECTANGLE_TOOL_MOVING);
}

/**
 * gimp_rectangle_tool_get_other_side:
 * @rect_tool: A #GimpRectangleTool.
 * @other_x:   Pointer to double of the other-x double.
 * @other_y:   Pointer to double of the other-y double.
 *
 * Calculates pointers to member variables that hold the coordinates
 * of the opposite side (either the opposite corner or literally the
 * opposite side), based on the current function. The opposite of a
 * corner needs two coordinates, the opposite of a side only needs
 * one.
 */
static void
gimp_rectangle_tool_get_other_side (GimpRectangleTool  *rect_tool,
                                    gdouble           **other_x,
                                    gdouble           **other_y)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      *other_x = &private->x1;
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      *other_x = &private->x2;
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
    default:
      *other_x = NULL;
      break;
    }

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      *other_y = &private->y1;
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      *other_y = &private->y2;
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
    default:
      *other_y = NULL;
      break;
    }
}

static void
gimp_rectangle_tool_get_other_side_coord (GimpRectangleTool *rect_tool,
                                          gdouble           *other_side_x,
                                          gdouble           *other_side_y)
{
  gdouble *other_x = NULL;
  gdouble *other_y = NULL;

  gimp_rectangle_tool_get_other_side (rect_tool,
                                      &other_x,
                                      &other_y);
  if (other_x)
    *other_side_x = *other_x;
  if (other_y)
    *other_side_y = *other_y;
}

static void
gimp_rectangle_tool_set_other_side_coord (GimpRectangleTool *rect_tool,
                                          gdouble            other_side_x,
                                          gdouble            other_side_y)
{
  gdouble *other_x = NULL;
  gdouble *other_y = NULL;

  gimp_rectangle_tool_get_other_side (rect_tool,
                                      &other_x,
                                      &other_y);
  if (other_x)
    *other_x = other_side_x;
  if (other_y)
    *other_y = other_side_y;

  gimp_rectangle_tool_check_function (rect_tool);

  gimp_rectangle_tool_update_int_rect (rect_tool);
}

/**
 * gimp_rectangle_tool_apply_coord:
 * @param:     A #GimpRectangleTool.
 * @coord_x:   X of coord.
 * @coord_y:   Y of coord.
 *
 * Adjust the rectangle to the new position specified by passed
 * coordinate, taking fixed_center into account, which means it
 * expands the rectagle around the center point.
 */
static void
gimp_rectangle_tool_apply_coord (GimpRectangleTool *rect_tool,
                                 gdouble            coord_x,
                                 gdouble            coord_y)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rect_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  if (private->function == GIMP_RECTANGLE_TOOL_INACTIVE)
    g_warning ("function is GIMP_RECTANGLE_TOOL_INACTIVE while mouse is moving");

  if (private->function == GIMP_RECTANGLE_TOOL_MOVING)
    {
      /* Preserve width and height while moving the grab-point to where the
       * cursor is.
       */
      gdouble w = private->x2 - private->x1;
      gdouble h = private->y2 - private->y1;

      private->x1 = coord_x;
      private->y1 = coord_y;

      private->x2 = private->x1 + w;
      private->y2 = private->y1 + h;

      /* We are done already. */
      return;
    }

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      private->x1 = coord_x;

      if (options_private->fixed_center)
        private->x2 = 2 * private->center_x_on_fixed_center - private->x1;

      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      private->x2 = coord_x;

      if (options_private->fixed_center)
        private->x1 = 2 * private->center_x_on_fixed_center - private->x2;

      break;
    }

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      private->y1 = coord_y;

      if (options_private->fixed_center)
        private->y2 = 2 * private->center_y_on_fixed_center - private->y1;

      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      private->y2 = coord_y;

      if (options_private->fixed_center)
        private->y1 = 2 * private->center_y_on_fixed_center - private->y2;

      break;
    }
}

static void
gimp_rectangle_tool_setup_snap_offsets (GimpRectangleTool *rect_tool,
                                        const GimpCoords  *coords)
{
  GimpTool                 *tool;
  GimpRectangleToolPrivate *private;
  gdouble                   pub_x1, pub_y1, pub_x2, pub_y2;
  gdouble                   pub_coord_x, pub_coord_y;

  tool    = GIMP_TOOL (rect_tool);
  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  gimp_rectangle_tool_get_public_rect (rect_tool,
                                       &pub_x1, &pub_y1, &pub_x2, &pub_y2);
  gimp_rectangle_tool_adjust_coord (rect_tool,
                                    coords->x, coords->y,
                                    &pub_coord_x, &pub_coord_y);

  switch (private->function)
    {
      gimp_tool_control_set_snap_offsets (tool->control, 0, 0, 0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x1 - pub_coord_x,
                                          pub_y1 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x2 - pub_coord_x,
                                          pub_y1 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x1 - pub_coord_x,
                                          pub_y2 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x2 - pub_coord_x,
                                          pub_y2 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x1 - pub_coord_x, 0,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x2 - pub_coord_x, 0,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, pub_y1 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          0, pub_y2 - pub_coord_y,
                                          0, 0);
      break;

    case GIMP_RECTANGLE_TOOL_MOVING:
      gimp_tool_control_set_snap_offsets (tool->control,
                                          pub_x1 - pub_coord_x,
                                          pub_y1 - pub_coord_y,
                                          pub_x2 - pub_x1,
                                          pub_y2 - pub_y1);
      break;

    default:
      break;
    }
}

/**
 * gimp_rectangle_tool_clamp_width:
 * @rect_tool:      A #GimpRectangleTool.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps rectangle inside specified bounds, providing information of
 * where clamping was done. Can also clamp symmetrically.
 */
static void
gimp_rectangle_tool_clamp (GimpRectangleTool       *rect_tool,
                           ClampedSide             *clamped_sides,
                           GimpRectangleConstraint  constraint,
                           gboolean                 symmetrically)
{
  gimp_rectangle_tool_clamp_width (rect_tool,
                                   clamped_sides,
                                   constraint,
                                   symmetrically);

  gimp_rectangle_tool_clamp_height (rect_tool,
                                    clamped_sides,
                                    constraint,
                                    symmetrically);
}

/**
 * gimp_rectangle_tool_clamp_width:
 * @rect_tool:      A #GimpRectangleTool.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps height of rectangle. Set symmetrically to true when using
 * for fixed_center:ed rectangles, since that will clamp symmetrically
 * which is just what is needed.
 *
 * When this function constrains, it puts what it constrains in
 * @constraint. This information is essential when an aspect ratio is
 * to be applied.
 */
static void
gimp_rectangle_tool_clamp_width (GimpRectangleTool       *rect_tool,
                                 ClampedSide             *clamped_sides,
                                 GimpRectangleConstraint  constraint,
                                 gboolean                 symmetrically)
{
  GimpRectangleToolPrivate *private;
  gint                      min_x;
  gint                      max_x;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  gimp_rectangle_tool_get_constraints (rect_tool,
                                       &min_x,
                                       NULL,
                                       &max_x,
                                       NULL,
                                       constraint);
  if (private->x1 < min_x)
    {
      gdouble dx = min_x - private->x1;

      private->x1 += dx;

      if (symmetrically)
        private->x2 -= dx;

      if (private->x2 < min_x)
        private->x2 = min_x;

      if (clamped_sides)
        *clamped_sides |= CLAMPED_LEFT;
    }

  if (private->x2 > max_x)
    {
      gdouble dx = max_x - private->x2;

      private->x2 += dx;

      if (symmetrically)
        private->x1 -= dx;

      if (private->x1 > max_x)
        private->x1 = max_x;

      if (clamped_sides)
        *clamped_sides |= CLAMPED_RIGHT;
    }
}

/**
 * gimp_rectangle_tool_clamp_height:
 * @rect_tool:      A #GimpRectangleTool.
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
gimp_rectangle_tool_clamp_height (GimpRectangleTool       *rect_tool,
                                  ClampedSide             *clamped_sides,
                                  GimpRectangleConstraint  constraint,
                                  gboolean                 symmetrically)
{
  GimpRectangleToolPrivate *private;
  gint                      min_y;
  gint                      max_y;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  gimp_rectangle_tool_get_constraints (rect_tool,
                                       NULL,
                                       &min_y,
                                       NULL,
                                       &max_y,
                                       constraint);
  if (private->y1 < min_y)
    {
      gdouble dy = min_y - private->y1;

      private->y1 += dy;

      if (symmetrically)
        private->y2 -= dy;

      if (private->y2 < min_y)
        private->y2 = min_y;

      if (clamped_sides)
        *clamped_sides |= CLAMPED_TOP;
    }

  if (private->y2 > max_y)
    {
      gdouble dy = max_y - private->y2;

      private->y2 += dy;

      if (symmetrically)
        private->y1 -= dy;

      if (private->y1 > max_y)
        private->y1 = max_y;

      if (clamped_sides)
        *clamped_sides |= CLAMPED_BOTTOM;
    }
}

/**
 * gimp_rectangle_tool_keep_inside:
 * @rect_tool: A #GimpRectangleTool.
 *
 * If the rectangle is outside of the canvas, move it into it. If the rectangle is
 * larger than the canvas in any direction, make it fill the canvas in that direction.
 */
static void
gimp_rectangle_tool_keep_inside (GimpRectangleTool      *rect_tool,
                                 GimpRectangleConstraint constraint)
{
  gimp_rectangle_tool_keep_inside_horizontally (rect_tool,
                                                constraint);

  gimp_rectangle_tool_keep_inside_vertically (rect_tool,
                                              constraint);
}

/**
 * gimp_rectangle_tool_keep_inside_horizontally:
 * @rect_tool:      A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint horizontally, move it
 * inside. If it is too big to fit inside, make it just as big as the width
 * limit.
 */
static void
gimp_rectangle_tool_keep_inside_horizontally (GimpRectangleTool       *rect_tool,
                                              GimpRectangleConstraint  constraint)
{
  GimpRectangleToolPrivate *private;
  gint                      min_x;
  gint                      max_x;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_rectangle_tool_get_constraints (rect_tool,
                                       &min_x,
                                       NULL,
                                       &max_x,
                                       NULL,
                                       constraint);

  if (max_x - min_x < private->x2 - private->x1)
    {
      private->x1 = min_x;
      private->x2 = max_x;
    }
  else
    {
      if (private->x1 < min_x)
        {
          gdouble dx = min_x - private->x1;

          private->x1 += dx;
          private->x2 += dx;
        }
      if (private->x2 > max_x)
        {
          gdouble dx = max_x - private->x2;

          private->x1 += dx;
          private->x2 += dx;
        }
    }
}

/**
 * gimp_rectangle_tool_keep_inside_vertically:
 * @rect_tool:      A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint vertically,
 * move it inside. If it is too big to fit inside, make it just as big
 * as the width limit.
 */
static void
gimp_rectangle_tool_keep_inside_vertically (GimpRectangleTool       *rect_tool,
                                            GimpRectangleConstraint  constraint)
{
  GimpRectangleToolPrivate *private;
  gint                      min_y;
  gint                      max_y;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_rectangle_tool_get_constraints (rect_tool,
                                       NULL,
                                       &min_y,
                                       NULL,
                                       &max_y,
                                       constraint);

  if (max_y - min_y < private->y2 - private->y1)
    {
      private->y1 = min_y;
      private->y2 = max_y;
    }
  else
    {
      if (private->y1 < min_y)
        {
          gdouble dy = min_y - private->y1;

          private->y1 += dy;
          private->y2 += dy;
        }
      if (private->y2 > max_y)
        {
          gdouble dy = max_y - private->y2;

          private->y1 += dy;
          private->y2 += dy;
        }
    }
}

/**
 * gimp_rectangle_tool_apply_fixed_width:
 * @rect_tool:      A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 * @width:
 *
 * Makes the rectangle have a fixed_width, following the constrainment
 * rules of fixed widths as well. Please refer to the rectangle tools
 * spec.
 */
static void
gimp_rectangle_tool_apply_fixed_width (GimpRectangleTool      *rect_tool,
                                       GimpRectangleConstraint constraint,
                                       gdouble                 width)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    width / 2;
      private->x2 = private->x1 + width;

      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:

      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    width / 2;
      private->x2 = private->x1 + width;

      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_rectangle_tool_keep_inside_horizontally (rect_tool,
                                                constraint);
}

/**
 * gimp_rectangle_tool_apply_fixed_height:
 * @rect_tool:      A #GimpRectangleTool.
 * @constraint:     Constraint to use.
 * @height:
 *
 * Makes the rectangle have a fixed_height, following the
 * constrainment rules of fixed heights as well. Please refer to the
 * rectangle tools spec.
 */
static void
gimp_rectangle_tool_apply_fixed_height (GimpRectangleTool      *rect_tool,
                                        GimpRectangleConstraint constraint,
                                        gdouble                 height)

{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (private->function)
    {
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_TOP:

      /* We always want to center around fixed_center here, since we
       * want the anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    height / 2;
      private->y2 = private->y1 + height;

      break;

    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
    case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
    case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:

      /* We always want to center around fixed_center here, since we
       * want the anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    height / 2;
      private->y2 = private->y1 + height;

      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_rectangle_tool_keep_inside_vertically (rect_tool,
                                              constraint);
}

/**
 * gimp_rectangle_tool_apply_aspect:
 * @rect_tool:      A #GimpRectangleTool.
 * @aspect:         The desired aspect.
 * @clamped_sides:  Bitfield of sides that have been clamped.
 *
 * Adjust the rectangle to the desired aspect.
 *
 * Sometimes, a side must not be moved outwards, for example if a the
 * RIGHT side has been clamped previously, we must not move the RIGHT
 * side to the right, since that would violate the constraint
 * again. The clamped_sides bitfield keeps track of sides that have
 * previously been clamped.
 *
 * If fixed_center is used, the function adjusts the aspect by
 * symmetrically adjusting the left and right, or top and bottom side.
 */
static void
gimp_rectangle_tool_apply_aspect (GimpRectangleTool *rect_tool,
                                  gdouble            aspect,
                                  gint               clamped_sides)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  gdouble                      current_w;
  gdouble                      current_h;
  gdouble                      current_aspect;
  SideToResize                 side_to_resize = SIDE_TO_RESIZE_NONE;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rect_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  current_w = private->x2 - private->x1;
  current_h = private->y2 - private->y1;

  current_aspect = current_w / (gdouble) current_h;

  /* Do we have to do anything? */
  if (current_aspect == aspect)
    return;

  if (options_private->fixed_center)
    {
      /* We may only adjust the sides symmetrically to get desired aspect. */
      if (current_aspect > aspect)
        {
          /* We prefer to use top and bottom (since that will make the
           * cursor remain on the rectangle edge), unless that is what
           * the user has grabbed.
           */
          switch (private->function)
            {
            case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
            case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
            case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
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

            case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
            case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
            default:
              side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
              break;
            }
        }
      else /* (current_aspect < aspect) */
        {
          /* We prefer to use left and right (since that will make the
           * cursor remain on the rectangle edge), unless that is what
           * the user has grabbed.
           */
          switch (private->function)
            {
            case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
            case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
            case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
            case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
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

            case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
            case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
            default:
              side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
              break;
            }
        }
    }
  else if (current_aspect > aspect)
    {
      /* We can safely pick LEFT or RIGHT, since using those sides
       * will make the rectangle smaller, so we don't need to check
       * for clamped_sides. We may only use TOP and BOTTOM if not
       * those sides have been clamped, since using them will make the
       * rectangle bigger.
       */
      switch (private->function)
        {
        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
          if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
          if (!(clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
          if (!(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
          if (!(clamped_sides & CLAMPED_TOP) &&
              !(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
          if (!(clamped_sides & CLAMPED_TOP) &&
              !(clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
        case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
          side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          break;

        case GIMP_RECTANGLE_TOOL_MOVING:
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
      /* We can safely pick TOP or BOTTOM, since using those sides
       * will make the rectangle smaller, so we don't need to check
       * for clamped_sides. We may only use LEFT and RIGHT if not
       * those sides have been clamped, since using them will make the
       * rectangle bigger.
       */
      switch (private->function)
        {
        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_LEFT:
          if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_UPPER_RIGHT:
          if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_LEFT:
          if (!(clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LOWER_RIGHT:
          if (!(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_TOP:
          if (!(clamped_sides & CLAMPED_LEFT) &&
              !(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_BOTTOM:
          if (!(clamped_sides & CLAMPED_LEFT) &&
              !(clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_RECTANGLE_TOOL_RESIZING_LEFT:
        case GIMP_RECTANGLE_TOOL_RESIZING_RIGHT:
          side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          break;

        case GIMP_RECTANGLE_TOOL_MOVING:
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

  /* We now know what side(s) we should resize, so now we just solve
   * the aspect equation for that side(s).
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
        gdouble correct_h = current_w / aspect;

        private->y1 = private->center_y_on_fixed_center - correct_h / 2;
        private->y2 = private->y1 + correct_h;
      }
      break;

    case SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY:
      {
        gdouble correct_w = current_h * aspect;

        private->x1 = private->center_x_on_fixed_center - correct_w / 2;
        private->x2 = private->x1 + correct_w;
      }
      break;
    }
}

/**
 * gimp_rectangle_tool_update_with_coord:
 * @rect_tool:      A #GimpRectangleTool.
 * @new_x:          New X-coordinate in the context of the current function.
 * @new_y:          New Y-coordinate in the context of the current function.
 *
 * The core rectangle adjustment function. It updates the rectangle
 * for the passed cursor coordinate, taking current function and tool
 * options into account.  It also updates the current
 * private->function if necessary.
 */
static void
gimp_rectangle_tool_update_with_coord (GimpRectangleTool *rect_tool,
                                       gdouble            new_x,
                                       gdouble            new_y)
{
  GimpRectangleToolPrivate *private;

  private = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  /* Move the corner or edge the user currently has grabbed. */
  gimp_rectangle_tool_apply_coord (rect_tool,
                                   new_x,
                                   new_y);

  /* Update private->function. The function changes if the user
   * "flips" the rectangle.
   */
  gimp_rectangle_tool_check_function (rect_tool);

  /* Clamp the rectangle if necessary */
  gimp_rectangle_tool_handle_general_clamping (rect_tool);

  /* If the rectangle is being moved, do not run through any further
   * rectangle adjusting functions since it's shape should not change
   * then.
   */
  if (private->function != GIMP_RECTANGLE_TOOL_MOVING)
    {
      gimp_rectangle_tool_apply_fixed_rule (rect_tool);
    }

  gimp_rectangle_tool_update_int_rect (rect_tool);
}

static void
gimp_rectangle_tool_apply_fixed_rule (GimpRectangleTool *rect_tool)
{
  GimpTool                    *tool;
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleConstraint      constraint_to_use;
  GimpImage                   *image;

  tool            = GIMP_TOOL (rect_tool);
  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);
  image           = gimp_display_get_image (tool->display);

  /* Calculate what constraint to use when needed. */
  constraint_to_use = gimp_rectangle_tool_get_constraint (rect_tool);

  if (gimp_rectangle_options_fixed_rule_active (options,
                                                GIMP_RECTANGLE_FIXED_ASPECT))
    {
      gdouble aspect;

      aspect = CLAMP (options_private->aspect_numerator /
                      options_private->aspect_denominator,
                      1.0 / gimp_image_get_height (image),
                      gimp_image_get_width (image));

      if (constraint_to_use == GIMP_RECTANGLE_CONSTRAIN_NONE)
        {
          gimp_rectangle_tool_apply_aspect (rect_tool,
                                            aspect,
                                            CLAMPED_NONE);
        }
      else
        {
          if (private->function != GIMP_RECTANGLE_TOOL_MOVING)
            {
              ClampedSide clamped_sides = CLAMPED_NONE;

              gimp_rectangle_tool_apply_aspect (rect_tool,
                                                aspect,
                                                clamped_sides);

              /* After we have applied aspect, we might have taken the
               * rectangle outside of constraint, so clamp and apply
               * aspect again. We will get the right result this time,
               * since 'clamped_sides' will be setup correctly now.
               */
              gimp_rectangle_tool_clamp (rect_tool,
                                         &clamped_sides,
                                         constraint_to_use,
                                         options_private->fixed_center);

              gimp_rectangle_tool_apply_aspect (rect_tool,
                                                aspect,
                                                clamped_sides);
            }
          else
            {
              gimp_rectangle_tool_apply_aspect (rect_tool,
                                                aspect,
                                                CLAMPED_NONE);

              gimp_rectangle_tool_keep_inside (rect_tool,
                                               constraint_to_use);
            }
        }
    }
  else if (gimp_rectangle_options_fixed_rule_active (options,
                                                     GIMP_RECTANGLE_FIXED_SIZE))
    {
      gimp_rectangle_tool_apply_fixed_width (rect_tool,
                                             constraint_to_use,
                                             options_private->desired_fixed_size_width);
      gimp_rectangle_tool_apply_fixed_height (rect_tool,
                                              constraint_to_use,
                                              options_private->desired_fixed_size_height);
    }
  else if (gimp_rectangle_options_fixed_rule_active (options,
                                                     GIMP_RECTANGLE_FIXED_WIDTH))
    {
      gimp_rectangle_tool_apply_fixed_width (rect_tool,
                                             constraint_to_use,
                                             options_private->desired_fixed_width);
    }
  else if (gimp_rectangle_options_fixed_rule_active (options,
                                                     GIMP_RECTANGLE_FIXED_HEIGHT))
    {
      gimp_rectangle_tool_apply_fixed_height (rect_tool,
                                              constraint_to_use,
                                              options_private->desired_fixed_height);
    }
}

/**
 * gimp_rectangle_tool_get_constraints:
 * @rect_tool:      A #GimpRectangleTool.
 * @min_x:
 * @min_y:
 * @max_x:
 * @max_y:          Pointers of where to put constraints. NULL allowed.
 * @constraint:     Whether to return image or layer constraints.
 *
 * Calculates constraint coordinates for image or layer.
 */
static void
gimp_rectangle_tool_get_constraints (GimpRectangleTool       *rect_tool,
                                     gint                    *min_x,
                                     gint                    *min_y,
                                     gint                    *max_x,
                                     gint                    *max_y,
                                     GimpRectangleConstraint  constraint)
{
  GimpTool  *tool = GIMP_TOOL (rect_tool);
  GimpImage *image;
  gint       min_x_dummy;
  gint       min_y_dummy;
  gint       max_x_dummy;
  gint       max_y_dummy;

  if (! min_x) min_x = &min_x_dummy;
  if (! min_y) min_y = &min_y_dummy;
  if (! max_x) max_x = &max_x_dummy;
  if (! max_y) max_y = &max_y_dummy;

  *min_x = 0;
  *min_y = 0;
  *max_x = 0;
  *max_y = 0;

  if (! tool->display)
    return;

  image = gimp_display_get_image (tool->display);

  switch (constraint)
    {
    case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
      *min_x = 0;
      *min_y = 0;
      *max_x = gimp_image_get_width  (image);
      *max_y = gimp_image_get_height (image);
      break;

    case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
      {
        GimpItem *item = GIMP_ITEM (tool->drawable);

        gimp_item_get_offset (item, min_x, min_y);
        *max_x = *min_x + gimp_item_get_width  (item);
        *max_y = *min_y + gimp_item_get_height (item);
      }
      break;

    default:
      g_warning ("Invalid rectangle constraint.\n");
      return;
    }
}

/**
 * gimp_rectangle_tool_handle_general_clamping:
 * @rect_tool: A #GimpRectangleTool.
 *
 * Make sure that contraints are applied to the rectangle, either by
 * manually doing it, or by looking at the rectangle tool options and
 * concluding it will be done later.
 */
static void
gimp_rectangle_tool_handle_general_clamping (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolPrivate    *private;
  GimpRectangleOptions        *options;
  GimpRectangleOptionsPrivate *options_private;
  GimpRectangleConstraint      constraint;

  private         = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);
  options         = GIMP_RECTANGLE_TOOL_GET_OPTIONS (rect_tool);
  options_private = GIMP_RECTANGLE_OPTIONS_GET_PRIVATE (options);

  constraint = gimp_rectangle_tool_get_constraint (rect_tool);

  /* fixed_aspect takes care of clamping by it self, so just return in
   * case that is in use. Also return if no constraints should be
   * enforced.
   */
  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  if (private->function != GIMP_RECTANGLE_TOOL_MOVING)
    {
      gimp_rectangle_tool_clamp (rect_tool,
                                 NULL,
                                 constraint,
                                 options_private->fixed_center);
    }
  else
    {
      gimp_rectangle_tool_keep_inside (rect_tool,
                                       constraint);
    }
}

/**
 * gimp_rectangle_tool_update_int_rect:
 * @rect_tool:
 *
 * Update integer representation of rectangle.
 **/
static void
gimp_rectangle_tool_update_int_rect (GimpRectangleTool *rect_tool)
{
  GimpRectangleToolPrivate *priv = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  priv->x1_int = SIGNED_ROUND (priv->x1);
  priv->y1_int = SIGNED_ROUND (priv->y1);

  if (gimp_rectangle_tool_rect_rubber_banding_func (rect_tool))
    {
      priv->width_int  = (gint) SIGNED_ROUND (priv->x2) - priv->x1_int;
      priv->height_int = (gint) SIGNED_ROUND (priv->y2) - priv->y1_int;
    }
}

/**
 * gimp_rectangle_tool_get_public_rect:
 * @rect_tool:
 * @pub_x1:
 * @pub_y1:
 * @pub_x2:
 * @pub_y2:
 *
 * This function returns the rectangle as it appears to be publicly
 * (based on integer or double precision-mode).
 **/
static void
gimp_rectangle_tool_get_public_rect (GimpRectangleTool *rect_tool,
                                     gdouble           *pub_x1,
                                     gdouble           *pub_y1,
                                     gdouble           *pub_x2,
                                     gdouble           *pub_y2)
{
  GimpRectangleToolPrivate *priv;

  priv = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (priv->precision)
    {
      case GIMP_RECTANGLE_PRECISION_INT:
        *pub_x1 = priv->x1_int;
        *pub_y1 = priv->y1_int;
        *pub_x2 = priv->x1_int + priv->width_int;
        *pub_y2 = priv->y1_int + priv->height_int;

        break;

      case GIMP_RECTANGLE_PRECISION_DOUBLE:
      default:
        *pub_x1 = priv->x1;
        *pub_y1 = priv->y1;
        *pub_x2 = priv->x2;
        *pub_y2 = priv->y2;
        break;
    }
}

/**
 * gimp_rectangle_tool_adjust_coord:
 * @rect_tool:
 * @ccoord_x_input:
 * @ccoord_x_input:
 * @ccoord_x_output:
 * @ccoord_x_output:
 *
 * Transforms a coordinate to better fit the public behaviour of the
 * rectangle.
 */
static void
gimp_rectangle_tool_adjust_coord (GimpRectangleTool *rect_tool,
                                  gdouble            coord_x_input,
                                  gdouble            coord_y_input,
                                  gdouble           *coord_x_output,
                                  gdouble           *coord_y_output)
{
  GimpRectangleToolPrivate *priv;

  priv = GIMP_RECTANGLE_TOOL_GET_PRIVATE (rect_tool);

  switch (priv->precision)
    {
      case GIMP_RECTANGLE_PRECISION_INT:
        *coord_x_output = RINT (coord_x_input);
        *coord_y_output = RINT (coord_y_input);
        break;

      case GIMP_RECTANGLE_PRECISION_DOUBLE:
      default:
        *coord_x_output = coord_x_input;
        *coord_y_output = coord_y_input;
        break;
    }
}
