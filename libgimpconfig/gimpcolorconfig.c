/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaColorConfig class
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmaconfigtypes.h"

#include "ligmacolorconfig.h"
#include "ligmaconfig-error.h"
#include "ligmaconfig-iface.h"
#include "ligmaconfig-params.h"
#include "ligmaconfig-path.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorconfig
 * @title: LigmaColorConfig
 * @short_description: Color management settings.
 *
 * Color management settings.
 **/


#define COLOR_MANAGEMENT_MODE_BLURB \
  _("How images are displayed on screen.")

#define DISPLAY_PROFILE_BLURB \
  _("The color profile of your (primary) monitor.")

#define DISPLAY_PROFILE_FROM_GDK_BLURB \
  _("When enabled, LIGMA will try to use the display color profile from " \
    "the windowing system.  The configured monitor profile is then only " \
    "used as a fallback.")

#define RGB_PROFILE_BLURB \
  _("The preferred RGB working space color profile. It will be offered " \
    "next to the built-in RGB profile when a color profile can be chosen.")

#define GRAY_PROFILE_BLURB \
  _("The preferred grayscale working space color profile. It will be offered " \
    "next to the built-in grayscale profile when a color profile can be chosen.")

#define CMYK_PROFILE_BLURB \
  _("The CMYK color profile used to convert between RGB and CMYK.")

#define SIMULATION_PROFILE_BLURB \
  _("The color profile to use for soft-proofing from your image's " \
    "color space to some other color space, including " \
    "soft-proofing to a printer or other output device profile.")

#define DISPLAY_RENDERING_INTENT_BLURB \
  _("How colors are converted from your image's color space to your " \
    "display device. Relative colorimetric is usually the best choice. " \
    "Unless you use a LUT monitor profile (most monitor profiles are " \
    "matrix), choosing perceptual intent really gives you relative " \
    "colorimetric." )

#define DISPLAY_USE_BPC_BLURB \
  _("Do use black point compensation (unless you know you have a reason " \
    "not to).")

#define DISPLAY_OPTIMIZE_BLURB \
  _("When disabled, image display might be of better quality " \
    "at the cost of speed.")

#define SIMULATION_RENDERING_INTENT_BLURB \
  _("How colors are converted from your image's color space to the "  \
    "output simulation device (usually your monitor). " \
    "Try them all and choose what looks the best.")

#define SIMULATION_USE_BPC_BLURB \
  _("Try with and without black point compensation "\
    "and choose what looks best.")

#define SIMULATION_OPTIMIZE_BLURB \
  _("When disabled, soft-proofing might be of better quality " \
    "at the cost of speed.")

#define SIMULATION_GAMUT_CHECK_BLURB \
  _("When enabled, the soft-proofing will mark colors " \
    "which can not be represented in the target color space.")

#define OUT_OF_GAMUT_COLOR_BLURB \
  _("The color to use for marking colors which are out of gamut.")


enum
{
  PROP_0,
  PROP_MODE,
  PROP_RGB_PROFILE,
  PROP_GRAY_PROFILE,
  PROP_CMYK_PROFILE,
  PROP_DISPLAY_PROFILE,
  PROP_DISPLAY_PROFILE_FROM_GDK,
  PROP_SIMULATION_PROFILE,
  PROP_DISPLAY_RENDERING_INTENT,
  PROP_DISPLAY_USE_BPC,
  PROP_DISPLAY_OPTIMIZE,
  PROP_SIMULATION_RENDERING_INTENT,
  PROP_SIMULATION_USE_BPC,
  PROP_SIMULATION_OPTIMIZE,
  PROP_SIMULATION_GAMUT_CHECK,
  PROP_OUT_OF_GAMUT_COLOR
};


struct _LigmaColorConfigPrivate
{
  LigmaColorManagementMode   mode;

  gchar                    *rgb_profile;
  gchar                    *gray_profile;
  gchar                    *cmyk_profile;
  gchar                    *display_profile;
  gboolean                  display_profile_from_gdk;
  gchar                    *printer_profile;

  LigmaColorRenderingIntent  display_intent;
  gboolean                  display_use_black_point_compensation;
  gboolean                  display_optimize;

  LigmaColorRenderingIntent  simulation_intent;
  gboolean                  simulation_use_black_point_compensation;
  gboolean                  simulation_optimize;
  gboolean                  simulation_gamut_check;
  LigmaRGB                   out_of_gamut_color;
};

