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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpguiconfig.h" /* playground */

#include "core/gimp.h" /* playground */
#include "core/gimp-transform-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimptoolhandlegrid.h"
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

static void   gimp_handle_transform_tool_modifier_key   (GimpTool          *tool,
                                                         GdkModifierType    key,
                                                         gboolean           press,
                                                         GdkModifierType    state,
                                                         GimpDisplay       *display);
static void   gimp_handle_transform_tool_options_notify (GimpTool          *tool,
                                                         GimpToolOptions   *options,
                                                         const GParamSpec  *pspec);

static void   gimp_handle_transform_tool_dialog         (GimpTransformTool *tr_tool);
static void   gimp_handle_transform_tool_dialog_update  (GimpTransformTool *tr_tool);
static void   gimp_handle_transform_tool_prepare        (GimpTransformTool *tr_tool);
static GimpToolWidget *
              gimp_handle_transform_tool_get_widget     (GimpTransformTool *tr_tool);
static void   gimp_handle_transform_tool_recalc_matrix  (GimpTransformTool *tr_tool,
                                                         GimpToolWidget    *widget);
static gchar *gimp_handle_transform_tool_get_undo_desc  (GimpTransformTool *tr_tool);

static void   gimp_handle_transform_tool_widget_changed (GimpToolWidget    *widget,
                                                         GimpTransformTool *tr_tool);


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
                  GIMP_ICON_TOOL_HANDLE_TRANSFORM,
                  data);
}

static void
gimp_handle_transform_tool_class_init (GimpHandleTransformToolClass *klass)
{
  GimpToolClass          *tool_class  = GIMP_TOOL_CLASS (klass);
  GimpTransformToolClass *trans_class = GIMP_TRANSFORM_TOOL_CLASS (klass);

  tool_class->modifier_key   = gimp_handle_transform_tool_modifier_key;
  tool_class->options_notify = gimp_handle_transform_tool_options_notify;

  trans_class->dialog        = gimp_handle_transform_tool_dialog;
  trans_class->dialog_update = gimp_handle_transform_tool_dialog_update;
  trans_class->prepare       = gimp_handle_transform_tool_prepare;
  trans_class->get_widget    = gimp_handle_transform_tool_get_widget;
  trans_class->recalc_matrix = gimp_handle_transform_tool_recalc_matrix;
  trans_class->get_undo_desc = gimp_handle_transform_tool_get_undo_desc;
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
      g_object_set (options,
                    "handle-mode", handle_mode,
                    NULL);
    }

  GIMP_TOOL_CLASS (parent_class)->modifier_key (tool, key, press,
                                                state, display);
}

