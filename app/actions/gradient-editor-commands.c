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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"

#include "widgets/gimpgradienteditor.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpviewabledialog.h"

#include "color-notebook.h"
#include "dialogs.h"
#include "gradient-editor-commands.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gradient_editor_left_color_changed     (ColorNotebook     *cnb,
                                                      const GimpRGB     *color,
                                                      ColorNotebookState state,
                                                      gpointer           data);
static void   gradient_editor_right_color_changed    (ColorNotebook     *cnb,
                                                      const GimpRGB     *color,
                                                      ColorNotebookState state,
                                                      gpointer           data);

static GimpGradientSegment *
              gradient_editor_save_selection         (GimpGradientEditor  *editor);
static void   gradient_editor_replace_selection      (GimpGradientEditor  *editor,
                                                      GimpGradientSegment *replace_seg);

static void   gradient_editor_split_uniform_response (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);
static void   gradient_editor_replicate_response     (GtkWidget           *widget,
                                                      gint                 response_id,
                                                      GimpGradientEditor  *editor);


/*  public functionss */

void
gradient_editor_left_color_cmd_callback (GtkWidget *widget,
                                         gpointer   data,
                                         guint      action)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->left_saved_dirty    = GIMP_DATA (gradient)->dirty;
  editor->left_saved_segments = gradient_editor_save_selection (editor);

  editor->color_notebook =
    color_notebook_new (GIMP_VIEWABLE (gradient),
                        _("Left Endpoint Color"),
                        GIMP_STOCK_GRADIENT,
                        _("Gradient Segment's Left Endpoint Color"),
                        GTK_WIDGET (editor),
                        global_dialog_factory,
                        "gimp-gradient-editor-color-dialog",
                        &editor->control_sel_l->left_color,
                        gradient_editor_left_color_changed, editor,
                        editor->instant_update, TRUE);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

void
gradient_editor_load_left_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpContext         *user_context;
  GimpGradientSegment *seg;
  GimpRGB              color;
  gint                 i;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->data_factory->gimp);

  i = (gint) action;

  switch (i)
    {
    case 0: /* Fetch from left neighbor's right endpoint */
      if (editor->control_sel_l->prev != NULL)
	seg = editor->control_sel_l->prev;
      else
	seg = gimp_gradient_segment_get_last (editor->control_sel_l);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &seg->right_color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      break;

    case 1: /* Fetch from right endpoint */
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_r->right_color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (user_context, &color);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      break;

    case 3: /* Fetch from BG color */
      gimp_context_get_background (user_context, &color);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      break;

    default: /* Load a color */
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->saved_colors[i - 4],
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gradient));
}

void
gradient_editor_save_left_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpGradientEditor *editor;

  editor = GIMP_GRADIENT_EDITOR (data);

  editor->saved_colors[action] = editor->control_sel_l->left_color;
}

void
gradient_editor_right_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data,
                                          guint      action)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->right_saved_dirty    = GIMP_DATA (gradient)->dirty;
  editor->right_saved_segments = gradient_editor_save_selection (editor);

  editor->color_notebook =
    color_notebook_new (GIMP_VIEWABLE (gradient),
                        _("Right Endpoint Color"),
                        GIMP_STOCK_GRADIENT,
                        _("Gradient Segment's Right Endpoint Color"),
                        GTK_WIDGET (editor),
                        global_dialog_factory,
                        "gimp-gradient-editor-color-dialog",
                        &editor->control_sel_l->right_color,
                        gradient_editor_right_color_changed, editor,
                        editor->instant_update, TRUE);

  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

void
gradient_editor_load_right_cmd_callback (GtkWidget *widget,
                                         gpointer   data,
                                         guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpContext         *user_context;
  GimpGradientSegment *seg;
  GimpRGB              color;
  gint                 i;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->data_factory->gimp);

  i = (gint) action;

  switch (i)
    {
    case 0: /* Fetch from right neighbor's left endpoint */
      if (editor->control_sel_r->next != NULL)
	seg = editor->control_sel_r->next;
      else
	seg = gimp_gradient_segment_get_first (editor->control_sel_r);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_r->left_color,
                                              &seg->left_color,
                                              TRUE, TRUE);
      break;

    case 1: /* Fetch from left endpoint */
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_l->left_color,
                                              &editor->control_sel_l->left_color,
                                              TRUE, TRUE);
      break;

    case 2: /* Fetch from FG color */
      gimp_context_get_foreground (user_context, &color);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_l->left_color,
                                              &color,
                                              TRUE, TRUE);
      break;

    case 3: /* Fetch from BG color */
      gimp_context_get_background (user_context, &color);

      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_l->left_color,
                                              &color,
                                              TRUE, TRUE);
      break;

    default: /* Load a color */
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_l->left_color,
                                              &editor->saved_colors[i - 4],
                                              TRUE, TRUE);
      break;
    }

  gimp_data_dirty (GIMP_DATA (gradient));
}

