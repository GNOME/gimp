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

#include <gegl.h>
#include <gtk/gtk.h>

#include "display-types.h"

#include "config/gimpdisplayconfig.h"

#include "core/gimp.h"
#include "core/gimp-cairo.h"
#include "core/gimpboundary.h"
#include "core/gimpchannel.h"
#include "core/gimpimage.h"

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

  GimpSegment      *segs_in;          /*  segments of area boundary         */
  gint              n_segs_in;        /*  number of segments in segs_in     */

  GimpSegment      *segs_out;         /*  segments of area boundary         */
  gint              n_segs_out;       /*  number of segments in segs_out    */

  guint             index;            /*  index of current stipple pattern  */
  gint              paused;           /*  count of pause requests           */
  gboolean          shell_visible;    /*  visility of the display shell     */
  gboolean          show_selection;   /*  is the selection visible?         */
  guint             timeout;          /*  timer for successive draws        */
  cairo_pattern_t  *segs_in_mask;     /*  cache for rendered segments       */
};


/*  local function prototypes  */

static void      selection_start          (Selection          *selection);
static void      selection_stop           (Selection          *selection);

static void      selection_undraw         (Selection          *selection);

static void      selection_render_mask    (Selection          *selection);

static void      selection_zoom_segs      (Selection          *selection,
                                           const GimpBoundSeg *src_segs,
                                           GimpSegment        *dest_segs,
                                           gint                n_segs,
                                           gint                canvas_offset_x,
                                           gint                canvas_offset_y);
static void      selection_generate_segs  (Selection          *selection);
static void      selection_free_segs      (Selection          *selection);

static gboolean  selection_timeout        (Selection          *selection);

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

  selection->shell          = shell;
  selection->shell_visible  = TRUE;
  selection->show_selection = gimp_display_shell_get_show_selection (shell);

  shell->selection = selection;
  shell->selection_update = g_get_monotonic_time ();

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
  Selection *selection;

  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  selection = shell->selection;

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

void
gimp_display_shell_selection_undraw (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  if (gimp_display_get_image (shell->display))
    {
      selection_undraw (shell->selection);
    }
  else
    {
      selection_stop (shell->selection);
      selection_free_segs (shell->selection);
    }
}

void
gimp_display_shell_selection_restart (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  if (gimp_display_get_image (shell->display))
    {
      selection_start (shell->selection);
    }
}

void
gimp_display_shell_selection_pause (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  if (gimp_display_get_image (shell->display))
    {
      if (shell->selection->paused == 0)
        selection_stop (shell->selection);

      shell->selection->paused++;
    }
}

void
gimp_display_shell_selection_resume (GimpDisplayShell *shell)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  if (gimp_display_get_image (shell->display))
    {
      shell->selection->paused--;

      if (shell->selection->paused == 0)
        selection_start (shell->selection);
    }
}

