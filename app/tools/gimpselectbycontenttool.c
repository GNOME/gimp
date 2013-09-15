/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* This tool is based on a paper from SIGGRAPH '95:
 *  "Intelligent Scissors for Image Composition", Eric N. Mortensen and
 *   William A. Barrett, Brigham Young University.
 *
 * thanks to Professor D. Forsyth for prompting us to implement this tool. */

/* Personal note: Dr. Barrett, one of the authors of the paper written above
 * is not only one of the most brilliant people I have ever met, he is an
 * incredible professor who genuinely cares about his students and wants them
 * to learn as much as they can about the topic.
 *
 * I didn't even notice I was taking a class from the person who wrote the
 * paper until halfway through the semester.
 *                                                   -- Rockwalrus
 */

/* The history of this implementation is lonog and varied.  It was
 * orignally done by Spencer and Peter, and worked fine in the 0.54
 * (motif only) release of GIMP.  Later revisions (0.99.something
 * until about 1.1.4) completely changed the algorithm used, until it
 * bore little resemblance to the one described in the paper above.
 * The 0.54 version of the algorithm was then forwards ported to 1.1.4
 * by Austin Donnelly.
 */

/* Livewire boundary implementation done by Laramie Leavitt */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp-utils.h"
#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpscanconvert.h"
#include "core/gimptoolinfo.h"
#include "core/gimplayer-floating-sel.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"

#include "gimpiscissorsoptions.h"
#include "gimpselectbycontent.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  defines  */
#define  MAX_GRADIENT      179.606  /* == sqrt (127^2 + 127^2) */
#define  GRADIENT_SEARCH   32  /* how far to look when snapping to an edge */
#define  EXTEND_BY         0.2 /* proportion to expand cost map by */
#define  FIXED             5   /* additional fixed size to expand cost map */
#define  MIN_GRADIENT      63  /* gradients < this are directionless */

#define  COST_WIDTH        2   /* number of bytes for each pixel in cost map  */

/* weight to give between gradient (_G) and direction (_D) */
#define  OMEGA_D           0.2
#define  OMEGA_G           0.8

/* sentinel to mark seed point in ?cost? map */
#define  SEED_POINT        9

/*  Functional defines  */
#define  PIXEL_COST(x)     ((x) >> 8)
#define  PIXEL_DIR(x)      ((x) & 0x000000ff)

/* Free Select Tool  */
#define POINT_GRAB_THRESHOLD_SQ SQR (GIMP_TOOL_HANDLE_SIZE_CIRCLE / 2)
#define POINT_SHOW_THRESHOLD_SQ SQR (GIMP_TOOL_HANDLE_SIZE_CIRCLE * 7)
#define N_ITEMS_PER_ALLOC       1024
#define INVALID_INDEX           (-1)
#define NO_CLICK_TIME_AVAILABLE 0

#define GET_PRIVATE(sbc)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((sbc), \
    GIMP_TYPE_SELECT_BY_CONTENT_TOOL, GimpSelectByContentToolPrivate))


struct _ICurve
{
  gint       x1, y1;
  gint       x2, y2;
  GPtrArray *points;
};

typedef struct
{
  /* Index of grabbed segment index. */
  gint               grabbed_segment_index;

  /* We need to keep track of a number of points when we move a
   * segment vertex
   */
  GimpVector2       *saved_points_lower_segment;
  GimpVector2       *saved_points_higher_segment;
  gint               max_n_saved_points_lower_segment;
  gint               max_n_saved_points_higher_segment;

  /* Keeps track wether or not a modification of the polygon has been
   * made between _button_press and _button_release
   */
  gboolean           polygon_modified;

  /* Point which is used to draw the polygon but which is not part of
   * it yet
   */
  GimpVector2        pending_point;
  gboolean           show_pending_point;

  /* The points of the polygon */
  GimpVector2       *points;
  gint               max_n_points;

  /* The number of points actually in use */
  gint               n_points;


  /* Any int array containing the indices for the points in the
   * polygon that connects different segments together
   */
  gint              *segment_indices;
  gint               max_n_segment_indices;

  /* The number of segment indices actually in use */
  gint               n_segment_indices;

  /* The selection operation active when the tool was started */
  GimpChannelOps     operation_at_start;

  /* Wether or not to constrain the angle for newly created polygonal
   * segments.
   */
  gboolean           constrain_angle;

  /* Wether or not to supress handles (so that new segments can be
   * created immediately after an existing segment vertex.
   */
  gboolean           supress_handles;

  /* Last _oper_update or _motion coords */
  GimpVector2        last_coords;

  /* A double-click commits the selection, keep track of last
   * click-time and click-position.
   */
  guint32            last_click_time;
  GimpCoords         last_click_coord;

} GimpSelectByContentToolPrivate;


/*  local function prototypes  */

static void   gimp_select_by_content_tool_finalize       (GObject               *object);

