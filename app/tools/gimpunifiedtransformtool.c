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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpunifiedtransformtool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  X0,
  Y0,
  X1,
  Y1,
  X2,
  Y2,
  X3,
  Y3,
  PIVOT_X,
  PIVOT_Y,
};


/*  local function prototypes  */

static void    gimp_unified_transform_tool_dialog        (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_dialog_update (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_prepare       (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_motion        (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_recalc_matrix (GimpTransformTool *tr_tool);
static gchar * gimp_unified_transform_tool_get_undo_desc (GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpUnifiedTransformationTool, gimp_unified_transform_tool,
               GIMP_TYPE_TRANSFORM_TOOL)


void
gimp_unified_transform_tool_register (GimpToolRegisterCallback  callback,
                                      gpointer                  data)
{
  (* callback) (GIMP_TYPE_UNIFIED_TRANSFORM_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                GIMP_CONTEXT_BACKGROUND_MASK,
                "gimp-unified-transform-tool",
                _("Unified Transformation"),
                _("Unified Transformation Tool: "
                  "Transform the layer, selection or path"),
                N_("_Unified Transformation"), "<shift>U",
                NULL, GIMP_HELP_TOOL_UNIFIED_TRANSFORM,
                GIMP_STOCK_TOOL_UNIFIED_TRANSFORM,
                data);
}

static void
gimp_unified_transform_tool_class_init (GimpUnifiedTransformationToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_unified_transform_tool_dialog;
  trans_class->dialog_update = gimp_unified_transform_tool_dialog_update;
  trans_class->prepare       = gimp_unified_transform_tool_prepare;
  trans_class->motion        = gimp_unified_transform_tool_motion;
  trans_class->recalc_matrix = gimp_unified_transform_tool_recalc_matrix;
  trans_class->get_undo_desc = gimp_unified_transform_tool_get_undo_desc;
}

static void
gimp_unified_transform_tool_init (GimpUnifiedTransformationTool *unified_tool)
{
  GimpTool          *tool    = GIMP_TOOL (unified_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (unified_tool);

  gimp_tool_control_set_tool_cursor (tool->control,
                                     GIMP_TOOL_CURSOR_UNIFIED_TRANSFORM);

  tr_tool->progress_text = _("Unified transform");

  tr_tool->use_grid        = TRUE;
  tr_tool->use_handles     = TRUE;
  tr_tool->use_center      = TRUE;
  tr_tool->use_mid_handles = TRUE;
  tr_tool->use_pivot       = TRUE;
}

static void
gimp_unified_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpUnifiedTransformationTool *unified = GIMP_UNIFIED_TRANSFORM_TOOL (tr_tool);
  GtkWidget                     *content_area;
  GtkWidget                     *frame;
  GtkWidget                     *table;
  gint                           x, y;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (tr_tool->dialog));

  frame = gimp_frame_new (_("Transformation Matrix"));
  gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
  gtk_box_pack_start (GTK_BOX (content_area), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      {
        GtkWidget *label = gtk_label_new (" ");

        gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.0);
        gtk_label_set_width_chars (GTK_LABEL (label), 12);
        gtk_table_attach (GTK_TABLE (table), label,
                          x, x + 1, y, y + 1, GTK_EXPAND, GTK_FILL, 0, 0);
        gtk_widget_show (label);

        unified->label[y][x] = label;
      }
}

static void
gimp_unified_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpUnifiedTransformationTool *unified = GIMP_UNIFIED_TRANSFORM_TOOL (tr_tool);
  gint                           x, y;

  for (y = 0; y < 3; y++)
    for (x = 0; x < 3; x++)
      {
        gchar buf[32];

        g_snprintf (buf, sizeof (buf),
                    "%10.5f", tr_tool->transform.coeff[y][x]);

        gtk_label_set_text (GTK_LABEL (unified->label[y][x]), buf);
      }
}

static void
gimp_unified_transform_tool_prepare (GimpTransformTool  *tr_tool)
{
  tr_tool->trans_info[PIVOT_X] = (gdouble) (tr_tool->x1 + tr_tool->x2) / 2.0;
  tr_tool->trans_info[PIVOT_Y] = (gdouble) (tr_tool->y1 + tr_tool->y2) / 2.0;

  tr_tool->trans_info[X0] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1] = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X2] = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y2] = (gdouble) tr_tool->y2;
  tr_tool->trans_info[X3] = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y3] = (gdouble) tr_tool->y2;
}


