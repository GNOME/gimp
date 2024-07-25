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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimp-transform-3d-utils.h"
#include "core/gimpimage.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimppivotselector.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimptoolgui.h"
#include "display/gimptooltransform3dgrid.h"

#include "gimptoolcontrol.h"
#include "gimptransform3doptions.h"
#include "gimptransform3dtool.h"

#include "gimp-intl.h"


/*  index into trans_info array  */
enum
{
  VANISHING_POINT_X,
  VANISHING_POINT_Y,
  LENS_MODE,
  LENS_VALUE,
  OFFSET_X,
  OFFSET_Y,
  OFFSET_Z,
  ROTATION_ORDER,
  ANGLE_X,
  ANGLE_Y,
  ANGLE_Z,
  PIVOT_X,
  PIVOT_Y,
  PIVOT_Z
};


/*  local function prototypes  */

static void             gimp_transform_3d_tool_modifier_key           (GimpTool              *tool,
                                                                       GdkModifierType        key,
                                                                       gboolean               press,
                                                                       GdkModifierType        state,
                                                                       GimpDisplay           *display);

static gboolean         gimp_transform_3d_tool_info_to_matrix         (GimpTransformGridTool *tg_tool,
                                                                       GimpMatrix3           *transform);
static void             gimp_transform_3d_tool_dialog                 (GimpTransformGridTool *tg_tool);
static void             gimp_transform_3d_tool_dialog_update          (GimpTransformGridTool *tg_tool);
static void             gimp_transform_3d_tool_prepare                (GimpTransformGridTool *tg_tool);
static GimpToolWidget * gimp_transform_3d_tool_get_widget             (GimpTransformGridTool *tg_tool);
static void             gimp_transform_3d_tool_update_widget          (GimpTransformGridTool *tg_tool);
static void             gimp_transform_3d_tool_widget_changed         (GimpTransformGridTool *tg_tool);

static void             gimp_transform_3d_tool_dialog_changed         (GObject               *object,
                                                                       GimpTransform3DTool   *t3d);
static void             gimp_transform_3d_tool_lens_mode_changed      (GtkComboBox           *combo,
                                                                       GimpTransform3DTool   *t3d);
static void             gimp_transform_3d_tool_rotation_order_clicked (GtkButton             *button,
                                                                       GimpTransform3DTool   *t3d);
static void             gimp_transform_3d_tool_pivot_changed          (GimpPivotSelector     *selector,
                                                                       GimpTransform3DTool   *t3d);

static gdouble          gimp_transform_3d_tool_get_focal_length       (GimpTransform3DTool   *t3d);


G_DEFINE_TYPE (GimpTransform3DTool, gimp_transform_3d_tool, GIMP_TYPE_TRANSFORM_GRID_TOOL)

#define parent_class gimp_transform_3d_tool_parent_class


void
gimp_transform_3d_tool_register (GimpToolRegisterCallback  callback,
                                 gpointer                  data)
{
  (* callback) (GIMP_TYPE_TRANSFORM_3D_TOOL,
                GIMP_TYPE_TRANSFORM_3D_OPTIONS,
                gimp_transform_3d_options_gui,
                GIMP_CONTEXT_PROP_MASK_BACKGROUND,
                "gimp-transform-3d-tool",
                _("3D Transform"),
                _("3D Transform Tool: Apply a 3D transformation to the layer, selection or path"),
                N_("_3D Transform"), "<shift>W",
                NULL, GIMP_HELP_TOOL_TRANSFORM_3D,
                GIMP_ICON_TOOL_TRANSFORM_3D,
                data);
}

static void
gimp_transform_3d_tool_class_init (GimpTransform3DToolClass *klass)
{
  GimpToolClass              *tool_class = GIMP_TOOL_CLASS (klass);
  GimpTransformToolClass     *tr_class   = GIMP_TRANSFORM_TOOL_CLASS (klass);
  GimpTransformGridToolClass *tg_class   = GIMP_TRANSFORM_GRID_TOOL_CLASS (klass);

  tool_class->modifier_key  = gimp_transform_3d_tool_modifier_key;

  tg_class->info_to_matrix  = gimp_transform_3d_tool_info_to_matrix;
  tg_class->dialog          = gimp_transform_3d_tool_dialog;
  tg_class->dialog_update   = gimp_transform_3d_tool_dialog_update;
  tg_class->prepare         = gimp_transform_3d_tool_prepare;
  tg_class->get_widget      = gimp_transform_3d_tool_get_widget;
  tg_class->update_widget   = gimp_transform_3d_tool_update_widget;
  tg_class->widget_changed  = gimp_transform_3d_tool_widget_changed;

  tr_class->undo_desc       = C_("undo-type", "3D Transform");
  tr_class->progress_text   = _("3D transformation");
}