static void   gimp_select_by_content_tool_control        (GimpTool              *tool,
                                                         GimpToolAction         action,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_button_press   (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpButtonPressType    press_type,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_button_release (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpButtonReleaseType  release_type,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_motion         (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_oper_update    (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_cursor_update  (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         GdkModifierType        state,
                                                         GimpDisplay           *display);
static gboolean gimp_select_by_content_tool_key_press    (GimpTool              *tool,
                                                         GdkEventKey           *kevent,
                                                         GimpDisplay           *display);

static void   gimp_select_by_content_tool_apply          (GimpSelectByContentTool     *sbc,
                                                         GimpDisplay           *display);
static void   gimp_select_by_content_tool_draw           (GimpDrawTool          *draw_tool);

/*Free Select Tool Functions */

static void     gimp_select_by_content_tool_modifier_key        (GimpTool              *tool,
                                                                GdkModifierType        key,
                                                                gboolean               press,
                                                                GdkModifierType        state,
                                                                GimpDisplay           *display);
static void     gimp_select_by_content_tool_active_modifier_key (GimpTool              *tool,
                                                                GdkModifierType        key,
                                                                gboolean               press,
                                                                GdkModifierType        state,
                                                                GimpDisplay           *display);

static void     gimp_select_by_content_tool_real_select         (GimpSelectByContentTool    *sbc,
                                                                GimpDisplay           *display);


/*IScissor Tool Options*/

static void          select_by_content_convert  (GimpSelectByContentTool *sbc,
                                                GimpDisplay       *display);
static TileManager * gradient_map_new           (GimpImage         *image);

static void          find_optimal_path         (TileManager       *gradient_map,
                                                TempBuf           *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2,
                                                gint               xs,
                                                gint               ys);
static void          find_max_gradient         (GimpSelectByContentTool *sbc,
                                                GimpImage         *image,
                                                gint              *x,
                                                gint              *y);
static void          calculate_curve           (GimpSelectByContentTool *sbc,
                                                ICurve            *curve);
static void          iscissors_draw_curve      (GimpDrawTool      *draw_tool,
                                                ICurve            *curve);

static gint          mouse_over_vertex         (GimpSelectByContentTool *sbc,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_vertex         (GimpSelectByContentTool *sbc,
                                                gdouble            x,
                                                gdouble            y);
static GList       * mouse_over_curve          (GimpSelectByContentTool *sbc,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_curve          (GimpSelectByContentTool *sbc,
                                                gdouble            x,
                                                gdouble            y);

static GPtrArray   * plot_pixels               (GimpSelectByContentTool *sbc,
                                                TempBuf           *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               xs,
                                                gint               ys,
                                                gint               xe,
                                                gint               ye);


G_DEFINE_TYPE (GimpSelectByContentTool, gimp_select_by_content_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_select_by_content_tool_parent_class


/*  static variables  */

/*  where to move on a given link direction  */

/*Free Select Tool*/
static const GimpVector2 vector2_zero = { 0.0, 0.0 };

static const gint move[8][2] =
{
  {  1,  0 },
  {  0,  1 },
  { -1,  1 },
  {  1,  1 },
  { -1,  0 },
  {  0, -1 },
  {  1, -1 },
  { -1, -1 },
};

/* IE:
 * '---+---+---`
 * | 7 | 5 | 6 |
 * +---+---+---+
 * | 4 |   | 0 |
 * +---+---+---+
 * | 2 | 1 | 3 |
 * `---+---+---'
 */


/*  temporary convolution buffers --  */
static guchar  maxgrad_conv0[TILE_WIDTH * TILE_HEIGHT * 4] = "";
static guchar  maxgrad_conv1[TILE_WIDTH * TILE_HEIGHT * 4] = "";
static guchar  maxgrad_conv2[TILE_WIDTH * TILE_HEIGHT * 4] = "";


static const gfloat horz_deriv[9] =
{
   1,  0, -1,
   2,  0, -2,
   1,  0, -1,
};

static const gfloat vert_deriv[9] =
{
   1,  2,  1,
   0,  0,  0,
  -1, -2, -1,
};

static const gfloat blur_32[9] =
{
   1,  1,  1,
   1, 24,  1,
   1,  1,  1,
};

static gfloat  distance_weights[GRADIENT_SEARCH * GRADIENT_SEARCH];

static gint    diagonal_weight[256];
static gint    direction_value[256][4];
static Tile   *cur_tile    = NULL;


void
gimp_select_by_content_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_SELECT_BY_CONTENT_TOOL,
                GIMP_TYPE_ISCISSORS_OPTIONS,
                gimp_iscissors_options_gui,
                0,
                "gimp-select-by-content-tool",
                _("Select By Content"),
                _("Select By Content Tool: Select shapes using Free and intelligent edge-fitting"),
                N_("Select_BY_CONTENT"),
                "I",
                NULL, GIMP_HELP_TOOL_SELECT_BY_CONTENT,
                GIMP_STOCK_TOOL_SELECT_BY_CONTENT,
                data);
}

static void
gimp_select_by_content_tool_class_init (GimpSelectByContentToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  gint               i;

  object_class->finalize     = gimp_select_by_content_tool_finalize;

  tool_class->control        = gimp_select_by_content_tool_control;
  tool_class->button_press   = gimp_select_by_content_tool_button_press;
  tool_class->button_release = gimp_select_by_content_tool_button_release;
  tool_class->motion         = gimp_select_by_content_tool_motion;
  tool_class->oper_update    = gimp_select_by_content_tool_oper_update;
  tool_class->cursor_update  = gimp_select_by_content_tool_cursor_update;
  tool_class->key_press      = gimp_select_by_content_tool_key_press;
  tool_class->modifier_key        = gimp_select_by_content_tool_modifier_key;
  tool_class->active_modifier_key = gimp_select_by_content_tool_active_modifier_key;

  draw_tool_class->draw      = gimp_select_by_content_tool_draw;

  for (i = 0; i < 256; i++)
    {
      /*  The diagonal weight array  */
      diagonal_weight[i] = (int) (i * G_SQRT2);

      /*  The direction value array  */
      direction_value[i][0] = (127 - abs (127 - i)) * 2;
      direction_value[i][1] = abs (127 - i) * 2;
      direction_value[i][2] = abs (191 - i) * 2;
      direction_value[i][3] = abs (63 - i) * 2;
    }

  /*  set the 256th index of the direction_values to the highest cost  */
  direction_value[255][0] = 255;
  direction_value[255][1] = 255;
  direction_value[255][2] = 255;
  direction_value[255][3] = 255;
  
  klass->select                   = gimp_select_by_content_tool_real_select;
  g_type_class_add_private (klass, sizeof (GimpSelectByContentToolPrivate));
}

static void
gimp_select_by_content_tool_init (GimpSelectByContentTool *sbc)
{
  GimpTool *tool = GIMP_TOOL (sbc);
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control,
                                     GIMP_DIRTY_IMAGE_SIZE |
                                     GIMP_DIRTY_ACTIVE_DRAWABLE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_ISCISSORS);
  gimp_tool_control_set_wants_click (tool->control, TRUE);
  gimp_tool_control_set_precision   (tool->control,
                                     GIMP_CURSOR_PRECISION_SUBPIXEL);

  
  sbc->op     = ISCISSORS_OP_NONE;
  sbc->curves = g_queue_new ();
  sbc->state  = NO_ACTION;
  
  /*Free Select Tool*/
  priv->grabbed_segment_index             = INVALID_INDEX;

  priv->saved_points_lower_segment        = NULL;
  priv->saved_points_higher_segment       = NULL;
  priv->max_n_saved_points_lower_segment  = 0;
  priv->max_n_saved_points_higher_segment = 0;

  priv->polygon_modified                  = FALSE;

  priv->show_pending_point                = FALSE;

  priv->points                            = NULL;
  priv->n_points                          = 0;
  priv->max_n_points                      = 0;

  priv->segment_indices                   = NULL;
  priv->n_segment_indices                 = 0;
  priv->max_n_segment_indices             = 0;

  priv->constrain_angle                   = FALSE;
  priv->supress_handles                   = FALSE;

  priv->last_click_time                   = NO_CLICK_TIME_AVAILABLE;
}

static void
gimp_select_by_content_tool_finalize (GObject *object)
{
  GimpSelectByContentTool *sbc = GIMP_ISCISSORS_TOOL (object);
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  g_queue_free (sbc->curves);
  
  g_free (priv->points);
  g_free (priv->segment_indices);
  g_free (priv->saved_points_lower_segment);
  g_free (priv->saved_points_higher_segment);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_select_by_content_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpSelectByContentPrivate *priv = GET_PRIVATE (sbc);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      /*  Free and reset the curve list  */
      if (sbc->magnetic)
   {
    while (! g_queue_is_empty (sbc->curves))
        {
          ICurve *curve = g_queue_pop_head (sbc->curves);

          if (curve->points)
            g_ptr_array_free (curve->points, TRUE);

          g_slice_free (ICurve, curve);
        }

      /*  free mask  */
      if (sbc->mask)
        {
          g_object_unref (sbc->mask);
          sbc->mask = NULL;
        }

      /* free the gradient map */
      if (sbc->gradient_map)
        {
          /* release any tile we were using */
          if (cur_tile)
            {
              tile_release (cur_tile, FALSE);
              cur_tile = NULL;
            }

          tile_manager_unref (sbc->gradient_map);
          sbc->gradient_map = NULL;
        }

      sbc->curve1      = NULL;
      sbc->curve2      = NULL;
      sbc->first_point = TRUE;
      sbc->connected   = FALSE;
      sbc->state       = NO_ACTION;

      /*  Reset the dp buffers  */
      if (sbc->dp_buf)
        {
          temp_buf_free (sbc->dp_buf);
          sbc->dp_buf = NULL;
        }
    }  
    
    else
    
      {
        priv->grabbed_segment_index = INVALID_INDEX;
          priv->show_pending_point    = FALSE;
          priv->n_points              = 0;
          priv->n_segment_indices     = 0;
     }
    
      break;
    }

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
gimp_select_by_content_tool_button_press (GimpTool            *tool,
                                  const GimpCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  GimpButtonPressType  press_type,
                                  GimpDisplay         *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpImage         *image     = gimp_display_get_image (display);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpSelectionOptions      *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  sbc->x = RINT (coords->x);
  sbc->y = RINT (coords->y);

  /*  If the tool was being used in another image...reset it  */
  if (display != tool->display)
    gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
  
  if (sbc->magnetic)
  {
  gimp_tool_control_activate (tool->control);
  tool->display = display;

  switch (sbc->state)
    {
    case NO_ACTION:
      sbc->state = SEED_PLACEMENT;

      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (sbc, image,
                           &sbc->x,
                           &sbc->y);

      sbc->x = CLAMP (sbc->x, 0, gimp_image_get_width  (image) - 1);
      sbc->y = CLAMP (sbc->y, 0, gimp_image_get_height (image) - 1);

      sbc->ix = sbc->x;
      sbc->iy = sbc->y;

      /*  Initialize the selection core only on starting the tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
      break;

    default:
      /*  Check if the mouse click occurred on a vertex or the curve itself  */
      if (clicked_on_vertex (sbc, coords->x, coords->y))
        {
          sbc->nx    = sbc->x;
          sbc->ny    = sbc->y;
          sbc->state = SEED_ADJUSTMENT;

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
      /*  If the iscissors is connected, check if the click was inside  */
      else if (sbc->connected && sbc->mask &&
               gimp_pickable_get_opacity_at (GIMP_PICKABLE (sbc->mask),
                                             sbc->x,
                                             sbc->y))
        {
          gimp_iscissors_tool_apply (sbc, display);
        }
      else if (! sbc->connected)
        {
          /*  if we're not connected, we're adding a new point  */

          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          sbc->state = SEED_PLACEMENT;

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
      break;
    }
  }
  else
  {
      if (tool->display == NULL)
    {
      /* First of all handle delegation to the selection mask edit logic
       * if appropriate.
       */
      if (gimp_selection_tool_start_edit (GIMP_SELECTION_TOOL (sbc),
                                          display, coords))
        {
          return;
        }

      tool->display = display;

      gimp_draw_tool_start (draw_tool, display);

      /* We want to apply the selection operation that was current when
       * the tool was started, so we save this information
       */
      priv->operation_at_start = options->operation;
    }

  gimp_tool_control_activate (tool->control);

  gimp_draw_tool_pause (draw_tool);

  if (gimp_select_by_content_is_point_grabbed (sbc))
    {
      gimp_select_by_content_prepare_for_move (sbc);
    }
  else
    {
      GimpVector2 point_to_add;

      /* Note that we add the pending point (unless it is the first
       * point we add) because the pending point is setup correctly
       * with regards to angle constraints.
       */
      if (priv->n_points > 0)
        {
          point_to_add = priv->pending_point;
        }
      else
        {
          point_to_add.x = coords->x;
          point_to_add.y = coords->y;
        }

      /* No point was grabbed, add a new point and mark this as a
       * segment divider. For a line segment, this will be the only
       * new point. For a free segment, this will be the first point
       * of the free segment.
       */
      gimp_select_by_content_tool_add_point (sbc,
                                       point_to_add.x,
                                       point_to_add.y);
      gimp_select_by_content_tool_add_segment_index (sbc,
                                               priv->n_points - 1);
    }

  gimp_draw_tool_resume (draw_tool);
  }
  
}

static void
iscissors_convert (GimpSelectByContentTool *sbc,
                   GimpDisplay       *display)
{
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (sbc);
  GimpImage            *image   = gimp_display_get_image (display);
  GimpScanConvert      *sc;
  GList                *list;
  GimpVector2          *points = NULL;
  guint                 n_total_points = 0;

  sc = gimp_scan_convert_new ();

  for (list = g_queue_peek_tail_link (sbc->curves);
       list;
       list = g_list_previous (list))
    {
      ICurve *icurve = list->data;

      n_total_points += icurve->points->len;
    }

  points = g_new (GimpVector2, n_total_points);
  n_total_points = 0;

  /* go over the curves in reverse order, adding the points we have */
  for (list = g_queue_peek_tail_link (sbc->curves);
       list;
       list = g_list_previous (list))
    {
      ICurve *icurve = list->data;
      gint    i;
      guint   n_points;

      n_points = icurve->points->len;

      for (i = 0; i < n_points; i++)
        {
          guint32  packed = GPOINTER_TO_INT (g_ptr_array_index (icurve->points,
                                                                i));

          points[n_total_points+i].x = packed & 0x0000ffff;
          points[n_total_points+i].y = packed >> 16;
        }

      n_total_points += n_points;
    }

  gimp_scan_convert_add_polyline (sc, n_total_points, points, TRUE);
  g_free (points);

  if (sbc->mask)
    g_object_unref (sbc->mask);

  sbc->mask = gimp_channel_new_mask (image,
                                           gimp_image_get_width  (image),
                                           gimp_image_get_height (image));
  gimp_scan_convert_render (sc,
                            gimp_drawable_get_tiles (GIMP_DRAWABLE (sbc->mask)),
                            0, 0, options->antialias);
  gimp_scan_convert_free (sc);
}

static void
gimp_select_by_content_tool_button_release (GimpTool              *tool,
                                    const GimpCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  gimp_tool_control_halt (tool->control);

  /* Make sure X didn't skip the button release event -- as it's known
   * to do
   */
  
  if (sbc->magnetic)
  {
  if (sbc->state == WAITING)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /*  Progress to the next stage of intelligent selection  */
      switch (sbc->state)
        {
        case SEED_PLACEMENT:
          /*  Add a new icurve  */
          if (!sbc->first_point)
            {
              /*  Determine if we're connecting to the first point  */
              if (! g_queue_is_empty (sbc->curves))
                {
                  ICurve *curve = g_queue_peek_head (sbc->curves);

                  if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                                sbc->x, sbc->y,
                                                GIMP_HANDLE_CIRCLE,
                                                curve->x1, curve->y1,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_HANDLE_ANCHOR_CENTER))
                    {
                      sbc->x = curve->x1;
                      sbc->y = curve->y1;
                      sbc->connected = TRUE;
                    }
                }

              /*  Create the new curve segment  */
              if (sbc->ix != sbc->x ||
                  sbc->iy != sbc->y)
                {
                  ICurve *curve = g_slice_new (ICurve);

                  curve->x1 = sbc->ix;
                  curve->y1 = sbc->iy;
                  sbc->ix = curve->x2 = sbc->x;
                  sbc->iy = curve->y2 = sbc->y;
                  curve->points = NULL;

                  g_queue_push_tail (sbc->curves, curve);

                  calculate_curve (sbc, curve);
                }
            }
          else /* this was our first point */
            {
              sbc->first_point = FALSE;
            }
          break;

        case SEED_ADJUSTMENT:
          /*  recalculate both curves  */
          if (sbc->curve1)
            {
              sbc->curve1->x1 = sbc->nx;
              sbc->curve1->y1 = sbc->ny;

              calculate_curve (sbc, sbc->curve1);
            }

          if (sbc->curve2)
            {
              sbc->curve2->x2 = sbc->nx;
              sbc->curve2->y2 = sbc->ny;

              calculate_curve (sbc, sbc->curve2);
            }
          break;

        default:
          break;
        }
    }

  sbc->state = WAITING;
  }

  else
  {
     gimp_draw_tool_pause (GIMP_DRAW_TOOL (sbc));

  switch (release_type)
    {
    case GIMP_BUTTON_RELEASE_CLICK:
    case GIMP_BUTTON_RELEASE_NO_MOTION:
      /* If a click was made, we don't consider the polygon modified */
      priv->polygon_modified = FALSE;

      gimp_select_by_content_tool_handle_click (sbc,
                                          coords,
                                          time,
                                          display);
      break;

    case GIMP_BUTTON_RELEASE_NORMAL:
      gimp_select_by_content_tool_handle_normal_release (sbc,
                                                   coords,
                                                   display);
      break;

    case GIMP_BUTTON_RELEASE_CANCEL:
      gimp_select_by_content_tool_handle_cancel (sbc);
      break;

    default:
      break;
    }

  /* Reset */
  priv->polygon_modified = FALSE;

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (sbc));
  }

  }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  /*  convert the curves into a region  */
  if (sbc->connected)
    sbc_convert (sbc, display);
}

static void
gimp_select_by_content_tool_motion (GimpTool         *tool,
                            const GimpCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            GimpDisplay      *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpImage         *image     = gimp_display_get_image (display);
  GimpSelectByContentToolPrivate *priv      = GET_PRIVATE (sbc);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);

  if (sbc->magnetic)
  {
  if (sbc->state == NO_ACTION)
    return;

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  sbc->x = RINT (coords->x);
  sbc->y = RINT (coords->y);

  switch (sbc->state)
    {
    case SEED_PLACEMENT:
      /*  Hold the shift key down to disable the auto-edge snap feature  */
      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (sbc, image,
                           &sbc->x, &sbc->y);

      sbc->x = CLAMP (sbc->x, 0, gimp_image_get_width  (image) - 1);
      sbc->y = CLAMP (sbc->y, 0, gimp_image_get_height (image) - 1);

      if (sbc->first_point)
        {
          sbc->ix = sbc->x;
          sbc->iy = sbc->y;
        }
      break;

    case SEED_ADJUSTMENT:
      /*  Move the current seed to the location of the cursor  */
      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (sbc, image,
                           &sbc->x, &sbc->y);

      sbc->x = CLAMP (sbc->x, 0, gimp_image_get_width  (image) - 1);
      sbc->y = CLAMP (sbc->y, 0, gimp_image_get_height (image) - 1);

      sbc->nx = sbc->x;
      sbc->ny = sbc->y;
      break;

    default:
      break;
    }
   } 

   else
   {
    priv->last_coords.x = coords->x;
    priv->last_coords.y = coords->y;

    gimp_select_by_content_tool_update_motion (sbc,
                                       coords->x,
                                       coords->y);
   }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

/**
 * gimp_free_select_tool_draw:
 * @draw_tool:
 *
 * Draw the line segments and handles around segment vertices, the
 * latter if the they are in proximity to cursor.
 **/

static void
gimp_select_by_content_tool_draw (GimpDrawTool *draw_tool)
{
  GimpSelectByContentTool    *sbc = GIMP_SELECT_BY_CONTENT_TOOL (draw_tool);
  GimpIscissorsOptions *options   = GIMP_SELECT_BY_CONTENT_TOOL_GET_OPTIONS (draw_tool);
  GimpSelectByContentToolPrivate *priv                  = GET_PRIVATE (sbc);
  GimpTool                  *tool                  = GIMP_TOOL (draw_tool);
  GimpCanvasGroup           *stroke_group;
  gboolean                   hovering_first_point  = FALSE;
  gboolean                   handles_wants_to_show = FALSE;
  GimpCoords                 coords                = { priv->last_coords.x,
                                                       priv->last_coords.y,
                                                       /* pad with 0 */ };
  if (sbc->magnetic)
  {  
  if (sbc->state == SEED_PLACEMENT)
    {
      /*  Draw the crosshairs target if we're placing a seed  */
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_CROSS,
                                 sbc->x, sbc->y,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);

      /* Draw a line boundary */
      if (! sbc->first_point)
        {
          if (! options->interactive)
            {
              gimp_draw_tool_add_line (draw_tool,
                                       sbc->ix, sbc->iy,
                                       sbc->x, sbc->y);
            }
          else
            {
              /* See if the mouse has moved.  If so, create a new segment... */
              if (! sbc->livewire ||
                  (sbc->livewire &&
                   (sbc->ix != sbc->livewire->x1 ||
                    sbc->x  != sbc->livewire->x2  ||
                    sbc->iy != sbc->livewire->y1 ||
                    sbc->y  != sbc->livewire->y2)))
                {
                  ICurve *curve = g_slice_new (ICurve);

                  curve->x1 = sbc->ix;
                  curve->y1 = sbc->iy;
                  curve->x2 = sbc->x;
                  curve->y2 = sbc->y;
                  curve->points = NULL;

                  if (sbc->livewire)
                    {
                      if (sbc->livewire->points)
                        g_ptr_array_free (sbc->livewire->points, TRUE);

                      g_slice_free (ICurve, sbc->livewire);

                      sbc->livewire = NULL;
                    }

                  sbc->livewire = curve;
                  calculate_curve (sbc, curve);
                }

              /*  plot the curve  */
              iscissors_draw_curve (draw_tool, sbc->livewire);
            }
        }
    }

  if (! sbc->first_point)
    {
      GList *list;

      /*  Draw a point at the init point coordinates  */
      if (! sbc->connected)
        {
          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_FILLED_CIRCLE,
                                     sbc->ix,
                                     sbc->iy,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);
        }

      /*  Go through the list of icurves, and render each one...  */
      for (list = g_queue_peek_head_link (sbc->curves);
           list;
           list = g_list_next (list))
        {
          ICurve *curve = list->data;

          if (sbc->state == SEED_ADJUSTMENT)
            {
              /*  don't draw curve1 at all  */
              if (curve == sbc->curve1)
                continue;
            }

          gimp_draw_tool_add_handle (draw_tool,
                                     GIMP_HANDLE_FILLED_CIRCLE,
                                     curve->x1,
                                     curve->y1,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                     GIMP_HANDLE_ANCHOR_CENTER);

          if (sbc->state == SEED_ADJUSTMENT)
            {
              /*  draw only the start handle of curve2  */
              if (curve == sbc->curve2)
                continue;
            }

          /*  plot the curve  */
          iscissors_draw_curve (draw_tool, curve);
        }
    }

  if (sbc->state == SEED_ADJUSTMENT)
    {
      /*  plot both curves, and the control point between them  */
      if (sbc->curve1)
        {
          gimp_draw_tool_add_line (draw_tool,
                                   sbc->curve1->x2,
                                   sbc->curve1->y2,
                                   sbc->nx,
                                   sbc->ny);
        }

      if (sbc->curve2)
        {
          gimp_draw_tool_add_line (draw_tool,
                                   sbc->curve2->x1,
                                   sbc->curve2->y1,
                                   sbc->nx,
                                   sbc->ny);
        }

      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_FILLED_CIRCLE,
                                 sbc->nx,
                                 sbc->ny,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);
    }
  }
  
  else
  {
     if (! tool->display)
    return;

  hovering_first_point =
    gimp_select_by_content_tool_should_close (sbc,
                                        tool->display,
                                        NO_CLICK_TIME_AVAILABLE,
                                        &coords);

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  gimp_draw_tool_push_group (draw_tool, stroke_group);
  gimp_draw_tool_add_lines (draw_tool,
                            priv->points, priv->n_points,
                            FALSE);
  gimp_draw_tool_pop_group (draw_tool);

  /* We always show the handle for the first point, even with button1
   * down, since releasing the button on the first point will close
   * the polygon, so it's a significant state which we must give
   * feedback for
   */
  handles_wants_to_show = (hovering_first_point ||
                           ! gimp_tool_control_is_active (tool->control));

  if (handles_wants_to_show &&
      ! priv->supress_handles)
    {
      gint i = 0;
      gint n = 0;

      /* If the first point is hovered while button1 is held down,
       * only draw the first handle, the other handles are not
       * relevant (see comment a few lines up)
       */
      if (gimp_tool_control_is_active (tool->control) && hovering_first_point)
        n = MIN (priv->n_segment_indices, 1);
      else
        n = priv->n_segment_indices;

      for (i = 0; i < n; i++)
        {
          GimpVector2   *point       = NULL;
          gdouble        dist        = 0.0;
          GimpHandleType handle_type = -1;

          point = &priv->points[priv->segment_indices[i]];

          dist  = gimp_draw_tool_calc_distance_square (draw_tool,
                                                       tool->display,
                                                       priv->last_coords.x,
                                                       priv->last_coords.y,
                                                       point->x,
                                                       point->y);

          /* If the cursor is over the point, fill, if it's just
           * close, draw an outline
           */
          if (dist < POINT_GRAB_THRESHOLD_SQ)
            handle_type = GIMP_HANDLE_FILLED_CIRCLE;
          else if (dist < POINT_SHOW_THRESHOLD_SQ)
            handle_type = GIMP_HANDLE_CIRCLE;

          if (handle_type != -1)
            {
              GimpCanvasItem *item;

              item = gimp_draw_tool_add_handle (draw_tool, handle_type,
                                                point->x,
                                                point->y,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                                GIMP_HANDLE_ANCHOR_CENTER);

              if (dist < POINT_GRAB_THRESHOLD_SQ)
                gimp_canvas_item_set_highlight (item, TRUE);
            }
        }
    }

  if (priv->show_pending_point)
    {
      GimpVector2 last = priv->points[priv->n_points - 1];

      gimp_draw_tool_push_group (draw_tool, stroke_group);
      gimp_draw_tool_add_line (draw_tool,
                               last.x,
                               last.y,
                               priv->pending_point.x,
                               priv->pending_point.y);
      gimp_draw_tool_pop_group (draw_tool);
    }

  }  
}


