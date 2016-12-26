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

#include "config/gimpguiconfig.h" /* playground */

#include "core/gimp.h" /* playground */

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpcanvasitem.h"
#include "display/gimpdisplay.h"
#include "display/gimptoolgui.h"

#include "gimphandletransformoptions.h"
#include "gimphandletransformtool.h"
#include "gimptoolcontrol.h"

#include "gimp-intl.h"


/* the transformation is defined by 8 points:
 *
 * 4 points on the original image and 4 corresponding points on the
 * transformed image. The first N_HANDLES points on the transformed
 * image are visible as handles.
 *
 * For these handles, the constants TRANSFORM_HANDLE_N,
 * TRANSFORM_HANDLE_S, TRANSFORM_HANDLE_E and TRANSFORM_HANDLE_W are
 * used. Actually, it makes no sense to name the handles with north,
 * south, east, and west.  But this way, we don't need to define even
 * more enum constants.
 */

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
  OX0,
  OY0,
  OX1,
  OY1,
  OX2,
  OY2,
  OX3,
  OY3,
  N_HANDLES
};


/*  local function prototypes  */

static void   gimp_handle_transform_tool_button_press  (GimpTool              *tool,
                                                        const GimpCoords      *coords,
                                                        guint32                time,
                                                        GdkModifierType        state,
                                                        GimpButtonPressType    press_type,
                                                        GimpDisplay           *display);
static void   gimp_handle_transform_tool_button_release (GimpTool              *tool,
                                                         const GimpCoords      *coords,
                                                         guint32                time,
                                                         GdkModifierType        state,
                                                         GimpButtonReleaseType    release_type,
                                                         GimpDisplay           *display);
static void   gimp_handle_transform_tool_modifier_key   (GimpTool              *tool,
                                                         GdkModifierType        key,
                                                         gboolean               press,
                                                         GdkModifierType        state,
                                                         GimpDisplay           *display);

static void   gimp_handle_transform_tool_dialog         (GimpTransformTool  *tr_tool);
static void   gimp_handle_transform_tool_dialog_update  (GimpTransformTool  *tr_tool);
static void   gimp_handle_transform_tool_prepare        (GimpTransformTool  *tr_tool);
static void   gimp_handle_transform_tool_motion         (GimpTransformTool  *tr_tool);
static void   gimp_handle_transform_tool_recalc_matrix  (GimpTransformTool  *tr_tool);
static gchar *gimp_handle_transform_tool_get_undo_desc  (GimpTransformTool  *tr_tool);
static TransformAction
              gimp_handle_transform_tool_pick_function  (GimpTransformTool  *tr_tool,
                                                         const GimpCoords   *coords,
                                                         GdkModifierType     state,
                                                         GimpDisplay        *display);
static void   gimp_handle_transform_tool_cursor_update  (GimpTransformTool  *tr_tool,
                                                         GimpCursorType     *cursor,
                                                         GimpCursorModifier *modifier);
static void   gimp_handle_transform_tool_draw_gui       (GimpTransformTool  *tr_tool,
                                                         gint                handle_w,
                                                         gint                handle_h);

static gboolean       is_handle_position_valid          (GimpTransformTool  *tr_tool,
                                                         gint                active_handle);
static void           handle_micro_move                 (GimpTransformTool *tr_tool,
                                                         gint               active_handle);
static inline gdouble calc_angle                        (gdouble  ax,
                                                         gdouble  ay,
                                                         gdouble  bx,
                                                         gdouble  by);
static inline gdouble calc_len                          (gdouble  a,
                                                         gdouble  b);
static inline gdouble calc_lineintersect_ratio          (gdouble  p1x,
                                                         gdouble  p1y,
                                                         gdouble  p2x,
                                                         gdouble  p2y,
                                                         gdouble  q1x,
                                                         gdouble  q1y,
                                                         gdouble  q2x,
                                                         gdouble  q2y);
static gboolean       mod_gauss                         (gdouble  matrix[],
                                                         gdouble  solution[],
                                                         gint     s);


G_DEFINE_TYPE (GimpHandleTransformTool, gimp_handle_transform_tool,
               GIMP_TYPE_TRANSFORM_TOOL)

#define parent_class gimp_handle_transform_tool_parent_class