static void
gimp_transform_3d_tool_init (GimpTransform3DTool *t3d)
{
}

static void
gimp_transform_3d_tool_modifier_key (GimpTool        *tool,
                                     GdkModifierType  key,
                                     gboolean         press,
                                     GdkModifierType  state,
                                     GimpDisplay     *display)
{
  GimpTransform3DOptions *options = GIMP_TRANSFORM_3D_TOOL_GET_OPTIONS (tool);

  if (key == gimp_get_extend_selection_mask ())
    {
      g_object_set (options,
                    "constrain-axis", ! options->constrain_axis,
                    NULL);
    }
  else if (key == gimp_get_constrain_behavior_mask ())
    {
      g_object_set (options,
                    "z-axis", ! options->z_axis,
                    NULL);
    }
  else if (key == GDK_MOD1_MASK)
    {
      g_object_set (options,
                    "local-frame", ! options->local_frame,
                    NULL);
    }
}

static gboolean
gimp_transform_3d_tool_info_to_matrix (GimpTransformGridTool *tg_tool,
                                       GimpMatrix3           *transform)
{
  GimpTransform3DTool *t3d = GIMP_TRANSFORM_3D_TOOL (tg_tool);

  gimp_transform_3d_matrix (transform,

                            tg_tool->trans_info[VANISHING_POINT_X],
                            tg_tool->trans_info[VANISHING_POINT_Y],
                            -gimp_transform_3d_tool_get_focal_length (t3d),

                            tg_tool->trans_info[OFFSET_X],
                            tg_tool->trans_info[OFFSET_Y],
                            tg_tool->trans_info[OFFSET_Z],

                            tg_tool->trans_info[ROTATION_ORDER],
                            tg_tool->trans_info[ANGLE_X],
                            tg_tool->trans_info[ANGLE_Y],
                            tg_tool->trans_info[ANGLE_Z],

                            tg_tool->trans_info[PIVOT_X],
                            tg_tool->trans_info[PIVOT_Y],
                            tg_tool->trans_info[PIVOT_Z]);

  return TRUE;
}

