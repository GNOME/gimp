/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationoffset.c
 * Copyright (C) 2019 Ell
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
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "operations-types.h"

#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "core/gimpcontext.h"

#include "gimpoperationoffset.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_TYPE,
  PROP_X,
  PROP_Y
};


static void            gimp_operation_offset_dispose                   (GObject              *object);
static void            gimp_operation_offset_get_property              (GObject              *object,
                                                                        guint                 property_id,
                                                                        GValue               *value,
                                                                        GParamSpec           *pspec);
static void            gimp_operation_offset_set_property              (GObject              *object,
                                                                        guint                 property_id,
                                                                        const GValue         *value,
                                                                        GParamSpec           *pspec);

static GeglRectangle   gimp_operation_offset_get_required_for_output   (GeglOperation        *operation,
                                                                        const gchar          *input_pad,
                                                                        const GeglRectangle  *output_roi);
static GeglRectangle   gimp_operation_offset_get_invalidated_by_change (GeglOperation        *operation,
                                                                        const gchar          *input_pad,
                                                                        const GeglRectangle  *input_roi);
static void            gimp_operation_offset_prepare                   (GeglOperation        *operation);
static gboolean        gimp_operation_offset_parent_process            (GeglOperation        *operation,
                                                                        GeglOperationContext *context,
                                                                        const gchar          *output_pad,
                                                                        const GeglRectangle  *result,
                                                                        gint                  level);

static gboolean        gimp_operation_offset_process                   (GeglOperation        *operation,
                                                                        GeglBuffer           *input,
                                                                        GeglBuffer           *output,
                                                                        const GeglRectangle  *roi,
                                                                        gint                  level);

static void            gimp_operation_offset_get_offset                (GimpOperationOffset  *offset,
                                                                        gboolean              invert,
                                                                        gint                 *x,
                                                                        gint                 *y);
static void            gimp_operation_offset_get_rect                  (GimpOperationOffset  *offset,
                                                                        gboolean              invert,
                                                                        const GeglRectangle  *roi,
                                                                        GeglRectangle        *rect);


