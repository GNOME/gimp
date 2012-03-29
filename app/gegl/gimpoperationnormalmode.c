/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationnormalmode.c
 * Copyright (C) 2012 Michael Natterer <mitch@gimp.org>
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

#include <gegl-plugin.h>

#include "gimp-gegl-types.h"

#include "gimpoperationnormalmode.h"


static gboolean gimp_operation_normal_parent_process (GeglOperation        *operation,
                                                      GeglOperationContext *context,
                                                      const gchar          *output_prop,
                                                      const GeglRectangle  *result,
                                                      gint                  level);
static gboolean gimp_operation_normal_mode_process   (GeglOperation        *operation,
                                                      void                 *in_buf,
                                                      void                 *aux_buf,
                                                      void                 *out_buf,
                                                      glong                 samples,
                                                      const GeglRectangle  *roi,
                                                      gint                  level);


G_DEFINE_TYPE (GimpOperationNormalMode, gimp_operation_normal_mode,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_normal_mode_parent_class


static void
gimp_operation_normal_mode_class_init (GimpOperationNormalModeClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
        "name"        , "gimp:normal-mode",
        "description" , "GIMP normal mode operation",
        NULL);


  operation_class->process     = gimp_operation_normal_parent_process;

  point_class->process         = gimp_operation_normal_mode_process;
}

static void
gimp_operation_normal_mode_init (GimpOperationNormalMode *self)
{
}

static gboolean
gimp_operation_normal_parent_process (GeglOperation        *operation,
                                      GeglOperationContext *context,
                                      const gchar          *output_prop,
                                      const GeglRectangle  *result,
                                      gint                  level)
{
  const GeglRectangle *in_extent  = NULL;
  const GeglRectangle *aux_extent = NULL;
  GObject             *input;
  GObject             *aux;

  /* get the raw values this does not increase the reference count */
  input = gegl_operation_context_get_object (context, "input");
  aux   = gegl_operation_context_get_object (context, "aux");

  /* pass the input/aux buffers directly through if they are not
   * overlapping
   */
  if (input)
    in_extent = gegl_buffer_get_abyss (GEGL_BUFFER (input));

  if (! input ||
      (aux && ! gegl_rectangle_intersect (NULL, in_extent, result)))
    {
      gegl_operation_context_set_object (context, "output", aux);
      return TRUE;
    }

  if (aux)
    aux_extent = gegl_buffer_get_abyss (GEGL_BUFFER (aux));

  if (! aux ||
      (input && ! gegl_rectangle_intersect (NULL, aux_extent, result)))
    {
      gegl_operation_context_set_object (context, "output", input);
      return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}

static gboolean
gimp_operation_normal_mode_process (GeglOperation       *operation,
                                    void                *in_buf,
                                    void                *aux_buf,
                                    void                *out_buf,
                                    glong                samples,
                                    const GeglRectangle *roi,
                                    gint                 level)
{
  GimpOperationPointLayerMode *point = GIMP_OPERATION_POINT_LAYER_MODE (operation);
  gfloat                      *in    = in_buf;
  gfloat                      *aux   = aux_buf;
  gfloat                      *out   = out_buf;

  if (point->premultiplied)
    {
      while (samples--)
        {
          out[0] = aux[0] + in[0] * (1.0f - aux[3]);
          out[1] = aux[1] + in[1] * (1.0f - aux[3]);
          out[2] = aux[2] + in[2] * (1.0f - aux[3]);
          out[3] = aux[3] + in[3] - aux[3] * in[3];

          in  += 4;
          aux += 4;
          out += 4;
        }
    }
  else
    {
      while (samples--)
        {
          out[RED]   = in[RED]   * (1.0 - aux[ALPHA]) + aux[RED]   * aux[ALPHA];
          out[GREEN] = in[GREEN] * (1.0 - aux[ALPHA]) + aux[GREEN] * aux[ALPHA];
          out[BLUE]  = in[BLUE]  * (1.0 - aux[ALPHA]) + aux[BLUE]  * aux[ALPHA];
          out[ALPHA] = in[ALPHA] + aux[ALPHA] * (1.0 - in[ALPHA]);

          in  += 4;
          aux += 4;
          out += 4;
        }
    }

  return TRUE;
}
