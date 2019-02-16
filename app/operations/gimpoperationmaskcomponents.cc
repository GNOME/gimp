/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationmaskcomponents.c
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

#include <gegl.h>

extern "C"
{

#include "operations-types.h"

#include "gimpoperationmaskcomponents.h"

} /* extern "C" */


enum
{
  PROP_0,
  PROP_MASK
};


static void       gimp_operation_mask_components_get_property (GObject             *object,
                                                               guint                property_id,
                                                               GValue              *value,
                                                               GParamSpec          *pspec);
static void       gimp_operation_mask_components_set_property (GObject             *object,
                                                               guint                property_id,
                                                               const GValue        *value,
                                                               GParamSpec          *pspec);

static void       gimp_operation_mask_components_prepare      (GeglOperation       *operation);
static gboolean gimp_operation_mask_components_parent_process (GeglOperation        *operation,
                                                               GeglOperationContext *context,
                                                               const gchar          *output_prop,
                                                               const GeglRectangle  *result,
                                                               gint                  level);
static gboolean   gimp_operation_mask_components_process      (GeglOperation       *operation,
                                                               void                *in_buf,
                                                               void                *aux_buf,
                                                               void                *out_buf,
                                                               glong                samples,
                                                               const GeglRectangle *roi,
                                                               gint                 level);


G_DEFINE_TYPE (GimpOperationMaskComponents, gimp_operation_mask_components,
               GEGL_TYPE_OPERATION_POINT_COMPOSER)

#define parent_class gimp_operation_mask_components_parent_class


static void
gimp_operation_mask_components_class_init (GimpOperationMaskComponentsClass *klass)
{
  GObjectClass                    *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  object_class->set_property = gimp_operation_mask_components_set_property;
  object_class->get_property = gimp_operation_mask_components_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:mask-components",
                                 "categories",  "gimp",
                                 "description", "Selectively pick components from src or aux",
                                 NULL);

  operation_class->prepare = gimp_operation_mask_components_prepare;
  operation_class->process = gimp_operation_mask_components_parent_process;

  point_class->process     = gimp_operation_mask_components_process;

  g_object_class_install_property (object_class, PROP_MASK,
                                   g_param_spec_flags ("mask",
                                                       "Mask",
                                                       "The component mask",
                                                       GIMP_TYPE_COMPONENT_MASK,
                                                       GIMP_COMPONENT_MASK_ALL,
                                                       (GParamFlags) (
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT)));
}

static void
gimp_operation_mask_components_init (GimpOperationMaskComponents *self)
{
}

static void
gimp_operation_mask_components_get_property (GObject    *object,
                                             guint       property_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  GimpOperationMaskComponents *self = GIMP_OPERATION_MASK_COMPONENTS (object);

  switch (property_id)
    {
    case PROP_MASK:
      g_value_set_flags (value, self->mask);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_mask_components_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  GimpOperationMaskComponents *self = GIMP_OPERATION_MASK_COMPONENTS (object);

  switch (property_id)
    {
    case PROP_MASK:
      self->mask = (GimpComponentMask) g_value_get_flags (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_mask_components_prepare (GeglOperation *operation)
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

static gboolean
gimp_operation_mask_components_parent_process (GeglOperation        *operation,
                                               GeglOperationContext *context,
                                               const gchar          *output_prop,
                                               const GeglRectangle  *result,
                                               gint                  level)
{
  GimpOperationMaskComponents *self = GIMP_OPERATION_MASK_COMPONENTS (operation);

  if (self->mask == 0)
    {
      GObject *input = gegl_operation_context_get_object (context, "input");

      gegl_operation_context_set_object (context, "output", input);

      return TRUE;
    }
  else if (self->mask == GIMP_COMPONENT_MASK_ALL)
    {
      GObject *aux = gegl_operation_context_get_object (context, "aux");

      gegl_operation_context_set_object (context, "output", aux);

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}

static gboolean
gimp_operation_mask_components_process (GeglOperation       *operation,
                                        void                *in_buf,
                                        void                *aux_buf,
                                        void                *out_buf,
                                        glong                samples,
                                        const GeglRectangle *roi,
                                        gint                 level)
{
  GimpOperationMaskComponents *self = GIMP_OPERATION_MASK_COMPONENTS (operation);
  const gfloat                *src  = (const gfloat *) in_buf;
  const gfloat                *aux  = (const gfloat *) aux_buf;
  gfloat                      *dest = (gfloat       *) out_buf;
  GimpComponentMask            mask = self->mask;
  static const gfloat          nothing[] = { 0.0, 0.0, 0.0, 1.0 };

  if (! aux)
    aux = (gfloat *) nothing;

  while (samples--)
    {
      dest[RED]   = (mask & GIMP_COMPONENT_MASK_RED)   ? aux[RED]   : src[RED];
      dest[GREEN] = (mask & GIMP_COMPONENT_MASK_GREEN) ? aux[GREEN] : src[GREEN];
      dest[BLUE]  = (mask & GIMP_COMPONENT_MASK_BLUE)  ? aux[BLUE]  : src[BLUE];
      dest[ALPHA] = (mask & GIMP_COMPONENT_MASK_ALPHA) ? aux[ALPHA] : src[ALPHA];

      src += 4;

      if (aux_buf)
        aux  += 4;

      dest += 4;
    }

  return TRUE;
}
