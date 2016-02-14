/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationflood.c
 * Copyright (C) 2016 Ell
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "operations-types.h"

#include "gimpoperationflood.h"


static void          gimp_operation_flood_prepare      (GeglOperation       *operation);
static GeglRectangle
          gimp_operation_flood_get_required_for_output (GeglOperation       *self,
                                                        const gchar         *input_pad,
                                                        const GeglRectangle *roi);
static GeglRectangle
                gimp_operation_flood_get_cached_region (GeglOperation       *self,
                                                        const GeglRectangle *roi);

static gboolean gimp_operation_flood_process_segment   (const gfloat        *src,
                                                        gfloat              *dest,
                                                        gchar               *changed,
                                                        gint                 size);
static gboolean gimp_operation_flood_process           (GeglOperation       *operation,
                                                        GeglBuffer          *input,
                                                        GeglBuffer          *output,
                                                        const GeglRectangle *roi,
                                                        gint                 level);


G_DEFINE_TYPE (GimpOperationFlood, gimp_operation_flood,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_flood_parent_class


static void
gimp_operation_flood_class_init (GimpOperationFloodClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->want_in_place = FALSE;
  operation_class->threaded      = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:flood",
                                 "categories",  "gimp",
                                 "description", "GIMP Flood operation",
                                 NULL);

  operation_class->prepare                 = gimp_operation_flood_prepare;
  operation_class->get_required_for_output = gimp_operation_flood_get_required_for_output;
  operation_class->get_cached_region       = gimp_operation_flood_get_cached_region;

  filter_class->process                    = gimp_operation_flood_process;
}

static void
gimp_operation_flood_init (GimpOperationFlood *self)
{
}

static void
gimp_operation_flood_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static GeglRectangle
gimp_operation_flood_get_required_for_output (GeglOperation       *self,
                                              const gchar         *input_pad,
                                              const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
gimp_operation_flood_get_cached_region (GeglOperation       *self,
                                        const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static gboolean
gimp_operation_flood_process_segment (const gfloat *src,
                                      gfloat       *dest,
                                      gchar        *changed,
                                      gint          size)
{
  gint     dir;
  gboolean any_changed = FALSE;

  for (dir = 1; dir >= -1; dir -= 2) /*  for dir in [1, -1]: ...  */
    {
      gint   i;
      gfloat level = 0.0;

      for (i = size; i; i--)
        {
          if      (*src  > level) { level    = *src;               }
          if      (*dest < level) { level    = *dest;              }
          else if (level < *dest) { *dest    = level;
                                    *changed = any_changed = TRUE; }

          if (i > 1)
            {
              src     += dir;
              dest    += dir;
              changed += dir;
            }
        }
    }

  return any_changed;
}

static gboolean
gimp_operation_flood_process (GeglOperation       *operation,
                              GeglBuffer          *input,
                              GeglBuffer          *output,
                              const GeglRectangle *roi,
                              gint                 level)
{
  const Babl *input_format  = babl_format ("Y float");
  const Babl *output_format = babl_format ("Y float");
  gfloat     *src_buffer, *dest_buffer;
  gchar      *horz_changed, *vert_changed;
  gboolean    any_changed;
  gint        forced;
  gboolean    horz;
  GeglColor  *color;

  g_return_val_if_fail (input != output, FALSE);

  src_buffer   = g_new (gfloat, MAX (roi->width, roi->height));
  dest_buffer  = g_new (gfloat, MAX (roi->width, roi->height));
  horz_changed = g_new (gchar, roi->width);
  vert_changed = g_new (gchar, roi->height);

  color = gegl_color_new ("#fff");
  gegl_buffer_set_color (output, roi, color);
  g_object_unref (color);

  forced = 2; /*  Force at least one horizontal and one vertical iterations.  */
  horz = TRUE;
  do
    {
      gint           count, size;
      GeglRectangle  rect = *roi;
      gint          *coord;
      gchar         *prev_changed, *curr_changed;
      gint           i;

      any_changed = FALSE;

      if (horz)
        {
          count        = roi->height;
          size         = roi->width;
          rect.height  = 1;
          coord        = &rect.y;
          prev_changed = vert_changed;
          curr_changed = horz_changed;
        }
      else
        {
          count        = roi->width;
          size         = roi->height;
          rect.width   = 1;
          coord        = &rect.x;
          prev_changed = horz_changed;
          curr_changed = vert_changed;
        }

      for (i = 0; i < count; i++, (*coord)++)
        {
          gboolean this_changed;

          if (! (forced || prev_changed[i]))
            continue;
          prev_changed[i] = FALSE;

          gegl_buffer_get (input, &rect, 1.0, input_format, src_buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
          gegl_buffer_get (output, &rect, 1.0, output_format, dest_buffer,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          this_changed = gimp_operation_flood_process_segment (src_buffer,
                                                               dest_buffer,
                                                               curr_changed,
                                                               size);

          if (this_changed)
            {
              gegl_buffer_set (output, &rect, 0, output_format, dest_buffer,
                               GEGL_AUTO_ROWSTRIDE);

              any_changed = TRUE;
            }
        }

      horz = ! horz;
      if (forced)
        forced--;
    }
  while (forced || any_changed);

  g_free (src_buffer);
  g_free (dest_buffer);
  g_free (horz_changed);
  g_free (vert_changed);

  return TRUE;
}