static void
iscissors_draw_curve (GimpDrawTool *draw_tool,
                      ICurve       *curve)
{
  GimpVector2 *points;
  gpointer    *point;
  gint         i, len;

  if (! curve->points)
    return;

  len = curve->points->len;

  points = g_new (GimpVector2, len);

  for (i = 0, point = curve->points->pdata; i < len; i++, point++)
    {
      guint32 coords = GPOINTER_TO_INT (*point);

      points[i].x = (coords & 0x0000ffff);
      points[i].y = (coords >> 16);
    }

  gimp_draw_tool_add_lines (draw_tool, points, len, FALSE);

  g_free (points);
}

static void
gimp_select_by_content_tool_oper_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);
  gboolean                   hovering_first_point;

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
  /* parent sets a message in the status bar, but it will be replaced here */
  if (sbc->magnetic)
  {
  if (mouse_over_vertex (sbc, coords->x, coords->y) > 1)
    {
      gchar *status;

      status = gimp_suggest_modifiers (_("Click-Drag to move this point"),
                                       GDK_SHIFT_MASK & ~state,
                                       _("%s: disable auto-snap"), NULL, NULL);
      gimp_tool_replace_status (tool, display, "%s", status);
      g_free (status);
      sbc->op = ISCISSORS_OP_MOVE_POINT;
    }
  else if (mouse_over_curve (sbc, coords->x, coords->y))
    {
      ICurve *curve = g_queue_peek_head (sbc->curves);

      if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                    RINT (coords->x), RINT (coords->y),
                                    GIMP_HANDLE_CIRCLE,
                                    curve->x1, curve->y1,
                                    GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                    GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                    GIMP_HANDLE_ANCHOR_CENTER))
        {
          gimp_tool_replace_status (tool, display, _("Click to close the"
                                                     " curve"));
          sbc->op = ISCISSORS_OP_CONNECT;
        }
      else
        {
          gimp_tool_replace_status (tool, display, _("Click to add a point"
                                                     " on this segment"));
          sbc->op = ISCISSORS_OP_ADD_POINT;
        }
    }
  else if (sbc->connected && sbc->mask)
    {
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (sbc->mask),
                                        RINT (coords->x),
                                        RINT (coords->y)))
        {
          if (proximity)
            {
              gimp_tool_replace_status (tool, display,
                                        _("Click or press Enter to convert to"
                                          " a selection"));
            }
          sbc->op = ISCISSORS_OP_SELECT;
        }
      else
        {
          if (proximity)
            {
              gimp_tool_replace_status (tool, display,
                                        _("Press Enter to convert to a"
                                          " selection"));
            }
          sbc->op = ISCISSORS_OP_IMPOSSIBLE;
        }
    }
  else
    {
      switch (sbc->state)
        {
        case WAITING:
          if (proximity)
            {
              gchar *status;

              status = gimp_suggest_modifiers (_("Click or Click-Drag to add a"
                                                 " point"),
                                               GDK_SHIFT_MASK & ~state,
                                               _("%s: disable auto-snap"),
                                               NULL, NULL);
              gimp_tool_replace_status (tool, display, "%s", status);
              g_free (status);
            }
          sbc->op = ISCISSORS_OP_ADD_POINT;
          break;

        default:
          /* if NO_ACTION, keep parent's status bar message (selection tool) */
          sbc->op = ISCISSORS_OP_NONE;
          break;
        }
    }
  }
  else 
  {
  gimp_select_by_content_tool_handle_segment_selection (sbc,
                                                  display,
                                                  coords);
  hovering_first_point =
    gimp_select_by_content_tool_should_close (sbc,
                                        display,
                                        NO_CLICK_TIME_AVAILABLE,
                                        coords);

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  priv->last_coords.x = coords->x;
  priv->last_coords.y = coords->y;

  if (priv->n_points == 0 ||
      (gimp_select_by_content_tool_is_point_grabbed (sbc) &&
       ! hovering_first_point) ||
      ! proximity)
    {
      priv->show_pending_point = FALSE;
    }
  else
    {
      priv->show_pending_point = TRUE;

      if (hovering_first_point)
        {
          priv->pending_point = priv->points[0];
        }
      else
        {
          priv->pending_point.x = coords->x;
          priv->pending_point.y = coords->y;

          if (priv->constrain_angle && priv->n_points > 0)
            {
              gdouble start_point_x;
              gdouble start_point_y;

              gimp_select_by_content_tool_get_last_point (sbc,
                                                    &start_point_x,
                                                    &start_point_y);

              gimp_constrain_line (start_point_x, start_point_y,
                                   &priv->pending_point.x,
                                   &priv->pending_point.y,
                                   GIMP_CONSTRAIN_LINE_15_DEGREES);
            }
        }
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool)); 
  }  
}