void
gimp_handle_transform_tool_register (GimpToolRegisterCallback  callback,
                                     gpointer                  data)
{
  /* we should not know that "data" is a Gimp*, but what the heck this
   * is experimental playground stuff
   */
  if (GIMP_GUI_CONFIG (GIMP (data)->config)->playground_handle_transform_tool)
    (* callback) (GIMP_TYPE_HANDLE_TRANSFORM_TOOL,
                  GIMP_TYPE_HANDLE_TRANSFORM_OPTIONS,
                  gimp_handle_transform_options_gui,
                  GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                  "gimp-handle-transform-tool",
                  _("Handle Transform"),
                  _("Handle Transform Tool: "
                    "Deform the layer, selection or path with handles"),
                  N_("_Handle Transform"), "<ctrl><shift>H",
                  NULL, GIMP_HELP_TOOL_HANDLE_TRANSFORM,
                  GIMP_STOCK_TOOL_HANDLE_TRANSFORM,
                  data);
}

static void
gimp_handle_transform_tool_class_init (GimpHandleTransformToolClass *klass)
{
  GimpToolClass          *tool_class  = GIMP_TOOL_CLASS (klass);
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  tool_class->button_press   = gimp_handle_transform_tool_button_press;
  tool_class->button_release = gimp_handle_transform_tool_button_release;
  tool_class->modifier_key   = gimp_handle_transform_tool_modifier_key;

  trans_class->dialog        = gimp_handle_transform_tool_dialog;
  trans_class->dialog_update = gimp_handle_transform_tool_dialog_update;
  trans_class->prepare       = gimp_handle_transform_tool_prepare;
  trans_class->motion        = gimp_handle_transform_tool_motion;
  trans_class->recalc_matrix = gimp_handle_transform_tool_recalc_matrix;
  trans_class->get_undo_desc = gimp_handle_transform_tool_get_undo_desc;
  trans_class->pick_function = gimp_handle_transform_tool_pick_function;
  trans_class->cursor_update = gimp_handle_transform_tool_cursor_update;
  trans_class->draw_gui      = gimp_handle_transform_tool_draw_gui;
}

static void
gimp_handle_transform_tool_init (GimpHandleTransformTool *ht_tool)
{
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (ht_tool);

  tr_tool->progress_text    = _("Handle transformation");
  tr_tool->use_grid         = TRUE;

  tr_tool->does_perspective = TRUE;

  ht_tool->saved_handle_mode = GIMP_HANDLE_MODE_ADD_TRANSFORM;
}

