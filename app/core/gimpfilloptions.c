/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpfilloptions.c
 * Copyright (C) 2003 Simon Budig
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "operations/layer-modes/gimp-layer-modes.h"

#include "gimp.h"
#include "gimp-palettes.h"
#include "gimpdrawable.h"
#include "gimpdrawable-fill.h"
#include "gimperror.h"
#include "gimpfilloptions.h"
#include "gimpimage.h"
#include "gimppattern.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_CUSTOM_STYLE,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE
};


typedef struct _GimpFillOptionsPrivate GimpFillOptionsPrivate;

struct _GimpFillOptionsPrivate
{
  GimpFillStyle   style;
  GimpCustomStyle custom_style;
  gboolean        antialias;
  gboolean        feather;
  gdouble         feather_radius;

  GimpViewType  pattern_view_type;
  GimpViewSize  pattern_view_size;

  const gchar  *undo_desc;
};

#define GET_PRIVATE(options) \
        ((GimpFillOptionsPrivate *) gimp_fill_options_get_instance_private ((GimpFillOptions *) (options)))


static void     gimp_fill_options_config_init  (GimpConfigInterface *iface);

static void     gimp_fill_options_set_property (GObject             *object,
                                                guint                property_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);
static void     gimp_fill_options_get_property (GObject             *object,
                                                guint                property_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);

static gboolean gimp_fill_options_serialize    (GimpConfig          *config,
                                                GimpConfigWriter    *writer,
                                                gpointer             data);


G_DEFINE_TYPE_WITH_CODE (GimpFillOptions, gimp_fill_options, GIMP_TYPE_CONTEXT,
                         G_ADD_PRIVATE (GimpFillOptions)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG,
                                                gimp_fill_options_config_init))


static void
gimp_fill_options_class_init (GimpFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_fill_options_set_property;
  object_class->get_property = gimp_fill_options_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Style"),
                         NULL,
                         GIMP_TYPE_FILL_STYLE,
                         GIMP_FILL_STYLE_FG_COLOR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_CUSTOM_STYLE,
                         "custom-style",
                         _("Custom style"),
                         NULL,
                         GIMP_TYPE_CUSTOM_STYLE,
                         GIMP_CUSTOM_STYLE_SOLID_COLOR,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            NULL,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of fill edges"),
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           GIMP_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_TYPE,
                                   g_param_spec_enum ("pattern-view-type",
                                                      NULL, NULL,
                                                      GIMP_TYPE_VIEW_TYPE,
                                                      GIMP_VIEW_TYPE_GRID,
                                                      G_PARAM_CONSTRUCT |
                                                      GIMP_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_SIZE,
                                   g_param_spec_int ("pattern-view-size",
                                                     NULL, NULL,
                                                     GIMP_VIEW_SIZE_TINY,
                                                     GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                                     GIMP_VIEW_SIZE_SMALL,
                                                     G_PARAM_CONSTRUCT |
                                                     GIMP_PARAM_READWRITE));
}

static void
gimp_fill_options_config_init (GimpConfigInterface *iface)
{
  iface->serialize = gimp_fill_options_serialize;
}

static void
gimp_fill_options_init (GimpFillOptions *options)
{
}

static void
gimp_fill_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      private->style = g_value_get_enum (value);
      private->undo_desc = NULL;
      break;
    case PROP_CUSTOM_STYLE:
      private->custom_style = g_value_get_enum (value);
      private->undo_desc = NULL;
      break;
    case PROP_ANTIALIAS:
      private->antialias = g_value_get_boolean (value);
      break;
    case PROP_FEATHER:
      private->feather = g_value_get_boolean (value);
      break;
    case PROP_FEATHER_RADIUS:
      private->feather_radius = g_value_get_double (value);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      private->pattern_view_type = g_value_get_enum (value);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      private->pattern_view_size = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_fill_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, private->style);
      break;
    case PROP_CUSTOM_STYLE:
      g_value_set_enum (value, private->custom_style);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, private->antialias);
      break;
    case PROP_FEATHER:
      g_value_set_boolean (value, private->feather);
      break;
    case PROP_FEATHER_RADIUS:
      g_value_set_double (value, private->feather_radius);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      g_value_set_enum (value, private->pattern_view_type);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      g_value_set_int (value, private->pattern_view_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
gimp_fill_options_serialize (GimpConfig       *config,
                             GimpConfigWriter *writer,
                             gpointer          data)
{
  return gimp_config_serialize_properties (config, writer);
}


/*  public functions  */

GimpFillOptions *
gimp_fill_options_new (Gimp        *gimp,
                       GimpContext *context,
                       gboolean     use_context_color)
{
  GimpFillOptions *options;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (context == NULL || GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  options = g_object_new (GIMP_TYPE_FILL_OPTIONS,
                          "gimp", gimp,
                          NULL);

  if (use_context_color)
    {
      gimp_context_define_properties (GIMP_CONTEXT (options),
                                      GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                                      GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                                      GIMP_CONTEXT_PROP_MASK_PATTERN,
                                      FALSE);

      gimp_context_set_parent (GIMP_CONTEXT (options), context);
    }

  return options;
}

GimpFillStyle
gimp_fill_options_get_style (GimpFillOptions *options)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), GIMP_FILL_STYLE_FG_COLOR);

  return GET_PRIVATE (options)->style;
}

