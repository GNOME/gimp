/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "paint-types.h"

#include "core/ligma.h"
#include "core/ligmabrush-header.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmaimage.h"
#include "core/ligmadynamics.h"
#include "core/ligmadynamicsoutput.h"
#include "core/ligmagradient.h"
#include "core/ligmapaintinfo.h"

#include "ligmabrushcore.h"
#include "ligmapaintoptions.h"

#include "ligma-intl.h"


#define DEFAULT_BRUSH_SIZE              20.0
#define DEFAULT_BRUSH_ASPECT_RATIO      0.0
#define DEFAULT_BRUSH_ANGLE             0.0
#define DEFAULT_BRUSH_SPACING           0.1
#define DEFAULT_BRUSH_HARDNESS          1.0 /* Generated brushes have their own */
#define DEFAULT_BRUSH_FORCE             0.5

#define DEFAULT_BRUSH_LINK_SIZE         TRUE
#define DEFAULT_BRUSH_LINK_ASPECT_RATIO TRUE
#define DEFAULT_BRUSH_LINK_ANGLE        TRUE
#define DEFAULT_BRUSH_LINK_SPACING      TRUE
#define DEFAULT_BRUSH_LINK_HARDNESS     TRUE

#define DEFAULT_BRUSH_LOCK_TO_VIEW      FALSE

#define DEFAULT_APPLICATION_MODE        LIGMA_PAINT_CONSTANT
#define DEFAULT_HARD                    FALSE

#define DEFAULT_USE_JITTER              FALSE
#define DEFAULT_JITTER_AMOUNT           0.2

#define DEFAULT_DYNAMICS_ENABLED        TRUE

#define DEFAULT_FADE_LENGTH             100.0
#define DEFAULT_FADE_REVERSE            FALSE
#define DEFAULT_FADE_REPEAT             LIGMA_REPEAT_NONE
#define DEFAULT_FADE_UNIT               LIGMA_UNIT_PIXEL

#define DEFAULT_GRADIENT_REVERSE        FALSE
#define DEFAULT_GRADIENT_BLEND_SPACE    LIGMA_GRADIENT_BLEND_RGB_PERCEPTUAL
#define DEFAULT_GRADIENT_REPEAT         LIGMA_REPEAT_NONE

#define DYNAMIC_MAX_VALUE               1.0
#define DYNAMIC_MIN_VALUE               0.0

#define DEFAULT_SMOOTHING_QUALITY       20
#define DEFAULT_SMOOTHING_FACTOR        50

enum
{
  PROP_0,

  PROP_PAINT_INFO,

  PROP_USE_APPLICATOR, /* temp debug */

  PROP_BRUSH_SIZE,
  PROP_BRUSH_ASPECT_RATIO,
  PROP_BRUSH_ANGLE,
  PROP_BRUSH_SPACING,
  PROP_BRUSH_HARDNESS,
  PROP_BRUSH_FORCE,

  PROP_BRUSH_LINK_SIZE,
  PROP_BRUSH_LINK_ASPECT_RATIO,
  PROP_BRUSH_LINK_ANGLE,
  PROP_BRUSH_LINK_SPACING,
  PROP_BRUSH_LINK_HARDNESS,

  PROP_BRUSH_LOCK_TO_VIEW,

  PROP_APPLICATION_MODE,
  PROP_HARD,

  PROP_USE_JITTER,
  PROP_JITTER_AMOUNT,

  PROP_DYNAMICS_ENABLED,

  PROP_FADE_LENGTH,
  PROP_FADE_REVERSE,
  PROP_FADE_REPEAT,
  PROP_FADE_UNIT,

  PROP_GRADIENT_REVERSE,
  PROP_GRADIENT_BLEND_COLOR_SPACE,
  PROP_GRADIENT_REPEAT,

  PROP_BRUSH_VIEW_TYPE,
  PROP_BRUSH_VIEW_SIZE,
  PROP_DYNAMICS_VIEW_TYPE,
  PROP_DYNAMICS_VIEW_SIZE,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE,
  PROP_GRADIENT_VIEW_TYPE,
  PROP_GRADIENT_VIEW_SIZE,

  PROP_USE_SMOOTHING,
  PROP_SMOOTHING_QUALITY,
  PROP_SMOOTHING_FACTOR
};


static void         ligma_paint_options_config_iface_init (LigmaConfigInterface *config_iface);

static void         ligma_paint_options_dispose           (GObject          *object);
static void         ligma_paint_options_finalize          (GObject          *object);
static void         ligma_paint_options_set_property      (GObject          *object,
                                                          guint             property_id,
                                                          const GValue     *value,
                                                          GParamSpec       *pspec);
static void         ligma_paint_options_get_property      (GObject          *object,
                                                          guint             property_id,
                                                          GValue           *value,
                                                          GParamSpec       *pspec);

static void         ligma_paint_options_brush_changed     (LigmaContext      *context,
                                                          LigmaBrush        *brush);