static void
gimp_handle_transform_tool_button_press (GimpTool            *tool,
                                         const GimpCoords    *coords,
                                         guint32              time,
                                         GdkModifierType      state,
                                         GimpButtonPressType  press_type,
                                         GimpDisplay         *display)
{
  GimpHandleTransformTool    *ht      = GIMP_HANDLE_TRANSFORM_TOOL (tool);
  GimpTransformTool          *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpHandleTransformOptions *options;
  gint                        n_handles;
  gint                        active_handle;

  options = GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  GIMP_TOOL_CLASS (parent_class)->button_press (tool, coords, time, state,
                                                press_type, display);

  n_handles     = (gint) tr_tool->trans_info[N_HANDLES];
  active_handle = tr_tool->function - TRANSFORM_HANDLE_N;

  switch (options->handle_mode)
    {
    case GIMP_HANDLE_MODE_ADD_TRANSFORM:
      if (n_handles < 4 && tr_tool->function == TRANSFORM_HANDLE_NONE)
        {
          /* add handle */

          GimpMatrix3 matrix;

          gimp_draw_tool_pause (GIMP_DRAW_TOOL (tool));

          active_handle = n_handles;

          tr_tool->trans_info[X0 + 2 * active_handle] = coords->x;
          tr_tool->trans_info[Y0 + 2 * active_handle] = coords->y;
          tr_tool->trans_info[N_HANDLES]++;

          if (! is_handle_position_valid (tr_tool, active_handle))
            {
              handle_micro_move (tr_tool, active_handle);
            }

          /* handle was added, calculate new original position */
          matrix = tr_tool->transform;
          gimp_matrix3_invert (&matrix);
          gimp_matrix3_transform_point (&matrix,
                                        tr_tool->trans_info[X0 + 2 * active_handle],
                                        tr_tool->trans_info[Y0 + 2 * active_handle],
                                        &tr_tool->trans_info[OX0 + 2 * active_handle],
                                        &tr_tool->trans_info[OY0 + 2 * active_handle]);

          /*  this is disgusting: we put the new handle's coordinates
           *  into the prev_trans_info array, because our motion
           *  handler needs them for doing the actual transform; we
           *  can only do this because the values will be ignored by
           *  anything but our motion handler because we don't
           *  increase the N_HANDLES value in prev_trans_info.
           */
          (*tr_tool->prev_trans_info)[X0 + 2 * active_handle] = tr_tool->trans_info[X0 + 2 * active_handle];
          (*tr_tool->prev_trans_info)[Y0 + 2 * active_handle] = tr_tool->trans_info[Y0 + 2 * active_handle];

          (*tr_tool->prev_trans_info)[OX0 + 2 * active_handle] = tr_tool->trans_info[OX0 + 2 * active_handle];
          (*tr_tool->prev_trans_info)[OY0 + 2 * active_handle] = tr_tool->trans_info[OY0 + 2 * active_handle];

          gimp_draw_tool_resume (GIMP_DRAW_TOOL (tool));

          tr_tool->function = TRANSFORM_HANDLE_N + active_handle;
        }
      break;

    case GIMP_HANDLE_MODE_MOVE:
      /* check for valid position and calculating of OX0...OY3 is
       * done on button release
       */

      /* move handles without changing the transformation matrix */
      ht->matrix_recalculation = FALSE;
      break;

    case GIMP_HANDLE_MODE_REMOVE:
      if (n_handles > 0      &&
          active_handle >= 0 &&
          active_handle < 4)
        {
          /* remove handle */

          gdouble tempx  = tr_tool->trans_info[X0  + 2 * active_handle];
          gdouble tempy  = tr_tool->trans_info[Y0  + 2 * active_handle];
          gdouble tempox = tr_tool->trans_info[OX0 + 2 * active_handle];
          gdouble tempoy = tr_tool->trans_info[OY0 + 2 * active_handle];
          gint    i;

          n_handles--;
          tr_tool->trans_info[N_HANDLES]--;

          for (i = active_handle; i < n_handles; i++)
            {
              tr_tool->trans_info[X0  + 2 * i] = tr_tool->trans_info[X1  + 2 * i];
              tr_tool->trans_info[Y0  + 2 * i] = tr_tool->trans_info[Y1  + 2 * i];
              tr_tool->trans_info[OX0 + 2 * i] = tr_tool->trans_info[OX1 + 2 * i];
              tr_tool->trans_info[OY0 + 2 * i] = tr_tool->trans_info[OY1 + 2 * i];
            }

          tr_tool->trans_info[X0  + 2 * n_handles] = tempx;
          tr_tool->trans_info[Y0  + 2 * n_handles] = tempy;
          tr_tool->trans_info[OX0 + 2 * n_handles] = tempox;
          tr_tool->trans_info[OY0 + 2 * n_handles] = tempoy;
        }
      break;
    }
}

static void
gimp_handle_transform_tool_button_release (GimpTool              *tool,
                                           const GimpCoords      *coords,
                                           guint32                time,
                                           GdkModifierType        state,
                                           GimpButtonReleaseType  release_type,
                                           GimpDisplay           *display)
{
  GimpHandleTransformTool    *ht      = GIMP_HANDLE_TRANSFORM_TOOL (tool);
  GimpTransformTool          *tr_tool = GIMP_TRANSFORM_TOOL (tool);
  GimpHandleTransformOptions *options;
  gint                        active_handle;

  options = GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  active_handle = tr_tool->function - TRANSFORM_HANDLE_N;

  if (options->handle_mode == GIMP_HANDLE_MODE_MOVE &&
      active_handle >= 0 &&
      active_handle < 4)
    {
      GimpMatrix3 matrix;

      if (! is_handle_position_valid (tr_tool, active_handle))
        {
          handle_micro_move (tr_tool, active_handle);
        }

      /* handle was moved, calculate new original position */
      matrix = tr_tool->transform;
      gimp_matrix3_invert (&matrix);
      gimp_matrix3_transform_point (&matrix,
                                    tr_tool->trans_info[X0 + 2 * active_handle],
                                    tr_tool->trans_info[Y0 + 2 * active_handle],
                                    &tr_tool->trans_info[OX0 + 2 * active_handle],
                                    &tr_tool->trans_info[OY0 + 2 * active_handle]);
    }

  ht->matrix_recalculation = TRUE;

  GIMP_TOOL_CLASS (parent_class)->button_release (tool, coords, time,
                                                  state, release_type, display);
}

