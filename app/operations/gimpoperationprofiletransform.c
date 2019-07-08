/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationprofiletransform.c
 * Copyright (C) 2016 Michael Natterer <mitch@gimp.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"

#include "operations-types.h"

#include "gimpoperationprofiletransform.h"


enum
{
  PROP_0,
  PROP_SRC_PROFILE,
  PROP_SRC_FORMAT,
  PROP_DEST_PROFILE,
  PROP_DEST_FORMAT,
  PROP_RENDERING_INTENT,
  PROP_BLACK_POINT_COMPENSATION
};


static void       gimp_operation_profile_transform_finalize     (GObject             *object);

static void       gimp_operation_profile_transform_get_property (GObject             *object,
                                                                 guint                property_id,
                                                                 GValue              *value,
                                                                 GParamSpec          *pspec);
static void       gimp_operation_profile_transform_set_property (GObject             *object,
                                                                 guint                property_id,
                                                                 const GValue        *value,
                                                                 GParamSpec          *pspec);

static void       gimp_operation_profile_transform_prepare      (GeglOperation       *operation);
static gboolean   gimp_operation_profile_transform_process      (GeglOperation       *operation,
                                                                 void                *in_buf,
                                                                 void                *out_buf,
                                                                 glong                samples,
                                                                 const GeglRectangle *roi,
                                                                 gint                 level);


G_DEFINE_TYPE (GimpOperationProfileTransform, gimp_operation_profile_transform,
               GEGL_TYPE_OPERATION_POINT_FILTER)

#define parent_class gimp_operation_profile_transform_parent_class


static void
gimp_operation_profile_transform_class_init (GimpOperationProfileTransformClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->finalize     = gimp_operation_profile_transform_finalize;
  object_class->set_property = gimp_operation_profile_transform_set_property;
  object_class->get_property = gimp_operation_profile_transform_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:profile-transform",
                                 "categories",  "color",
                                 "description",
                                 "Transform between two color profiles",
                                 NULL);

  operation_class->prepare = gimp_operation_profile_transform_prepare;

  point_class->process     = gimp_operation_profile_transform_process;

  g_object_class_install_property (object_class, PROP_SRC_PROFILE,
                                   g_param_spec_object ("src-profile",
                                                        "Source Profile",
                                                        "Source Profile",
                                                        GIMP_TYPE_COLOR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SRC_FORMAT,
                                   g_param_spec_pointer ("src-format",
                                                         "Source Format",
                                                         "Source Format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DEST_PROFILE,
                                   g_param_spec_object ("dest-profile",
                                                        "Destination Profile",
                                                        "Destination Profile",
                                                        GIMP_TYPE_COLOR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DEST_FORMAT,
                                   g_param_spec_pointer ("dest-format",
                                                         "Destination Format",
                                                         "Destination Format",
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RENDERING_INTENT,
                                   g_param_spec_enum ("rendering-intent",
                                                      "Rendering Intent",
                                                      "Rendering Intent",
                                                      GIMP_TYPE_COLOR_RENDERING_INTENT,
                                                      GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BLACK_POINT_COMPENSATION,
                                   g_param_spec_boolean ("black-point-compensation",
                                                         "Black Point Compensation",
                                                         "Black Point Compensation",
                                                         TRUE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));
}

static void
gimp_operation_profile_transform_init (GimpOperationProfileTransform *self)
{
}

static void
gimp_operation_profile_transform_finalize (GObject *object)
{
  GimpOperationProfileTransform *self = GIMP_OPERATION_PROFILE_TRANSFORM (object);

  g_clear_object (&self->src_profile);
  g_clear_object (&self->dest_profile);
  g_clear_object (&self->transform);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_operation_profile_transform_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
  GimpOperationProfileTransform *self = GIMP_OPERATION_PROFILE_TRANSFORM (object);

  switch (property_id)
    {
    case PROP_SRC_PROFILE:
      g_value_set_object (value, self->src_profile);
      break;

    case PROP_SRC_FORMAT:
      g_value_set_pointer (value, (gpointer) self->src_format);
      break;

    case PROP_DEST_PROFILE:
      g_value_set_object (value, self->dest_profile);
      break;

    case PROP_DEST_FORMAT:
      g_value_set_pointer (value, (gpointer) self->dest_format);
      break;

    case PROP_RENDERING_INTENT:
      g_value_set_enum (value, self->rendering_intent);
      break;

    case PROP_BLACK_POINT_COMPENSATION:
      g_value_set_boolean (value, self->black_point_compensation);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_profile_transform_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
  GimpOperationProfileTransform *self = GIMP_OPERATION_PROFILE_TRANSFORM (object);

  switch (property_id)
    {
    case PROP_SRC_PROFILE:
      if (self->src_profile)
        g_object_unref (self->src_profile);
      self->src_profile = g_value_dup_object (value);
      break;

    case PROP_SRC_FORMAT:
      self->src_format = g_value_get_pointer (value);
      break;

    case PROP_DEST_PROFILE:
      if (self->dest_profile)
        g_object_unref (self->dest_profile);
      self->dest_profile = g_value_dup_object (value);
      break;

    case PROP_DEST_FORMAT:
      self->dest_format = g_value_get_pointer (value);
      break;

    case PROP_RENDERING_INTENT:
      self->rendering_intent = g_value_get_enum (value);
      break;

    case PROP_BLACK_POINT_COMPENSATION:
      self->black_point_compensation = g_value_get_boolean (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_operation_profile_transform_prepare (GeglOperation *operation)
{
  GimpOperationProfileTransform *self = GIMP_OPERATION_PROFILE_TRANSFORM (operation);

  g_clear_object (&self->transform);

  if (! self->src_format)
    self->src_format = babl_format ("RGBA float");

  if (! self->dest_format)
    self->dest_format = babl_format ("RGBA float");

  if (self->src_profile && self->dest_profile)
    {
      GimpColorTransformFlags flags = 0;

      if (self->black_point_compensation)
        flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      flags |= GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;

      self->transform = gimp_color_transform_new (self->src_profile,
                                                  self->src_format,
                                                  self->dest_profile,
                                                  self->dest_format,
                                                  self->rendering_intent,
                                                  flags);
    }

  gegl_operation_set_format (operation, "input",  self->src_format);
  gegl_operation_set_format (operation, "output", self->dest_format);
}

static gboolean
gimp_operation_profile_transform_process (GeglOperation       *operation,
                                          void                *in_buf,
                                          void                *out_buf,
                                          glong                samples,
                                          const GeglRectangle *roi,
                                          gint                 level)
{
  GimpOperationProfileTransform *self = GIMP_OPERATION_PROFILE_TRANSFORM (operation);
  gpointer                      *src  = in_buf;
  gpointer                      *dest = out_buf;

  if (self->transform)
    {
      gimp_color_transform_process_pixels (self->transform,
                                           self->src_format,
                                           src,
                                           self->dest_format,
                                           dest,
                                           samples);
    }
  else
    {
      babl_process (babl_fish (self->src_format,
                               self->dest_format),
                    src, dest, samples);
    }

  return TRUE;
}