#define GET_PRIVATE(obj) (((LigmaColorConfig *) (obj))->priv)


static void  ligma_color_config_finalize               (GObject          *object);
static void  ligma_color_config_set_property           (GObject          *object,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec);
static void  ligma_color_config_get_property           (GObject          *object,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec);

static void  ligma_color_config_set_rgb_profile        (LigmaColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  ligma_color_config_set_gray_profile       (LigmaColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  ligma_color_config_set_cmyk_profile       (LigmaColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  ligma_color_config_set_display_profile    (LigmaColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  ligma_color_config_set_simulation_profile (LigmaColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);


G_DEFINE_TYPE_WITH_CODE (LigmaColorConfig, ligma_color_config, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaColorConfig)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL)
                         ligma_type_set_translation_domain (g_define_type_id,
                                                           GETTEXT_PACKAGE "-libligma"))

#define parent_class ligma_color_config_parent_class


static void
ligma_color_config_class_init (LigmaColorConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  LigmaRGB       color;

  ligma_rgba_set (&color, 1.0, 0.0, 1.0, 1.0); /* magenta */

  object_class->finalize     = ligma_color_config_finalize;
  object_class->set_property = ligma_color_config_set_property;
  object_class->get_property = ligma_color_config_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode of operation"),
                         COLOR_MANAGEMENT_MODE_BLURB,
                         LIGMA_TYPE_COLOR_MANAGEMENT_MODE,
                         LIGMA_COLOR_MANAGEMENT_DISPLAY,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_PATH (object_class, PROP_RGB_PROFILE,
                         "rgb-profile",
                         _("Preferred RGB profile"),
                         RGB_PROFILE_BLURB,
                         LIGMA_CONFIG_PATH_FILE, NULL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_PATH (object_class, PROP_GRAY_PROFILE,
                         "gray-profile",
                         _("Preferred grayscale profile"),
                         GRAY_PROFILE_BLURB,
                         LIGMA_CONFIG_PATH_FILE, NULL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_PATH (object_class, PROP_CMYK_PROFILE,
                         "cmyk-profile",
                         _("CMYK profile"),
                         CMYK_PROFILE_BLURB,
                         LIGMA_CONFIG_PATH_FILE, NULL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_PATH (object_class, PROP_DISPLAY_PROFILE,
                         "display-profile",
                         _("Monitor profile"),
                         DISPLAY_PROFILE_BLURB,
                         LIGMA_CONFIG_PATH_FILE, NULL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_PROFILE_FROM_GDK,
                            "display-profile-from-gdk",
                            _("Use the system monitor profile"),
                            DISPLAY_PROFILE_FROM_GDK_BLURB,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_PATH (object_class, PROP_SIMULATION_PROFILE,
                         "simulation-profile",
                         _("Simulation profile for soft-proofing"),
                         SIMULATION_PROFILE_BLURB,
                         LIGMA_CONFIG_PATH_FILE, NULL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_DISPLAY_RENDERING_INTENT,
                         "display-rendering-intent",
                         _("Display rendering intent"),
                         DISPLAY_RENDERING_INTENT_BLURB,
                         LIGMA_TYPE_COLOR_RENDERING_INTENT,
                         LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_USE_BPC,
                            "display-use-black-point-compensation",
                            _("Use black point compensation for the display"),
                            DISPLAY_USE_BPC_BLURB,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_OPTIMIZE,
                            "display-optimize",
                            _("Optimize display color transformations"),
                            DISPLAY_OPTIMIZE_BLURB,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_SIMULATION_RENDERING_INTENT,
                         "simulation-rendering-intent",
                         _("Soft-proofing rendering intent"),
                         SIMULATION_RENDERING_INTENT_BLURB,
                         LIGMA_TYPE_COLOR_RENDERING_INTENT,
                         LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_USE_BPC,
                            "simulation-use-black-point-compensation",
                            _("Use black point compensation for soft-proofing"),
                            SIMULATION_USE_BPC_BLURB,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_OPTIMIZE,
                            "simulation-optimize",
                            _("Optimize soft-proofing color transformations"),
                            SIMULATION_OPTIMIZE_BLURB,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_GAMUT_CHECK,
                            "simulation-gamut-check",
                            _("Mark out of gamut colors"),
                            SIMULATION_GAMUT_CHECK_BLURB,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, PROP_OUT_OF_GAMUT_COLOR,
                        "out-of-gamut-color",
                        _("Out of gamut warning color"),
                        OUT_OF_GAMUT_COLOR_BLURB,
                        FALSE, &color,
                        LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_color_config_init (LigmaColorConfig *config)
{
  config->priv = ligma_color_config_get_instance_private (config);
}

static void
ligma_color_config_finalize (GObject *object)
{
  LigmaColorConfigPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->rgb_profile,     g_free);
  g_clear_pointer (&priv->gray_profile,    g_free);
  g_clear_pointer (&priv->cmyk_profile,    g_free);
  g_clear_pointer (&priv->display_profile, g_free);
  g_clear_pointer (&priv->printer_profile, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_color_config_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaColorConfig        *config = LIGMA_COLOR_CONFIG (object);
  LigmaColorConfigPrivate *priv   = GET_PRIVATE (object);
  GError                 *error  = NULL;

  switch (property_id)
    {
    case PROP_MODE:
      priv->mode = g_value_get_enum (value);
      break;
    case PROP_RGB_PROFILE:
      ligma_color_config_set_rgb_profile (config,
                                         g_value_get_string (value),
                                         &error);
      break;
    case PROP_GRAY_PROFILE:
      ligma_color_config_set_gray_profile (config,
                                          g_value_get_string (value),
                                          &error);
      break;
    case PROP_CMYK_PROFILE:
      ligma_color_config_set_cmyk_profile (config,
                                          g_value_get_string (value),
                                          &error);
      break;
    case PROP_DISPLAY_PROFILE:
      ligma_color_config_set_display_profile (config,
                                             g_value_get_string (value),
                                             &error);
      break;
    case PROP_DISPLAY_PROFILE_FROM_GDK:
      priv->display_profile_from_gdk = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_PROFILE:
      ligma_color_config_set_simulation_profile (config,
                                                g_value_get_string (value),
                                                &error);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      priv->display_intent = g_value_get_enum (value);
      break;
    case PROP_DISPLAY_USE_BPC:
      priv->display_use_black_point_compensation = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_OPTIMIZE:
      priv->display_optimize = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      priv->simulation_intent = g_value_get_enum (value);
      break;
    case PROP_SIMULATION_USE_BPC:
      priv->simulation_use_black_point_compensation = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_OPTIMIZE:
      priv->simulation_optimize = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      priv->simulation_gamut_check = g_value_get_boolean (value);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      priv->out_of_gamut_color = *(LigmaRGB *) g_value_get_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }
}

static void
ligma_color_config_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaColorConfigPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_RGB_PROFILE:
      g_value_set_string (value, priv->rgb_profile);
      break;
    case PROP_GRAY_PROFILE:
      g_value_set_string (value, priv->gray_profile);
      break;
    case PROP_CMYK_PROFILE:
      g_value_set_string (value, priv->cmyk_profile);
      break;
    case PROP_DISPLAY_PROFILE:
      g_value_set_string (value, priv->display_profile);
      break;
    case PROP_DISPLAY_PROFILE_FROM_GDK:
      g_value_set_boolean (value, priv->display_profile_from_gdk);
      break;
    case PROP_SIMULATION_PROFILE:
      g_value_set_string (value, priv->printer_profile);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      g_value_set_enum (value, priv->display_intent);
      break;
    case PROP_DISPLAY_USE_BPC:
      g_value_set_boolean (value, priv->display_use_black_point_compensation);
      break;
    case PROP_DISPLAY_OPTIMIZE:
      g_value_set_boolean (value, priv->display_optimize);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      g_value_set_enum (value, priv->simulation_intent);
      break;
    case PROP_SIMULATION_USE_BPC:
      g_value_set_boolean (value, priv->simulation_use_black_point_compensation);
      break;
    case PROP_SIMULATION_OPTIMIZE:
      g_value_set_boolean (value, priv->simulation_optimize);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      g_value_set_boolean (value, priv->simulation_gamut_check);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      g_value_set_boxed (value, &priv->out_of_gamut_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * ligma_color_config_get_mode:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
LigmaColorManagementMode
ligma_color_config_get_mode (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config),
                        LIGMA_COLOR_MANAGEMENT_OFF);

  return GET_PRIVATE (config)->mode;
}

/**
 * ligma_color_config_get_display_intent:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
LigmaColorRenderingIntent
ligma_color_config_get_display_intent (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config),
                        LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL);

  return GET_PRIVATE (config)->display_intent;
}

/**
 * ligma_color_config_get_display_bpc:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_display_bpc (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->display_use_black_point_compensation;
}

/**
 * ligma_color_config_get_display_optimize:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_display_optimize (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->display_optimize;
}

/**
 * ligma_color_config_get_display_profile_from_gdk:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_display_profile_from_gdk (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->display_profile_from_gdk;
}

/**
 * ligma_color_config_get_simulation_intent:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
LigmaColorRenderingIntent
ligma_color_config_get_simulation_intent (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config),
                        LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL);

  return GET_PRIVATE (config)->simulation_intent;
}

/**
 * ligma_color_config_get_simulation_bpc:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_simulation_bpc (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->simulation_use_black_point_compensation;
}

/**
 * ligma_color_config_get_simulation_optimize:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_simulation_optimize (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->simulation_optimize;
}

/**
 * ligma_color_config_get_simulation_gamut_check:
 * @config: a #LigmaColorConfig
 *
 * Since: 2.10
 **/
gboolean
ligma_color_config_get_simulation_gamut_check (LigmaColorConfig *config)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->simulation_gamut_check;
}

/**
 * ligma_color_config_get_out_of_gamut_color:
 * @config: a #LigmaColorConfig
 * @color:  return location for a #LigmaRGB
 *
 * Since: 3.0
 **/
void
ligma_color_config_get_out_of_gamut_color (LigmaColorConfig *config,
                                          LigmaRGB         *color)
{
  g_return_if_fail (LIGMA_IS_COLOR_CONFIG (config));
  g_return_if_fail (color != NULL);

  *color = GET_PRIVATE (config)->out_of_gamut_color;
}

/**
 * ligma_color_config_get_rgb_color_profile:
 * @config: a #LigmaColorConfig
 * @error:  return location for a #GError
 *
 * Returns: (transfer full): the default RGB color profile.
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_color_config_get_rgb_color_profile (LigmaColorConfig  *config,
                                         GError          **error)
{
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (GET_PRIVATE (config)->rgb_profile)
    {
      GFile *file = ligma_file_new_for_config_path (GET_PRIVATE (config)->rgb_profile,
                                                   error);

      if (file)
        {
          profile = ligma_color_profile_new_from_file (file, error);

          if (profile && ! ligma_color_profile_is_rgb (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for RGB color space."),
                           ligma_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * ligma_color_config_get_gray_color_profile:
 * @config: a #LigmaColorConfig
 * @error:  return location for a #GError
 *
 * Returns: (transfer full): the default grayscale color profile.
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_color_config_get_gray_color_profile (LigmaColorConfig  *config,
                                          GError          **error)
{
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (GET_PRIVATE (config)->gray_profile)
    {
      GFile *file = ligma_file_new_for_config_path (GET_PRIVATE (config)->gray_profile,
                                                   error);

      if (file)
        {
          profile = ligma_color_profile_new_from_file (file, error);

          if (profile && ! ligma_color_profile_is_gray (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for GRAY color space."),
                           ligma_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * ligma_color_config_get_cmyk_color_profile:
 * @config: a #LigmaColorConfig
 * @error:  return location for a #GError
 *
 * Returns: (transfer full): the default CMYK color profile.
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_color_config_get_cmyk_color_profile (LigmaColorConfig  *config,
                                          GError          **error)
{
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (GET_PRIVATE (config)->cmyk_profile)
    {
      GFile *file = ligma_file_new_for_config_path (GET_PRIVATE (config)->cmyk_profile,
                                                   error);

      if (file)
        {
          profile = ligma_color_profile_new_from_file (file, error);

          if (profile && ! ligma_color_profile_is_cmyk (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for CMYK color space."),
                           ligma_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * ligma_color_config_get_display_color_profile:
 * @config: a #LigmaColorConfig
 * @error:  return location for a #GError
 *
 * Returns: (transfer full): the default display color profile.
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_color_config_get_display_color_profile (LigmaColorConfig  *config,
                                             GError          **error)
{
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (GET_PRIVATE (config)->display_profile)
    {
      GFile *file = ligma_file_new_for_config_path (GET_PRIVATE (config)->display_profile,
                                                   error);

      if (file)
        {
          profile = ligma_color_profile_new_from_file (file, error);

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * ligma_color_config_get_simulation_color_profile:
 * @config: a #LigmaColorConfig
 * @error:  return location for a #GError
 *
 * Returns: (transfer full): the default soft-proofing color
 *                                profile.
 *
 * Since: 2.10
 **/
LigmaColorProfile *
ligma_color_config_get_simulation_color_profile (LigmaColorConfig  *config,
                                                GError          **error)
{
  LigmaColorProfile *profile = NULL;

  g_return_val_if_fail (LIGMA_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (GET_PRIVATE (config)->printer_profile)
    {
      GFile *file = ligma_file_new_for_config_path (GET_PRIVATE (config)->printer_profile,
                                                   error);

      if (file)
        {
          profile = ligma_color_profile_new_from_file (file, error);

          g_object_unref (file);
        }
    }

  return profile;
}


/*  private functions  */

static void
ligma_color_config_set_rgb_profile (LigmaColorConfig  *config,
                                   const gchar      *filename,
                                   GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = ligma_file_new_for_config_path (filename, error);

      if (file)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! ligma_color_profile_is_rgb (profile))
                {
                  g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for RGB "
                                 "color space."),
                               ligma_file_get_utf8_name (file));
                  success = FALSE;
                }

              g_object_unref (profile);
            }
          else
            {
              success = FALSE;
            }

          g_object_unref (file);
        }
      else
        {
          success = FALSE;
        }
    }

  if (success)
    {
      g_free (GET_PRIVATE (config)->rgb_profile);
      GET_PRIVATE (config)->rgb_profile = g_strdup (filename);
    }
}

static void
ligma_color_config_set_gray_profile (LigmaColorConfig  *config,
                                    const gchar      *filename,
                                    GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = ligma_file_new_for_config_path (filename, error);

      if (file)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! ligma_color_profile_is_gray (profile))
                {
                  g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for GRAY "
                                 "color space."),
                               ligma_file_get_utf8_name (file));
                  success = FALSE;
                }

              g_object_unref (profile);
            }
          else
            {
              success = FALSE;
            }

          g_object_unref (file);
        }
      else
        {
          success = FALSE;
        }
    }

  if (success)
    {
      g_free (GET_PRIVATE (config)->gray_profile);
      GET_PRIVATE (config)->gray_profile = g_strdup (filename);
    }
}