static void
gimp_select_by_content_tool_cursor_update (GimpTool         *tool,
                                   const GimpCoords *coords,
                                   GdkModifierType   state,
                                   GimpDisplay      *display)
{
  GimpSelectByContentTool  *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);
  GimpCursorModifier  modifier  = GIMP_CURSOR_MODIFIER_NONE;

  if (sbc->magnetic)
  {
   switch (sbc->op)
    {
    case ISCISSORS_OP_SELECT:
      {
        GimpSelectionOptions *options;

        options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

        /* Do not overwrite the modifiers for add, subtract, intersect */
        if (options->operation == GIMP_CHANNEL_OP_REPLACE)
          {
            modifier = GIMP_CURSOR_MODIFIER_SELECT;
          }
      }
      break;

    case ISCISSORS_OP_MOVE_POINT:
      modifier = GIMP_CURSOR_MODIFIER_MOVE;
      break;

    case ISCISSORS_OP_ADD_POINT:
      modifier = GIMP_CURSOR_MODIFIER_PLUS;
      break;

    case ISCISSORS_OP_CONNECT:
      modifier = GIMP_CURSOR_MODIFIER_JOIN;
      break;

    case ISCISSORS_OP_IMPOSSIBLE:
      modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;

    default:
      break;
    }

  if (modifier != GIMP_CURSOR_MODIFIER_NONE)
    {
      gimp_tool_set_cursor (tool, display,
                            GIMP_CURSOR_MOUSE,
                            GIMP_TOOL_CURSOR_ISCISSORS,
                            modifier);
    }
  else
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords,
                                                     state, display);
    }
  }
  else
  {
    if (tool->display == NULL)
    {
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool,
                                                     coords,
                                                     state,
                                                     display);
    }
 
 else
    {
      GimpCursorModifier modifier;

      if (gimp_select_by_content_tool_is_point_grabbed (sbc) &&
          ! gimp_select_by_content_select_tool_should_close (sbc,
                                                display,
                                                NO_CLICK_TIME_AVAILABLE,
                                                coords))
        {
          modifier = GIMP_CURSOR_MODIFIER_MOVE;
        }
      else
        {
          modifier = GIMP_CURSOR_MODIFIER_NONE;
        }

      gimp_tool_set_cursor (tool, display,
                            gimp_tool_control_get_cursor (tool->control),
                            gimp_tool_control_get_tool_cursor (tool->control),
                            modifier);
    }
  }
}

static gboolean
gimp_select_by_content_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpSelectByContentTool *sbc = GIMP_SELECT_BY_CONTENT_TOOL (tool);

  if (display != tool->display)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
    if (sbc->magnetic)
    {
      if (sbc->connected && sbc->mask)
        {
          gimp_select_by_content_tool_apply (sbc, display);
          return TRUE;
        }
      return FALSE;
    }

    else
    {
      gimp_select_by_content_tool_commit (sbc, display);
      return TRUE; 
    }  

    case GDK_KEY_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    case GDK_KEY_BackSpace:
      gimp_select_by_content_tool_remove_last_segment (sbc);
      return TRUE;  

    default:
      return FALSE;
    }
    return FALSE;
}

static void
gimp_select_by_content_tool_apply (GimpSelectByContentTool *sbc,
                           GimpDisplay       *display)
{
  GimpTool             *tool    = GIMP_TOOL (sbc);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);
  GimpImage            *image   = gimp_display_get_image (display);

  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_channel_select_channel (gimp_image_get_mask (image),
                               tool->tool_info->blurb,
                               iscissors->mask,
                               0, 0,
                               options->operation,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);

  gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);

  gimp_image_flush (image);
}


/* XXX need some scan-conversion routines from somewhere.  maybe. ? */

static gint
mouse_over_vertex (GimpSelectByContentTool *sbc,
                   gdouble            x,
                   gdouble            y)
{
  GList *list;
  gint   curves_found = 0;

  /*  traverse through the list, returning non-zero if the current cursor
   *  position is on an existing curve vertex.  Set the curve1 and curve2
   *  variables to the two curves containing the vertex in question
   */

  sbc->curve1 = sbc->curve2 = NULL;

  for (list = g_queue_peek_head_link (sbc->curves);
       list;
       list = g_list_next (list))
    {
      ICurve *curve = list->data;

      if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (sbc),
                                    GIMP_TOOL (sbc)->display,
                                    x, y,
                                    GIMP_HANDLE_CIRCLE,
                                    curve->x1, curve->y1,
                                    GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                    GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                    GIMP_HANDLE_ANCHOR_CENTER))
        {
          sbc->curve1 = curve;

          if (curves_found++)
            return curves_found;
        }
      else if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (sbc),
                                         GIMP_TOOL (sbc)->display,
                                         x, y,
                                         GIMP_HANDLE_CIRCLE,
                                         curve->x2, curve->y2,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                         GIMP_HANDLE_ANCHOR_CENTER))
        {
          sbc->curve2 = curve;

          if (curves_found++)
            return curves_found;
        }
    }

  return curves_found;
}

static gboolean
clicked_on_vertex (GimpSelectByContentTool *sbc,
                   gdouble            x,
                   gdouble            y)
{
  gint curves_found = 0;

  curves_found = mouse_over_vertex (sbc, x, y);

  if (curves_found > 1)
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (sbc));

      return TRUE;
    }

  /*  if only one curve was found, the curves are unconnected, and
   *  the user only wants to move either the first or last point
   *  disallow this for now.
   */
  if (curves_found == 1)
    return FALSE;

  return clicked_on_curve (sbc, x, y);
}


static GList *
mouse_over_curve (GimpSelectByContentTool *sbc,
                  gdouble            x,
                  gdouble            y)
{
  GList *list;

  /*  traverse through the list, returning the curve segment's list element
   *  if the current cursor position is on a curve...
   */
  for (list = g_queue_peek_head_link (sbc->curves);
       list;
       list = g_list_next (list))
    {
      ICurve   *curve = list->data;
      gpointer *pt;
      gint      len;

      pt = curve->points->pdata;
      len = curve->points->len;

      while (len--)
        {
          guint32 coords = GPOINTER_TO_INT (*pt);
          gint    tx, ty;

          pt++;
          tx = coords & 0x0000ffff;
          ty = coords >> 16;

          /*  Is the specified point close enough to the curve?  */
          if (gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (sbc),
                                                   GIMP_TOOL (sbc)->display,
                                                   tx, ty,
                                                   x, y) < SQR (GIMP_TOOL_HANDLE_SIZE_CIRCLE / 2))
            {
              return list;
            }
        }
    }

  return NULL;
}

static gboolean
clicked_on_curve (GimpSelectByContentTool *sbc,
                  gdouble            x,
                  gdouble            y)
{
  GList *list = mouse_over_curve (sbc, x, y);

  /*  traverse through the list, getting back the curve segment's list
   *  element if the current cursor position is on a curve...
   *  If this occurs, replace the curve with two new curves,
   *  separated by a new vertex.
   */

  if (list)
    {
      ICurve *curve = list->data;
      ICurve *new_curve;

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (sbc));

      /*  Create the new curve  */
      new_curve = g_slice_new (ICurve);

      new_curve->x2 = curve->x2;
      new_curve->y2 = curve->y2;
      new_curve->x1 = curve->x2 = sbc->x;
      new_curve->y1 = curve->y2 = sbc->y;
      new_curve->points = NULL;

      /*  Create the new link and supply the new curve as data  */
      g_queue_insert_after (sbc->curves, list, new_curve);

      sbc->curve1 = new_curve;
      sbc->curve2 = curve;

      return TRUE;
    }

  return FALSE;
}