static void
gimp_transform_3d_tool_dialog (GimpTransformGridTool *tg_tool)
{
  GimpTransform3DTool    *t3d   = GIMP_TRANSFORM_3D_TOOL (tg_tool);
  GimpTransform3DOptions *options   = GIMP_TRANSFORM_3D_TOOL_GET_OPTIONS (tg_tool);
  GtkWidget              *notebook;
  GtkWidget              *label;
  GtkWidget              *vbox;
  GtkWidget              *frame;
  GtkWidget              *vbox2;
  GtkWidget              *se;
  GtkWidget              *spinbutton;
  GtkWidget              *combo;
  GtkWidget              *scale;
  GtkWidget              *grid;
  GtkWidget              *button;
  GtkWidget              *selector;
  gint                    i;

  /* main notebook */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
  gtk_box_pack_start (GTK_BOX (gimp_tool_gui_get_vbox (tg_tool->gui)),
                      notebook, FALSE, FALSE, 0);
  gtk_widget_show (notebook);

  t3d->notebook = notebook;

  /* camera page */
  label = gtk_image_new_from_icon_name (GIMP_ICON_TRANSFORM_3D_CAMERA,
                                        GTK_ICON_SIZE_MENU);
  gimp_help_set_help_data (label, _("Camera"), NULL);
  gtk_widget_show (label);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (vbox);

  /* vanishing-point frame */
  frame = gimp_frame_new (_("Vanishing Point"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* vanishing-point size entry */
  se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a", TRUE, TRUE, FALSE, 6,
                            GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_row_spacing (GTK_GRID (se), 2);
  gtk_grid_set_column_spacing (GTK_GRID (se), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), se, FALSE, FALSE, 0);
  gtk_widget_show (se);

  t3d->vanishing_point_se = se;

  spinbutton = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (se), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (se), _("_X:"), 0, 0, 0.0);
  gimp_size_entry_attach_label (GIMP_SIZE_ENTRY (se), _("_Y:"), 1, 0, 0.0);

  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 0, 2);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 1, 2);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_transform_3d_tool_dialog_changed),
                    t3d);

  /* lens frame */
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  /* lens-mode combo */
  combo = gimp_enum_combo_box_new (GIMP_TYPE_TRANSFORM_3D_LENS_MODE);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);
  gtk_widget_show (combo);

  t3d->lens_mode_combo = combo;

  g_signal_connect (combo, "changed",
                    G_CALLBACK (gimp_transform_3d_tool_lens_mode_changed),
                    t3d);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* focal-length size entry */
  se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a", TRUE, FALSE, FALSE, 6,
                            GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_row_spacing (GTK_GRID (se), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), se, FALSE, FALSE, 0);

  t3d->focal_length_se = se;

  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 0, 2);

  gimp_size_entry_set_value_boundaries (GIMP_SIZE_ENTRY (se), 0,
                                        0.0, G_MAXDOUBLE);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_transform_3d_tool_dialog_changed),
                    t3d);

  /* angle-of-view spin scale */
  t3d->angle_of_view_adj = (GtkAdjustment *)
    gtk_adjustment_new (0, 0, 180, 1, 10, 0);
  scale = gimp_spin_scale_new (t3d->angle_of_view_adj,
                               _("Angle"), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  t3d->angle_of_view_scale = scale;

  g_signal_connect (t3d->angle_of_view_adj, "value-changed",
                    G_CALLBACK (gimp_transform_3d_tool_dialog_changed),
                    t3d);

  /* move page */
  label = gtk_image_new_from_icon_name (GIMP_ICON_TRANSFORM_3D_MOVE,
                                        GTK_ICON_SIZE_MENU);
  gimp_help_set_help_data (label, _("Move"), NULL);
  gtk_widget_show (label);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
  gtk_widget_show (vbox);

  /* offset frame */
  frame = gimp_frame_new (_("Offset"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  /* offset size entry */
  se = gimp_size_entry_new (1, gimp_unit_pixel (), "%a", TRUE, TRUE, FALSE, 6,
                            GIMP_SIZE_ENTRY_UPDATE_SIZE);
  gtk_grid_set_row_spacing (GTK_GRID (se), 2);
  gtk_grid_set_column_spacing (GTK_GRID (se), 2);
  gtk_box_pack_start (GTK_BOX (vbox2), se, FALSE, FALSE, 0);
  gtk_widget_show (se);

  t3d->offset_se = se;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_attach (GTK_GRID (se), grid, 0, 0, 2, 1);
  gtk_widget_show (grid);

  spinbutton = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 1, 1, 1, 1);
  gtk_widget_show (spinbutton);

  spinbutton = gimp_spin_button_new_with_range (0.0, 0.0, 1.0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);
  gimp_size_entry_add_field (GIMP_SIZE_ENTRY (se),
                             GTK_SPIN_BUTTON (spinbutton), NULL);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 1, 0, 1, 1);
  gtk_widget_show (spinbutton);

  label = gtk_label_new_with_mnemonic (_("_X:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Y:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  label = gtk_label_new_with_mnemonic (_("_Z:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (se), label, 0, 1, 1, 1);
  gtk_widget_show (label);

  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (se), 0,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (se), 1,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (GIMP_SIZE_ENTRY (se), 2,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 0, 2);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 1, 2);
  gimp_size_entry_set_refval_digits (GIMP_SIZE_ENTRY (se), 2, 2);

  g_signal_connect (se, "value-changed",
                    G_CALLBACK (gimp_transform_3d_tool_dialog_changed),
                    t3d);

  /* rotate page */
  label = gtk_image_new_from_icon_name (GIMP_ICON_TRANSFORM_3D_ROTATE,
                                        GTK_ICON_SIZE_MENU);
  gimp_help_set_help_data (label, _("Rotate"), NULL);
  gtk_widget_show (label);

  /* angle frame */
  frame = gimp_frame_new (_("Angle"));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), frame, label);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  for (i = 0; i < 3; i++)
    {
      const gchar *labels[3] = {_("X"), _("Y"), _("Z")};

      /* rotation-order button */
      button = gtk_button_new ();
      gimp_help_set_help_data (button, _("Rotation axis order"), NULL);
      gtk_grid_attach (GTK_GRID (grid), button, 0, i, 1, 1);
      gtk_widget_show (button);

      t3d->rotation_order_buttons[i] = button;

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_transform_3d_tool_rotation_order_clicked),
                        t3d);

      /* angle spin scale */
      t3d->angle_adj[i] = (GtkAdjustment *)
        gtk_adjustment_new (0, -180, 180, 1, 10, 0);
      scale = gimp_spin_scale_new (t3d->angle_adj[i],
                                   labels[i], 2);
      gtk_widget_set_hexpand (GTK_WIDGET (scale), TRUE);
      gtk_spin_button_set_wrap (GTK_SPIN_BUTTON (scale), TRUE);
      gtk_grid_attach (GTK_GRID (grid), scale, 1, i, 1, 1);
      gtk_widget_show (scale);

      g_signal_connect (t3d->angle_adj[i], "value-changed",
                        G_CALLBACK (gimp_transform_3d_tool_dialog_changed),
                        t3d);
    }

  /* pivot selector */
  selector = gimp_pivot_selector_new (0.0, 0.0, 0.0, 0.0);
  gtk_grid_attach (GTK_GRID (grid), selector, 2, 0, 1, 3);
  gtk_widget_show (selector);

  t3d->pivot_selector = selector;

  g_signal_connect (selector, "changed",
                    G_CALLBACK (gimp_transform_3d_tool_pivot_changed),
                    t3d);

  g_object_bind_property (options,        "mode",
                          t3d->notebook,  "page",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_BIDIRECTIONAL);
}

