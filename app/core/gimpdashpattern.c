/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdashpattern.c
 * Copyright (C) 2003 Simon Budig
 * Copyright (C) 2005 Sven Neumann
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

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimpdashpattern.h"


typedef GArray GimpDashPattern;
G_DEFINE_BOXED_TYPE (GimpDashPattern, gimp_dash_pattern, gimp_dash_pattern_copy, gimp_dash_pattern_free)

GArray *
gimp_dash_pattern_new_from_preset (GimpDashPreset preset)
{
  GArray *pattern;
  gdouble dash;
  gint    i;

  pattern = g_array_new (FALSE, FALSE, sizeof (gdouble));

  switch (preset)
    {
    case GIMP_DASH_CUSTOM:
      g_warning ("GIMP_DASH_CUSTOM passed to gimp_dash_pattern_from_preset()");
      break;

    case GIMP_DASH_LINE:
      break;

    case GIMP_DASH_LONG_DASH:
      dash = 9.0; g_array_append_val (pattern, dash);
      dash = 3.0; g_array_append_val (pattern, dash);
      break;

    case GIMP_DASH_MEDIUM_DASH:
      dash = 6.0; g_array_append_val (pattern, dash);
      dash = 6.0; g_array_append_val (pattern, dash);
      break;

    case GIMP_DASH_SHORT_DASH:
      dash = 3.0; g_array_append_val (pattern, dash);
      dash = 9.0; g_array_append_val (pattern, dash);
      break;

    case GIMP_DASH_SPARSE_DOTS:
      for (i = 0; i < 2; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
          dash = 5.0; g_array_append_val (pattern, dash);
        }
      break;

    case GIMP_DASH_NORMAL_DOTS:
      for (i = 0; i < 3; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
          dash = 3.0; g_array_append_val (pattern, dash);
        }
      break;

    case GIMP_DASH_DENSE_DOTS:
      for (i = 0; i < 12; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
        }
      break;

    case GIMP_DASH_STIPPLES:
      for (i = 0; i < 24; i++)
        {
          dash = 0.5; g_array_append_val (pattern, dash);
        }
      break;

    case GIMP_DASH_DASH_DOT:
      dash = 7.0; g_array_append_val (pattern, dash);
      dash = 2.0; g_array_append_val (pattern, dash);
      dash = 1.0; g_array_append_val (pattern, dash);
      dash = 2.0; g_array_append_val (pattern, dash);
      break;

    case GIMP_DASH_DASH_DOT_DOT:
      dash = 7.0; g_array_append_val (pattern, dash);
      for (i=0; i < 5; i++)
        {
          dash = 1.0; g_array_append_val (pattern, dash);
        }
      break;
    }

  if (pattern->len < 2)
    {
      gimp_dash_pattern_free (pattern);
      return NULL;
    }

  return pattern;
}

GArray *
gimp_dash_pattern_new_from_segments (const gboolean *segments,
                                     gint            n_segments,
                                     gdouble         dash_length)
{
  GArray   *pattern;
  gint      i;
  gint      count;
  gboolean  state;

  g_return_val_if_fail (segments != NULL || n_segments == 0, NULL);

  pattern = g_array_new (FALSE, FALSE, sizeof (gdouble));

  for (i = 0, count = 0, state = TRUE; i <= n_segments; i++)
    {
      if (i < n_segments && segments[i] == state)
        {
          count++;
        }
      else
        {
          gdouble l = (dash_length * count) / n_segments;

          g_array_append_val (pattern, l);

          count = 1;
          state = ! state;
        }
    }

  if (pattern->len < 2)
    {
      gimp_dash_pattern_free (pattern);
      return NULL;
    }

  return pattern;
}