static void
calculate_curve (GimpSelectByContentTool *sbc,
                 ICurve            *curve)
{
  GimpDisplay *display   = GIMP_TOOL (sbc)->display;
  GimpImage   *image     = gimp_display_get_image (display);
  gint         x, y, dir;
  gint         xs, ys, xe, ye;
  gint         x1, y1, x2, y2;
  gint         width, height;
  gint         ewidth, eheight;

  /*  Calculate the lowest cost path from one vertex to the next as specified
   *  by the parameter "curve".
   *    Here are the steps:
   *      1)  Calculate the appropriate working area for this operation
   *      2)  Allocate a temp buf for the dynamic programming array
   *      3)  Run the dynamic programming algorithm to find the optimal path
   *      4)  Translate the optimal path into pixels in the icurve data
   *            structure.
   */

  /*  Get the bounding box  */
  xs = CLAMP (curve->x1, 0, gimp_image_get_width  (image) - 1);
  ys = CLAMP (curve->y1, 0, gimp_image_get_height (image) - 1);
  xe = CLAMP (curve->x2, 0, gimp_image_get_width  (image) - 1);
  ye = CLAMP (curve->y2, 0, gimp_image_get_height (image) - 1);
  x1 = MIN (xs, xe);
  y1 = MIN (ys, ye);
  x2 = MAX (xs, xe) + 1;  /*  +1 because if xe = 199 & xs = 0, x2 - x1, width = 200  */
  y2 = MAX (ys, ye) + 1;

  /*  expand the boundaries past the ending points by
   *  some percentage of width and height.  This serves the following purpose:
   *  It gives the algorithm more area to search so better solutions
   *  are found.  This is particularly helpful in finding "bumps" which
   *  fall outside the bounding box represented by the start and end
   *  coordinates of the "curve".
   */
  ewidth  = (x2 - x1) * EXTEND_BY + FIXED;
  eheight = (y2 - y1) * EXTEND_BY + FIXED;

  if (xe >= xs)
    x2 += CLAMP (ewidth, 0, gimp_image_get_width (image) - x2);
  else
    x1 -= CLAMP (ewidth, 0, x1);

  if (ye >= ys)
    y2 += CLAMP (eheight, 0, gimp_image_get_height (image) - y2);
  else
    y1 -= CLAMP (eheight, 0, y1);

  /* blow away any previous points list we might have */
  if (curve->points)
    {
      g_ptr_array_free (curve->points, TRUE);
      curve->points = NULL;
    }

  /*  If the bounding box has width and height...  */
  if ((x2 - x1) && (y2 - y1))
    {
      width = (x2 - x1);
      height = (y2 - y1);

      /* Initialise the gradient map tile manager for this image if we
       * don't already have one. */
      if (!sbc->gradient_map)
          sbc->gradient_map = gradient_map_new (image);

      /*  allocate the dynamic programming array  */
      sbc->dp_buf =
        temp_buf_resize (sbc->dp_buf, 4, x1, y1, width, height);

      /*  find the optimal path of pixels from (x1, y1) to (x2, y2)  */
      find_optimal_path (sbc->gradient_map, sbc->dp_buf,
                         x1, y1, x2, y2, xs, ys);

      /*  get a list of the pixels in the optimal path  */
      curve->points = plot_pixels (sbc, sbc->dp_buf,
                                   x1, y1, xs, ys, xe, ye);
    }
  /*  If the bounding box has no width  */
  else if ((x2 - x1) == 0)
    {
      /*  plot a vertical line  */
      y = ys;
      dir = (ys > ye) ? -1 : 1;
      curve->points = g_ptr_array_new ();
      while (y != ye)
        {
          g_ptr_array_add (curve->points, GINT_TO_POINTER ((y << 16) + xs));
          y += dir;
        }
    }
  /*  If the bounding box has no height  */
  else if ((y2 - y1) == 0)
    {
      /*  plot a horizontal line  */
      x = xs;
      dir = (xs > xe) ? -1 : 1;
      curve->points = g_ptr_array_new ();
      while (x != xe)
        {
          g_ptr_array_add (curve->points, GINT_TO_POINTER ((ys << 16) + x));
          x += dir;
        }
    }
}


/* badly need to get a replacement - this is _way_ too expensive */
static gboolean
gradient_map_value (TileManager *map,
                    gint         x,
                    gint         y,
                    guint8      *grad,
                    guint8      *dir)
{
  static gint   cur_tilex;
  static gint   cur_tiley;
  const guint8 *p;

  if (! cur_tile ||
      x / TILE_WIDTH != cur_tilex ||
      y / TILE_HEIGHT != cur_tiley)
    {
      if (cur_tile)
        tile_release (cur_tile, FALSE);

      cur_tile = tile_manager_get_tile (map, x, y, TRUE, FALSE);

      if (!cur_tile)
        return FALSE;

      cur_tilex = x / TILE_WIDTH;
      cur_tiley = y / TILE_HEIGHT;
    }

  p = tile_data_pointer (cur_tile, x, y);

  *grad = p[0];
  *dir  = p[1];

  return TRUE;
}

static gint
calculate_link (TileManager *gradient_map,
                gint         x,
                gint         y,
                guint32      pixel,
                gint         link)
{
  gint   value = 0;
  guint8 grad1, dir1, grad2, dir2;

  if (!gradient_map_value (gradient_map, x, y, &grad1, &dir1))
    {
      grad1 = 0;
      dir1 = 255;
    }

  /* Convert the gradient into a cost: large gradients are good, and
   * so have low cost. */
  grad1 = 255 - grad1;

  /*  calculate the contribution of the gradient magnitude  */
  if (link > 1)
    value += diagonal_weight[grad1] * OMEGA_G;
  else
    value += grad1 * OMEGA_G;

  /*  calculate the contribution of the gradient direction  */
  x += (gint8)(pixel & 0xff);
  y += (gint8)((pixel & 0xff00) >> 8);

  if (!gradient_map_value (gradient_map, x, y, &grad2, &dir2))
    {
      grad2 = 0;
      dir2 = 255;
    }

  value +=
    (direction_value[dir1][link] + direction_value[dir2][link]) * OMEGA_D;

  return value;
}


static GPtrArray *
plot_pixels (GimpSelectByContentTool *sbc,
             TempBuf           *dp_buf,
             gint               x1,
             gint               y1,
             gint               xs,
             gint               ys,
             gint               xe,
             gint               ye)
{
  gint       x, y;
  guint32    coords;
  gint       link;
  gint       width;
  guint     *data;
  GPtrArray *list;

  width = dp_buf->width;

  /*  Start the data pointer at the correct location  */
  data = (guint *) temp_buf_get_data (dp_buf) + (ye - y1) * width + (xe - x1);

  x = xe;
  y = ye;

  list = g_ptr_array_new ();

  while (TRUE)
    {
      coords = (y << 16) + x;
      g_ptr_array_add (list, GINT_TO_POINTER (coords));

      link = PIXEL_DIR (*data);
      if (link == SEED_POINT)
        return list;

      x += move[link][0];
      y += move[link][1];
      data += move[link][1] * width + move[link][0];
    }

  /*  won't get here  */
  return NULL;
}


#define PACK(x, y) ((((y) & 0xff) << 8) | ((x) & 0xff))
#define OFFSET(pixel) ((gint8)((pixel) & 0xff) + \
  ((gint8)(((pixel) & 0xff00) >> 8)) * dp_buf->width)


static void
find_optimal_path (TileManager *gradient_map,
                   TempBuf     *dp_buf,
                   gint         x1,
                   gint         y1,
                   gint         x2,
                   gint         y2,
                   gint         xs,
                   gint         ys)
{
  gint     i, j, k;
  gint     x, y;
  gint     link;
  gint     linkdir;
  gint     dirx, diry;
  gint     min_cost;
  gint     new_cost;
  gint     offset;
  gint     cum_cost[8];
  gint     link_cost[8];
  gint     pixel_cost[8];
  guint32  pixel[8];
  guint32 *data;
  guint32 *d;

  /*  initialize the dynamic programming buffer  */
  data = (guint32 *) temp_buf_data_clear (dp_buf);

  /*  what directions are we filling the array in according to?  */
  dirx = (xs - x1 == 0) ? 1 : -1;
  diry = (ys - y1 == 0) ? 1 : -1;
  linkdir = (dirx * diry);

  y = ys;

  for (i = 0; i < dp_buf->height; i++)
    {
      x = xs;

      d = data + (y-y1) * dp_buf->width + (x-x1);

      for (j = 0; j < dp_buf->width; j++)
        {
          min_cost = G_MAXINT;

          /* pixel[] array encodes how to get to a neigbour, if possible.
           * 0 means no connection (eg edge).
           * Rest packed as bottom two bytes: y offset then x offset.
           * Initially, we assume we can't get anywhere. */
          for (k = 0; k < 8; k++)
            pixel[k] = 0;

          /*  Find the valid neighboring pixels  */
          /*  the previous pixel  */
          if (j)
            pixel[((dirx == 1) ? 4 : 0)] = PACK (-dirx, 0);

          /*  the previous row of pixels  */
          if (i)
            {
              pixel[((diry == 1) ? 5 : 1)] = PACK (0, -diry);

              link = (linkdir == 1) ? 3 : 2;
              if (j)
                pixel[((diry == 1) ? (link + 4) : link)] = PACK(-dirx, -diry);

              link = (linkdir == 1) ? 2 : 3;
              if (j != dp_buf->width - 1)
                pixel[((diry == 1) ? (link + 4) : link)] = PACK (dirx, -diry);
            }

          /*  find the minimum cost of going through each neighbor to reach the
           *  seed point...
           */
          link = -1;
          for (k = 0; k < 8; k ++)
            if (pixel[k])
              {
                link_cost[k] = calculate_link (gradient_map,
                                               xs + j*dirx, ys + i*diry,
                                               pixel [k],
                                               ((k > 3) ? k - 4 : k));
                offset = OFFSET (pixel [k]);
                pixel_cost[k] = PIXEL_COST (d[offset]);
                cum_cost[k] = pixel_cost[k] + link_cost[k];
                if (cum_cost[k] < min_cost)
                  {
                    min_cost = cum_cost[k];
                    link = k;
                  }
              }

          /*  If anything can be done...  */
          if (link >= 0)
            {
              /*  set the cumulative cost of this pixel and the new direction  */
              *d = (cum_cost[link] << 8) + link;

              /*  possibly change the links from the other pixels to this pixel...
               *  these changes occur if a neighboring pixel will receive a lower
               *  cumulative cost by going through this pixel.
               */
              for (k = 0; k < 8; k ++)
                if (pixel[k] && k != link)
                  {
                    /*  if the cumulative cost at the neighbor is greater than
                     *  the cost through the link to the current pixel, change the
                     *  neighbor's link to point to the current pixel.
                     */
                    new_cost = link_cost[k] + cum_cost[link];
                    if (pixel_cost[k] > new_cost)
                    {
                      /*  reverse the link direction   /-----------------------\ */
                      offset = OFFSET (pixel[k]);
                      d[offset] = (new_cost << 8) + ((k > 3) ? k - 4 : k + 4);
                    }
                  }
            }
          /*  Set the seed point  */
          else if (!i && !j)
            *d = SEED_POINT;

          /*  increment the data pointer and the x counter  */
          d += dirx;
          x += dirx;
        }

      /*  increment the y counter  */
      y += diry;
    }
}


