/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdodgemode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
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

#include "gimpoperationdodgemode.h"


static void     gimp_operation_dodge_mode_prepare (GeglOperation       *operation);
static gboolean gimp_operation_dodge_mode_process (GeglOperation       *operation,
                                                   void                *in_buf,
                                                   void                *aux_buf,
                                                   void                *out_buf,
                                                   glong                samples,
                                                   const GeglRectangle *roi,
                                                   gint                 level);


G_DEFINE_TYPE (GimpOperationDodgeMode, gimp_operation_dodge_mode,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_dodge_mode_class_init (GimpOperationDodgeModeClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:dodge-mode",
                                 "description", "GIMP dodge mode operation",
                                 NULL);

  point_class->process     = gimp_operation_dodge_mode_process;
  operation_class->prepare = gimp_operation_dodge_mode_prepare;
}

static void
gimp_operation_dodge_mode_init (GimpOperationDodgeMode *self)
{
}

static void
gimp_operation_dodge_mode_prepare (GeglOperation *operation)
{
  const Babl *format = babl_format ("R'G'B'A float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
gimp_operation_dodge_mode_process (GeglOperation       *operation,
                                   void                *in_buf,
                                   void                *aux_buf,
                                   void                *out_buf,
                                   glong                samples,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  gfloat *in    = in_buf;
  gfloat *layer = aux_buf;
  gfloat *out   = out_buf;

  while (samples--)
    {
      gint b;
      gfloat comp_alpha = in[ALPHA] * layer[ALPHA];
      gfloat new_alpha  = in[ALPHA] + (1 - in[ALPHA]) * comp_alpha;
      gfloat ratio      = comp_alpha / new_alpha;

      for (b = RED; b < ALPHA; b++)
        {
          gfloat comp = MIN (in[b] / (1 - layer[b]), 1);

          out[b] = comp * ratio + in[b] * (1 - ratio);
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;
    }

  return TRUE;
}