void
gimp_fill_options_set_style (GimpFillOptions *options,
                             GimpFillStyle    style)
{
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  g_object_set (options, "style", style, NULL);
}

GimpCustomStyle
gimp_fill_options_get_custom_style (GimpFillOptions *options)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options),
                        GIMP_CUSTOM_STYLE_SOLID_COLOR);

  return GET_PRIVATE (options)->custom_style;
}

void
gimp_fill_options_set_custom_style (GimpFillOptions *options,
                                    GimpCustomStyle  custom_style)
{
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  g_object_set (options, "custom-style", custom_style, NULL);
}

gboolean
gimp_fill_options_get_antialias (GimpFillOptions *options)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->antialias;
}

void
gimp_fill_options_set_antialias (GimpFillOptions *options,
                                 gboolean         antialias)
{
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  g_object_set (options, "antialias", antialias, NULL);
}

gboolean
gimp_fill_options_get_feather (GimpFillOptions *options,
                               gdouble         *radius)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);

  if (radius)
    *radius = GET_PRIVATE (options)->feather_radius;

  return GET_PRIVATE (options)->feather;
}

void
gimp_fill_options_set_feather (GimpFillOptions *options,
                               gboolean         feather,
                               gdouble          radius)
{
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));

  g_object_set (options, "feather", feather, NULL);
  g_object_set (options, "feather-radius", radius, NULL);
}

gboolean
gimp_fill_options_set_by_fill_type (GimpFillOptions  *options,
                                    GimpContext      *context,
                                    GimpFillType      fill_type,
                                    GError          **error)
{
  GimpFillOptionsPrivate *private;
  GeglColor              *color = NULL;
  const gchar            *undo_desc;

  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GET_PRIVATE (options);

  private->undo_desc = NULL;

  switch (fill_type)
    {
    case GIMP_FILL_FOREGROUND:
      color = gegl_color_duplicate (gimp_context_get_foreground (context));
      undo_desc = C_("undo-type", "Fill with Foreground Color");
      break;

    case GIMP_FILL_BACKGROUND:
      color = gegl_color_duplicate (gimp_context_get_background (context));
      undo_desc = C_("undo-type", "Fill with Background Color");
      break;

    case GIMP_FILL_CIELAB_MIDDLE_GRAY:
      {
        const float cielab_pixel[3] = {50, 0, 0};

        color = gegl_color_new (NULL);
        gegl_color_set_pixel (color, babl_format ("CIE Lab float"), cielab_pixel);

        undo_desc = C_("undo-type", "Fill with Middle Gray (CIELAB) Color");
      }
      break;

    case GIMP_FILL_WHITE:
      color = gegl_color_new ("white");
      undo_desc = C_("undo-type", "Fill with White");
      break;

    case GIMP_FILL_TRANSPARENT:
      color = gegl_color_duplicate (gimp_context_get_background (context));
      gimp_context_set_paint_mode (GIMP_CONTEXT (options),
                                   GIMP_LAYER_MODE_ERASE);
      undo_desc = C_("undo-type", "Fill with Transparency");
      break;

    case GIMP_FILL_PATTERN:
      {
        GimpPattern *pattern = gimp_context_get_pattern (context);

        if (! pattern)
          {
            g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                                 _("No patterns available for this operation."));
            return FALSE;
          }

        gimp_fill_options_set_style (options, GIMP_FILL_STYLE_PATTERN);
        gimp_context_set_pattern (GIMP_CONTEXT (options), pattern);
        private->undo_desc = C_("undo-type", "Fill with Pattern");

        return TRUE;
      }
      break;

    default:
      g_warning ("%s: invalid fill_type %d", G_STRFUNC, fill_type);
      return FALSE;
    }

  g_return_val_if_fail (color != NULL, FALSE);

  gimp_fill_options_set_style (options, GIMP_FILL_STYLE_FG_COLOR);
  gimp_context_set_foreground (GIMP_CONTEXT (options), color);
  private->undo_desc = undo_desc;

  g_object_unref (color);

  return TRUE;
}