static void
gimp_transform_3d_tool_dialog_update (GimpTransformGridTool *tg_tool)
{
  GimpTransform3DTool     *t3d     = GIMP_TRANSFORM_3D_TOOL (tg_tool);
  Gimp3DTransformLensMode  lens_mode;
  gint                     permutation[3];
  gint                     i;

  t3d->updating = TRUE;

  /* vanishing-point size entry */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->vanishing_point_se),
                              0, tg_tool->trans_info[VANISHING_POINT_X]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->vanishing_point_se),
                              1, tg_tool->trans_info[VANISHING_POINT_Y]);

  lens_mode = tg_tool->trans_info[LENS_MODE];

  /* lens-mode combo */
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (t3d->lens_mode_combo),
                                 lens_mode);

  /* focal-length size entry / angle-of-view spin scale */
  gtk_widget_set_visible (t3d->focal_length_se,
                          lens_mode ==
                          GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH);
  gtk_widget_set_visible (t3d->angle_of_view_scale,
                          lens_mode !=
                          GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH);

  switch (lens_mode)
    {
    case GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH:
      gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->focal_length_se),
                                  0, tg_tool->trans_info[LENS_VALUE]);
      break;

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE:
    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM:
      gtk_adjustment_set_value (
        t3d->angle_of_view_adj,
        gimp_rad_to_deg (tg_tool->trans_info[LENS_VALUE]));
      break;
    }

  /* offset size entry */
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->offset_se),
                              0, tg_tool->trans_info[OFFSET_X]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->offset_se),
                              1, tg_tool->trans_info[OFFSET_Y]);
  gimp_size_entry_set_refval (GIMP_SIZE_ENTRY (t3d->offset_se),
                              2, tg_tool->trans_info[OFFSET_Z]);

  /* rotation-order buttons */
  gimp_transform_3d_rotation_order_to_permutation (
    tg_tool->trans_info[ROTATION_ORDER], permutation);

  for (i = 0; i < 3; i++)
    {
      gchar *label;

      label = g_strdup_printf ("%d", i + 1);

      gtk_button_set_label (
        GTK_BUTTON (t3d->rotation_order_buttons[permutation[i]]), label);

      g_free (label);
    }

  /* angle spin scales */
  gtk_adjustment_set_value (t3d->angle_adj[0],
                            gimp_rad_to_deg (tg_tool->trans_info[ANGLE_X]));
  gtk_adjustment_set_value (t3d->angle_adj[1],
                            gimp_rad_to_deg (tg_tool->trans_info[ANGLE_Y]));
  gtk_adjustment_set_value (t3d->angle_adj[2],
                            gimp_rad_to_deg (tg_tool->trans_info[ANGLE_Z]));

  /* pivot selector */
  gimp_pivot_selector_set_position (GIMP_PIVOT_SELECTOR (t3d->pivot_selector),
                                    tg_tool->trans_info[PIVOT_X],
                                    tg_tool->trans_info[PIVOT_Y]);

  t3d->updating = FALSE;
}

