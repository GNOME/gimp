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


#include "core/gimpboundary.h"
#include "core/gimpchannel.h"
#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"
#include "core/gimp-utils.h"

#include "vectors/gimpvectors.h"
#include "vectors/gimpstroke.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpcanvasgroup.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"

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

static void    gimp_transform_tool_oper_update           (GimpTool              *tool,
                                                          const GimpCoords      *coords,
                                                          GdkModifierType        state,
                                                          gboolean               proximity,
                                                          GimpDisplay           *display);
static void    gimp_unified_transform_tool_draw          (GimpDrawTool      *draw_tool);
static void    gimp_unified_transform_tool_dialog        (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_dialog_update (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_prepare       (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_motion        (GimpTransformTool *tr_tool);
static void    gimp_unified_transform_tool_recalc_matrix (GimpTransformTool *tr_tool);
static gchar * gimp_unified_transform_tool_get_undo_desc (GimpTransformTool *tr_tool);


G_DEFINE_TYPE (GimpUnifiedTransformTool, gimp_unified_transform_tool,
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
                _("Unified Transform"),
                _("Unified Transform Tool: "
                  "Transform the layer, selection or path"),
                N_("_Unified Transform"), "<shift>L",
                NULL, GIMP_HELP_TOOL_UNIFIED_TRANSFORM,
                GIMP_STOCK_TOOL_UNIFIED_TRANSFORM,
                data);
}

static void
gimp_unified_transform_tool_class_init (GimpUnifiedTransformToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpToolClass          *tool_class  = GIMP_TOOL_CLASS (klass);
  GimpDrawToolClass      *draw_class  = GIMP_DRAW_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_unified_transform_tool_dialog;
  trans_class->dialog_update = gimp_unified_transform_tool_dialog_update;
  trans_class->prepare       = gimp_unified_transform_tool_prepare;
  trans_class->motion        = gimp_unified_transform_tool_motion;
  trans_class->recalc_matrix = gimp_unified_transform_tool_recalc_matrix;
  trans_class->get_undo_desc = gimp_unified_transform_tool_get_undo_desc;

  tool_class->oper_update = gimp_transform_tool_oper_update;

  draw_class->draw = gimp_unified_transform_tool_draw;
}

static void
gimp_unified_transform_tool_init (GimpUnifiedTransformTool *unified_tool)
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

/*hack*/
static void
gimp_transform_tool_set_function (GimpTransformTool *tr_tool,
                                  TransformAction    function)
{
  if (function != tr_tool->function)
    {
      if (tr_tool->handles[tr_tool->function] &&
          gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
        {
          gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                          FALSE);
        }

      tr_tool->function = function;

      if (tr_tool->handles[tr_tool->function] &&
          gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
        {
          gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                          TRUE);
        }
    }
}
static void
gimp_transform_tool_oper_update (GimpTool         *tool,
                                 const GimpCoords *coords,
                                 GdkModifierType   state,
                                 gboolean          proximity,
                                 GimpDisplay      *display)
{
  GimpTransformTool *tr_tool   = GIMP_TRANSFORM_TOOL (tool);
  GimpDrawTool      *draw_tool = GIMP_DRAW_TOOL (tool);
  TransformAction    function  = TRANSFORM_HANDLE_NONE;
  TransformAction    i;

  if (display != tool->display || draw_tool->item == NULL)
    {
      gimp_transform_tool_set_function (tr_tool, TRANSFORM_HANDLE_NONE);
      return;
    }

  for (i = TRANSFORM_HANDLE_NONE + 1; i < TRANSFORM_HANDLE_NUM; i++) {
    if (gimp_canvas_item_hit (tr_tool->handles[i], coords->x, coords->y))
      {
        function = i;
        break;
      }
  }

  gimp_transform_tool_set_function (tr_tool, function);
}
/* hack */
static void
gimp_transform_tool_handles_recalc (GimpTransformTool *tr_tool,
                                    GimpDisplay       *display,
                                    gint              *handle_w,
                                    gint              *handle_h)
{
  gint dx1, dy1;
  gint dx2, dy2;
  gint dx3, dy3;
  gint dx4, dy4;
  gint x1, y1;
  gint x2, y2;

  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx1, tr_tool->ty1,
                                   &dx1, &dy1);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx2, tr_tool->ty2,
                                   &dx2, &dy2);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx3, tr_tool->ty3,
                                   &dx3, &dy3);
  gimp_display_shell_transform_xy (gimp_display_get_shell (display),
                                   tr_tool->tx4, tr_tool->ty4,
                                   &dx4, &dy4);

  x1 = MIN4 (dx1, dx2, dx3, dx4);
  y1 = MIN4 (dy1, dy2, dy3, dy4);
  x2 = MAX4 (dx1, dx2, dx3, dx4);
  y2 = MAX4 (dy1, dy2, dy3, dy4);

  *handle_w = CLAMP ((x2 - x1) / 3,
                     6, GIMP_TOOL_HANDLE_SIZE_LARGE);
  *handle_h = CLAMP ((y2 - y1) / 3,
                     6, GIMP_TOOL_HANDLE_SIZE_LARGE);
}
static void
gimp_unified_transform_tool_draw (GimpDrawTool *draw_tool)
{
  GimpTool             *tool    = GIMP_TOOL (draw_tool);
  GimpTransformTool    *tr_tool = GIMP_TRANSFORM_TOOL (draw_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tool);
  GimpImage            *image   = gimp_display_get_image (tool->display);
  GimpCanvasGroup      *stroke_group;
  gint                  handle_w, handle_h;
  gint                  i, d;
  gdouble               x, y;

  for (i = 0; i < G_N_ELEMENTS (tr_tool->handles); i++)
    tr_tool->handles[i] = NULL;

  if (tr_tool->use_grid)
    {
      if (gimp_transform_options_show_preview (options))
        {
          gimp_draw_tool_add_transform_preview (draw_tool,
                                                tool->drawable,
                                                &tr_tool->transform,
                                                tr_tool->x1,
                                                tr_tool->y1,
                                                tr_tool->x2,
                                                tr_tool->y2,
                                                TRUE,
                                                options->preview_opacity);
        }

      gimp_draw_tool_add_transform_guides (draw_tool,
                                           &tr_tool->transform,
                                           options->grid_type,
                                           options->grid_size,
                                           tr_tool->x1,
                                           tr_tool->y1,
                                           tr_tool->x2,
                                           tr_tool->y2);
    }

  gimp_transform_tool_handles_recalc (tr_tool, tool->display,
                                      &handle_w, &handle_h);

  /*  draw the scale handles  */
  tr_tool->handles[TRANSFORM_HANDLE_NW] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx1, tr_tool->ty1,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_NORTH_WEST);

  tr_tool->handles[TRANSFORM_HANDLE_NE] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx2, tr_tool->ty2,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_NORTH_EAST);

  tr_tool->handles[TRANSFORM_HANDLE_SW] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx3, tr_tool->ty3,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_SOUTH_WEST);

  tr_tool->handles[TRANSFORM_HANDLE_SE] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx4, tr_tool->ty4,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_SOUTH_EAST);

  /*  draw the perspective handles  */
  tr_tool->handles[TRANSFORM_HANDLE_NW_P] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx1, tr_tool->ty1,
                               handle_w/4*3, handle_h/4*3,
                               GIMP_HANDLE_ANCHOR_SOUTH_EAST);

  tr_tool->handles[TRANSFORM_HANDLE_NE_P] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx2, tr_tool->ty2,
                               handle_w/4*3, handle_h/4*3,
                               GIMP_HANDLE_ANCHOR_SOUTH_WEST);

  tr_tool->handles[TRANSFORM_HANDLE_SW_P] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx3, tr_tool->ty3,
                               handle_w/4*3, handle_h/4*3,
                               GIMP_HANDLE_ANCHOR_NORTH_EAST);

  tr_tool->handles[TRANSFORM_HANDLE_SE_P] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               tr_tool->tx4, tr_tool->ty4,
                               handle_w/4*3, handle_h/4*3,
                               GIMP_HANDLE_ANCHOR_NORTH_WEST);

  /*  draw the side handles  */
  x = (tr_tool->tx1 + tr_tool->tx2) / 2.0;
  y = (tr_tool->ty1 + tr_tool->ty2) / 2.0;

  tr_tool->handles[TRANSFORM_HANDLE_N] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx2 + tr_tool->tx4) / 2.0;
  y = (tr_tool->ty2 + tr_tool->ty4) / 2.0;

  tr_tool->handles[TRANSFORM_HANDLE_E] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx3 + tr_tool->tx4) / 2.0;
  y = (tr_tool->ty3 + tr_tool->ty4) / 2.0;

  tr_tool->handles[TRANSFORM_HANDLE_S] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx3 + tr_tool->tx1) / 2.0;
  y = (tr_tool->ty3 + tr_tool->ty1) / 2.0;

  tr_tool->handles[TRANSFORM_HANDLE_W] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_SQUARE,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  /*  draw the shear handles  */
  x = (tr_tool->tx1 * 2 + tr_tool->tx2 * 3) / 5;
  y = (tr_tool->ty1 * 2 + tr_tool->ty2 * 3) / 5;

  tr_tool->handles[TRANSFORM_HANDLE_N_S] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_FILLED_DIAMOND,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx2 * 2 + tr_tool->tx4 * 3) / 5;
  y = (tr_tool->ty2 * 2 + tr_tool->ty4 * 3) / 5;

  tr_tool->handles[TRANSFORM_HANDLE_E_S] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_FILLED_DIAMOND,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx3 * 3 + tr_tool->tx4 * 2) / 5;
  y = (tr_tool->ty3 * 3 + tr_tool->ty4 * 2) / 5;

  tr_tool->handles[TRANSFORM_HANDLE_S_S] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_FILLED_DIAMOND,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);

  x = (tr_tool->tx3 * 3 + tr_tool->tx1 * 2) / 5;
  y = (tr_tool->ty3 * 3 + tr_tool->ty1 * 2) / 5;

  tr_tool->handles[TRANSFORM_HANDLE_W_S] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_FILLED_DIAMOND,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);


  /*  draw the rotation handle  */
  x = (tr_tool->tx1 * 3 + tr_tool->tx2 * 2) / 5;
  y = (tr_tool->ty1 * 3 + tr_tool->ty2 * 2) / 5;

  tr_tool->handles[TRANSFORM_HANDLE_ROTATION] =
    gimp_draw_tool_add_handle (draw_tool,
                               GIMP_HANDLE_FILLED_CIRCLE,
                               x, y,
                               handle_w, handle_h,
                               GIMP_HANDLE_ANCHOR_CENTER);


  /*  draw the rotation center axis handle  */
  d = MIN (handle_w, handle_h) * 2; /* so you can grab it from under the center handle */

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  tr_tool->handles[TRANSFORM_HANDLE_PIVOT] = GIMP_CANVAS_ITEM (stroke_group);

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_SQUARE,
                             tr_tool->tpx, tr_tool->tpy,
                             d, d,
                             GIMP_HANDLE_ANCHOR_CENTER);
  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_CROSS,
                             tr_tool->tpx, tr_tool->tpy,
                             d, d,
                             GIMP_HANDLE_ANCHOR_CENTER);

  gimp_draw_tool_pop_group (draw_tool);

  /*  draw the move handle  */
  d = MIN (handle_w, handle_h);

  stroke_group = gimp_draw_tool_add_stroke_group (draw_tool);

  tr_tool->handles[TRANSFORM_HANDLE_CENTER] = GIMP_CANVAS_ITEM (stroke_group);

  gimp_draw_tool_push_group (draw_tool, stroke_group);

  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_CIRCLE,
                             tr_tool->tcx, tr_tool->tcy,
                             d, d,
                             GIMP_HANDLE_ANCHOR_CENTER);
  gimp_draw_tool_add_handle (draw_tool,
                             GIMP_HANDLE_CROSS,
                             tr_tool->tcx, tr_tool->tcy,
                             d, d,
                             GIMP_HANDLE_ANCHOR_CENTER);

  gimp_draw_tool_pop_group (draw_tool);

  if (tr_tool->handles[tr_tool->function])
    {
      gimp_canvas_item_set_highlight (tr_tool->handles[tr_tool->function],
                                      TRUE);
    }

  /* the rest of the function is the same as in the parent class */
  if (options->type == GIMP_TRANSFORM_TYPE_SELECTION)
    {
      GimpMatrix3         matrix = tr_tool->transform;
      const GimpBoundSeg *orig_in;
      const GimpBoundSeg *orig_out;
      GimpBoundSeg       *segs_in;
      GimpBoundSeg       *segs_out;
      gint                num_segs_in;
      gint                num_segs_out;

      gimp_channel_boundary (gimp_image_get_mask (image),
                             &orig_in, &orig_out,
                             &num_segs_in, &num_segs_out,
                             0, 0, 0, 0);

      segs_in  = g_memdup (orig_in,  num_segs_in  * sizeof (GimpBoundSeg));
      segs_out = g_memdup (orig_out, num_segs_out * sizeof (GimpBoundSeg));

      if (segs_in)
        {
          for (i = 0; i < num_segs_in; i++)
            {
              gdouble tx, ty;

              gimp_matrix3_transform_point (&matrix,
                                            segs_in[i].x1, segs_in[i].y1,
                                            &tx, &ty);
              segs_in[i].x1 = RINT (tx);
              segs_in[i].y1 = RINT (ty);

              gimp_matrix3_transform_point (&matrix,
                                            segs_in[i].x2, segs_in[i].y2,
                                            &tx, &ty);
              segs_in[i].x2 = RINT (tx);
              segs_in[i].y2 = RINT (ty);
            }

          gimp_draw_tool_add_boundary (draw_tool,
                                       segs_in, num_segs_in,
                                       NULL,
                                       0, 0);
          g_free (segs_in);
        }

      if (segs_out)
        {
          for (i = 0; i < num_segs_out; i++)
            {
              gdouble tx, ty;

              gimp_matrix3_transform_point (&matrix,
                                            segs_out[i].x1, segs_out[i].y1,
                                            &tx, &ty);
              segs_out[i].x1 = RINT (tx);
              segs_out[i].y1 = RINT (ty);

              gimp_matrix3_transform_point (&matrix,
                                            segs_out[i].x2, segs_out[i].y2,
                                            &tx, &ty);
              segs_out[i].x2 = RINT (tx);
              segs_out[i].y2 = RINT (ty);
            }

          gimp_draw_tool_add_boundary (draw_tool,
                                       segs_out, num_segs_out,
                                       NULL,
                                       0, 0);
          g_free (segs_out);
        }
    }
  else if (options->type == GIMP_TRANSFORM_TYPE_PATH)
    {
      GimpVectors *vectors;
      GimpStroke  *stroke = NULL;
      GimpMatrix3  matrix = tr_tool->transform;

      vectors = gimp_image_get_active_vectors (image);

      if (vectors)
        {
          if (options->direction == GIMP_TRANSFORM_BACKWARD)
            gimp_matrix3_invert (&matrix);

          while ((stroke = gimp_vectors_stroke_get_next (vectors, stroke)))
            {
              GArray   *coords;
              gboolean  closed;

              coords = gimp_stroke_interpolate (stroke, 1.0, &closed);

              if (coords && coords->len)
                {
                  gint i;

                  for (i = 0; i < coords->len; i++)
                    {
                      GimpCoords *curr = &g_array_index (coords, GimpCoords, i);

                      gimp_matrix3_transform_point (&matrix,
                                                    curr->x, curr->y,
                                                    &curr->x, &curr->y);
                    }

                  gimp_draw_tool_add_strokes (draw_tool,
                                              &g_array_index (coords,
                                                              GimpCoords, 0),
                                              coords->len, FALSE);
                }

              if (coords)
                g_array_free (coords, TRUE);
            }
        }
    }
}

