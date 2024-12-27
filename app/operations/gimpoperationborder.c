/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationborder.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "gimpoperationborder.h"


enum
{
  PROP_0,
  PROP_RADIUS_X,
  PROP_RADIUS_Y,
  PROP_FEATHER,
  PROP_EDGE_LOCK
};


static void     gimp_operation_border_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void     gimp_operation_border_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);

static GeglRectangle
gimp_operation_border_get_required_for_output (GeglOperation       *self,
                                               const gchar         *input_pad,
                                               const GeglRectangle *roi);
static GeglRectangle
      gimp_operation_border_get_cached_region (GeglOperation       *self,
                                               const GeglRectangle *roi);
static void     gimp_operation_border_prepare (GeglOperation       *operation);
static gboolean gimp_operation_border_process (GeglOperation       *operation,
                                               GeglBuffer          *input,
                                               GeglBuffer          *output,
                                               const GeglRectangle *roi,
                                               gint                 level);


G_DEFINE_TYPE (GimpOperationBorder, gimp_operation_border,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_border_parent_class


static void
gimp_operation_border_class_init (GimpOperationBorderClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->set_property   = gimp_operation_border_set_property;
  object_class->get_property   = gimp_operation_border_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:border",
                                 "categories",  "gimp",
                                 "description", "GIMP Border operation",
                                 NULL);

  operation_class->prepare                 = gimp_operation_border_prepare;
  operation_class->get_required_for_output = gimp_operation_border_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_border_get_cached_region;
  operation_class->threaded                = FALSE;

  filter_class->process                    = gimp_operation_border_process;

  g_object_class_install_property (object_class, PROP_RADIUS_X,
                                   g_param_spec_int ("radius-x",
                                                     "Radius X",
                                                     "Border radius in X direction",
                                                     1, 2342, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RADIUS_Y,
                                   g_param_spec_int ("radius-y",
                                                     "Radius Y",
                                                     "Border radius in Y direction",
                                                     1, 2342, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FEATHER,
                                   g_param_spec_boolean ("feather",
                                                         "Feather",
                                                         "Feather the border",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_EDGE_LOCK,
                                   g_param_spec_boolean ("edge-lock",
                                                         "Edge Lock",
                                                         "Shrink from border",
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_operation_border_init (GimpOperationBorder *self)
{
}

static void
gimp_operation_border_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
 GimpOperationBorder *self = GIMP_OPERATION_BORDER (object);

  switch (property_id)
    {
    case PROP_RADIUS_X:
      g_value_set_int (value, self->radius_x);
      break;

    case PROP_RADIUS_Y:
      g_value_set_int (value, self->radius_y);
      break;

    case PROP_FEATHER:
      g_value_set_boolean (value, self->feather);
      break;

    case PROP_EDGE_LOCK:
      g_value_set_boolean (value, self->edge_lock);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_border_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpOperationBorder *self = GIMP_OPERATION_BORDER (object);

  switch (property_id)
    {
    case PROP_RADIUS_X:
      self->radius_x = g_value_get_int (value);
      break;

    case PROP_RADIUS_Y:
      self->radius_y = g_value_get_int (value);
      break;

    case PROP_FEATHER:
      self->feather = g_value_get_boolean (value);
      break;

    case PROP_EDGE_LOCK:
      self->edge_lock = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_border_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("Y float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("Y float", space));
}

static GeglRectangle
gimp_operation_border_get_required_for_output (GeglOperation       *self,
                                               const gchar         *input_pad,
                                               const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
gimp_operation_border_get_cached_region (GeglOperation       *self,
                                         const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static inline void
rotate_pointers (gfloat  **p,
                 guint32   n)
{
  guint32  i;
  gfloat  *tmp;

  tmp = p[0];

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];

  p[i] = tmp;
}

/* Computes whether pixels in `buf[1]', if they are selected, have neighbouring
   pixels that are unselected. Put result in `transition'. */
static void
compute_transition (gfloat    *transition,
                    gfloat   **buf,
                    gint32     width,
                    gboolean   edge_lock)
{
  register gint32 x = 0;

  if (width == 1)
    {
      if (buf[1][0] >= 0.5 && (buf[0][0] < 0.5 || buf[2][0] < 0.5))
        transition[0] = 1.0;
      else
        transition[0] = 0.0;
      return;
    }

  if (buf[1][0] >= 0.5 && edge_lock)
    {
      /* The pixel to the left (outside of the canvas) is considered selected,
         so we check if there are any unselected pixels in neighbouring pixels
         _on_ the canvas. */
      if (buf[0][x] < 0.5 || buf[0][x + 1] < 0.5 ||
                             buf[1][x + 1] < 0.5 ||
          buf[2][x] < 0.5 || buf[2][x + 1] < 0.5 )
        {
          transition[x] = 1.0;
        }
      else
        {
          transition[x] = 0.0;
        }
    }
  else if (buf[1][0] >= 0.5 && !edge_lock)
    {
      /* We must not care about neighbouring pixels on the image canvas since
         there always are unselected pixels to the left (which is outside of
         the image canvas). */
      transition[x] = 1.0;
    }
  else
    {
      transition[x] = 0.0;
    }

  for (x = 1; x < width - 1; x++)
    {
      if (buf[1][x] >= 0.5)
        {
          if (buf[0][x - 1] < 0.5 || buf[0][x] < 0.5 || buf[0][x + 1] < 0.5 ||
              buf[1][x - 1] < 0.5 ||                    buf[1][x + 1] < 0.5 ||
              buf[2][x - 1] < 0.5 || buf[2][x] < 0.5 || buf[2][x + 1] < 0.5)
            transition[x] = 1.0;
          else
            transition[x] = 0.0;
        }
      else
        {
          transition[x] = 0.0;
        }
    }

  if (buf[1][width - 1] >= 0.5 && edge_lock)
    {
      /* The pixel to the right (outside of the canvas) is considered selected,
         so we check if there are any unselected pixels in neighbouring pixels
         _on_ the canvas. */
      if ( buf[0][x - 1] < 0.5 || buf[0][x] < 0.5 ||
           buf[1][x - 1] < 0.5 ||
           buf[2][x - 1] < 0.5 || buf[2][x] < 0.5)
        {
          transition[width - 1] = 1.0;
        }
      else
        {
          transition[width - 1] = 0.0;
        }
    }
  else if (buf[1][width - 1] >= 0.5 && !edge_lock)
    {
      /* We must not care about neighbouring pixels on the image canvas since
         there always are unselected pixels to the right (which is outside of
         the image canvas). */
      transition[width - 1] = 1.0;
    }
  else
    {
      transition[width - 1] = 0.0;
    }
}

static gboolean
gimp_operation_border_process (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *output,
                               const GeglRectangle *roi,
                               gint                 level)
{
  /* This function has no bugs, but if you imagine some you can blame
   * them on jaycox@gimp.org
   */
  GimpOperationBorder *self          = GIMP_OPERATION_BORDER (operation);
  const Babl          *input_format  = gegl_operation_get_format (operation, "input");
  const Babl          *output_format = gegl_operation_get_format (operation, "output");

  gint32 i, j, x, y;

  /* A cache used in the algorithm as it works its way down. `buf[1]' is the
     current row. Thus, at algorithm initialization, `buf[0]' represents the
     row 'above' the first row of the region. */
  gfloat  *buf[3];

  /* The resulting selection is calculated row by row, and this buffer holds the
     output for each individual row, on each iteration. */
  gfloat  *out;

  /* Keeps track of transitional pixels (pixels that are selected and have
     unselected neighbouring pixels). */
  gfloat **transition;

  /* TODO: Figure out role clearly in algorithm. */
  gint16  *max;

  /* TODO: Figure out role clearly in algorithm. */
  gfloat **density;

  gint16   last_index;

  /* optimize this case specifically */
  if (self->radius_x == 1 && self->radius_y == 1)
    {
      gfloat *transition;
      gfloat *source[3];

      for (i = 0; i < 3; i++)
        source[i] = g_new (gfloat, roi->width);

      transition = g_new (gfloat, roi->width);

      /* With `self->edge_lock', initialize row above image as
       * selected, otherwise, initialize as unselected.
       */
      if (self->edge_lock)
        {
          for (i = 0; i < roi->width; i++)
            source[0][i] = 1.0;
        }
      else
        {
          memset (source[0], 0, roi->width * sizeof (gfloat));
        }

      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x, roi->y + 0,
                                       roi->width, 1),
                       1.0, input_format, source[1],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (roi->height > 1)
        gegl_buffer_get (input,
                         GEGL_RECTANGLE (roi->x, roi->y + 1,
                                         roi->width, 1),
                         1.0, input_format, source[2],
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      else
        memcpy (source[2], source[1], roi->width * sizeof (gfloat));

      compute_transition (transition, source, roi->width, self->edge_lock);
      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y,
                                       roi->width, 1),
                       0, output_format, transition,
                       GEGL_AUTO_ROWSTRIDE);

      for (y = 1; y < roi->height; y++)
        {
          rotate_pointers (source, 3);

          if (y + 1 < roi->height)
            {
              gegl_buffer_get (input,
                               GEGL_RECTANGLE (roi->x, roi->y + y + 1,
                                               roi->width, 1),
                               1.0, input_format, source[2],
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
            }
          else
            {
              /* Depending on `self->edge_lock', set the row below the
               * image as either selected or non-selected.
               */
              if (self->edge_lock)
                {
                  for (i = 0; i < roi->width; i++)
                    source[2][i] = 1.0;
                }
              else
                {
                  memset (source[2], 0, roi->width * sizeof (gfloat));
                }
            }

          compute_transition (transition, source, roi->width, self->edge_lock);
          gegl_buffer_set (output,
                           GEGL_RECTANGLE (roi->x, roi->y + y,
                                           roi->width, 1),
                           0, output_format, transition,
                           GEGL_AUTO_ROWSTRIDE);
        }

      for (i = 0; i < 3; i++)
        g_free (source[i]);

      g_free (transition);

      /* Finished handling the radius = 1 special case, return here. */
      return TRUE;
    }

  max = g_new (gint16, roi->width + 2 * self->radius_x);

  for (i = 0; i < (roi->width + 2 * self->radius_x); i++)
    max[i] = self->radius_y + 2;

  max += self->radius_x;

  for (i = 0; i < 3; i++)
    buf[i] = g_new (gfloat, roi->width);

  transition = g_new (gfloat *, self->radius_y + 1);

  for (i = 0; i < self->radius_y + 1; i++)
    {
      transition[i] = g_new (gfloat, roi->width + 2 * self->radius_x);
      memset (transition[i], 0,
              (roi->width + 2 * self->radius_x) * sizeof (gfloat));
      transition[i] += self->radius_x;
    }

  out = g_new (gfloat, roi->width);

  density = g_new (gfloat *, 2 * self->radius_x + 1);
  density += self->radius_x;

  /* allocate density[][] */
  for (x = 0; x < (self->radius_x + 1); x++)
    {
      density[ x]  = g_new (gfloat, 2 * self->radius_y + 1);
      density[ x] += self->radius_y;
      density[-x]  = density[x];
    }

  /* compute density[][] */
  for (x = 0; x < (self->radius_x + 1); x++)
    {
      gdouble tmpx, tmpy, dist;
      gfloat  a;

      if (x > 0)
        tmpx = x - 0.5;
      else if (x < 0)
        tmpx = x + 0.5;
      else
        tmpx = 0.0;

      for (y = 0; y < (self->radius_y + 1); y++)
        {
          if (y > 0)
            tmpy = y - 0.5;
          else if (y < 0)
            tmpy = y + 0.5;
          else
            tmpy = 0.0;

          dist = ((tmpy * tmpy) / (self->radius_y * self->radius_y) +
                  (tmpx * tmpx) / (self->radius_x * self->radius_x));

          if (dist < 1.0)
            {
              if (self->feather)
                a = 1.0 - sqrt (dist);
              else
                a = 1.0;
            }
          else
            {
              a = 0.0;
            }

          density[ x][ y] = a;
          density[ x][-y] = a;
          density[-x][ y] = a;
          density[-x][-y] = a;
        }
    }

  /* Since the algorithm considerers `buf[0]' to be 'over' the row
   * currently calculated, we must start with `buf[0]' as non-selected
   * if there is no `self->edge_lock. If there is an
   * 'self->edge_lock', initialize the first row to 'selected'. Refer
   * to bug #350009.
   */
  if (self->edge_lock)
    {
      for (i = 0; i < roi->width; i++)
        buf[0][i] = 1.0;
    }
  else
    {
      memset (buf[0], 0, roi->width * sizeof (gfloat));
    }

  gegl_buffer_get (input,
                   GEGL_RECTANGLE (roi->x, roi->y + 0,
                                   roi->width, 1),
                   1.0, input_format, buf[1],
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (roi->height > 1)
    gegl_buffer_get (input,
                     GEGL_RECTANGLE (roi->x, roi->y + 1,
                                     roi->width, 1),
                     1.0, input_format, buf[2],
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  else
    memcpy (buf[2], buf[1], roi->width * sizeof (gfloat));

  compute_transition (transition[1], buf, roi->width, self->edge_lock);

   /* set up top of image */
  for (y = 1; y < self->radius_y && y + 1 < roi->height; y++)
    {
      rotate_pointers (buf, 3);
      gegl_buffer_get (input,
                       GEGL_RECTANGLE (roi->x, roi->y + y + 1,
                                       roi->width, 1),
                       1.0, input_format, buf[2],
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      compute_transition (transition[y + 1], buf, roi->width, self->edge_lock);
    }

  /* set up max[] for top of image */
  for (x = 0; x < roi->width; x++)
    {
      max[x] = -(self->radius_y + 7);

      for (j = 1; j < self->radius_y + 1; j++)
        if (transition[j][x])
          {
            max[x] = j;
            break;
          }
    }

  /* main calculation loop */
  for (y = 0; y < roi->height; y++)
    {
      rotate_pointers (buf, 3);
      rotate_pointers (transition, self->radius_y + 1);

      if (y < roi->height - (self->radius_y + 1))
        {
          gegl_buffer_get (input,
                           GEGL_RECTANGLE (roi->x,
                                           roi->y + y + self->radius_y + 1,
                                           roi->width, 1),
                           1.0, input_format, buf[2],
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
          compute_transition (transition[self->radius_y], buf, roi->width, self->edge_lock);
        }
      else
        {
          if (self->edge_lock)
            {
              memcpy (transition[self->radius_y], transition[self->radius_y - 1], roi->width * sizeof (gfloat));
            }
          else
            {
              /* No edge lock, set everything 'below canvas' as seen
               * from the algorithm as unselected.
               */
              memset (buf[2], 0, roi->width * sizeof (gfloat));
              compute_transition (transition[self->radius_y], buf, roi->width, self->edge_lock);
            }
        }

      /* update max array */
      for (x = 0; x < roi->width; x++)
        {
          if (max[x] < 1)
            {
              if (max[x] <= -self->radius_y)
                {
                  if (transition[self->radius_y][x])
                    max[x] = self->radius_y;
                  else
                    max[x]--;
                }
              else
                {
                  if (transition[-max[x]][x])
                    max[x] = -max[x];
                  else if (transition[-max[x] + 1][x])
                    max[x] = -max[x] + 1;
                  else
                    max[x]--;
                }
            }
          else
            {
              max[x]--;
            }

          if (max[x] < -self->radius_y - 1)
            max[x] = -self->radius_y - 1;
        }

      last_index = 1;

       /* render scan line */
      for (x = 0 ; x < roi->width; x++)
        {
          gfloat last_max;

          last_index--;

          if (last_index >= 0)
            {
              last_max = 0.0;

              for (i = self->radius_x; i >= 0; i--)
                if (max[x + i] <= self->radius_y && max[x + i] >= -self->radius_y &&
                    density[i][max[x+i]] > last_max)
                  {
                    last_max = density[i][max[x + i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }
          else
            {
              last_max = 0.0;

              for (i = self->radius_x; i >= -self->radius_x; i--)
                if (max[x + i] <= self->radius_y && max[x + i] >= -self->radius_y &&
                    density[i][max[x + i]] > last_max)
                  {
                    last_max = density[i][max[x + i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }

          if (last_max <= 0.0)
            {
              for (i = x + 1; i < roi->width; i++)
                {
                  if (max[i] >= -self->radius_y)
                    break;
                }

              if (i - x > self->radius_x)
                {
                  for (; x < i - self->radius_x; x++)
                    out[x] = 0;

                  x--;
                }

              last_index = self->radius_x;
            }
        }

      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y + y,
                                       roi->width, 1),
                       0, output_format, out,
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (out);

  for (i = 0; i < 3; i++)
    g_free (buf[i]);

  max -= self->radius_x;
  g_free (max);

  for (i = 0; i < self->radius_y + 1; i++)
    {
      transition[i] -= self->radius_x;
      g_free (transition[i]);
    }

  g_free (transition);

  for (i = 0; i < self->radius_x + 1 ; i++)
    {
      density[i] -= self->radius_y;
      g_free (density[i]);
    }

  density -= self->radius_x;
  g_free (density);

  return TRUE;
}
