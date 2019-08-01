/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcomposecrop.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
 * Copyright (C) 2016 Massimo Valentini <mvalentini@src.gnome.org>
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

#include "operations-types.h"

#include "gimpoperationcomposecrop.h"


enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};


static void            gimp_operation_compose_crop_get_property            (GObject              *object,
                                                                            guint                 property_id,
                                                                            GValue               *value,
                                                                            GParamSpec           *pspec);
static void            gimp_operation_compose_crop_set_property            (GObject              *object,
                                                                            guint                 property_id,
                                                                            const GValue         *value,
                                                                            GParamSpec           *pspec);

static void            gimp_operation_compose_crop_prepare                 (GeglOperation        *operation);
static GeglRectangle   gimp_operation_compose_crop_get_required_for_output (GeglOperation        *operation,
                                                                            const gchar          *input_pad,
                                                                            const GeglRectangle  *output_roi);
static gboolean        gimp_operation_compose_crop_parent_process          (GeglOperation        *operation,
                                                                            GeglOperationContext *context,
                                                                            const gchar          *output_pad,
                                                                            const GeglRectangle  *roi,
                                                                            gint                  level);

static gboolean        gimp_operation_compose_crop_process                 (GeglOperation        *operation,
                                                                            void                 *in_buf,
                                                                            void                 *aux_buf,
                                                                            void                 *out_buf,
                                                                            glong                 samples,
                                                                            const GeglRectangle  *roi,
                                                                            gint                  level);


G_DEFINE_TYPE (GimpOperationComposeCrop, gimp_operation_compose_crop,
               GEGL_TYPE_OPERATION_POINT_COMPOSER)

#define parent_class gimp_operation_compose_crop_parent_class


static void
gimp_operation_compose_crop_class_init (GimpOperationComposeCropClass *klass)
{
  GObjectClass                    *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  object_class->set_property = gimp_operation_compose_crop_set_property;
  object_class->get_property = gimp_operation_compose_crop_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:compose-crop",
                                 "categories",  "gimp",
                                 "description", "Selectively pick components from src or aux",
                                 NULL);

  operation_class->prepare                   = gimp_operation_compose_crop_prepare;
  operation_class->get_invalidated_by_change = gimp_operation_compose_crop_get_required_for_output;
  operation_class->get_required_for_output   = gimp_operation_compose_crop_get_required_for_output;
  operation_class->process                   = gimp_operation_compose_crop_parent_process;

  point_class->process                       = gimp_operation_compose_crop_process;

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "x",
                                                     "x",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "y",
                                                     "y",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width",
                                                     "width",
                                                     "width",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height",
                                                     "height",
                                                     "height",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
gimp_operation_compose_crop_init (GimpOperationComposeCrop *self)
{
}

static void
gimp_operation_compose_crop_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpOperationComposeCrop *self = GIMP_OPERATION_COMPOSE_CROP (object);

  switch (property_id)
    {
    case PROP_X:
      g_value_set_int (value, self->rect.x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->rect.y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, self->rect.width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, self->rect.height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_compose_crop_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpOperationComposeCrop *self = GIMP_OPERATION_COMPOSE_CROP (object);

  switch (property_id)
    {
    case PROP_X:
      self->rect.x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->rect.y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      self->rect.width = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      self->rect.height = g_value_get_int (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_compose_crop_prepare (GeglOperation *operation)
{
  const Babl *input_format  = gegl_operation_get_source_format (operation, "input");
  const Babl *aux_format    = gegl_operation_get_source_format (operation, "aux");
  const Babl *format;

  if (input_format)
    {
      if (input_format == aux_format)
        {
          format = input_format;
        }
      else
        {
          const Babl *model = babl_format_get_model (input_format);

          if (! strcmp (babl_get_name (model), "R'G'B'A"))
            format = babl_format_with_space ("R'G'B'A float", input_format);
          else
            format = babl_format_with_space ("RGBA float", input_format);
        }
    }
  else
    {
      format = babl_format_with_space ("RGBA float", input_format);
    }

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
gimp_operation_compose_crop_get_required_for_output (GeglOperation       *operation,
                                                     const gchar         *input_pad,
                                                     const GeglRectangle *output_roi)
{
  GimpOperationComposeCrop *self = GIMP_OPERATION_COMPOSE_CROP (operation);
  GeglRectangle             result;

  if (! strcmp (input_pad, "input"))
    gegl_rectangle_intersect (&result, output_roi, &self->rect);
  else if (! strcmp (input_pad, "aux"))
    gegl_rectangle_subtract_bounding_box (&result, output_roi, &self->rect);
  else
    g_return_val_if_reached (*output_roi);

  return result;
}

static gboolean
gimp_operation_compose_crop_parent_process (GeglOperation        *operation,
                                            GeglOperationContext *context,
                                            const gchar          *output_pad,
                                            const GeglRectangle  *roi,
                                            gint                  level)
{
  GimpOperationComposeCrop *self = GIMP_OPERATION_COMPOSE_CROP (operation);

  if (gegl_rectangle_contains (&self->rect, roi))
    {
      GObject *input;

      input = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_set_object (context, "output", input);

      return TRUE;
    }
  else if (! gegl_rectangle_intersect (NULL, &self->rect, roi))
    {
      GObject *aux;

      aux = gegl_operation_context_get_object (context, "aux");
      gegl_operation_context_set_object (context, "output", aux);

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_pad, roi, level);
}

static gboolean
gimp_operation_compose_crop_process (GeglOperation       *operation,
                                     void                *in_buf,
                                     void                *aux_buf,
                                     void                *out_buf,
                                     glong                samples,
                                     const GeglRectangle *roi,
                                     gint                 level)
{
  GimpOperationComposeCrop *self   = GIMP_OPERATION_COMPOSE_CROP (operation);
  const Babl               *format = gegl_operation_get_format (operation, "output");
  gint                      bpp    = babl_format_get_bytes_per_pixel (format);
  const guchar             *in     = in_buf;
  const guchar             *aux    = aux_buf;
  guchar                   *out    = out_buf;
  gint                      x0, x1;
  gint                      y0, y1;
  gint                      y;

#define COPY(src, n)                                                           \
  do                                                                           \
    {                                                                          \
      gint size = (n) * bpp;                                                   \
                                                                               \
      if (src)                                                                 \
        memcpy (out, (src), size);                                             \
      else                                                                     \
        memset (out, 0, size);                                                 \
                                                                               \
      in           += size;                                                    \
      if (aux) aux += size;                                                    \
      out          += size;                                                    \
    }                                                                          \
  while (FALSE)

  x0 = CLAMP (self->rect.x,                     roi->x, roi->x + roi->width);
  x1 = CLAMP (self->rect.x + self->rect.width,  roi->x, roi->x + roi->width);

  y0 = CLAMP (self->rect.y,                     roi->y, roi->y + roi->height);
  y1 = CLAMP (self->rect.y + self->rect.height, roi->y, roi->y + roi->height);

  COPY (aux, (y0 - roi->y) * roi->width);

  for (y = y0; y < y1; y++)
    {
      COPY (aux, x0 - roi->x);
      COPY (in,  x1 - x0);
      COPY (aux, roi->x + roi->width - x1);
    }

  COPY (aux, (roi->y + roi->height - y1) * roi->width);

#undef COPY

  return TRUE;
}
