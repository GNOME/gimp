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

#include <stdlib.h>

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-utils.h"
#include "core/gimpimage.h"
#include "core/gimpdrawable-transform.h"

#include "widgets/gimphelp-ids.h"

#include "display/gimpdisplay.h"

#include "gimpsheartool.h"
#include "gimptoolcontrol.h"
#include "gimptransformoptions.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
#define HORZ_OR_VERT 0
#define XSHEAR       1
#define YSHEAR       2

/*  the minimum movement before direction of shear can be determined (pixels) */
#define MIN_MOVE     5

#define SB_WIDTH     10


/*  local function prototypes  */

static void   gimp_shear_tool_dialog        (GimpTransformTool  *tr_tool);
static void   gimp_shear_tool_dialog_update (GimpTransformTool  *tr_tool);

static void   gimp_shear_tool_prepare       (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);
static void   gimp_shear_tool_motion        (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);
static void   gimp_shear_tool_recalc        (GimpTransformTool  *tr_tool,
                                             GimpDisplay        *display);

static void   shear_x_mag_changed           (GtkAdjustment      *adj,
                                             GimpTransformTool  *tr_tool);
static void   shear_y_mag_changed           (GtkAdjustment      *adj,
                                             GimpTransformTool  *tr_tool);


G_DEFINE_TYPE (GimpShearTool, gimp_shear_tool, GIMP_TYPE_TRANSFORM_TOOL)


void
gimp_shear_tool_register (GimpToolRegisterCallback  callback,
                          gpointer                  data)
{
  (* callback) (GIMP_TYPE_SHEAR_TOOL,
                GIMP_TYPE_TRANSFORM_OPTIONS,
                gimp_transform_options_gui,
                0,
                "gimp-shear-tool",
                _("Shear"),
                _("Shear the layer or selection"),
                N_("S_hear"), "<shift>S",
                NULL, GIMP_HELP_TOOL_SHEAR,
                GIMP_STOCK_TOOL_SHEAR,
                data);
}

static void
gimp_shear_tool_class_init (GimpShearToolClass *klass)
{
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  trans_class->dialog        = gimp_shear_tool_dialog;
  trans_class->dialog_update = gimp_shear_tool_dialog_update;
  trans_class->prepare       = gimp_shear_tool_prepare;
  trans_class->motion        = gimp_shear_tool_motion;
  trans_class->recalc        = gimp_shear_tool_recalc;
}

static void
gimp_shear_tool_init (GimpShearTool *shear_tool)
{
  GimpTool          *tool    = GIMP_TOOL (shear_tool);
  GimpTransformTool *tr_tool = GIMP_TRANSFORM_TOOL (shear_tool);

  gimp_tool_control_set_tool_cursor (tool->control, GIMP_TOOL_CURSOR_SHEAR);

  tr_tool->undo_desc     = Q_("command|Shear");
  tr_tool->shell_desc    = _("Shearing Information");
  tr_tool->progress_text = _("Shearing");

  tr_tool->use_grid      = TRUE;
}

static void
gimp_shear_tool_dialog (GimpTransformTool *tr_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tr_tool);
  GtkWidget     *table;
  GtkWidget     *button;

  table = gtk_table_new (2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (tr_tool->dialog)->vbox), table,
                      FALSE, FALSE, 0);
  gtk_widget_show (table);

  button = gimp_spin_button_new (&shear->x_adj,
                                 0, -65536, 65536, 1, 15, 1, 1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0, _("Shear magnitude X:"),
                             0.0, 0.5, button, 1, TRUE);

  g_signal_connect (shear->x_adj, "value-changed",
                    G_CALLBACK (shear_x_mag_changed),
                    tr_tool);

  button = gimp_spin_button_new (&shear->y_adj,
                                 0, -65536, 65536, 1, 15, 1, 1, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (button), SB_WIDTH);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1, _("Shear magnitude Y:"),
                             0.0, 0.5, button, 1, TRUE);

  g_signal_connect (shear->y_adj, "value-changed",
                    G_CALLBACK (shear_y_mag_changed),
                    tr_tool);
}