static void         ligma_paint_options_brush_notify      (LigmaBrush        *brush,
                                                          const GParamSpec *pspec,
                                                          LigmaPaintOptions *options);

static LigmaConfig * ligma_paint_options_duplicate         (LigmaConfig       *config);
static gboolean     ligma_paint_options_copy              (LigmaConfig       *src,
                                                          LigmaConfig       *dest,
                                                          GParamFlags       flags);
static void         ligma_paint_options_reset             (LigmaConfig       *config);


G_DEFINE_TYPE_WITH_CODE (LigmaPaintOptions, ligma_paint_options,
                         LIGMA_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_paint_options_config_iface_init))

#define parent_class ligma_paint_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_paint_options_class_init (LigmaPaintOptionsClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  LigmaContextClass *context_class = LIGMA_CONTEXT_CLASS (klass);

  object_class->dispose         = ligma_paint_options_dispose;
  object_class->finalize        = ligma_paint_options_finalize;
  object_class->set_property    = ligma_paint_options_set_property;
  object_class->get_property    = ligma_paint_options_get_property;

  context_class->brush_changed  = ligma_paint_options_brush_changed;

  g_object_class_install_property (object_class, PROP_PAINT_INFO,
                                   g_param_spec_object ("paint-info",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_PAINT_INFO,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_USE_APPLICATOR,
                                   g_param_spec_boolean ("use-applicator",
                                                         "Use LigmaApplicator",
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_SIZE,
                           "brush-size",
                           _("Size"),
                           _("Brush Size"),
                           1.0, LIGMA_BRUSH_MAX_SIZE, DEFAULT_BRUSH_SIZE,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_ASPECT_RATIO,
                           "brush-aspect-ratio",
                           _("Aspect Ratio"),
                           _("Brush Aspect Ratio"),
                           -20.0, 20.0, DEFAULT_BRUSH_ASPECT_RATIO,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_ANGLE,
                           "brush-angle",
                           _("Angle"),
                           _("Brush Angle"),
                           -180.0, 180.0, DEFAULT_BRUSH_ANGLE,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_SPACING,
                           "brush-spacing",
                           _("Spacing"),
                           _("Brush Spacing"),
                           0.01, 50.0, DEFAULT_BRUSH_SPACING,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_HARDNESS,
                           "brush-hardness",
                           _("Hardness"),
                           _("Brush Hardness"),
                           0.0, 1.0, DEFAULT_BRUSH_HARDNESS,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BRUSH_FORCE,
                           "brush-force",
                           _("Force"),
                           _("Brush Force"),
                           0.0, 1.0, DEFAULT_BRUSH_FORCE,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LINK_SIZE,
                            "brush-link-size",
                            _("Link Size"),
                            _("Link brush size to brush native"),
                            DEFAULT_BRUSH_LINK_SIZE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LINK_ASPECT_RATIO,
                            "brush-link-aspect-ratio",
                            _("Link Aspect Ratio"),
                            _("Link brush aspect ratio to brush native"),
                            DEFAULT_BRUSH_LINK_ASPECT_RATIO,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LINK_ANGLE,
                            "brush-link-angle",
                            _("Link Angle"),
                            _("Link brush angle to brush native"),
                            DEFAULT_BRUSH_LINK_ANGLE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LINK_SPACING,
                            "brush-link-spacing",
                            _("Link Spacing"),
                            _("Link brush spacing to brush native"),
                            DEFAULT_BRUSH_LINK_SPACING,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LINK_HARDNESS,
                            "brush-link-hardness",
                            _("Link Hardness"),
                            _("Link brush hardness to brush native"),
                            DEFAULT_BRUSH_LINK_HARDNESS,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_BRUSH_LOCK_TO_VIEW,
                            "brush-lock-to-view",
                            _("Lock brush to view"),
                            _("Keep brush appearance fixed relative to the view"),
                            DEFAULT_BRUSH_LOCK_TO_VIEW,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_APPLICATION_MODE,
                         "application-mode",
                         _("Incremental"),
                         _("Every stamp has its own opacity"),
                         LIGMA_TYPE_PAINT_APPLICATION_MODE,
                         DEFAULT_APPLICATION_MODE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_HARD,
                            "hard",
                            _("Hard edge"),
                            _("Ignore fuzziness of the current brush"),
                            DEFAULT_HARD,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_JITTER,
                            "use-jitter",
                            _("Apply Jitter"),
                            _("Scatter brush as you paint"),
                            DEFAULT_USE_JITTER,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DYNAMICS_ENABLED,
                            "dynamics-enabled",
                            _("Enable dynamics"),
                            _("Apply dynamics curves to paint settings"),
                            DEFAULT_DYNAMICS_ENABLED,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_JITTER_AMOUNT,
                           "jitter-amount",
                           _("Amount"),
                           _("Distance of scattering"),
                           0.0, 50.0, DEFAULT_JITTER_AMOUNT,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FADE_LENGTH,
                           "fade-length",
                           _("Fade length"),
                           _("Distance over which strokes fade out"),
                           0.0, 32767.0, DEFAULT_FADE_LENGTH,
                           LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_FADE_UNIT,
                         "fade-unit",
                         NULL, NULL,
                         TRUE, TRUE, DEFAULT_FADE_UNIT,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_FADE_REVERSE,
                            "fade-reverse",
                            _("Reverse"),
                            _("Reverse direction of fading"),
                            DEFAULT_FADE_REVERSE,
                            LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FADE_REPEAT,
                         "fade-repeat",
                         _("Repeat"),
                         _("How fade is repeated as you paint"),
                         LIGMA_TYPE_REPEAT_MODE,
                         DEFAULT_FADE_REPEAT,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_GRADIENT_REVERSE,
                            "gradient-reverse",
                            NULL, NULL,
                            DEFAULT_GRADIENT_REVERSE,
                            LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_GRADIENT_BLEND_COLOR_SPACE,
                         "gradient-blend-color-space",
                         _("Blend Color Space"),
                         _("Which color space to use when blending RGB gradient segments"),
                         LIGMA_TYPE_GRADIENT_BLEND_COLOR_SPACE,
                         DEFAULT_GRADIENT_BLEND_SPACE,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_GRADIENT_REPEAT,
                         "gradient-repeat",
                         _("Repeat"),
                         NULL,
                         LIGMA_TYPE_REPEAT_MODE,
                         DEFAULT_GRADIENT_REPEAT,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BRUSH_VIEW_TYPE,
                         "brush-view-type",
                         NULL, NULL,
                         LIGMA_TYPE_VIEW_TYPE,
                         LIGMA_VIEW_TYPE_GRID,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_BRUSH_VIEW_SIZE,
                        "brush-view-size",
                        NULL, NULL,
                        LIGMA_VIEW_SIZE_TINY,
                        LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                        LIGMA_VIEW_SIZE_SMALL,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_DYNAMICS_VIEW_TYPE,
                         "dynamics-view-type",
                         NULL, NULL,
                         LIGMA_TYPE_VIEW_TYPE,
                         LIGMA_VIEW_TYPE_LIST,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_DYNAMICS_VIEW_SIZE,
                        "dynamics-view-size",
                        NULL, NULL,
                        LIGMA_VIEW_SIZE_TINY,
                        LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                        LIGMA_VIEW_SIZE_SMALL,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_PATTERN_VIEW_TYPE,
                         "pattern-view-type",
                         NULL, NULL,
                         LIGMA_TYPE_VIEW_TYPE,
                         LIGMA_VIEW_TYPE_GRID,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_PATTERN_VIEW_SIZE,
                        "pattern-view-size",
                        NULL, NULL,
                        LIGMA_VIEW_SIZE_TINY,
                        LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                        LIGMA_VIEW_SIZE_SMALL,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_GRADIENT_VIEW_TYPE,
                         "gradient-view-type",
                         NULL, NULL,
                         LIGMA_TYPE_VIEW_TYPE,
                         LIGMA_VIEW_TYPE_LIST,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_GRADIENT_VIEW_SIZE,
                        "gradient-view-size",
                        NULL, NULL,
                        LIGMA_VIEW_SIZE_TINY,
                        LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                        LIGMA_VIEW_SIZE_LARGE,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_SMOOTHING,
                            "use-smoothing",
                            _("Smooth stroke"),
                            _("Paint smoother strokes"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_SMOOTHING_QUALITY,
                        "smoothing-quality",
                        _("Quality"),
                        _("Depth of smoothing"),
                        1, 100, DEFAULT_SMOOTHING_QUALITY,
                        LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_SMOOTHING_FACTOR,
                           "smoothing-factor",
                           _("Weight"),
                           _("Gravity of the pen"),
                           /* Max velocity is set to 3; allowing for
                            * smoothing factor to be less than velcoty
                            * results in numeric instablility
                            */
                           3.0, 1000.0, DEFAULT_SMOOTHING_FACTOR,
                           LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_paint_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (LIGMA_TYPE_CONFIG);

  config_iface->duplicate = ligma_paint_options_duplicate;
  config_iface->copy      = ligma_paint_options_copy;
  config_iface->reset     = ligma_paint_options_reset;
}

static void
ligma_paint_options_init (LigmaPaintOptions *options)
{
  options->application_mode_save = DEFAULT_APPLICATION_MODE;

  options->jitter_options    = g_slice_new0 (LigmaJitterOptions);
  options->fade_options      = g_slice_new0 (LigmaFadeOptions);
  options->gradient_options  = g_slice_new0 (LigmaGradientPaintOptions);
  options->smoothing_options = g_slice_new0 (LigmaSmoothingOptions);
}

static void
ligma_paint_options_dispose (GObject *object)
{
  LigmaPaintOptions *options = LIGMA_PAINT_OPTIONS (object);

  g_clear_object (&options->paint_info);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_paint_options_finalize (GObject *object)
{
  LigmaPaintOptions *options = LIGMA_PAINT_OPTIONS (object);

  g_slice_free (LigmaJitterOptions,        options->jitter_options);
  g_slice_free (LigmaFadeOptions,          options->fade_options);
  g_slice_free (LigmaGradientPaintOptions, options->gradient_options);
  g_slice_free (LigmaSmoothingOptions,     options->smoothing_options);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_paint_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaPaintOptions         *options           = LIGMA_PAINT_OPTIONS (object);
  LigmaFadeOptions          *fade_options      = options->fade_options;
  LigmaJitterOptions        *jitter_options    = options->jitter_options;
  LigmaGradientPaintOptions *gradient_options  = options->gradient_options;
  LigmaSmoothingOptions     *smoothing_options = options->smoothing_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      options->paint_info = g_value_dup_object (value);
      break;

    case PROP_USE_APPLICATOR:
      options->use_applicator = g_value_get_boolean (value);
      break;

    case PROP_BRUSH_SIZE:
      options->brush_size = g_value_get_double (value);
      break;
    case PROP_BRUSH_ASPECT_RATIO:
      options->brush_aspect_ratio = g_value_get_double (value);
      break;
    case PROP_BRUSH_ANGLE:
      options->brush_angle = - 1.0 * g_value_get_double (value) / 360.0; /* let's make the angle mathematically correct */
      break;
    case PROP_BRUSH_SPACING:
      options->brush_spacing = g_value_get_double (value);
      break;
    case PROP_BRUSH_HARDNESS:
      options->brush_hardness = g_value_get_double (value);
      break;
    case PROP_BRUSH_FORCE:
      options->brush_force = g_value_get_double (value);
      break;

    case PROP_BRUSH_LINK_SIZE:
      options->brush_link_size = g_value_get_boolean (value);
      break;
    case PROP_BRUSH_LINK_ASPECT_RATIO:
      options->brush_link_aspect_ratio = g_value_get_boolean (value);
      break;
    case PROP_BRUSH_LINK_ANGLE:
      options->brush_link_angle = g_value_get_boolean (value);
      break;
    case PROP_BRUSH_LINK_SPACING:
      options->brush_link_spacing = g_value_get_boolean (value);
      break;
    case PROP_BRUSH_LINK_HARDNESS:
      options->brush_link_hardness = g_value_get_boolean (value);
      break;

    case PROP_BRUSH_LOCK_TO_VIEW:
      options->brush_lock_to_view = g_value_get_boolean (value);
      break;

    case PROP_APPLICATION_MODE:
      options->application_mode = g_value_get_enum (value);
      break;
    case PROP_HARD:
      options->hard = g_value_get_boolean (value);
      break;

    case PROP_USE_JITTER:
      jitter_options->use_jitter = g_value_get_boolean (value);
      break;
    case PROP_JITTER_AMOUNT:
      jitter_options->jitter_amount = g_value_get_double (value);
      break;

    case PROP_DYNAMICS_ENABLED:
      options->dynamics_enabled = g_value_get_boolean (value);
      break;

    case PROP_FADE_LENGTH:
      fade_options->fade_length = g_value_get_double (value);
      break;
    case PROP_FADE_REVERSE:
      fade_options->fade_reverse = g_value_get_boolean (value);
      break;
    case PROP_FADE_REPEAT:
      fade_options->fade_repeat = g_value_get_enum (value);
      break;
    case PROP_FADE_UNIT:
      fade_options->fade_unit = g_value_get_int (value);
      break;

    case PROP_GRADIENT_REVERSE:
      gradient_options->gradient_reverse = g_value_get_boolean (value);
      break;
    case PROP_GRADIENT_BLEND_COLOR_SPACE:
      gradient_options->gradient_blend_color_space = g_value_get_enum (value);
      break;
    case PROP_GRADIENT_REPEAT:
      gradient_options->gradient_repeat = g_value_get_enum (value);
      break;

    case PROP_BRUSH_VIEW_TYPE:
      options->brush_view_type = g_value_get_enum (value);
      break;
    case PROP_BRUSH_VIEW_SIZE:
      options->brush_view_size = g_value_get_int (value);
      break;

    case PROP_DYNAMICS_VIEW_TYPE:
      options->dynamics_view_type = g_value_get_enum (value);
      break;
    case PROP_DYNAMICS_VIEW_SIZE:
      options->dynamics_view_size = g_value_get_int (value);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      options->pattern_view_type = g_value_get_enum (value);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      options->pattern_view_size = g_value_get_int (value);
      break;

    case PROP_GRADIENT_VIEW_TYPE:
      options->gradient_view_type = g_value_get_enum (value);
      break;
    case PROP_GRADIENT_VIEW_SIZE:
      options->gradient_view_size = g_value_get_int (value);
      break;

    case PROP_USE_SMOOTHING:
      smoothing_options->use_smoothing = g_value_get_boolean (value);
      break;
    case PROP_SMOOTHING_QUALITY:
      smoothing_options->smoothing_quality = g_value_get_int (value);
      break;
    case PROP_SMOOTHING_FACTOR:
      smoothing_options->smoothing_factor = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_paint_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaPaintOptions         *options           = LIGMA_PAINT_OPTIONS (object);
  LigmaFadeOptions          *fade_options      = options->fade_options;
  LigmaJitterOptions        *jitter_options    = options->jitter_options;
  LigmaGradientPaintOptions *gradient_options  = options->gradient_options;
  LigmaSmoothingOptions     *smoothing_options = options->smoothing_options;

  switch (property_id)
    {
    case PROP_PAINT_INFO:
      g_value_set_object (value, options->paint_info);
      break;

    case PROP_USE_APPLICATOR:
      g_value_set_boolean (value, options->use_applicator);
      break;

    case PROP_BRUSH_SIZE:
      g_value_set_double (value, options->brush_size);
      break;
    case PROP_BRUSH_ASPECT_RATIO:
      g_value_set_double (value, options->brush_aspect_ratio);
      break;
    case PROP_BRUSH_ANGLE:
      g_value_set_double (value, - 1.0 * options->brush_angle * 360.0); /* mathematically correct -> intuitively correct */
      break;
    case PROP_BRUSH_SPACING:
      g_value_set_double (value, options->brush_spacing);
      break;
    case PROP_BRUSH_HARDNESS:
      g_value_set_double (value, options->brush_hardness);
      break;
    case PROP_BRUSH_FORCE:
      g_value_set_double (value, options->brush_force);
      break;

    case PROP_BRUSH_LINK_SIZE:
      g_value_set_boolean (value, options->brush_link_size);
      break;
    case PROP_BRUSH_LINK_ASPECT_RATIO:
      g_value_set_boolean (value, options->brush_link_aspect_ratio);
      break;
    case PROP_BRUSH_LINK_ANGLE:
      g_value_set_boolean (value, options->brush_link_angle);
      break;
    case PROP_BRUSH_LINK_SPACING:
      g_value_set_boolean (value, options->brush_link_spacing);
      break;
    case PROP_BRUSH_LINK_HARDNESS:
      g_value_set_boolean (value, options->brush_link_hardness);
      break;

    case PROP_BRUSH_LOCK_TO_VIEW:
      g_value_set_boolean (value, options->brush_lock_to_view);
      break;

    case PROP_APPLICATION_MODE:
      g_value_set_enum (value, options->application_mode);
      break;
    case PROP_HARD:
      g_value_set_boolean (value, options->hard);
      break;

    case PROP_USE_JITTER:
      g_value_set_boolean (value, jitter_options->use_jitter);
      break;
    case PROP_JITTER_AMOUNT:
      g_value_set_double (value, jitter_options->jitter_amount);
      break;

    case PROP_DYNAMICS_ENABLED:
      g_value_set_boolean (value, options->dynamics_enabled);
      break;

    case PROP_FADE_LENGTH:
      g_value_set_double (value, fade_options->fade_length);
      break;
    case PROP_FADE_REVERSE:
      g_value_set_boolean (value, fade_options->fade_reverse);
      break;
    case PROP_FADE_REPEAT:
      g_value_set_enum (value, fade_options->fade_repeat);
      break;
    case PROP_FADE_UNIT:
      g_value_set_int (value, fade_options->fade_unit);
      break;

    case PROP_GRADIENT_REVERSE:
      g_value_set_boolean (value, gradient_options->gradient_reverse);
      break;
    case PROP_GRADIENT_BLEND_COLOR_SPACE:
      g_value_set_enum (value, gradient_options->gradient_blend_color_space);
      break;
    case PROP_GRADIENT_REPEAT:
      g_value_set_enum (value, gradient_options->gradient_repeat);
      break;

    case PROP_BRUSH_VIEW_TYPE:
      g_value_set_enum (value, options->brush_view_type);
      break;
    case PROP_BRUSH_VIEW_SIZE:
      g_value_set_int (value, options->brush_view_size);
      break;

    case PROP_DYNAMICS_VIEW_TYPE:
      g_value_set_enum (value, options->dynamics_view_type);
      break;
    case PROP_DYNAMICS_VIEW_SIZE:
      g_value_set_int (value, options->dynamics_view_size);
      break;

    case PROP_PATTERN_VIEW_TYPE:
      g_value_set_enum (value, options->pattern_view_type);
      break;
    case PROP_PATTERN_VIEW_SIZE:
      g_value_set_int (value, options->pattern_view_size);
      break;

    case PROP_GRADIENT_VIEW_TYPE:
      g_value_set_enum (value, options->gradient_view_type);
      break;
    case PROP_GRADIENT_VIEW_SIZE:
      g_value_set_int (value, options->gradient_view_size);
      break;

    case PROP_USE_SMOOTHING:
      g_value_set_boolean (value, smoothing_options->use_smoothing);
      break;
    case PROP_SMOOTHING_QUALITY:
      g_value_set_int (value, smoothing_options->smoothing_quality);
      break;
    case PROP_SMOOTHING_FACTOR:
      g_value_set_double (value, smoothing_options->smoothing_factor);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_paint_options_brush_changed (LigmaContext *context,
                                  LigmaBrush   *brush)
{
  LigmaPaintOptions *options = LIGMA_PAINT_OPTIONS (context);

  if (options->paint_info &&
      g_type_is_a (options->paint_info->paint_type,
                   LIGMA_TYPE_BRUSH_CORE))
    {
      if (options->brush)
        {
          g_signal_handlers_disconnect_by_func (options->brush,
                                                ligma_paint_options_brush_notify,
                                                options);
          g_object_remove_weak_pointer (G_OBJECT (options->brush),
                                        (gpointer) &options->brush);
        }

      options->brush = brush;

      if (options->brush)
        {
          g_object_add_weak_pointer (G_OBJECT (options->brush),
                                     (gpointer) &options->brush);
          g_signal_connect_object (options->brush, "notify",
                                   G_CALLBACK (ligma_paint_options_brush_notify),
                                   options, 0);

          ligma_paint_options_brush_notify (options->brush, NULL, options);
        }
    }
}

static void
ligma_paint_options_brush_notify (LigmaBrush        *brush,
                                 const GParamSpec *pspec,
                                 LigmaPaintOptions *options)
{
  if (ligma_tool_options_get_gui_mode (LIGMA_TOOL_OPTIONS (options)))
    {
#define IS_PSPEC(p,n) (p == NULL || ! strcmp (n, p->name))

      if (options->brush_link_size && IS_PSPEC (pspec, "radius"))
        ligma_paint_options_set_default_brush_size (options, brush);

      if (options->brush_link_aspect_ratio && IS_PSPEC (pspec, "aspect-ratio"))
        ligma_paint_options_set_default_brush_aspect_ratio (options, brush);

      if (options->brush_link_angle && IS_PSPEC (pspec, "angle"))
        ligma_paint_options_set_default_brush_angle (options, brush);

      if (options->brush_link_spacing && IS_PSPEC (pspec, "spacing"))
        ligma_paint_options_set_default_brush_spacing (options, brush);

      if (options->brush_link_hardness && IS_PSPEC (pspec, "hardness"))
        ligma_paint_options_set_default_brush_hardness (options, brush);

#undef IS_SPEC
    }
}

static LigmaConfig *
ligma_paint_options_duplicate (LigmaConfig *config)
{
  return parent_config_iface->duplicate (config);
}

static gboolean
ligma_paint_options_copy (LigmaConfig  *src,
                         LigmaConfig  *dest,
                         GParamFlags  flags)
{
  return parent_config_iface->copy (src, dest, flags);
}

static void
ligma_paint_options_reset (LigmaConfig *config)
{
  LigmaBrush *brush = ligma_context_get_brush (LIGMA_CONTEXT (config));

  parent_config_iface->reset (config);

  if (brush)
    {
      ligma_paint_options_set_default_brush_size (LIGMA_PAINT_OPTIONS (config),
                                                 brush);
      ligma_paint_options_set_default_brush_hardness (LIGMA_PAINT_OPTIONS (config),
                                                     brush);
      ligma_paint_options_set_default_brush_aspect_ratio (LIGMA_PAINT_OPTIONS (config),
                                                         brush);
      ligma_paint_options_set_default_brush_angle (LIGMA_PAINT_OPTIONS (config),
                                                  brush);
      ligma_paint_options_set_default_brush_spacing (LIGMA_PAINT_OPTIONS (config),
                                                    brush);
    }
}

LigmaPaintOptions *
ligma_paint_options_new (LigmaPaintInfo *paint_info)
{
  LigmaPaintOptions *options;

  g_return_val_if_fail (LIGMA_IS_PAINT_INFO (paint_info), NULL);

  options = g_object_new (paint_info->paint_options_type,
                          "ligma",       paint_info->ligma,
                          "name",       ligma_object_get_name (paint_info),
                          "paint-info", paint_info,
                          NULL);

  return options;
}

gdouble
ligma_paint_options_get_fade (LigmaPaintOptions *paint_options,
                             LigmaImage        *image,
                             gdouble           pixel_dist)
{
  LigmaFadeOptions *fade_options;
  gdouble          z        = -1.0;
  gdouble          fade_out =  0.0;
  gdouble          unit_factor;
  gdouble          pos;

  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options),
                        DYNAMIC_MAX_VALUE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), DYNAMIC_MAX_VALUE);

  fade_options = paint_options->fade_options;

  switch (fade_options->fade_unit)
    {
    case LIGMA_UNIT_PIXEL:
      fade_out = fade_options->fade_length;
      break;

    case LIGMA_UNIT_PERCENT:
      fade_out = (MAX (ligma_image_get_width  (image),
                       ligma_image_get_height (image)) *
                  fade_options->fade_length / 100);
      break;

    default:
      {
        gdouble xres;
        gdouble yres;

        ligma_image_get_resolution (image, &xres, &yres);

        unit_factor = ligma_unit_get_factor (fade_options->fade_unit);
        fade_out    = (fade_options->fade_length *
                       MAX (xres, yres) / unit_factor);
      }
      break;
    }

  /*  factor in the fade out value  */
  if (fade_out > 0.0)
    {
      pos = pixel_dist / fade_out;
    }
  else
    pos = DYNAMIC_MAX_VALUE;

  /*  for no repeat, set pos close to 1.0 after the first chunk  */
  if (fade_options->fade_repeat == LIGMA_REPEAT_NONE && pos >= DYNAMIC_MAX_VALUE)
    pos = DYNAMIC_MAX_VALUE - 0.0000001;

  if (((gint) pos & 1) &&
      fade_options->fade_repeat != LIGMA_REPEAT_SAWTOOTH)
    pos = DYNAMIC_MAX_VALUE - (pos - (gint) pos);
  else
    pos = pos - (gint) pos;

  z = pos;

  if (fade_options->fade_reverse)
    z = 1.0 - z;

  return z;    /*  ln (1/255)  */
}

gdouble
ligma_paint_options_get_jitter (LigmaPaintOptions *paint_options,
                               LigmaImage        *image)
{
  LigmaJitterOptions *jitter_options;

  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options), 0.0);

  jitter_options = paint_options->jitter_options;

  if (jitter_options->use_jitter)
    {
      return jitter_options->jitter_amount;
    }

  return 0.0;
}

gboolean
ligma_paint_options_get_gradient_color (LigmaPaintOptions *paint_options,
                                       LigmaImage        *image,
                                       gdouble           grad_point,
                                       gdouble           pixel_dist,
                                       LigmaRGB          *color)
{
  LigmaDynamics *dynamics;

  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  dynamics = ligma_context_get_dynamics (LIGMA_CONTEXT (paint_options));

  if (ligma_dynamics_is_output_enabled (dynamics, LIGMA_DYNAMICS_OUTPUT_COLOR))
    {
      LigmaGradientPaintOptions *gradient_options = paint_options->gradient_options;
      LigmaGradient             *gradient;

      gradient = ligma_context_get_gradient (LIGMA_CONTEXT (paint_options));

      ligma_gradient_get_color_at (gradient, LIGMA_CONTEXT (paint_options),
                                  NULL, grad_point,
                                  gradient_options->gradient_reverse,
                                  gradient_options->gradient_blend_color_space,
                                  color);

      return TRUE;
    }

  return FALSE;
}

LigmaBrushApplicationMode
ligma_paint_options_get_brush_mode (LigmaPaintOptions *paint_options)
{
  LigmaDynamics *dynamics;
  gboolean      dynamic_force = FALSE;

  g_return_val_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options), LIGMA_BRUSH_SOFT);

  if (paint_options->hard)
    return LIGMA_BRUSH_HARD;

  dynamics = ligma_context_get_dynamics (LIGMA_CONTEXT (paint_options));

  dynamic_force = ligma_dynamics_is_output_enabled (dynamics,
                                                   LIGMA_DYNAMICS_OUTPUT_FORCE);

  if (dynamic_force || (paint_options->brush_force != 0.5))
    return LIGMA_BRUSH_PRESSURE;

  return LIGMA_BRUSH_SOFT;
}

