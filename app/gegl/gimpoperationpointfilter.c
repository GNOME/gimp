/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationpointfilter.c
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "gimp-gegl-types.h"

#include "gimpoperationpointfilter.h"


static void   gimp_operation_point_filter_finalize (GObject *object);


G_DEFINE_TYPE (GimpOperationPointFilter, gimp_operation_point_filter,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_point_filter_parent_class


static void
gimp_operation_point_filter_class_init (GimpOperationPointFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_operation_point_filter_finalize;
}

static void
gimp_operation_point_filter_init (GimpOperationPointFilter *self)
{
}

static void
gimp_operation_point_filter_finalize (GObject *object)
{
  GimpOperationPointFilter *self = GIMP_OPERATION_POINT_FILTER (object);

  if (self->config)
    {
      g_object_unref (self->config);
      self->config = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

void
gimp_operation_point_filter_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  GimpOperationPointFilter *self = GIMP_OPERATION_POINT_FILTER (object);

  switch (property_id)
    {
    case GIMP_OPERATION_POINT_FILTER_PROP_CONFIG:
      g_value_set_object (value, self->config);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_operation_point_filter_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  GimpOperationPointFilter *self = GIMP_OPERATION_POINT_FILTER (object);

  switch (property_id)
    {
    case GIMP_OPERATION_POINT_FILTER_PROP_CONFIG:
      if (self->config)
        g_object_unref (self->config);
      self->config = g_value_dup_object (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