static void
gimp_shear_tool_dialog_update (GimpTransformTool *tr_tool)
{
  GimpShearTool *shear = GIMP_SHEAR_TOOL (tr_tool);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (shear->x_adj),
                            tr_tool->trans_info[XSHEAR]);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (shear->y_adj),
                            tr_tool->trans_info[YSHEAR]);
}

static void
gimp_shear_tool_prepare (GimpTransformTool *tr_tool,
                         GimpDisplay       *display)
{
  tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_UNKNOWN;
  tr_tool->trans_info[XSHEAR]       = 0.0;
  tr_tool->trans_info[YSHEAR]       = 0.0;
}

static void
gimp_shear_tool_motion (GimpTransformTool *tr_tool,
                        GimpDisplay       *display)
{
  gdouble diffx = tr_tool->curx - tr_tool->lastx;
  gdouble diffy = tr_tool->cury - tr_tool->lasty;

  /*  If we haven't yet decided on which way to control shearing
   *  decide using the maximum differential
   */

  if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_UNKNOWN)
    {
      if (abs (diffx) > MIN_MOVE || abs (diffy) > MIN_MOVE)
        {
          if (abs (diffx) > abs (diffy))
            {
              tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_HORIZONTAL;
              tr_tool->trans_info[XSHEAR] = 0.0;
            }
          else
            {
              tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_VERTICAL;
              tr_tool->trans_info[XSHEAR] = 0.0;
            }
        }
      /*  set the current coords to the last ones  */
      else
        {
          tr_tool->curx = tr_tool->lastx;
          tr_tool->cury = tr_tool->lasty;
        }
    }

  /*  if the direction is known, keep track of the magnitude  */
  if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_HORIZONTAL)
    {
      if (tr_tool->cury > (tr_tool->ty1 + tr_tool->ty3) / 2)
        tr_tool->trans_info[XSHEAR] += diffx;
      else
        tr_tool->trans_info[XSHEAR] -= diffx;
    }
  else if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_VERTICAL)
    {
      if (tr_tool->curx > (tr_tool->tx1 + tr_tool->tx2) / 2)
        tr_tool->trans_info[YSHEAR] += diffy;
      else
        tr_tool->trans_info[YSHEAR] -= diffy;
    }
}

static void
gimp_shear_tool_recalc (GimpTransformTool *tr_tool,
                        GimpDisplay       *display)
{
  gdouble amount;

  if (tr_tool->trans_info[XSHEAR] == 0.0 &&
      tr_tool->trans_info[YSHEAR] == 0.0)
    {
      tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_UNKNOWN;
    }

  if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_HORIZONTAL)
    amount = tr_tool->trans_info[XSHEAR];
  else
    amount = tr_tool->trans_info[YSHEAR];

  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_shear (&tr_tool->transform,
                               tr_tool->x1,
                               tr_tool->y1,
                               tr_tool->x2 - tr_tool->x1,
                               tr_tool->y2 - tr_tool->y1,
                               tr_tool->trans_info[HORZ_OR_VERT],
                               amount);
}

static void
shear_x_mag_changed (GtkAdjustment     *adj,
                     GimpTransformTool *tr_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tr_tool->trans_info[XSHEAR])
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_UNKNOWN)
        tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_HORIZONTAL;

      tr_tool->trans_info[XSHEAR] = value;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->display);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}

static void
shear_y_mag_changed (GtkAdjustment     *adj,
                     GimpTransformTool *tr_tool)
{
  gdouble value = gtk_adjustment_get_value (adj);

  if (value != tr_tool->trans_info[YSHEAR])
    {
      gimp_draw_tool_pause (GIMP_DRAW_TOOL (tr_tool));

      if (tr_tool->trans_info[HORZ_OR_VERT] == GIMP_ORIENTATION_UNKNOWN)
        tr_tool->trans_info[HORZ_OR_VERT] = GIMP_ORIENTATION_VERTICAL;

      tr_tool->trans_info[YSHEAR] = value;

      gimp_transform_tool_recalc (tr_tool, GIMP_TOOL (tr_tool)->display);

      gimp_transform_tool_expose_preview (tr_tool);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (tr_tool));
    }
}
