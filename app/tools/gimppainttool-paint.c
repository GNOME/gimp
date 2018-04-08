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

#include "tools-types.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpprojection.h"

#include "paint/gimppaintcore.h"
#include "paint/gimppaintoptions.h"

#include "display/gimpdisplay.h"

#include "gimppainttool.h"
#include "gimppainttool-paint.h"


#define DISPLAY_UPDATE_INTERVAL 10000 /* microseconds */


typedef enum
{
  PAINT_ITEM_TYPE_INTERPOLATE,
  PAINT_ITEM_TYPE_FINISH
} PaintItemType;


typedef struct
{
  PaintItemType      type;

  union
  {
    struct
    {
      GimpPaintTool *paint_tool;
      GimpCoords     coords;
      guint32        time;
    };

    gboolean        *finished;
  };
} PaintItem;


/*  local function prototypes  */

static gboolean   gimp_paint_tool_paint_timeout    (GimpPaintTool *paint_tool);

static gpointer   gimp_paint_tool_paint_thread     (gpointer       data);

static gboolean   gimp_paint_tool_paint_use_thread (GimpPaintTool *paint_tool);


/*  static variables  */

static GThread           *paint_thread;

static GMutex             paint_mutex;
static GCond              paint_cond;

static GQueue             paint_queue = G_QUEUE_INIT;
static GMutex             paint_queue_mutex;
static GCond              paint_queue_cond;

static guint              paint_timeout_id;
static volatile gboolean  paint_timeout_pending;


/*  private functions  */


static gboolean
gimp_paint_tool_paint_timeout (GimpPaintTool *paint_tool)
{
  GimpTool *tool = GIMP_TOOL (paint_tool);
  gboolean  update;

  paint_timeout_pending = TRUE;

  g_mutex_lock (&paint_mutex);

  update = gimp_drawable_flush_paint (tool->drawable);

  paint_timeout_pending = FALSE;
  g_cond_signal (&paint_cond);

  g_mutex_unlock (&paint_mutex);

  if (update)
    {
      GimpDisplay *display = tool->display;
      GimpImage   *image   = gimp_display_get_image (display);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (paint_tool));

      gimp_projection_flush_now (gimp_image_get_projection (image));
      gimp_display_flush_now (display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (paint_tool));
    }

  return G_SOURCE_CONTINUE;
}

static gpointer
gimp_paint_tool_paint_thread (gpointer data)
{
  g_mutex_lock (&paint_queue_mutex);

  while (TRUE)
    {
      PaintItem *item;

      while (! (item = g_queue_pop_head (&paint_queue)))
        g_cond_wait (&paint_queue_cond, &paint_queue_mutex);

      switch (item->type)
        {
        case PAINT_ITEM_TYPE_INTERPOLATE:
          g_mutex_unlock (&paint_queue_mutex);
          g_mutex_lock (&paint_mutex);

          while (paint_timeout_pending)
            g_cond_wait (&paint_cond, &paint_mutex);

          gimp_paint_core_interpolate (
            item->paint_tool->core,
            GIMP_TOOL (item->paint_tool)->drawable,
            GIMP_PAINT_TOOL_GET_OPTIONS (item->paint_tool),
            &item->coords, item->time);

          g_mutex_unlock (&paint_mutex);
          g_mutex_lock (&paint_queue_mutex);

          break;

        case PAINT_ITEM_TYPE_FINISH:
          *item->finished = TRUE;
          g_cond_signal (&paint_queue_cond);

          break;
        }

      g_slice_free (PaintItem, item);
    }

  g_mutex_unlock (&paint_queue_mutex);

  return NULL;
}

static gboolean
gimp_paint_tool_paint_use_thread (GimpPaintTool *paint_tool)
{
  if (GIMP_PAINT_TOOL_GET_CLASS (paint_tool)->use_paint_thread)
    {
      if (! paint_thread)
        {
          static gint use_paint_thread = -1;

          if (use_paint_thread < 0)
            use_paint_thread = g_getenv ("GIMP_NO_PAINT_THREAD") == NULL;

          if (use_paint_thread)
            {
              paint_thread = g_thread_new ("paint",
                                           gimp_paint_tool_paint_thread, NULL);
            }
        }

      return paint_thread != NULL;
    }

  return FALSE;
}


/*  public functions  */


void
gimp_paint_tool_paint_start (GimpPaintTool *paint_tool)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (paint_tool));

  if (gimp_paint_tool_paint_use_thread (paint_tool))
    {
      gimp_drawable_start_paint (GIMP_TOOL (paint_tool)->drawable);

      paint_timeout_id = g_timeout_add_full (
        G_PRIORITY_HIGH_IDLE,
        DISPLAY_UPDATE_INTERVAL / 1000,
        (GSourceFunc) gimp_paint_tool_paint_timeout,
        paint_tool, NULL);
    }
}

void
gimp_paint_tool_paint_end (GimpPaintTool *paint_tool)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (paint_tool));

  if (gimp_paint_tool_paint_use_thread (paint_tool))
    {
      PaintItem *item;
      gboolean   finished = FALSE;
      guint64    end_time;

      g_source_remove (paint_timeout_id);
      paint_timeout_id = 0;

      item = g_slice_new (PaintItem);

      item->type     = PAINT_ITEM_TYPE_FINISH;
      item->finished = &finished;

      g_mutex_lock (&paint_queue_mutex);

      g_queue_push_tail (&paint_queue, item);
      g_cond_signal (&paint_queue_cond);

      end_time = g_get_monotonic_time () + DISPLAY_UPDATE_INTERVAL;

      while (! finished)
        {
          if (! g_cond_wait_until (&paint_queue_cond, &paint_queue_mutex,
                                   end_time))
            {
              g_mutex_unlock (&paint_queue_mutex);

              gimp_paint_tool_paint_timeout (paint_tool);

              g_mutex_lock (&paint_queue_mutex);

              end_time = g_get_monotonic_time () + DISPLAY_UPDATE_INTERVAL;
            }
        }

      g_mutex_unlock (&paint_queue_mutex);

      gimp_drawable_end_paint (GIMP_TOOL (paint_tool)->drawable);
    }
}

void
gimp_paint_tool_paint_interpolate (GimpPaintTool    *paint_tool,
                                   const GimpCoords *coords,
                                   guint32           time)
{
  g_return_if_fail (GIMP_IS_PAINT_TOOL (paint_tool));
  g_return_if_fail (coords != NULL);

  if (gimp_paint_tool_paint_use_thread (paint_tool))
    {
      PaintItem *item;

      item = g_slice_new (PaintItem);

      item->type       = PAINT_ITEM_TYPE_INTERPOLATE;
      item->paint_tool = paint_tool;
      item->coords     = *coords;
      item->time       = time;

      g_mutex_lock (&paint_queue_mutex);

      g_queue_push_tail (&paint_queue, item);
      g_cond_signal (&paint_queue_cond);

      g_mutex_unlock (&paint_queue_mutex);
    }
  else
    {
      GimpTool    *tool    = GIMP_TOOL (paint_tool);
      GimpDisplay *display = tool->display;
      GimpImage   *image   = gimp_display_get_image (display);

      gimp_draw_tool_pause (GIMP_DRAW_TOOL (paint_tool));

      gimp_paint_core_interpolate (paint_tool->core,
                                   tool->drawable,
                                   GIMP_PAINT_TOOL_GET_OPTIONS (paint_tool),
                                   coords, time);

      gimp_projection_flush_now (gimp_image_get_projection (image));
      gimp_display_flush_now (display);

      gimp_draw_tool_resume (GIMP_DRAW_TOOL (paint_tool));
    }
}
