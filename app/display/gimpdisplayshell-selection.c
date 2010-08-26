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

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "base/boundary.h"

#include "core/gimp.h"
#include "core/gimplayer.h"
#include "core/gimplayermask.h"
#include "core/gimpimage.h"

#include "gimpcanvas.h"
#include "gimpdisplay.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-draw.h"
#include "gimpdisplayshell-expose.h"
#include "gimpdisplayshell-selection.h"
#include "gimpdisplayshell-transform.h"


struct _Selection
{
  GimpDisplayShell *shell;            /*  shell that owns the selection     */

  GdkSegment       *segs_in;          /*  gdk segments of area boundary     */
  gint              n_segs_in;        /*  number of segments in segs_in     */

  GdkSegment       *segs_out;         /*  gdk segments of area boundary     */
  gint              n_segs_out;       /*  number of segments in segs_out    */

  GdkSegment       *segs_layer;       /*  segments of layer boundary        */
  gint              n_segs_layer;     /*  number of segments in segs_layer  */

  guint             index;            /*  index of current stipple pattern  */
  gint              paused;           /*  count of pause requests           */
  gboolean          visible;          /*  visility of the display shell     */
  gboolean          hidden;           /*  is the selection hidden?          */
  gboolean          layer_hidden;     /*  is the layer boundary hidden?     */
  guint             timeout;          /*  timer for successive draws        */
  cairo_pattern_t  *segs_in_mask;     /*  cache for rendered segments       */
};


/*  local function prototypes  */

static void      selection_start          (Selection      *selection);
static void      selection_stop           (Selection      *selection);

static void      selection_pause          (Selection      *selection);
static void      selection_resume         (Selection      *selection);

static void      selection_draw           (Selection      *selection);
static void      selection_undraw         (Selection      *selection);
static void      selection_layer_undraw   (Selection      *selection);
static void      selection_layer_draw     (Selection      *selection);

static void      selection_render_mask    (Selection      *selection);

static void      selection_transform_segs (Selection      *selection,
                                           const BoundSeg *src_segs,
                                           GdkSegment     *dest_segs,
                                           gint            n_segs);
static void      selection_generate_segs  (Selection      *selection);
static void      selection_free_segs      (Selection      *selection);

static gboolean  selection_start_timeout  (Selection      *selection);
static gboolean  selection_timeout        (Selection      *selection);

static gboolean  selection_window_state_event      (GtkWidget           *shell,
                                                    GdkEventWindowState *event,
                                                    Selection           *selection);
static gboolean  selection_visibility_notify_event (GtkWidget           *shell,
                                                    GdkEventVisibility  *event,
                                                    Selection           *selection);


/*  public functions  */

void
gimp_display_shell_selection_init (GimpDisplayShell *shell)
{
  Selection *selection;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection == NULL);

  selection = g_slice_new0 (Selection);

  selection->shell        = shell;
  selection->visible      = TRUE;
  selection->hidden       = ! gimp_display_shell_get_show_selection (shell);
  selection->layer_hidden = ! gimp_display_shell_get_show_layer (shell);

  shell->selection = selection;

  g_signal_connect (shell, "window-state-event",
                    G_CALLBACK (selection_window_state_event),
                    selection);
  g_signal_connect (shell, "visibility-notify-event",
                    G_CALLBACK (selection_visibility_notify_event),
                    selection);
}

void
gimp_display_shell_selection_free (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection)
    {
      Selection *selection = shell->selection;

      selection_stop (selection);

      g_signal_handlers_disconnect_by_func (shell,
                                            selection_window_state_event,
                                            selection);
      g_signal_handlers_disconnect_by_func (shell,
                                            selection_visibility_notify_event,
                                            selection);

      selection_free_segs (selection);

      g_slice_free (Selection, selection);

      shell->selection = NULL;
    }
}

void
gimp_display_shell_selection_control (GimpDisplayShell     *shell,
                                      GimpSelectionControl  control)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection && gimp_display_get_image (shell->display))
    {
      Selection *selection = shell->selection;

      switch (control)
        {
        case GIMP_SELECTION_OFF:
          selection_undraw (selection);
          break;

        case GIMP_SELECTION_LAYER_OFF:
          selection_layer_undraw (selection);
          break;

        case GIMP_SELECTION_LAYER_ON:
          if (! selection->layer_hidden)
            selection_layer_draw (selection);
          break;

        case GIMP_SELECTION_ON:
          selection_start (selection);
          break;

        case GIMP_SELECTION_PAUSE:
          selection_pause (selection);
          break;

        case GIMP_SELECTION_RESUME:
          selection_resume (selection);
          break;
        }
    }
  else if (shell->selection)
    {
      selection_stop (shell->selection);
      selection_free_segs (shell->selection);
    }
}

void
gimp_display_shell_selection_set_hidden (GimpDisplayShell *shell,
                                         gboolean          hidden)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection && gimp_display_get_image (shell->display))
    {
      Selection *selection = shell->selection;

      if (hidden != selection->hidden)
        {
          selection_undraw (selection);
          selection_layer_undraw (selection);

          selection->hidden = hidden;

          selection_start (selection);
        }
    }
}