void
gradient_editor_save_right_cmd_callback (GtkWidget *widget,
                                         gpointer   data,
                                         guint      action)
{
  GimpGradientEditor *editor;

  editor = GIMP_GRADIENT_EDITOR (data);

  editor->saved_colors[action] = editor->control_sel_l->left_color;
}

void
gradient_editor_blending_func_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor      *editor;
  GimpGradient            *gradient;
  GimpGradientSegmentType  type;
  GimpGradientSegment     *seg, *aseg;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  type = (GimpGradientSegmentType) action;

  seg = editor->control_sel_l;

  do
    {
      seg->type = type;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));
}

void
gradient_editor_coloring_type_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor       *editor;
  GimpGradient             *gradient;
  GimpGradientSegmentColor  color;
  GimpGradientSegment      *seg, *aseg;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  color = (GimpGradientSegmentColor) action;

  seg = editor->control_sel_l;

  do
    {
      seg->color = color;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));
}

void
gradient_editor_flip_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *lseg, *rseg;
  gdouble              left, right;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  left  = editor->control_sel_l->left;
  right = editor->control_sel_r->right;

  /* Build flipped segments */

  prev = NULL;
  oseg = editor->control_sel_r;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      if (prev == NULL)
	{
	  seg->left = left;
	  tmp = seg; /* Remember first segment */
	}
      else
	seg->left = left + right - oseg->right;

      seg->middle = left + right - oseg->middle;
      seg->right  = left + right - oseg->left;

      seg->left_color = oseg->right_color;

      seg->right_color = oseg->left_color;

      switch (oseg->type)
	{
	case GIMP_GRAD_SPHERE_INCREASING:
	  seg->type = GIMP_GRAD_SPHERE_DECREASING;
	  break;

	case GIMP_GRAD_SPHERE_DECREASING:
	  seg->type = GIMP_GRAD_SPHERE_INCREASING;
	  break;

	default:
	  seg->type = oseg->type;
	}

      switch (oseg->color)
	{
	case GIMP_GRAD_HSV_CCW:
	  seg->color = GIMP_GRAD_HSV_CW;
	  break;

	case GIMP_GRAD_HSV_CW:
	  seg->color = GIMP_GRAD_HSV_CCW;
	  break;

	default:
	  seg->color = oseg->color;
	}

      seg->prev = prev;
      seg->next = NULL;

      if (prev)
	prev->next = seg;

      prev = seg;

      oaseg = oseg;
      oseg  = oseg->prev; /* Move backwards! */
    }
  while (oaseg != editor->control_sel_l);

  seg->right = right; /* Squish accumulative error */

  /* Free old segments */

  lseg = editor->control_sel_l->prev;
  rseg = editor->control_sel_r->next;

  oseg = editor->control_sel_l;

  do
    {
      oaseg = oseg->next;
      gimp_gradient_segment_free (oseg);
      oseg = oaseg;
    }
  while (oaseg != rseg);

  /* Link in new segments */

  if (lseg)
    lseg->next = tmp;
  else
    gradient->segments = tmp;

  tmp->prev = lseg;

  seg->next = rseg;

  if (rseg)
    rseg->prev = seg;

  /* Reset selection */

  editor->control_sel_l = tmp;
  editor->control_sel_r = seg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));
  gimp_gradient_editor_update (editor);
}

void
gradient_editor_replicate_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpGradientEditor *editor;
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *label;
  GtkWidget          *scale;
  GtkObject          *scale_data;
  const gchar        *title;
  const gchar        *desc;

  editor = GIMP_GRADIENT_EDITOR (data);

  if (editor->control_sel_l == editor->control_sel_r)
    {
      title = _("Replicate Segment");
      desc  = _("Replicate Gradient Segment");
    }
  else
    {
      title = _("Replicate Selection");
      desc  = _("Replicate Gradient Selection");
    }

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (GIMP_DATA_EDITOR (editor)->data),
                              title, "gimp-gradient-segment-replicate",
                              GIMP_STOCK_GRADIENT, desc,
                              GTK_WIDGET (editor),
                              gimp_standard_help_func,
                              GIMP_HELP_GRADIENT_EDITOR_REPLICATE,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              _("Replicate"),   GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gradient_editor_replicate_response),
                    editor);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  if (editor->control_sel_l == editor->control_sel_r)
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selected segment."));
  else
    label = gtk_label_new (_("Select the number of times\n"
                             "to replicate the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  editor->replicate_times = 2;
  scale_data  = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, TRUE, 4);
  gtk_widget_show (scale);

  g_signal_connect (scale_data, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &editor->replicate_times);

  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

void
gradient_editor_split_midpoint_cmd_callback (GtkWidget *widget,
                                             gpointer   data,
                                             guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *lseg, *rseg;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  seg = editor->control_sel_l;

  do
    {
      gimp_gradient_segment_split_midpoint (gradient, seg, &lseg, &rseg);
      seg = rseg->next;
    }
  while (lseg != editor->control_sel_r);

  editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));
  gimp_gradient_editor_update (editor);
}

