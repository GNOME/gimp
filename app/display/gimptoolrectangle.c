/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptoolrectangle.c
 * Copyright (C) 2017 Michael Natterer <mitch@gimp.org>
 *
 * Based on GimpRectangleTool
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "display-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"
#include "core/gimppickable.h"
#include "core/gimppickable-auto-shrink.h"

#include "widgets/gimpwidgets-utils.h"

#include "gimpcanvasarc.h"
#include "gimpcanvascorner.h"
#include "gimpcanvashandle.h"
#include "gimpcanvasitem-utils.h"
#include "gimpcanvasrectangle.h"
#include "gimpcanvasrectangleguides.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-scroll.h"
#include "gimptoolrectangle.h"

#include "gimp-intl.h"


/*  speed of key movement  */
#define ARROW_VELOCITY   25

#define MAX_HANDLE_SIZE         50
#define MIN_HANDLE_SIZE         15
#define NARROW_MODE_HANDLE_SIZE 15
#define NARROW_MODE_THRESHOLD   45


enum
{
  PROP_0,
  PROP_X1,
  PROP_Y1,
  PROP_X2,
  PROP_Y2,
  PROP_CONSTRAINT,
  PROP_PRECISION,
  PROP_NARROW_MODE,
  PROP_FORCE_NARROW_MODE,
  PROP_DRAW_ELLIPSE,
  PROP_ROUND_CORNERS,
  PROP_CORNER_RADIUS,
  PROP_STATUS_TITLE,

  PROP_HIGHLIGHT,
  PROP_HIGHLIGHT_OPACITY,
  PROP_GUIDE,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_FIXED_RULE_ACTIVE,
  PROP_FIXED_RULE,
  PROP_DESIRED_FIXED_WIDTH,
  PROP_DESIRED_FIXED_HEIGHT,
  PROP_DESIRED_FIXED_SIZE_WIDTH,
  PROP_DESIRED_FIXED_SIZE_HEIGHT,
  PROP_ASPECT_NUMERATOR,
  PROP_ASPECT_DENOMINATOR,
  PROP_FIXED_CENTER
};

enum
{
  CHANGE_COMPLETE,
  LAST_SIGNAL
};

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


struct _GimpToolRectanglePrivate
{
  /* The following members are "constants", that is, variables that are setup
   * during gimp_tool_rectangle_button_press and then only read.
   */

  /* Whether or not the rectangle currently being rubber-banded is the
   * first one created with this instance, this determines if we can
   * undo it on button_release.
   */
  gboolean                is_first;

  /* Whether or not the rectangle currently being rubber-banded was
   * created from scratch.
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

  /* Integer coordinates of upper left corner and size. We must
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

  /* How to constrain the rectangle. */
  GimpRectangleConstraint constraint;

  /* What precision the rectangle will appear to have externally (it
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

  /* This boolean allows to always set narrow mode */
  gboolean                force_narrow_mode;

  /* Whether or not to draw an ellipse inside the rectangle */
  gboolean                draw_ellipse;

  /* Whether to draw round corners */
  gboolean                round_corners;
  gdouble                 corner_radius;

  /* The title for the statusbar coords */
  gchar                  *status_title;

  /* For saving in case of cancellation. */
  gdouble                 saved_x1;
  gdouble                 saved_y1;
  gdouble                 saved_x2;
  gdouble                 saved_y2;

  gint                    suppress_updates;

  GimpRectangleFunction   function;

  /* The following values are externally synced with GimpRectangleOptions */

  gboolean                highlight;
  gdouble                 highlight_opacity;
  GimpGuidesType          guide;

  gdouble                 x;
  gdouble                 y;
  gdouble                 width;
  gdouble                 height;

  gboolean                fixed_rule_active;
  GimpRectangleFixedRule  fixed_rule;
  gdouble                 desired_fixed_width;
  gdouble                 desired_fixed_height;
  gdouble                 desired_fixed_size_width;
  gdouble                 desired_fixed_size_height;
  gdouble                 aspect_numerator;
  gdouble                 aspect_denominator;
  gboolean                fixed_center;

  /* Canvas items for drawing the GUI */

  GimpCanvasItem         *guides;
  GimpCanvasItem         *rectangle;
  GimpCanvasItem         *ellipse;
  GimpCanvasItem         *corners[4];
  GimpCanvasItem         *center;
  GimpCanvasItem         *creating_corners[4];
  GimpCanvasItem         *handles[GIMP_N_TOOL_RECTANGLE_FUNCTIONS];
  GimpCanvasItem         *highlight_handles[GIMP_N_TOOL_RECTANGLE_FUNCTIONS];

  /* Flags to prevent temporary changes to the Expand from center and Fixed
     options for the rectangle select tool, ellipse select tool and the crop
     tool due to use of the modifier keys becoming latched See issue #7954 and
     MR !779. */
  gboolean                fixed_center_copy;
  gboolean                fixed_rule_active_copy;
  gboolean                modifier_toggle_allowed;
};


/*  local function prototypes  */

static void     gimp_tool_rectangle_constructed     (GObject               *object);
static void     gimp_tool_rectangle_finalize        (GObject               *object);
static void     gimp_tool_rectangle_set_property    (GObject               *object,
                                                     guint                  property_id,
                                                     const GValue          *value,
                                                     GParamSpec            *pspec);
static void     gimp_tool_rectangle_get_property    (GObject               *object,
                                                     guint                  property_id,
                                                     GValue                *value,
                                                     GParamSpec            *pspec);
static void     gimp_tool_rectangle_notify          (GObject               *object,
                                                     GParamSpec            *pspec);

static void     gimp_tool_rectangle_changed         (GimpToolWidget        *widget);
static gint     gimp_tool_rectangle_button_press    (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonPressType    press_type);
static void     gimp_tool_rectangle_button_release  (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state,
                                                     GimpButtonReleaseType  release_type);
static void     gimp_tool_rectangle_motion          (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     guint32                time,
                                                     GdkModifierType        state);
static GimpHit  gimp_tool_rectangle_hit             (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity);
static void     gimp_tool_rectangle_hover           (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     gboolean               proximity);
static void     gimp_tool_rectangle_leave_notify    (GimpToolWidget        *widget);
static gboolean gimp_tool_rectangle_key_press       (GimpToolWidget        *widget,
                                                     GdkEventKey           *kevent);
static void     gimp_tool_rectangle_motion_modifier (GimpToolWidget        *widget,
                                                     GdkModifierType        key,
                                                     gboolean               press,
                                                     GdkModifierType        state);
static gboolean gimp_tool_rectangle_get_cursor      (GimpToolWidget        *widget,
                                                     const GimpCoords      *coords,
                                                     GdkModifierType        state,
                                                     GimpCursorType        *cursor,
                                                     GimpToolCursorType    *tool_cursor,
                                                     GimpCursorModifier    *modifier);

static void     gimp_tool_rectangle_change_complete (GimpToolRectangle     *rectangle);

static void     gimp_tool_rectangle_update_options  (GimpToolRectangle     *rectangle);
static void     gimp_tool_rectangle_update_handle_sizes
                                                    (GimpToolRectangle     *rectangle);
static void     gimp_tool_rectangle_update_status   (GimpToolRectangle     *rectangle);

static void     gimp_tool_rectangle_synthesize_motion
                                                    (GimpToolRectangle     *rectangle,
                                                     gint               function,
                                                     gdouble            new_x,
                                                     gdouble            new_y);

static GimpRectangleFunction
                gimp_tool_rectangle_calc_function   (GimpToolRectangle     *rectangle,
                                                     const GimpCoords      *coords,
                                                     gboolean               proximity);
static void     gimp_tool_rectangle_check_function  (GimpToolRectangle     *rectangle);

static gboolean gimp_tool_rectangle_coord_outside   (GimpToolRectangle     *rectangle,
                                                     const GimpCoords      *coords);

static gboolean gimp_tool_rectangle_coord_on_handle (GimpToolRectangle     *rectangle,
                                                     const GimpCoords      *coords,
                                                     GimpHandleAnchor       anchor);

static GimpHandleAnchor gimp_tool_rectangle_get_anchor
                                                    (GimpRectangleFunction  function);
static gboolean gimp_tool_rectangle_rect_rubber_banding_func
                                                    (GimpToolRectangle     *rectangle);
static gboolean gimp_tool_rectangle_rect_adjusting_func
                                                    (GimpToolRectangle     *rectangle);

static void     gimp_tool_rectangle_get_other_side  (GimpToolRectangle     *rectangle,
                                                     gdouble              **other_x,
                                                     gdouble              **other_y);
static void     gimp_tool_rectangle_get_other_side_coord
                                                    (GimpToolRectangle     *rectangle,
                                                     gdouble               *other_side_x,
                                                     gdouble               *other_side_y);
static void     gimp_tool_rectangle_set_other_side_coord
                                                    (GimpToolRectangle     *rectangle,
                                                     gdouble                other_side_x,
                                                     gdouble                other_side_y);

static void     gimp_tool_rectangle_apply_coord     (GimpToolRectangle     *rectangle,
                                                     gdouble                coord_x,
                                                     gdouble                coord_y);
static void     gimp_tool_rectangle_setup_snap_offsets
                                                    (GimpToolRectangle     *rectangle,
                                                     const GimpCoords      *coords);

static void     gimp_tool_rectangle_clamp           (GimpToolRectangle     *rectangle,
                                                     ClampedSide           *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean               symmetrically);
static void     gimp_tool_rectangle_clamp_width     (GimpToolRectangle     *rectangle,
                                                     ClampedSide           *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean               symmetrically);
static void     gimp_tool_rectangle_clamp_height    (GimpToolRectangle     *rectangle,
                                                     ClampedSide           *clamped_sides,
                                                     GimpRectangleConstraint constraint,
                                                     gboolean               symmetrically);

static void     gimp_tool_rectangle_keep_inside     (GimpToolRectangle     *rectangle,
                                                     GimpRectangleConstraint constraint);
static void     gimp_tool_rectangle_keep_inside_horizontally
                                                    (GimpToolRectangle     *rectangle,
                                                     GimpRectangleConstraint constraint);
static void     gimp_tool_rectangle_keep_inside_vertically
                                                    (GimpToolRectangle     *rectangle,
                                                     GimpRectangleConstraint constraint);

static void     gimp_tool_rectangle_apply_fixed_width
                                                    (GimpToolRectangle     *rectangle,
                                                     GimpRectangleConstraint constraint,
                                                     gdouble                width);
static void     gimp_tool_rectangle_apply_fixed_height
                                                    (GimpToolRectangle     *rectangle,
                                                     GimpRectangleConstraint constraint,
                                                     gdouble                height);

static void     gimp_tool_rectangle_apply_aspect    (GimpToolRectangle     *rectangle,
                                                     gdouble                aspect,
                                                     gint                   clamped_sides);

static void     gimp_tool_rectangle_update_with_coord
                                                    (GimpToolRectangle     *rectangle,
                                                     gdouble                new_x,
                                                     gdouble                new_y);
static void     gimp_tool_rectangle_apply_fixed_rule(GimpToolRectangle     *rectangle);

static void     gimp_tool_rectangle_get_constraints (GimpToolRectangle     *rectangle,
                                                     gint                  *min_x,
                                                     gint                  *min_y,
                                                     gint                  *max_x,
                                                     gint                  *max_y,
                                                     GimpRectangleConstraint constraint);

static void     gimp_tool_rectangle_handle_general_clamping
                                                    (GimpToolRectangle     *rectangle);
static void     gimp_tool_rectangle_update_int_rect (GimpToolRectangle     *rectangle);
static void     gimp_tool_rectangle_adjust_coord    (GimpToolRectangle     *rectangle,
                                                     gdouble                coord_x_input,
                                                     gdouble                coord_y_input,
                                                     gdouble               *coord_x_output,
                                                     gdouble               *coord_y_output);
static void     gimp_tool_rectangle_recalculate_center_xy
                                                    (GimpToolRectangle     *rectangle);


G_DEFINE_TYPE_WITH_PRIVATE (GimpToolRectangle, gimp_tool_rectangle,
                            GIMP_TYPE_TOOL_WIDGET)

