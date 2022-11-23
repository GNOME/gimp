/* The LIGMA -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmafilloptions.c
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "ligma.h"
#include "ligma-palettes.h"
#include "ligmadrawable.h"
#include "ligmadrawable-fill.h"
#include "ligmaerror.h"
#include "ligmafilloptions.h"
#include "ligmapattern.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_ANTIALIAS,
  PROP_FEATHER,
  PROP_FEATHER_RADIUS,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE
};


typedef struct _LigmaFillOptionsPrivate LigmaFillOptionsPrivate;

struct _LigmaFillOptionsPrivate
{
  LigmaFillStyle style;
  gboolean      antialias;
  gboolean      feather;
  gdouble       feather_radius;

  LigmaViewType  pattern_view_type;
  LigmaViewSize  pattern_view_size;

  const gchar  *undo_desc;
};

#define GET_PRIVATE(options) \
        ((LigmaFillOptionsPrivate *) ligma_fill_options_get_instance_private ((LigmaFillOptions *) (options)))


static void     ligma_fill_options_config_init  (LigmaConfigInterface *iface);

static void     ligma_fill_options_set_property (GObject             *object,
                                                guint                property_id,
                                                const GValue        *value,
                                                GParamSpec          *pspec);
static void     ligma_fill_options_get_property (GObject             *object,
                                                guint                property_id,
                                                GValue              *value,
                                                GParamSpec          *pspec);

static gboolean ligma_fill_options_serialize    (LigmaConfig          *config,
                                                LigmaConfigWriter    *writer,
                                                gpointer             data);


G_DEFINE_TYPE_WITH_CODE (LigmaFillOptions, ligma_fill_options, LIGMA_TYPE_CONTEXT,
                         G_ADD_PRIVATE (LigmaFillOptions)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_fill_options_config_init))


static void
ligma_fill_options_class_init (LigmaFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_fill_options_set_property;
  object_class->get_property = ligma_fill_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_STYLE,
                         "style",
                         _("Style"),
                         NULL,
                         LIGMA_TYPE_FILL_STYLE,
                         LIGMA_FILL_STYLE_SOLID,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FEATHER,
                            "feather",
                            _("Feather edges"),
                            _("Enable feathering of fill edges"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FEATHER_RADIUS,
                           "feather-radius",
                           _("Radius"),
                           _("Radius of feathering"),
                           0.0, 100.0, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_TYPE,
                                   g_param_spec_enum ("pattern-view-type",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_VIEW_TYPE,
                                                      LIGMA_VIEW_TYPE_GRID,
                                                      G_PARAM_CONSTRUCT |
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_PATTERN_VIEW_SIZE,
                                   g_param_spec_int ("pattern-view-size",
                                                     NULL, NULL,
                                                     LIGMA_VIEW_SIZE_TINY,
                                                     LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                                                     LIGMA_VIEW_SIZE_SMALL,
                                                     G_PARAM_CONSTRUCT |
                                                     LIGMA_PARAM_READWRITE));
}

static void
ligma_fill_options_config_init (LigmaConfigInterface *iface)
{
  iface->serialize = ligma_fill_options_serialize;
}

static void
ligma_fill_options_init (LigmaFillOptions *options)
{
}

static void
ligma_fill_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      private->style = g_value_get_enum (value);
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
ligma_fill_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaFillOptionsPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_STYLE:
      g_value_set_enum (value, private->style);
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
ligma_fill_options_serialize (LigmaConfig       *config,
                             LigmaConfigWriter *writer,
                             gpointer          data)
{
  return ligma_config_serialize_properties (config, writer);
}


/*  public functions  */