static void
gimp_handle_transform_tool_modifier_key (GimpTool        *tool,
                                         GdkModifierType  key,
                                         gboolean         press,
                                         GdkModifierType  state,
                                         GimpDisplay     *display)
{
  GimpHandleTransformTool    *ht_tool = GIMP_HANDLE_TRANSFORM_TOOL (tool);
  GimpHandleTransformOptions *options;
  GdkModifierType             shift   = gimp_get_extend_selection_mask ();
  GdkModifierType             ctrl    = gimp_get_constrain_behavior_mask ();
  GimpTransformHandleMode     handle_mode;

  options = GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tool);

  handle_mode = options->handle_mode;

  if (press)
    {
      if (key == (state & (shift | ctrl)))
        {
          /*  first modifier pressed  */
          ht_tool->saved_handle_mode = options->handle_mode;
        }
    }
  else
    {
      if (! (state & (shift | ctrl)))
        {
          /*  last modifier released  */
          handle_mode = ht_tool->saved_handle_mode;
        }
    }

  if (state & shift)
    {
      handle_mode = GIMP_HANDLE_MODE_MOVE;
    }
  else if (state & ctrl)
    {
      handle_mode = GIMP_HANDLE_MODE_REMOVE;
    }

  if (handle_mode != options->handle_mode)
    {
      g_object_set (options, "handle-mode", handle_mode, NULL);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                state, display);
}

static void
gimp_handle_transform_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpHandleTransformTool *ht_tool = GIMP_HANDLE_TRANSFORM_TOOL (tr_tool);
  GtkWidget               *frame;
  GtkWidget               *table;
  gint                     x, y;

  frame = gimp_frame_new (_("Transformation Matrix"));
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tr_tool->gui)), frame,
                      FALSE, FALSE, 0);
  gtk_widget_show (frame);

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_container_add (GTK_CONTAINER (frame), table);
  gtk_widget_show (table);

  for (y = 0; y < 3; y++)
    {
      for (x = 0; x < 3; x++)
        {
          GtkWidget *label = gtk_label_new (" ");

          gtk_label_set_xalign (GTK_LABEL (label), 1.0);
          gtk_label_set_width_chars (GTK_LABEL (label), 12);
          gtk_table_attach (GTK_TABLE (table), label,
                            x, x + 1, y, y + 1, GTK_EXPAND, GTK_FILL, 0, 0);
          gtk_widget_show (label);

          ht_tool->label[y][x] = label;
        }
    }
}

static void
gimp_handle_transform_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpHandleTransformTool *ht_tool = GIMP_HANDLE_TRANSFORM_TOOL (tr_tool);
  gint                     x, y;

  for (y = 0; y < 3; y++)
    {
      for (x = 0; x < 3; x++)
        {
          gchar buf[32];

          g_snprintf (buf, sizeof (buf),
                      "%10.5f", tr_tool->transform.coeff[y][x]);

          gtk_label_set_text (GTK_LABEL (ht_tool->label[y][x]), buf);
        }
    }
}

static void
gimp_handle_transform_tool_prepare (GimpTransformTool  *tr_tool)
{
  GimpHandleTransformTool *ht_tool = GIMP_HANDLE_TRANSFORM_TOOL (tr_tool);

  tr_tool->trans_info[X0]        = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y0]        = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X1]        = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y1]        = (gdouble) tr_tool->y1;
  tr_tool->trans_info[X2]        = (gdouble) tr_tool->x1;
  tr_tool->trans_info[Y2]        = (gdouble) tr_tool->y2;
  tr_tool->trans_info[X3]        = (gdouble) tr_tool->x2;
  tr_tool->trans_info[Y3]        = (gdouble) tr_tool->y2;
  tr_tool->trans_info[OX0]       = (gdouble) tr_tool->x1;
  tr_tool->trans_info[OY0]       = (gdouble) tr_tool->y1;
  tr_tool->trans_info[OX1]       = (gdouble) tr_tool->x2;
  tr_tool->trans_info[OY1]       = (gdouble) tr_tool->y1;
  tr_tool->trans_info[OX2]       = (gdouble) tr_tool->x1;
  tr_tool->trans_info[OY2]       = (gdouble) tr_tool->y2;
  tr_tool->trans_info[OX3]       = (gdouble) tr_tool->x2;
  tr_tool->trans_info[OY3]       = (gdouble) tr_tool->y2;
  tr_tool->trans_info[N_HANDLES] = 0;

  ht_tool->matrix_recalculation = TRUE;
}

