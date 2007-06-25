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

#include "core/gimpchannel.h"
#include "core/gimpchannel-select.h"
#include "core/gimpimage.h"
#include "core/gimppickable.h"
#include "core/gimpscanconvert.h"
#include "core/gimptoolinfo.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"

#include "gimpiscissorstool.h"
#include "gimpselectionoptions.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/*  defines  */
#define  MAX_GRADIENT      179.606  /* == sqrt (127^2 + 127^2) */
#define  GRADIENT_SEARCH   32  /* how far to look when snapping to an edge */
#define  TARGET_SIZE       25
#define  POINT_WIDTH       12  /* size (in pixels) of seed handles */
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


struct _ICurve
{
  gint       x1, y1;
  gint       x2, y2;
  GPtrArray *points;
};


/*  local function prototypes  */

static void   gimp_iscissors_tool_finalize       (GObject               *object);

static void   gimp_iscissors_tool_control        (GimpTool              *tool,
                                                  GimpToolAction         action,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_button_press   (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_button_release (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpButtonReleaseType  release_type,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_motion         (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  guint32                time,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_oper_update    (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  GdkModifierType        state,
                                                  gboolean               proximity,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_cursor_update  (GimpTool              *tool,
                                                  GimpCoords            *coords,
                                                  GdkModifierType        state,
                                                  GimpDisplay           *display);
static gboolean gimp_iscissors_tool_key_press    (GimpTool              *tool,
                                                  GdkEventKey           *kevent,
                                                  GimpDisplay           *display);

static void   gimp_iscissors_tool_apply          (GimpIscissorsTool     *iscissors,
                                                  GimpDisplay           *display);
static void   gimp_iscissors_tool_reset          (GimpIscissorsTool     *iscissors);
static void   gimp_iscissors_tool_draw           (GimpDrawTool          *draw_tool);


static void          iscissors_convert         (GimpIscissorsTool *iscissors,
                                                GimpDisplay       *display);
static TileManager * gradient_map_new          (GimpImage         *image);

static void          find_optimal_path         (TileManager       *gradient_map,
                                                TempBuf           *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2,
                                                gint               xs,
                                                gint               ys);
static void          find_max_gradient         (GimpIscissorsTool *iscissors,
                                                GimpImage         *image,
                                                gint              *x,
                                                gint              *y);
static void          calculate_curve           (GimpTool          *tool,
                                                ICurve            *curve);
static void          iscissors_draw_curve      (GimpDrawTool      *draw_tool,
                                                ICurve            *curve);
static void          iscissors_free_icurves    (GQueue            *curves);

static gint          mouse_over_vertex         (GimpIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_vertex         (GimpIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static GList       * mouse_over_curve          (GimpIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_curve          (GimpIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);

static GPtrArray   * plot_pixels               (GimpIscissorsTool *iscissors,
                                                TempBuf           *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               xs,
                                                gint               ys,
                                                gint               xe,
                                                gint               ye);


G_DEFINE_TYPE (GimpIscissorsTool, gimp_iscissors_tool,
               GIMP_TYPE_SELECTION_TOOL)

#define parent_class gimp_iscissors_tool_parent_class


/*  static variables  */

/*  where to move on a given link direction  */
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
gimp_iscissors_tool_register (GimpToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (GIMP_TYPE_ISCISSORS_TOOL,
                GIMP_TYPE_SELECTION_OPTIONS,
                gimp_selection_options_gui,
                0,
                "gimp-iscissors-tool",
                _("Scissors"),
                _("Scissors Select Tool: Select shapes using intelligent edge-fitting"),
                N_("Intelligent _Scissors"),
                "I",
                NULL, GIMP_HELP_TOOL_ISCISSORS,
                GIMP_STOCK_TOOL_ISCISSORS,
                data);
}

static void
gimp_iscissors_tool_class_init (GimpIscissorsToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GimpToolClass     *tool_class      = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass *draw_tool_class = GIMP_DRAW_TOOL_CLASS (klass);
  gint               i;

  object_class->finalize     = gimp_iscissors_tool_finalize;

  tool_class->control        = gimp_iscissors_tool_control;
  tool_class->button_press   = gimp_iscissors_tool_button_press;
  tool_class->button_release = gimp_iscissors_tool_button_release;
  tool_class->motion         = gimp_iscissors_tool_motion;
  tool_class->oper_update    = gimp_iscissors_tool_oper_update;
  tool_class->cursor_update  = gimp_iscissors_tool_cursor_update;
  tool_class->key_press      = gimp_iscissors_tool_key_press;

  draw_tool_class->draw      = gimp_iscissors_tool_draw;

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

  /*  set the 256th index of the direction_values to the hightest cost  */
  direction_value[255][0] = 255;
  direction_value[255][1] = 255;
  direction_value[255][2] = 255;
  direction_value[255][3] = 255;
}

static void
gimp_iscissors_tool_init (GimpIscissorsTool *iscissors)
{
  GimpTool *tool = GIMP_TOOL (iscissors);

  iscissors->op           = ISCISSORS_OP_NONE;
  iscissors->dp_buf       = NULL;
  iscissors->curves       = g_queue_new ();
  iscissors->draw         = DRAW_NOTHING;
  iscissors->state        = NO_ACTION;
  iscissors->mask         = NULL;
  iscissors->gradient_map = NULL;
  iscissors->livewire     = NULL;

  gimp_tool_control_set_scroll_lock (tool->control, TRUE);
  gimp_tool_control_set_snap_to     (tool->control, FALSE);
  gimp_tool_control_set_preserve    (tool->control, FALSE);
  gimp_tool_control_set_dirty_mask  (tool->control, GIMP_DIRTY_IMAGE_SIZE);
  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_ISCISSORS);

  gimp_iscissors_tool_reset (iscissors);
}

static void
gimp_iscissors_tool_finalize (GObject *object)
{
  GimpIscissorsTool *iscissors = GIMP_ISCISSORS_TOOL (object);

  gimp_iscissors_tool_reset (iscissors);

  g_queue_free (iscissors->curves);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_iscissors_tool_control (GimpTool       *tool,
                             GimpToolAction  action,
                             GimpDisplay    *display)
{
  GimpIscissorsTool *iscissors = GIMP_ISCISSORS_TOOL (tool);
  IscissorsDraw      draw;

  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      draw = DRAW_CURVE | DRAW_CURRENT_SEED;
      break;

    case SEED_ADJUSTMENT:
      draw = DRAW_CURVE | DRAW_ACTIVE_CURVE;
      break;

    default:
      draw = DRAW_CURVE;
      break;
    }

  iscissors->draw = draw;

  GIMP_TOOL_CLASS (parent_class)->control (tool, action, display);

  switch (action)
    {
    case GIMP_TOOL_ACTION_PAUSE:
    case GIMP_TOOL_ACTION_RESUME:
      break;

    case GIMP_TOOL_ACTION_HALT:
      gimp_iscissors_tool_reset (iscissors);
      break;
    }
}

static void
gimp_iscissors_tool_button_press (GimpTool        *tool,
                                  GimpCoords      *coords,
                                  guint32          time,
                                  GdkModifierType  state,
                                  GimpDisplay     *display)
{
  GimpIscissorsTool    *iscissors = GIMP_ISCISSORS_TOOL (tool);
  GimpSelectionOptions *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  iscissors->x = RINT (coords->x);
  iscissors->y = RINT (coords->y);

  /*  If the tool was being used in another image...reset it  */

  if (gimp_tool_control_is_active (tool->control))
    {
      if (display != tool->display)
        {
          gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));
          gimp_iscissors_tool_reset (iscissors);
          gimp_tool_control_activate (tool->control);
        }
    }
  else
    {
      gimp_tool_control_activate (tool->control);
    }

  tool->display = display;

  switch (iscissors->state)
    {
    case NO_ACTION:
      iscissors->state = SEED_PLACEMENT;
      iscissors->draw  = DRAW_CURRENT_SEED;

      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (iscissors,
                           display->image,
                           &iscissors->x,
                           &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, display->image->width - 1);
      iscissors->y = CLAMP (iscissors->y, 0, display->image->height - 1);

      iscissors->ix = iscissors->x;
      iscissors->iy = iscissors->y;

      /*  Initialize the selection core only on starting the tool  */
      gimp_draw_tool_start (GIMP_DRAW_TOOL (tool), display);
      break;

    default:
      /*  Check if the mouse click occurred on a vertex or the curve itself  */
      if (clicked_on_vertex (iscissors, coords->x, coords->y))
        {
          iscissors->nx    = iscissors->x;
          iscissors->ny    = iscissors->y;
          iscissors->state = SEED_ADJUSTMENT;

          iscissors->draw = DRAW_CURVE | DRAW_ACTIVE_CURVE;
          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
      /*  If the iscissors is connected, check if the click was inside  */
      else if (iscissors->connected && iscissors->mask &&
               gimp_pickable_get_opacity_at (GIMP_PICKABLE (iscissors->mask),
                                             iscissors->x,
                                             iscissors->y))
        {
          gimp_iscissors_tool_apply (iscissors, display);
        }
      else if (! iscissors->connected)
        {
          /*  if we're not connected, we're adding a new point  */

          /*  pause the tool, but undraw nothing  */
          iscissors->draw = DRAW_NOTHING;
          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          iscissors->state = SEED_PLACEMENT;
          iscissors->draw  = DRAW_CURRENT_SEED;

          if (options->interactive)
            iscissors->draw |= DRAW_LIVEWIRE;

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
        }
      break;
    }
}


static void
iscissors_convert (GimpIscissorsTool *iscissors,
                   GimpDisplay       *display)
{
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (iscissors);
  GimpScanConvert      *sc;
  GList                *list;

  sc = gimp_scan_convert_new ();

  /* go over the curves in reverse order, adding the points we have */
  for (list = g_queue_peek_tail_link (iscissors->curves);
       list;
       list = g_list_previous (list))
    {
      ICurve      *icurve = list->data;
      GimpVector2 *points;
      guint        n_points;
      gint         i;

      n_points = icurve->points->len;
      points   = g_new (GimpVector2, n_points);

      for (i = 0; i < n_points; i ++)
        {
          guint32  packed = GPOINTER_TO_INT (g_ptr_array_index (icurve->points,
                                                                i));

          points[i].x = packed & 0x0000ffff;
          points[i].y = packed >> 16;
        }

      gimp_scan_convert_add_points (sc, n_points, points, FALSE);
      g_free (points);
    }

  if (iscissors->mask)
    g_object_unref (iscissors->mask);

  iscissors->mask = gimp_channel_new_mask (display->image,
                                           display->image->width,
                                           display->image->height);
  gimp_scan_convert_render (sc, GIMP_DRAWABLE (iscissors->mask)->tiles,
                            0, 0, options->antialias);
  gimp_scan_convert_free (sc);
}

static void
gimp_iscissors_tool_button_release (GimpTool              *tool,
                                    GimpCoords            *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    GimpButtonReleaseType  release_type,
                                    GimpDisplay           *display)
{
  GimpIscissorsTool    *iscissors = GIMP_ISCISSORS_TOOL (tool);
  GimpSelectionOptions *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  /* Make sure X didn't skip the button release event -- as it's known
   * to do
   */
  if (iscissors->state == WAITING)
    return;

  /*  Undraw everything  */
  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      iscissors->draw = DRAW_CURVE | DRAW_CURRENT_SEED;
      if (options->interactive)
        iscissors->draw |= DRAW_LIVEWIRE;
      break;
    case SEED_ADJUSTMENT:
      iscissors->draw = DRAW_CURVE | DRAW_ACTIVE_CURVE;
      break;
    default:
      break;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  if (release_type != GIMP_BUTTON_RELEASE_CANCEL)
    {
      /*  Progress to the next stage of intelligent selection  */
      switch (iscissors->state)
        {
        case SEED_PLACEMENT:
          /*  Add a new icurve  */
          if (!iscissors->first_point)
            {
              /*  Determine if we're connecting to the first point  */
              if (! g_queue_is_empty (iscissors->curves))
                {
                  ICurve *curve = g_queue_peek_head (iscissors->curves);

                  if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                                iscissors->x, iscissors->y,
                                                GIMP_HANDLE_CIRCLE,
                                                curve->x1, curve->y1,
                                                POINT_WIDTH, POINT_WIDTH,
                                                GTK_ANCHOR_CENTER,
                                                FALSE))
                    {
                      iscissors->x = curve->x1;
                      iscissors->y = curve->y1;
                      iscissors->connected = TRUE;
                    }
                }

              /*  Create the new curve segment  */
              if (iscissors->ix != iscissors->x ||
                  iscissors->iy != iscissors->y)
                {
                  ICurve *curve = g_slice_new (ICurve);

                  curve->x1 = iscissors->ix;
                  curve->y1 = iscissors->iy;
                  iscissors->ix = curve->x2 = iscissors->x;
                  iscissors->iy = curve->y2 = iscissors->y;
                  curve->points = NULL;

                  g_queue_push_tail (iscissors->curves, curve);

                  calculate_curve (tool, curve);
                }
            }
          else /* this was our first point */
            {
              iscissors->first_point = FALSE;
            }
          break;

        case SEED_ADJUSTMENT:
          /*  recalculate both curves  */
          if (iscissors->curve1)
            {
              iscissors->curve1->x1 = iscissors->nx;
              iscissors->curve1->y1 = iscissors->ny;
              calculate_curve (tool, iscissors->curve1);
            }
          if (iscissors->curve2)
            {
              iscissors->curve2->x2 = iscissors->nx;
              iscissors->curve2->y2 = iscissors->ny;
              calculate_curve (tool, iscissors->curve2);
            }
          break;

        default:
          break;
        }
    }

  iscissors->state = WAITING;

  /*  Draw only the boundary  */
  iscissors->draw = DRAW_CURVE;
  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

  /*  convert the curves into a region  */
  if (iscissors->connected)
    iscissors_convert (iscissors, display);
}

static void
gimp_iscissors_tool_motion (GimpTool        *tool,
                            GimpCoords      *coords,
                            guint32          time,
                            GdkModifierType  state,
                            GimpDisplay     *display)
{
  GimpIscissorsTool    *iscissors = GIMP_ISCISSORS_TOOL (tool);
  GimpSelectionOptions *options   = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  if (iscissors->state == NO_ACTION)
    return;

  if (iscissors->state == SEED_PLACEMENT)
    {
      iscissors->draw = DRAW_CURRENT_SEED;

      if (options->interactive)
        iscissors->draw |= DRAW_LIVEWIRE;
    }
  else if (iscissors->state == SEED_ADJUSTMENT)
    {
      iscissors->draw = DRAW_ACTIVE_CURVE;
    }

  gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

  iscissors->x = RINT (coords->x);
  iscissors->y = RINT (coords->y);

  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      /*  Hold the shift key down to disable the auto-edge snap feature  */
      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (iscissors, display->image,
                           &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, display->image->width  - 1);
      iscissors->y = CLAMP (iscissors->y, 0, display->image->height - 1);

      if (iscissors->first_point)
        {
          iscissors->ix = iscissors->x;
          iscissors->iy = iscissors->y;
        }
      break;

    case SEED_ADJUSTMENT:
      /*  Move the current seed to the location of the cursor  */
      if (! (state & GDK_SHIFT_MASK))
        find_max_gradient (iscissors, display->image,
                           &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, display->image->width  - 1);
      iscissors->y = CLAMP (iscissors->y, 0, display->image->height - 1);

      iscissors->nx = iscissors->x;
      iscissors->ny = iscissors->y;
      break;

    default:
      break;
    }

  gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));
}

static void
gimp_iscissors_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool          *tool      = GIMP_TOOL (draw_tool);
  GimpIscissorsTool *iscissors = GIMP_ISCISSORS_TOOL (draw_tool);

  /*  Draw the crosshairs target if we're placing a seed  */
  if (iscissors->draw & DRAW_CURRENT_SEED)
    {
      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_CROSS,
                                  iscissors->x, iscissors->y,
                                  TARGET_SIZE, TARGET_SIZE,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);

      /* Draw a line boundary */
      if (! iscissors->first_point && ! (iscissors->draw & DRAW_LIVEWIRE))
        {
          gimp_draw_tool_draw_line (draw_tool,
                                    iscissors->ix, iscissors->iy,
                                    iscissors->x, iscissors->y,
                                    FALSE);
        }
    }

  /* Draw the livewire boundary */
  if ((iscissors->draw & DRAW_LIVEWIRE) && ! iscissors->first_point)
    {
      /* See if the mouse has moved.  If so, create a new segment... */
      if (! iscissors->livewire ||
          (iscissors->livewire &&
           (iscissors->ix != iscissors->livewire->x1 ||
            iscissors->x  != iscissors->livewire->x2  ||
            iscissors->iy != iscissors->livewire->y1 ||
            iscissors->y  != iscissors->livewire->y2)))
        {
          ICurve *curve = g_slice_new (ICurve);

          curve->x1 = iscissors->ix;
          curve->y1 = iscissors->iy;
          curve->x2 = iscissors->x;
          curve->y2 = iscissors->y;
          curve->points = NULL;

          if (iscissors->livewire)
            {
              if (iscissors->livewire->points)
                g_ptr_array_free (iscissors->livewire->points, TRUE);

              g_slice_free (ICurve, iscissors->livewire);

              iscissors->livewire = NULL;
            }

          iscissors->livewire = curve;
          calculate_curve (tool, curve);
        }

      /*  plot the curve  */
      iscissors_draw_curve (draw_tool, iscissors->livewire);
    }

  if ((iscissors->draw & DRAW_CURVE) && ! iscissors->first_point)
    {
      GList *list;

      /*  Draw a point at the init point coordinates  */
      if (! iscissors->connected)
        {
          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_FILLED_CIRCLE,
                                      iscissors->ix,
                                      iscissors->iy,
                                      POINT_WIDTH,
                                      POINT_WIDTH,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);
        }

      /*  Go through the list of icurves, and render each one...  */
      for (list = g_queue_peek_head_link (iscissors->curves);
           list;
           list = g_list_next (list))
        {
          ICurve *curve = list->data;

          if (iscissors->draw & DRAW_ACTIVE_CURVE)
            {
              /*  don't draw curve1 at all  */
              if (curve == iscissors->curve1)
                continue;
            }

          gimp_draw_tool_draw_handle (draw_tool,
                                      GIMP_HANDLE_FILLED_CIRCLE,
                                      curve->x1,
                                      curve->y1,
                                      POINT_WIDTH,
                                      POINT_WIDTH,
                                      GTK_ANCHOR_CENTER,
                                      FALSE);

          if (iscissors->draw & DRAW_ACTIVE_CURVE)
            {
              /*  draw only the start handle of curve2  */
              if (curve == iscissors->curve2)
                continue;
            }

          /*  plot the curve  */
          iscissors_draw_curve (draw_tool, curve);
        }
    }

  if (iscissors->draw & DRAW_ACTIVE_CURVE)
    {
      /*  plot both curves, and the control point between them  */
      if (iscissors->curve1)
        {
          gimp_draw_tool_draw_line (draw_tool,
                                    iscissors->curve1->x2,
                                    iscissors->curve1->y2,
                                    iscissors->nx,
                                    iscissors->ny,
                                    FALSE);
        }
      if (iscissors->curve2)
        {
          gimp_draw_tool_draw_line (draw_tool,
                                    iscissors->curve2->x1,
                                    iscissors->curve2->y1,
                                    iscissors->nx,
                                    iscissors->ny,
                                    FALSE);
        }

      gimp_draw_tool_draw_handle (draw_tool,
                                  GIMP_HANDLE_FILLED_CIRCLE,
                                  iscissors->nx,
                                  iscissors->ny,
                                  POINT_WIDTH,
                                  POINT_WIDTH,
                                  GTK_ANCHOR_CENTER,
                                  FALSE);
    }
}


