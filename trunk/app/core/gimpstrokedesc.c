/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpstrokedesc.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimpcontainer.h"
#include "gimpcontext.h"
#include "gimppaintinfo.h"
#include "gimpstrokedesc.h"
#include "gimpstrokeoptions.h"

#include "paint/gimppaintoptions.h"


enum
{
  PROP_0,
  PROP_GIMP,
  PROP_METHOD,
  PROP_STROKE_OPTIONS,
  PROP_PAINT_INFO,
  PROP_PAINT_OPTIONS
};


static void      gimp_stroke_desc_config_iface_init (gpointer    iface,
                                                     gpointer    iface_data);

static GObject * gimp_stroke_desc_constructor  (GType            type,
                                                guint            n_params,
                                                GObjectConstructParam *params);
static void      gimp_stroke_desc_finalize     (GObject         *object);
static void      gimp_stroke_desc_set_property (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);
static void      gimp_stroke_desc_get_property (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);

static GimpConfig * gimp_stroke_desc_duplicate (GimpConfig      *config);


G_DEFINE_TYPE_WITH_CODE (GimpStrokeDesc, gimp_stroke_desc, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_stroke_desc_config_iface_init))

#define parent_class gimp_stroke_desc_parent_class


static void
gimp_stroke_desc_class_init (GimpStrokeDescClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor  = gimp_stroke_desc_constructor;
  object_class->finalize     = gimp_stroke_desc_finalize;
  object_class->set_property = gimp_stroke_desc_set_property;
  object_class->get_property = gimp_stroke_desc_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_METHOD,
                                 "method", NULL,
                                 GIMP_TYPE_STROKE_METHOD,
                                 GIMP_STROKE_METHOD_LIBART,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_STROKE_OPTIONS,
                                   "stroke-options", NULL,
                                   GIMP_TYPE_STROKE_OPTIONS,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_AGGREGATE);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_PAINT_INFO,
                                   "paint-info", NULL,
                                   GIMP_TYPE_PAINT_INFO,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_PAINT_OPTIONS,
                                   "paint-options", NULL,
                                   GIMP_TYPE_PAINT_OPTIONS,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_stroke_desc_config_iface_init (gpointer  iface,
                                    gpointer  iface_data)
{
  GimpConfigInterface *config_iface = (GimpConfigInterface *) iface;

  config_iface->duplicate = gimp_stroke_desc_duplicate;
}

static void
gimp_stroke_desc_init (GimpStrokeDesc *desc)
{
}

static GObject *
gimp_stroke_desc_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
  GObject        *object;
  GimpStrokeDesc *desc;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  desc = GIMP_STROKE_DESC (object);

  g_assert (GIMP_IS_GIMP (desc->gimp));

  desc->stroke_options = g_object_new (GIMP_TYPE_STROKE_OPTIONS,
                                       "gimp", desc->gimp,
                                       NULL);

  return object;
}