static void
gimp_handle_transform_tool_motion (GimpTransformTool *tr_tool)
{
  GimpHandleTransformOptions *options;
  gint                        n_handles;
  gint                        active_handle;

  options = GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  n_handles     = (gint) tr_tool->trans_info[N_HANDLES];
  active_handle = tr_tool->function - TRANSFORM_HANDLE_N;

  if (active_handle >= 0 && active_handle < 4)
    {
      if (options->handle_mode == GIMP_HANDLE_MODE_MOVE)
        {
          tr_tool->trans_info[X0 + 2 * active_handle] += tr_tool->curx - tr_tool->lastx;
          tr_tool->trans_info[Y0 + 2 * active_handle] += tr_tool->cury - tr_tool->lasty;

          /* check for valid position and calculating of OX0...OY3 is
           * done on button release hopefully this makes the code run
           * faster Moving could be even faster if there was caching
           * for the image preview
           */
        }
      else if (options->handle_mode == GIMP_HANDLE_MODE_ADD_TRANSFORM)
        {
          gdouble angle, angle_sin, angle_cos, scale;
          gdouble fixed_handles_x[3];
          gdouble fixed_handles_y[3];
          gdouble oldpos_x[4], oldpos_y[4];
          gdouble newpos_x[4], newpos_y[4];
          gint    i, j;

          for (i = 0, j = 0; i < 4; i++)
            {
              /* Find all visible handles that are not being moved */
              if (i < n_handles && i != active_handle)
                {
                  fixed_handles_x[j] = tr_tool->trans_info[X0 + i * 2];
                  fixed_handles_y[j] = tr_tool->trans_info[Y0 + i * 2];
                  j++;
                }

              newpos_x[i] = oldpos_x[i] = (*tr_tool->prev_trans_info)[X0 + i * 2];
              newpos_y[i] = oldpos_y[i] = (*tr_tool->prev_trans_info)[Y0 + i * 2];
            }

          newpos_x[active_handle] = oldpos_x[active_handle] + tr_tool->curx - tr_tool->mousex;
          newpos_y[active_handle] = oldpos_y[active_handle] + tr_tool->cury - tr_tool->mousey;

          switch (n_handles)
            {
            case 1:
              /* move */
              for (i = 1; i < 4; i++)
                {
                  newpos_x[i] = oldpos_x[i] + tr_tool->curx - tr_tool->mousex;
                  newpos_y[i] = oldpos_y[i] + tr_tool->cury - tr_tool->mousey;
                }
              break;

            case 2:
              /* rotate and keep-aspect-scale */
              scale = calc_len (newpos_x[active_handle] - fixed_handles_x[0],
                                newpos_y[active_handle] - fixed_handles_y[0])
                / calc_len (oldpos_x[active_handle] - fixed_handles_x[0],
                            oldpos_y[active_handle] - fixed_handles_y[0]);

              angle = calc_angle (oldpos_x[active_handle] - fixed_handles_x[0],
                                  oldpos_y[active_handle] - fixed_handles_y[0],
                                  newpos_x[active_handle] - fixed_handles_x[0],
                                  newpos_y[active_handle] - fixed_handles_y[0]);

              angle_sin = sin (angle);
              angle_cos = cos (angle);

              for (i = 2; i < 4; i++)
                {
                  newpos_x[i] = fixed_handles_x[0]
                    + scale * (angle_cos * (oldpos_x[i]-fixed_handles_x[0])
                               + angle_sin * (oldpos_y[i]-fixed_handles_y[0]) );
                  newpos_y[i] = fixed_handles_y[0]
                    + scale * (-angle_sin * (oldpos_x[i]-fixed_handles_x[0])
                               + angle_cos * (oldpos_y[i]-fixed_handles_y[0]) );
                }
              break;

            case 3:
              /* shear and non-aspect-scale */
              scale = calc_lineintersect_ratio (oldpos_x[3], oldpos_y[3],
                                                oldpos_x[active_handle], oldpos_y[active_handle],
                                                fixed_handles_x[0], fixed_handles_y[0],
                                                fixed_handles_x[1], fixed_handles_y[1]);

              newpos_x[3] = oldpos_x[3] + scale * (tr_tool->curx - tr_tool->mousex);
              newpos_y[3] = oldpos_y[3] + scale * (tr_tool->cury - tr_tool->mousey);
              break;
            }

          for (i = 0; i < 4; i++)
            {
              tr_tool->trans_info[X0 + 2 * i] = newpos_x[i];
              tr_tool->trans_info[Y0 + 2 * i] = newpos_y[i];
            }
        }
    }
}