void
ligma_paint_options_set_default_brush_size (LigmaPaintOptions *paint_options,
                                           LigmaBrush        *brush)
{
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  if (! brush)
    brush = ligma_context_get_brush (LIGMA_CONTEXT (paint_options));

  if (brush)
    {
      gint height;
      gint width;

      ligma_brush_transform_size (brush, 1.0, 0.0, 0.0, FALSE, &width, &height);

      g_object_set (paint_options,
                    "brush-size", (gdouble) MAX (height, width),
                    NULL);
    }
}

void
ligma_paint_options_set_default_brush_angle (LigmaPaintOptions *paint_options,
                                            LigmaBrush        *brush)
{
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  if (! brush)
    brush = ligma_context_get_brush (LIGMA_CONTEXT (paint_options));

  if (LIGMA_IS_BRUSH_GENERATED (brush))
    {
      LigmaBrushGenerated *generated_brush = LIGMA_BRUSH_GENERATED (brush);

      g_object_set (paint_options,
                    "brush-angle", (gdouble) ligma_brush_generated_get_angle (generated_brush),
                    NULL);
    }
  else
    {
      g_object_set (paint_options,
                    "brush-angle", DEFAULT_BRUSH_ANGLE,
                    NULL);
    }
}

void
ligma_paint_options_set_default_brush_aspect_ratio (LigmaPaintOptions *paint_options,
                                                   LigmaBrush        *brush)
{
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  if (! brush)
    brush = ligma_context_get_brush (LIGMA_CONTEXT (paint_options));

  if (LIGMA_IS_BRUSH_GENERATED (brush))
    {
      LigmaBrushGenerated *generated_brush = LIGMA_BRUSH_GENERATED (brush);
      gdouble             ratio;

      ratio = ligma_brush_generated_get_aspect_ratio (generated_brush);

      ratio = (ratio - 1.0) * 20.0 / 19.0;

      g_object_set (paint_options,
                    "brush-aspect-ratio", ratio,
                    NULL);
    }
  else
    {
      g_object_set (paint_options,
                    "brush-aspect-ratio", DEFAULT_BRUSH_ASPECT_RATIO,
                    NULL);
    }
}

