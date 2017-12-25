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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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


static void            gimp_operation_compose_crop_get_property            (GObject             *object,
                                                                            guint                property_id,
                                                                            GValue              *value,
                                                                            GParamSpec          *pspec);
static void            gimp_operation_compose_crop_set_property            (GObject             *object,
                                                                            guint                property_id,
                                                                            const GValue        *value,
                                                                            GParamSpec          *pspec);

static void            gimp_operation_compose_crop_prepare                 (GeglOperation       *operation);
static GeglRectangle   gimp_operation_compose_crop_get_required_for_output (GeglOperation       *operation,
                                                                            const gchar         *input_pad,
                                                                            const GeglRectangle *output_roi);
static gboolean        gimp_operation_compose_crop_process                 (GeglOperation       *operation,
                                                                            void                *in_buf,
                                                                            void                *aux_buf,
                                                                            void                *out_buf,
                                                                            glong                samples,
                                                                            const GeglRectangle *roi,
                                                                            gint                 level);


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

  operation_class->prepare                 = gimp_operation_compose_crop_prepare;
  operation_class->get_required_for_output = gimp_operation_compose_crop_get_required_for_output;
  point_class->process                     = gimp_operation_compose_crop_process;

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_int ("x",
                                                     "x",
                                                     "x",
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_int ("y",
                                                     "y",
                                                     "y",
                                                     0, G_MAXINT, 0,
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
      g_value_set_int (value, self->x);
      break;
    case PROP_Y:
      g_value_set_int (value, self->y);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, self->w);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, self->h);
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
      self->x = g_value_get_int (value);
      break;
    case PROP_Y:
      self->y = g_value_get_int (value);
      break;
    case PROP_WIDTH:
      self->w = g_value_get_int (value);
      break;
    case PROP_HEIGHT:
      self->h = g_value_get_int (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_compose_crop_prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  if (format)
    {
      const Babl *model = babl_format_get_model (format);

      if (model == babl_model ("R'G'B'A"))
        format = babl_format ("R'G'B'A float");
      else
        format = babl_format ("RGBA float");
    }
  else
    {
      format = babl_format ("RGBA float");
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
    {
      gegl_rectangle_intersect (&result,
                                output_roi,
                                GEGL_RECTANGLE (self->x, self->y,
                                                self->w, self->h));
    }
  else if (! strcmp (input_pad, "aux"))
    {
      gegl_rectangle_subtract_bounding_box (&result,
                                            output_roi,
                                            GEGL_RECTANGLE (self->x, self->y,
                                                            self->w, self->h));
    }
  else
    {
      g_return_val_if_reached (*output_roi);
    }

  return result;
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
  GimpOperationComposeCrop *self = GIMP_OPERATION_COMPOSE_CROP (operation);
  const gfloat       nothing[4] = { 0, };
  const gfloat      *src  = in_buf;
  const gfloat      *aux  = aux_buf;
  gfloat            *dest = out_buf;
  gint               x0 = self->x;
  gint               x1 = self->x + self->w;
  gint               y0 = self->y;
  gint               y1 = self->y + self->h;
  gint               i, j;

  if (! aux)
    aux = nothing;

  for (i = 0; i < roi->height; ++i)
    for (j = 0; j < roi->width; ++j)
      {
        if (i + roi->y < y0 ||
            i + roi->y >= y1 ||
            j + roi->x < x0 ||
            j + roi->x >= x1)
          {
            dest[RED]   = aux[RED];
            dest[GREEN] = aux[GREEN];
            dest[BLUE]  = aux[BLUE];
            dest[ALPHA] = aux[ALPHA];
          }
        else
          {
            dest[RED]   = src[RED];
            dest[GREEN] = src[GREEN];
            dest[BLUE]  = src[BLUE];
            dest[ALPHA] = src[ALPHA];
          }

        src += 4;

        if (aux_buf)
          aux  += 4;

        dest += 4;
      }

  return TRUE;
}