static void
ligma_color_config_set_cmyk_profile (LigmaColorConfig  *config,
                                    const gchar      *filename,
                                    GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = ligma_file_new_for_config_path (filename, error);

      if (file)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! ligma_color_profile_is_cmyk (profile))
                {
                  g_set_error (error, LIGMA_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for CMYK "
                                 "color space."),
                               ligma_file_get_utf8_name (file));
                  success = FALSE;
                }

              g_object_unref (profile);
            }
          else
            {
              success = FALSE;
            }

          g_object_unref (file);
        }
      else
        {
          success = FALSE;
        }
    }

  if (success)
    {
      g_free (GET_PRIVATE (config)->cmyk_profile);
      GET_PRIVATE (config)->cmyk_profile = g_strdup (filename);
    }
}

static void
ligma_color_config_set_display_profile (LigmaColorConfig  *config,
                                       const gchar      *filename,
                                       GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = ligma_file_new_for_config_path (filename, error);

      if (file)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_file (file, error);

          if (profile)
            {
              g_object_unref (profile);
            }
          else
            {
              success = FALSE;
            }

          g_object_unref (file);
        }
      else
        {
          success = FALSE;
        }
    }

  if (success)
    {
      g_free (GET_PRIVATE (config)->display_profile);
      GET_PRIVATE (config)->display_profile = g_strdup (filename);
    }
}

static void
ligma_color_config_set_simulation_profile (LigmaColorConfig  *config,
                                          const gchar      *filename,
                                          GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = ligma_file_new_for_config_path (filename, error);

      if (file)
        {
          LigmaColorProfile *profile;

          profile = ligma_color_profile_new_from_file (file, error);

          if (profile)
            {
              g_object_unref (profile);
            }
          else
            {
              success = FALSE;
            }

          g_object_unref (file);
        }
      else
        {
          success = FALSE;
        }
    }

  if (success)
    {
      g_free (GET_PRIVATE (config)->printer_profile);
      GET_PRIVATE (config)->printer_profile = g_strdup (filename);
    }
}