void
ligma_paint_options_set_default_brush_spacing (LigmaPaintOptions *paint_options,
                                              LigmaBrush        *brush)
{
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  if (! brush)
    brush = ligma_context_get_brush (LIGMA_CONTEXT (paint_options));

  if (brush)
    {
      g_object_set (paint_options,
                    "brush-spacing", (gdouble) ligma_brush_get_spacing (brush) / 100.0,
                    NULL);
    }
}

void
ligma_paint_options_set_default_brush_hardness (LigmaPaintOptions *paint_options,
                                               LigmaBrush        *brush)
{
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (paint_options));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  if (! brush)
    brush = ligma_context_get_brush (LIGMA_CONTEXT (paint_options));

  if (LIGMA_IS_BRUSH_GENERATED (brush))
    {
      LigmaBrushGenerated *generated_brush = LIGMA_BRUSH_GENERATED (brush);

      g_object_set (paint_options,
                    "brush-hardness", (gdouble) ligma_brush_generated_get_hardness (generated_brush),
                    NULL);
    }
  else
    {
      g_object_set (paint_options,
                    "brush-hardness", DEFAULT_BRUSH_HARDNESS,
                    NULL);
    }
}

gboolean
ligma_paint_options_are_dynamics_enabled (LigmaPaintOptions *paint_options)
{
  return paint_options->dynamics_enabled;
}