void
gimp_display_shell_selection_set_show (GimpDisplayShell *shell,
                                       gboolean          show)
{
  g_return_if_fail (GIMP_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (shell->selection != NULL);

  if (gimp_display_get_image (shell->display))
    {
      Selection *selection = shell->selection;

      if (show != selection->show_selection)
        {
          selection_undraw (selection);

          selection->show_selection = show;

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
  if (selection->paused == 0                             &&
      gimp_display_get_image (selection->shell->display) &&
      selection->show_selection)
    {
      /*  Draw the ants once  */
      selection_timeout (selection);

      if (selection->segs_in && selection->shell_visible)
        {
          GimpDisplayConfig *config = selection->shell->display->config;

          selection->timeout = g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
                                                   config->marching_ants_speed,
                                                   (GSourceFunc) selection_timeout,
                                                   selection, NULL);
        }
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

void
gimp_display_shell_selection_draw (GimpDisplayShell *shell,
                                   cairo_t          *cr)
{
  if (gimp_display_get_image (shell->display) &&
      shell->selection && shell->selection->show_selection)
    {
      GimpDisplayConfig *config = shell->display->config;
      gint64             time   = g_get_monotonic_time ();

      if ((time - shell->selection_update) / 1000 > config->marching_ants_speed &&
          shell->selection->paused == 0)
        {
          shell->selection_update = time;
          shell->selection->index++;
        }

      selection_generate_segs (shell->selection);

      if (shell->selection->segs_in)
        {
          gimp_display_shell_draw_selection_in (shell->selection->shell, cr,
                                                shell->selection->segs_in_mask,
                                                shell->selection->index % 8);
        }

      if (shell->selection->segs_out)
        {
          if (shell->selection->shell->rotate_transform)
            cairo_transform (cr, shell->selection->shell->rotate_transform);

          gimp_display_shell_draw_selection_out (shell->selection->shell, cr,
                                                 shell->selection->segs_out,
                                                 shell->selection->n_segs_out);
        }
    }
}

static void
selection_undraw (Selection *selection)
{
  gint x, y, w, h;

  selection_stop (selection);

  if (gimp_display_shell_mask_bounds (selection->shell, &x, &y, &w, &h))
    {
      /* expose will restart the selection */
      gimp_display_shell_expose_area (selection->shell, x, y, w, h);
    }
  else
    {
      selection_start (selection);
    }
}

static void
selection_render_mask (Selection *selection)
{
  GdkWindow       *window;
  cairo_surface_t *surface;
  cairo_t         *cr;

  window = gtk_widget_get_window (GTK_WIDGET (selection->shell));
  surface = gdk_window_create_similar_surface (window, CAIRO_CONTENT_ALPHA,
                                               gdk_window_get_width  (window),
                                               gdk_window_get_height (window));
  cr = cairo_create (surface);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (cr, 1.0);

  if (selection->shell->rotate_transform)
    cairo_transform (cr, selection->shell->rotate_transform);

  gimp_cairo_segments (cr,
                       selection->segs_in,
                       selection->n_segs_in);
  cairo_stroke (cr);

  selection->segs_in_mask = cairo_pattern_create_for_surface (surface);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
}

static void
selection_zoom_segs (Selection          *selection,
                     const GimpBoundSeg *src_segs,
                     GimpSegment        *dest_segs,
                     gint                n_segs,
                     gint                canvas_offset_x,
                     gint                canvas_offset_y)
{
  const gint xclamp = selection->shell->disp_width + 1;
  const gint yclamp = selection->shell->disp_height + 1;
  gint       i;

  gimp_display_shell_zoom_segments (selection->shell,
                                    src_segs, dest_segs, n_segs,
                                    0.0, 0.0);

  for (i = 0; i < n_segs; i++)
    {
      if (! selection->shell->rotate_transform)
        {
          dest_segs[i].x1 = CLAMP (dest_segs[i].x1, -1, xclamp) + canvas_offset_x;
          dest_segs[i].y1 = CLAMP (dest_segs[i].y1, -1, yclamp) + canvas_offset_y;

          dest_segs[i].x2 = CLAMP (dest_segs[i].x2, -1, xclamp) + canvas_offset_x;
          dest_segs[i].y2 = CLAMP (dest_segs[i].y2, -1, yclamp) + canvas_offset_y;
        }

      /*  If this segment is a closing segment && the segments lie inside
       *  the region, OR if this is an opening segment and the segments
       *  lie outside the region...
       *  we need to transform it by one display pixel
       */
      if (! src_segs[i].open)
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
  GimpImage          *image = gimp_display_get_image (selection->shell->display);
  const GimpBoundSeg *segs_in;
  const GimpBoundSeg *segs_out;
  gint                canvas_offset_x = 0;
  gint                canvas_offset_y = 0;

  selection_free_segs (selection);

  /*  Ask the image for the boundary of its selected region...
   *  Then transform that information into a new buffer of GimpSegments
   */
  gimp_channel_boundary (gimp_image_get_mask (image),
                         &segs_in, &segs_out,
                         &selection->n_segs_in, &selection->n_segs_out,
                         0, 0, 0, 0);

  if (selection->n_segs_in)
    {
      selection->segs_in = g_new (GimpSegment, selection->n_segs_in);
      selection_zoom_segs (selection, segs_in,
                           selection->segs_in, selection->n_segs_in,
                           canvas_offset_x, canvas_offset_y);

      selection_render_mask (selection);
    }

  /*  Possible secondary boundary representation  */
  if (selection->n_segs_out)
    {
      selection->segs_out = g_new (GimpSegment, selection->n_segs_out);
      selection_zoom_segs (selection, segs_out,
                           selection->segs_out, selection->n_segs_out,
                           canvas_offset_x, canvas_offset_y);
    }
}

static void
selection_free_segs (Selection *selection)
{
  g_clear_pointer (&selection->segs_in, g_free);
  selection->n_segs_in = 0;

  g_clear_pointer (&selection->segs_out, g_free);
  selection->n_segs_out = 0;

  g_clear_pointer (&selection->segs_in_mask, cairo_pattern_destroy);
}

static gboolean
selection_timeout (Selection *selection)
{
  GimpDisplayConfig *config = selection->shell->display->config;
  gint64             time   = g_get_monotonic_time ();

  if ((time - selection->shell->selection_update) / 1000 > config->marching_ants_speed)
    {
      GdkWindow             *window;
      cairo_rectangle_int_t  rect;
      cairo_region_t        *region;

      window = gtk_widget_get_window (GTK_WIDGET (selection->shell));

      rect.x      = 0;
      rect.y      = 0;
      rect.width  = gdk_window_get_width  (window);
      rect.height = gdk_window_get_height (window);

      region = cairo_region_create_rectangle (&rect);
      gtk_widget_queue_draw_region (GTK_WIDGET (selection->shell), region);
      cairo_region_destroy (region);
    }

  return G_SOURCE_CONTINUE;
}

static void
selection_set_shell_visible (Selection *selection,
                             gboolean   shell_visible)
{
  if (selection->shell_visible != shell_visible)
    {
      selection->shell_visible = shell_visible;

      if (shell_visible)
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
  selection_set_shell_visible (selection,
                               (event->new_window_state & (GDK_WINDOW_STATE_WITHDRAWN |
                                                           GDK_WINDOW_STATE_ICONIFIED)) == 0);

  return FALSE;
}

static gboolean
selection_visibility_notify_event (GtkWidget          *shell,
                                   GdkEventVisibility *event,
                                   Selection          *selection)
{
  selection_set_shell_visible (selection,
                               event->state != GDK_VISIBILITY_FULLY_OBSCURED);

  return FALSE;
}