static void
gimp_stroke_desc_finalize (GObject *object)
{
  GimpStrokeDesc *desc = GIMP_STROKE_DESC (object);

  if (desc->stroke_options)
    {
      g_object_unref (desc->stroke_options);
      desc->stroke_options = NULL;
    }

  if (desc->paint_info)
    {
      g_object_unref (desc->paint_info);
      desc->paint_info = NULL;
    }

  if (desc->paint_options)
    {
      g_object_unref (desc->paint_options);
      desc->paint_options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_stroke_desc_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpStrokeDesc *desc = GIMP_STROKE_DESC (object);

  switch (property_id)
    {
    case PROP_GIMP:
      desc->gimp = g_value_get_object (value);
      break;
    case PROP_METHOD:
      desc->method = g_value_get_enum (value);
      break;
    case PROP_STROKE_OPTIONS:
      if (g_value_get_object (value))
        gimp_config_sync (g_value_get_object (value),
                          G_OBJECT (desc->stroke_options), 0);
      break;
    case PROP_PAINT_INFO:
      if (desc->paint_info)
        g_object_unref (desc->paint_info);
      desc->paint_info = (GimpPaintInfo *) g_value_dup_object (value);
      break;
    case PROP_PAINT_OPTIONS:
      if (desc->paint_options)
        g_object_unref (desc->paint_options);
      desc->paint_options = (GimpPaintOptions *) g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_stroke_desc_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpStrokeDesc *desc = GIMP_STROKE_DESC (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, desc->gimp);
      break;
    case PROP_METHOD:
      g_value_set_enum (value, desc->method);
      break;
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, desc->stroke_options);
      break;
    case PROP_PAINT_INFO:
      g_value_set_object (value, desc->paint_info);
      break;
    case PROP_PAINT_OPTIONS:
      g_value_set_object (value, desc->paint_options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GimpConfig *
gimp_stroke_desc_duplicate (GimpConfig *config)
{
  GimpStrokeDesc *desc = GIMP_STROKE_DESC (config);
  GimpStrokeDesc *new_desc;

  new_desc = gimp_stroke_desc_new (desc->gimp,
                                   GIMP_CONTEXT (desc->stroke_options));

  gimp_config_sync (G_OBJECT (desc->stroke_options),
                    G_OBJECT (new_desc->stroke_options), 0);
  gimp_config_sync (G_OBJECT (desc->paint_info),
                    G_OBJECT (new_desc->paint_info), 0);

  if (desc->paint_options)
    {
      GObject *options;

      options = gimp_config_duplicate (GIMP_CONFIG (desc->paint_options));
      g_object_set (new_desc, "paint-options", options, NULL);
      g_object_unref (options);
    }

  return GIMP_CONFIG (new_desc);
}


/*  public functions  */

GimpStrokeDesc *
gimp_stroke_desc_new (Gimp        *gimp,
                      GimpContext *context)
{
  GimpPaintInfo  *paint_info = NULL;
  GimpStrokeDesc *desc;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);

  if (context)
    {
      paint_info = gimp_context_get_paint_info (context);

      if (! paint_info)
        paint_info = gimp_paint_info_get_standard (gimp);
    }

  desc = g_object_new (GIMP_TYPE_STROKE_DESC,
                       "gimp",       gimp,
                       "paint-info", paint_info,
                       NULL);

  gimp_context_define_properties (GIMP_CONTEXT (desc->stroke_options),
                                  GIMP_CONTEXT_FOREGROUND_MASK |
                                  GIMP_CONTEXT_PATTERN_MASK,
                                  FALSE);

  if (context)
    gimp_context_set_parent (GIMP_CONTEXT (desc->stroke_options), context);

  return desc;
}

void
gimp_stroke_desc_prepare (GimpStrokeDesc *desc,
                          GimpContext    *context,
                          gboolean        use_default_values)
{
  g_return_if_fail (GIMP_IS_STROKE_DESC (desc));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  switch (desc->method)
    {
    case GIMP_STROKE_METHOD_LIBART:
      break;

    case GIMP_STROKE_METHOD_PAINT_CORE:
      {
        GimpPaintInfo    *paint_info = desc->paint_info;
        GimpPaintOptions *paint_options;

        if (use_default_values)
          {
            paint_options = gimp_paint_options_new (paint_info);

            /*  undefine the paint-relevant context properties and get them
             *  from the passed context
             */
            gimp_context_define_properties (GIMP_CONTEXT (paint_options),
                                            GIMP_CONTEXT_PAINT_PROPS_MASK,
                                            FALSE);
            gimp_context_set_parent (GIMP_CONTEXT (paint_options), context);
          }
        else
          {
            GimpCoreConfig      *config       = context->gimp->config;
            GimpContextPropMask  global_props = 0;

            paint_options =
              gimp_config_duplicate (GIMP_CONFIG (paint_info->paint_options));

            /*  FG and BG are always shared between all tools  */
            global_props |= GIMP_CONTEXT_FOREGROUND_MASK;
            global_props |= GIMP_CONTEXT_BACKGROUND_MASK;

            if (config->global_brush)
              global_props |= GIMP_CONTEXT_BRUSH_MASK;
            if (config->global_pattern)
              global_props |= GIMP_CONTEXT_PATTERN_MASK;
            if (config->global_palette)
              global_props |= GIMP_CONTEXT_PALETTE_MASK;
            if (config->global_gradient)
              global_props |= GIMP_CONTEXT_GRADIENT_MASK;
            if (config->global_font)
              global_props |= GIMP_CONTEXT_FONT_MASK;

            gimp_context_copy_properties (context,
                                          GIMP_CONTEXT (paint_options),
                                          global_props);
          }

        g_object_set (desc, "paint-options", paint_options, NULL);
        g_object_unref (paint_options);
      }
      break;

    default:
      g_return_if_reached ();
    }
}

void
gimp_stroke_desc_finish (GimpStrokeDesc *desc)
{
  g_return_if_fail (GIMP_IS_STROKE_DESC (desc));

  g_object_set (desc, "paint-options", NULL, NULL);
}