static void
gimp_unified_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpUnifiedTransformTool *unified = GIMP_UNIFIED_TRANSFORM_TOOL (tr_tool);
  GtkWidget                     *content_area;
  GtkWidget                     *frame;
  GtkWidget                     *table;
  gint                           x, y;

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (tr_tool->dialog));

  frame = gimp_frame_new (_("Transform Matrix"));
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
  GimpUnifiedTransformTool *unified = GIMP_UNIFIED_TRANSFORM_TOOL (tr_tool);
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

static inline GimpVector2 vectorproject (GimpVector2 a, GimpVector2 b) {
    return scalemult (b, dotprod (a, b)/dotprod (b, b));
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble calcangle (GimpVector2 a, GimpVector2 b) {
    gdouble angle, angle2, length = norm (a) * norm (b);
    angle = acos (dotprod (a, b)/length);
    angle2 = b.y;
    b.y = -b.x;
    b.x = angle2;
    angle2 = acos (dotprod (a, b)/length);
    return ((angle2 > G_PI/2.) ? angle : 2*G_PI-angle);
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
  gdouble dx = transform_tool->curx - transform_tool->mousex;
  gdouble dy = transform_tool->cury - transform_tool->mousey;
  gdouble *x[4], *y[4], px[5], py[5], *pivot_x, *pivot_y, ppivot_x, ppivot_y;
  gint i;
  gboolean horizontal = FALSE;
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (transform_tool);
  gboolean constrain = options->constrain;
  gboolean fromcenter = options->alternate;
  TransformAction function = transform_tool->function;

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

  /* put center point in this array too */
  px[4] = (px[0] + px[1] + px[2] + px[3]) / 4.;
  py[4] = (py[0] + py[1] + py[2] + py[3]) / 4.;
  
  pivot_x = &transform_tool->trans_info[PIVOT_X];
  pivot_y = &transform_tool->trans_info[PIVOT_Y];

  ppivot_x = (*transform_tool->prev_trans_info)[PIVOT_X];
  ppivot_y = (*transform_tool->prev_trans_info)[PIVOT_Y];

  /* move */
  if (function == TRANSFORM_HANDLE_CENTER)
    {
      if (constrain)
        {
          /* snap to 45 degree vectors from starting point */
          gdouble angle = calcangle ((GimpVector2){1., 0.}, (GimpVector2){dx, dy}) / (2.*G_PI);
          if (angle < 1./16 || angle > 15./16)
            dy = 0;
          else if (angle < 3./16)
            dy = -(dx = sqrt (dx*dx+dy*dy)/sqrt (2));
          else if (angle < 5./16)
            dx = 0;
          else if (angle < 7./16)
            dx = dy = -sqrt (dx*dx+dy*dy)/sqrt (2);
          else if (angle < 9./16)
            dy = 0;
          else if (angle < 11./16)
            dx = -(dy = sqrt (dx*dx+dy*dy)/sqrt (2));
          else if (angle < 13./16)
            dx = 0;
          else if (angle < 15./16)
            dx = dy = sqrt (dx*dx+dy*dy)/sqrt (2);
        }
      for (i = 0; i < 4; i++)
      {
        *x[i] = px[i] + dx;
        *y[i] = py[i] + dy;
      }
    }

  /* rotate */
  if (function == TRANSFORM_HANDLE_ROTATION)
    {
      GimpVector2 m = { .x = transform_tool->curx,   .y = transform_tool->cury };
      GimpVector2 p = { .x = transform_tool->mousex, .y = transform_tool->mousey };
      GimpVector2 c = { .x = ppivot_x,               .y = ppivot_y };
      gdouble angle = calcangle (vectorsubtract (m, c), vectorsubtract (p, c));
      if (constrain)
        {
          /* round to 15 degree multiple */
          angle /= 2*G_PI/24.;
          angle = round (angle);
          angle *= 2*G_PI/24.;
        }
      for (i = 0; i < 4; i++) {
        p.x = px[i]; p.y = py[i];
        m = vectoradd (c, rotate2d (vectorsubtract (p, c), angle));
        *x[i] = m.x;
        *y[i] = m.y;
      }
    }

  /* move rotation axis */
  if (function == TRANSFORM_HANDLE_PIVOT)
    {
      gint screenx, screeny;

      if (constrain)
        {
          /* snap to corner points and center */
          gint closest = 0;
          gdouble closest_dist = G_MAXDOUBLE, dist;
          for (i = 0; i < 5; i++)
            {
              dist = norm (vectorsubtract ((GimpVector2){transform_tool->curx, transform_tool->cury}, (GimpVector2){px[i], py[i]}));
              if (dist < closest_dist)
                {
                  closest_dist = dist;
                  closest = i;
                }
            }
          if (closest_dist * gimp_display_get_shell (GIMP_TOOL (transform_tool)->display)->scale_x < 50)
            {
              *pivot_x = px[closest];
              *pivot_y = py[closest];

              return;
            }
        }
      *pivot_x = ppivot_x + dx;
      *pivot_y = ppivot_y + dy;
    }

  if (function == TRANSFORM_HANDLE_NW ||
      function == TRANSFORM_HANDLE_NE ||
      function == TRANSFORM_HANDLE_SE ||
      function == TRANSFORM_HANDLE_SW)
    {
      //TODO: scale through corner
    }

  if (function == TRANSFORM_HANDLE_N ||
      function == TRANSFORM_HANDLE_E ||
      function == TRANSFORM_HANDLE_S ||
      function == TRANSFORM_HANDLE_W)
    {
      //TODO: scale through side
    }

  /* shear */
  if (function == TRANSFORM_HANDLE_N_S ||
      function == TRANSFORM_HANDLE_E_S ||
      function == TRANSFORM_HANDLE_S_S ||
      function == TRANSFORM_HANDLE_W_S)
    {
      gint left, right;

      if (function == TRANSFORM_HANDLE_N_S) {
        left = 1; right = 0;
      } else if (function == TRANSFORM_HANDLE_W_S) {
        left = 0; right = 2;
      } else if (function == TRANSFORM_HANDLE_S_S) {
        left = 2; right = 3;
      } else if (function == TRANSFORM_HANDLE_E_S) {
        left = 3; right = 1;
      } else g_assert_not_reached ();

      if (constrain)
        {
          /* restrict to movement along the side */
          GimpVector2 lp = { .x = px[left],  .y = py[left] },
                      rp = { .x = px[right], .y = py[right] },
                      p =  { .x = dx,        .y = dy },
                      side = vectorsubtract (rp, lp);

          p = vectorproject (p, side);

          dx = p.x;
          dy = p.y;
        }

      *x[left] = px[left] + dx;
      *y[left] = py[left] + dy;

      *x[right] = px[right] + dx;
      *y[right] = py[right] + dy;
    }

  /* perspective transform */
  if (function == TRANSFORM_HANDLE_NW_P ||
      function == TRANSFORM_HANDLE_NE_P ||
      function == TRANSFORM_HANDLE_SE_P ||
      function == TRANSFORM_HANDLE_SW_P)
    {
      gint this, left, right, opposite;

      /* 0: northwest, 1: northeast, 2: southwest, 3: southeast */
      if (function == TRANSFORM_HANDLE_NW_P) {
        this = 0; left = 1; right = 2; opposite = 3;
      } else if (function == TRANSFORM_HANDLE_NE_P) {
        this = 1; left = 3; right = 0; opposite = 2;
      } else if (function == TRANSFORM_HANDLE_SW_P) {
        this = 2; left = 0; right = 3; opposite = 1;
      } else if (function == TRANSFORM_HANDLE_SE_P) {
        this = 3; left = 2; right = 1; opposite = 0;
      } else g_assert_not_reached ();

      if (constrain)
        { /* when the constrain transformation constraint is enabled, the
             translation shall only be either along the side angles of the
             two sides that run to this corner point, or along the
             diagonal that runs trough this corner point. */

          GimpVector2 l = { .x = px[left],     .y = py[left] };
          GimpVector2 r = { .x = px[right],    .y = py[right] };
          GimpVector2 o = { .x = px[opposite], .y = py[opposite] };
          GimpVector2 t = { .x = px[this],     .y = py[this] };
          GimpVector2 p = { .x = dx,           .y = dy };
          GimpVector2 lp, rp, op;
          gdouble rej_lp, rej_rp, rej_op;

          /* get the vectors along the sides and the diagonal */
          l = vectorsubtract (t, l);
          r = vectorsubtract (t, r);
          o = vectorsubtract (t, o);

          /* project p on l, r and o and see which has the shortest rejection */
          lp = vectorproject (p, l);
          rp = vectorproject (p, r);
          op = vectorproject (p, o);

          rej_lp = norm (vectorsubtract (p, lp));
          rej_rp = norm (vectorsubtract (p, rp));
          rej_op = norm (vectorsubtract (p, op));

          if (rej_lp < rej_rp && rej_lp < rej_op)
            p = lp;
          else if (rej_rp < rej_op)
            p = rp;
          else
            p = op;

          dx = p.x;
          dy = p.y;
        }

      *x[this] = px[this] + dx;
      *y[this] = py[this] + dy;
    }

  /* old code below */
#if 0
  switch (function)
    {
    case TRANSFORM_HANDLE_E:
    case TRANSFORM_HANDLE_W:
      horizontal = TRUE;
    case TRANSFORM_HANDLE_N:
    case TRANSFORM_HANDLE_S:
      if (!constrain)
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
#endif
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