/* Called to fill in a newly referenced tile in the gradient map */
static void
gradmap_tile_validate (TileManager *tm,
                       Tile        *tile,
                       GimpImage   *image)
{
  static gboolean first_gradient = TRUE;

  GimpPickable *pickable;
  Tile         *srctile;
  PixelRegion   srcPR;
  PixelRegion   destPR;
  gint          x, y;
  gint          dw, dh;
  gint          sw, sh;
  gint          i, j;
  gint          b;
  gfloat        gradient;
  guint8       *tiledata;
  guint8       *gradmap;

  if (first_gradient)
    {
      gint radius = GRADIENT_SEARCH >> 1;

      /*  compute the distance weights  */
      for (i = 0; i < GRADIENT_SEARCH; i++)
        for (j = 0; j < GRADIENT_SEARCH; j++)
          distance_weights[i * GRADIENT_SEARCH + j] =
            1.0 / (1 + sqrt (SQR (i - radius) + SQR (j - radius)));

      first_gradient = FALSE;
    }

  tile_manager_get_tile_coordinates (tm, tile, &x, &y);

  dw = tile_ewidth (tile);
  dh = tile_eheight (tile);

  pickable = GIMP_PICKABLE (gimp_image_get_projection (image));

  gimp_pickable_flush (pickable);

  /* get corresponding tile in the image */
  srctile = tile_manager_get_tile (gimp_pickable_get_tiles (pickable),
                                   x, y, TRUE, FALSE);
  if (! srctile)
    return;

  sw = tile_ewidth (srctile);
  sh = tile_eheight (srctile);

  pixel_region_init_data (&srcPR,
                          tile_data_pointer (srctile, 0, 0),
                          gimp_pickable_get_bytes (pickable),
                          gimp_pickable_get_bytes (pickable) *
                          MIN (dw, sw),
                          0, 0, MIN (dw, sw), MIN (dh, sh));


  /* XXX tile edges? */

  /*  Blur the source to get rid of noise  */
  pixel_region_init_data (&destPR, maxgrad_conv0, 4, TILE_WIDTH * 4,
                          0, 0, srcPR.w, srcPR.h);
  convolve_region (&srcPR, &destPR, blur_32, 3, 32, GIMP_NORMAL_CONVOL, FALSE);

  /*  Use the blurred region as the new source pixel region  */
  pixel_region_init_data (&srcPR, maxgrad_conv0, 4, TILE_WIDTH * 4,
                          0, 0, srcPR.w, srcPR.h);

  /*  Get the horizontal derivative  */
  pixel_region_init_data (&destPR, maxgrad_conv1, 4, TILE_WIDTH * 4,
                          0, 0, srcPR.w, srcPR.h);
  convolve_region (&srcPR, &destPR, horz_deriv, 3, 1, GIMP_NEGATIVE_CONVOL,
                   FALSE);

  /*  Get the vertical derivative  */
  pixel_region_init_data (&destPR, maxgrad_conv2, 4, TILE_WIDTH * 4,
                          0, 0, srcPR.w, srcPR.h);
  convolve_region (&srcPR, &destPR, vert_deriv, 3, 1, GIMP_NEGATIVE_CONVOL,
                   FALSE);

  /* calculate overall gradient */
  tiledata = tile_data_pointer (tile, 0, 0);

  for (i = 0; i < srcPR.h; i++)
    {
      const guint8 *datah = maxgrad_conv1 + srcPR.rowstride * i;
      const guint8 *datav = maxgrad_conv2 + srcPR.rowstride * i;

      gradmap = tiledata + tile_ewidth (tile) * COST_WIDTH * i;

      for (j = 0; j < srcPR.w; j++)
        {
          gint8 hmax = datah[0] - 128;
          gint8 vmax = datav[0] - 128;

          for (b = 1; b < srcPR.bytes; b++)
            {
              if (abs (datah[b] - 128) > abs (hmax))
                hmax = datah[b] - 128;

              if (abs (datav[b] - 128) > abs (vmax))
                vmax = datav[b] - 128;
            }

          if (i == 0 || j == 0 || i == srcPR.h-1 || j == srcPR.w-1)
            {
              gradmap[j * COST_WIDTH + 0] = 0;
              gradmap[j * COST_WIDTH + 1] = 255;
              goto contin;
            }

          /* 1 byte absolute magnitude first */
          gradient = sqrt (SQR (hmax) + SQR (vmax));
          gradmap[j * COST_WIDTH] = gradient * 255 / MAX_GRADIENT;

          /* then 1 byte direction */
          if (gradient > MIN_GRADIENT)
            {
              gfloat direction;

              if (!hmax)
                direction = (vmax > 0) ? G_PI_2 : -G_PI_2;
              else
                direction = atan ((gdouble) vmax / (gdouble) hmax);

              /* Scale the direction from between 0 and 254,
               *  corresponding to -PI/2, PI/2 255 is reserved for
               *  d9irectionless pixels */
              gradmap[j * COST_WIDTH + 1] =
                (guint8) (254 * (direction + G_PI_2) / G_PI);
            }
          else
            {
              gradmap[j * COST_WIDTH + 1] = 255; /* reserved for weak gradient */
            }

        contin:
          datah += srcPR.bytes;
          datav += srcPR.bytes;
        }
    }

  tile_release (srctile, FALSE);
}

static TileManager *
gradient_map_new (GimpImage *image)
{
  TileManager *tm;

  tm = tile_manager_new (gimp_image_get_width  (image),
                         gimp_image_get_height (image),
                         sizeof (guint8) * COST_WIDTH);

  tile_manager_set_validate_proc (tm,
                                  (TileValidateProc) gradmap_tile_validate,
                                  image);

  return tm;
}

static void
find_max_gradient (GimpSelectByContentTool *sbc,
                   GimpImage         *image,
                   gint              *x,
                   gint              *y)
{
  PixelRegion  srcPR;
  gint         radius;
  gint         i, j;
  gint         endx, endy;
  gint         cx, cy;
  gint         x1, y1, x2, y2;
  gpointer     pr;
  gfloat       max_gradient;

  /* Initialise the gradient map tile manager for this image if we
   * don't already have one. */
  if (! sbc->gradient_map)
    sbc->gradient_map = gradient_map_new (image);

  radius = GRADIENT_SEARCH >> 1;

  /*  calculate the extent of the search  */
  cx = CLAMP (*x, 0, gimp_image_get_width  (image));
  cy = CLAMP (*y, 0, gimp_image_get_height (image));
  x1 = CLAMP (cx - radius, 0, gimp_image_get_width  (image));
  y1 = CLAMP (cy - radius, 0, gimp_image_get_height (image));
  x2 = CLAMP (cx + radius, 0, gimp_image_get_width  (image));
  y2 = CLAMP (cy + radius, 0, gimp_image_get_height (image));
  /*  calculate the factor to multiply the distance from the cursor by  */

  max_gradient = 0;
  *x = cx;
  *y = cy;

  /*  Find the point of max gradient  */
  pixel_region_init (&srcPR, iscissors->gradient_map,
                     x1, y1, x2 - x1, y2 - y1, FALSE);

  /* this iterates over 1, 2 or 4 tiles only */
  for (pr = pixel_regions_register (1, &srcPR);
       pr != NULL;
       pr = pixel_regions_process (pr))
    {
      endx = srcPR.x + srcPR.w;
      endy = srcPR.y + srcPR.h;

      for (i = srcPR.y; i < endy; i++)
        {
          const guint8 *gradient = srcPR.data + srcPR.rowstride * (i - srcPR.y);

          for (j = srcPR.x; j < endx; j++)
            {
              gfloat g = *gradient;

              gradient += COST_WIDTH;

              g *= distance_weights [(i-y1) * GRADIENT_SEARCH + (j-x1)];

              if (g > max_gradient)
                {
                  max_gradient = g;

                  *x = j;
                  *y = i;
                }
            }
        }
    }
}

/*Free Select Tool Functions*/
static void
gimp_select_by_content_tool_get_segment (GimpSelectByContetnTool  *sbc
                                   GimpVector2        **points,
                                   gint                *n_points,
                                   gint                 segment_start,
                                   gint                 segment_end)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  *points   = &priv->points[priv->segment_indices[segment_start]];
  *n_points = priv->segment_indices[segment_end] -
              priv->segment_indices[segment_start] +
              1;
}

static void
gimp_select_by_content_tool_get_segment_point (GimpSelectByContentTool *sbc,
                                         gdouble            *start_point_x,
                                         gdouble            *start_point_y,
                                         gint                segment_index)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  *start_point_x = priv->points[priv->segment_indices[segment_index]].x;
  *start_point_y = priv->points[priv->segment_indices[segment_index]].y;
}

static void
gimp_select_by_content_tool_get_last_point (GimpSelectByContentTool  *sbc,
                                      gdouble            *start_point_x,
                                      gdouble            *start_point_y)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  gimp_select_by_content_tool_get_segment_point (sbc,
                                           start_point_x,
                                           start_point_y,
                                           priv->n_segment_indices - 1);
}

static void
gimp_select_by_content_tool_get_double_click_settings (gint *double_click_time,
                                                 gint *double_click_distance)
{
  GdkScreen *screen = gdk_screen_get_default ();

  if (screen != NULL)
    {
      GtkSettings *settings = gtk_settings_get_for_screen (screen);

      g_object_get (settings,
                    "gtk-double-click-time",     double_click_time,
                    "gtk-double-click-distance", double_click_distance,
                    NULL);
    }
}

