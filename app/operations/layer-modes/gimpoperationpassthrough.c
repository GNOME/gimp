/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpassthrough.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2017 Ell
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimp-layer-modes.h"
#include "gimpoperationpassthrough.h"


static gboolean   gimp_operation_pass_through_parent_process (GeglOperation        *operation,
                                                              GeglOperationContext *context,
                                                              const gchar          *output_prop,
                                                              const GeglRectangle  *result,
                                                              gint                  level);


G_DEFINE_TYPE (GimpOperationPassThrough, gimp_operation_pass_through,
               GIMP_TYPE_OPERATION_REPLACE)

#define parent_class gimp_operation_pass_through_parent_class


static void
gimp_operation_pass_through_class_init (GimpOperationPassThroughClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *layer_mode_class = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:pass-through",
                                 "description", "GIMP pass through mode operation",
                                 NULL);

  operation_class->process              = gimp_operation_pass_through_parent_process;

  /* don't use REPLACE mode's specialized get_affected_region(); PASS_THROUGH
   * behaves like an ordinary layer mode here.
   */
  layer_mode_class->get_affected_region = NULL;
}

static void
gimp_operation_pass_through_init (GimpOperationPassThrough *self)
{
}

static gboolean
gimp_operation_pass_through_parent_process (GeglOperation        *operation,
                                            GeglOperationContext *context,
                                            const gchar          *output_prop,
                                            const GeglRectangle  *result,
                                            gint                  level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) operation;

  /* if the layer's opacity is 100%, it has no mask, and its composite mode
   * contains "aux" (the latter should always be the case for pass through
   * mode,) we can just pass "aux" directly as output.  note that the same
   * optimization would more generally apply to REPLACE mode, save for the fact
   * that when both the backdrop and the layer have a pixel with 0% alpha, we
   * want to maintain the color value of the backdrop, not the layer; since,
   * for pass through groups, the layer is already composited against the
   * backdrop, such pixels will have the same color value for both the backdrop
   * and the layer.
   */
  if (layer_mode->opacity == 1.0                            &&
      ! gegl_operation_context_get_object (context, "aux2") &&
      (gimp_layer_mode_get_included_region (layer_mode->layer_mode,
                                            layer_mode->real_composite_mode) &
       GIMP_LAYER_COMPOSITE_REGION_SOURCE))
    {
      GObject *aux;

      aux = gegl_operation_context_get_object (context, "aux");

      gegl_operation_context_set_object (context, "output", aux);

      return TRUE;
    }

  return GEGL_OPERATION_CLASS (parent_class)->process (operation, context,
                                                       output_prop, result,
                                                       level);
}