void
ligma_paint_options_enable_dynamics (LigmaPaintOptions *paint_options,
                                    gboolean          enable)
{
  if (paint_options->dynamics_enabled != enable)
    {
      g_object_set (paint_options,
                    "dynamics-enabled", enable,
                    NULL);
    }
}

static const gchar *brush_props[] =
{
  "brush-size",
  "brush-angle",
  "brush-aspect-ratio",
  "brush-spacing",
  "brush-hardness",
  "brush-force",
  "brush-link-size",
  "brush-link-angle",
  "brush-link-aspect-ratio",
  "brush-link-spacing",
  "brush-link-hardness",
  "brush-lock-to-view"
};

static const gchar *dynamics_props[] =
{
  "dynamics-enabled",
  "fade-reverse",
  "fade-length",
  "fade-unit",
  "fade-repeat"
};

static const gchar *gradient_props[] =
{
  "gradient-reverse",
  "gradient-blend-color-space",
  "gradient-repeat"
};

static const gint max_n_props = (G_N_ELEMENTS (brush_props) +
                                 G_N_ELEMENTS (dynamics_props) +
                                 G_N_ELEMENTS (gradient_props));

gboolean
ligma_paint_options_is_prop (const gchar         *prop_name,
                            LigmaContextPropMask  prop_mask)
{
  gint i;

  g_return_val_if_fail (prop_name != NULL, FALSE);

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_BRUSH)
    {
      for (i = 0; i < G_N_ELEMENTS (brush_props); i++)
        if (! strcmp (prop_name, brush_props[i]))
          return TRUE;
    }

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_DYNAMICS)
    {
      for (i = 0; i < G_N_ELEMENTS (dynamics_props); i++)
        if (! strcmp (prop_name, dynamics_props[i]))
          return TRUE;
    }

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_GRADIENT)
    {
      for (i = 0; i < G_N_ELEMENTS (gradient_props); i++)
        if (! strcmp (prop_name, gradient_props[i]))
          return TRUE;
    }

  return FALSE;
}

void
ligma_paint_options_copy_props (LigmaPaintOptions    *src,
                               LigmaPaintOptions    *dest,
                               LigmaContextPropMask  prop_mask)
{
  const gchar *names[max_n_props];
  GValue       values[max_n_props];
  gint         n_props = 0;
  gint         i;

  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (src));
  g_return_if_fail (LIGMA_IS_PAINT_OPTIONS (dest));

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_BRUSH)
    {
      for (i = 0; i < G_N_ELEMENTS (brush_props); i++)
        names[n_props++] = brush_props[i];
    }

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_DYNAMICS)
    {
      for (i = 0; i < G_N_ELEMENTS (dynamics_props); i++)
        names[n_props++] = dynamics_props[i];
    }

  if (prop_mask & LIGMA_CONTEXT_PROP_MASK_GRADIENT)
    {
      for (i = 0; i < G_N_ELEMENTS (gradient_props); i++)
        names[n_props++] = gradient_props[i];
    }

  if (n_props > 0)
    {
      g_object_getv (G_OBJECT (src), n_props, names, values);
      g_object_setv (G_OBJECT (dest), n_props, names, values);

      while (n_props--)
        g_value_unset (&values[n_props]);
    }
}