gboolean
gimp_fill_options_set_by_fill_mode (GimpFillOptions     *options,
                                    GimpContext         *context,
                                    GimpBucketFillMode   fill_mode,
                                    GError             **error)
{
  GimpFillType fill_type;

  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  switch (fill_mode)
    {
    default:
    case GIMP_BUCKET_FILL_FG:
      fill_type = GIMP_FILL_FOREGROUND;
      break;

    case GIMP_BUCKET_FILL_BG:
      fill_type = GIMP_FILL_BACKGROUND;
      break;

    case GIMP_BUCKET_FILL_PATTERN:
      fill_type = GIMP_FILL_PATTERN;
      break;
    }

  return gimp_fill_options_set_by_fill_type (options, context,
                                             fill_type, error);
}

const gchar *
gimp_fill_options_get_undo_desc (GimpFillOptions *options)
{
  GimpFillOptionsPrivate *private;

  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);

  private = GET_PRIVATE (options);

  if (private->undo_desc)
    return private->undo_desc;

  switch (private->style)
    {
    case GIMP_FILL_STYLE_FG_COLOR:
      return C_("undo-type", "Fill with Foreground Color");

    case GIMP_FILL_STYLE_BG_COLOR:
      return C_("undo-type", "Fill with Background Color");

    case GIMP_FILL_STYLE_PATTERN:
      return C_("undo-type", "Fill with Pattern");
    }

  g_return_val_if_reached (NULL);
}

const Babl *
gimp_fill_options_get_format (GimpFillOptions *options,
                              GimpDrawable    *drawable)
{
  GimpContext *context;

  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  context = GIMP_CONTEXT (options);

  return gimp_layer_mode_get_format (gimp_context_get_paint_mode (context),
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     GIMP_LAYER_COLOR_SPACE_AUTO,
                                     gimp_layer_mode_get_paint_composite_mode (
                                       gimp_context_get_paint_mode (context)),
                                     gimp_drawable_get_format (drawable));
}

GeglBuffer *
gimp_fill_options_create_buffer (GimpFillOptions     *options,
                                 GimpDrawable        *drawable,
                                 const GeglRectangle *rect,
                                 gint                 pattern_offset_x,
                                 gint                 pattern_offset_y)
{
  GeglBuffer *buffer;

  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (gimp_fill_options_get_style (options) !=
                        GIMP_FILL_STYLE_PATTERN ||
                        gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL,
                        NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = gegl_buffer_new (rect,
                            gimp_fill_options_get_format (options, drawable));

  gimp_fill_options_fill_buffer (options, drawable, buffer,
                                 pattern_offset_x, pattern_offset_y);

  return buffer;
}

void
gimp_fill_options_fill_buffer (GimpFillOptions *options,
                               GimpDrawable    *drawable,
                               GeglBuffer      *buffer,
                               gint             pattern_offset_x,
                               gint             pattern_offset_y)
{
  g_return_if_fail (GIMP_IS_FILL_OPTIONS (options));
  g_return_if_fail (gimp_fill_options_get_style (options) !=
                    GIMP_FILL_STYLE_PATTERN ||
                    gimp_context_get_pattern (GIMP_CONTEXT (options)) != NULL);
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  switch (gimp_fill_options_get_style (options))
    {
    case GIMP_FILL_STYLE_FG_COLOR:
      {
        GeglColor *color;

        color = gimp_context_get_foreground (GIMP_CONTEXT (options));
        gimp_palettes_add_color_history (GIMP_CONTEXT (options)->gimp, color);

        gimp_drawable_fill_buffer (drawable, buffer, color, NULL, 0, 0);
      }
      break;

    case GIMP_FILL_STYLE_BG_COLOR:
      {
        GeglColor *color;

        color = gimp_context_get_background (GIMP_CONTEXT (options));
        gimp_palettes_add_color_history (GIMP_CONTEXT (options)->gimp, color);

        gimp_drawable_fill_buffer (drawable, buffer, color, NULL, 0, 0);
      }
      break;

    case GIMP_FILL_STYLE_PATTERN:
      {
        GimpPattern *pattern;

        pattern = gimp_context_get_pattern (GIMP_CONTEXT (options));

        gimp_drawable_fill_buffer (drawable, buffer,
                                   NULL, pattern,
                                   pattern_offset_x,
                                   pattern_offset_y);
      }
      break;
    }
}