static inline gdouble dotprod (GimpVector2 a, GimpVector2 b) {
    return a.x*b.x + a.y*b.y;
}

static inline gdouble norm (GimpVector2 a) {
    return sqrt (dotprod (a, a));
}

static inline GimpVector2 vectorsubtract (GimpVector2 a, GimpVector2 b) {
    GimpVector2 c;
    c.x = a.x - b.x;
    c.y = a.y - b.y;
    return c;
}

static inline GimpVector2 vectoradd (GimpVector2 a, GimpVector2 b) {
    GimpVector2 c;
    c.x = a.x + b.x;
    c.y = a.y + b.y;
    return c;
}

static inline GimpVector2 scalemult (GimpVector2 a, gdouble b) {
    GimpVector2 c;
    c.x = a.x * b;
    c.y = a.y * b;
    return c;
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble calcangle (GimpVector2 a, GimpVector2 b) {
    gdouble angle, angle2, length = norm (a) * norm (b);
    angle = acos (dotprod (a, b)/length);
    angle2 = b.y;
    b.y = -b.x;
    b.x = angle2;
    angle2 = acos (dotprod (a, b)/length);
    return -((angle2 > G_PI/2.) ? 2*G_PI-angle : angle);
}

static inline GimpVector2 rotate2d (GimpVector2 p, gdouble angle) {
    GimpVector2 ret;
    ret.x = cos (angle)*p.x-sin (angle)*p.y;
    ret.y = sin (angle)*p.x+cos (angle)*p.y;
    return ret;
}

static void
gimp_unified_transform_tool_motion (GimpTransformTool *transform_tool)
{
  gdouble diff_x = transform_tool->curx - transform_tool->lastx,
          diff_y = transform_tool->cury - transform_tool->lasty;
  gdouble *x[4], *y[4], px[4], py[4], *pivot_x, *pivot_y;
  gint i;
  gboolean horizontal = FALSE;
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (transform_tool);

  x[0] = &transform_tool->trans_info[X0];
  x[1] = &transform_tool->trans_info[X1];
  x[2] = &transform_tool->trans_info[X2];
  x[3] = &transform_tool->trans_info[X3];
  y[0] = &transform_tool->trans_info[Y0];
  y[1] = &transform_tool->trans_info[Y1];
  y[2] = &transform_tool->trans_info[Y2];
  y[3] = &transform_tool->trans_info[Y3];
  
  px[0] = (*transform_tool->prev_trans_info)[X0];
  px[1] = (*transform_tool->prev_trans_info)[X1];
  px[2] = (*transform_tool->prev_trans_info)[X2];
  px[3] = (*transform_tool->prev_trans_info)[X3];
  py[0] = (*transform_tool->prev_trans_info)[Y0];
  py[1] = (*transform_tool->prev_trans_info)[Y1];
  py[2] = (*transform_tool->prev_trans_info)[Y2];
  py[3] = (*transform_tool->prev_trans_info)[Y3];

  pivot_x = &transform_tool->trans_info[PIVOT_X];
  pivot_y = &transform_tool->trans_info[PIVOT_Y];

  if (options->alternate)
    {
      gdouble *x0, *x1, *y0, *y1;
      gboolean moveedge = FALSE;

      switch (transform_tool->function)
        {
        case TRANSFORM_HANDLE_W:
          x0 = x[0]; y0 = y[0];
          x1 = x[2]; y1 = y[2];
          moveedge = TRUE;
          break;

        case TRANSFORM_HANDLE_S:
          x0 = x[2]; y0 = y[2];
          x1 = x[3]; y1 = y[3];
          moveedge = TRUE;
          break;

        case TRANSFORM_HANDLE_N:
          x0 = x[0]; y0 = y[0];
          x1 = x[1]; y1 = y[1];
          moveedge = TRUE;
          break;

        case TRANSFORM_HANDLE_E:
          x0 = x[1]; y0 = y[1];
          x1 = x[3]; y1 = y[3];
          moveedge = TRUE;
          break;

        case TRANSFORM_HANDLE_NW:
          *x[0] += diff_x;
          *y[0] += diff_y;
          return;

        case TRANSFORM_HANDLE_NE:
          *x[1] += diff_x;
          *y[1] += diff_y;
          return;

        case TRANSFORM_HANDLE_SW:
          *x[2] += diff_x;
          *y[2] += diff_y;
          return;

        case TRANSFORM_HANDLE_SE:
          *x[3] += diff_x;
          *y[3] += diff_y;
          return;

        default:
          break;
        }
      if (moveedge)
        {
          *x0 += diff_x;
          *x1 += diff_x;
          *y0 += diff_y;
          *y1 += diff_y;
          return;
        }
    }

  switch (transform_tool->function)
    {
    case TRANSFORM_HANDLE_NW:
    case TRANSFORM_HANDLE_NE:
    case TRANSFORM_HANDLE_SW:
    case TRANSFORM_HANDLE_SE:
    {
      GimpVector2 m = { .x = transform_tool->curx,   .y = transform_tool->cury };
      GimpVector2 p = { .x = transform_tool->mousex, .y = transform_tool->mousey };
      GimpVector2 c = { .x = *pivot_x,               .y = *pivot_y };
      gdouble angle = calcangle (vectorsubtract (m, c), vectorsubtract (p, c));
      for (i = 0; i < 4; i++) {
        p.x = px[i]; p.y = py[i];
        m = vectoradd (c, rotate2d (vectorsubtract (p, c), angle));
        *x[i] = m.x;
        *y[i] = m.y;
      }
      return;
    }
    case TRANSFORM_HANDLE_CENTER:
      *x[0] += diff_x;
      *y[0] += diff_y;
      *x[1] += diff_x;
      *y[1] += diff_y;
      *x[2] += diff_x;
      *y[2] += diff_y;
      *x[3] += diff_x;
      *y[3] += diff_y;
      break;

    case TRANSFORM_HANDLE_PIVOT:
      *pivot_x += diff_x;
      *pivot_y += diff_y;
      break;

    case TRANSFORM_HANDLE_E:
    case TRANSFORM_HANDLE_W:
      horizontal = TRUE;
    case TRANSFORM_HANDLE_N:
    case TRANSFORM_HANDLE_S:
      if (! options->constrain)
        {
          for (i = 0; i < 4; i++)
            {
              if (horizontal)
                *x[i] = *pivot_x + (*pivot_x-transform_tool->curx)/(*pivot_x-transform_tool->mousex)*(px[i]-*pivot_x);
              else
                *y[i] = *pivot_y + (*pivot_y-transform_tool->cury)/(*pivot_y-transform_tool->mousey)*(py[i]-*pivot_y);
            }
        } else {
          GimpVector2 m = { .x = transform_tool->curx,   .y = transform_tool->cury };
          GimpVector2 p = { .x = transform_tool->mousex, .y = transform_tool->mousey };
          GimpVector2 c = { .x = *pivot_x,               .y = *pivot_y };
          gdouble onorm = 1./norm (vectorsubtract (c, p));
          gdouble distance = norm (vectorsubtract (c, m)) * onorm;
          for (i = 0; i < 4; i++) {
            p.x = px[i]; p.y = py[i];
            m = vectoradd (c, scalemult (vectorsubtract (p, c), distance));
            *x[i] = m.x;
            *y[i] = m.y;
          }
        }
      break;

    default:
      break;
    }
}

static void
gimp_unified_transform_tool_recalc_matrix (GimpTransformTool *tr_tool)
{
  tr_tool->px = tr_tool->trans_info[PIVOT_X];
  tr_tool->py = tr_tool->trans_info[PIVOT_Y];

  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_perspective (&tr_tool->transform,
                                     tr_tool->x1,
                                     tr_tool->y1,
                                     tr_tool->x2 - tr_tool->x1,
                                     tr_tool->y2 - tr_tool->y1,
                                     tr_tool->trans_info[X0],
                                     tr_tool->trans_info[Y0],
                                     tr_tool->trans_info[X1],
                                     tr_tool->trans_info[Y1],
                                     tr_tool->trans_info[X2],
                                     tr_tool->trans_info[Y2],
                                     tr_tool->trans_info[X3],
                                     tr_tool->trans_info[Y3]);
}

static gchar *
gimp_unified_transform_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (C_("undo-type", "Unified Transform"));
}