G_DEFINE_TYPE (GimpOperationOffset, gimp_operation_offset,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class gimp_operation_offset_parent_class


static void
gimp_operation_offset_class_init (GimpOperationOffsetClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->dispose                      = gimp_operation_offset_dispose;
  object_class->set_property                 = gimp_operation_offset_set_property;
  object_class->get_property                 = gimp_operation_offset_get_property;

  operation_class->get_required_for_output   = gimp_operation_offset_get_required_for_output;
  operation_class->get_invalidated_by_change = gimp_operation_offset_get_invalidated_by_change;
  operation_class->prepare                   = gimp_operation_offset_prepare;
  operation_class->process                   = gimp_operation_offset_parent_process;

  operation_class->threaded                  = FALSE;
  operation_class->cache_policy              = GEGL_CACHE_POLICY_NEVER;

  filter_class->process                      = gimp_operation_offset_process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:offset",
                                 "categories",  "transform",
                                 "description", _("Shift the pixels, optionally wrapping them at the borders"),
                                 NULL);

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        "Context",
                                                        "A GimpContext",
                                                        GIMP_TYPE_CONTEXT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum  ("type",
                                                       "Type",
                                                       "Offset type",
                                                       GIMP_TYPE_OFFSET_TYPE,
                                                       GIMP_OFFSET_WRAP_AROUND,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "X Offset",
                                                     "X offset",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "Y Offset",
                                                     "Y offset",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_operation_offset_init (GimpOperationOffset *self)
{
}

static void
gimp_operation_offset_dispose (GObject *object)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (object);

  g_clear_object (&offset->context);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_operation_offset_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, offset->context);
      break;

    case PROP_TYPE:
      g_value_set_enum (value, offset->type);
      break;

    case PROP_X:
      g_value_set_int (value, offset->x);
      break;

    case PROP_Y:
      g_value_set_int (value, offset->y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_offset_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (object);

  switch (property_id)
    {
    case PROP_CONTEXT:
      g_set_object (&offset->context, g_value_get_object (value));
      break;

    case PROP_TYPE:
      offset->type = g_value_get_enum (value);
      break;

    case PROP_X:
      offset->x = g_value_get_int (value);
      break;

    case PROP_Y:
      offset->y = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GeglRectangle
gimp_operation_offset_get_required_for_output (GeglOperation       *operation,
                                               const gchar         *input_pad,
                                               const GeglRectangle *output_roi)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (operation);
  GeglRectangle        rect;

  gimp_operation_offset_get_rect (offset, TRUE, output_roi, &rect);

  return rect;
}

static GeglRectangle
gimp_operation_offset_get_invalidated_by_change (GeglOperation       *operation,
                                                 const gchar         *input_pad,
                                                 const GeglRectangle *input_roi)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (operation);
  GeglRectangle        rect;

  gimp_operation_offset_get_rect (offset, FALSE, input_roi, &rect);

  return rect;
}

static void
gimp_operation_offset_prepare (GeglOperation *operation)
{
  const Babl *format;

  format = gegl_operation_get_source_format (operation, "input");

  if (! format)
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
gimp_operation_offset_parent_process (GeglOperation        *operation,
                                      GeglOperationContext *context,
                                      const gchar          *output_pad,
                                      const GeglRectangle  *result,
                                      gint                  level)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (operation);
  GObject             *input;
  gint                 x;
  gint                 y;

  input = gegl_operation_context_get_object (context, "input");

  gimp_operation_offset_get_offset (offset, FALSE, &x, &y);

  if (x == 0 && y == 0)
    {
      gegl_operation_context_set_object (context, "output", input);

      return TRUE;
    }
  else if (offset->type  == GIMP_OFFSET_TRANSPARENT ||
           (offset->type == GIMP_OFFSET_BACKGROUND  &&
            ! offset->context))
    {
      GObject *output = NULL;

      if (input)
        {
          GeglRectangle bounds;
          GeglRectangle extent;

          bounds = gegl_operation_get_bounding_box (GEGL_OPERATION (offset));

          extent = *gegl_buffer_get_extent (GEGL_BUFFER (input));

          extent.x += x;
          extent.y += y;

          if (gegl_rectangle_intersect (&extent, &extent, &bounds))
            {
              output = g_object_new (GEGL_TYPE_BUFFER,
                                     "source",  input,
                                     "x",       extent.x,
                                     "y",       extent.y,
                                     "width",   extent.width,
                                     "height",  extent.height,
                                     "shift-x", -x,
                                     "shift-y", -y,
                                     NULL);

              if (gegl_object_get_has_forked (input))
                gegl_object_set_has_forked (output);
            }
        }

      gegl_operation_context_take_object (context, "output", output);

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_pad, result,
                                                       level);
}

static gboolean
gimp_operation_offset_process (GeglOperation       *operation,
                               GeglBuffer          *input,
                               GeglBuffer          *output,
                               const GeglRectangle *roi,
                               gint                 level)
{
  GimpOperationOffset *offset = GIMP_OPERATION_OFFSET (operation);
  GeglColor           *color  = NULL;
  GeglRectangle        bounds;
  gint                 x;
  gint                 y;
  gint                 i;

  bounds = gegl_operation_get_bounding_box (GEGL_OPERATION (offset));

  gimp_operation_offset_get_offset (offset, FALSE, &x, &y);

  if (offset->type == GIMP_OFFSET_BACKGROUND && offset->context)
    color = gimp_context_get_background (offset->context);

  for (i = 0; i < 4; i++)
    {
      GeglRectangle offset_bounds = bounds;
      gint          offset_x      = x;
      gint          offset_y      = y;

      if (i & 1)
        offset_x += x < 0 ? bounds.width  : -bounds.width;
      if (i & 2)
        offset_y += y < 0 ? bounds.height : -bounds.height;

      offset_bounds.x += offset_x;
      offset_bounds.y += offset_y;

      if (gegl_rectangle_intersect (&offset_bounds, &offset_bounds, roi))
        {
          if (i == 0 || offset->type == GIMP_OFFSET_WRAP_AROUND)
            {
              GeglRectangle offset_roi = offset_bounds;

              offset_roi.x -= offset_x;
              offset_roi.y -= offset_y;

              gimp_gegl_buffer_copy (input,  &offset_roi, GEGL_ABYSS_NONE,
                                     output, &offset_bounds);
            }
          else if (color)
            {
              gegl_buffer_set_color (output, &offset_bounds, color);
            }
        }
    }

  return TRUE;
}

static void
gimp_operation_offset_get_offset (GimpOperationOffset *offset,
                                  gboolean             invert,
                                  gint                *x,
                                  gint                *y)
{
  GeglRectangle bounds;

  bounds = gegl_operation_get_bounding_box (GEGL_OPERATION (offset));

  if (gegl_rectangle_is_empty (&bounds))
    {
      *x = 0;
      *y = 0;

      return;
    }

  *x = offset->x;
  *y = offset->y;

  if (invert)
    {
      *x = -*x;
      *y = -*y;
    }

  if (offset->type == GIMP_OFFSET_WRAP_AROUND)
    {
      *x %= bounds.width;

      if (*x < 0)
        *x += bounds.width;

      *y %= bounds.height;

      if (*y < 0)
        *y += bounds.height;
    }
  else
    {
      *x = CLAMP (*x, -bounds.width,  +bounds.width);
      *y = CLAMP (*y, -bounds.height, +bounds.height);
    }
}

static void
gimp_operation_offset_get_rect (GimpOperationOffset *offset,
                                gboolean             invert,
                                const GeglRectangle *roi,
                                GeglRectangle       *rect)
{
  GeglRectangle bounds;
  gint          x;
  gint          y;

  bounds = gegl_operation_get_bounding_box (GEGL_OPERATION (offset));

  if (gegl_rectangle_is_empty (&bounds))
    {
      rect->x      = 0;
      rect->y      = 0;
      rect->width  = 0;
      rect->height = 0;

      return;
    }

  gimp_operation_offset_get_offset (offset, invert, &x, &y);

  *rect = *roi;

  rect->x += x;
  rect->y += y;

  if (offset->type == GIMP_OFFSET_WRAP_AROUND)
    {
      if (rect->x + rect->width > bounds.x + bounds.width)
        {
          rect->x      = bounds.x;
          rect->width  = bounds.width;
        }

      if (rect->y + rect->height > bounds.y + bounds.height)
        {
          rect->y      = bounds.y;
          rect->height = bounds.height;
        }
    }

  gegl_rectangle_intersect (rect, rect, &bounds);
}