static gboolean
gimp_select_by_content_tool_should_close (GimpSelectByContentTool *sbc,
                                    GimpDisplay        *display,
                                    guint32             time,
                                    const GimpCoords   *coords)
{
  GimpSelectByContentToolPrivate *priv         = GET_PRIVATE (sbc);
  gboolean                   double_click = FALSE;
  gdouble                    dist         = G_MAXDOUBLE;

  if (priv->polygon_modified       ||
      priv->n_segment_indices <= 0 ||
      GIMP_TOOL (sbc)->display == NULL)
    return FALSE;

  dist = gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (sbc),
                                              display,
                                              coords->x,
                                              coords->y,
                                              priv->points[0].x,
                                              priv->points[0].y);

  /* Handle double-click. It must be within GTK+ global double-click
   * time since last click, and it must be within GTK+ global
   * double-click distance away from the last point
   */
  if (time != NO_CLICK_TIME_AVAILABLE)
    {
      gint    double_click_time;
      gint    double_click_distance;
      gint    click_time_passed;
      gdouble dist_from_last_point;

      click_time_passed = time - priv->last_click_time;

      dist_from_last_point =
        gimp_draw_tool_calc_distance_square (GIMP_DRAW_TOOL (sbc),
                                             display,
                                             coords->x,
                                             coords->y,
                                             priv->last_click_coord.x,
                                             priv->last_click_coord.y);

      gimp_select_by_content_tool_get_double_click_settings (&double_click_time,
                                                       &double_click_distance);

      double_click = click_time_passed    < double_click_time &&
                     dist_from_last_point < double_click_distance;
    }

  return ((! priv->supress_handles && dist < POINT_GRAB_THRESHOLD_SQ) ||
          double_click);
}

static void
gimp_select_by_content_tool_handle_segment_selection (GimpSelectByContentTool *sbc,
                                                GimpDisplay        *display,
                                                const GimpCoords   *coords)
{
  GimpSelecByContentToolPrivate *priv                  = GET_PRIVATE (sbc);
  GimpDrawTool              *draw_tool             = GIMP_DRAW_TOOL (sbc);
  gdouble                    shortest_dist         = POINT_GRAB_THRESHOLD_SQ;
  gint                       grabbed_segment_index = INVALID_INDEX;
  gint                       i;

  if (GIMP_TOOL (sbc)->display != NULL &&
      ! priv->supress_handles)
    {
      for (i = 0; i < priv->n_segment_indices; i++)
        {
          gdouble      dist;
          GimpVector2 *point;

          point = &priv->points[priv->segment_indices[i]];

          dist = gimp_draw_tool_calc_distance_square (draw_tool,
                                                      display,
                                                      coords->x,
                                                      coords->y,
                                                      point->x,
                                                      point->y);

          if (dist < shortest_dist)
            {
              grabbed_segment_index = i;
            }
        }
    }

  if (grabbed_segment_index != priv->grabbed_segment_index)
    {
      gimp_draw_tool_pause (draw_tool);

      priv->grabbed_segment_index = grabbed_segment_index;

      gimp_draw_tool_resume (draw_tool);
    }
}

static void
gimp_select_by_content_tool_revert_to_last_segment (GimpSelectByContentTool *sbc)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  priv->n_points = priv->segment_indices[priv->n_segment_indices - 1] + 1;
}

static void
gimp_select_by_content_tool_remove_last_segment (GimpSelectByContentTool *sbc)
{
  GimpSelectByContentToolPrivate *priv      = GET_PRIVATE (sbc);
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (sbc);

  gimp_draw_tool_pause (draw_tool);

  if (priv->n_segment_indices > 0)
    priv->n_segment_indices--;

  if (priv->n_segment_indices <= 0)
    {
      gimp_tool_control (GIMP_TOOL (sbc), GIMP_TOOL_ACTION_HALT,
                         GIMP_TOOL (sbc)->display);
    }
  else
    {
      gimp_select_by_content_tool_revert_to_last_segment (sbc);
    }

  gimp_draw_tool_resume (draw_tool);
}

static void
gimp_select_by_content_tool_add_point (GimpSelectByContentTool *sbc,
                                 gdouble             x,
                                 gdouble             y)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  if (priv->n_points >= priv->max_n_points)
    {
      priv->max_n_points += N_ITEMS_PER_ALLOC;

      priv->points = g_realloc (priv->points,
                                    sizeof (GimpVector2) * priv->max_n_points);
    }

  priv->points[priv->n_points].x = x;
  priv->points[priv->n_points].y = y;

  priv->n_points++;
}

static void
gimp_select_by_content_tool_add_segment_index (GimpSelectByContentTool *sbc,
                                         gint                index)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  if (priv->n_segment_indices >= priv->max_n_segment_indices)
    {
      priv->max_n_segment_indices += N_ITEMS_PER_ALLOC;

      priv->segment_indices = g_realloc (priv->segment_indices,
                                         sizeof (GimpVector2) *
                                         priv->max_n_segment_indices);
    }

  priv->segment_indices[priv->n_segment_indices] = index;

  priv->n_segment_indices++;
}

static gboolean
gimp_select_by_content_tool_is_point_grabbed (GimpSelectByContentTool *sbc)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  return priv->grabbed_segment_index != INVALID_INDEX;
}

static void
gimp_select_by_content_tool_fit_segment (GimpSelectByContentTool *sbc,
                                   GimpVector2        *dest_points,
                                   GimpVector2         dest_start_target,
                                   GimpVector2         dest_end_target,
                                   const GimpVector2  *source_points,
                                   gint                n_points)
{
  GimpVector2 origo_translation_offset;
  GimpVector2 untranslation_offset;
  gdouble     rotation;
  gdouble     scale;

  /* Handle some quick special cases */
  if (n_points <= 0)
    {
      return;
    }
  else if (n_points == 1)
    {
      dest_points[0] = dest_end_target;
      return;
    }
  else if (n_points == 2)
    {
      dest_points[0] = dest_start_target;
      dest_points[1] = dest_end_target;
      return;
    }

  /* Copy from source to dest; we work on the dest data */
  memcpy (dest_points, source_points, sizeof (GimpVector2) * n_points);

  /* Transform the destination end point */
  {
    GimpVector2 *dest_end;
    GimpVector2  origo_translated_end_target;
    gdouble      target_rotation;
    gdouble      current_rotation;
    gdouble      target_length;
    gdouble      current_length;

    dest_end = &dest_points[n_points - 1];

    /* Transate to origin */
    gimp_vector2_sub (&origo_translation_offset,
                      &vector2_zero,
                      &dest_points[0]);
    gimp_vector2_add (dest_end,
                      dest_end,
                      &origo_translation_offset);

    /* Calculate origo_translated_end_target */
    gimp_vector2_sub (&origo_translated_end_target,
                      &dest_end_target,
                      &dest_start_target);

    /* Rotate */
    target_rotation  = atan2 (vector2_zero.y - origo_translated_end_target.y,
                              vector2_zero.x - origo_translated_end_target.x);
    current_rotation = atan2 (vector2_zero.y - dest_end->y,
                              vector2_zero.x - dest_end->x);
    rotation         = current_rotation - target_rotation;

    gimp_vector2_rotate (dest_end, rotation);


    /* Scale */
    target_length  = gimp_vector2_length (&origo_translated_end_target);
    current_length = gimp_vector2_length (dest_end);
    scale          = target_length / current_length;

    gimp_vector2_mul (dest_end, scale);


    /* Untranslate */
    gimp_vector2_sub (&untranslation_offset,
                      &dest_end_target,
                      dest_end);
    gimp_vector2_add (dest_end,
                      dest_end,
                      &untranslation_offset);
  }

  /* Do the same transformation for the rest of the points */
  {
    gint i;

    for (i = 0; i < n_points - 1; i++)
      {
        /* Translate */
        gimp_vector2_add (&dest_points[i],
                          &origo_translation_offset,
                          &dest_points[i]);

        /* Rotate */
        gimp_vector2_rotate (&dest_points[i],
                             rotation);

        /* Scale */
        gimp_vector2_mul (&dest_points[i],
                          scale);

        /* Untranslate */
        gimp_vector2_add (&dest_points[i],
                          &dest_points[i],
                          &untranslation_offset);
      }
  }
}

static void
gimp_select_by_content_tool_move_segment_vertex_to (GimpSelectByContentTool *sbc,
                                              gint                segment_index,
                                              gdouble             new_x,
                                              gdouble             new_y)
{
  GimpSelectByContentToolPrivate *priv         = GET_PRIVATE (sbc);
  GimpVector2                cursor_point = { new_x, new_y };
  GimpVector2               *dest;
  GimpVector2               *dest_start_target;
  GimpVector2               *dest_end_target;
  gint                       n_points;

  /* Handle the segment before the grabbed point */
  if (segment_index > 0)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index - 1,
                                         priv->grabbed_segment_index);

      dest_start_target = &dest[0];
      dest_end_target   = &cursor_point;

      gimp_select_by_content_tool_fit_segment (sbc,
                                         dest,
                                         *dest_start_target,
                                         *dest_end_target,
                                         priv->saved_points_lower_segment,
                                         n_points);
    }

  /* Handle the segment after the grabbed point */
  if (segment_index < priv->n_segment_indices - 1)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index,
                                         priv->grabbed_segment_index + 1);

      dest_start_target = &cursor_point;
      dest_end_target   = &dest[n_points - 1];

      gimp_select_by_content_tool_fit_segment (sbc,
                                         dest,
                                         *dest_start_target,
                                         *dest_end_target,
                                         priv->saved_points_higher_segment,
                                         n_points);
    }

  /* Handle when there only is one point */
  if (segment_index == 0 &&
      priv->n_segment_indices == 1)
    {
      priv->points[0].x = new_x;
      priv->points[0].y = new_y;
    }
}

/**
 * gimp_free_select_tool_finish_line_segment:
 * @free_ploy_sel_tool:
 * @end_x:
 * @end_y:
 *
 * Adds a line segment. Also cancels a pending free segment if any.
 **/
static void
gimp_select_by_content_tool_finish_line_segment (GimpSelectByContentTool *sbc)
{
  /* Revert any free segment points that might have been added */
  gimp_select_by_content_tool_revert_to_last_segment (sbc);
}

/**
 * gimp_free_select_tool_finish_free_segment:
 * @sbc:
 *
 * Finnishes off the creation of a free segment.
 **/
static void
gimp_select_by_content_tool_finish_free_segment (GimpSelectByContentTool *sbc)
{
  GimpSelectBycontentToolPrivate *priv = GET_PRIVATE (sbc);

  /* The points are all setup, just make a segment */
  gimp_select_by_content_tool_add_segment_index (sbc,
                                           priv->n_points - 1);
}

static void
gimp_select_by_content_tool_commit (GimpSelectBycontentTool *sbc,
                              GimpDisplay        *display)
{
  GimpSelectBycontentToolPrivate *priv = GET_PRIVATE (sbc);

  if (priv->n_points >= 3)
    {
      gimp_select_by_content_tool_select (sbc, display);
    }

  gimp_image_flush (gimp_display_get_image (display));
}

static void
gimp_select_by_content_tool_revert_to_saved_state (GimpSelectByContentTool *sbc)
{
  GimpSelectBycontentToolPrivate *priv = GET_PRIVATE (sbc);
  GimpVector2               *dest;
  gint                       n_points;

  /* Without a point grab we have no sensible information to fall back
   * on, bail out
   */
  if (! gimp_select_by_content_tool_is_point_grabbed (sbc))
    {
      return;
    }

  if (priv->grabbed_segment_index > 0)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index - 1,
                                         priv->grabbed_segment_index);

      memcpy (dest,
              priv->saved_points_lower_segment,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &dest,
                                         &n_points,
                                         priv->grabbed_segment_index,
                                         priv->grabbed_segment_index + 1);

      memcpy (dest,
              priv->saved_points_higher_segment,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      priv->points[0] = *priv->saved_points_lower_segment;
    }
}

