/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 * originally done by Spencer and Peter, and worked fine in the 0.54
 * (motif only) release of LIGMA.  Later revisions (0.99.something
 * until about 1.1.4) completely changed the algorithm used, until it
 * bore little resemblance to the one described in the paper above.
 * The 0.54 version of the algorithm was then forwards ported to 1.1.4
 * by Austin Donnelly.
 */

/* Livewire boundary implementation done by Laramie Leavitt */

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmaimage.h"
#include "core/ligmapickable.h"
#include "core/ligmascanconvert.h"
#include "core/ligmatempbuf.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmacanvasitem.h"
#include "display/ligmadisplay.h"

#include "ligmaiscissorsoptions.h"
#include "ligmaiscissorstool.h"
#include "ligmatilehandleriscissors.h"
#include "ligmatoolcontrol.h"

#include "ligma-intl.h"


/*  defines  */
#define  GRADIENT_SEARCH   32  /* how far to look when snapping to an edge */
#define  EXTEND_BY         0.2 /* proportion to expand cost map by */
#define  FIXED             5   /* additional fixed size to expand cost map */

#define  COST_WIDTH        2   /* number of bytes for each pixel in cost map  */

/* weight to give between gradient (_G) and direction (_D) */
#define  OMEGA_D           0.2
#define  OMEGA_G           0.8

/* sentinel to mark seed point in ?cost? map */
#define  SEED_POINT        9

/*  Functional defines  */
#define  PIXEL_COST(x)     ((x) >> 8)
#define  PIXEL_DIR(x)      ((x) & 0x000000ff)


struct _ISegment
{
  gint       x1, y1;
  gint       x2, y2;
  GPtrArray *points;
};

struct _ICurve
{
  GQueue   *segments;
  gboolean  first_point;
  gboolean  closed;
};


/*  local function prototypes  */

static void          ligma_iscissors_tool_finalize       (GObject               *object);