static void
gimp_transform_3d_tool_prepare (GimpTransformGridTool *tg_tool)
{
  GimpTool            *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool   *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransform3DTool *t3d     = GIMP_TRANSFORM_3D_TOOL (tg_tool);
  GimpDisplay         *display = tool->display;
  GimpDisplayShell    *shell   = gimp_display_get_shell (display);
  GimpImage           *image   = gimp_display_get_image (display);
  GimpSizeEntry       *se;
  gdouble              xres;
  gdouble              yres;
  gint                 width;
  gint                 height;

  gimp_image_get_resolution (image, &xres, &yres);

  width  = gimp_image_get_width  (image);
  height = gimp_image_get_height (image);

  tg_tool->trans_info[VANISHING_POINT_X] = (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[VANISHING_POINT_Y] = (tr_tool->y1 + tr_tool->y2) / 2.0;

  tg_tool->trans_info[LENS_MODE]         = GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM;
  tg_tool->trans_info[LENS_VALUE]        = gimp_deg_to_rad (45.0);

  tg_tool->trans_info[OFFSET_X]          = 0.0;
  tg_tool->trans_info[OFFSET_Y]          = 0.0;
  tg_tool->trans_info[OFFSET_Z]          = 0.0;

  tg_tool->trans_info[ROTATION_ORDER]    =
    gimp_transform_3d_permutation_to_rotation_order ((const gint[]) {0, 1, 2});
  tg_tool->trans_info[ANGLE_X]           = 0.0;
  tg_tool->trans_info[ANGLE_Y]           = 0.0;
  tg_tool->trans_info[ANGLE_Z]           = 0.0;

  tg_tool->trans_info[PIVOT_X]           = (tr_tool->x1 + tr_tool->x2) / 2.0;
  tg_tool->trans_info[PIVOT_Y]           = (tr_tool->y1 + tr_tool->y2) / 2.0;
  tg_tool->trans_info[PIVOT_Z]           = 0.0;

  t3d->updating = TRUE;

  /* vanishing-point size entry */
  se = GIMP_SIZE_ENTRY (t3d->vanishing_point_se);

  gimp_size_entry_set_unit (se, shell->unit);

  gimp_size_entry_set_resolution (se, 0, xres, FALSE);
  gimp_size_entry_set_resolution (se, 1, yres, FALSE);

  gimp_size_entry_set_refval_boundaries (se, 0,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);
  gimp_size_entry_set_refval_boundaries (se, 1,
                                         -GIMP_MAX_IMAGE_SIZE,
                                         GIMP_MAX_IMAGE_SIZE);

  gimp_size_entry_set_size (se, 0, tr_tool->x1, tr_tool->x2);
  gimp_size_entry_set_size (se, 1, tr_tool->y1, tr_tool->y2);

  /* focal-length size entry */
  se = GIMP_SIZE_ENTRY (t3d->focal_length_se);

  gimp_size_entry_set_unit (se, shell->unit);

  gimp_size_entry_set_resolution (se, 0, width >= height ? xres : yres, FALSE);

  /* offset size entry */
  se = GIMP_SIZE_ENTRY (t3d->offset_se);

  gimp_size_entry_set_unit (se, shell->unit);

  gimp_size_entry_set_resolution (se, 0, xres, FALSE);
  gimp_size_entry_set_resolution (se, 1, yres, FALSE);
  gimp_size_entry_set_resolution (se, 2, width >= height ? xres : yres, FALSE);

  gimp_size_entry_set_size (se, 0, 0, width);
  gimp_size_entry_set_size (se, 1, 0, height);
  gimp_size_entry_set_size (se, 2, 0, MAX (width, height));

  /* pivot selector */
  gimp_pivot_selector_set_bounds (GIMP_PIVOT_SELECTOR (t3d->pivot_selector),
                                  tr_tool->x1, tr_tool->y1,
                                  tr_tool->x2, tr_tool->y2);

  t3d->updating = FALSE;
}

static GimpToolWidget *
gimp_transform_3d_tool_get_widget (GimpTransformGridTool *tg_tool)
{
  GimpTool               *tool    = GIMP_TOOL (tg_tool);
  GimpTransformTool      *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransform3DTool    *t3d     = GIMP_TRANSFORM_3D_TOOL (tg_tool);
  GimpTransform3DOptions *options = GIMP_TRANSFORM_3D_TOOL_GET_OPTIONS (tg_tool);
  GimpDisplayShell       *shell   = gimp_display_get_shell (tool->display);
  GimpToolWidget         *widget;
  gint                    i;

  static const gchar *bound_properties[] =
  {
    "mode",
    "unified",
    "constrain-axis",
    "z-axis",
    "local-frame",
  };

  widget = gimp_tool_transform_3d_grid_new (
    shell,
    tr_tool->x1,
    tr_tool->y1,
    tr_tool->x2,
    tr_tool->y2,
    tg_tool->trans_info[VANISHING_POINT_X],
    tg_tool->trans_info[VANISHING_POINT_Y],
    -gimp_transform_3d_tool_get_focal_length (t3d));

  for (i = 0; i < G_N_ELEMENTS (bound_properties); i++)
    {
      g_object_bind_property (options, bound_properties[i],
                              widget,  bound_properties[i],
                              G_BINDING_SYNC_CREATE |
                              G_BINDING_BIDIRECTIONAL);
    }

  return widget;
}

static void
gimp_transform_3d_tool_update_widget (GimpTransformGridTool *tg_tool)
{
  GimpTransformTool   *tr_tool = GIMP_TRANSFORM_TOOL (tg_tool);
  GimpTransform3DTool *t3d     = GIMP_TRANSFORM_3D_TOOL (tg_tool);
  GimpMatrix3          transform;

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->update_widget (tg_tool);

  gimp_transform_grid_tool_info_to_matrix (tg_tool, &transform);

  g_object_set (
    tg_tool->widget,

    "x1",             (gdouble) tr_tool->x1,
    "y1",             (gdouble) tr_tool->y1,
    "x2",             (gdouble) tr_tool->x2,
    "y2",             (gdouble) tr_tool->y2,

    "camera-x",       tg_tool->trans_info[VANISHING_POINT_X],
    "camera-y",       tg_tool->trans_info[VANISHING_POINT_Y],
    "camera-z",       -gimp_transform_3d_tool_get_focal_length (t3d),

    "offset-x",       tg_tool->trans_info[OFFSET_X],
    "offset-y",       tg_tool->trans_info[OFFSET_Y],
    "offset-z",       tg_tool->trans_info[OFFSET_Z],

    "rotation-order", (gint) tg_tool->trans_info[ROTATION_ORDER],
    "angle-x",        tg_tool->trans_info[ANGLE_X],
    "angle-y",        tg_tool->trans_info[ANGLE_Y],
    "angle-z",        tg_tool->trans_info[ANGLE_Z],

    "pivot-3d-x",     tg_tool->trans_info[PIVOT_X],
    "pivot-3d-y",     tg_tool->trans_info[PIVOT_Y],
    "pivot-3d-z",     tg_tool->trans_info[PIVOT_Z],

    "transform",      &transform,

    NULL);
}

static void
gimp_transform_3d_tool_widget_changed (GimpTransformGridTool *tg_tool)
{
  g_object_get (
    tg_tool->widget,

    "camera-x", &tg_tool->trans_info[VANISHING_POINT_X],
    "camera-y", &tg_tool->trans_info[VANISHING_POINT_Y],

    "offset-x", &tg_tool->trans_info[OFFSET_X],
    "offset-y", &tg_tool->trans_info[OFFSET_Y],
    "offset-z", &tg_tool->trans_info[OFFSET_Z],

    "angle-x",  &tg_tool->trans_info[ANGLE_X],
    "angle-y",  &tg_tool->trans_info[ANGLE_Y],
    "angle-z",  &tg_tool->trans_info[ANGLE_Z],

    NULL);

  GIMP_TRANSFORM_GRID_TOOL_CLASS (parent_class)->widget_changed (tg_tool);
}

static void
gimp_transform_3d_tool_dialog_changed (GObject             *object,
                                       GimpTransform3DTool *t3d)
{
  GimpTool              *tool    = GIMP_TOOL (t3d);
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (t3d);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (t3d);

  if (t3d->updating)
    return;

  /* vanishing-point size entry */
  tg_tool->trans_info[VANISHING_POINT_X] = gimp_size_entry_get_refval (
    GIMP_SIZE_ENTRY (t3d->vanishing_point_se), 0);
  tg_tool->trans_info[VANISHING_POINT_Y] = gimp_size_entry_get_refval (
    GIMP_SIZE_ENTRY (t3d->vanishing_point_se), 1);

  /* focal-length size entry / angle-of-view spin scale */
  switch ((gint) tg_tool->trans_info[LENS_MODE])
    {
    case GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH:
      tg_tool->trans_info[LENS_VALUE] = gimp_size_entry_get_refval (
        GIMP_SIZE_ENTRY (t3d->focal_length_se), 0);
      break;

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE:
    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM:
      tg_tool->trans_info[LENS_VALUE] = gimp_deg_to_rad (
        gtk_adjustment_get_value (t3d->angle_of_view_adj));
      break;
    }

  /* offset size entry */
  tg_tool->trans_info[OFFSET_X] = gimp_size_entry_get_refval (
    GIMP_SIZE_ENTRY (t3d->offset_se), 0);
  tg_tool->trans_info[OFFSET_Y] = gimp_size_entry_get_refval (
    GIMP_SIZE_ENTRY (t3d->offset_se), 1);
  tg_tool->trans_info[OFFSET_Z] = gimp_size_entry_get_refval (
    GIMP_SIZE_ENTRY (t3d->offset_se), 2);

  /* angle spin scales */
  tg_tool->trans_info[ANGLE_X] = gimp_deg_to_rad (gtk_adjustment_get_value (
    t3d->angle_adj[0]));
  tg_tool->trans_info[ANGLE_Y] = gimp_deg_to_rad (gtk_adjustment_get_value (
    t3d->angle_adj[1]));
  tg_tool->trans_info[ANGLE_Z] = gimp_deg_to_rad (gtk_adjustment_get_value (
    t3d->angle_adj[2]));

  gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
}

static void
gimp_transform_3d_tool_lens_mode_changed (GtkComboBox         *combo,
                                          GimpTransform3DTool *t3d)
{
  GimpTool              *tool    = GIMP_TOOL (t3d);
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (t3d);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (t3d);
  GimpDisplay           *display = tool->display;
  GimpImage             *image   = gimp_display_get_image (display);
  gdouble                x1      = 0.0;
  gdouble                y1      = 0.0;
  gdouble                x2      = 0.0;
  gdouble                y2      = 0.0;
  gint                   width;
  gint                   height;
  gint                   lens_mode;
  gdouble                focal_length;

  if (t3d->updating)
    return;

  if (! gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (combo), &lens_mode))
    return;

  width  = gimp_image_get_width  (image);
  height = gimp_image_get_height (image);

  focal_length = gimp_transform_3d_tool_get_focal_length (t3d);

  tg_tool->trans_info[LENS_MODE] = lens_mode;

  switch (lens_mode)
    {
    case GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH:
    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE:
      x1 = 0.0;
      y1 = 0.0;

      x2 = width;
      y2 = height;
      break;

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM:
      x1 = tr_tool->x1;
      y1 = tr_tool->y1;

      x2 = tr_tool->x2;
      y2 = tr_tool->y2;
      break;
    }

  switch (lens_mode)
    {
    case GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH:
      tg_tool->trans_info[LENS_VALUE] = focal_length;
      break;

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE:
    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM:
      tg_tool->trans_info[LENS_VALUE] =
        gimp_transform_3d_focal_length_to_angle_of_view (
          focal_length, x2 - x1, y2 - y1);
      break;
    }

  /* vanishing-point size entry */
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (t3d->vanishing_point_se), 0,
                            x1, x2);
  gimp_size_entry_set_size (GIMP_SIZE_ENTRY (t3d->vanishing_point_se), 1,
                            y1, y2);

  gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
}