static void
iscissors_draw_curve (GimpDrawTool *draw_tool,
                      ICurve       *curve)
{
  gdouble  *points;
  gpointer *point;
  gint      i, len;

  if (! curve->points)
    return;

  len = curve->points->len;

  points = g_new (gdouble, 2 * len);

  for (i = 0, point = curve->points->pdata; i < len; i++, point++)
    {
      guint32 coords = GPOINTER_TO_INT (*point);

      points[i * 2]     = (coords & 0x0000ffff);
      points[i * 2 + 1] = (coords >> 16);
    }

  gimp_draw_tool_draw_lines (draw_tool, points, len, FALSE, FALSE);

  g_free (points);
}

static void
gimp_iscissors_tool_oper_update (GimpTool        *tool,
                                 GimpCoords      *coords,
                                 GdkModifierType  state,
                                 gboolean         proximity,
                                 GimpDisplay     *display)
{
  GimpIscissorsTool *iscissors = GIMP_ISCISSORS_TOOL (tool);

  GIMP_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
  /* parent sets a message in the status bar, but it will be replaced here */

  if (mouse_over_vertex (iscissors, coords->x, coords->y) > 1)
    {
      gchar *status;

      status = gimp_suggest_modifiers (_("Click-Drag to move this point"),
                                       GDK_SHIFT_MASK & ~state,
                                       _("%s: disable auto-snap"), NULL, NULL);
      gimp_tool_replace_status (tool, display, status);
      g_free (status);
      iscissors->op = ISCISSORS_OP_MOVE_POINT;
    }
  else if (mouse_over_curve (iscissors, coords->x, coords->y))
    {
      ICurve *curve = g_queue_peek_head (iscissors->curves);

      if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (tool), display,
                                    RINT (coords->x), RINT (coords->y),
                                    GIMP_HANDLE_CIRCLE,
                                    curve->x1, curve->y1,
                                    POINT_WIDTH, POINT_WIDTH,
                                    GTK_ANCHOR_CENTER,
                                    FALSE))
        {
          gimp_tool_replace_status (tool, display, _("Click to close the"
                                                     " curve"));
          iscissors->op = ISCISSORS_OP_CONNECT;
        }
      else
        {
          gimp_tool_replace_status (tool, display, _("Click to add a point"
                                                     " on this segment"));
          iscissors->op = ISCISSORS_OP_ADD_POINT;
        }
    }
  else if (iscissors->connected && iscissors->mask)
    {
      if (gimp_pickable_get_opacity_at (GIMP_PICKABLE (iscissors->mask),
                                        RINT (coords->x),
                                        RINT (coords->y)))
        {
          if (proximity)
            {
              gimp_tool_replace_status (tool, display,
                                        _("Click or press Enter to convert to"
                                          " a selection"));
            }
          iscissors->op = ISCISSORS_OP_SELECT;
        }
      else
        {
          if (proximity)
            {
              gimp_tool_replace_status (tool, display,
                                        _("Press Enter to convert to a"
                                          " selection"));
            }
          iscissors->op = ISCISSORS_OP_IMPOSSIBLE;
        }
    }
  else
    {
      switch (iscissors->state)
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
              gimp_tool_replace_status (tool, display, status);
              g_free (status);
            }
          iscissors->op = ISCISSORS_OP_ADD_POINT;
          break;

        default:
          /* if NO_ACTION, keep parent's status bar message (selection tool) */
          iscissors->op = ISCISSORS_OP_NONE;
          break;
        }
    }
}

