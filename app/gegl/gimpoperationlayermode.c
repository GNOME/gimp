/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointcomposer.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
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

#include <gegl.h>

#include "gegl-types.h"

#include "gimpoperationlayermode.h"


static gboolean gimp_operation_layer_mode_process (GeglOperation       *operation,
                                                   void                *in_buf,
                                                   void                *aux_buf,
                                                   void                *out_buf,
                                                   glong                samples,
                                                   const GeglRectangle *roi);


G_DEFINE_ABSTRACT_TYPE (GimpOperationLayerMode, gimp_operation_layer_mode,
                        GEGL_TYPE_OPERATION_POINT_COMPOSER)


static void
gimp_operation_layer_mode_class_init (GimpOperationLayerModeClass *klass)
{
  GeglOperationClass              *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointComposerClass *point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  operation_class->categories = "compositors";

  point_class->process        = gimp_operation_layer_mode_process;
}

static void
gimp_operation_layer_mode_init (GimpOperationLayerMode *self)
{
}

static gboolean
gimp_operation_layer_mode_process (GeglOperation       *operation,
                                   void                *in_buf,
                                   void                *aux_buf,
                                   void                *out_buf,
                                   glong                samples,
                                   const GeglRectangle *roi)
{
  return GIMP_OPERATION_LAYER_MODE_GET_CLASS (operation)->process (operation,
                                                                   in_buf,
                                                                   aux_buf,
                                                                   out_buf,
                                                                   samples,
                                                                   roi);
}