static void
gimp_transform_3d_tool_rotation_order_clicked (GtkButton           *button,
                                               GimpTransform3DTool *t3d)
{
  GimpTool              *tool = GIMP_TOOL (t3d);
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (t3d);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (t3d);
  GimpMatrix4            matrix;
  gint                   permutation[3];
  gint                   b;
  gint                   i;

  for (b = 0; b < 3; b++)
    {
      if (GTK_BUTTON (t3d->rotation_order_buttons[b]) == button)
        break;
    }

  gimp_transform_3d_rotation_order_to_permutation (
    tg_tool->trans_info[ROTATION_ORDER], permutation);

  if (permutation[0] == b)
    {
      gint temp;

      temp           = permutation[1];
      permutation[1] = permutation[2];
      permutation[2] = temp;
    }
  else
    {
      gint temp;

      temp           = permutation[0];
      permutation[0] = b;

      for (i = 1; i < 3; i++)
        {
          if (permutation[i] == b)
            {
              permutation[i] = temp;

              break;
            }
        }
    }

  gimp_matrix4_identity (&matrix);

  gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                          tg_tool->trans_info[ROTATION_ORDER],
                                          tg_tool->trans_info[ANGLE_X],
                                          tg_tool->trans_info[ANGLE_Y],
                                          tg_tool->trans_info[ANGLE_Z],
                                          0.0, 0.0, 0.0);

  tg_tool->trans_info[ROTATION_ORDER] =
    gimp_transform_3d_permutation_to_rotation_order (permutation);

  gimp_transform_3d_matrix4_rotate_euler_decompose (
    &matrix,
    tg_tool->trans_info[ROTATION_ORDER],
    &tg_tool->trans_info[ANGLE_X],
    &tg_tool->trans_info[ANGLE_Y],
    &tg_tool->trans_info[ANGLE_Z]);

  gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
}