static void
gimp_handle_transform_tool_options_notify (GimpTool         *tool,
                                           GimpToolOptions  *options,
                                           const GParamSpec *pspec)
{
  GimpTransformTool          *tr_tool    = GIMP_TRANSFORM_TOOL (tool);
  GimpHandleTransformOptions *ht_options = GIMP_HANDLE_TRANSFORM_OPTIONS (options);

  if (! strcmp (pspec->name, "handle-mode"))
    {
      if (tr_tool->widget)
        g_object_set (tr_tool->widget,
                      "handle-mode", ht_options->handle_mode,
                      NULL);
    }
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
gimp_handle_transform_tool_prepare (GimpTransformTool *tr_tool)
{
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
}

static GimpToolWidget *
gimp_handle_transform_tool_get_widget (GimpTransformTool *tr_tool)
{
  GimpTool             *tool    = GIMP_TOOL (tr_tool);
  GimpTransformOptions *options = GIMP_TRANSFORM_TOOL_GET_OPTIONS (tr_tool);
  GimpDisplayShell     *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget       *widget;

  widget = gimp_tool_handle_grid_new (shell,
                                      tr_tool->x1,
                                      tr_tool->y1,
                                      tr_tool->x2,
                                      tr_tool->y2);

  g_object_set (widget,
                "n-handles", (gint) tr_tool->trans_info[N_HANDLES],
                "orig-x1",   tr_tool->trans_info[OX0],
                "orig-y1",   tr_tool->trans_info[OY0],
                "orig-x2",   tr_tool->trans_info[OX1],
                "orig-y2",   tr_tool->trans_info[OY1],
                "orig-x3",   tr_tool->trans_info[OX2],
                "orig-y3",   tr_tool->trans_info[OY2],
                "orig-x4",   tr_tool->trans_info[OX3],
                "orig-y4",   tr_tool->trans_info[OY3],
                "trans-x1",  tr_tool->trans_info[X0],
                "trans-y1",  tr_tool->trans_info[Y0],
                "trans-x2",  tr_tool->trans_info[X1],
                "trans-y2",  tr_tool->trans_info[Y1],
                "trans-x3",  tr_tool->trans_info[X2],
                "trans-y3",  tr_tool->trans_info[Y2],
                "trans-x4",  tr_tool->trans_info[X3],
                "trans-y4",  tr_tool->trans_info[Y3],
                NULL);

  g_object_set (widget,
                "guide-type", options->grid_type,
                "n-guides",   options->grid_size,
                NULL);

  g_signal_connect (widget, "changed",
                    G_CALLBACK (gimp_handle_transform_tool_widget_changed),
                    tr_tool);

  return widget;
}

static void
gimp_handle_transform_tool_recalc_matrix (GimpTransformTool *tr_tool,
                                          GimpToolWidget    *widget)
{
  gimp_matrix3_identity (&tr_tool->transform);
  gimp_transform_matrix_handles (&tr_tool->transform,
                                 tr_tool->trans_info[OX0],
                                 tr_tool->trans_info[OY0],
                                 tr_tool->trans_info[OX1],
                                 tr_tool->trans_info[OY1],
                                 tr_tool->trans_info[OX2],
                                 tr_tool->trans_info[OY2],
                                 tr_tool->trans_info[OX3],
                                 tr_tool->trans_info[OY3],
                                 tr_tool->trans_info[X0],
                                 tr_tool->trans_info[Y0],
                                 tr_tool->trans_info[X1],
                                 tr_tool->trans_info[Y1],
                                 tr_tool->trans_info[X2],
                                 tr_tool->trans_info[Y2],
                                 tr_tool->trans_info[X3],
                                 tr_tool->trans_info[Y3]);

  if (widget)
    g_object_set (widget,
                  "transform", &tr_tool->transform,
                  "n-handles", (gint) tr_tool->trans_info[N_HANDLES],
                  "orig-x1",   tr_tool->trans_info[OX0],
                  "orig-y1",   tr_tool->trans_info[OY0],
                  "orig-x2",   tr_tool->trans_info[OX1],
                  "orig-y2",   tr_tool->trans_info[OY1],
                  "orig-x3",   tr_tool->trans_info[OX2],
                  "orig-y3",   tr_tool->trans_info[OY2],
                  "orig-x4",   tr_tool->trans_info[OX3],
                  "orig-y4",   tr_tool->trans_info[OY3],
                  "trans-x1",  tr_tool->trans_info[X0],
                  "trans-y1",  tr_tool->trans_info[Y0],
                  "trans-x2",  tr_tool->trans_info[X1],
                  "trans-y2",  tr_tool->trans_info[Y1],
                  "trans-x3",  tr_tool->trans_info[X2],
                  "trans-y3",  tr_tool->trans_info[Y2],
                  "trans-x4",  tr_tool->trans_info[X3],
                  "trans-y4",  tr_tool->trans_info[Y3],
                  NULL);
}

static gchar *
gimp_handle_transform_tool_get_undo_desc (GimpTransformTool *tr_tool)
{
  return g_strdup (C_("undo-type", "Handle transform"));
}

static void
gimp_handle_transform_tool_widget_changed (GimpToolWidget    *widget,
                                           GimpTransformTool *tr_tool)
{
  gint n_handles;

  g_object_get (widget,
                "n-handles", &n_handles,
                "orig-x1",   &tr_tool->trans_info[OX0],
                "orig-y1",   &tr_tool->trans_info[OY0],
                "orig-x2",   &tr_tool->trans_info[OX1],
                "orig-y2",   &tr_tool->trans_info[OY1],
                "orig-x3",   &tr_tool->trans_info[OX2],
                "orig-y3",   &tr_tool->trans_info[OY2],
                "orig-x4",   &tr_tool->trans_info[OX3],
                "orig-y4",   &tr_tool->trans_info[OY3],
                "trans-x1",  &tr_tool->trans_info[X0],
                "trans-y1",  &tr_tool->trans_info[Y0],
                "trans-x2",  &tr_tool->trans_info[X1],
                "trans-y2",  &tr_tool->trans_info[Y1],
                "trans-x3",  &tr_tool->trans_info[X2],
                "trans-y3",  &tr_tool->trans_info[Y2],
                "trans-x4",  &tr_tool->trans_info[X3],
                "trans-y4",  &tr_tool->trans_info[Y3],
                NULL);

  tr_tool->trans_info[N_HANDLES] = n_handles;

  gimp_transform_tool_recalc_matrix (tr_tool, NULL);
}