static void
gimp_handle_transform_tool_recalc_matrix (GimpTransformTool *tr_tool)
{
  GimpHandleTransformTool *ht_tool = GIMP_HANDLE_TRANSFORM_TOOL (tr_tool);
  gdouble                  coeff[8 * 9];
  gdouble                  sol[8];
  gdouble                  opos_x[4];
  gdouble                  opos_y[4];
  gdouble                  pos_x[4];
  gdouble                  pos_y[4];
  gint                     i;

  if (ht_tool->matrix_recalculation)
    {
      for (i = 0; i < 4; i++)
        {
          pos_x[i]  = tr_tool->trans_info[X0 + i * 2];
          pos_y[i]  = tr_tool->trans_info[Y0 + i * 2];
          opos_x[i] = tr_tool->trans_info[OX0 + i * 2];
          opos_y[i] = tr_tool->trans_info[OY0 + i * 2];
        }

      for (i = 0; i < 4; i++)
        {
          coeff[i * 9 + 0] = opos_x[i];
          coeff[i * 9 + 1] = opos_y[i];
          coeff[i * 9 + 2] = 1;
          coeff[i * 9 + 3] = 0;
          coeff[i * 9 + 4] = 0;
          coeff[i * 9 + 5] = 0;
          coeff[i * 9 + 6] = -opos_x[i] * pos_x[i];
          coeff[i * 9 + 7] = -opos_y[i] * pos_x[i];
          coeff[i * 9 + 8] = pos_x[i];

          coeff[(i + 4) * 9 + 0] = 0;
          coeff[(i + 4) * 9 + 1] = 0;
          coeff[(i + 4) * 9 + 2] = 0;
          coeff[(i + 4) * 9 + 3] = opos_x[i];
          coeff[(i + 4) * 9 + 4] = opos_y[i];
          coeff[(i + 4) * 9 + 5] = 1;
          coeff[(i + 4) * 9 + 6] = -opos_x[i] * pos_y[i];
          coeff[(i + 4) * 9 + 7] = -opos_y[i] * pos_y[i];
          coeff[(i + 4) * 9 + 8] = pos_y[i];
        }

      if (mod_gauss (coeff, sol, 8))
        {
          tr_tool->transform.coeff[0][0] = sol[0];
          tr_tool->transform.coeff[0][1] = sol[1];
          tr_tool->transform.coeff[0][2] = sol[2];
          tr_tool->transform.coeff[1][0] = sol[3];
          tr_tool->transform.coeff[1][1] = sol[4];
          tr_tool->transform.coeff[1][2] = sol[5];
          tr_tool->transform.coeff[2][0] = sol[6];
          tr_tool->transform.coeff[2][1] = sol[7];
          tr_tool->transform.coeff[2][2] = 1;
        }
      else
        {
          /* this should not happen reset the matrix so the user sees
           * that something went wrong
           */
          gimp_matrix3_identity (&tr_tool->transform);
        }
    }
}

static gchar *
gimp_handle_transform_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (C_("undo-type", "Handle transform"));
}

static TransformAction
gimp_handle_transform_tool_pick_function (GimpTransformTool *tr_tool,
                                          const GimpCoords  *coords,
                                          GdkModifierType    state,
                                          GimpDisplay       *display)
{
  TransformAction i;

  for (i = TRANSFORM_HANDLE_N; i < TRANSFORM_HANDLE_N + 4; i++)
    {
      if (tr_tool->handles[i] &&
          gimp_canvas_item_hit (tr_tool->handles[i], coords->x, coords->y))
        {
          return i;
        }
    }

  return TRANSFORM_HANDLE_NONE;
}