static void
gimp_transform_3d_tool_pivot_changed (GimpPivotSelector   *selector,
                                      GimpTransform3DTool *t3d)
{
  GimpTool              *tool = GIMP_TOOL (t3d);
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (t3d);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (t3d);
  GimpMatrix4            matrix;
  gdouble                offset_x;
  gdouble                offset_y;
  gdouble                offset_z;

  if (t3d->updating)
    return;

  gimp_matrix4_identity (&matrix);

  gimp_transform_3d_matrix4_rotate_euler (&matrix,
                                          tg_tool->trans_info[ROTATION_ORDER],
                                          tg_tool->trans_info[ANGLE_X],
                                          tg_tool->trans_info[ANGLE_Y],
                                          tg_tool->trans_info[ANGLE_Z],
                                          tg_tool->trans_info[PIVOT_X],
                                          tg_tool->trans_info[PIVOT_Y],
                                          tg_tool->trans_info[PIVOT_Z]);

  gimp_pivot_selector_get_position (GIMP_PIVOT_SELECTOR (t3d->pivot_selector),
                                    &tg_tool->trans_info[PIVOT_X],
                                    &tg_tool->trans_info[PIVOT_Y]);

  gimp_matrix4_transform_point (&matrix,
                                tg_tool->trans_info[PIVOT_X],
                                tg_tool->trans_info[PIVOT_Y],
                                tg_tool->trans_info[PIVOT_Z],
                                &offset_x, &offset_y, &offset_z);

  tg_tool->trans_info[OFFSET_X] += offset_x - tg_tool->trans_info[PIVOT_X];
  tg_tool->trans_info[OFFSET_Y] += offset_y - tg_tool->trans_info[PIVOT_Y];
  tg_tool->trans_info[OFFSET_Z] += offset_z - tg_tool->trans_info[PIVOT_Z];

  gimp_transform_grid_tool_push_internal_undo (tg_tool, TRUE);

  gimp_transform_tool_recalc_matrix (tr_tool, tool->display);
}

static gdouble
gimp_transform_3d_tool_get_focal_length (GimpTransform3DTool *t3d)
{
  GimpTool              *tool    = GIMP_TOOL (t3d);
  GimpTransformTool     *tr_tool = GIMP_TRANSFORM_TOOL (t3d);
  GimpTransformGridTool *tg_tool = GIMP_TRANSFORM_GRID_TOOL (t3d);
  GimpImage             *image   = gimp_display_get_image (tool->display);
  gdouble                width   = 0.0;
  gdouble                height  = 0.0;

  switch ((int) tg_tool->trans_info[LENS_MODE])
    {
    case GIMP_TRANSFORM_3D_LENS_MODE_FOCAL_LENGTH:
      return tg_tool->trans_info[LENS_VALUE];

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_IMAGE:
      width  = gimp_image_get_width  (image);
      height = gimp_image_get_height (image);
      break;

    case GIMP_TRANSFORM_3D_LENS_MODE_FOV_ITEM:
      width  = tr_tool->x2 - tr_tool->x1;
      height = tr_tool->y2 - tr_tool->y1;
      break;
    }

  return gimp_transform_3d_angle_of_view_to_focal_length (
    tg_tool->trans_info[LENS_VALUE], width, height);
}