LigmaFillOptions *
ligma_fill_options_new (Ligma        *ligma,
                       LigmaContext *context,
                       gboolean     use_context_color)
{
  LigmaFillOptions *options;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (use_context_color == FALSE || context != NULL, NULL);

  options = g_object_new (LIGMA_TYPE_FILL_OPTIONS,
                          "ligma", ligma,
                          NULL);

  if (use_context_color)
    {
      ligma_context_define_properties (LIGMA_CONTEXT (options),
                                      LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                                      LIGMA_CONTEXT_PROP_MASK_PATTERN,
                                      FALSE);

      ligma_context_set_parent (LIGMA_CONTEXT (options), context);
    }

  return options;
}

LigmaFillStyle
ligma_fill_options_get_style (LigmaFillOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), LIGMA_FILL_STYLE_SOLID);

  return GET_PRIVATE (options)->style;
}

void
ligma_fill_options_set_style (LigmaFillOptions *options,
                             LigmaFillStyle    style)
{
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));

  g_object_set (options, "style", style, NULL);
}

gboolean
ligma_fill_options_get_antialias (LigmaFillOptions *options)
{
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->antialias;
}

void
ligma_fill_options_set_antialias (LigmaFillOptions *options,
                                 gboolean         antialias)
{
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));

  g_object_set (options, "antialias", antialias, NULL);
}

gboolean
ligma_fill_options_get_feather (LigmaFillOptions *options,
                               gdouble         *radius)
{
  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), FALSE);

  if (radius)
    *radius = GET_PRIVATE (options)->feather_radius;

  return GET_PRIVATE (options)->feather;
}

void
ligma_fill_options_set_feather (LigmaFillOptions *options,
                               gboolean         feather,
                               gdouble          radius)
{
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));

  g_object_set (options, "feather", feather, NULL);
  g_object_set (options, "feather-radius", radius, NULL);
}

gboolean
ligma_fill_options_set_by_fill_type (LigmaFillOptions  *options,
                                    LigmaContext      *context,
                                    LigmaFillType      fill_type,
                                    GError          **error)
{
  LigmaFillOptionsPrivate *private;
  LigmaRGB                 color;
  const gchar            *undo_desc;

  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  private = GET_PRIVATE (options);

  private->undo_desc = NULL;

  switch (fill_type)
    {
    case LIGMA_FILL_FOREGROUND:
      ligma_context_get_foreground (context, &color);
      undo_desc = C_("undo-type", "Fill with Foreground Color");
      break;

    case LIGMA_FILL_BACKGROUND:
      ligma_context_get_background (context, &color);
      undo_desc = C_("undo-type", "Fill with Background Color");
      break;

    case LIGMA_FILL_WHITE:
      ligma_rgba_set (&color, 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);
      undo_desc = C_("undo-type", "Fill with White");
      break;

    case LIGMA_FILL_TRANSPARENT:
      ligma_context_get_background (context, &color);
      ligma_context_set_paint_mode (LIGMA_CONTEXT (options),
                                   LIGMA_LAYER_MODE_ERASE);
      undo_desc = C_("undo-type", "Fill with Transparency");
      break;

    case LIGMA_FILL_PATTERN:
      {
        LigmaPattern *pattern = ligma_context_get_pattern (context);

        if (! pattern)
          {
            g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                                 _("No patterns available for this operation."));
            return FALSE;
          }

        ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_PATTERN);
        ligma_context_set_pattern (LIGMA_CONTEXT (options), pattern);
        private->undo_desc = C_("undo-type", "Fill with Pattern");

        return TRUE;
      }
      break;

    default:
      g_warning ("%s: invalid fill_type %d", G_STRFUNC, fill_type);
      return FALSE;
    }

  ligma_fill_options_set_style (options, LIGMA_FILL_STYLE_SOLID);
  ligma_context_set_foreground (LIGMA_CONTEXT (options), &color);
  private->undo_desc = undo_desc;

  return TRUE;
}