void
gimp_display_shell_selection_set_layer_hidden (GimpDisplayShell *shell,
                                               gboolean          hidden)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));

  if (shell->selection && gimp_display_get_image (shell->display))
    {
      Selection *selection = shell->selection;

      if (hidden != selection->layer_hidden)
        {
          selection_undraw (selection);
          selection_layer_undraw (selection);

          selection->layer_hidden = hidden;

          selection_start (selection);
        }
    }
}


/*  private functions  */

static void
selection_start (Selection *selection)
{
  selection_stop (selection);

  /*  If this selection is paused, do not start it  */
  if (selection->paused == 0)
    {
      selection->timeout = g_idle_add ((GSourceFunc) selection_start_timeout,
                                       selection);
    }
}

static void
selection_stop (Selection *selection)
{
  if (selection->timeout)
    {
      g_source_remove (selection->timeout);
      selection->timeout = 0;
    }
}

static void
selection_pause (Selection *selection)
{
  if (selection->paused == 0)
    selection_stop (selection);

  selection->paused++;
}

static void
selection_resume (Selection *selection)
{
  selection->paused--;

  if (selection->paused == 0)
    selection_start (selection);
}

static void
selection_draw (Selection *selection)
{
  if (selection->segs_in)
    {
      cairo_t *cr;

      cr = gdk_cairo_create (gtk_widget_get_window (selection->shell->canvas));

      gimp_display_shell_draw_selection_in_mask (selection->shell, cr,
                                                 selection->segs_in_mask,
                                                 selection->index % 8);

      cairo_destroy (cr);
    }
}

static void
selection_undraw (Selection *selection)
{
  gint x1, y1, x2, y2;

  selection_stop (selection);

  if (gimp_display_shell_mask_bounds (selection->shell, &x1, &y1, &x2, &y2))
    {
      /* expose will restart the selection */
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y1, (x2 - x1), (y2 - y1));
    }
  else
    {
      selection_start (selection);
    }
}

static void
selection_layer_draw (Selection *selection)
{
  if (selection->segs_layer)
    {
      GimpImage    *image;
      GimpDrawable *drawable;
      cairo_t      *cr;

      image    = gimp_display_get_image (selection->shell->display);
      drawable = gimp_image_get_active_drawable (image);

      cr = gdk_cairo_create (gtk_widget_get_window (selection->shell->canvas));

      gimp_display_shell_draw_layer_boundary (selection->shell, cr, drawable,
                                              selection->segs_layer,
                                              selection->n_segs_layer);

      cairo_destroy (cr);
    }
}

static void
selection_layer_undraw (Selection *selection)
{
  selection_stop (selection);

  if (selection->segs_layer && selection->n_segs_layer == 4)
    {
      gint x1 = selection->segs_layer[0].x1 - 1;
      gint y1 = selection->segs_layer[0].y1 - 1;
      gint x2 = selection->segs_layer[3].x2 + 1;
      gint y2 = selection->segs_layer[3].y2 + 1;

      gint x3 = selection->segs_layer[0].x1 + 1;
      gint y3 = selection->segs_layer[0].y1 + 1;
      gint x4 = selection->segs_layer[3].x2 - 1;
      gint y4 = selection->segs_layer[3].y2 - 1;

      /*  expose the region, this will restart the selection  */
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y1, (x2 - x1) + 1, (y3 - y1) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y3, (x3 - x1) + 1, (y4 - y3) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x1, y4, (x2 - x1) + 1, (y2 - y4) + 1);
      gimp_display_shell_expose_area (selection->shell,
                                      x4, y3, (x2 - x4) + 1, (y4 - y3) + 1);
    }
  else
    {
      selection_start (selection);
    }
}

static void
selection_render_mask (Selection *selection)
{
  cairo_t *cr;

  cr = gdk_cairo_create (gtk_widget_get_window (selection->shell->canvas));

  cairo_push_group_with_content (cr, CAIRO_CONTENT_ALPHA);

  gimp_display_shell_draw_selection_segments (selection->shell, cr,
                                              selection->segs_in,
                                              selection->n_segs_in);

  selection->segs_in_mask = cairo_pop_group (cr);

  cairo_destroy (cr);
}

static void
selection_transform_segs (Selection      *selection,
                          const BoundSeg *src_segs,
                          GdkSegment     *dest_segs,
                          gint            n_segs)
{
  gint xclamp = selection->shell->disp_width + 1;
  gint yclamp = selection->shell->disp_height + 1;
  gint i;

  gimp_display_shell_transform_segments (selection->shell,
                                         src_segs, dest_segs, n_segs, FALSE);

  for (i = 0; i < n_segs; i++)
    {
      dest_segs[i].x1 = CLAMP (dest_segs[i].x1, -1, xclamp);
      dest_segs[i].y1 = CLAMP (dest_segs[i].y1, -1, yclamp);

      dest_segs[i].x2 = CLAMP (dest_segs[i].x2, -1, xclamp);
      dest_segs[i].y2 = CLAMP (dest_segs[i].y2, -1, yclamp);

      /*  If this segment is a closing segment && the segments lie inside
       *  the region, OR if this is an opening segment and the segments
       *  lie outside the region...
       *  we need to transform it by one display pixel
       */
      if (!src_segs[i].open)
        {
          /*  If it is vertical  */
          if (dest_segs[i].x1 == dest_segs[i].x2)
            {
              dest_segs[i].x1 -= 1;
              dest_segs[i].x2 -= 1;
            }
          else
            {
              dest_segs[i].y1 -= 1;
              dest_segs[i].y2 -= 1;
            }
        }
    }
}