void
gradient_editor_split_uniformly_cmd_callback (GtkWidget *widget,
                                              gpointer   data,
                                              guint      action)
{
  GimpGradientEditor *editor;
  GtkWidget          *dialog;
  GtkWidget          *vbox;
  GtkWidget          *label;
  GtkWidget          *scale;
  GtkObject          *scale_data;
  const gchar        *title;
  const gchar        *desc;

  editor = GIMP_GRADIENT_EDITOR (data);

  if (editor->control_sel_l == editor->control_sel_r)
    {
      title = _("Split Segment Uniformly");
      desc  = _("Split Gradient Segment Uniformly");
    }
  else
    {
      title = _("Split Segments Uniformly");
      desc  = _("Split Gradient Segments Uniformly");
    }

  dialog =
    gimp_viewable_dialog_new (GIMP_VIEWABLE (GIMP_DATA_EDITOR (editor)->data),
                              title, "gimp-gradient-segment-split-uniformly",
                              GIMP_STOCK_GRADIENT, desc,
                              GTK_WIDGET (editor),
                              gimp_standard_help_func,
                              GIMP_HELP_GRADIENT_EDITOR_SPLIT_UNIFORM,

                              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                              _("Split"),       GTK_RESPONSE_OK,

                              NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gradient_editor_split_uniform_response),
                    editor);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  if (editor->control_sel_l == editor->control_sel_r)
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the selected segment."));
  else
    label = gtk_label_new (_("Select the number of uniform parts\n"
                             "in which to split the segments in the selection."));

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  Scale  */
  editor->split_parts = 2;
  scale_data = gtk_adjustment_new (2.0, 2.0, 21.0, 1.0, 1.0, 1.0);

  scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (scale), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 4);
  gtk_widget_show (scale);

  g_signal_connect (scale_data, "value_changed",
		    G_CALLBACK (gimp_int_adjustment_update),
		    &editor->split_parts);

  /*  Show!  */
  gtk_widget_show (dialog);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), FALSE);
}

void
gradient_editor_delete_cmd_callback (GtkWidget *widget,
                                     gpointer   data,
                                     guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg, *seg, *aseg, *next;
  gdouble              join;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  /* Remember segments to the left and to the right of the selection */

  lseg = editor->control_sel_l->prev;
  rseg = editor->control_sel_r->next;

  /* Cannot delete all the segments in the gradient */

  if ((lseg == NULL) && (rseg == NULL))
    return;

  /* Calculate join point */

  join = (editor->control_sel_l->left +
	  editor->control_sel_r->right) / 2.0;

  if (lseg == NULL)
    join = 0.0;
  else if (rseg == NULL)
    join = 1.0;

  /* Move segments */

  if (lseg != NULL)
    gimp_gradient_segments_compress_range (lseg, lseg, lseg->left, join);

  if (rseg != NULL)
    gimp_gradient_segments_compress_range (rseg, rseg, join, rseg->right);

  /* Link */

  if (lseg)
    lseg->next = rseg;

  if (rseg)
    rseg->prev = lseg;

  /* Delete old segments */

  seg = editor->control_sel_l;

  do
    {
      next = seg->next;
      aseg = seg;

      gimp_gradient_segment_free (seg);

      seg = next;
    }
  while (aseg != editor->control_sel_r);

  /* Change selection */

  if (rseg)
    {
      editor->control_sel_l = rseg;
      editor->control_sel_r = rseg;
    }
  else
    {
      editor->control_sel_l = lseg;
      editor->control_sel_r = lseg;
    }

  if (lseg == NULL)
    gradient->segments = rseg;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));
  gimp_gradient_editor_update (editor);
}

