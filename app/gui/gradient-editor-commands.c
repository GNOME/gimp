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
#include "core/gimpgradient.h"

#include "widgets/gimpgradienteditor.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpwidgets-utils.h"

#include "color-notebook.h"
#include "gradient-editor-commands.h"

#include "libgimp/gimpintl.h"


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
              gradient_editor_save_selection         (GimpGradientEditor    *editor);
static void   gradient_editor_replace_selection      (GimpGradientEditor    *editor,
                                                      GimpGradientSegment *replace_seg);

static void   gradient_editor_dialog_cancel_callback (GtkWidget         *widget,
                                                      GimpGradientEditor    *editor);
static void   gradient_editor_split_uniform_callback (GtkWidget         *widget,
                                                      GimpGradientEditor    *editor);
static void   gradient_editor_replicate_callback     (GtkWidget         *widget,
                                                      GimpGradientEditor    *editor);


/*  public functionss */

void
gradient_editor_left_color_cmd_callback (GtkWidget *widget,
                                         gpointer   data,
                                         guint      action)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->left_saved_dirty    = GIMP_DATA (gradient)->dirty;
  editor->left_saved_segments = gradient_editor_save_selection (editor);

  color_notebook_new (_("Left Endpoint Color"),
		      &editor->control_sel_l->left_color,
		      gradient_editor_left_color_changed,
		      editor,
		      editor->instant_update,
		      TRUE);

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

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

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

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_save_left_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpGradientEditor *editor;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  editor->saved_colors[action] = editor->control_sel_l->left_color;
}