static void
gimp_select_by_content_tool_handle_click (GimpSelectByContentTool *sbc,
                                    const GimpCoords   *coords,
                                    guint32             time,
                                    GimpDisplay        *display)
{
  GimpSelectByContentToolPrivate *priv  = GET_PRIVATE (sbc);
  GimpImage                 *image = gimp_display_get_image (display);

  /*  If there is a floating selection, anchor it  */
  if (gimp_image_get_floating_selection (image))
    {
      floating_sel_anchor (gimp_image_get_floating_selection (image));

      gimp_tool_control (GIMP_TOOL (sbc), GIMP_TOOL_ACTION_HALT, display);
    }
  else
    {
      /* First finish of the line segment if no point was grabbed */
      if (! gimp_select_by_content_tool_is_point_grabbed (sbc))
        {
          gimp_select_by_content_tool_finish_line_segment (sbc);
        }

      /* After the segments are up to date and we have handled
       * double-click, see if it's commiting time
       */
      if (gimp_select_by_content_tool_should_close (sbc,
                                              display,
                                              time,
                                              coords))
        {
          /* We can get a click notification even though the end point
           * has been moved a few pixels. Since a move will change the
           * free selection, revert it before doing the commit.
           */
          gimp_select_by_content_tool_revert_to_saved_state (sbc);

          gimp_select_by_content_tool_commit (sbc, display);
        }

      priv->last_click_time  = time;
      priv->last_click_coord = *coords;
    }
}

static void
gimp_select_by_content_tool_handle_normal_release (GimpSelectByContentTool *sbc,
                                             const GimpCoords   *coords,
                                             GimpDisplay        *display)
{
  /* First finish of the free segment if no point was grabbed */
  if (! gimp_select_by_content_tool_is_point_grabbed (sbc))
    {
      gimp_select_by_content_tool_finish_free_segment (sbc);
    }

  /* After the segments are up to date, see if it's commiting time */
  if (gimp_select_by_content_tool_should_close (sbc,
                                          display,
                                          NO_CLICK_TIME_AVAILABLE,
                                          coords))
    {
      gimp_select_by_content_tool_commit (sbc, display);
    }
}

static void
gimp_select_by_content_tool_handle_cancel (GimpSelectByContentTool *sbc)
{
  if (gimp_select_by_content_tool_is_point_grabbed (sbc))
    {
      gimp_select_by_content_tool_revert_to_saved_state (sbc);
    }
  else
    {
      gimp_select_by_content_tool_remove_last_segment (sbc);
    }
}

void
gimp_select_by_content_tool_select (GimpSelectByContentTool *sbc,
                              GimpDisplay        *display)
{
  g_return_if_fail (GIMP_IS_SELECT_BY_CONTENT_TOOL (sbc));
  g_return_if_fail (GIMP_IS_DISPLAY (display));

  GIMP_SELECT_BY_CONTENT_TOOL_GET_CLASS (sbc)->select (sbc,
                                                 display);
}

static void
gimp_select_by_content_tool_prepare_for_move (GimpSelectByContentTool *sbc)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);
  GimpVector2               *source;
  gint                       n_points;

  if (priv->grabbed_segment_index > 0)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &source,
                                         &n_points,
                                         priv->grabbed_segment_index - 1,
                                         priv->grabbed_segment_index);

      if (n_points > priv->max_n_saved_points_lower_segment)
        {
          priv->max_n_saved_points_lower_segment = n_points;

          priv->saved_points_lower_segment = g_realloc (priv->saved_points_lower_segment,
                                                        sizeof (GimpVector2) *
                                                        n_points);
        }

      memcpy (priv->saved_points_lower_segment,
              source,
              sizeof (GimpVector2) * n_points);
    }

  if (priv->grabbed_segment_index < priv->n_segment_indices - 1)
    {
      gimp_select_by_content_tool_get_segment (sbc,
                                         &source,
                                         &n_points,
                                         priv->grabbed_segment_index,
                                         priv->grabbed_segment_index + 1);

      if (n_points > priv->max_n_saved_points_higher_segment)
        {
          priv->max_n_saved_points_higher_segment = n_points;

          priv->saved_points_higher_segment = g_realloc (priv->saved_points_higher_segment,
                                                         sizeof (GimpVector2) * n_points);
        }

      memcpy (priv->saved_points_higher_segment,
              source,
              sizeof (GimpVector2) * n_points);
    }

  /* A special-case when there only is one point */
  if (priv->grabbed_segment_index == 0 &&
      priv->n_segment_indices     == 1)
    {
      if (priv->max_n_saved_points_lower_segment == 0)
        {
          priv->max_n_saved_points_lower_segment = 1;

          priv->saved_points_lower_segment = g_new0 (GimpVector2, 1);
        }

      *priv->saved_points_lower_segment = priv->points[0];
    }
}

static void
gimp_select_by_content_tool_update_motion (GimpSelectByContentTool *sbc,
                                     gdouble             new_x,
                                     gdouble             new_y)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  if (gimp_select_by_content_tool_is_point_grabbed (sbc))
    {
      priv->polygon_modified = TRUE;

      if (priv->constrain_angle &&
          priv->n_segment_indices > 1 )
        {
          gdouble start_point_x;
          gdouble start_point_y;
          gint    segment_index;

          /* Base constraints on the last segment vertex if we move
           * the first one, otherwise base on the previous segment
           * vertex
           */
          if (priv->grabbed_segment_index == 0)
            {
              segment_index = priv->n_segment_indices - 1;
            }
          else
            {
              segment_index = priv->grabbed_segment_index - 1;
            }

          gimp_select_by_content_tool_get_segment_point (sbc,
                                                   &start_point_x,
                                                   &start_point_y,
                                                   segment_index);

          gimp_constrain_line (start_point_x,
                               start_point_y,
                               &new_x,
                               &new_y,
                               GIMP_CONSTRAIN_LINE_15_DEGREES);
        }

      gimp_select_by_content_tool_move_segment_vertex_to (sbc,
                                                    priv->grabbed_segment_index,
                                                    new_x,
                                                    new_y);

      /* We also must update the pending point if we are moving the
       * first point
       */
      if (priv->grabbed_segment_index == 0)
        {
          priv->pending_point.x = new_x;
          priv->pending_point.y = new_y;
        }
    }
  else
    {
      /* Don't show the pending point while we are adding points */
      priv->show_pending_point = FALSE;

      gimp_select_by_content_tool_add_point (sbc,
                                       new_x,
                                       new_y);
    }
}

static void
gimp_select_by_content_tool_status_update (GimpSelectByContentTool *sbc,
                                     GimpDisplay        *display,
                                     const GimpCoords   *coords,
                                     gboolean            proximity)
{
  GimpTool                  *tool = GIMP_TOOL (sbc);
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  gimp_tool_pop_status (tool, display);

  if (proximity)
    {
      const gchar *status_text = NULL;

      if (gimp_select_by_content_tool_is_point_grabbed (sbc))
        {
          if (gimp_select_by_content_tool_should_close (sbc,
                                                  display,
                                                  NO_CLICK_TIME_AVAILABLE,
                                                  coords))
            {
              status_text = _("Click to complete selection");
            }
          else
            {
              status_text = _("Click-Drag to move segment vertex");
            }
        }
      else if (priv->n_segment_indices >= 3)
        {
          status_text = _("Return commits, Escape cancels, Backspace removes last segment");
        }
      else
        {
          status_text = _("Click-Drag adds a free segment, Click adds a polygonal segment");
        }

      if (status_text)
        {
          gimp_tool_push_status (tool, display, "%s", status_text);
        }
    }
}


  if (tool->display == NULL)
    {
      GIMP_TOOL_CLASS (parent_class)->oper_update (tool,
                                                   coords,
                                                   state,
                                                   proximity,
                                                   display);
    }
  else
    {
      gimp_select_by_content_tool_status_update (sbc, display, coords, proximity);
    }
}

static void
gimp_select_by_content_tool_modifier_key (GimpTool        *tool,
                                    GdkModifierType  key,
                                    gboolean         press,
                                    GdkModifierType  state,
                                    GimpDisplay     *display)
{
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpSelectByContentToolPrivate *priv      = GET_PRIVATE (tool);

  if (tool->display == display)
    {
      gimp_draw_tool_pause (draw_tool);

      priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                               TRUE : FALSE);

      priv->supress_handles = state & GDK_SHIFT_MASK ? TRUE : FALSE;

      gimp_draw_tool_resume (draw_tool);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool,
                                                key,
                                                press,
                                                state,
                                                display);
}

static void
gimp_select_by_content_tool_active_modifier_key (GimpTool        *tool,
                                           GdkModifierType  key,
                                           gboolean         press,
                                           GdkModifierType  state,
                                           GimpDisplay     *display)
{
  GimpDrawTool              *draw_tool = GIMP_DRAW_TOOL (tool);
  GimpSelectByContentToolPrivate *priv      = GET_PRIVATE (tool);

  if (tool->display != display)
    return;

  gimp_draw_tool_pause (draw_tool);

  priv->constrain_angle = ((state & gimp_get_constrain_behavior_mask ()) ?
                           TRUE : FALSE);

  /* If we didn't came here due to a mouse release, immediately update
   * the position of the thing we move.
   */
  if (state & GDK_BUTTON1_MASK)
    {
      gimp_select_by_content_tool_update_motion (GIMP_SELECT_BY_CONTENT_TOOL (tool),
                                           priv->last_coords.x,
                                           priv->last_coords.y);
    }

  gimp_draw_tool_resume (draw_tool);

  GIMP_TOOL_CLASS (parent_class)->active_modifier_key (tool,
                                                       key,
                                                       press,
                                                       state,
                                                       display);
}

static void
gimp_select_by_content_tool_real_select (GimpSelectByContentTool *sbc,
                                   GimpDisplay        *display)
{
  GimpSelectionOptions      *options = GIMP_SELECTION_TOOL_GET_OPTIONS (sbc);
  GimpSelectByContentToolPrivate *priv    = GET_PRIVATE (sbc);
  GimpImage                 *image   = gimp_display_get_image (display);

  gimp_channel_select_polygon (gimp_image_get_mask (image),
                               C_("command", "Free Select"),
                               priv->n_points,
                               priv->points,
                               priv->operation_at_start,
                               options->antialias,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius,
                               TRUE);

  gimp_tool_control (GIMP_TOOL (sbc), GIMP_TOOL_ACTION_HALT, display);
}

void
gimp_select_by_content_tool_get_points (GimpSelectByContentTool  *sbc,
                                  const GimpVector2  **points,
                                  gint                *n_points)
{
  GimpSelectByContentToolPrivate *priv = GET_PRIVATE (sbc);

  g_return_if_fail (points != NULL && n_points != NULL);

  *points   = priv->points;
  *n_points = priv->n_points;
}