void
gimp_dash_pattern_fill_segments (GArray   *pattern,
                                 gboolean *segments,
                                 gint      n_segments)
{
  gdouble   factor;
  gdouble   sum;
  gint      i, j;
  gboolean  paint;

  g_return_if_fail (segments != NULL || n_segments == 0);

  if (pattern == NULL || pattern->len <= 1)
    {
      for (i = 0; i < n_segments; i++)
        segments[i] = TRUE;

      return;
    }

  for (i = 0, sum = 0; i < pattern->len ; i++)
    {
      sum += g_array_index (pattern, gdouble, i);
    }

  factor = ((gdouble) n_segments) / sum;

  j = 0;
  sum = g_array_index (pattern, gdouble, j) * factor;
  paint = TRUE;

  for (i = 0; i < n_segments ; i++)
    {
      while (j < pattern->len && sum <= i)
        {
          paint = ! paint;
          j++;
          sum += g_array_index (pattern, gdouble, j) * factor;
        }

      segments[i] = paint;
    }
}

GArray *
gimp_dash_pattern_from_value_array (GimpValueArray *value_array)
{
  if (value_array == NULL || gimp_value_array_length (value_array) == 0)
    {
      return NULL;
    }
  else
    {
      GArray *pattern;
      gint    length;
      gint    i;

      length = gimp_value_array_length (value_array);

      pattern = g_array_sized_new (FALSE, FALSE, sizeof (gdouble), length);

      for (i = 0; i < length; i++)
        {
          GValue *item = gimp_value_array_index (value_array, i);
          gdouble val;

          g_return_val_if_fail (G_VALUE_HOLDS_DOUBLE (item), NULL);

          val = g_value_get_double (item);

          g_array_append_val (pattern, val);
        }

      return pattern;
    }
}

GimpValueArray *
gimp_dash_pattern_to_value_array (GArray *pattern)
{
  if (pattern != NULL && pattern->len > 0)
    {
      GimpValueArray *value_array = gimp_value_array_new (pattern->len);
      GValue          item        = G_VALUE_INIT;
      gint            i;

      g_value_init (&item, G_TYPE_DOUBLE);

      for (i = 0; i < pattern->len; i++)
        {
          g_value_set_double (&item, g_array_index (pattern, gdouble, i));
          gimp_value_array_append (value_array, &item);
        }

      g_value_unset (&item);

      return value_array;
    }

  return NULL;
}

GArray *
gimp_dash_pattern_from_double_array (gint           n_dashes,
                                     const gdouble *dashes)
{
  if (n_dashes > 0 && dashes != NULL)
    {
      GArray *pattern;
      gint    i;

      pattern = g_array_new (FALSE, FALSE, sizeof (gdouble));

      for (i = 0; i < n_dashes; i++)
        {
          if (dashes[i] >= 0.0)
            {
              g_array_append_val (pattern, dashes[i]);
            }
          else
            {
              g_array_free (pattern, TRUE);
              return NULL;
            }
        }

      return pattern;
    }

  return NULL;
}

gdouble *
gimp_dash_pattern_to_double_array (GArray *pattern,
                                   gsize  *n_dashes)
{
  if (pattern != NULL && pattern->len > 0)
    {
      gdouble *dashes;
      gint     i;

      dashes = g_new0 (gdouble, pattern->len);

      for (i = 0; i < pattern->len; i++)
        {
          dashes[i] = g_array_index (pattern, gdouble, i);
        }

      if (n_dashes)
        *n_dashes = pattern->len;

      return dashes;
    }

  return NULL;
}

GArray *
gimp_dash_pattern_copy (GArray *pattern)
{
  if (pattern)
    {
      GArray *copy;
      gint    i;

      copy = g_array_sized_new (FALSE, FALSE, sizeof (gdouble), pattern->len);

      for (i = 0; i < pattern->len; i++)
        g_array_append_val (copy, g_array_index (pattern, gdouble, i));

      return copy;
    }

  return NULL;
}

void
gimp_dash_pattern_free (GArray *pattern)
{
  if (pattern)
    g_array_free (pattern, TRUE);
}