void
gradient_editor_right_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data,
                                          guint      action)
{
  GimpGradientEditor *editor;
  GimpGradient       *gradient;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  editor->right_saved_dirty    = GIMP_DATA (gradient)->dirty;
  editor->right_saved_segments = gradient_editor_save_selection (editor);

  color_notebook_new (_("Right Endpoint Color"),
		      &editor->control_sel_l->right_color,
		      gradient_editor_right_color_changed,
		      editor,
		      editor->instant_update,
		      TRUE);

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

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

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

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_save_right_cmd_callback (GtkWidget *widget,
                                         gpointer   data,
                                         guint      action)
{
  GimpGradientEditor *editor;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  editor->saved_colors[action] = editor->control_sel_l->left_color;
}

void
gradient_editor_blending_func_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor          *editor;
  GimpGradient            *gradient;
  GimpGradientSegmentType  type;
  GimpGradientSegment     *seg, *aseg;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_coloring_type_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor           *editor;
  GimpGradient             *gradient;
  GimpGradientSegmentColor  color;
  GimpGradientSegment      *seg, *aseg;

  if (! GTK_CHECK_MENU_ITEM (widget)->active)
    return;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_flip_cmd_callback (GtkWidget *widget,
                                   gpointer   data,
                                   guint      action)
{
  GimpGradientEditor      *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *oseg, *oaseg;
  GimpGradientSegment *seg, *prev, *tmp;
  GimpGradientSegment *lseg, *rseg;
  gdouble              left, right;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
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

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  dialog =
    gimp_dialog_new ((editor->control_sel_l == editor->control_sel_r) ?
		     _("Replicate segment") :
		     _("Replicate selection"),
		     "gradient_segment_replicate",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/replicate_segment.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, gradient_editor_dialog_cancel_callback,
		     editor, NULL, NULL, TRUE, TRUE,

		     _("Replicate"), gradient_editor_replicate_callback,
		     editor, NULL, NULL, FALSE, FALSE,

		     NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  label = gtk_label_new (_("Select the number of times"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = gtk_label_new ((editor->control_sel_l == editor->control_sel_r) ?
			 _("to replicate the selected segment") :
			 _("to replicate the selection"));
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

  g_signal_connect (G_OBJECT (scale_data), "value_changed",
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
  GimpGradientEditor      *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *lseg, *rseg;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
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

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  /*  Create dialog window  */
  dialog =
    gimp_dialog_new ((editor->control_sel_l == editor->control_sel_r) ?
		     _("Split Segment Uniformly") :
		     _("Split Segments Uniformly"),
		     "gradient_segment_split_uniformly",
		     gimp_standard_help_func,
		     "dialogs/gradient_editor/split_segments_uniformly.html",
		     GTK_WIN_POS_MOUSE,
		     FALSE, TRUE, FALSE,

		     GTK_STOCK_CANCEL, gradient_editor_dialog_cancel_callback,
		     editor, NULL, NULL, FALSE, TRUE,

		     _("Split"), gradient_editor_split_uniform_callback,
		     editor, NULL, NULL, TRUE, FALSE,

		     NULL);

  /*  The main vbox  */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), vbox);
  gtk_widget_show (vbox);

  /*  Instructions  */
  label = gtk_label_new (_("Please select the number of uniform parts"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label =
    gtk_label_new ((editor->control_sel_l == editor->control_sel_r) ?
		   _("in which to split the selected segment") :
		   _("in which to split the segments in the selection"));
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

  g_signal_connect (G_OBJECT (scale_data), "value_changed",
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
  GimpGradientEditor      *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *lseg, *rseg, *seg, *aseg, *next;
  gdouble              join;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

void
gradient_editor_recenter_cmd_callback (GtkWidget *widget,
                                       gpointer   data,
                                       guint      action)
{
  GimpGradientEditor      *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

void
gradient_editor_redistribute_cmd_callback (GtkWidget *widget,
                                           gpointer   data,
                                           guint      action)
{
  GimpGradientEditor      *editor;
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg;
  gdouble              left, right, seg_len;
  gint                 num_segs;
  gint                 i;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

void
gradient_editor_blend_color_cmd_callback (GtkWidget *widget,
                                          gpointer   data,
                                          guint      action)
{
  GimpGradientEditor *editor;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                          editor->control_sel_r,
                                          &editor->control_sel_l->left_color,
                                          &editor->control_sel_r->right_color,
                                          TRUE, FALSE);

  gimp_data_dirty (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_blend_opacity_cmd_callback (GtkWidget *widget,
                                            gpointer   data,
                                            guint      action)
{
  GimpGradientEditor *editor;

  editor = (GimpGradientEditor *) gimp_widget_get_callback_context (widget);

  if (! editor)
    return;

  gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                          editor->control_sel_r,
                                          &editor->control_sel_l->left_color,
                                          &editor->control_sel_r->right_color,
                                          FALSE, TRUE);

  gimp_data_dirty (GIMP_DATA_EDITOR (editor)->data);

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
}

void
gradient_editor_menu_update (GtkItemFactory *factory,
                             gpointer        data)
{
  GimpGradientEditor  *editor;
  GimpContext         *user_context;
  GimpGradientSegment *left_seg;
  GimpGradientSegment *right_seg;
  GimpRGB              fg;
  GimpRGB              bg;
  gboolean             blending_equal = TRUE;
  gboolean             coloring_equal = TRUE;
  gboolean             selection;
  gboolean             delete;

  editor = GIMP_GRADIENT_EDITOR (data);

  user_context = gimp_get_user_context (GIMP_DATA_EDITOR (editor)->gimp);

  if (editor->control_sel_l->prev)
    left_seg = editor->control_sel_l->prev;
  else
    left_seg = gimp_gradient_segment_get_last (editor->control_sel_l);

  if (editor->control_sel_r->next)
    right_seg = editor->control_sel_r->next;
  else
    right_seg = gimp_gradient_segment_get_first (editor->control_sel_r);

  gimp_context_get_foreground (user_context, &fg);
  gimp_context_get_background (user_context, &bg);

  {
    GimpGradientSegmentType  type;
    GimpGradientSegmentColor color;
    GimpGradientSegment     *seg, *aseg;

    type  = editor->control_sel_l->type;
    color = editor->control_sel_l->color;

    seg = editor->control_sel_l;

    do
      {
        blending_equal = blending_equal && (seg->type == type);
        coloring_equal = coloring_equal && (seg->color == color);

        aseg = seg;
        seg  = seg->next;
      }
    while (aseg != editor->control_sel_r);
  }

  selection = (editor->control_sel_l != editor->control_sel_r);
  delete    = (editor->control_sel_l->prev || editor->control_sel_r->next);

#define SET_ACTIVE(menu,active) \
        gimp_item_factory_set_active (factory, menu, (active))
#define SET_COLOR(menu,color,set_label) \
        gimp_item_factory_set_color (factory, menu, (color), (set_label))
#define SET_LABEL(menu,label) \
        gimp_item_factory_set_label (factory, menu, (label))
#define SET_SENSITIVE(menu,condition) \
        gimp_item_factory_set_sensitive (factory, menu, (condition) != 0)
#define SET_VISIBLE(menu,condition) \
        gimp_item_factory_set_visible (factory, menu, (condition) != 0)

  SET_COLOR ("/Left Endpoint's Color...",
             &editor->control_sel_l->left_color, FALSE);
  SET_COLOR ("/Load Left Color From/Left Neighbor's Right Endpoint",
             &left_seg->right_color, FALSE);
  SET_COLOR ("/Load Left Color From/Right Endpoint",
             &editor->control_sel_r->right_color, FALSE);
  SET_COLOR ("/Load Left Color From/FG Color", &fg, FALSE);
  SET_COLOR ("/Load Left Color From/BG Color", &bg, FALSE);

  SET_COLOR ("/Load Left Color From/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Load Left Color From/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Load Left Color From/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Load Left Color From/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Load Left Color From/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Load Left Color From/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Load Left Color From/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Load Left Color From/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Load Left Color From/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Load Left Color From/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Save Left Color To/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Save Left Color To/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Save Left Color To/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Save Left Color To/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Save Left Color To/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Save Left Color To/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Save Left Color To/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Save Left Color To/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Save Left Color To/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Save Left Color To/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Right Endpoint's Color...",
             &editor->control_sel_r->right_color, FALSE);
  SET_COLOR ("/Load Right Color From/Right Neighbor's Left Endpoint",
             &right_seg->left_color, FALSE);
  SET_COLOR ("/Load Right Color From/Left Endpoint",
             &editor->control_sel_l->left_color, FALSE);
  SET_COLOR ("/Load Right Color From/FG Color", &fg, FALSE);
  SET_COLOR ("/Load Right Color From/BG Color", &bg, FALSE);

  SET_COLOR ("/Load Right Color From/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Load Right Color From/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Load Right Color From/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Load Right Color From/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Load Right Color From/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Load Right Color From/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Load Right Color From/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Load Right Color From/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Load Right Color From/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Load Right Color From/10", &editor->saved_colors[9], TRUE);

  SET_COLOR ("/Save Right Color To/01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("/Save Right Color To/02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("/Save Right Color To/03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("/Save Right Color To/04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("/Save Right Color To/05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("/Save Right Color To/06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("/Save Right Color To/07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("/Save Right Color To/08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("/Save Right Color To/09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("/Save Right Color To/10", &editor->saved_colors[9], TRUE);

  if (! selection)
    {
      SET_LABEL ("/flip",             _("Flip Segment"));
      SET_LABEL ("/replicate",        _("Replicate Segment"));
      SET_LABEL ("/blendingfunction", _("Blending Function for Segment"));
      SET_LABEL ("/coloringtype",     _("Coloring Type for Segment"));
      SET_LABEL ("/splitmidpoint",    _("Split Segment at Midpoint"));
      SET_LABEL ("/splituniformly",   _("Split Segment Uniformly"));
      SET_LABEL ("/delete",           _("Delete Segment"));
      SET_LABEL ("/recenter",         _("Re-center Segment's Midpoint"));
      SET_LABEL ("/redistribute",     _("Re-distribute Handles in Segment"));
    }
  else
    {
      SET_LABEL ("/flip",             _("Flip Selection"));
      SET_LABEL ("/replicate",        _("Replicate Selection"));
      SET_LABEL ("/blendingfunction", _("Blending Function for Selection"));
      SET_LABEL ("/coloringtype",     _("Coloring Type for Selection"));
      SET_LABEL ("/splitmidpoint",    _("Split Segments at Midpoints"));
      SET_LABEL ("/splituniformly",   _("Split Segments Uniformly"));
      SET_LABEL ("/delete",           _("Delete Selection"));
      SET_LABEL ("/recenter",         _("Re-center Midpoints in Selection"));
      SET_LABEL ("/redistribute",     _("Re-distribute Handles in Selection"));
    }

  SET_SENSITIVE ("/blendingfunction/(Varies)", FALSE);
  SET_SENSITIVE ("/coloringtype/(Varies)",     FALSE);

  if (blending_equal)
    {
      SET_VISIBLE ("/blendingfunction/(Varies)", FALSE);

      switch (editor->control_sel_l->type)
        {
        case GIMP_GRAD_LINEAR:
          SET_ACTIVE ("/blendingfunction/Linear", TRUE);
          break;
        case GIMP_GRAD_CURVED:
          SET_ACTIVE ("/blendingfunction/Curved", TRUE);
          break;
        case GIMP_GRAD_SINE:
          SET_ACTIVE ("/blendingfunction/Sinusodial", TRUE);
          break;
        case GIMP_GRAD_SPHERE_INCREASING:
          SET_ACTIVE ("/blendingfunction/Spherical (increasing)", TRUE);
          break;
        case GIMP_GRAD_SPHERE_DECREASING:
          SET_ACTIVE ("/blendingfunction/Spherical (decreasing)", TRUE);
          break;
        }
    }
  else
    {
      SET_VISIBLE ("/blendingfunction/(Varies)", TRUE);
      SET_ACTIVE ("/blendingfunction/(Varies)", TRUE);
    }

  if (coloring_equal)
    {
      SET_VISIBLE ("/coloringtype/(Varies)", FALSE);

      switch (editor->control_sel_l->color)
        {
        case GIMP_GRAD_RGB:
          SET_ACTIVE ("/coloringtype/RGB", TRUE);
          break;
        case GIMP_GRAD_HSV_CCW:
          SET_ACTIVE ("/coloringtype/HSV (counter-clockwise hue)", TRUE);
          break;
        case GIMP_GRAD_HSV_CW:
          SET_ACTIVE ("/coloringtype/HSV (clockwise hue)", TRUE);
          break;
        }
    }
  else
    {
      SET_VISIBLE ("/coloringtype/(Varies)", TRUE);
      SET_ACTIVE ("/coloringtype/(Varies)", TRUE);
    }

  SET_SENSITIVE ("/Blend Endpoints' Colors",  selection);
  SET_SENSITIVE ("/Blend Endpoints' Opacity", selection);
  SET_SENSITIVE ("/delete", delete);

#undef SET_ACTIVE
#undef SET_COLOR
#undef SET_LABEL
#undef SET_SENSITIVE
#undef SET_VISIBLE
}

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
    case COLOR_NOTEBOOK_OK:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              (GimpRGB *) color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      gimp_gradient_segments_free (editor->left_saved_segments);
      gimp_data_dirty (GIMP_DATA (gradient));
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;

    case COLOR_NOTEBOOK_UPDATE:
      gimp_gradient_segments_blend_endpoints (editor->control_sel_l,
                                              editor->control_sel_r,
                                              (GimpRGB *) color,
                                              &editor->control_sel_r->right_color,
                                              TRUE, TRUE);
      gimp_data_dirty (GIMP_DATA (gradient));
      break;

    case COLOR_NOTEBOOK_CANCEL:
      gradient_editor_replace_selection (editor, editor->left_saved_segments);
      GIMP_DATA (gradient)->dirty = editor->left_saved_dirty;
      gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;
    }

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
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
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;

    case COLOR_NOTEBOOK_CANCEL:
      gradient_editor_replace_selection (editor, editor->right_saved_segments);
      GIMP_DATA (gradient)->dirty = editor->right_saved_dirty;
      color_notebook_free (cnb);
      gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
      break;
    }

  gimp_gradient_editor_update (editor, GRAD_UPDATE_GRADIENT);
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
gradient_editor_dialog_cancel_callback (GtkWidget          *widget,
                                        GimpGradientEditor *editor)
{
  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);
}

static void
gradient_editor_split_uniform_callback (GtkWidget          *widget,
                                        GimpGradientEditor *editor)
{
  GimpGradient        *gradient;
  GimpGradientSegment *seg, *aseg, *lseg, *rseg, *lsel;

  gradient = GIMP_GRADIENT (GIMP_DATA_EDITOR (editor)->data);

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

  seg  = editor->control_sel_l;
  lsel = NULL;

  do
    {
      aseg = seg;

      gimp_gradient_segment_split_uniform (gradient, seg,
                                           editor->split_parts, &lseg, &rseg);

      if (seg == editor->control_sel_l)
	lsel = lseg;

      seg = rseg->next;
    }
  while (aseg != editor->control_sel_r);

  editor->control_sel_l = lsel;
  editor->control_sel_r = rseg;

  gimp_data_dirty (GIMP_DATA (gradient));

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}

static void
gradient_editor_replicate_callback (GtkWidget          *widget,
                                    GimpGradientEditor *editor)
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

  gtk_widget_destroy (gtk_widget_get_toplevel (widget));
  gtk_widget_set_sensitive (GTK_WIDGET (editor), TRUE);

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
	    seg->left = new_left + factor * (oseg->left - sel_left);

	  seg->middle = new_left + factor * (oseg->middle - sel_left);
	  seg->right  = new_left + factor * (oseg->right - sel_left);

	  seg->left_color = oseg->right_color;

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

  gimp_gradient_editor_update (editor,
                               GRAD_UPDATE_GRADIENT | GRAD_UPDATE_CONTROL);
}