static void
gimp_handle_transform_tool_cursor_update (GimpTransformTool  *tr_tool,
                                          GimpCursorType     *cursor,
                                          GimpCursorModifier *modifier)
{
  GimpHandleTransformOptions *options;
  GimpToolCursorType          tool_cursor = GIMP_TOOL_CURSOR_NONE;

  options = GIMP_HANDLE_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);

  *cursor   = GIMP_CURSOR_CROSSHAIR_SMALL;
  *modifier = GIMP_CURSOR_MODIFIER_NONE;

  /* do not show modifiers when the tool isn't active */
  if (! gimp_draw_tool_is_active (GIMP_DRAW_TOOL (tr_tool)))
    return;

  switch (options->handle_mode)
    {
    case GIMP_HANDLE_MODE_ADD_TRANSFORM:
      if (tr_tool->function > TRANSFORM_HANDLE_NONE)
        {
          switch ((gint) tr_tool->trans_info[N_HANDLES])
            {
            case 1:
              tool_cursor = GIMP_TOOL_CURSOR_MOVE;
              break;
            case 2:
              tool_cursor = GIMP_TOOL_CURSOR_ROTATE;
              break;
            case 3:
              tool_cursor = GIMP_TOOL_CURSOR_SHEAR;
              break;
            case 4:
              tool_cursor = GIMP_TOOL_CURSOR_PERSPECTIVE;
              break;
            }
        }
      else
        {
          if ((gint) tr_tool->trans_info[N_HANDLES] < 4)
            *modifier = GIMP_CURSOR_MODIFIER_PLUS;
          else
            *modifier = GIMP_CURSOR_MODIFIER_BAD;
        }
      break;

    case GIMP_HANDLE_MODE_MOVE:
      if (tr_tool->function > TRANSFORM_HANDLE_NONE)
        *modifier = GIMP_CURSOR_MODIFIER_MOVE;
      else
        *modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;

    case GIMP_HANDLE_MODE_REMOVE:
      if (tr_tool->function > TRANSFORM_HANDLE_NONE)
        *modifier = GIMP_CURSOR_MODIFIER_MINUS;
      else
        *modifier = GIMP_CURSOR_MODIFIER_BAD;
      break;
    }

  gimp_tool_control_set_tool_cursor (GIMP_TOOL (tr_tool)->control,
                                     tool_cursor);
}

static void
gimp_handle_transform_tool_draw_gui (GimpTransformTool *tr_tool,
                                     gint               handle_w,
                                     gint               handle_h)
{
  GimpDrawTool *draw_tool = GIMP_DRAW_TOOL (tr_tool);
  gint          i;

#if 0
  /* show additional points for debugging */
  for (i = tr_tool->trans_info[N_HANDLES]; i < 4; i++)
    {
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_FILLED_CIRCLE,
                                 tr_tool->trans_info[X0 + 2 * i],
                                 tr_tool->trans_info[Y0 + 2 * i],
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);
      gimp_draw_tool_add_handle (draw_tool,
                                 GIMP_HANDLE_FILLED_DIAMOND,
                                 tr_tool->trans_info[OX0 + 2 * i],
                                 tr_tool->trans_info[OY0 + 2 * i],
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                 GIMP_HANDLE_ANCHOR_CENTER);
      }

  for (i = 0; i < tr_tool->trans_info[N_HANDLES]; i++)
    {
      tr_tool->handles[TRANSFORM_HANDLE_N + i] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_DIAMOND,
                                   tr_tool->trans_info[OX0 + 2 * i],
                                   tr_tool->trans_info[OY0 + 2 * i],
                                   GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                   GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                   GIMP_HANDLE_ANCHOR_CENTER);
    }
#endif

  for (i = 0; i < tr_tool->trans_info[N_HANDLES]; i++)
    {
      tr_tool->handles[TRANSFORM_HANDLE_N + i] =
        gimp_draw_tool_add_handle (draw_tool,
                                   GIMP_HANDLE_CIRCLE,
                                   tr_tool->trans_info[X0 + 2 * i],
                                   tr_tool->trans_info[Y0 + 2 * i],
                                   GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                   GIMP_TOOL_HANDLE_SIZE_CIRCLE,
                                   GIMP_HANDLE_ANCHOR_CENTER);
    }
}

/* check if a handle is not on the connection line of two other handles */
static gboolean
is_handle_position_valid (GimpTransformTool *tr_tool,
                          gint               active_handle)
{
  gint i, j, k;

  for (i = 0; i < 2; i++)
    {
      for (j = i + 1; j < 3; j++)
        {
          for (k = j + 1; i < 4; i++)
            {
              if (active_handle == i ||
                  active_handle == j ||
                  active_handle == k)
                {
                  if ((tr_tool->trans_info[X0 + 2 * i] -
                       tr_tool->trans_info[X0 + 2 * j]) *
                      (tr_tool->trans_info[Y0 + 2 * j] -
                       tr_tool->trans_info[Y0 + 2 * k]) ==

                      (tr_tool->trans_info[X0 + 2 * j] -
                       tr_tool->trans_info[X0 + 2 * k]) *
                      (tr_tool->trans_info[Y0 + 2 * i] -
                       tr_tool->trans_info[Y0 + 2 * j]))
                    {
                      return FALSE;
                    }
                }
            }
        }
    }

  return TRUE;
}

/* three handles on a line causes problems.
 * Let's move the new handle around a bit to find a better position */
