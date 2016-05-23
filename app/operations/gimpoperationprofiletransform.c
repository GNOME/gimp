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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Ported from the threshold-alpha plug-in
 * Copyright (C) 1997 Shuji Narazaki <narazaki@InetQ.or.jp>
 */

#include "config.h"

#include <cairo.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <lcms2.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"

#include "operations-types.h"

#include "gimpoperationprofiletransform.h"


enum
{
  PROP_0,
  PROP_SRC_PROFILE,
  PROP_DEST_PROFILE,
  PROP_INTENT,
  PROP_BPC
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

  g_object_class_install_property (object_class, PROP_DEST_PROFILE,
                                   g_param_spec_object ("dest-profile",
                                                        "Destination Profile",
                                                        "Destination Profile",
                                                        GIMP_TYPE_COLOR_PROFILE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_INTENT,
                                   g_param_spec_enum ("intent",
                                                      "Rendering Intent",
                                                      "Rendering Intent",
                                                      GIMP_TYPE_COLOR_RENDERING_INTENT,
                                                      GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_BPC,
                                   g_param_spec_boolean ("blc",
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

  if (self->src_profile)
    {
      g_object_unref (self->src_profile);
      self->src_profile = NULL;
    }

  if (self->dest_profile)
    {
      g_object_unref (self->dest_profile);
      self->dest_profile = NULL;
    }

  if (self->transform)
    {
      cmsDeleteTransform (self->transform);
      self->transform = NULL;
    }

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

    case PROP_DEST_PROFILE:
      g_value_set_object (value, self->dest_profile);
      break;

    case PROP_INTENT:
      g_value_set_enum (value, self->intent);
      break;

    case PROP_BPC:
      g_value_set_boolean (value, self->bpc);
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
      self->src_profile = g_value_get_object (value);
      break;

    case PROP_DEST_PROFILE:
      if (self->dest_profile)
        g_object_unref (self->dest_profile);
      self->dest_profile = g_value_get_object (value);
      break;

    case PROP_INTENT:
      self->intent = g_value_get_enum (value);
      break;

    case PROP_BPC:
      self->bpc = g_value_get_boolean (value);
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
  const Babl                    *format;

  format = gegl_operation_get_source_format (operation, "input");

  if (! format)
    format = babl_format ("RGBA float");

  if (self->transform)
    {
      cmsDeleteTransform (self->transform);
      self->transform = NULL;
    }

  self->src_format  = format;
  self->dest_format = format;

  if (self->src_profile && self->dest_profile)
    {
      cmsHPROFILE     src_lcms;
      cmsHPROFILE     dest_lcms;
      cmsUInt32Number lcms_src_format;
      cmsUInt32Number lcms_dest_format;
      cmsUInt32Number flags;

      src_lcms  = gimp_color_profile_get_lcms_profile (self->src_profile);
      dest_lcms = gimp_color_profile_get_lcms_profile (self->dest_profile);

      self->src_format  = gimp_color_profile_get_format (format,
                                                         &lcms_src_format);
      self->dest_format = gimp_color_profile_get_format (format,
                                                         &lcms_dest_format);

      flags = cmsFLAGS_NOOPTIMIZE;

      if (self->bpc)
        flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;

      self->transform = cmsCreateTransform (src_lcms,  lcms_src_format,
                                            dest_lcms, lcms_dest_format,
                                            self->intent, flags);
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
      /* copy the alpha channel */
      if (babl_format_has_alpha (self->dest_format))
        babl_process (babl_fish (self->src_format,
                                 self->dest_format),
                      src, dest, samples);

      cmsDoTransform (self->transform, src, dest, samples);
    }
  else
    {
      babl_process (babl_fish (self->src_format,
                               self->dest_format),
                    src, dest, samples);
    }

  return TRUE;
}
