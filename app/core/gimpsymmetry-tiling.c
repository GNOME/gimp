/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsymmetry-tiling.c
 * Copyright (C) 2015 Jehan <jehan@gimp.org>
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

#include <math.h>
#include <string.h>

#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpsymmetry-tiling.h"

#include "gimp-intl.h"


/* Using same epsilon as in GLIB. */
#define G_DOUBLE_EPSILON        (1e-90)

enum
{
  PROP_0,

  PROP_INTERVAL_X,
  PROP_INTERVAL_Y,
  PROP_SHIFT,
  PROP_MAX_X,
  PROP_MAX_Y
};


/* Local function prototypes */

static void       gimp_tiling_constructed        (GObject      *object);
static void       gimp_tiling_finalize           (GObject      *object);
static void       gimp_tiling_set_property       (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void       gimp_tiling_get_property       (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);

static void       gimp_tiling_update_strokes     (GimpSymmetry *tiling,
                                                  GimpDrawable *drawable,
                                                  GimpCoords   *origin);
static void    gimp_tiling_image_size_changed_cb (GimpImage    *image,
                                                  gint          previous_origin_x,
                                                  gint          previous_origin_y,
                                                  gint          previous_width,
                                                  gint          previous_height,
                                                  GimpSymmetry *sym);


G_DEFINE_TYPE (GimpTiling, gimp_tiling, GIMP_TYPE_SYMMETRY)

#define parent_class gimp_tiling_parent_class


static void
gimp_tiling_class_init (GimpTilingClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpSymmetryClass *symmetry_class = GIMP_SYMMETRY_CLASS (klass);
  GParamSpec        *pspec;

  object_class->constructed       = gimp_tiling_constructed;
  object_class->finalize          = gimp_tiling_finalize;
  object_class->set_property      = gimp_tiling_set_property;
  object_class->get_property      = gimp_tiling_get_property;

  symmetry_class->label           = _("Tiling");
  symmetry_class->update_strokes  = gimp_tiling_update_strokes;

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_INTERVAL_X,
                           "interval-x",
                           _("Interval X"),
                           _("Interval on the X axis (pixels)"),
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "interval-x");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-distance");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_INTERVAL_Y,
                           "interval-y",
                           _("Interval Y"),
                           _("Interval on the Y axis (pixels)"),
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "interval-y");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-distance");
  gegl_param_spec_set_property_key (pspec, "axis", "y");

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_SHIFT,
                           "shift",
                           _("Shift"),
                           _("X-shift between lines (pixels)"),
                           0.0, G_MAXDOUBLE, 0.0,
                           GIMP_PARAM_STATIC_STRINGS |
                           GIMP_SYMMETRY_PARAM_GUI);

  pspec = g_object_class_find_property (object_class, "shift");
  gegl_param_spec_set_property_key (pspec, "unit", "pixel-distance");
  gegl_param_spec_set_property_key (pspec, "axis", "x");

  GIMP_CONFIG_PROP_INT (object_class, PROP_MAX_X,
                        "max-x",
                        _("Max strokes X"),
                        _("Maximum number of strokes on the X axis"),
                        0, 100, 0,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_SYMMETRY_PARAM_GUI);

  GIMP_CONFIG_PROP_INT (object_class, PROP_MAX_Y,
                        "max-y",
                        _("Max strokes Y"),
                        _("Maximum number of strokes on the Y axis"),
                        0, 100, 0,
                        GIMP_PARAM_STATIC_STRINGS |
                        GIMP_SYMMETRY_PARAM_GUI);
}

static void
gimp_tiling_init (GimpTiling *tiling)
{
}

static void
gimp_tiling_constructed (GObject *object)
{
  GimpSymmetry *sym    = GIMP_SYMMETRY (object);
  GimpTiling   *tiling = GIMP_TILING (object);

  g_signal_connect_object (sym->image, "size-changed-detailed",
                           G_CALLBACK (gimp_tiling_image_size_changed_cb),
                           sym, 0);

  /* Set reasonable defaults. */
  tiling->interval_x = gimp_image_get_width (sym->image) / 2;
  tiling->interval_y = gimp_image_get_height (sym->image) / 2;
}