static void
gimp_iscissors_tool_cursor_update (GimpTool        *tool,
                                   GimpCoords      *coords,
                                   GdkModifierType  state,
                                   GimpDisplay     *display)
{
  GimpIscissorsTool  *iscissors = GIMP_ISCISSORS_TOOL (tool);
  GimpCursorModifier  modifier  = GIMP_CURSOR_MODIFIER_NONE;

  switch (iscissors->op)
    {
    case ISCISSORS_OP_SELECT:
      GIMP_TOOL_CLASS (parent_class)->cursor_update (tool, coords,
                                                     state, display);
      return;

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

  gimp_tool_set_cursor (tool, display,
                        GIMP_CURSOR_MOUSE,
                        GIMP_TOOL_CURSOR_ISCISSORS,
                        modifier);
}

static gboolean
gimp_iscissors_tool_key_press (GimpTool    *tool,
                               GdkEventKey *kevent,
                               GimpDisplay *display)
{
  GimpIscissorsTool *iscissors = GIMP_ISCISSORS_TOOL (tool);

  if (display != tool->display)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KP_Enter:
    case GDK_Return:
      if (iscissors->connected && iscissors->mask)
        {
          gimp_iscissors_tool_apply (iscissors, display);
          return TRUE;
        }
      return FALSE;

    case GDK_Escape:
      gimp_tool_control (tool, GIMP_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      return FALSE;
    }
}

static void
gimp_iscissors_tool_apply (GimpIscissorsTool *iscissors,
                           GimpDisplay       *display)
{
  GimpTool             *tool    = GIMP_TOOL (iscissors);
  GimpSelectionOptions *options = GIMP_SELECTION_TOOL_GET_OPTIONS (tool);

  /*  Undraw the curve  */
  gimp_tool_control_halt (tool->control);

  iscissors->draw = DRAW_CURVE;
  gimp_draw_tool_stop (GIMP_DRAW_TOOL (tool));

  gimp_channel_select_channel (gimp_image_get_mask (display->image),
                               tool->tool_info->blurb,
                               iscissors->mask,
                               0, 0,
                               options->operation,
                               options->feather,
                               options->feather_radius,
                               options->feather_radius);

  gimp_iscissors_tool_reset (iscissors);

  gimp_image_flush (display->image);
}

static void
gimp_iscissors_tool_reset (GimpIscissorsTool *iscissors)
{
  /*  Free and reset the curve list  */
  iscissors_free_icurves (iscissors->curves);

  /*  free mask  */
  if (iscissors->mask)
    {
      g_object_unref (iscissors->mask);
      iscissors->mask = NULL;
    }

  /* free the gradient map */
  if (iscissors->gradient_map)
    {
      /* release any tile we were using */
      if (cur_tile)
        {
          tile_release (cur_tile, FALSE);
          cur_tile = NULL;
        }

      tile_manager_unref (iscissors->gradient_map);
      iscissors->gradient_map = NULL;
    }

  iscissors->curve1      = NULL;
  iscissors->curve2      = NULL;
  iscissors->first_point = TRUE;
  iscissors->connected   = FALSE;
  iscissors->draw        = DRAW_NOTHING;
  iscissors->state       = NO_ACTION;

  /*  Reset the dp buffers  */
  if (iscissors->dp_buf)
    {
      temp_buf_free (iscissors->dp_buf);
      iscissors->dp_buf = NULL;
    }
}


static void
iscissors_free_icurves (GQueue *curves)
{
  while (! g_queue_is_empty (curves))
    {
      ICurve *curve = g_queue_pop_head (curves);

      if (curve->points)
        g_ptr_array_free (curve->points, TRUE);

      g_slice_free (ICurve, curve);
    }
}


/* XXX need some scan-conversion routines from somewhere.  maybe. ? */

static gint
mouse_over_vertex (GimpIscissorsTool *iscissors,
                   gdouble            x,
                   gdouble            y)
{
  GList *list;
  gint   curves_found = 0;

  /*  traverse through the list, returning non-zero if the current cursor
   *  position is on an existing curve vertex.  Set the curve1 and curve2
   *  variables to the two curves containing the vertex in question
   */

  iscissors->curve1 = iscissors->curve2 = NULL;

  for (list = g_queue_peek_head_link (iscissors->curves);
       list;
       list = g_list_next (list))
    {
      ICurve *curve = list->data;

      if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (iscissors),
                                    GIMP_TOOL (iscissors)->display,
                                    x, y,
                                    GIMP_HANDLE_CIRCLE,
                                    curve->x1, curve->y1,
                                    POINT_WIDTH, POINT_WIDTH,
                                    GTK_ANCHOR_CENTER,
                                    FALSE))
        {
          iscissors->curve1 = curve;

          if (curves_found++)
            return curves_found;
        }
      else if (gimp_draw_tool_on_handle (GIMP_DRAW_TOOL (iscissors),
                                         GIMP_TOOL (iscissors)->display,
                                         x, y,
                                         GIMP_HANDLE_CIRCLE,
                                         curve->x2, curve->y2,
                                         POINT_WIDTH, POINT_WIDTH,
                                         GTK_ANCHOR_CENTER,
                                         FALSE))
        {
          iscissors->curve2 = curve;

          if (curves_found++)
            return curves_found;
        }
    }

  return curves_found;
}