void
gradient_editor_recenter_cmd_callback (GtkWidget *widget,
                                       gpointer   data,
                                       guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  seg = editor->control_sel_l;

  do
    {
      seg->middle = (seg->left + seg->right) / 2.0;

      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != editor->control_sel_r);

  gimp_data_dirty (GIMP_DATA (gradient));
  gimp_gradient_editor_update (editor);
}

void
gradient_editor_redistribute_cmd_callback (GtkWidget *widget,
                                           gpointer   data,
                                           guint      action)
{
  GimpGradientEditor  *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;
  gdouble              left, right, seg_len;
  gint                 num_segs;
  gint                 i;

  editor = GIMP_GRADIENT_EDITOR (data);

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  /* Count number of segments in selection */

  num_segs = 0;
  seg      = editor->control_sel_l;

  do
    {
      num_segs++;
      aseg = seg;
      seg  = seg->next;
    }
  while (aseg != editor->control_sel_r);

  /* Calculate new segment length */

  left    = editor->control_sel_l->left;
  right   = editor->control_sel_r->right;
  seg_len = (right - left) / num_segs;

  /* Redistribute */

  seg = editor->control_sel_l;

  for (i = 0; i < num_segs; i++)
    {
      seg->left   = left + i * seg_len;
      seg->right  = left + (i + 1) * seg_len;
      seg->middle = (seg->left + seg->right) / 2.0;

      seg = seg->next;
    }

  /* Fix endpoints to squish accumulative error */

  editor->control_sel_l->left  = left;
  editor->control_sel_r->right = right;

  /* Done */

  gimp_data_dirty (GIMP_DATA (gradient));
  gimp_gradient_editor_update (editor);
}

void
gradient_editor_blend_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data,
                                          guint      action)
{
  GimpGradientEditor *editor;

  editor = GIMP_GRADIENT_EDITOR (data);

  gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                          editor->control_sel_r,
                                          &editor->control_sel_l->left_color,
                                          &editor->control_sel_r->right_color,
                                          TRUE, FALSE);

  gimp_data_dirty (GIMP_DATA_EDITOR (editor)->data);
}

void
gradient_editor_blend_opacity_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor *editor;

  editor = GIMP_GRADIENT_EDITOR (data);

  gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                          editor->control_sel_r,
                                          &editor->control_sel_l->left_color,
                                          &editor->control_sel_r->right_color,
                                          FALSE, TRUE);

  gimp_data_dirty (GIMP_DATA_EDITOR (editor)->data);
}


/*  private functions  */

static void
gradient_editor_left_color_changed (ColorNotebook      *cnb,
                                    const GimpRGB      *color,
                                    ColorNotebookState  state,
                                    gpointer            data)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = (GimpGradientEditor *) data;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              (GimpRGB *) color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_OK:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              (GimpRGB *) color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      gimp_gradient_segments_free (editor->left_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      editor->color_notebook = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;

    case COLOR_NOTEBOOK_CANCEL:
      gradient_editor_replace_selection (editor, editor->left_saved_segments);
      GIMP_DATA (gradient)->dirty = editor->left_saved_dirty;
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gradient));
      color_notebook_free (cnb);
      editor->color_notebook = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;
    }
}

static void
gradient_editor_right_color_changed (ColorNotebook      *cnb,
                                     const GimpRGB      *color,
                                     ColorNotebookState  state,
                                     gpointer            data)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = (GimpGradientEditor *) data;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  switch (state)
    {
    case COLOR_NOTEBOOK_UPDATE:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_r->left_color,
                                              (GimpRGB *) color,
                                              TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_OK:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              &editor->control_sel_r->left_color,
                                              (GimpRGB *) color,
                                              TRUE, TRUE);
      gimp_gradient_segments_free (editor->right_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      editor->color_notebook = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;

    case COLOR_NOTEBOOK_CANCEL:
      gradient_editor_replace_selection (editor, editor->right_saved_segments);
      GIMP_DATA (gradient)->dirty = editor->right_saved_dirty;
      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (gradient));
      color_notebook_free (cnb);
      editor->color_notebook = NULL;
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;
    }
}

static GimpGradientSegment *
gradient_editor_save_selection (GimpGradientEditor *editor)
{
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *oseg, *oaseg;

  prev = NULL;
  oseg = editor->control_sel_l;
  tmp  = NULL;

  do
    {
      seg = gimp_gradient_segment_new ();

      *seg = *oseg; /* Copy everything */

      if (prev == NULL)
	tmp = seg; /* Remember first segment */
      else
	prev->next = seg;

      seg->prev = prev;
      seg->next = NULL;

      prev  = seg;
      oaseg = oseg;
      oseg  = oseg->next;
    }
  while (oaseg != editor->control_sel_r);

  return tmp;
}