static void
selection_generate_segs (Selection *selection)
{
  GimpImage      *image = gimp_display_get_image (selection->shell->display);
  const BoundSeg *segs_in;
  const BoundSeg *segs_out;
  GimpLayer      *layer;

  /*  Ask the image for the boundary of its selected region...
   *  Then transform that information into a new buffer of GdkSegments
   */
  gimp_channel_boundary (gimp_image_get_mask (image),
                         &segs_in, &segs_out,
                         &selection->n_segs_in, &selection->n_segs_out,
                         0, 0, 0, 0);

  if (selection->n_segs_in)
    {
      selection->segs_in = g_new (GdkSegment, selection->n_segs_in);
      selection_transform_segs (selection, segs_in,
                                selection->segs_in, selection->n_segs_in);

      selection_render_mask (selection);
    }
  else
    {
      selection->segs_in = NULL;
    }

  /*  Possible secondary boundary representation  */
  if (selection->n_segs_out)
    {
      selection->segs_out = g_new (GdkSegment, selection->n_segs_out);
      selection_transform_segs (selection, segs_out,
                                selection->segs_out, selection->n_segs_out);
    }
  else
    {
      selection->segs_out = NULL;
    }

  layer = gimp_image_get_active_layer (image);

  if (layer)
    {
      BoundSeg *segs;

      segs = gimp_layer_boundary (layer, &selection->n_segs_layer);

      if (selection->n_segs_layer)
        {
          selection->segs_layer = g_new (GdkSegment, selection->n_segs_layer);

          selection_transform_segs (selection,
                                    segs,
                                    selection->segs_layer,
                                    selection->n_segs_layer);
        }

      g_free (segs);
    }
}

static void
selection_free_segs (Selection *selection)
{
  if (selection->segs_in)
    {
      g_free (selection->segs_in);
      selection->segs_in   = NULL;
      selection->n_segs_in = 0;
    }

  if (selection->segs_out)
    {
      g_free (selection->segs_out);
      selection->segs_out   = NULL;
      selection->n_segs_out = 0;
    }

  if (selection->segs_layer)
    {
      g_free (selection->segs_layer);
      selection->segs_layer   = NULL;
      selection->n_segs_layer = 0;
    }

  if (selection->segs_in_mask)
    {
      cairo_pattern_destroy (selection->segs_in_mask);
      selection->segs_in_mask = NULL;
    }
}

static gboolean
selection_start_timeout (Selection *selection)
{
  selection_free_segs (selection);
  selection->timeout = 0;

  if (! gimp_display_get_image (selection->shell->display))
    return FALSE;

  selection_generate_segs (selection);

  selection->index = 0;

  if (! selection->layer_hidden)
    selection_layer_draw (selection);

  /*  Draw the ants  */
  if (! selection->hidden)
    {
      GimpDisplayConfig *config = selection->shell->display->config;

      selection_draw (selection);

      if (selection->segs_out)
        {
          cairo_t *cr;

          cr = gdk_cairo_create (gtk_widget_get_window (selection->shell->canvas));

          gimp_display_shell_draw_selection_out (selection->shell, cr,
                                                 selection->segs_out,
                                                 selection->n_segs_out);

          cairo_destroy (cr);
        }

      if (selection->segs_in && selection->visible)
        selection->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
                                                 config->marching_ants_speed,
                                                 (GSourceFunc) selection_timeout,
                                                 selection, NULL);
    }

  return FALSE;
}

static gboolean
selection_timeout (Selection *selection)
{
  selection->index++;
  selection_draw (selection);

  return TRUE;
}

static void
selection_set_visible (Selection *selection,
                       gboolean   visible)
{
  if (selection->visible != visible)
    {
      selection->visible = visible;

      if (visible)
        selection_start (selection);
      else
        selection_stop (selection);
    }
}

static gboolean
selection_window_state_event (GtkWidget           *shell,
                              GdkEventWindowState *event,
                              Selection           *selection)
{
  selection_set_visible (selection,
                         (event->new_window_state & (GDK_WINDOW_STATE_WITHDRAWN |
                                                     GDK_WINDOW_STATE_ICONIFIED)) == 0);

  return FALSE;
}

static gboolean
selection_visibility_notify_event (GtkWidget          *shell,
                                   GdkEventVisibility *event,
                                   Selection          *selection)
{
  selection_set_visible (selection,
                         event->state != GDK_VISIBILITY_FULLY_OBSCURED);

  return FALSE;
}