static gboolean
clicked_on_vertex (GimpIscissorsTool *iscissors,
                   gdouble            x,
                   gdouble            y)
{
  gint curves_found = 0;

  curves_found = mouse_over_vertex (iscissors, x, y);

  if (curves_found > 1)
    {
      /*  undraw the curve  */
      iscissors->draw = DRAW_CURVE;
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (iscissors));

      return TRUE;
    }

  /*  if only one curve was found, the curves are unconnected, and
   *  the user only wants to move either the first or last point
   *  disallow this for now.
   */
  if (curves_found == 1)
    return FALSE;

  return clicked_on_curve (iscissors, x, y);
}


static GList *
mouse_over_curve (GimpIscissorsTool *iscissors,
                  gdouble            x,
                  gdouble            y)
{
  GList *list;

  /*  traverse through the list, returning the curve segment's list element
   *  if the current cursor position is on a curve...
   */
  for (list = g_queue_peek_head_link (iscissors->curves);
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
          if (gimp_draw_tool_in_radius (GIMP_DRAW_TOOL (iscissors),
                                        GIMP_TOOL (iscissors)->display,
                                        tx, ty,
                                        x, y,
                                        POINT_WIDTH / 2))
            {
              return list;
            }
        }
    }

  return NULL;
}

static gboolean
clicked_on_curve (GimpIscissorsTool *iscissors,
                  gdouble            x,
                  gdouble            y)
{
  GList *list = mouse_over_curve (iscissors, x, y);

  /*  traverse through the list, getting back the curve segment's list
   *  element if the current cursor position is on a curve...
   *  If this occurs, replace the curve with two new curves,
   *  separated by a new vertex.
   */

  if (list)
    {
      ICurve *curve = list->data;
      ICurve *new_curve;

      /*  undraw the curve  */
      iscissors->draw = DRAW_CURVE;
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (iscissors));

      /*  Create the new curve  */
      new_curve = g_slice_new (ICurve);

      new_curve->x2 = curve->x2;
      new_curve->y2 = curve->y2;
      new_curve->x1 = curve->x2 = iscissors->x;
      new_curve->y1 = curve->y2 = iscissors->y;
      new_curve->points = NULL;

      /*  Create the new link and supply the new curve as data  */
      g_queue_insert_after (iscissors->curves, list, new_curve);

      iscissors->curve1 = new_curve;
      iscissors->curve2 = curve;

      return TRUE;
    }

  return FALSE;
}