static void          ligma_iscissors_tool_control        (LigmaTool              *tool,
                                                         LigmaToolAction         action,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_button_press   (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonPressType    press_type,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_button_release (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaButtonReleaseType  release_type,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_motion         (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_oper_update    (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         GdkModifierType        state,
                                                         gboolean               proximity,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_cursor_update  (LigmaTool              *tool,
                                                         const LigmaCoords      *coords,
                                                         GdkModifierType        state,
                                                         LigmaDisplay           *display);
static gboolean      ligma_iscissors_tool_key_press      (LigmaTool              *tool,
                                                         GdkEventKey           *kevent,
                                                         LigmaDisplay           *display);
static const gchar * ligma_iscissors_tool_can_undo       (LigmaTool              *tool,
                                                         LigmaDisplay           *display);
static const gchar * ligma_iscissors_tool_can_redo       (LigmaTool              *tool,
                                                         LigmaDisplay           *display);
static gboolean      ligma_iscissors_tool_undo           (LigmaTool              *tool,
                                                         LigmaDisplay           *display);
static gboolean      ligma_iscissors_tool_redo           (LigmaTool              *tool,
                                                         LigmaDisplay           *display);

static void          ligma_iscissors_tool_draw           (LigmaDrawTool          *draw_tool);

static void          ligma_iscissors_tool_push_undo      (LigmaIscissorsTool     *iscissors);
static void          ligma_iscissors_tool_pop_undo       (LigmaIscissorsTool     *iscissors);
static void          ligma_iscissors_tool_free_redo      (LigmaIscissorsTool     *iscissors);

static void          ligma_iscissors_tool_halt           (LigmaIscissorsTool     *iscissors,
                                                         LigmaDisplay           *display);
static void          ligma_iscissors_tool_commit         (LigmaIscissorsTool     *iscissors,
                                                         LigmaDisplay           *display);

static void          iscissors_convert         (LigmaIscissorsTool *iscissors,
                                                LigmaDisplay       *display);
static GeglBuffer  * gradient_map_new          (LigmaPickable      *pickable);

static void          find_optimal_path         (GeglBuffer        *gradient_map,
                                                LigmaTempBuf       *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2,
                                                gint               xs,
                                                gint               ys);
static void          find_max_gradient         (LigmaIscissorsTool *iscissors,
                                                LigmaPickable      *pickable,
                                                gint              *x,
                                                gint              *y);
static void          calculate_segment         (LigmaIscissorsTool *iscissors,
                                                ISegment          *segment);
static LigmaCanvasItem * iscissors_draw_segment (LigmaDrawTool      *draw_tool,
                                                ISegment          *segment);

static gint          mouse_over_vertex         (LigmaIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_vertex         (LigmaIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static GList       * mouse_over_segment        (LigmaIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);
static gboolean      clicked_on_segment        (LigmaIscissorsTool *iscissors,
                                                gdouble            x,
                                                gdouble            y);

static GPtrArray   * plot_pixels               (LigmaTempBuf       *dp_buf,
                                                gint               x1,
                                                gint               y1,
                                                gint               xs,
                                                gint               ys,
                                                gint               xe,
                                                gint               ye);

static ISegment    * isegment_new              (gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2);
static ISegment    * isegment_copy             (ISegment          *segment);
static void          isegment_free             (ISegment          *segment);

static ICurve      * icurve_new                (void);
static ICurve      * icurve_copy               (ICurve            *curve);
static void          icurve_clear              (ICurve            *curve);
static void          icurve_free               (ICurve            *curve);

static ISegment    * icurve_append_segment     (ICurve            *curve,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2);
static ISegment    * icurve_insert_segment     (ICurve            *curve,
                                                GList             *sibling,
                                                gint               x1,
                                                gint               y1,
                                                gint               x2,
                                                gint               y2);
static void          icurve_delete_segment     (ICurve            *curve,
                                                ISegment          *segment);

static void          icurve_close              (ICurve            *curve);

static LigmaScanConvert *
                    icurve_create_scan_convert (ICurve            *curve);


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

static gfloat  distance_weights[GRADIENT_SEARCH * GRADIENT_SEARCH];

static gint    diagonal_weight[256];
static gint    direction_value[256][4];


G_DEFINE_TYPE (LigmaIscissorsTool, ligma_iscissors_tool,
               LIGMA_TYPE_SELECTION_TOOL)

#define parent_class ligma_iscissors_tool_parent_class


void
ligma_iscissors_tool_register (LigmaToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (LIGMA_TYPE_ISCISSORS_TOOL,
                LIGMA_TYPE_ISCISSORS_OPTIONS,
                ligma_iscissors_options_gui,
                0,
                "ligma-iscissors-tool",
                _("Scissors Select"),
                _("Scissors Select Tool: Select shapes using intelligent edge-fitting"),
                N_("Intelligent _Scissors"),
                "I",
                NULL, LIGMA_HELP_TOOL_ISCISSORS,
                LIGMA_ICON_TOOL_ISCISSORS,
                data);
}

static void
ligma_iscissors_tool_class_init (LigmaIscissorsToolClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  LigmaToolClass     *tool_class      = LIGMA_TOOL_CLASS (klass);
  LigmaDrawToolClass *draw_tool_class = LIGMA_DRAW_TOOL_CLASS (klass);
  gint               i, j;
  gint               radius;

  object_class->finalize     = ligma_iscissors_tool_finalize;

  tool_class->control        = ligma_iscissors_tool_control;
  tool_class->button_press   = ligma_iscissors_tool_button_press;
  tool_class->button_release = ligma_iscissors_tool_button_release;
  tool_class->motion         = ligma_iscissors_tool_motion;
  tool_class->key_press      = ligma_iscissors_tool_key_press;
  tool_class->oper_update    = ligma_iscissors_tool_oper_update;
  tool_class->cursor_update  = ligma_iscissors_tool_cursor_update;
  tool_class->can_undo       = ligma_iscissors_tool_can_undo;
  tool_class->can_redo       = ligma_iscissors_tool_can_redo;
  tool_class->undo           = ligma_iscissors_tool_undo;
  tool_class->redo           = ligma_iscissors_tool_redo;

  draw_tool_class->draw      = ligma_iscissors_tool_draw;

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

  /*  compute the distance weights  */
  radius = GRADIENT_SEARCH >> 1;

  for (i = 0; i < GRADIENT_SEARCH; i++)
    for (j = 0; j < GRADIENT_SEARCH; j++)
      distance_weights[i * GRADIENT_SEARCH + j] =
        1.0 / (1 + sqrt (SQR (i - radius) + SQR (j - radius)));
}

static void
ligma_iscissors_tool_init (LigmaIscissorsTool *iscissors)
{
  LigmaTool *tool = LIGMA_TOOL (iscissors);

  ligma_tool_control_set_scroll_lock (tool->control, TRUE);
  ligma_tool_control_set_snap_to     (tool->control, FALSE);
  ligma_tool_control_set_preserve    (tool->control, FALSE);
  ligma_tool_control_set_dirty_mask  (tool->control,
                                     LIGMA_DIRTY_IMAGE_SIZE |
                                     LIGMA_DIRTY_ACTIVE_DRAWABLE);
  ligma_tool_control_set_tool_cursor (tool->control,
                                     LIGMA_TOOL_CURSOR_ISCISSORS);

  iscissors->op    = ISCISSORS_OP_NONE;
  iscissors->curve = icurve_new ();
  iscissors->state = NO_ACTION;
}

static void
ligma_iscissors_tool_finalize (GObject *object)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (object);

  icurve_free (iscissors->curve);
  iscissors->curve = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_iscissors_tool_control (LigmaTool       *tool,
                             LigmaToolAction  action,
                             LigmaDisplay    *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  switch (action)
    {
    case LIGMA_TOOL_ACTION_PAUSE:
    case LIGMA_TOOL_ACTION_RESUME:
      break;

    case LIGMA_TOOL_ACTION_HALT:
      ligma_iscissors_tool_halt (iscissors, display);
      break;

    case LIGMA_TOOL_ACTION_COMMIT:
      ligma_iscissors_tool_commit (iscissors, display);
      break;
    }

  LIGMA_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static void
ligma_iscissors_tool_button_press (LigmaTool            *tool,
                                  const LigmaCoords    *coords,
                                  guint32              time,
                                  GdkModifierType      state,
                                  LigmaButtonPressType  press_type,
                                  LigmaDisplay         *display)
{
  LigmaIscissorsTool    *iscissors = LIGMA_ISCISSORS_TOOL (tool);
  LigmaIscissorsOptions *options   = LIGMA_ISCISSORS_TOOL_GET_OPTIONS (tool);
  LigmaImage            *image     = ligma_display_get_image (display);
  ISegment             *segment;

  iscissors->x = RINT (coords->x);
  iscissors->y = RINT (coords->y);

  /*  If the tool was being used in another image...reset it  */
  if (display != tool->display)
    ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);

  ligma_tool_control_activate (tool->control);
  tool->display = display;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  switch (iscissors->state)
    {
    case NO_ACTION:
      iscissors->state = SEED_PLACEMENT;

      if (! (state & ligma_get_extend_selection_mask ()))
        find_max_gradient (iscissors, LIGMA_PICKABLE (image),
                           &iscissors->x, &iscissors->y);

      iscissors->x = CLAMP (iscissors->x, 0, ligma_image_get_width  (image) - 1);
      iscissors->y = CLAMP (iscissors->y, 0, ligma_image_get_height (image) - 1);

      ligma_iscissors_tool_push_undo (iscissors);

      segment = icurve_append_segment (iscissors->curve,
                                       iscissors->x,
                                       iscissors->y,
                                       iscissors->x,
                                       iscissors->y);

      /*  Initialize the draw tool only on starting the tool  */
      ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
      break;

    default:
      /*  Check if the mouse click occurred on a vertex or the curve itself  */
      if (clicked_on_vertex (iscissors, coords->x, coords->y))
        {
          iscissors->state = SEED_ADJUSTMENT;

          /*  recalculate both segments  */
          if (iscissors->segment1)
            {
              iscissors->segment1->x1 = iscissors->x;
              iscissors->segment1->y1 = iscissors->y;

              if (options->interactive)
                calculate_segment (iscissors, iscissors->segment1);
            }

          if (iscissors->segment2)
            {
              iscissors->segment2->x2 = iscissors->x;
              iscissors->segment2->y2 = iscissors->y;

              if (options->interactive)
                calculate_segment (iscissors, iscissors->segment2);
            }
        }
      /*  If the iscissors is closed, check if the click was inside  */
      else if (iscissors->curve->closed && iscissors->mask &&
               ligma_pickable_get_opacity_at (LIGMA_PICKABLE (iscissors->mask),
                                             iscissors->x,
                                             iscissors->y))
        {
          ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
        }
      else if (! iscissors->curve->closed)
        {
          /*  if we're not closed, we're adding a new point  */

          ISegment *last = g_queue_peek_tail (iscissors->curve->segments);

          iscissors->state = SEED_PLACEMENT;

          ligma_iscissors_tool_push_undo (iscissors);

          if (last->x1 == last->x2 &&
              last->y1 == last->y2)
            {
              last->x2 = iscissors->x;
              last->y2 = iscissors->y;

              if (options->interactive)
                calculate_segment (iscissors, last);
            }
          else
            {
              segment = icurve_append_segment (iscissors->curve,
                                               last->x2,
                                               last->y2,
                                               iscissors->x,
                                               iscissors->y);

              if (options->interactive)
                calculate_segment (iscissors, segment);
            }
        }
      break;
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
iscissors_convert (LigmaIscissorsTool *iscissors,
                   LigmaDisplay       *display)
{
  LigmaSelectionOptions *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (iscissors);
  LigmaImage            *image   = ligma_display_get_image (display);
  LigmaScanConvert      *sc;

  sc = icurve_create_scan_convert (iscissors->curve);

  if (iscissors->mask)
    g_object_unref (iscissors->mask);

  iscissors->mask = ligma_channel_new_mask (image,
                                           ligma_image_get_width  (image),
                                           ligma_image_get_height (image));
  ligma_scan_convert_render (sc,
                            ligma_drawable_get_buffer (LIGMA_DRAWABLE (iscissors->mask)),
                            0, 0, options->antialias);

  ligma_scan_convert_free (sc);
}

static void
ligma_iscissors_tool_button_release (LigmaTool              *tool,
                                    const LigmaCoords      *coords,
                                    guint32                time,
                                    GdkModifierType        state,
                                    LigmaButtonReleaseType  release_type,
                                    LigmaDisplay           *display)
{
  LigmaIscissorsTool    *iscissors = LIGMA_ISCISSORS_TOOL (tool);
  LigmaIscissorsOptions *options   = LIGMA_ISCISSORS_TOOL_GET_OPTIONS (tool);

  ligma_tool_control_halt (tool->control);

  /* Make sure X didn't skip the button release event -- as it's known
   * to do
   */
  if (iscissors->state == WAITING)
    return;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if (release_type != LIGMA_BUTTON_RELEASE_CANCEL)
    {
      /*  Progress to the next stage of intelligent selection  */
      switch (iscissors->state)
        {
        case SEED_PLACEMENT:
          /*  Add a new segment  */
          if (! iscissors->curve->first_point)
            {
              /*  Determine if we're connecting to the first point  */

              ISegment *segment = g_queue_peek_head (iscissors->curve->segments);

              if (ligma_draw_tool_on_handle (LIGMA_DRAW_TOOL (tool), display,
                                            iscissors->x, iscissors->y,
                                            LIGMA_HANDLE_CIRCLE,
                                            segment->x1, segment->y1,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_HANDLE_ANCHOR_CENTER))
                {
                  iscissors->x = segment->x1;
                  iscissors->y = segment->y1;

                  icurve_close (iscissors->curve);

                  if (! options->interactive)
                    {
                      segment = g_queue_peek_tail (iscissors->curve->segments);
                      calculate_segment (iscissors, segment);
                    }

                  ligma_iscissors_tool_free_redo (iscissors);
                }
              else
                {
                  segment = g_queue_peek_tail (iscissors->curve->segments);

                  if (segment->x1 != segment->x2 ||
                      segment->y1 != segment->y2)
                    {
                      if (! options->interactive)
                        calculate_segment (iscissors, segment);

                      ligma_iscissors_tool_free_redo (iscissors);
                    }
                  else
                    {
                      ligma_iscissors_tool_pop_undo (iscissors);
                    }
                }
            }
          else /* this was our first point */
            {
              iscissors->curve->first_point = FALSE;

              ligma_iscissors_tool_free_redo (iscissors);
            }
          break;

        case SEED_ADJUSTMENT:
          if (state & ligma_get_modify_selection_mask ())
            {
              if (iscissors->segment1 && iscissors->segment2)
                {
                  icurve_delete_segment (iscissors->curve,
                                         iscissors->segment2);

                  calculate_segment (iscissors, iscissors->segment1);
                }
            }
          else
            {
              /*  recalculate both segments  */

              if (iscissors->segment1)
                {
                  if (! options->interactive)
                    calculate_segment (iscissors, iscissors->segment1);
                }

              if (iscissors->segment2)
                {
                  if (! options->interactive)
                    calculate_segment (iscissors, iscissors->segment2);
                }
            }

          ligma_iscissors_tool_free_redo (iscissors);
          break;

        default:
          break;
        }
    }
  else
    {
      switch (iscissors->state)
        {
        case SEED_PLACEMENT:
        case SEED_ADJUSTMENT:
          ligma_iscissors_tool_pop_undo (iscissors);
          break;

        default:
          break;
        }
    }

  if (iscissors->curve->first_point)
    iscissors->state = NO_ACTION;
  else
    iscissors->state = WAITING;

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  /*  convert the curves into a region  */
  if (iscissors->curve->closed)
    iscissors_convert (iscissors, display);
}

static void
ligma_iscissors_tool_motion (LigmaTool         *tool,
                            const LigmaCoords *coords,
                            guint32           time,
                            GdkModifierType   state,
                            LigmaDisplay      *display)
{
  LigmaIscissorsTool    *iscissors = LIGMA_ISCISSORS_TOOL (tool);
  LigmaIscissorsOptions *options   = LIGMA_ISCISSORS_TOOL_GET_OPTIONS (tool);
  LigmaImage            *image     = ligma_display_get_image (display);
  ISegment             *segment;

  if (iscissors->state == NO_ACTION)
    return;

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  iscissors->x = RINT (coords->x);
  iscissors->y = RINT (coords->y);

  /*  Hold the shift key down to disable the auto-edge snap feature  */
  if (! (state & ligma_get_extend_selection_mask ()))
    find_max_gradient (iscissors, LIGMA_PICKABLE (image),
                       &iscissors->x, &iscissors->y);

  iscissors->x = CLAMP (iscissors->x, 0, ligma_image_get_width  (image) - 1);
  iscissors->y = CLAMP (iscissors->y, 0, ligma_image_get_height (image) - 1);

  switch (iscissors->state)
    {
    case SEED_PLACEMENT:
      segment = g_queue_peek_tail (iscissors->curve->segments);

      segment->x2 = iscissors->x;
      segment->y2 = iscissors->y;

      if (iscissors->curve->first_point)
        {
          segment->x1 = segment->x2;
          segment->y1 = segment->y2;
        }
      else
        {
          if (options->interactive)
            calculate_segment (iscissors, segment);
        }
      break;

    case SEED_ADJUSTMENT:
      if (iscissors->segment1)
        {
          iscissors->segment1->x1 = iscissors->x;
          iscissors->segment1->y1 = iscissors->y;

          if (options->interactive)
            calculate_segment (iscissors, iscissors->segment1);
        }

      if (iscissors->segment2)
        {
          iscissors->segment2->x2 = iscissors->x;
          iscissors->segment2->y2 = iscissors->y;

          if (options->interactive)
            calculate_segment (iscissors, iscissors->segment2);
        }
      break;

    default:
      break;
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
}

static void
ligma_iscissors_tool_draw (LigmaDrawTool *draw_tool)
{
  LigmaIscissorsTool    *iscissors = LIGMA_ISCISSORS_TOOL (draw_tool);
  LigmaIscissorsOptions *options   = LIGMA_ISCISSORS_TOOL_GET_OPTIONS (draw_tool);
  LigmaCanvasItem       *item;
  GList                *list;

  /*  First, render all segments and lines  */
  if (! iscissors->curve->first_point)
    {
      for (list = g_queue_peek_head_link (iscissors->curve->segments);
           list;
           list = g_list_next (list))
        {
          ISegment *segment = list->data;

          /*  plot the segment  */
          item = iscissors_draw_segment (draw_tool, segment);

          /*  if this segment is currently being added or adjusted  */
          if ((iscissors->state == SEED_PLACEMENT  &&
               ! list->next)
              ||
              (iscissors->state == SEED_ADJUSTMENT &&
               (segment == iscissors->segment1 ||
                segment == iscissors->segment2)))
            {
              if (! options->interactive)
                item = ligma_draw_tool_add_line (draw_tool,
                                                segment->x1, segment->y1,
                                                segment->x2, segment->y2);

              if (item)
                ligma_canvas_item_set_highlight (item, TRUE);
            }
        }
    }

  /*  Then, render the handles on top of the segments  */
  for (list = g_queue_peek_head_link (iscissors->curve->segments);
       list;
       list = g_list_next (list))
    {
      ISegment *segment = list->data;

      if (! iscissors->curve->first_point)
        {
          gboolean adjustment = (iscissors->state == SEED_ADJUSTMENT &&
                                 segment == iscissors->segment1);

          item = ligma_draw_tool_add_handle (draw_tool,
                                            adjustment ?
                                            LIGMA_HANDLE_CROSS :
                                            LIGMA_HANDLE_FILLED_CIRCLE,
                                            segment->x1,
                                            segment->y1,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_HANDLE_ANCHOR_CENTER);

          if (adjustment)
            ligma_canvas_item_set_highlight (item, TRUE);
        }

      /*  Draw the last point if the curve is not closed  */
      if (! list->next && ! iscissors->curve->closed)
        {
          gboolean placement = (iscissors->state == SEED_PLACEMENT);

          item = ligma_draw_tool_add_handle (draw_tool,
                                            placement ?
                                            LIGMA_HANDLE_CROSS :
                                            LIGMA_HANDLE_FILLED_CIRCLE,
                                            segment->x2,
                                            segment->y2,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                            LIGMA_HANDLE_ANCHOR_CENTER);

          if (placement)
            ligma_canvas_item_set_highlight (item, TRUE);
        }
    }
}

static LigmaCanvasItem *
iscissors_draw_segment (LigmaDrawTool *draw_tool,
                        ISegment     *segment)
{
  LigmaCanvasItem *item;
  LigmaVector2    *points;
  gpointer       *point;
  gint            i, len;

  if (! segment->points)
    return NULL;

  len = segment->points->len;

  points = g_new (LigmaVector2, len);

  for (i = 0, point = segment->points->pdata; i < len; i++, point++)
    {
      guint32 coords = GPOINTER_TO_INT (*point);

      points[i].x = (coords & 0x0000ffff);
      points[i].y = (coords >> 16);
    }

  item = ligma_draw_tool_add_lines (draw_tool, points, len, NULL, FALSE);

  g_free (points);

  return item;
}

static void
ligma_iscissors_tool_oper_update (LigmaTool         *tool,
                                 const LigmaCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 LigmaDisplay      *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  LIGMA_TOOL_CLASS (parent_class)->oper_update (tool, coords, state, proximity,
                                               display);
  /* parent sets a message in the status bar, but it will be replaced here */

  if (mouse_over_vertex (iscissors, coords->x, coords->y) > 1)
    {
      GdkModifierType snap_mask   = ligma_get_extend_selection_mask ();
      GdkModifierType remove_mask = ligma_get_modify_selection_mask ();

      if (state & remove_mask)
        {
          ligma_tool_replace_status (tool, display,
                                    _("Click to remove this point"));
          iscissors->op = ISCISSORS_OP_REMOVE_POINT;
        }
      else
        {
          gchar *status =
            ligma_suggest_modifiers (_("Click-Drag to move this point"),
                                    (snap_mask | remove_mask) & ~state,
                                    _("%s: disable auto-snap"),
                                    _("%s: remove this point"),
                                    NULL);
          ligma_tool_replace_status (tool, display, "%s", status);
          g_free (status);
          iscissors->op = ISCISSORS_OP_MOVE_POINT;
        }
    }
  else if (mouse_over_segment (iscissors, coords->x, coords->y))
    {
      ISegment *segment = g_queue_peek_head (iscissors->curve->segments);

      if (ligma_draw_tool_on_handle (LIGMA_DRAW_TOOL (tool), display,
                                    RINT (coords->x), RINT (coords->y),
                                    LIGMA_HANDLE_CIRCLE,
                                    segment->x1, segment->y1,
                                    LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                    LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                    LIGMA_HANDLE_ANCHOR_CENTER))
        {
          ligma_tool_replace_status (tool, display,
                                    _("Click to close the curve"));
          iscissors->op = ISCISSORS_OP_CONNECT;
        }
      else
        {
          ligma_tool_replace_status (tool, display,
                                    _("Click to add a point on this segment"));
          iscissors->op = ISCISSORS_OP_ADD_POINT;
        }
    }
  else if (iscissors->curve->closed && iscissors->mask)
    {
      if (ligma_pickable_get_opacity_at (LIGMA_PICKABLE (iscissors->mask),
                                        RINT (coords->x),
                                        RINT (coords->y)))
        {
          if (proximity)
            {
              ligma_tool_replace_status (tool, display,
                                        _("Click or press Enter to convert to"
                                          " a selection"));
            }
          iscissors->op = ISCISSORS_OP_SELECT;
        }
      else
        {
          if (proximity)
            {
              ligma_tool_replace_status (tool, display,
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
              GdkModifierType  snap_mask = ligma_get_extend_selection_mask ();
              gchar           *status;

              status = ligma_suggest_modifiers (_("Click or Click-Drag to add a"
                                                 " point"),
                                               snap_mask & ~state,
                                               _("%s: disable auto-snap"),
                                               NULL, NULL);
              ligma_tool_replace_status (tool, display, "%s", status);
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
ligma_iscissors_tool_cursor_update (LigmaTool         *tool,
                                   const LigmaCoords *coords,
                                   GdkModifierType   state,
                                   LigmaDisplay      *display)
{
  LigmaIscissorsTool  *iscissors = LIGMA_ISCISSORS_TOOL (tool);
  LigmaCursorModifier  modifier  = LIGMA_CURSOR_MODIFIER_NONE;

  switch (iscissors->op)
    {
    case ISCISSORS_OP_SELECT:
      {
        LigmaSelectionOptions *options;

        options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);

        /* Do not overwrite the modifiers for add, subtract, intersect */
        if (options->operation == LIGMA_CHANNEL_OP_REPLACE)
          {
            modifier = LIGMA_CURSOR_MODIFIER_SELECT;
          }
      }
      break;

    case ISCISSORS_OP_MOVE_POINT:
      modifier = LIGMA_CURSOR_MODIFIER_MOVE;
      break;

    case ISCISSORS_OP_ADD_POINT:
      modifier = LIGMA_CURSOR_MODIFIER_PLUS;
      break;

    case ISCISSORS_OP_REMOVE_POINT:
      modifier = LIGMA_CURSOR_MODIFIER_MINUS;
      break;

    case ISCISSORS_OP_CONNECT:
      modifier = LIGMA_CURSOR_MODIFIER_JOIN;
      break;

    case ISCISSORS_OP_IMPOSSIBLE:
      modifier = LIGMA_CURSOR_MODIFIER_BAD;
      break;

    default:
      break;
    }

  if (modifier != LIGMA_CURSOR_MODIFIER_NONE)
    {
      ligma_tool_set_cursor (tool, display,
                            LIGMA_CURSOR_MOUSE,
                            LIGMA_TOOL_CURSOR_ISCISSORS,
                            modifier);
    }
  else
    {
      LIGMA_TOOL_CLASS (parent_class)->cursor_update (tool, coords,
                                                     state, display);
    }
}

static gboolean
ligma_iscissors_tool_key_press (LigmaTool    *tool,
                               GdkEventKey *kevent,
                               LigmaDisplay *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  if (display != tool->display)
    return FALSE;

  switch (kevent->keyval)
    {
    case GDK_KEY_BackSpace:
      if (! iscissors->curve->closed &&
          g_queue_peek_tail (iscissors->curve->segments))
        {
          ISegment *segment = g_queue_peek_tail (iscissors->curve->segments);

          if (g_queue_get_length (iscissors->curve->segments) > 1)
            {
              ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

              ligma_iscissors_tool_push_undo (iscissors);
              icurve_delete_segment (iscissors->curve, segment);
              ligma_iscissors_tool_free_redo (iscissors);

              ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
            }
          else if (segment->x2 != segment->x1 || segment->y2 != segment->y1)
            {
              ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

              ligma_iscissors_tool_push_undo (iscissors);
              segment->x2 = segment->x1;
              segment->y2 = segment->y1;
              g_ptr_array_remove_range (segment->points,
                                        0, segment->points->len);
              ligma_iscissors_tool_free_redo (iscissors);

              ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));
            }
          else
            {
              ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
            }
          return TRUE;
        }
      return FALSE;

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    case GDK_KEY_ISO_Enter:
      if (iscissors->curve->closed && iscissors->mask)
        {
          ligma_tool_control (tool, LIGMA_TOOL_ACTION_COMMIT, display);
          return TRUE;
        }
      return FALSE;

    case GDK_KEY_Escape:
      ligma_tool_control (tool, LIGMA_TOOL_ACTION_HALT, display);
      return TRUE;

    default:
      return FALSE;
    }
}

static const gchar *
ligma_iscissors_tool_can_undo (LigmaTool    *tool,
                              LigmaDisplay *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  if (! iscissors->undo_stack)
    return NULL;

  return _("Modify Scissors Curve");
}

static const gchar *
ligma_iscissors_tool_can_redo (LigmaTool    *tool,
                              LigmaDisplay *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  if (! iscissors->redo_stack)
    return NULL;

  return _("Modify Scissors Curve");
}

static gboolean
ligma_iscissors_tool_undo (LigmaTool    *tool,
                          LigmaDisplay *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  iscissors->redo_stack = g_list_prepend (iscissors->redo_stack,
                                          iscissors->curve);

  iscissors->curve = iscissors->undo_stack->data;

  iscissors->undo_stack = g_list_remove (iscissors->undo_stack,
                                         iscissors->curve);

  if (! iscissors->undo_stack)
    {
      iscissors->state = NO_ACTION;

      ligma_draw_tool_stop (LIGMA_DRAW_TOOL (tool));
    }

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  return TRUE;
}

static gboolean
ligma_iscissors_tool_redo (LigmaTool    *tool,
                          LigmaDisplay *display)
{
  LigmaIscissorsTool *iscissors = LIGMA_ISCISSORS_TOOL (tool);

  ligma_draw_tool_pause (LIGMA_DRAW_TOOL (tool));

  if (! iscissors->undo_stack)
    {
      iscissors->state = WAITING;

      ligma_draw_tool_start (LIGMA_DRAW_TOOL (tool), display);
    }

  iscissors->undo_stack = g_list_prepend (iscissors->undo_stack,
                                          iscissors->curve);

  iscissors->curve = iscissors->redo_stack->data;

  iscissors->redo_stack = g_list_remove (iscissors->redo_stack,
                                         iscissors->curve);

  ligma_draw_tool_resume (LIGMA_DRAW_TOOL (tool));

  return TRUE;
}

static void
ligma_iscissors_tool_push_undo (LigmaIscissorsTool *iscissors)
{
  iscissors->undo_stack = g_list_prepend (iscissors->undo_stack,
                                          icurve_copy (iscissors->curve));
}

static void
ligma_iscissors_tool_pop_undo (LigmaIscissorsTool *iscissors)
{
  icurve_free (iscissors->curve);
  iscissors->curve = iscissors->undo_stack->data;

  iscissors->undo_stack = g_list_remove (iscissors->undo_stack,
                                         iscissors->curve);

  if (! iscissors->undo_stack)
    {
      iscissors->state = NO_ACTION;

      ligma_draw_tool_stop (LIGMA_DRAW_TOOL (iscissors));
    }
}

static void
ligma_iscissors_tool_free_redo (LigmaIscissorsTool *iscissors)
{
  g_list_free_full (iscissors->redo_stack,
                    (GDestroyNotify) icurve_free);
  iscissors->redo_stack = NULL;

  /*  update the undo actions / menu items  */
  ligma_image_flush (ligma_display_get_image (LIGMA_TOOL (iscissors)->display));
}

static void
ligma_iscissors_tool_halt (LigmaIscissorsTool *iscissors,
                          LigmaDisplay       *display)
{
  icurve_clear (iscissors->curve);

  iscissors->segment1 = NULL;
  iscissors->segment2 = NULL;
  iscissors->state    = NO_ACTION;

  if (iscissors->undo_stack)
    {
      g_list_free_full (iscissors->undo_stack, (GDestroyNotify) icurve_free);
      iscissors->undo_stack = NULL;
    }

  if (iscissors->redo_stack)
    {
      g_list_free_full (iscissors->redo_stack, (GDestroyNotify) icurve_free);
      iscissors->redo_stack = NULL;
    }

  g_clear_object (&iscissors->gradient_map);
  g_clear_object (&iscissors->mask);
}

static void
ligma_iscissors_tool_commit (LigmaIscissorsTool *iscissors,
                            LigmaDisplay       *display)
{
  LigmaTool             *tool    = LIGMA_TOOL (iscissors);
  LigmaSelectionOptions *options = LIGMA_SELECTION_TOOL_GET_OPTIONS (tool);
  LigmaImage            *image   = ligma_display_get_image (display);

  if (! iscissors->curve->closed)
    {
      ISegment *first = g_queue_peek_head (iscissors->curve->segments);
      ISegment *last  = g_queue_peek_tail (iscissors->curve->segments);

      if (first && last && first != last)
        {
          ISegment *segment;

          segment = icurve_append_segment (iscissors->curve,
                                           last->x2,
                                           last->y2,
                                           first->x1,
                                           first->y1);
          icurve_close (iscissors->curve);
          calculate_segment (iscissors, segment);

          iscissors_convert (iscissors, display);
        }
    }

  if (iscissors->curve->closed && iscissors->mask)
    {
      ligma_channel_select_channel (ligma_image_get_mask (image),
                                   ligma_tool_get_undo_desc (tool),
                                   iscissors->mask,
                                   0, 0,
                                   options->operation,
                                   options->feather,
                                   options->feather_radius,
                                   options->feather_radius);

      ligma_image_flush (image);
    }
}


/* XXX need some scan-conversion routines from somewhere.  maybe. ? */

static gint
mouse_over_vertex (LigmaIscissorsTool *iscissors,
                   gdouble            x,
                   gdouble            y)
{
  GList *list;
  gint   segments_found = 0;

  /*  traverse through the list, returning non-zero if the current cursor
   *  position is on an existing curve vertex.  Set the segment1 and segment2
   *  variables to the two segments containing the vertex in question
   */

  iscissors->segment1 = iscissors->segment2 = NULL;

  for (list = g_queue_peek_head_link (iscissors->curve->segments);
       list;
       list = g_list_next (list))
    {
      ISegment *segment = list->data;

      if (ligma_draw_tool_on_handle (LIGMA_DRAW_TOOL (iscissors),
                                    LIGMA_TOOL (iscissors)->display,
                                    x, y,
                                    LIGMA_HANDLE_CIRCLE,
                                    segment->x1, segment->y1,
                                    LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                    LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                    LIGMA_HANDLE_ANCHOR_CENTER))
        {
          iscissors->segment1 = segment;

          if (segments_found++)
            return segments_found;
        }
      else if (ligma_draw_tool_on_handle (LIGMA_DRAW_TOOL (iscissors),
                                         LIGMA_TOOL (iscissors)->display,
                                         x, y,
                                         LIGMA_HANDLE_CIRCLE,
                                         segment->x2, segment->y2,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_TOOL_HANDLE_SIZE_CIRCLE,
                                         LIGMA_HANDLE_ANCHOR_CENTER))
        {
          iscissors->segment2 = segment;

          if (segments_found++)
            return segments_found;
        }
    }

  return segments_found;
}

static gboolean
clicked_on_vertex (LigmaIscissorsTool *iscissors,
                   gdouble            x,
                   gdouble            y)
{
  gint segments_found  = mouse_over_vertex (iscissors, x, y);

  if (segments_found > 1)
    {
      ligma_iscissors_tool_push_undo (iscissors);

      return TRUE;
    }

  /*  if only one segment was found, the segments are unconnected, and
   *  the user only wants to move either the first or last point
   *  disallow this for now.
   */
  if (segments_found == 1)
    return FALSE;

  return clicked_on_segment (iscissors, x, y);
}


static GList *
mouse_over_segment (LigmaIscissorsTool *iscissors,
                    gdouble            x,
                    gdouble            y)
{
  GList *list;

  /*  traverse through the list, returning the curve segment's list element
   *  if the current cursor position is on a curve...
   */
  for (list = g_queue_peek_head_link (iscissors->curve->segments);
       list;
       list = g_list_next (list))
    {
      ISegment *segment = list->data;
      gpointer *pt;
      gint      len;

      if (! segment->points)
        continue;

      pt  = segment->points->pdata;
      len = segment->points->len;

      while (len--)
        {
          guint32 coords = GPOINTER_TO_INT (*pt);
          gint    tx, ty;

          pt++;
          tx = coords & 0x0000ffff;
          ty = coords >> 16;

          /*  Is the specified point close enough to the segment?  */
          if (ligma_draw_tool_calc_distance_square (LIGMA_DRAW_TOOL (iscissors),
                                                   LIGMA_TOOL (iscissors)->display,
                                                   tx, ty,
                                                   x, y) < SQR (LIGMA_TOOL_HANDLE_SIZE_CIRCLE / 2))
            {
              return list;
            }
        }
    }

  return NULL;
}

static gboolean
clicked_on_segment (LigmaIscissorsTool *iscissors,
                    gdouble            x,
                    gdouble            y)
{
  GList *list = mouse_over_segment (iscissors, x, y);

  /*  traverse through the list, getting back the curve segment's list
   *  element if the current cursor position is on a segment...
   *  If this occurs, replace the segment with two new segments,
   *  separated by a new vertex.
   */

  if (list)
    {
      ISegment *segment = list->data;
      ISegment *new_segment;

      ligma_iscissors_tool_push_undo (iscissors);

      new_segment = icurve_insert_segment (iscissors->curve,
                                           list,
                                           iscissors->x,
                                           iscissors->y,
                                           segment->x2,
                                           segment->y2);

      iscissors->segment1 = new_segment;
      iscissors->segment2 = segment;

      return TRUE;
    }

  return FALSE;
}


static void
calculate_segment (LigmaIscissorsTool *iscissors,
                   ISegment          *segment)
{
  LigmaDisplay  *display  = LIGMA_TOOL (iscissors)->display;
  LigmaPickable *pickable = LIGMA_PICKABLE (ligma_display_get_image (display));
  gint          width;
  gint          height;
  gint          xs, ys, xe, ye;
  gint          x1, y1, x2, y2;
  gint          ewidth, eheight;

  /* Initialise the gradient map buffer for this pickable if we don't
   * already have one.
   */
  if (! iscissors->gradient_map)
    iscissors->gradient_map = gradient_map_new (pickable);

  width  = gegl_buffer_get_width  (iscissors->gradient_map);
  height = gegl_buffer_get_height (iscissors->gradient_map);

  /*  Calculate the lowest cost path from one vertex to the next as specified
   *  by the parameter "segment".
   *    Here are the steps:
   *      1)  Calculate the appropriate working area for this operation
   *      2)  Allocate a temp buf for the dynamic programming array
   *      3)  Run the dynamic programming algorithm to find the optimal path
   *      4)  Translate the optimal path into pixels in the isegment data
   *            structure.
   */

  /*  Get the bounding box  */
  xs = CLAMP (segment->x1, 0, width  - 1);
  ys = CLAMP (segment->y1, 0, height - 1);
  xe = CLAMP (segment->x2, 0, width  - 1);
  ye = CLAMP (segment->y2, 0, height - 1);
  x1 = MIN (xs, xe);
  y1 = MIN (ys, ye);
  x2 = MAX (xs, xe) + 1;  /*  +1 because if xe = 199 & xs = 0, x2 - x1, width = 200  */
  y2 = MAX (ys, ye) + 1;

  /*  expand the boundaries past the ending points by
   *  some percentage of width and height.  This serves the following purpose:
   *  It gives the algorithm more area to search so better solutions
   *  are found.  This is particularly helpful in finding "bumps" which
   *  fall outside the bounding box represented by the start and end
   *  coordinates of the "segment".
   */
  ewidth  = (x2 - x1) * EXTEND_BY + FIXED;
  eheight = (y2 - y1) * EXTEND_BY + FIXED;

  if (xe >= xs)
    x2 += CLAMP (ewidth, 0, width - x2);
  else
    x1 -= CLAMP (ewidth, 0, x1);

  if (ye >= ys)
    y2 += CLAMP (eheight, 0, height - y2);
  else
    y1 -= CLAMP (eheight, 0, y1);

  /* blow away any previous points list we might have */
  if (segment->points)
    {
      g_ptr_array_free (segment->points, TRUE);
      segment->points = NULL;
    }

  if ((x2 - x1) && (y2 - y1))
    {
      /*  If the bounding box has width and height...  */

      LigmaTempBuf *dp_buf; /*  dynamic programming buffer  */
      gint         dp_width  = (x2 - x1);
      gint         dp_height = (y2 - y1);

      dp_buf = ligma_temp_buf_new (dp_width, dp_height,
                                  babl_format ("Y u32"));

      /*  find the optimal path of pixels from (x1, y1) to (x2, y2)  */
      find_optimal_path (iscissors->gradient_map, dp_buf,
                         x1, y1, x2, y2, xs, ys);

      /*  get a list of the pixels in the optimal path  */
      segment->points = plot_pixels (dp_buf, x1, y1, xs, ys, xe, ye);

      ligma_temp_buf_unref (dp_buf);
    }
  else if ((x2 - x1) == 0)
    {
      /*  If the bounding box has no width  */

      /*  plot a vertical line  */
      gint y   = ys;
      gint dir = (ys > ye) ? -1 : 1;

      segment->points = g_ptr_array_new ();
      while (y != ye)
        {
          g_ptr_array_add (segment->points, GINT_TO_POINTER ((y << 16) + xs));
          y += dir;
        }
    }
  else if ((y2 - y1) == 0)
    {
      /*  If the bounding box has no height  */

      /*  plot a horizontal line  */
      gint x   = xs;
      gint dir = (xs > xe) ? -1 : 1;

      segment->points = g_ptr_array_new ();
      while (x != xe)
        {
          g_ptr_array_add (segment->points, GINT_TO_POINTER ((ys << 16) + x));
          x += dir;
        }
    }
}


/* badly need to get a replacement - this is _way_ too expensive */
static gboolean
gradient_map_value (GeglSampler         *map_sampler,
                    const GeglRectangle *map_extent,
                    gint                 x,
                    gint                 y,
                    guint8              *grad,
                    guint8              *dir)
{
  if (x >= map_extent->x     &&
      y >= map_extent->y     &&
      x <  map_extent->width &&
      y <  map_extent->height)
    {
      guint8 sample[2];

      gegl_sampler_get (map_sampler, x, y, NULL, sample, GEGL_ABYSS_NONE);

      *grad = sample[0];
      *dir  = sample[1];

      return TRUE;
    }

  return FALSE;
}

static gint
calculate_link (GeglSampler         *map_sampler,
                const GeglRectangle *map_extent,
                gint                 x,
                gint                 y,
                guint32              pixel,
                gint                 link)
{
  gint   value = 0;
  guint8 grad1, dir1, grad2, dir2;

  if (! gradient_map_value (map_sampler, map_extent, x, y, &grad1, &dir1))
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

  if (! gradient_map_value (map_sampler, map_extent, x, y, &grad2, &dir2))
    {
      grad2 = 0;
      dir2 = 255;
    }

  value +=
    (direction_value[dir1][link] + direction_value[dir2][link]) * OMEGA_D;

  return value;
}


static GPtrArray *
plot_pixels (LigmaTempBuf *dp_buf,
             gint         x1,
             gint         y1,
             gint         xs,
             gint         ys,
             gint         xe,
             gint         ye)
{
  gint       x, y;
  guint32    coords;
  gint       link;
  gint       width = ligma_temp_buf_get_width (dp_buf);
  guint     *data;
  GPtrArray *list;

  /*  Start the data pointer at the correct location  */
  data = (guint *) ligma_temp_buf_get_data (dp_buf) + (ye - y1) * width + (xe - x1);

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


#define PACK(x, y)    ((((y) & 0xff) << 8) | ((x) & 0xff))
#define OFFSET(pixel) ((gint8)((pixel) & 0xff) + \
                       ((gint8)(((pixel) & 0xff00) >> 8)) * \
                       ligma_temp_buf_get_width (dp_buf))

static void
find_optimal_path (GeglBuffer  *gradient_map,
                   LigmaTempBuf *dp_buf,
                   gint         x1,
                   gint         y1,
                   gint         x2,
                   gint         y2,
                   gint         xs,
                   gint         ys)
{
  GeglSampler         *map_sampler;
  const GeglRectangle *map_extent;
  gint                 i, j, k;
  gint                 x, y;
  gint                 link;
  gint                 linkdir;
  gint                 dirx, diry;
  gint                 min_cost;
  gint                 new_cost;
  gint                 offset;
  gint                 cum_cost[8];
  gint                 link_cost[8];
  gint                 pixel_cost[8];
  guint32              pixel[8];
  guint32             *data;
  guint32             *d;
  gint                 dp_buf_width  = ligma_temp_buf_get_width  (dp_buf);
  gint                 dp_buf_height = ligma_temp_buf_get_height (dp_buf);

  /*  initialize the gradient map sampler and extent  */
  map_sampler = gegl_buffer_sampler_new (gradient_map,
                                         gegl_buffer_get_format (gradient_map),
                                         GEGL_SAMPLER_NEAREST);
  map_extent  = gegl_buffer_get_extent (gradient_map);

  /*  initialize the dynamic programming buffer  */
  data = (guint32 *) ligma_temp_buf_data_clear (dp_buf);

  /*  what directions are we filling the array in according to?  */
  dirx = (xs - x1 == 0) ? 1 : -1;
  diry = (ys - y1 == 0) ? 1 : -1;
  linkdir = (dirx * diry);

  y = ys;

  for (i = 0; i < dp_buf_height; i++)
    {
      x = xs;

      d = data + (y-y1) * dp_buf_width + (x-x1);

      for (j = 0; j < dp_buf_width; j++)
        {
          min_cost = G_MAXINT;

          /* pixel[] array encodes how to get to a neighbour, if possible.
           * 0 means no connection (eg edge).
           * Rest packed as bottom two bytes: y offset then x offset.
           * Initially, we assume we can't get anywhere.
           */
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
                pixel[((diry == 1) ? (link + 4) : link)] = PACK (-dirx, -diry);

              link = (linkdir == 1) ? 2 : 3;
              if (j != dp_buf_width - 1)
                pixel[((diry == 1) ? (link + 4) : link)] = PACK (dirx, -diry);
            }

          /*  find the minimum cost of going through each neighbor to reach the
           *  seed point...
           */
          link = -1;
          for (k = 0; k < 8; k ++)
            if (pixel[k])
              {
                link_cost[k] = calculate_link (map_sampler, map_extent,
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
              /*  set the cumulative cost of this pixel and the new direction
               */
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
                      /*  reverse the link direction   /--------------------\ */
                      offset = OFFSET (pixel[k]);
                      d[offset] = (new_cost << 8) + ((k > 3) ? k - 4 : k + 4);
                    }
                  }
            }
          /*  Set the seed point  */
          else if (!i && !j)
            {
              *d = SEED_POINT;
            }

          /*  increment the data pointer and the x counter  */
          d += dirx;
          x += dirx;
        }

      /*  increment the y counter  */
      y += diry;
    }

  g_object_unref (map_sampler);
}

static GeglBuffer *
gradient_map_new (LigmaPickable *pickable)
{
  GeglBuffer      *buffer;
  GeglTileHandler *handler;

  buffer = ligma_pickable_get_buffer (pickable);

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            gegl_buffer_get_width  (buffer),
                                            gegl_buffer_get_height (buffer)),
                            babl_format_n (babl_type ("u8"), 2));

  handler = ligma_tile_handler_iscissors_new (pickable);

  ligma_tile_handler_validate_assign (LIGMA_TILE_HANDLER_VALIDATE (handler),
                                     buffer);

  ligma_tile_handler_validate_invalidate (LIGMA_TILE_HANDLER_VALIDATE (handler),
                                         GEGL_RECTANGLE (0, 0,
                                                         gegl_buffer_get_width  (buffer),
                                                         gegl_buffer_get_height (buffer)));

  g_object_unref (handler);

  return buffer;
}

static void
find_max_gradient (LigmaIscissorsTool *iscissors,
                   LigmaPickable      *pickable,
                   gint              *x,
                   gint              *y)
{
  GeglBufferIterator *iter;
  GeglRectangle      *roi;
  gint                width;
  gint                height;
  gint                radius;
  gint                cx, cy;
  gint                x1, y1, x2, y2;
  gfloat              max_gradient;

  /* Initialise the gradient map buffer for this pickable if we don't
   * already have one.
   */
  if (! iscissors->gradient_map)
    iscissors->gradient_map = gradient_map_new (pickable);

  width  = gegl_buffer_get_width  (iscissors->gradient_map);
  height = gegl_buffer_get_height (iscissors->gradient_map);

  radius = GRADIENT_SEARCH >> 1;

  /*  calculate the extent of the search  */
  cx = CLAMP (*x, 0, width);
  cy = CLAMP (*y, 0, height);
  x1 = CLAMP (cx - radius, 0, width);
  y1 = CLAMP (cy - radius, 0, height);
  x2 = CLAMP (cx + radius, 0, width);
  y2 = CLAMP (cy + radius, 0, height);
  /*  calculate the factor to multiply the distance from the cursor by  */

  max_gradient = 0;
  *x = cx;
  *y = cy;

  iter = gegl_buffer_iterator_new (iscissors->gradient_map,
                                   GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                                   0, NULL,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 1);
  roi = &iter->items[0].roi;

  while (gegl_buffer_iterator_next (iter))
    {
      guint8 *data = iter->items[0].data;
      gint    endx = roi->x + roi->width;
      gint    endy = roi->y + roi->height;
      gint    i, j;

      for (i = roi->y; i < endy; i++)
        {
          const guint8 *gradient = data + 2 * roi->width * (i - roi->y);

          for (j = roi->x; j < endx; j++)
            {
              gfloat g = *gradient;

              gradient += COST_WIDTH;

              g *= distance_weights [(i - y1) * GRADIENT_SEARCH + (j - x1)];

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

static ISegment *
isegment_new (gint x1,
              gint y1,
              gint x2,
              gint y2)
{
  ISegment *segment = g_slice_new0 (ISegment);

  segment->x1 = x1;
  segment->y1 = y1;
  segment->x2 = x2;
  segment->y2 = y2;

  return segment;
}

static ISegment *
isegment_copy (ISegment *segment)
{
  ISegment *copy = isegment_new (segment->x1,
                                 segment->y1,
                                 segment->x2,
                                 segment->y2);

  if (segment->points)
    {
      gint i;

      copy->points = g_ptr_array_sized_new (segment->points->len);

      for (i = 0; i < segment->points->len; i++)
        {
          gpointer value = g_ptr_array_index (segment->points, i);

          g_ptr_array_add (copy->points, value);
        }
    }

  return copy;
}

static void
isegment_free (ISegment *segment)
{
  if (segment->points)
    g_ptr_array_free (segment->points, TRUE);

  g_slice_free (ISegment, segment);
}

static ICurve *
icurve_new (void)
{
  ICurve *curve = g_slice_new0 (ICurve);

  curve->segments    = g_queue_new ();
  curve->first_point = TRUE;

  return curve;
}

static ICurve *
icurve_copy (ICurve *curve)
{
  ICurve *copy = icurve_new ();
  GList  *link;

  for (link = g_queue_peek_head_link (curve->segments);
       link;
       link = g_list_next (link))
    {
      g_queue_push_tail (copy->segments, isegment_copy (link->data));
    }

  copy->first_point = curve->first_point;
  copy->closed      = curve->closed;

  return copy;
}

static void
icurve_clear (ICurve *curve)
{
  while (! g_queue_is_empty (curve->segments))
    isegment_free (g_queue_pop_head (curve->segments));

  curve->first_point = TRUE;
  curve->closed      = FALSE;
}

static void
icurve_free (ICurve *curve)
{
  g_queue_free_full (curve->segments, (GDestroyNotify) isegment_free);

  g_slice_free (ICurve, curve);
}

static ISegment *
icurve_append_segment (ICurve *curve,
                       gint    x1,
                       gint    y1,
                       gint    x2,
                       gint    y2)
{
  ISegment *segment = isegment_new (x1, y1, x2, y2);

  g_queue_push_tail (curve->segments, segment);

  return segment;
}

static ISegment *
icurve_insert_segment (ICurve *curve,
                       GList  *sibling,
                       gint    x1,
                       gint    y1,
                       gint    x2,
                       gint    y2)
{
  ISegment *segment = sibling->data;
  ISegment *new_segment;

  new_segment = isegment_new (x1, y1, x2, y2);

  segment->x2 = x1;
  segment->y2 = y1;

  g_queue_insert_after (curve->segments, sibling, new_segment);

  return new_segment;
}

static void
icurve_delete_segment (ICurve   *curve,
                       ISegment *segment)
{
  GList    *link         = g_queue_find (curve->segments, segment);
  ISegment *next_segment = NULL;

  if (link->next)
    next_segment = link->next->data;
  else if (curve->closed)
    next_segment = g_queue_peek_head (curve->segments);

  if (next_segment)
    {
      next_segment->x1 = segment->x1;
      next_segment->y1 = segment->y1;
    }

  g_queue_remove (curve->segments, segment);
  isegment_free (segment);
}

static void
icurve_close (ICurve *curve)
{
  ISegment *first = g_queue_peek_head (curve->segments);
  ISegment *last  = g_queue_peek_tail (curve->segments);

  last->x2 = first->x1;
  last->y2 = first->y1;

  curve->closed = TRUE;
}

static LigmaScanConvert *
icurve_create_scan_convert (ICurve *curve)
{
  LigmaScanConvert *sc;
  GList           *list;
  LigmaVector2     *points;
  guint            n_total_points = 0;

  sc = ligma_scan_convert_new ();

  for (list = g_queue_peek_tail_link (curve->segments);
       list;
       list = g_list_previous (list))
    {
      ISegment *segment = list->data;

      n_total_points += segment->points->len;
    }

  points = g_new (LigmaVector2, n_total_points);
  n_total_points = 0;

  /* go over the segments in reverse order, adding the points we have */
  for (list = g_queue_peek_tail_link (curve->segments);
       list;
       list = g_list_previous (list))
    {
      ISegment *segment = list->data;
      guint     n_points;
      gint      i;

      n_points = segment->points->len;

      for (i = 0; i < n_points; i++)
        {
          guint32 packed = GPOINTER_TO_INT (g_ptr_array_index (segment->points,
                                                               i));

          points[n_total_points + i].x = packed & 0x0000ffff;
          points[n_total_points + i].y = packed >> 16;
        }

      n_total_points += n_points;
    }

  ligma_scan_convert_add_polyline (sc, n_total_points, points, TRUE);
  g_free (points);

  return sc;
}