#define parent_class gimp_tool_rectangle_parent_class

static guint rectangle_signals[LAST_SIGNAL] = { 0, };


static void
gimp_tool_rectangle_class_init (GimpToolRectangleClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  GimpToolWidgetClass *widget_class = GIMP_TOOL_WIDGET_CLASS (klass);

  object_class->constructed     = gimp_tool_rectangle_constructed;
  object_class->finalize        = gimp_tool_rectangle_finalize;
  object_class->set_property    = gimp_tool_rectangle_set_property;
  object_class->get_property    = gimp_tool_rectangle_get_property;
  object_class->notify          = gimp_tool_rectangle_notify;

  widget_class->changed         = gimp_tool_rectangle_changed;
  widget_class->button_press    = gimp_tool_rectangle_button_press;
  widget_class->button_release  = gimp_tool_rectangle_button_release;
  widget_class->motion          = gimp_tool_rectangle_motion;
  widget_class->hit             = gimp_tool_rectangle_hit;
  widget_class->hover           = gimp_tool_rectangle_hover;
  widget_class->leave_notify    = gimp_tool_rectangle_leave_notify;
  widget_class->key_press       = gimp_tool_rectangle_key_press;
  widget_class->motion_modifier = gimp_tool_rectangle_motion_modifier;
  widget_class->get_cursor      = gimp_tool_rectangle_get_cursor;
  widget_class->update_on_scale = TRUE;

  rectangle_signals[CHANGE_COMPLETE] =
    g_signal_new ("change-complete",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpToolRectangleClass, change_complete),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_X1,
                                   g_param_spec_double ("x1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y1,
                                   g_param_spec_double ("y1",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X2,
                                   g_param_spec_double ("x2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y2,
                                   g_param_spec_double ("y2",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CONSTRAINT,
                                   g_param_spec_enum ("constraint",
                                                      NULL, NULL,
                                                      GIMP_TYPE_RECTANGLE_CONSTRAINT,
                                                      GIMP_RECTANGLE_CONSTRAIN_NONE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_PRECISION,
                                   g_param_spec_enum ("precision",
                                                      NULL, NULL,
                                                      GIMP_TYPE_RECTANGLE_PRECISION,
                                                      GIMP_RECTANGLE_PRECISION_INT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_NARROW_MODE,
                                   g_param_spec_boolean ("narrow-mode",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FORCE_NARROW_MODE,
                                   g_param_spec_boolean ("force-narrow-mode",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DRAW_ELLIPSE,
                                   g_param_spec_boolean ("draw-ellipse",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ROUND_CORNERS,
                                   g_param_spec_boolean ("round-corners",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_CORNER_RADIUS,
                                   g_param_spec_double ("corner-radius",
                                                        NULL, NULL,
                                                        0.0, 10000.0, 10.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_STATUS_TITLE,
                                   g_param_spec_string ("status-title",
                                                        NULL, NULL,
                                                        _("Rectangle: "),
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGHLIGHT,
                                   g_param_spec_boolean ("highlight",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HIGHLIGHT_OPACITY,
                                   g_param_spec_double ("highlight-opacity",
                                                        NULL, NULL,
                                                        0.0, 1.0, 0.5,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_GUIDE,
                                   g_param_spec_enum ("guide",
                                                      NULL, NULL,
                                                      GIMP_TYPE_GUIDES_TYPE,
                                                      GIMP_GUIDES_NONE,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y",
                                                        NULL, NULL,
                                                        -GIMP_MAX_IMAGE_SIZE,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_double ("width",
                                                        NULL, NULL,
                                                        0.0,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_double ("height",
                                                        NULL, NULL,
                                                        0.0,
                                                        GIMP_MAX_IMAGE_SIZE,
                                                        0.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FIXED_RULE_ACTIVE,
                                   g_param_spec_boolean ("fixed-rule-active",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FIXED_RULE,
                                   g_param_spec_enum ("fixed-rule",
                                                      NULL, NULL,
                                                      GIMP_TYPE_RECTANGLE_FIXED_RULE,
                                                      GIMP_RECTANGLE_FIXED_ASPECT,
                                                      GIMP_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DESIRED_FIXED_WIDTH,
                                   g_param_spec_double ("desired-fixed-width",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DESIRED_FIXED_HEIGHT,
                                   g_param_spec_double ("desired-fixed-height",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DESIRED_FIXED_SIZE_WIDTH,
                                   g_param_spec_double ("desired-fixed-size-width",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DESIRED_FIXED_SIZE_HEIGHT,
                                   g_param_spec_double ("desired-fixed-size-height",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        100.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ASPECT_NUMERATOR,
                                   g_param_spec_double ("aspect-numerator",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ASPECT_DENOMINATOR,
                                   g_param_spec_double ("aspect-denominator",
                                                        NULL, NULL,
                                                        0.0, GIMP_MAX_IMAGE_SIZE,
                                                        1.0,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FIXED_CENTER,
                                   g_param_spec_boolean ("fixed-center",
                                                         NULL, NULL,
                                                         FALSE,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_tool_rectangle_init (GimpToolRectangle *rectangle)
{
  rectangle->private = gimp_tool_rectangle_get_instance_private (rectangle);

  rectangle->private->function = GIMP_TOOL_RECTANGLE_CREATING;
  rectangle->private->is_first = TRUE;
  rectangle->private->modifier_toggle_allowed = FALSE;
}

static void
gimp_tool_rectangle_constructed (GObject *object)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (object);
  GimpToolWidget           *widget    = GIMP_TOOL_WIDGET (object);
  GimpToolRectanglePrivate *private   = rectangle->private;
  GimpCanvasGroup          *stroke_group;
  gint                      i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  stroke_group = gimp_tool_widget_add_stroke_group (widget);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->guides = gimp_tool_widget_add_rectangle_guides (widget,
                                                           0, 0, 10, 10,
                                                           GIMP_GUIDES_NONE);

  private->rectangle = gimp_tool_widget_add_rectangle (widget,
                                                       0, 0, 10, 10,
                                                       FALSE);

  private->ellipse = gimp_tool_widget_add_arc (widget,
                                               0, 0, 10, 10,
                                               0.0, 2 * G_PI,
                                               FALSE);

  for (i = 0; i < 4; i++)
    private->corners[i] = gimp_tool_widget_add_arc (widget,
                                                    0, 0, 10, 10,
                                                    0.0, 2 * G_PI,
                                                    FALSE);

  gimp_tool_widget_pop_group (widget);

  private->center = gimp_tool_widget_add_handle (widget,
                                                 GIMP_HANDLE_CROSS,
                                                 0, 0,
                                                 GIMP_CANVAS_HANDLE_SIZE_SMALL,
                                                 GIMP_CANVAS_HANDLE_SIZE_SMALL,
                                                 GIMP_HANDLE_ANCHOR_CENTER);

  gimp_tool_widget_push_group (widget, stroke_group);

  private->creating_corners[0] =
    gimp_tool_widget_add_corner (widget,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_NORTH_WEST,
                                 10, 10,
                                 FALSE);

  private->creating_corners[1] =
    gimp_tool_widget_add_corner (widget,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_NORTH_EAST,
                                 10, 10,
                                 FALSE);

  private->creating_corners[2] =
    gimp_tool_widget_add_corner (widget,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_SOUTH_WEST,
                                 10, 10,
                                 FALSE);

  private->creating_corners[3] =
    gimp_tool_widget_add_corner (widget,
                                 0, 0, 10, 10,
                                 GIMP_HANDLE_ANCHOR_SOUTH_EAST,
                                 10, 10,
                                 FALSE);

  gimp_tool_widget_pop_group (widget);

  for (i = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT;
       i <= GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM;
       i++)
    {
      GimpHandleAnchor anchor;

      anchor = gimp_tool_rectangle_get_anchor (i);

      gimp_tool_widget_push_group (widget, stroke_group);

      private->handles[i] = gimp_tool_widget_add_corner (widget,
                                                         0, 0, 10, 10,
                                                         anchor,
                                                         10, 10,
                                                         FALSE);

      gimp_tool_widget_pop_group (widget);

      private->highlight_handles[i] = gimp_tool_widget_add_corner (widget,
                                                                   0, 0, 10, 10,
                                                                   anchor,
                                                                   10, 10,
                                                                   FALSE);
      gimp_canvas_item_set_highlight (private->highlight_handles[i], TRUE);
    }

  gimp_tool_rectangle_changed (widget);
}

static void
gimp_tool_rectangle_finalize (GObject *object)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (object);
  GimpToolRectanglePrivate *private   = rectangle->private;

  g_clear_pointer (&private->status_title, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tool_rectangle_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (object);
  GimpToolRectanglePrivate *private   = rectangle->private;

  switch (property_id)
    {
    case PROP_X1:
      private->x1 = g_value_get_double (value);
      break;
    case PROP_Y1:
      private->y1 = g_value_get_double (value);
      break;
    case PROP_X2:
      private->x2 = g_value_get_double (value);
      break;
    case PROP_Y2:
      private->y2 = g_value_get_double (value);
      break;

    case PROP_CONSTRAINT:
      private->constraint = g_value_get_enum (value);
      break;
    case PROP_PRECISION:
      private->precision = g_value_get_enum (value);
      break;

    case PROP_NARROW_MODE:
      private->narrow_mode = g_value_get_boolean (value);
      break;
    case PROP_FORCE_NARROW_MODE:
      private->force_narrow_mode = g_value_get_boolean (value);
      break;
    case PROP_DRAW_ELLIPSE:
      private->draw_ellipse = g_value_get_boolean (value);
      break;
    case PROP_ROUND_CORNERS:
      private->round_corners = g_value_get_boolean (value);
      break;
    case PROP_CORNER_RADIUS:
      private->corner_radius = g_value_get_double (value);
      break;

    case PROP_STATUS_TITLE:
      g_set_str (&private->status_title, g_value_get_string (value));
      if (! private->status_title)
        private->status_title = g_strdup (_("Rectangle: "));
      break;

    case PROP_HIGHLIGHT:
      private->highlight = g_value_get_boolean (value);
      break;
    case PROP_HIGHLIGHT_OPACITY:
      private->highlight_opacity = g_value_get_double (value);
      break;
    case PROP_GUIDE:
      private->guide = g_value_get_enum (value);
      break;

    case PROP_X:
      private->x = g_value_get_double (value);
      break;
    case PROP_Y:
      private->y = g_value_get_double (value);
      break;
    case PROP_WIDTH:
      private->width = g_value_get_double (value);
      break;
    case PROP_HEIGHT:
      private->height = g_value_get_double (value);
      break;

    case PROP_FIXED_RULE_ACTIVE:
      private->fixed_rule_active = g_value_get_boolean (value);
      break;
    case PROP_FIXED_RULE:
      private->fixed_rule = g_value_get_enum (value);
      break;
    case PROP_DESIRED_FIXED_WIDTH:
      private->desired_fixed_width = g_value_get_double (value);
      break;
    case PROP_DESIRED_FIXED_HEIGHT:
      private->desired_fixed_height = g_value_get_double (value);
      break;
    case PROP_DESIRED_FIXED_SIZE_WIDTH:
      private->desired_fixed_size_width = g_value_get_double (value);
      break;
    case PROP_DESIRED_FIXED_SIZE_HEIGHT:
      private->desired_fixed_size_height = g_value_get_double (value);
      break;
    case PROP_ASPECT_NUMERATOR:
      private->aspect_numerator = g_value_get_double (value);
      break;
    case PROP_ASPECT_DENOMINATOR:
      private->aspect_denominator = g_value_get_double (value);
      break;

    case PROP_FIXED_CENTER:
      private->fixed_center = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_rectangle_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (object);
  GimpToolRectanglePrivate *private   = rectangle->private;

  switch (property_id)
    {
    case PROP_X1:
      g_value_set_double (value, private->x1);
      break;
    case PROP_Y1:
      g_value_set_double (value, private->y1);
      break;
    case PROP_X2:
      g_value_set_double (value, private->x2);
      break;
    case PROP_Y2:
      g_value_set_double (value, private->y2);
      break;

    case PROP_CONSTRAINT:
      g_value_set_enum (value, private->constraint);
      break;
    case PROP_PRECISION:
      g_value_set_enum (value, private->precision);
      break;

    case PROP_NARROW_MODE:
      g_value_set_boolean (value, private->narrow_mode);
      break;
    case PROP_FORCE_NARROW_MODE:
      g_value_set_boolean (value, private->force_narrow_mode);
      break;
    case PROP_DRAW_ELLIPSE:
      g_value_set_boolean (value, private->draw_ellipse);
      break;
    case PROP_ROUND_CORNERS:
      g_value_set_boolean (value, private->round_corners);
      break;
    case PROP_CORNER_RADIUS:
      g_value_set_double (value, private->corner_radius);
      break;

    case PROP_STATUS_TITLE:
      g_value_set_string (value, private->status_title);
      break;

    case PROP_HIGHLIGHT:
      g_value_set_boolean (value, private->highlight);
      break;
    case PROP_HIGHLIGHT_OPACITY:
      g_value_set_double (value, private->highlight_opacity);
      break;
    case PROP_GUIDE:
      g_value_set_enum (value, private->guide);
      break;

    case PROP_X:
      g_value_set_double (value, private->x);
      break;
    case PROP_Y:
      g_value_set_double (value, private->y);
      break;
    case PROP_WIDTH:
      g_value_set_double (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_double (value, private->height);
      break;

    case PROP_FIXED_RULE_ACTIVE:
      g_value_set_boolean (value, private->fixed_rule_active);
      break;
    case PROP_FIXED_RULE:
      g_value_set_enum (value, private->fixed_rule);
      break;
    case PROP_DESIRED_FIXED_WIDTH:
      g_value_set_double (value, private->desired_fixed_width);
      break;
    case PROP_DESIRED_FIXED_HEIGHT:
      g_value_set_double (value, private->desired_fixed_height);
      break;
    case PROP_DESIRED_FIXED_SIZE_WIDTH:
      g_value_set_double (value, private->desired_fixed_size_width);
      break;
    case PROP_DESIRED_FIXED_SIZE_HEIGHT:
      g_value_set_double (value, private->desired_fixed_size_height);
      break;
    case PROP_ASPECT_NUMERATOR:
      g_value_set_double (value, private->aspect_numerator);
      break;
    case PROP_ASPECT_DENOMINATOR:
      g_value_set_double (value, private->aspect_denominator);
      break;

    case PROP_FIXED_CENTER:
      g_value_set_boolean (value, private->fixed_center);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tool_rectangle_notify (GObject    *object,
                            GParamSpec *pspec)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (object);
  GimpToolRectanglePrivate *private   = rectangle->private;

  if (G_OBJECT_CLASS (parent_class)->notify)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);

  if (! strcmp (pspec->name, "x1") ||
      ! strcmp (pspec->name, "y1") ||
      ! strcmp (pspec->name, "x2") ||
      ! strcmp (pspec->name, "y2"))
    {
      gimp_tool_rectangle_update_int_rect (rectangle);

      gimp_tool_rectangle_recalculate_center_xy (rectangle);

      gimp_tool_rectangle_update_options (rectangle);
    }
  else if (! strcmp  (pspec->name, "x") &&
           ! PIXEL_FEQUAL (private->x1, private->x))
    {
      gimp_tool_rectangle_synthesize_motion (rectangle,
                                             GIMP_TOOL_RECTANGLE_MOVING,
                                             private->x,
                                             private->y1);
    }
  else if (! strcmp  (pspec->name, "y") &&
           ! PIXEL_FEQUAL (private->y1, private->y))
    {
      gimp_tool_rectangle_synthesize_motion (rectangle,
                                             GIMP_TOOL_RECTANGLE_MOVING,
                                             private->x1,
                                             private->y);
    }
  else if (! strcmp  (pspec->name, "width") &&
           ! PIXEL_FEQUAL (private->x2 - private->x1, private->width))
    {
      /* Calculate x2, y2 that will create a rectangle of given width,
       * for the current options.
       */
      gdouble x2;

      if (private->fixed_center)
        {
          x2 = private->center_x_on_fixed_center +
               private->width / 2;
        }
      else
        {
          x2 = private->x1 + private->width;
        }

      gimp_tool_rectangle_synthesize_motion (rectangle,
                                             GIMP_TOOL_RECTANGLE_RESIZING_RIGHT,
                                             x2,
                                             private->y2);
    }
  else if (! strcmp  (pspec->name, "height") &&
           ! PIXEL_FEQUAL (private->y2 - private->y1, private->height))
    {
      /* Calculate x2, y2 that will create a rectangle of given
       * height, for the current options.
       */
      gdouble y2;

      if (private->fixed_center)
        {
          y2 = private->center_y_on_fixed_center +
               private->height / 2;
        }
      else
        {
          y2 = private->y1 + private->height;
        }

      gimp_tool_rectangle_synthesize_motion (rectangle,
                                             GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM,
                                             private->x2,
                                             y2);
    }
  else if (! strcmp (pspec->name, "desired-fixed-size-width"))
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
      if (private->fixed_rule_active                          &&
#if 0
          tool->button_press_state                            == 0    &&
          tool->active_modifier_state                         == 0    &&
#endif
          FEQUAL (private->desired_fixed_size_width,  height) &&
          FEQUAL (private->desired_fixed_size_height, width))
        {
          gdouble x = private->x1;
          gdouble y = private->y1;

          gimp_tool_rectangle_synthesize_motion (rectangle,
                                                 GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT,
                                                 private->x2,
                                                 private->y2);

          /* For some reason these needs to be set separately... */
          g_object_set (rectangle,
                        "x", x,
                        NULL);
          g_object_set (rectangle,
                        "y", y,
                        NULL);
        }
    }
  else if (! strcmp (pspec->name, "aspect-numerator"))
    {
      /* We are only interested in when numerator and denominator
       * swaps, so it's enough to only check e.g. for numerator.
       */

      double    width             = private->x2 - private->x1;
      double    height            = private->y2 - private->y1;
      gdouble   new_inverse_ratio = private->aspect_denominator /
                                    private->aspect_numerator;
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
      if (private->fixed_rule_active               &&
#if 0
          tool->button_press_state    == 0                 &&
          tool->active_modifier_state == 0                 &&
#endif
          lower_ratio                 <  new_inverse_ratio &&
          higher_ratio                >  new_inverse_ratio)
        {
          gdouble new_x2 = private->x1 + private->y2 - private->y1;
          gdouble new_y2 = private->y1 + private->x2 - private->x1;

          gimp_tool_rectangle_synthesize_motion (rectangle,
                                                 GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT,
                                                 new_x2,
                                                 new_y2);
        }
    }
}

static void
gimp_tool_rectangle_changed (GimpToolWidget *widget)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  GimpDisplayShell         *shell     = gimp_tool_widget_get_shell (widget);
  gdouble                   x1, y1, x2, y2;
  gint                      handle_width;
  gint                      handle_height;
  gint                      i;

  gimp_tool_rectangle_update_handle_sizes (rectangle);

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

  gimp_canvas_rectangle_guides_set (private->guides,
                                    x1, y1,
                                    x2 - x1,
                                    y2 - y1,
                                    private->guide, 4);

  gimp_canvas_rectangle_set (private->rectangle,
                             x1, y1,
                             x2 - x1,
                             y2 - y1);

  if (private->draw_ellipse)
    {
      gimp_canvas_arc_set (private->ellipse,
                           (x1 + x2) / 2.0,
                           (y1 + y2) / 2.0,
                           (x2 - x1) / 2.0,
                           (y2 - y1) / 2.0,
                           0.0, 2 * G_PI);
      gimp_canvas_item_set_visible (private->ellipse, TRUE);
    }
  else
    {
      gimp_canvas_item_set_visible (private->ellipse, FALSE);
    }

  if (private->round_corners && private->corner_radius > 0.0)
    {
      gdouble radius;

      radius = MIN (private->corner_radius,
                    MIN ((x2 - x1) / 2.0, (y2 - y1) / 2.0));

      gimp_canvas_arc_set (private->corners[0],
                           x1 + radius,
                           y1 + radius,
                           radius, radius,
                           G_PI / 2.0, G_PI / 2.0);

      gimp_canvas_arc_set (private->corners[1],
                           x2 - radius,
                           y1 + radius,
                           radius, radius,
                           0.0, G_PI / 2.0);

      gimp_canvas_arc_set (private->corners[2],
                           x1 + radius,
                           y2 - radius,
                           radius, radius,
                           G_PI, G_PI / 2.0);

      gimp_canvas_arc_set (private->corners[3],
                           x2 - radius,
                           y2 - radius,
                           radius, radius,
                           G_PI * 1.5, G_PI / 2.0);

      for (i = 0; i < 4; i++)
        gimp_canvas_item_set_visible (private->corners[i], TRUE);
    }
  else
    {
      for (i = 0; i < 4; i++)
        gimp_canvas_item_set_visible (private->corners[i], FALSE);
    }

  gimp_canvas_item_set_visible (private->center, FALSE);

  for (i = 0; i < 4; i++)
    {
      gimp_canvas_item_set_visible (private->creating_corners[i], FALSE);
      gimp_canvas_item_set_highlight (private->creating_corners[i], FALSE);
    }

  for (i = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT;
       i <= GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM;
       i++)
    {
      gimp_canvas_item_set_visible (private->handles[i], FALSE);
      gimp_canvas_item_set_visible (private->highlight_handles[i], FALSE);
    }

  handle_width  = private->corner_handle_w;
  handle_height = private->corner_handle_h;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_MOVING:
      if (private->rect_adjusting)
        {
          /* Mark the center because we snap to it */
          gimp_canvas_handle_set_position (private->center,
                                           (x1 + x2) / 2.0,
                                           (y1 + y2) / 2.0);
          gimp_canvas_item_set_visible (private->center, TRUE);
          break;
        }

      /* else fallthrough */

    case GIMP_TOOL_RECTANGLE_DEAD:
    case GIMP_TOOL_RECTANGLE_CREATING:
    case GIMP_TOOL_RECTANGLE_AUTO_SHRINK:
      for (i = 0; i < 4; i++)
        {
          gimp_canvas_corner_set (private->creating_corners[i],
                                  x1, y1, x2 - x1, y2 - y1,
                                  handle_width, handle_height,
                                  private->narrow_mode);
          gimp_canvas_item_set_visible (private->creating_corners[i], TRUE);
        }
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      handle_width = private->top_and_bottom_handle_w;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      handle_height = private->left_and_right_handle_h;
      break;

    default:
      break;
    }

  if (handle_width  >= 3                                           &&
      handle_height >= 3                                           &&
      private->function >= GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT &&
      private->function <= GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM)
    {
      GimpCanvasItem *corner;

      if (private->rect_adjusting)
        corner = private->handles[private->function];
      else
        corner = private->highlight_handles[private->function];

      gimp_canvas_corner_set (corner,
                              x1, y1, x2 - x1, y2 - y1,
                              handle_width, handle_height,
                              private->narrow_mode);
      gimp_canvas_item_set_visible (corner, TRUE);
    }

  if (private->highlight && ! private->rect_adjusting)
    {
      GdkRectangle rect;

      rect.x      = x1;
      rect.y      = y1;
      rect.width  = x2 - x1;
      rect.height = y2 - y1;

      gimp_display_shell_set_highlight (shell, &rect, private->highlight_opacity);
    }
  else
    {
      gimp_display_shell_set_highlight (shell, NULL, 0.0);
    }
}

gint
gimp_tool_rectangle_button_press (GimpToolWidget      *widget,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  gdouble                   snapped_x, snapped_y;
  gint                      snap_x, snap_y;

  /* prevent the latching of toggled modifiers (Ctrl and Shift) when the
     selection is cancelled whilst one or both of these modifiers are
     being pressed  (see issue #7954 and MR !779) */
  private->fixed_center_copy = private->fixed_center;
  private->fixed_rule_active_copy = private->fixed_rule_active;
  private->modifier_toggle_allowed = TRUE;


  /* save existing shape in case of cancellation */
  private->saved_x1 = private->x1;
  private->saved_y1 = private->y1;
  private->saved_x2 = private->x2;
  private->saved_y2 = private->y2;

  gimp_tool_rectangle_setup_snap_offsets (rectangle, coords);
  gimp_tool_widget_get_snap_offsets (widget, &snap_x, &snap_y, NULL, NULL);

  snapped_x = coords->x + snap_x;
  snapped_y = coords->y + snap_y;

  private->lastx = snapped_x;
  private->lasty = snapped_y;

  if (private->function == GIMP_TOOL_RECTANGLE_CREATING)
    {
      /* Remember that this rectangle was created from scratch. */
      private->is_new = TRUE;

      private->x1 = private->x2 = snapped_x;
      private->y1 = private->y2 = snapped_y;

      /* Unless forced, created rectangles should not be started in
       * narrow-mode
       */
      if (private->force_narrow_mode)
        private->narrow_mode = TRUE;
      else
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
       * immediate "other side" for that.
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

      gimp_tool_rectangle_get_other_side_coord (rectangle,
                                                &private->other_side_x,
                                                &private->other_side_y);
    }

  gimp_tool_rectangle_update_int_rect (rectangle);

  /* Is the rectangle being rubber-banded? */
  private->rect_adjusting = gimp_tool_rectangle_rect_adjusting_func (rectangle);

  gimp_tool_rectangle_changed (widget);

  gimp_tool_rectangle_update_status (rectangle);

  return 1;
}

void
gimp_tool_rectangle_button_release (GimpToolWidget        *widget,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  gint                      response  = 0;

  gimp_tool_widget_set_status (widget, NULL);

  /* On button release, we are not rubber-banding the rectangle any longer. */
  private->rect_adjusting = FALSE;

  gimp_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);

 /* prevent the latching of toggled modifiers (Ctrl and Shift) when the
    selection is cancelled whilst one or both of these modifiers are
    being pressed  (see issue #7954 and MR !779) */
  private->modifier_toggle_allowed = FALSE;
  g_object_set (rectangle,
                "fixed-center", private->fixed_center_copy,
                NULL);
  g_object_set (rectangle,
                "fixed-rule-active", private->fixed_rule_active_copy,
                NULL);

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_NO_MOTION:
      /* If the first created rectangle was not expanded, halt the
       * tool...
       */
      if (gimp_tool_rectangle_rectangle_is_first (rectangle))
        {
          response = GIMP_TOOL_WIDGET_RESPONSE_CANCEL;
          break;
        }

      /* ...else fallthrough and treat a long click without movement
       * like a normal change
       */

    case GIMP_BUTTON_RELEASE_NORMAL:
      /* If a normal click-drag-release actually created a rectangle
       * with content...
       */
      if (private->x1 != private->x2 &&
          private->y1 != private->y2)
        {
          gimp_tool_rectangle_change_complete (rectangle);
          break;
        }

      /* ...else fallthrough and undo the operation, we can't have
       * zero-extent rectangles
       */

    case GIMP_BUTTON_RELEASE_CANCEL:
      private->x1 = private->saved_x1;
      private->y1 = private->saved_y1;
      private->x2 = private->saved_x2;
      private->y2 = private->saved_y2;

      gimp_tool_rectangle_update_int_rect (rectangle);

      /* If the first created rectangle was canceled, halt the tool */
      if (gimp_tool_rectangle_rectangle_is_first (rectangle))
        response = GIMP_TOOL_WIDGET_RESPONSE_CANCEL;
      break;

    case GIMP_BUTTON_RELEASE_CLICK:
      /* When a dead area is clicked, don't execute. */
      if (private->function != GIMP_TOOL_RECTANGLE_DEAD)
        response = GIMP_TOOL_WIDGET_RESPONSE_CONFIRM;
      break;
    }

  /* We must update this. */
  gimp_tool_rectangle_recalculate_center_xy (rectangle);

  gimp_tool_rectangle_update_options (rectangle);

  gimp_tool_rectangle_changed (widget);

  private->is_first = FALSE;

  /*  emit response at the end, so everything is up to date even if
   *  a signal handler decides hot to shut down the rectangle
   */
  if (response != 0)
    gimp_tool_widget_response (widget, response);
}

void
gimp_tool_rectangle_motion (GimpToolWidget   *widget,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  gdouble                   snapped_x;
  gdouble                   snapped_y;
  gint                      snap_x, snap_y;

  /* Motion events should be ignored when we're just waiting for the
   * button release event to execute or if the user has grabbed a dead
   * area of the rectangle.
   */
  if (private->function == GIMP_TOOL_RECTANGLE_EXECUTING ||
      private->function == GIMP_TOOL_RECTANGLE_DEAD)
    return;

  /* Handle snapping. */
  gimp_tool_widget_get_snap_offsets (widget, &snap_x, &snap_y, NULL, NULL);

  snapped_x = coords->x + snap_x;
  snapped_y = coords->y + snap_y;

  /* This is the core rectangle shape updating function: */
  gimp_tool_rectangle_update_with_coord (rectangle, snapped_x, snapped_y);

  gimp_tool_rectangle_update_status (rectangle);

  if (private->function == GIMP_TOOL_RECTANGLE_CREATING)
    {
      GimpRectangleFunction function = GIMP_TOOL_RECTANGLE_CREATING;
      gdouble               dx       = snapped_x - private->lastx;
      gdouble               dy       = snapped_y - private->lasty;

      /* When the user starts to move the cursor, set the current
       * function to one of the corner-grabbed functions, depending on
       * in what direction the user starts dragging the rectangle.
       */
      if (dx < 0)
        {
          function = (dy < 0 ?
                      GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT :
                      GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT);
        }
      else if (dx > 0)
        {
          function = (dy < 0 ?
                      GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT :
                      GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT);
        }
      else if (dy < 0)
        {
          function = (dx < 0 ?
                      GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT :
                      GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT);
        }
      else if (dy > 0)
        {
          function = (dx < 0 ?
                      GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT :
                      GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT);
        }

      gimp_tool_rectangle_set_function (rectangle, function);

      if (private->fixed_rule_active &&
          private->fixed_rule == GIMP_RECTANGLE_FIXED_SIZE)
        {
          /* For fixed size, set the function to moving immediately since the
           * rectangle can not be resized anyway.
           */

          /* We fake a coord update to get the right size. */
          gimp_tool_rectangle_update_with_coord (rectangle,
                                                 snapped_x,
                                                 snapped_y);

          gimp_tool_widget_set_snap_offsets (widget,
                                             -(private->x2 - private->x1) / 2,
                                             -(private->y2 - private->y1) / 2,
                                             private->x2 - private->x1,
                                             private->y2 - private->y1);

          gimp_tool_rectangle_set_function (rectangle,
                                            GIMP_TOOL_RECTANGLE_MOVING);
        }
    }

  gimp_tool_rectangle_update_options (rectangle);

  private->lastx = snapped_x;
  private->lasty = snapped_y;
}

GimpHit
gimp_tool_rectangle_hit (GimpToolWidget   *widget,
                         const GimpCoords *coords,
                         GdkModifierType   state,
                         gboolean          proximity)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  GimpRectangleFunction     function;

  if (private->suppress_updates)
    {
      function = gimp_tool_rectangle_get_function (rectangle);
    }
  else
    {
      function = gimp_tool_rectangle_calc_function (rectangle,
                                                    coords, proximity);
    }

  switch (function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      return GIMP_HIT_DIRECT;

    case GIMP_TOOL_RECTANGLE_CREATING:
    case GIMP_TOOL_RECTANGLE_MOVING:
      return GIMP_HIT_INDIRECT;

    case GIMP_TOOL_RECTANGLE_DEAD:
    case GIMP_TOOL_RECTANGLE_AUTO_SHRINK:
    case GIMP_TOOL_RECTANGLE_EXECUTING:
    default:
      return GIMP_HIT_NONE;
    }
}

void
gimp_tool_rectangle_hover (GimpToolWidget   *widget,
                           const GimpCoords *coords,
                           GdkModifierType   state,
                           gboolean          proximity)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  GimpRectangleFunction     function;

  if (private->suppress_updates)
    {
      private->suppress_updates--;
      return;
    }

  function = gimp_tool_rectangle_calc_function (rectangle, coords, proximity);

  gimp_tool_rectangle_set_function (rectangle, function);
}

static void
gimp_tool_rectangle_leave_notify (GimpToolWidget *widget)
{
  GimpToolRectangle *rectangle = GIMP_TOOL_RECTANGLE (widget);

  gimp_tool_rectangle_set_function (rectangle, GIMP_TOOL_RECTANGLE_DEAD);

  GIMP_TOOL_WIDGET_CLASS (parent_class)->leave_notify (widget);
}

static gboolean
gimp_tool_rectangle_key_press (GimpToolWidget *widget,
                               GdkEventKey    *kevent)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  gint                      dx        = 0;
  gint                      dy        = 0;
  gdouble                   new_x     = 0;
  gdouble                   new_y     = 0;

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

    default:
      return GIMP_TOOL_WIDGET_CLASS (parent_class)->key_press (widget, kevent);
    }

  /*  If the shift key is down, move by an accelerated increment  */
  if (kevent->state & gimp_get_extend_selection_mask ())
    {
      dx *= ARROW_VELOCITY;
      dy *= ARROW_VELOCITY;
    }

  gimp_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);

  /*  Resize the rectangle if the mouse is over a handle, otherwise move it  */
  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_MOVING:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y1 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
      new_x = private->x1 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
      new_x = private->x2 + dx;
      new_y = private->y2 + dy;
      private->lastx = new_x;
      private->lasty = new_y;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      new_x = private->x1 + dx;
      private->lastx = new_x;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      new_x = private->x2 + dx;
      private->lastx = new_x;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      new_y = private->y1 + dy;
      private->lasty = new_y;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      new_y = private->y2 + dy;
      private->lasty = new_y;
      break;

    default:
      return TRUE;
    }

  gimp_tool_rectangle_update_with_coord (rectangle, new_x, new_y);

  gimp_tool_rectangle_recalculate_center_xy (rectangle);

  gimp_tool_rectangle_update_options (rectangle);

  gimp_tool_rectangle_change_complete (rectangle);

  /*  Evil hack to suppress oper updates. We do this because we don't
   *  want the rectangle tool to change function while the rectangle
   *  is being resized or moved using the keyboard.
   */
  private->suppress_updates = 2;

  return TRUE;
}

static void
gimp_tool_rectangle_motion_modifier (GimpToolWidget  *widget,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;
  gboolean                  button1_down;

  button1_down = (state & GDK_BUTTON1_MASK);

  if (key == gimp_get_extend_selection_mask ())
    {
#if 0
      /* Here we want to handle manually when to update the rectangle, so we
       * don't want gimp_tool_rectangle_options_notify to do anything.
       */
      g_signal_handlers_block_by_func (options,
                                       gimp_tool_rectangle_options_notify,
                                       rectangle);
#endif
      if (private->modifier_toggle_allowed)
        g_object_set (rectangle,
                      "fixed-rule-active", ! private->fixed_rule_active,
                      NULL);

#if 0
      g_signal_handlers_unblock_by_func (options,
                                         gimp_tool_rectangle_options_notify,
                                         rectangle);
#endif

      /* Only change the shape if the mouse is still down (i.e. the user is
       * still editing the rectangle.
       */
      if (button1_down)
        {
          if (! private->fixed_rule_active)
            {
              /* Reset anchor point */
              gimp_tool_rectangle_set_other_side_coord (rectangle,
                                                        private->other_side_x,
                                                        private->other_side_y);
            }

          gimp_tool_rectangle_update_with_coord (rectangle,
                                                 private->lastx,
                                                 private->lasty);
        }
    }

  if (key == gimp_get_toggle_behavior_mask ())
    {
      if (private->modifier_toggle_allowed)
        g_object_set (rectangle,
                      "fixed-center", ! private->fixed_center,
                      NULL);

      if (private->fixed_center)
        {
          gimp_tool_rectangle_update_with_coord (rectangle,
                                                 private->lastx,
                                                 private->lasty);

          /* Only emit the rectangle-changed signal if the button is
           * not down. If it is down, the signal will and shall be
           * emitted on _button_release instead.
           */
          if (! button1_down)
            {
              gimp_tool_rectangle_change_complete (rectangle);
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
          gimp_tool_rectangle_set_other_side_coord (rectangle,
                                                    private->other_side_x,
                                                    private->other_side_y);
        }
    }

  gimp_tool_rectangle_update_options (rectangle);
}

static gboolean
gimp_tool_rectangle_get_cursor (GimpToolWidget     *widget,
                                const GimpCoords   *coords,
                                GdkModifierType     state,
                                GimpCursorType     *cursor,
                                GimpToolCursorType *tool_cursor,
                                GimpCursorModifier *modifier)
{
  GimpToolRectangle        *rectangle = GIMP_TOOL_RECTANGLE (widget);
  GimpToolRectanglePrivate *private   = rectangle->private;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_CREATING:
      *cursor = GIMP_CURSOR_CROSSHAIR_SMALL;
      break;
    case GIMP_TOOL_RECTANGLE_MOVING:
      *cursor   = GIMP_CURSOR_MOVE;
      *modifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
      *cursor = GIMP_CURSOR_CORNER_TOP_LEFT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
      *cursor = GIMP_CURSOR_CORNER_TOP_RIGHT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
      *cursor = GIMP_CURSOR_CORNER_BOTTOM_LEFT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
      *cursor = GIMP_CURSOR_CORNER_BOTTOM_RIGHT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      *cursor = GIMP_CURSOR_SIDE_LEFT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      *cursor = GIMP_CURSOR_SIDE_RIGHT;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      *cursor = GIMP_CURSOR_SIDE_TOP;
      break;
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      *cursor = GIMP_CURSOR_SIDE_BOTTOM;
      break;

    default:
      return FALSE;
    }

  return TRUE;
}

static void
gimp_tool_rectangle_change_complete (GimpToolRectangle *rectangle)
{
  g_signal_emit (rectangle, rectangle_signals[CHANGE_COMPLETE], 0);
}

static void
gimp_tool_rectangle_update_options (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gdouble                   x1      = 0;
  gdouble                   y1      = 0;
  gdouble                   x2      = 0;
  gdouble                   y2      = 0;

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

#if 0
  g_signal_handlers_block_by_func (options,
                                   gimp_tool_rectangle_options_notify,
                                   rect_tool);
#endif

  g_object_freeze_notify (G_OBJECT (rectangle));

  if (! FEQUAL (private->x, x1))
    g_object_set (rectangle, "x", x1, NULL);

  if (! FEQUAL (private->y, y1))
    g_object_set (rectangle, "y", y1, NULL);

  if (! FEQUAL (private->width, x2 - x1))
    g_object_set (rectangle, "width", x2 - x1, NULL);

  if (! FEQUAL (private->height, y2 - y1))
    g_object_set (rectangle, "height", y2 - y1, NULL);

  g_object_thaw_notify (G_OBJECT (rectangle));

#if 0
  g_signal_handlers_unblock_by_func (options,
                                     gimp_tool_rectangle_options_notify,
                                     rect_tool);
#endif
}

static void
gimp_tool_rectangle_update_handle_sizes (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  GimpDisplayShell         *shell;
  gint                      visible_rectangle_width;
  gint                      visible_rectangle_height;
  gint                      rectangle_width;
  gint                      rectangle_height;
  gdouble                   pub_x1, pub_y1;
  gdouble                   pub_x2, pub_y2;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));

  gimp_tool_rectangle_get_public_rect (rectangle,
                                       &pub_x1, &pub_y1, &pub_x2, &pub_y2);
  {
    /* Calculate rectangles of the selection rectangle and the display
     * shell, with origin at (0, 0) of image, and in screen coordinate
     * scale.
     */
    gint x1 =  pub_x1 * shell->scale_x;
    gint y1 =  pub_y1 * shell->scale_y;
    gint w1 = (pub_x2 - pub_x1) * shell->scale_x;
    gint h1 = (pub_y2 - pub_y1) * shell->scale_y;

    gint x2, y2, w2, h2;

    gimp_display_shell_scroll_get_scaled_viewport (shell, &x2, &y2, &w2, &h2);

    rectangle_width  = w1;
    rectangle_height = h1;

    /* Handle size calculations shall be based on the visible part of
     * the rectangle, so calculate the size for the visible rectangle
     * by intersecting with the viewport rectangle.
     */
    gimp_rectangle_intersect (x1, y1,
                              w1, h1,
                              x2, y2,
                              w2, h2,
                              NULL, NULL,
                              &visible_rectangle_width,
                              &visible_rectangle_height);

    /* Determine if we are in narrow-mode or not. */
    if (private->force_narrow_mode)
      private->narrow_mode = TRUE;
    else
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
}

static void
gimp_tool_rectangle_update_status (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gdouble                   x1      = 0;
  gdouble                   y1      = 0;
  gdouble                   x2      = 0;
  gdouble                   y2      = 0;

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

  if (private->function == GIMP_TOOL_RECTANGLE_MOVING)
    {
      gimp_tool_widget_set_status_coords (GIMP_TOOL_WIDGET (rectangle),
                                          _("Position: "),
                                          x1, ", ", y1,
                                          NULL);
    }
  else
    {
      gchar *aspect_text = NULL;
      gint   width       = x2 - x1;
      gint   height      = y2 - y1;

      if (width > 0.0 && height > 0.0)
        {
          aspect_text = g_strdup_printf ("  (%.2f:1)",
                                         (gdouble) width / (gdouble) height);
        }

      gimp_tool_widget_set_status_coords (GIMP_TOOL_WIDGET (rectangle),
                                          private->status_title,
                                          width, " × ", height,
                                          aspect_text);
      g_free (aspect_text);
    }
}

static void
gimp_tool_rectangle_synthesize_motion (GimpToolRectangle *rectangle,
                                       gint               function,
                                       gdouble            new_x,
                                       gdouble            new_y)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  GimpRectangleFunction     old_function;

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
  if (private->rect_adjusting)
    return;

  old_function = private->function;

  gimp_tool_rectangle_set_function (rectangle, function);

  gimp_tool_rectangle_update_with_coord (rectangle, new_x, new_y);

  /* We must update this. */
  gimp_tool_rectangle_recalculate_center_xy (rectangle);

  gimp_tool_rectangle_update_options (rectangle);

  gimp_tool_rectangle_set_function (rectangle, old_function);

  gimp_tool_rectangle_change_complete (rectangle);
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

static GimpRectangleFunction
gimp_tool_rectangle_calc_function (GimpToolRectangle *rectangle,
                                   const GimpCoords  *coords,
                                   gboolean           proximity)
{
  if (! proximity)
    {
      return GIMP_TOOL_RECTANGLE_DEAD;
    }
  else if (gimp_tool_rectangle_coord_outside (rectangle, coords))
    {
      /* The cursor is outside of the rectangle, clicking should
       * create a new rectangle.
       */
      return GIMP_TOOL_RECTANGLE_CREATING;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_NORTH_WEST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH_EAST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT;
    }
  else if  (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                 coords,
                                                 GIMP_HANDLE_ANCHOR_NORTH_EAST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH_WEST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_WEST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_LEFT;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_EAST))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_RIGHT;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_NORTH))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_TOP;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_SOUTH))
    {
      return GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM;
    }
  else if (gimp_tool_rectangle_coord_on_handle (rectangle,
                                                coords,
                                                GIMP_HANDLE_ANCHOR_CENTER))
    {
      return GIMP_TOOL_RECTANGLE_MOVING;
    }
  else
    {
      return GIMP_TOOL_RECTANGLE_DEAD;
    }
}

/* gimp_tool_rectangle_check_function() is needed to deal with
 * situations where the user drags a corner or edge across one of the
 * existing edges, thereby changing its function.  Ugh.
 */
static void
gimp_tool_rectangle_check_function (GimpToolRectangle *rectangle)

{
  GimpToolRectanglePrivate *private  = rectangle->private;
  GimpRectangleFunction     function = private->function;

  if (private->x2 < private->x1)
    {
      swap_doubles (&private->x1, &private->x2);

      switch (function)
        {
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_RIGHT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_LEFT;
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
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
          function = GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
          function = GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM;
          break;
        case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
          function = GIMP_TOOL_RECTANGLE_RESIZING_TOP;
          break;
        default:
          break;
        }
    }

  gimp_tool_rectangle_set_function (rectangle, function);
}

/**
 * gimp_tool_rectangle_coord_outside:
 *
 * Returns: %TRUE if the coord is outside the rectangle bounds
 *          including any outside handles.
 */
static gboolean
gimp_tool_rectangle_coord_outside (GimpToolRectangle *rectangle,
                                   const GimpCoords  *coord)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  GimpDisplayShell         *shell;
  gboolean                  narrow_mode = private->narrow_mode;
  gdouble                   x1, y1, x2, y2;
  gdouble                   x1_b, y1_b, x2_b, y2_b;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

  x1_b = x1 - (narrow_mode ? private->corner_handle_w / shell->scale_x : 0);
  x2_b = x2 + (narrow_mode ? private->corner_handle_w / shell->scale_x : 0);
  y1_b = y1 - (narrow_mode ? private->corner_handle_h / shell->scale_y : 0);
  y2_b = y2 + (narrow_mode ? private->corner_handle_h / shell->scale_y : 0);

  return (coord->x < x1_b ||
          coord->x > x2_b ||
          coord->y < y1_b ||
          coord->y > y2_b);
}

/**
 * gimp_tool_rectangle_coord_on_handle:
 *
 * Returns: %TRUE if the coord is on the handle that corresponds to
 *          @anchor.
 */
static gboolean
gimp_tool_rectangle_coord_on_handle (GimpToolRectangle *rectangle,
                                     const GimpCoords  *coords,
                                     GimpHandleAnchor   anchor)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  GimpDisplayShell         *shell;
  gdouble                   x1 = 0;
  gdouble                   y1 = 0;
  gdouble                   x2 = 0;
  gdouble                   y2 = 0;
  gdouble                   rect_w, rect_h;
  gdouble                   handle_x          = 0;
  gdouble                   handle_y          = 0;
  gdouble                   handle_width      = 0;
  gdouble                   handle_height     = 0;
  gint                      narrow_mode_x_dir = 0;
  gint                      narrow_mode_y_dir = 0;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

  rect_w = x2 - x1;
  rect_h = y2 - y1;

  switch (anchor)
    {
    case GIMP_HANDLE_ANCHOR_NORTH_WEST:
      handle_x      = x1;
      handle_y      = y1;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_EAST:
      handle_x      = x2;
      handle_y      = y2;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH_EAST:
      handle_x      = x2;
      handle_y      = y1;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH_WEST:
      handle_x      = x1;
      handle_y      = y2;
      handle_width  = private->corner_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_WEST:
      handle_x      = x1;
      handle_y      = y1 + rect_h / 2;
      handle_width  = private->corner_handle_w;
      handle_height = private->left_and_right_handle_h;

      narrow_mode_x_dir = -1;
      narrow_mode_y_dir =  0;
      break;

    case GIMP_HANDLE_ANCHOR_EAST:
      handle_x      = x2;
      handle_y      = y1 + rect_h / 2;
      handle_width  = private->corner_handle_w;
      handle_height = private->left_and_right_handle_h;

      narrow_mode_x_dir =  1;
      narrow_mode_y_dir =  0;
      break;

    case GIMP_HANDLE_ANCHOR_NORTH:
      handle_x      = x1 + rect_w / 2;
      handle_y      = y1;
      handle_width  = private->top_and_bottom_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  0;
      narrow_mode_y_dir = -1;
      break;

    case GIMP_HANDLE_ANCHOR_SOUTH:
      handle_x      = x1 + rect_w / 2;
      handle_y      = y2;
      handle_width  = private->top_and_bottom_handle_w;
      handle_height = private->corner_handle_h;

      narrow_mode_x_dir =  0;
      narrow_mode_y_dir =  1;
      break;

    case GIMP_HANDLE_ANCHOR_CENTER:
      handle_x      = x1 + rect_w / 2;
      handle_y      = y1 + rect_h / 2;

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

  return gimp_canvas_item_on_handle (private->rectangle,
                                     coords->x, coords->y,
                                     GIMP_HANDLE_SQUARE,
                                     handle_x,     handle_y,
                                     handle_width, handle_height,
                                     anchor);
}

static GimpHandleAnchor
gimp_tool_rectangle_get_anchor (GimpRectangleFunction function)
{
  switch (function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
      return GIMP_HANDLE_ANCHOR_NORTH_WEST;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
      return GIMP_HANDLE_ANCHOR_NORTH_EAST;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
      return GIMP_HANDLE_ANCHOR_SOUTH_WEST;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
      return GIMP_HANDLE_ANCHOR_SOUTH_EAST;

    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      return GIMP_HANDLE_ANCHOR_WEST;

    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      return GIMP_HANDLE_ANCHOR_EAST;

    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      return GIMP_HANDLE_ANCHOR_NORTH;

    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      return GIMP_HANDLE_ANCHOR_SOUTH;

    default:
      return GIMP_HANDLE_ANCHOR_CENTER;
    }
}

static gboolean
gimp_tool_rectangle_rect_rubber_banding_func (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_CREATING:
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_AUTO_SHRINK:
      return TRUE;

    case GIMP_TOOL_RECTANGLE_MOVING:
    case GIMP_TOOL_RECTANGLE_DEAD:
    default:
      break;
    }

  return FALSE;
}

/**
 * gimp_tool_rectangle_rect_adjusting_func:
 * @rectangle:
 *
 * Returns: %TRUE if the current function is a rectangle adjusting
 *          function.
 */
static gboolean
gimp_tool_rectangle_rect_adjusting_func (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  return (gimp_tool_rectangle_rect_rubber_banding_func (rectangle) ||
          private->function == GIMP_TOOL_RECTANGLE_MOVING);
}

/**
 * gimp_tool_rectangle_get_other_side:
 * @rectangle: A #GimpToolRectangle.
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
gimp_tool_rectangle_get_other_side (GimpToolRectangle  *rectangle,
                                    gdouble           **other_x,
                                    gdouble           **other_y)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      *other_x = &private->x1;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      *other_x = &private->x2;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
    default:
      *other_x = NULL;
      break;
    }

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      *other_y = &private->y1;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      *other_y = &private->y2;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
    default:
      *other_y = NULL;
      break;
    }
}

static void
gimp_tool_rectangle_get_other_side_coord (GimpToolRectangle *rectangle,
                                          gdouble           *other_side_x,
                                          gdouble           *other_side_y)
{
  gdouble *other_x = NULL;
  gdouble *other_y = NULL;

  gimp_tool_rectangle_get_other_side (rectangle, &other_x, &other_y);

  if (other_x)
    *other_side_x = *other_x;
  if (other_y)
    *other_side_y = *other_y;
}

static void
gimp_tool_rectangle_set_other_side_coord (GimpToolRectangle *rectangle,
                                          gdouble            other_side_x,
                                          gdouble            other_side_y)
{
  gdouble *other_x = NULL;
  gdouble *other_y = NULL;

  gimp_tool_rectangle_get_other_side (rectangle, &other_x, &other_y);

  if (other_x)
    *other_x = other_side_x;
  if (other_y)
    *other_y = other_side_y;

  gimp_tool_rectangle_check_function (rectangle);

  gimp_tool_rectangle_update_int_rect (rectangle);
}

/**
 * gimp_tool_rectangle_apply_coord:
 * @param:     A #GimpToolRectangle.
 * @coord_x:   X of coord.
 * @coord_y:   Y of coord.
 *
 * Adjust the rectangle to the new position specified by passed
 * coordinate, taking fixed_center into account, which means it
 * expands the rectangle around the center point.
 */
static void
gimp_tool_rectangle_apply_coord (GimpToolRectangle *rectangle,
                                 gdouble            coord_x,
                                 gdouble            coord_y)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  if (private->function == GIMP_TOOL_RECTANGLE_MOVING)
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
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      private->x1 = coord_x;

      if (private->fixed_center)
        private->x2 = 2 * private->center_x_on_fixed_center - private->x1;

      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      private->x2 = coord_x;

      if (private->fixed_center)
        private->x1 = 2 * private->center_x_on_fixed_center - private->x2;

      break;

    default:
      break;
    }

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      private->y1 = coord_y;

      if (private->fixed_center)
        private->y2 = 2 * private->center_y_on_fixed_center - private->y1;

      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      private->y2 = coord_y;

      if (private->fixed_center)
        private->y1 = 2 * private->center_y_on_fixed_center - private->y2;

      break;

    default:
      break;
    }
}

static void
gimp_tool_rectangle_setup_snap_offsets (GimpToolRectangle *rectangle,
                                        const GimpCoords  *coords)
{
  GimpToolWidget           *widget  = GIMP_TOOL_WIDGET (rectangle);
  GimpToolRectanglePrivate *private = rectangle->private;
  gdouble                   x1      = 0;
  gdouble                   y1      = 0;
  gdouble                   x2      = 0;
  gdouble                   y2      = 0;
  gdouble                   coord_x, coord_y;

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);
  gimp_tool_rectangle_adjust_coord (rectangle,
                                    coords->x, coords->y,
                                    &coord_x, &coord_y);

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_CREATING:
      gimp_tool_widget_set_snap_offsets (widget, 0, 0, 0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x1 - coord_x,
                                         y1 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x2 - coord_x,
                                         y1 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x1 - coord_x,
                                         y2 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x2 - coord_x,
                                         y2 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x1 - coord_x, 0,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x2 - coord_x, 0,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      gimp_tool_widget_set_snap_offsets (widget,
                                         0, y1 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      gimp_tool_widget_set_snap_offsets (widget,
                                         0, y2 - coord_y,
                                         0, 0);
      break;

    case GIMP_TOOL_RECTANGLE_MOVING:
      gimp_tool_widget_set_snap_offsets (widget,
                                         x1 - coord_x,
                                         y1 - coord_y,
                                         x2 - x1,
                                         y2 - y1);
      break;

    default:
      break;
    }
}

/**
 * gimp_tool_rectangle_clamp:
 * @rectangle:      A #GimpToolRectangle.
 * @clamped_sides:  Where to put contrainment information.
 * @constraint:     Constraint to use.
 * @symmetrically:  Whether or not to clamp symmetrically.
 *
 * Clamps rectangle inside specified bounds, providing information of
 * where clamping was done. Can also clamp symmetrically.
 */
static void
gimp_tool_rectangle_clamp (GimpToolRectangle       *rectangle,
                           ClampedSide             *clamped_sides,
                           GimpRectangleConstraint  constraint,
                           gboolean                 symmetrically)
{
  gimp_tool_rectangle_clamp_width (rectangle,
                                   clamped_sides,
                                   constraint,
                                   symmetrically);

  gimp_tool_rectangle_clamp_height (rectangle,
                                    clamped_sides,
                                    constraint,
                                    symmetrically);
}

/**
 * gimp_tool_rectangle_clamp_width:
 * @rectangle:      A #GimpToolRectangle.
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
gimp_tool_rectangle_clamp_width (GimpToolRectangle       *rectangle,
                                 ClampedSide             *clamped_sides,
                                 GimpRectangleConstraint  constraint,
                                 gboolean                 symmetrically)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gint                      min_x;
  gint                      max_x;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_tool_rectangle_get_constraints (rectangle,
                                       &min_x, NULL,
                                       &max_x, NULL,
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
 * gimp_tool_rectangle_clamp_height:
 * @rectangle:      A #GimpToolRectangle.
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
gimp_tool_rectangle_clamp_height (GimpToolRectangle       *rectangle,
                                  ClampedSide             *clamped_sides,
                                  GimpRectangleConstraint  constraint,
                                  gboolean                 symmetrically)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gint                      min_y;
  gint                      max_y;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_tool_rectangle_get_constraints (rectangle,
                                       NULL, &min_y,
                                       NULL, &max_y,
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
 * gimp_tool_rectangle_keep_inside:
 * @rectangle: A #GimpToolRectangle.
 *
 * If the rectangle is outside of the canvas, move it into it. If the rectangle is
 * larger than the canvas in any direction, make it fill the canvas in that direction.
 */
static void
gimp_tool_rectangle_keep_inside (GimpToolRectangle      *rectangle,
                                 GimpRectangleConstraint constraint)
{
  gimp_tool_rectangle_keep_inside_horizontally (rectangle, constraint);
  gimp_tool_rectangle_keep_inside_vertically   (rectangle, constraint);
}

/**
 * gimp_tool_rectangle_keep_inside_horizontally:
 * @rectangle:      A #GimpToolRectangle.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint horizontally, move it
 * inside. If it is too big to fit inside, make it just as big as the width
 * limit.
 */
static void
gimp_tool_rectangle_keep_inside_horizontally (GimpToolRectangle       *rectangle,
                                              GimpRectangleConstraint  constraint)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gint                      min_x;
  gint                      max_x;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_tool_rectangle_get_constraints (rectangle,
                                       &min_x, NULL,
                                       &max_x, NULL,
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
 * gimp_tool_rectangle_keep_inside_vertically:
 * @rectangle:      A #GimpToolRectangle.
 * @constraint:     Constraint to use.
 *
 * If the rectangle is outside of the given constraint vertically,
 * move it inside. If it is too big to fit inside, make it just as big
 * as the width limit.
 */
static void
gimp_tool_rectangle_keep_inside_vertically (GimpToolRectangle       *rectangle,
                                            GimpRectangleConstraint  constraint)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gint                      min_y;
  gint                      max_y;

  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  gimp_tool_rectangle_get_constraints (rectangle,
                                       NULL, &min_y,
                                       NULL, &max_y,
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
 * gimp_tool_rectangle_apply_fixed_width:
 * @rectangle:      A #GimpToolRectangle.
 * @constraint:     Constraint to use.
 * @width:
 *
 * Makes the rectangle have a fixed_width, following the constrainment
 * rules of fixed widths as well. Please refer to the rectangle tools
 * spec.
 */
static void
gimp_tool_rectangle_apply_fixed_width (GimpToolRectangle      *rectangle,
                                       GimpRectangleConstraint constraint,
                                       gdouble                 width)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    width / 2;
      private->x2 = private->x1 + width;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
      /* We always want to center around fixed_center here, since we want the
       * anchor point to be directly on the opposite side.
       */
      private->x1 = private->center_x_on_fixed_center -
                    width / 2;
      private->x2 = private->x1 + width;
      break;

    default:
      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_tool_rectangle_keep_inside_horizontally (rectangle, constraint);
}

/**
 * gimp_tool_rectangle_apply_fixed_height:
 * @rectangle:      A #GimpToolRectangle.
 * @constraint:     Constraint to use.
 * @height:
 *
 * Makes the rectangle have a fixed_height, following the
 * constrainment rules of fixed heights as well. Please refer to the
 * rectangle tools spec.
 */
static void
gimp_tool_rectangle_apply_fixed_height (GimpToolRectangle      *rectangle,
                                        GimpRectangleConstraint constraint,
                                        gdouble                 height)

{
  GimpToolRectanglePrivate *private = rectangle->private;

  switch (private->function)
    {
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
      /* We always want to center around fixed_center here, since we
       * want the anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    height / 2;
      private->y2 = private->y1 + height;
      break;

    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
    case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
    case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
      /* We always want to center around fixed_center here, since we
       * want the anchor point to be directly on the opposite side.
       */
      private->y1 = private->center_y_on_fixed_center -
                    height / 2;
      private->y2 = private->y1 + height;
      break;

    default:
      break;
    }

  /* Width shall be kept even after constraints, so we move the
   * rectangle sideways rather than adjusting a side.
   */
  gimp_tool_rectangle_keep_inside_vertically (rectangle, constraint);
}

/**
 * gimp_tool_rectangle_apply_aspect:
 * @rectangle:      A #GimpToolRectangle.
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
gimp_tool_rectangle_apply_aspect (GimpToolRectangle *rectangle,
                                  gdouble            aspect,
                                  gint               clamped_sides)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  gdouble                   current_w;
  gdouble                   current_h;
  gdouble                   current_aspect;
  SideToResize              side_to_resize = SIDE_TO_RESIZE_NONE;

  current_w = private->x2 - private->x1;
  current_h = private->y2 - private->y1;

  current_aspect = (gdouble) current_w / (gdouble) current_h;

  /* Do we have to do anything? */
  if (current_aspect == aspect)
    return;

  if (private->fixed_center)
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
            case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
            case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
            case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
              if (! (clamped_sides & CLAMPED_TOP) &&
                  ! (clamped_sides & CLAMPED_BOTTOM))
                {
                  side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
                }
              else
                {
                  side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
                }
              break;

            case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
            case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
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
            case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
            case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
            case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
            case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
              if (! (clamped_sides & CLAMPED_LEFT) &&
                  ! (clamped_sides & CLAMPED_RIGHT))
                {
                  side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
                }
              else
                {
                  side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
                }
              break;

            case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
            case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
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
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
          if (! (clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
          if (! (clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
          if (! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
          if (! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
          if (! (clamped_sides & CLAMPED_TOP) &&
              ! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
          if (! (clamped_sides & CLAMPED_TOP) &&
              ! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
        case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
          side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          break;

        case GIMP_TOOL_RECTANGLE_MOVING:
        default:
          if (! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else if (! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else if (! (clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else if (! (clamped_sides & CLAMPED_LEFT))
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
        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_LEFT:
          if (! (clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_UPPER_RIGHT:
          if (! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_LEFT:
          if (! (clamped_sides & CLAMPED_LEFT))
            side_to_resize = SIDE_TO_RESIZE_LEFT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LOWER_RIGHT:
          if (! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_TOP:
          if (! (clamped_sides & CLAMPED_LEFT) &&
              ! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_TOP;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_BOTTOM:
          if (! (clamped_sides & CLAMPED_LEFT) &&
              ! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_LEFT_AND_RIGHT_SYMMETRICALLY;
          else
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          break;

        case GIMP_TOOL_RECTANGLE_RESIZING_LEFT:
        case GIMP_TOOL_RECTANGLE_RESIZING_RIGHT:
          side_to_resize = SIDE_TO_RESIZE_TOP_AND_BOTTOM_SYMMETRICALLY;
          break;

        case GIMP_TOOL_RECTANGLE_MOVING:
        default:
          if (! (clamped_sides & CLAMPED_BOTTOM))
            side_to_resize = SIDE_TO_RESIZE_BOTTOM;
          else if (! (clamped_sides & CLAMPED_RIGHT))
            side_to_resize = SIDE_TO_RESIZE_RIGHT;
          else if (! (clamped_sides & CLAMPED_TOP))
            side_to_resize = SIDE_TO_RESIZE_TOP;
          else if (! (clamped_sides & CLAMPED_LEFT))
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
 * gimp_tool_rectangle_update_with_coord:
 * @rectangle:      A #GimpToolRectangle.
 * @new_x:          New X-coordinate in the context of the current function.
 * @new_y:          New Y-coordinate in the context of the current function.
 *
 * The core rectangle adjustment function. It updates the rectangle
 * for the passed cursor coordinate, taking current function and tool
 * options into account.  It also updates the current
 * private->function if necessary.
 */
static void
gimp_tool_rectangle_update_with_coord (GimpToolRectangle *rectangle,
                                       gdouble            new_x,
                                       gdouble            new_y)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  /* Move the corner or edge the user currently has grabbed. */
  gimp_tool_rectangle_apply_coord (rectangle, new_x, new_y);

  /* Update private->function. The function changes if the user
   * "flips" the rectangle.
   */
  gimp_tool_rectangle_check_function (rectangle);

  /* Clamp the rectangle if necessary */
  gimp_tool_rectangle_handle_general_clamping (rectangle);

  /* If the rectangle is being moved, do not run through any further
   * rectangle adjusting functions since it's shape should not change
   * then.
   */
  if (private->function != GIMP_TOOL_RECTANGLE_MOVING)
    {
      gimp_tool_rectangle_apply_fixed_rule (rectangle);
    }

  gimp_tool_rectangle_update_int_rect (rectangle);
}

static void
gimp_tool_rectangle_apply_fixed_rule (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate    *private = rectangle->private;
  GimpRectangleConstraint      constraint_to_use;
  GimpDisplayShell            *shell;
  GimpImage                   *image;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));
  image = gimp_display_get_image (shell->display);

  /* Calculate what constraint to use when needed. */
  constraint_to_use = gimp_tool_rectangle_get_constraint (rectangle);

  if (private->fixed_rule_active &&
      private->fixed_rule == GIMP_RECTANGLE_FIXED_ASPECT)
    {
      gdouble aspect;

      aspect = CLAMP (private->aspect_numerator /
                      private->aspect_denominator,
                      1.0 / gimp_image_get_height (image),
                      gimp_image_get_width (image));

      if (constraint_to_use == GIMP_RECTANGLE_CONSTRAIN_NONE)
        {
          gimp_tool_rectangle_apply_aspect (rectangle,
                                            aspect,
                                            CLAMPED_NONE);
        }
      else
        {
          if (private->function != GIMP_TOOL_RECTANGLE_MOVING)
            {
              ClampedSide clamped_sides = CLAMPED_NONE;

              gimp_tool_rectangle_apply_aspect (rectangle,
                                                aspect,
                                                clamped_sides);

              /* After we have applied aspect, we might have taken the
               * rectangle outside of constraint, so clamp and apply
               * aspect again. We will get the right result this time,
               * since 'clamped_sides' will be setup correctly now.
               */
              gimp_tool_rectangle_clamp (rectangle,
                                         &clamped_sides,
                                         constraint_to_use,
                                         private->fixed_center);

              gimp_tool_rectangle_apply_aspect (rectangle,
                                                aspect,
                                                clamped_sides);
            }
          else
            {
              gimp_tool_rectangle_apply_aspect (rectangle,
                                                aspect,
                                                CLAMPED_NONE);

              gimp_tool_rectangle_keep_inside (rectangle,
                                               constraint_to_use);
            }
        }
    }
  else if (private->fixed_rule_active &&
           private->fixed_rule == GIMP_RECTANGLE_FIXED_SIZE)
    {
      gimp_tool_rectangle_apply_fixed_width (rectangle,
                                             constraint_to_use,
                                             private->desired_fixed_size_width);
      gimp_tool_rectangle_apply_fixed_height (rectangle,
                                              constraint_to_use,
                                              private->desired_fixed_size_height);
    }
  else if (private->fixed_rule_active &&
           private->fixed_rule == GIMP_RECTANGLE_FIXED_WIDTH)
    {
      gimp_tool_rectangle_apply_fixed_width (rectangle,
                                             constraint_to_use,
                                             private->desired_fixed_width);
    }
  else if (private->fixed_rule_active &&
           private->fixed_rule == GIMP_RECTANGLE_FIXED_HEIGHT)
    {
      gimp_tool_rectangle_apply_fixed_height (rectangle,
                                              constraint_to_use,
                                              private->desired_fixed_height);
    }
}

/**
 * gimp_tool_rectangle_get_constraints:
 * @rectangle:      A #GimpToolRectangle.
 * @min_x:
 * @min_y:
 * @max_x:
 * @max_y:          Pointers of where to put constraints. NULL allowed.
 * @constraint:     Whether to return image or layer constraints.
 *
 * Calculates constraint coordinates for image or layer.
 */
static void
gimp_tool_rectangle_get_constraints (GimpToolRectangle       *rectangle,
                                     gint                    *min_x,
                                     gint                    *min_y,
                                     gint                    *max_x,
                                     gint                    *max_y,
                                     GimpRectangleConstraint  constraint)
{
  GimpDisplayShell *shell;
  GimpImage        *image;
  gint              min_x_dummy;
  gint              min_y_dummy;
  gint              max_x_dummy;
  gint              max_y_dummy;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));
  image = gimp_display_get_image (shell->display);

  if (! min_x) min_x = &min_x_dummy;
  if (! min_y) min_y = &min_y_dummy;
  if (! max_x) max_x = &max_x_dummy;
  if (! max_y) max_y = &max_y_dummy;

  *min_x = 0;
  *min_y = 0;
  *max_x = 0;
  *max_y = 0;

  switch (constraint)
    {
    case GIMP_RECTANGLE_CONSTRAIN_IMAGE:
      if (image)
        {
          *min_x = 0;
          *min_y = 0;
          *max_x = gimp_image_get_width  (image);
          *max_y = gimp_image_get_height (image);
        }
      break;

    case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
      if (image)
        {
          GList *items = gimp_image_get_selected_drawables (image);
          GList *iter;

          if (items != NULL)
            gimp_item_get_offset (items->data, min_x, min_y);

          /* Min and max constraints are respectively the smallest and
           * highest drawable coordinates.
           */
          for (iter = items; iter; iter = iter->next)
            {
              gint item_min_x;
              gint item_min_y;

              gimp_item_get_offset (iter->data, &item_min_x, &item_min_y);

              *min_x = MIN (*min_x, item_min_x);
              *min_y = MIN (*min_y, item_min_y);
              *max_x = MAX (*max_x, item_min_x + gimp_item_get_width  (iter->data));
              *max_y = MAX (*max_y, item_min_y + gimp_item_get_height (iter->data));
            }

          g_list_free (items);
        }
      break;

    default:
      g_warning ("Invalid rectangle constraint.\n");
      return;
    }
}

/**
 * gimp_tool_rectangle_handle_general_clamping:
 * @rectangle: A #GimpToolRectangle.
 *
 * Make sure that constraints are applied to the rectangle, either by
 * manually doing it, or by looking at the rectangle tool options and
 * concluding it will be done later.
 */
static void
gimp_tool_rectangle_handle_general_clamping (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;
  GimpRectangleConstraint   constraint;

  constraint = gimp_tool_rectangle_get_constraint (rectangle);

  /* fixed_aspect takes care of clamping by it self, so just return in
   * case that is in use. Also return if no constraints should be
   * enforced.
   */
  if (constraint == GIMP_RECTANGLE_CONSTRAIN_NONE)
    return;

  if (private->function != GIMP_TOOL_RECTANGLE_MOVING)
    {
      gimp_tool_rectangle_clamp (rectangle,
                                 NULL,
                                 constraint,
                                 private->fixed_center);
    }
  else
    {
      gimp_tool_rectangle_keep_inside (rectangle, constraint);
    }
}

/**
 * gimp_tool_rectangle_update_int_rect:
 * @rectangle:
 *
 * Update integer representation of rectangle.
 **/
static void
gimp_tool_rectangle_update_int_rect (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  private->x1_int = SIGNED_ROUND (private->x1);
  private->y1_int = SIGNED_ROUND (private->y1);

  if (gimp_tool_rectangle_rect_rubber_banding_func (rectangle))
    {
      private->width_int  = (gint) SIGNED_ROUND (private->x2) - private->x1_int;
      private->height_int = (gint) SIGNED_ROUND (private->y2) - private->y1_int;
    }
}

/**
 * gimp_tool_rectangle_adjust_coord:
 * @rectangle:
 * @ccoord_x_input:
 * @ccoord_x_input:
 * @ccoord_x_output:
 * @ccoord_x_output:
 *
 * Transforms a coordinate to better fit the public behavior of the
 * rectangle.
 */
static void
gimp_tool_rectangle_adjust_coord (GimpToolRectangle *rectangle,
                                  gdouble            coord_x_input,
                                  gdouble            coord_y_input,
                                  gdouble           *coord_x_output,
                                  gdouble           *coord_y_output)
{
  GimpToolRectanglePrivate *priv = rectangle->private;

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

static void
gimp_tool_rectangle_recalculate_center_xy (GimpToolRectangle *rectangle)
{
  GimpToolRectanglePrivate *private = rectangle->private;

  private->center_x_on_fixed_center = (private->x1 + private->x2) / 2;
  private->center_y_on_fixed_center = (private->y1 + private->y2) / 2;
}


/*  public functions  */

GimpToolWidget *
gimp_tool_rectangle_new (GimpDisplayShell  *shell)
{
  g_return_val_if_fail (GIMP_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (GIMP_TYPE_TOOL_RECTANGLE,
                       "shell",      shell,
                       NULL);
}

GimpRectangleFunction
gimp_tool_rectangle_get_function (GimpToolRectangle *rectangle)
{
  g_return_val_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle),
                        GIMP_TOOL_RECTANGLE_DEAD);

  return rectangle->private->function;
}

void
gimp_tool_rectangle_set_function (GimpToolRectangle     *rectangle,
                                  GimpRectangleFunction  function)
{
  GimpToolRectanglePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));

  private = rectangle->private;

  if (private->function != function)
    {
      private->function = function;

      gimp_tool_rectangle_changed (GIMP_TOOL_WIDGET (rectangle));
    }
}

void
gimp_tool_rectangle_set_constraint (GimpToolRectangle       *rectangle,
                                    GimpRectangleConstraint  constraint)
{
  GimpToolRectanglePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));

  private = rectangle->private;

  if (constraint != private->constraint)
    {
      g_object_freeze_notify (G_OBJECT (rectangle));

      private->constraint = constraint;
      g_object_notify (G_OBJECT (rectangle), "constraint");

      gimp_tool_rectangle_clamp (rectangle, NULL, constraint, FALSE);

      g_object_thaw_notify (G_OBJECT (rectangle));

      gimp_tool_rectangle_change_complete (rectangle);
    }
}

GimpRectangleConstraint
gimp_tool_rectangle_get_constraint (GimpToolRectangle *rectangle)
{
  g_return_val_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle), 0);

  return rectangle->private->constraint;
}

/**
 * gimp_tool_rectangle_get_public_rect:
 * @rectangle:
 * @x1:
 * @y1:
 * @x2:
 * @y2:
 *
 * This function returns the rectangle as it appears to be publicly
 * (based on integer or double precision-mode).
 **/
void
gimp_tool_rectangle_get_public_rect (GimpToolRectangle *rectangle,
                                     gdouble           *x1,
                                     gdouble           *y1,
                                     gdouble           *x2,
                                     gdouble           *y2)
{
  GimpToolRectanglePrivate *priv;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));
  g_return_if_fail (x1 != NULL);
  g_return_if_fail (y1 != NULL);
  g_return_if_fail (x2 != NULL);
  g_return_if_fail (y2 != NULL);

  priv = rectangle->private;

  switch (priv->precision)
    {
    case GIMP_RECTANGLE_PRECISION_INT:
      *x1 = priv->x1_int;
      *y1 = priv->y1_int;
      *x2 = priv->x1_int + priv->width_int;
      *y2 = priv->y1_int + priv->height_int;
      break;

    case GIMP_RECTANGLE_PRECISION_DOUBLE:
    default:
      *x1 = priv->x1;
      *y1 = priv->y1;
      *x2 = priv->x2;
      *y2 = priv->y2;
      break;
    }
}

/**
 * gimp_tool_rectangle_pending_size_set:
 * @width_property:  Option property to set to pending rectangle width.
 * @height_property: Option property to set to pending rectangle height.
 *
 * Sets specified rectangle tool options properties to the width and
 * height of the current pending rectangle.
 */
void
gimp_tool_rectangle_pending_size_set (GimpToolRectangle *rectangle,
                                      GObject           *object,
                                      const gchar       *width_property,
                                      const gchar       *height_property)
{
  GimpToolRectanglePrivate *private;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));
  g_return_if_fail (width_property  != NULL);
  g_return_if_fail (height_property != NULL);

  private = rectangle->private;

  g_object_set (object,
                width_property,  MAX (private->x2 - private->x1, 1.0),
                height_property, MAX (private->y2 - private->y1, 1.0),
                NULL);
}

/**
 * gimp_tool_rectangle_constraint_size_set:
 * @width_property:  Option property to set to current constraint width.
 * @height_property: Option property to set to current constraint height.
 *
 * Sets specified rectangle tool options properties to the width and
 * height of the current constraint size.
 */
void
gimp_tool_rectangle_constraint_size_set (GimpToolRectangle *rectangle,
                                         GObject           *object,
                                         const gchar       *width_property,
                                         const gchar       *height_property)
{
  GimpDisplayShell *shell;
  GimpContext      *context;
  GimpImage        *image;
  gdouble           width;
  gdouble           height;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));

  shell   = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));
  context = gimp_get_user_context (shell->display->gimp);
  image   = gimp_context_get_image (context);

  if (! image)
    {
      width  = 1.0;
      height = 1.0;
    }
  else
    {
      GimpRectangleConstraint constraint;

      constraint = gimp_tool_rectangle_get_constraint (rectangle);

      switch (constraint)
        {
        case GIMP_RECTANGLE_CONSTRAIN_DRAWABLE:
          {
            GList *items = gimp_image_get_selected_layers (image);
            GList *iter;

            width  = 0.0;
            height = 0.0;
            for (iter = items; iter; iter = iter->next)
              {
                if (width == 0.0 || height == 0.0)
                  {
                    width  = gimp_item_get_width  (iter->data);
                    height = gimp_item_get_height (iter->data);
                  }
                else if (width  != gimp_item_get_width  (iter->data) ||
                         height != gimp_item_get_height (iter->data))
                  {
                    width  = 0.0;
                    height = 0.0;
                    break;
                  }
              }

            /* Set constraint to the selected layers' dimensions if all
             * selected layers have the same dimension. Otherwise set to
             * image dimensions.
             */
            if (width == 0.0 || height == 0.0)
              {
                width  = gimp_image_get_width  (image);
                height = gimp_image_get_height (image);
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
 * gimp_tool_rectangle_rectangle_is_first:
 * @rectangle:
 *
 * Returns: %TRUE if the user is creating the first rectangle with
 * this instance from scratch, %FALSE if modifying an existing
 * rectangle, or creating a new rectangle, discarding the existing
 * one. This function is only meaningful in _motion and
 * _button_release.
 */
gboolean
gimp_tool_rectangle_rectangle_is_first (GimpToolRectangle *rectangle)
{
  g_return_val_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle), FALSE);

  return rectangle->private->is_first;
}

/**
 * gimp_tool_rectangle_rectangle_is_new:
 * @rectangle:
 *
 * Returns: %TRUE if the user is creating a new rectangle from
 * scratch, %FALSE if modifying n previously existing rectangle. This
 * function is only meaningful in _motion and _button_release.
 */
gboolean
gimp_tool_rectangle_rectangle_is_new (GimpToolRectangle *rectangle)
{
  g_return_val_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle), FALSE);

  return rectangle->private->is_new;
}

/**
 * gimp_tool_rectangle_point_in_rectangle:
 * @rectangle:
 * @x:         X-coord of point to test (in image coordinates)
 * @y:         Y-coord of point to test (in image coordinates)
 *
 * Returns: %TRUE if the passed point was within the rectangle
 **/
gboolean
gimp_tool_rectangle_point_in_rectangle (GimpToolRectangle *rectangle,
                                        gdouble            x,
                                        gdouble            y)
{
  gdouble x1 = 0;
  gdouble y1 = 0;
  gdouble x2 = 0;
  gdouble y2 = 0;

  g_return_val_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle), FALSE);

  gimp_tool_rectangle_get_public_rect (rectangle, &x1, &y1, &x2, &y2);

  return (x >= x1 && x <= x2 &&
          y >= y1 && y <= y2);
}

/**
 * gimp_tool_rectangle_frame_item:
 * @rectangle: a #GimpToolRectangle interface
 * @item:      a #GimpItem attached to the image on which a
 *             rectangle is being shown.
 *
 * Convenience function to set the corners of the rectangle to
 * match the bounds of the specified item.  The rectangle interface
 * must be active (i.e., showing a rectangle), and the item must be
 * attached to the image on which the rectangle is active.
 **/
void
gimp_tool_rectangle_frame_item (GimpToolRectangle *rectangle,
                                GimpItem          *item)
{
  GimpDisplayShell      *shell;
  gint                   offset_x;
  gint                   offset_y;
  gint                   width;
  gint                   height;
  GimpRectangleFunction  old_function;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));

  g_return_if_fail (gimp_display_get_image (shell->display) ==
                    gimp_item_get_image (item));

  width  = gimp_item_get_width  (item);
  height = gimp_item_get_height (item);

  gimp_item_get_offset (item, &offset_x, &offset_y);

  old_function = rectangle->private->function;
  gimp_tool_rectangle_set_function (rectangle, GIMP_TOOL_RECTANGLE_CREATING);

  g_object_set (rectangle,
                "x1", (gdouble) offset_x,
                "y1", (gdouble) offset_y,
                "x2", (gdouble) (offset_x + width),
                "y2", (gdouble) (offset_y + height),
                NULL);

  /* kludge to force handle sizes to update.  This call may be harmful
   * if this function is ever moved out of the text tool code.
   */
  gimp_tool_rectangle_set_constraint (rectangle, GIMP_RECTANGLE_CONSTRAIN_NONE);
  gimp_tool_rectangle_set_function (rectangle, old_function);
}

void
gimp_tool_rectangle_auto_shrink (GimpToolRectangle *rectangle,
                                 gboolean           shrink_merged)
{
  GimpToolRectanglePrivate *private;
  GimpDisplayShell         *shell;
  GimpImage                *image;
  GList                    *pickables = NULL;
  GList                    *iter;
  gdouble                   new_x1, new_y1;
  gdouble                   new_x2, new_y2;

  g_return_if_fail (GIMP_IS_TOOL_RECTANGLE (rectangle));

  private = rectangle->private;

  shell = gimp_tool_widget_get_shell (GIMP_TOOL_WIDGET (rectangle));
  image = gimp_display_get_image (shell->display);

  if (shrink_merged)
    pickables = g_list_prepend (NULL, image);
  else
    pickables = gimp_image_get_selected_drawables (image);

  if (! pickables)
    return;

  new_x1 = new_y1 = G_MAXINT;
  new_x2 = new_y2 = G_MININT;
  for (iter = pickables; iter; iter = iter->next)
    {
      gint x1, y1;
      gint x2, y2;
      gint offset_x = 0;
      gint offset_y = 0;
      gint shrunk_x;
      gint shrunk_y;
      gint shrunk_width;
      gint shrunk_height;

      if (GIMP_IS_IMAGE (iter->data))
        {
          x1 = private->x1;
          y1 = private->y1;
          x2 = private->x2;
          y2 = private->y2;
        }
      else
        {
          gimp_item_get_offset (GIMP_ITEM (iter->data), &offset_x, &offset_y);

          x1 = private->x1 - offset_x;
          y1 = private->y1 - offset_y;
          x2 = private->x2 - offset_x;
          y2 = private->y2 - offset_y;
        }

      switch (gimp_pickable_auto_shrink (iter->data,
                                         x1, y1, x2 - x1, y2 - y1,
                                         &shrunk_x,
                                         &shrunk_y,
                                         &shrunk_width,
                                         &shrunk_height))
        {
        case GIMP_AUTO_SHRINK_SHRINK:
            {
              new_x1 = MIN (new_x1, offset_x + shrunk_x);
              new_y1 = MIN (new_y1, offset_y + shrunk_y);
              new_x2 = MAX (new_x2, offset_x + shrunk_x + shrunk_width);
              new_y2 = MAX (new_y2, offset_y + shrunk_y + shrunk_height);
            }
          break;

        default:
          break;
        }
    }

  if (new_x1 != G_MAXINT && new_y1 != G_MAXINT &&
      new_x2 != G_MININT && new_y2 != G_MININT)
    {
      GimpRectangleFunction original_function = private->function;

      private->function = GIMP_TOOL_RECTANGLE_AUTO_SHRINK;

      private->x1 = new_x1;
      private->y1 = new_y1;
      private->x2 = new_x2;
      private->y2 = new_y2;

      gimp_tool_rectangle_update_int_rect (rectangle);

      gimp_tool_rectangle_change_complete (rectangle);

      private->function = original_function;

      gimp_tool_rectangle_update_options (rectangle);
    }

  g_list_free (pickables);
}