static void
calculate_curve (GimpTool *tool,
                 ICurve   *curve)
{
  GimpIscissorsTool *iscissors;
  GimpDisplay       *display;
  gint               x, y, dir;
  gint               xs, ys, xe, ye;
  gint               x1, y1, x2, y2;
  gint               width, height;
  gint               ewidth, eheight;

  /*  Calculate the lowest cost path from one vertex to the next as specified
   *  by the parameter "curve".
   *    Here are the steps:
   *      1)  Calculate the appropriate working area for this operation
   *      2)  Allocate a temp buf for the dynamic programming array
   *      3)  Run the dynamic programming algorithm to find the optimal path
   *      4)  Translate the optimal path into pixels in the icurve data
   *            structure.
   */

  iscissors = GIMP_ISCISSORS_TOOL (tool);

  display = tool->display;

  /*  Get the bounding box  */
  xs = CLAMP (curve->x1, 0, display->image->width - 1);
  ys = CLAMP (curve->y1, 0, display->image->height - 1);
  xe = CLAMP (curve->x2, 0, display->image->width - 1);
  ye = CLAMP (curve->y2, 0, display->image->height - 1);
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
    x2 += CLAMP (ewidth, 0, display->image->width - x2);
  else
    x1 -= CLAMP (ewidth, 0, x1);
  if (ye >= ys)
    y2 += CLAMP (eheight, 0, display->image->height - y2);
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
      if (!iscissors->gradient_map)
          iscissors->gradient_map = gradient_map_new (display->image);

      /*  allocate the dynamic programming array  */
      iscissors->dp_buf =
        temp_buf_resize (iscissors->dp_buf, 4, x1, y1, width, height);

      /*  find the optimal path of pixels from (x1, y1) to (x2, y2)  */
      find_optimal_path (iscissors->gradient_map, iscissors->dp_buf,
                         x1, y1, x2, y2, xs, ys);

      /*  get a list of the pixels in the optimal path  */
      curve->points = plot_pixels (iscissors, iscissors->dp_buf,
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
  static gint  cur_tilex;
  static gint  cur_tiley;
  guint8      *p;

  if (!cur_tile ||
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

  p = tile_data_pointer (cur_tile, x % TILE_WIDTH, y % TILE_HEIGHT);
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
plot_pixels (GimpIscissorsTool *iscissors,
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
  data = (guint *) temp_buf_data (dp_buf) + (ye - y1) * width + (xe - x1);

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

  pickable = GIMP_PICKABLE (image->projection);

  gimp_pickable_flush (pickable);

  /* get corresponding tile in the image */
  srctile = tile_manager_get_tile (gimp_pickable_get_tiles (pickable),
                                   x, y, TRUE, FALSE);
  if (! srctile)
    return;

  sw = tile_ewidth (srctile);
  sh = tile_eheight (srctile);

  pixel_region_init_data (&srcPR, tile_data_pointer (srctile, 0, 0),
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

  tm = tile_manager_new (image->width, image->height,
                         sizeof (guint8) * COST_WIDTH);

  tile_manager_set_validate_proc (tm,
                                  (TileValidateProc) gradmap_tile_validate,
                                  image);

  return tm;
}

static void
find_max_gradient (GimpIscissorsTool *iscissors,
                   GimpImage         *image,
                   gint              *x,
                   gint              *y)
{
  PixelRegion  srcPR;
  gint         radius;
  gint         i, j;
  gint         endx, endy;
  gint         sx, sy, cx, cy;
  gint         x1, y1, x2, y2;
  gpointer     pr;
  gfloat       max_gradient;

  /* Initialise the gradient map tile manager for this image if we
   * don't already have one. */
  if (! iscissors->gradient_map)
    iscissors->gradient_map = gradient_map_new (image);

  radius = GRADIENT_SEARCH >> 1;

  /*  calculate the extent of the search  */
  cx = CLAMP (*x, 0, image->width);
  cy = CLAMP (*y, 0, image->height);
  sx = cx - radius;
  sy = cy - radius;
  x1 = CLAMP (cx - radius, 0, image->width);
  y1 = CLAMP (cy - radius, 0, image->height);
  x2 = CLAMP (cx + radius, 0, image->width);
  y2 = CLAMP (cy + radius, 0, image->height);
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
