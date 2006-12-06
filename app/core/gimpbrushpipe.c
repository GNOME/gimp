/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbrushpipe.h"
#include "gimpbrushpipe-load.h"


static void        gimp_brush_pipe_finalize         (GObject      *object);

static gint64      gimp_brush_pipe_get_memsize      (GimpObject   *object,
                                                     gint64       *gui_size);

static gboolean    gimp_brush_pipe_get_popup_size   (GimpViewable *viewable,
                                                     gint          width,
                                                     gint          height,
                                                     gboolean      dot_for_dot,
                                                     gint         *popup_width,
                                                     gint         *popup_height);

static GimpBrush * gimp_brush_pipe_select_brush     (GimpBrush    *brush,
                                                     GimpCoords   *last_coords,
                                                     GimpCoords   *cur_coords);
static gboolean    gimp_brush_pipe_want_null_motion (GimpBrush    *brush,
                                                     GimpCoords   *last_coords,
                                                     GimpCoords   *cur_coords);


G_DEFINE_TYPE (GimpBrushPipe, gimp_brush_pipe, GIMP_TYPE_BRUSH);

#define parent_class gimp_brush_pipe_parent_class


static void
gimp_brush_pipe_class_init (GimpBrushPipeClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpBrushClass    *brush_class       = GIMP_BRUSH_CLASS (klass);

  object_class->finalize         = gimp_brush_pipe_finalize;

  gimp_object_class->get_memsize = gimp_brush_pipe_get_memsize;

  viewable_class->get_popup_size = gimp_brush_pipe_get_popup_size;

  brush_class->select_brush      = gimp_brush_pipe_select_brush;
  brush_class->want_null_motion  = gimp_brush_pipe_want_null_motion;
}

static void
gimp_brush_pipe_init (GimpBrushPipe *pipe)
{
  pipe->current   = NULL;
  pipe->dimension = 0;
  pipe->rank      = NULL;
  pipe->stride    = NULL;
  pipe->nbrushes  = 0;
  pipe->brushes   = NULL;
  pipe->select    = NULL;
  pipe->index     = NULL;
}

static void
gimp_brush_pipe_finalize (GObject *object)
{
  GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (object);

  if (pipe->rank)
    {
      g_free (pipe->rank);
      pipe->rank = NULL;
    }
  if (pipe->stride)
    {
      g_free (pipe->stride);
      pipe->stride = NULL;
    }

  if (pipe->brushes)
    {
      gint i;

      for (i = 0; i < pipe->nbrushes; i++)
        if (pipe->brushes[i])
          g_object_unref (pipe->brushes[i]);

      g_free (pipe->brushes);
      pipe->brushes = NULL;
    }

  if (pipe->select)
    {
      g_free (pipe->select);
      pipe->select = NULL;
    }
  if (pipe->index)
    {
      g_free (pipe->index);
      pipe->index = NULL;
    }

  GIMP_BRUSH (pipe)->mask   = NULL;
  GIMP_BRUSH (pipe)->pixmap = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_brush_pipe_get_memsize (GimpObject *object,
                             gint64     *gui_size)
{
  GimpBrushPipe *pipe    = GIMP_BRUSH_PIPE (object);
  gint64         memsize = 0;
  gint           i;

  memsize += pipe->dimension * (sizeof (gint) /* rank   */ +
                                sizeof (gint) /* stride */ +
                                sizeof (PipeSelectModes));

  for (i = 0; i < pipe->nbrushes; i++)
    memsize += gimp_object_get_memsize (GIMP_OBJECT (pipe->brushes[i]),
                                        gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_brush_pipe_get_popup_size (GimpViewable *viewable,
                                gint          width,
                                gint          height,
                                gboolean      dot_for_dot,
                                gint         *popup_width,
                                gint         *popup_height)
{
  return gimp_viewable_get_size (viewable, popup_width, popup_height);
}

static GimpBrush *
gimp_brush_pipe_select_brush (GimpBrush  *brush,
                              GimpCoords *last_coords,
                              GimpCoords *cur_coords)
{
  GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (brush);
  gint           i, brushix, ix;
  gdouble        angle, velocity, spacing;

  if (pipe->nbrushes == 1)
    return GIMP_BRUSH (pipe->current);


  /* calculates brush native spacing in pixels, based on it's width)*/
  spacing = ((gdouble) gimp_brush_get_spacing (pipe->current) / 100) *
             MAX (brush->mask->width, brush->mask->height);

  brushix = 0;
  for (i = 0; i < pipe->dimension; i++)
    {
      switch (pipe->select[i])
        {
        case PIPE_SELECT_INCREMENTAL:
          ix = (pipe->index[i] + 1) % pipe->rank[i];
          break;

        case PIPE_SELECT_ANGULAR:
          angle = atan2 (cur_coords->y - last_coords->y,
                         cur_coords->x - last_coords->x);
          /* Offset angle to be compatible with PSP tubes */
          angle += G_PI_2;
          /* Map it to the [0..2*G_PI) interval */
          if (angle < 0)
            angle += 2.0 * G_PI;
          else if (angle > 2.0 * G_PI)
            angle -= 2.0 * G_PI;
          ix = RINT (angle / (2.0 * G_PI) * pipe->rank[i]);
          break;

        case PIPE_SELECT_VELOCITY:
          velocity = sqrt (SQR (cur_coords->x - last_coords->x) +
                           SQR (cur_coords->y - last_coords->y));

          /* I don't know how much velocity is enough velocity. I will assume 0  to
           brush' saved spacing (converted to pixels) to be 'enough' velocity */
          ix = ROUND ((1.0 - MIN (1.0, velocity / (spacing))) * pipe->rank[i]);
          break;

        case PIPE_SELECT_RANDOM:
          /* This probably isn't the right way */
          ix = g_random_int_range (0, pipe->rank[i]);
          break;

        case PIPE_SELECT_PRESSURE:
          ix = RINT (cur_coords->pressure * (pipe->rank[i] - 1));
          break;

        case PIPE_SELECT_TILT_X:
          ix = RINT (cur_coords->xtilt / 2.0 * pipe->rank[i]) + pipe->rank[i] / 2;
          break;

        case PIPE_SELECT_TILT_Y:
          ix = RINT (cur_coords->ytilt / 2.0 * pipe->rank[i]) + pipe->rank[i] / 2;
          break;

        case PIPE_SELECT_CONSTANT:
        default:
          ix = pipe->index[i];
          break;
        }

      pipe->index[i] = CLAMP (ix, 0, pipe->rank[i] - 1);
      brushix += pipe->stride[i] * pipe->index[i];
    }

  /* Make sure is inside bounds */
  brushix = CLAMP (brushix, 0, pipe->nbrushes - 1);

  pipe->current = pipe->brushes[brushix];

  return GIMP_BRUSH (pipe->current);
}

static gboolean
gimp_brush_pipe_want_null_motion (GimpBrush  *brush,
                                  GimpCoords *last_coords,
                                  GimpCoords *cur_coords)
{
  GimpBrushPipe *pipe = GIMP_BRUSH_PIPE (brush);
  gint           i;

  if (pipe->nbrushes == 1)
    return TRUE;

  for (i = 0; i < pipe->dimension; i++)
    if (pipe->select[i] == PIPE_SELECT_ANGULAR)
      return FALSE;

  return TRUE;
}