static void
handle_micro_move (GimpTransformTool *tr_tool,
                   gint               active_handle)
{
  gdouble posx = tr_tool->trans_info[X0 + 2 * active_handle];
  gdouble posy = tr_tool->trans_info[Y0 + 2 * active_handle];
  gdouble dx, dy;

  for (dx = -0.1; dx < 0.11; dx += 0.1)
    {
      tr_tool->trans_info[X0 + 2 * active_handle] = posx + dx;

      for (dy = -0.1; dy < 0.11; dy += 0.1)
        {
          tr_tool->trans_info[Y0 + 2 * active_handle] = posy + dy;

          if (is_handle_position_valid (tr_tool, active_handle))
            {
              return;
            }
        }
    }
}

/* finds the clockwise angle between the vectors given, 0-2π */
static inline gdouble
calc_angle (gdouble ax,
            gdouble ay,
            gdouble bx,
            gdouble by)
{
  gdouble angle;
  gdouble direction;
  gdouble length = sqrt ((ax * ax + ay * ay) * (bx * bx + by * by));

  angle = acos ((ax * bx + ay * by) / length);
  direction = ax * by - ay * bx;

  return ((direction < 0) ? angle : 2 * G_PI - angle);
}

static inline gdouble
calc_len  (gdouble a,
           gdouble b)
{
  return sqrt (a * a + b * b);
}


/* imagine two lines, one through the points p1 and p2, the other one
 * through the points q1 and q2. Find the intersection point r.
 * Calculate (distance p1 to r)/(distance p2 to r)
 */
static inline gdouble
calc_lineintersect_ratio (gdouble p1x, gdouble p1y,
                          gdouble p2x, gdouble p2y,
                          gdouble q1x, gdouble q1y,
                          gdouble q2x, gdouble q2y)
{
  gdouble denom, u;

  denom = (q2y - q1y) * (p2x - p1x) - (q2x - q1x) * (p2y - p1y);
  if (denom == 0.0)
    {
      /* u is infinite, so u/(u-1) is 1 */
      return 1.0;
    }

  u = (q2y - q1y) * (q1x - p1x) - (q1y - p1y) * (q2x - q1x);
  u /= denom;

  return u / (u - 1);
}


/* modified gaussian algorithm
 * solves a system of linear equations
 *
 * Example:
 * 1x + 2y + 4z = 25
 * 2x + 1y      = 4
 * 3x + 5y + 2z = 23
 * Solution: x=1, y=2, z=5
 *
 * Input:
 * matrix = { 1,2,4,25,2,1,0,4,3,5,2,23 }
 * s = 3 (Number of variables)
 * Output:
 * return value == TRUE (TRUE, if there is a single unique solution)
 * solution == { 1,2,5 } (if the return value is FALSE, the content
 * of solution is of no use)
 */
static gboolean
mod_gauss (gdouble matrix[],
           gdouble solution[],
           gint    s)
{
  gint    p[s]; /* row permutation */
  gint    i, j, r, temp;
  gdouble q;
  gint    t = s + 1;

  for (i = 0; i < s; i++)
    {
      p[i] = i;
    }

  for (r = 0; r < s; r++)
    {
      /* make sure that (r,r) is not 0 */
      if (matrix[p[r] * t + r] == 0.0)
        {
          /* we need to permutate rows */
          for (i = r + 1; i <= s; i++)
            {
              if (i == s)
                {
                  /* if this happens, the linear system has zero or
                   * more than one solutions.
                   */
                  return FALSE;
                }

              if (matrix[p[i] * t + r] != 0.0)
                break;
            }

          temp = p[r];
          p[r] = p[i];
          p[i] = temp;
        }

      /* make (r,r) == 1 */
      q = 1.0 / matrix[p[r] * t + r];
      matrix[p[r] * t + r] = 1.0;

      for (j = r + 1; j < t; j++)
        {
          matrix[p[r] * t + j] *= q;
        }

      /* make that all entries in column r are 0 (except (r,r)) */
      for (i = 0; i < s; i++)
        {
          if (i == r)
            continue;

          for (j = r + 1; j < t ; j++)
            {
              matrix[p[i] * t + j] -= matrix[p[r] * t + j] * matrix[p[i] * t + r];
            }

          /* we don't need to execute the following line
           * since we won't access this element again:
           *
           * matrix[p[i] * t + r] = 0.0;
           */
        }
    }

  for (i = 0; i < s; i++)
    {
      solution[i] = matrix[p[i] * t + s];
    }

  return TRUE;
}
