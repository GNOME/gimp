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

template <class T>
struct ProcessGeneric
{
  static void
  process (gconstpointer     in_buf,
           gconstpointer     aux_buf,
           gpointer          out_buf,
           gint              n,
           GimpComponentMask mask)
  {
    T    *out = (T *) out_buf;
    gint  i;
    gint  c;

    if (aux_buf)
      {
        const T *in[4];

        for (c = 0; c < 4; c++)
          {
            if (mask & (1 << c))
              in[c] = (const T *) aux_buf + c;
            else
              in[c] = (const T *) in_buf  + c;
          }

        for (i = 0; i < n; i++)
          {
            for (c = 0; c < 4; c++)
              {
                out[c] = *in[c];

                in[c] += 4;
              }

            out += 4;
          }
      }
    else
      {
        const T *in = (const T*) in_buf;

        for (i = 0; i < n; i++)
          {
            for (c = 0; c < 4; c++)
              {
                if (mask & (1 << c))
                  out[c] = 0;
                else
                  out[c] = in[c];
              }

            in  += 4;
            out += 4;
          }
      }
  }
};

template <class T>
struct Process : ProcessGeneric<T>
{
};

template <>
struct Process<guint8>
{
  static void
  process (gconstpointer     in_buf,
           gconstpointer     aux_buf,
           gpointer          out_buf,
           gint              n,
           GimpComponentMask mask)
  {
    const guint32 *in;
    guint32       *out;
    guint32        in_mask = 0;
    gint           i;
    gint           c;

    if (((guintptr) in_buf | (guintptr) aux_buf | (guintptr) out_buf) % 4)
      {
        ProcessGeneric<guint8>::process (in_buf, aux_buf, out_buf, n, mask);

        return;
      }

    in  = (const guint32 *) in_buf;
    out = (guint32       *) out_buf;

    for (c = 0; c < 4; c++)
      {
        if (! (mask & (1 << c)))
          in_mask |= 0xff << (8 * c);
      }

    if (aux_buf)
      {
        const guint32 *aux      = (const guint32 *) aux_buf;
        guint32        aux_mask = ~in_mask;

        for (i = 0; i < n; i++)
          {
            *out = (*in & in_mask) | (*aux & aux_mask);

            in++;
            aux++;
            out++;
          }
      }
    else
      {
        for (i = 0; i < n; i++)
          {
            *out = *in & in_mask;

            in++;
            out++;
          }
      }
  }
};

template <class T>
static gboolean
gimp_operation_mask_components_process (GimpOperationMaskComponents *self,
                                        void                        *in_buf,
                                        void                        *aux_buf,
                                        void                        *out_buf,
                                        glong                        samples,
                                        const GeglRectangle         *roi,
                                        gint                         level)
{
  Process<T>::process (in_buf, aux_buf, out_buf, samples, self->mask);

  return TRUE;
}

static void
gimp_operation_mask_components_prepare (GeglOperation *operation)
{
  GimpOperationMaskComponents *self = GIMP_OPERATION_MASK_COMPONENTS (operation);
  const Babl                  *format;

  format = gimp_operation_mask_components_get_format (
    gegl_operation_get_source_format (operation, "input"));

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);

  switch (babl_format_get_bytes_per_pixel (format))
    {
    case 4:
      self->process = (gpointer)
                        gimp_operation_mask_components_process<guint8>;
      break;

    case 8:
      self->process = (gpointer)
                        gimp_operation_mask_components_process<guint16>;
      break;

    case 16:
      self->process = (gpointer)
                        gimp_operation_mask_components_process<guint32>;
      break;

    default:
      g_return_if_reached ();
    }
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
  typedef gboolean (* ProcessFunc) (GimpOperationMaskComponents *self,
                                    void                        *in_buf,
                                    void                        *aux_buf,
                                    void                        *out_buf,
                                    glong                        samples,
                                    const GeglRectangle         *roi,
                                    gint                         level);

  GimpOperationMaskComponents *self = (GimpOperationMaskComponents *) operation;

  return ((ProcessFunc) self->process) (self,
                                        in_buf, aux_buf, out_buf, samples,
                                        roi, level);
}

const Babl *
gimp_operation_mask_components_get_format (const Babl *input_format)
{
  const Babl *format = NULL;

  if (input_format)
    {
      const Babl  *model      = babl_format_get_model (input_format);
      const gchar *model_name = babl_get_name (model);
      const Babl  *type       = babl_format_get_type (input_format, 0);
      const gchar *type_name  = babl_get_name (type);

      if (! strcmp (model_name, "Y")   ||
          ! strcmp (model_name, "YA")  ||
          ! strcmp (model_name, "RGB") ||
          ! strcmp (model_name, "RGBA"))
        {
          if (! strcmp (type_name, "u8"))
            format = babl_format ("RGBA u8");
          else if (! strcmp (type_name, "u16"))
            format = babl_format ("RGBA u16");
          else if (! strcmp (type_name, "u32"))
            format = babl_format ("RGBA u32");
          else if (! strcmp (type_name, "half"))
            format = babl_format ("RGBA half");
          else if (! strcmp (type_name, "float"))
            format = babl_format ("RGBA float");
        }
      else if (! strcmp (model_name, "Y'")      ||
               ! strcmp (model_name, "Y'A")     ||
               ! strcmp (model_name, "R'G'B'")  ||
               ! strcmp (model_name, "R'G'B'A") ||
               babl_format_is_palette (input_format))
        {
          if (! strcmp (type_name, "u8"))
            format = babl_format ("R'G'B'A u8");
          else if (! strcmp (type_name, "u16"))
            format = babl_format ("R'G'B'A u16");
          else if (! strcmp (type_name, "u32"))
            format = babl_format ("R'G'B'A u32");
          else if (! strcmp (type_name, "half"))
            format = babl_format ("R'G'B'A half");
          else if (! strcmp (type_name, "float"))
            format = babl_format ("R'G'B'A float");
        }
    }

  if (! format)
    format = babl_format ("RGBA float");

  return format;
}

void
gimp_operation_mask_components_process (const Babl        *format,
                                        gconstpointer      in,
                                        gconstpointer      aux,
                                        gpointer           out,
                                        gint               n,
                                        GimpComponentMask  mask)
{
  g_return_if_fail (format != NULL);
  g_return_if_fail (in != NULL);
  g_return_if_fail (out != NULL);
  g_return_if_fail (n >= 0);

  switch (babl_format_get_bytes_per_pixel (format))
    {
    case 4:
      Process<guint8>::process (in, aux, out, n, mask);
      break;

    case 8:
      Process<guint16>::process (in, aux, out, n, mask);
      break;

    case 16:
      Process<guint32>::process (in, aux, out, n, mask);
      break;

    default:
      g_return_if_reached ();
    }
}