static void
gradient_editor_replace_selection (GimpGradientEditor      *editor,
                                   GimpGradientSegment *replace_seg)
{
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg;
  GimpGradientSegment *replace_last;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  /* Remember left and right segments */

  lseg = editor->control_sel_l->prev;
  rseg = editor->control_sel_r->next;

  replace_last = gimp_gradient_segment_get_last (replace_seg);

  /* Free old selection */

  editor->control_sel_r->next = NULL;

  gimp_gradient_segments_free (editor->control_sel_l);

  /* Link in new segments */

  if (lseg)
    lseg->next = replace_seg;
  else
    gradient->segments = replace_seg;

  replace_seg->prev = lseg;

  if (rseg)
    rseg->prev = replace_last;

  replace_last->next = rseg;

  editor->control_sel_l = replace_seg;
  editor->control_sel_r = replace_last;

  gradient->last_visited = NULL; /* Force re-search */
}

static void
gradient_editor_split_uniform_response (GtkWidget          *widget,
                                        gint                response_id,
                                        GimpGradientEditor *editor)
{
  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpGradient        *gradient;
      GimpGradientSegment *seg, *aseg, *lseg, *rseg, *lsel;

      gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

      seg  = editor->control_sel_l;
      lsel = NULL;

      do
        {
          aseg = seg;

          gimp_gradient_segment_split_uniform (gradient, seg,
                                               editor->split_parts,
                                               &lseg, &rseg);

          if (seg == editor->control_sel_l)
            lsel = lseg;

          seg = rseg->next;
        }
      while (aseg != editor->control_sel_r);

      editor->control_sel_l = lsel;
      editor->control_sel_r = rseg;

      gimp_data_dirty (GIMP_DATA (gradient));
      gimp_gradient_editor_update (editor);
    }
}

static void
gradient_editor_replicate_response (GtkWidget          *widget,
                                    gint                response_id,
                                    GimpGradientEditor *editor)
{
  gtk_widget_destroy (widget);
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  if (response_id == GTK_RESPONSE_OK)
    {
      GimpGradient        *gradient;
      gdouble              sel_left, sel_right, sel_len;
      gdouble              new_left;
      gdouble              factor;
      GimpGradientSegment *prev, *seg, *tmp;
      GimpGradientSegment *oseg, *oaseg;
      GimpGradientSegment *lseg, *rseg;
      gint                 i;

      gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

      /* Remember original parameters */
      sel_left  = editor->control_sel_l->left;
      sel_right = editor->control_sel_r->right;
      sel_len   = sel_right - sel_left;

      factor = 1.0 / editor->replicate_times;

      /* Build replicated segments */

      prev = NULL;
      seg  = NULL;
      tmp  = NULL;

      for (i = 0; i < editor->replicate_times; i++)
        {
          /* Build one cycle */

          new_left  = sel_left + i * factor * sel_len;

          oseg = editor->control_sel_l;

          do
            {
              seg = gimp_gradient_segment_new ();

              if (prev == NULL)
                {
                  seg->left = sel_left;
                  tmp = seg; /* Remember first segment */
                }
              else
                {
                  seg->left = new_left + factor * (oseg->left - sel_left);
                }

              seg->middle = new_left + factor * (oseg->middle - sel_left);
              seg->right  = new_left + factor * (oseg->right - sel_left);

              seg->left_color  = oseg->left_color;
              seg->right_color = oseg->right_color;

              seg->type  = oseg->type;
              seg->color = oseg->color;

              seg->prev = prev;
              seg->next = NULL;

              if (prev)
                prev->next = seg;

              prev = seg;

              oaseg = oseg;
              oseg  = oseg->next;
            }
          while (oaseg != editor->control_sel_r);
        }

      seg->right = sel_right; /* Squish accumulative error */

      /* Free old segments */

      lseg = editor->control_sel_l->prev;
      rseg = editor->control_sel_r->next;

      oseg = editor->control_sel_l;

      do
        {
          oaseg = oseg->next;
          gimp_gradient_segment_free (oseg);
          oseg = oaseg;
        }
      while (oaseg != rseg);

      /* Link in new segments */

      if (lseg)
        lseg->next = tmp;
      else
        gradient->segments = tmp;

      tmp->prev = lseg;

      seg->next = rseg;

      if (rseg)
        rseg->prev = seg;

      /* Reset selection */

      editor->control_sel_l = tmp;
      editor->control_sel_r = seg;

      /* Done */

      gimp_data_dirty (GIMP_DATA (gradient));
      gimp_gradient_editor_update (editor);
    }
}