static void
gimp_tiling_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_tiling_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpTiling   *tiling = GIMP_TILING (object);
  GimpSymmetry *sym    = GIMP_SYMMETRY (tiling);

  switch (property_id)
    {
    case PROP_INTERVAL_X:
      if (sym->image)
        {
          gdouble new_x = g_value_get_double (value);

          if (new_x < gimp_image_get_width (sym->image))
            {
              tiling->interval_x = new_x;

              if (tiling->interval_x <= tiling->shift + G_DOUBLE_EPSILON)
                {
                  GValue val = G_VALUE_INIT;

                  g_value_init (&val, G_TYPE_DOUBLE);
                  g_value_set_double (&val, 0.0);
                  g_object_set_property (G_OBJECT (object), "shift", &val);
                }
              if (sym->drawable)
                gimp_tiling_update_strokes (sym, sym->drawable, sym->origin);
            }
        }
      break;

    case PROP_INTERVAL_Y:
        {
          gdouble new_y = g_value_get_double (value);

          if (new_y < gimp_image_get_height (sym->image))
            {
              tiling->interval_y = new_y;

              if (tiling->interval_y <= G_DOUBLE_EPSILON)
                {
                  GValue val = G_VALUE_INIT;

                  g_value_init (&val, G_TYPE_DOUBLE);
                  g_value_set_double (&val, 0.0);
                  g_object_set_property (G_OBJECT (object), "shift", &val);
                }
              if (sym->drawable)
                gimp_tiling_update_strokes (sym, sym->drawable, sym->origin);
            }
        }
      break;

    case PROP_SHIFT:
        {
          gdouble new_shift = g_value_get_double (value);

          if (new_shift == 0.0 ||
              (tiling->interval_y != 0.0 && new_shift < tiling->interval_x))
            {
              tiling->shift = new_shift;
              if (sym->drawable)
                gimp_tiling_update_strokes (sym, sym->drawable, sym->origin);
            }
        }
      break;

    case PROP_MAX_X:
      tiling->max_x = g_value_get_int (value);
      if (sym->drawable)
        gimp_tiling_update_strokes (sym, sym->drawable, sym->origin);
      break;

    case PROP_MAX_Y:
      tiling->max_y = g_value_get_int (value);
      if (sym->drawable)
        gimp_tiling_update_strokes (sym, sym->drawable, sym->origin);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tiling_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpTiling *tiling = GIMP_TILING (object);

  switch (property_id)
    {
    case PROP_INTERVAL_X:
      g_value_set_double (value, tiling->interval_x);
      break;
    case PROP_INTERVAL_Y:
      g_value_set_double (value, tiling->interval_y);
      break;
    case PROP_SHIFT:
      g_value_set_double (value, tiling->shift);
      break;
    case PROP_MAX_X:
      g_value_set_int (value, tiling->max_x);
      break;
    case PROP_MAX_Y:
      g_value_set_int (value, tiling->max_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_tiling_update_strokes (GimpSymmetry *sym,
                            GimpDrawable *drawable,
                            GimpCoords   *origin)
{
  GimpTiling *tiling  = GIMP_TILING (sym);
  GList      *strokes = NULL;
  GimpCoords *coords;
  gdouble     width;
  gdouble     height;
  gdouble     startx = origin->x;
  gdouble     starty = origin->y;
  gdouble     x;
  gdouble     y;
  gint        x_count;
  gint        y_count;

  g_list_free_full (sym->strokes, g_free);
  sym->strokes = NULL;

  width  = gimp_item_get_width (GIMP_ITEM (drawable));
  height = gimp_item_get_height (GIMP_ITEM (drawable));

  if (sym->stateful)
    {
      /* While I can compute exactly the right number of strokes to
       * paint on-canvas for stateless tools, stateful tools need to
       * always have the same number and order of strokes. For this
       * reason, I compute strokes to fill 2 times the width and height.
       * This makes the symmetry less efficient with stateful tools, but
       * also weird behavior may happen if you decide to paint out of
       * canvas and expect tiling to work in-canvas since it won't
       * actually be infinite (as no new strokes can be added while
       * painting since we are stateful).
       */
      gint i, j;

      if (tiling->interval_x < 1.0)
        {
          x_count = 1;
        }
      else if (tiling->max_x == 0)
        {
          x_count = (gint) ceil (width / tiling->interval_x);
          startx -= tiling->interval_x * (gdouble) x_count;
          x_count = 2 * x_count + 1;
        }
      else
        {
          x_count = tiling->max_x;
        }

      if (tiling->interval_y < 1.0)
        {
          y_count = 1;
        }
      else if (tiling->max_y == 0)
        {
          y_count = (gint) ceil (height / tiling->interval_y);
          starty -= tiling->interval_y * (gdouble) y_count;
          y_count = 2 * y_count + 1;
        }
      else
        {
          y_count = tiling->max_y;
        }

      for (i = 0, x = startx; i < x_count; i++)
        {
          for (j = 0, y = starty; j < y_count; j++)
            {
              coords = g_memdup (origin, sizeof (GimpCoords));
              coords->x = x;
              coords->y = y;
              strokes = g_list_prepend (strokes, coords);

              y += tiling->interval_y;
            }
          x += tiling->interval_x;
        }
    }
  else
    {
      if (origin->x > 0 && tiling->max_x == 0 && tiling->interval_x >= 1.0)
        startx = fmod (origin->x, tiling->interval_x) - tiling->interval_x;

      if (origin->y > 0 && tiling->max_y == 0 && tiling->interval_y >= 1.0)
        {
          starty = fmod (origin->y, tiling->interval_y) - tiling->interval_y;

          if (tiling->shift > 0.0)
            startx -= tiling->shift * floor (origin->y / tiling->interval_y + 1);
        }

      for (y_count = 0, y = starty; y < height + tiling->interval_y;
           y_count++, y += tiling->interval_y)
        {
          if (tiling->max_y && y_count >= tiling->max_y)
            break;

          for (x_count = 0, x = startx; x < width + tiling->interval_x;
               x_count++, x += tiling->interval_x)
            {
              if (tiling->max_x && x_count >= tiling->max_x)
                break;

              coords = g_memdup (origin, sizeof (GimpCoords));
              coords->x = x;
              coords->y = y;
              strokes = g_list_prepend (strokes, coords);

              if (tiling->interval_x < 1.0)
                break;
            }

          if (tiling->max_x || startx + tiling->shift <= 0.0)
            startx = startx + tiling->shift;
          else
            startx = startx - tiling->interval_x + tiling->shift;

          if (tiling->interval_y < 1.0)
            break;
        }
    }

  sym->strokes = strokes;

  g_signal_emit_by_name (sym, "strokes-updated", sym->image);
}

static void
gimp_tiling_image_size_changed_cb (GimpImage    *image,
                                   gint          previous_origin_x,
                                   gint          previous_origin_y,
                                   gint          previous_width,
                                   gint          previous_height,
                                   GimpSymmetry *sym)
{
  if (previous_width  != gimp_image_get_width  (image) ||
      previous_height != gimp_image_get_height (image))
    {
      g_signal_emit_by_name (sym, "gui-param-changed", sym->image);
    }
}