gboolean
ligma_fill_options_set_by_fill_mode (LigmaFillOptions     *options,
                                    LigmaContext         *context,
                                    LigmaBucketFillMode   fill_mode,
                                    GError             **error)
{
  LigmaFillType fill_type;

  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), FALSE);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  switch (fill_mode)
    {
    default:
    case LIGMA_BUCKET_FILL_FG:
      fill_type = LIGMA_FILL_FOREGROUND;
      break;

    case LIGMA_BUCKET_FILL_BG:
      fill_type = LIGMA_FILL_BACKGROUND;
      break;

    case LIGMA_BUCKET_FILL_PATTERN:
      fill_type = LIGMA_FILL_PATTERN;
      break;
    }

  return ligma_fill_options_set_by_fill_type (options, context,
                                             fill_type, error);
}

const gchar *
ligma_fill_options_get_undo_desc (LigmaFillOptions *options)
{
  LigmaFillOptionsPrivate *private;

  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);

  private = GET_PRIVATE (options);

  if (private->undo_desc)
    return private->undo_desc;

  switch (private->style)
    {
    case LIGMA_FILL_STYLE_SOLID:
      return C_("undo-type", "Fill with Solid Color");

    case LIGMA_FILL_STYLE_PATTERN:
      return C_("undo-type", "Fill with Pattern");
    }

  g_return_val_if_reached (NULL);
}

const Babl *
ligma_fill_options_get_format (LigmaFillOptions *options,
                              LigmaDrawable    *drawable)
{
  LigmaContext *context;

  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  context = LIGMA_CONTEXT (options);

  return ligma_layer_mode_get_format (ligma_context_get_paint_mode (context),
                                     LIGMA_LAYER_COLOR_SPACE_AUTO,
                                     LIGMA_LAYER_COLOR_SPACE_AUTO,
                                     ligma_layer_mode_get_paint_composite_mode (
                                       ligma_context_get_paint_mode (context)),
                                     ligma_drawable_get_format (drawable));
}

GeglBuffer *
ligma_fill_options_create_buffer (LigmaFillOptions     *options,
                                 LigmaDrawable        *drawable,
                                 const GeglRectangle *rect,
                                 gint                 pattern_offset_x,
                                 gint                 pattern_offset_y)
{
  GeglBuffer *buffer;

  g_return_val_if_fail (LIGMA_IS_FILL_OPTIONS (options), NULL);
  g_return_val_if_fail (ligma_fill_options_get_style (options) !=
                        LIGMA_FILL_STYLE_PATTERN ||
                        ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL,
                        NULL);
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (rect != NULL, NULL);

  buffer = gegl_buffer_new (rect,
                            ligma_fill_options_get_format (options, drawable));

  ligma_fill_options_fill_buffer (options, drawable, buffer,
                                 pattern_offset_x, pattern_offset_y);

  return buffer;
}

void
ligma_fill_options_fill_buffer (LigmaFillOptions *options,
                               LigmaDrawable    *drawable,
                               GeglBuffer      *buffer,
                               gint             pattern_offset_x,
                               gint             pattern_offset_y)
{
  g_return_if_fail (LIGMA_IS_FILL_OPTIONS (options));
  g_return_if_fail (ligma_fill_options_get_style (options) !=
                    LIGMA_FILL_STYLE_PATTERN ||
                    ligma_context_get_pattern (LIGMA_CONTEXT (options)) != NULL);
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  switch (ligma_fill_options_get_style (options))
    {
    case LIGMA_FILL_STYLE_SOLID:
      {
        LigmaRGB color;

        ligma_context_get_foreground (LIGMA_CONTEXT (options), &color);
        ligma_palettes_add_color_history (LIGMA_CONTEXT (options)->ligma, &color);

        ligma_drawable_fill_buffer (drawable, buffer,
                                   &color, NULL, 0, 0);
      }
      break;

    case LIGMA_FILL_STYLE_PATTERN:
      {
        LigmaPattern *pattern;

        pattern = ligma_context_get_pattern (LIGMA_CONTEXT (options));

        ligma_drawable_fill_buffer (drawable, buffer,
                                   NULL, pattern,
                                   pattern_offset_x,
                                   pattern_offset_y);
      }
      break;
    }
}
