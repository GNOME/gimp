/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorConfig class
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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "gimpconfigtypes.h"

#include "gimpcolorconfig.h"
#include "gimpconfig-error.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-params.h"
#include "gimpconfig-path.h"

#include "libgimp/libgimp-intl.h"


/**
 * SECTION: gimpcolorconfig
 * @title: GimpColorConfig
 * @short_description: Color management settings.
 *
 * Color management settings.
 **/


#define COLOR_MANAGEMENT_MODE_BLURB \
  _("How images are displayed on screen.")

#define DISPLAY_PROFILE_BLURB \
  _("The color profile of your (primary) monitor.")

#define DISPLAY_PROFILE_FROM_GDK_BLURB \
  _("When enabled, GIMP will try to use the display color profile from " \
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
    "soft-proofing to a printer or other output device profile. ")

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
    "Try them all and choose what looks the best. ")

#define SIMULATION_USE_BPC_BLURB \
  _("Try with and without black point compensation "\
    "and choose what looks best. ")

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


typedef struct _GimpColorConfigPrivate GimpColorConfigPrivate;

struct _GimpColorConfigPrivate
{
  gboolean display_optimize;
  gboolean simulation_optimize;
};

#define GET_PRIVATE(obj) \
        G_TYPE_INSTANCE_GET_PRIVATE (obj, \
                                     GIMP_TYPE_COLOR_CONFIG, \
                                     GimpColorConfigPrivate)


static void  gimp_color_config_finalize               (GObject          *object);
static void  gimp_color_config_set_property           (GObject          *object,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec);
static void  gimp_color_config_get_property           (GObject          *object,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec);

static void  gimp_color_config_set_rgb_profile        (GimpColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  gimp_color_config_set_gray_profile       (GimpColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  gimp_color_config_set_cmyk_profile       (GimpColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  gimp_color_config_set_display_profile    (GimpColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);
static void  gimp_color_config_set_simulation_profile (GimpColorConfig  *config,
                                                       const gchar      *filename,
                                                       GError          **error);


G_DEFINE_TYPE_WITH_CODE (GimpColorConfig, gimp_color_config, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL)
                         gimp_type_set_translation_domain (g_define_type_id,
                                                           GETTEXT_PACKAGE "-libgimp"))

#define parent_class gimp_color_config_parent_class


static void
gimp_color_config_class_init (GimpColorConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GimpRGB       color;

  gimp_rgba_set (&color, 1.0, 0.0, 1.0, 1.0); /* magenta */

  object_class->finalize     = gimp_color_config_finalize;
  object_class->set_property = gimp_color_config_set_property;
  object_class->get_property = gimp_color_config_get_property;

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode of operation"),
                         COLOR_MANAGEMENT_MODE_BLURB,
                         GIMP_TYPE_COLOR_MANAGEMENT_MODE,
                         GIMP_COLOR_MANAGEMENT_DISPLAY,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_RGB_PROFILE,
                         "rgb-profile",
                         _("Preferred RGB profile"),
                         RGB_PROFILE_BLURB,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_GRAY_PROFILE,
                         "gray-profile",
                         _("Preferred grayscale profile"),
                         GRAY_PROFILE_BLURB,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_CMYK_PROFILE,
                         "cmyk-profile",
                         _("CMYK profile"),
                         CMYK_PROFILE_BLURB,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_DISPLAY_PROFILE,
                         "display-profile",
                         _("Monitor profile"),
                         DISPLAY_PROFILE_BLURB,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_PROFILE_FROM_GDK,
                            "display-profile-from-gdk",
                            _("Use the system monitor profile"),
                            DISPLAY_PROFILE_FROM_GDK_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_SIMULATION_PROFILE,
                         /* FIXME: 3.0: change to simulation-profile */
                         "printer-profile",
                         _("Simulation profile for soft-proofing"),
                         SIMULATION_PROFILE_BLURB,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_DISPLAY_RENDERING_INTENT,
                         "display-rendering-intent",
                         _("Display rendering intent"),
                         DISPLAY_RENDERING_INTENT_BLURB,
                         GIMP_TYPE_COLOR_RENDERING_INTENT,
                         GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_USE_BPC,
                            "display-use-black-point-compensation",
                            _("Use black point compensation for the display"),
                            DISPLAY_USE_BPC_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_DISPLAY_OPTIMIZE,
                            "display-optimize",
                            _("Optimize display color transformations"),
                            DISPLAY_OPTIMIZE_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_SIMULATION_RENDERING_INTENT,
                         "simulation-rendering-intent",
                         _("Soft-proofing rendering intent"),
                         SIMULATION_RENDERING_INTENT_BLURB,
                         GIMP_TYPE_COLOR_RENDERING_INTENT,
                         GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                         GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_USE_BPC,
                            "simulation-use-black-point-compensation",
                            _("Use black point compensation for soft-proofing"),
                            SIMULATION_USE_BPC_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_OPTIMIZE,
                            "simulation-optimize",
                            _("Optimize soft-proofing color transformations"),
                            SIMULATION_OPTIMIZE_BLURB,
                            TRUE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_SIMULATION_GAMUT_CHECK,
                            "simulation-gamut-check",
                            _("Mark out of gamut colors"),
                            SIMULATION_GAMUT_CHECK_BLURB,
                            FALSE,
                            GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_PROP_RGB (object_class, PROP_OUT_OF_GAMUT_COLOR,
                        "out-of-gamut-color",
                        _("Out of gamut warning color"),
                        OUT_OF_GAMUT_COLOR_BLURB,
                        FALSE, &color,
                        GIMP_PARAM_STATIC_STRINGS);

  g_type_class_add_private (object_class, sizeof (GimpColorConfigPrivate));
}

static void
gimp_color_config_init (GimpColorConfig *config)
{
}

static void
gimp_color_config_finalize (GObject *object)
{
  GimpColorConfig *color_config = GIMP_COLOR_CONFIG (object);

  if (color_config->rgb_profile)
    g_free (color_config->rgb_profile);

  if (color_config->gray_profile)
    g_free (color_config->gray_profile);

  if (color_config->cmyk_profile)
    g_free (color_config->cmyk_profile);

  if (color_config->display_profile)
    g_free (color_config->display_profile);

  if (color_config->printer_profile)
    g_free (color_config->printer_profile);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_config_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpColorConfig        *color_config = GIMP_COLOR_CONFIG (object);
  GimpColorConfigPrivate *priv         = GET_PRIVATE (object);
  GError                 *error        = NULL;

  switch (property_id)
    {
    case PROP_MODE:
      color_config->mode = g_value_get_enum (value);
      break;
    case PROP_RGB_PROFILE:
      gimp_color_config_set_rgb_profile (color_config,
                                         g_value_get_string (value),
                                         &error);
      break;
    case PROP_GRAY_PROFILE:
      gimp_color_config_set_gray_profile (color_config,
                                          g_value_get_string (value),
                                          &error);
      break;
    case PROP_CMYK_PROFILE:
      gimp_color_config_set_cmyk_profile (color_config,
                                          g_value_get_string (value),
                                          &error);
      break;
    case PROP_DISPLAY_PROFILE:
      gimp_color_config_set_display_profile (color_config,
                                             g_value_get_string (value),
                                             &error);
      break;
    case PROP_DISPLAY_PROFILE_FROM_GDK:
      color_config->display_profile_from_gdk = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_PROFILE:
      gimp_color_config_set_simulation_profile (color_config,
                                                g_value_get_string (value),
                                                &error);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      color_config->display_intent = g_value_get_enum (value);
      break;
    case PROP_DISPLAY_USE_BPC:
      color_config->display_use_black_point_compensation = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_OPTIMIZE:
      priv->display_optimize = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      color_config->simulation_intent = g_value_get_enum (value);
      break;
    case PROP_SIMULATION_USE_BPC:
      color_config->simulation_use_black_point_compensation = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_OPTIMIZE:
      priv->simulation_optimize = g_value_get_boolean (value);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      color_config->simulation_gamut_check = g_value_get_boolean (value);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      color_config->out_of_gamut_color = *(GimpRGB *) g_value_get_boxed (value);
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
gimp_color_config_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorConfig        *color_config = GIMP_COLOR_CONFIG (object);
  GimpColorConfigPrivate *priv         = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, color_config->mode);
      break;
    case PROP_RGB_PROFILE:
      g_value_set_string (value, color_config->rgb_profile);
      break;
    case PROP_GRAY_PROFILE:
      g_value_set_string (value, color_config->gray_profile);
      break;
    case PROP_CMYK_PROFILE:
      g_value_set_string (value, color_config->cmyk_profile);
      break;
    case PROP_DISPLAY_PROFILE:
      g_value_set_string (value, color_config->display_profile);
      break;
    case PROP_DISPLAY_PROFILE_FROM_GDK:
      g_value_set_boolean (value, color_config->display_profile_from_gdk);
      break;
    case PROP_SIMULATION_PROFILE:
      g_value_set_string (value, color_config->printer_profile);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      g_value_set_enum (value, color_config->display_intent);
      break;
    case PROP_DISPLAY_USE_BPC:
      g_value_set_boolean (value, color_config->display_use_black_point_compensation);
      break;
    case PROP_DISPLAY_OPTIMIZE:
      g_value_set_boolean (value, priv->display_optimize);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      g_value_set_enum (value, color_config->simulation_intent);
      break;
    case PROP_SIMULATION_USE_BPC:
      g_value_set_boolean (value, color_config->simulation_use_black_point_compensation);
      break;
    case PROP_SIMULATION_OPTIMIZE:
      g_value_set_boolean (value, priv->simulation_optimize);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      g_value_set_boolean (value, color_config->simulation_gamut_check);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      g_value_set_boxed (value, &color_config->out_of_gamut_color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * gimp_color_config_get_mode:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
GimpColorManagementMode
gimp_color_config_get_mode (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config),
                        GIMP_COLOR_MANAGEMENT_OFF);

  return config->mode;
}

/**
 * gimp_color_config_get_display_intent:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
GimpColorRenderingIntent
gimp_color_config_get_display_intent (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config),
                        GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL);

  return config->display_intent;
}

/**
 * gimp_color_config_get_display_bpc:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_display_bpc (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return config->display_use_black_point_compensation;
}

/**
 * gimp_color_config_get_display_optimize:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_display_optimize (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->display_optimize;
}

/**
 * gimp_color_config_get_display_profile_from_gdk:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_display_profile_from_gdk (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return config->display_profile_from_gdk;
}

/**
 * gimp_color_config_get_simulation_intent:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
GimpColorRenderingIntent
gimp_color_config_get_simulation_intent (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config),
                        GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL);

  return config->simulation_intent;
}

/**
 * gimp_color_config_get_simulation_bpc:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_simulation_bpc (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return config->simulation_use_black_point_compensation;
}

/**
 * gimp_color_config_get_simulation_optimize:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_simulation_optimize (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return GET_PRIVATE (config)->simulation_optimize;
}

/**
 * gimp_color_config_get_simulation_gamut_check:
 * @config: a #GimpColorConfig
 *
 * Since: 2.10
 **/
gboolean
gimp_color_config_get_simulation_gamut_check (GimpColorConfig *config)
{
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), FALSE);

  return config->simulation_gamut_check;
}

/**
 * gimp_color_config_get_rgb_color_profile:
 * @config: a #GimpColorConfig
 * @error:  return location for a #GError
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_config_get_rgb_color_profile (GimpColorConfig  *config,
                                         GError          **error)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (config->rgb_profile)
    {
      GFile *file = gimp_file_new_for_config_path (config->rgb_profile,
                                                   error);

      if (file)
        {
          profile = gimp_color_profile_new_from_file (file, error);

          if (profile && ! gimp_color_profile_is_rgb (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, GIMP_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for RGB color space."),
                           gimp_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * gimp_color_config_get_gray_color_profile:
 * @config: a #GimpColorConfig
 * @error:  return location for a #GError
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_config_get_gray_color_profile (GimpColorConfig  *config,
                                          GError          **error)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (config->gray_profile)
    {
      GFile *file = gimp_file_new_for_config_path (config->gray_profile,
                                                   error);

      if (file)
        {
          profile = gimp_color_profile_new_from_file (file, error);

          if (profile && ! gimp_color_profile_is_gray (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, GIMP_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for GRAY color space."),
                           gimp_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * gimp_color_config_get_cmyk_color_profile:
 * @config: a #GimpColorConfig
 * @error:  return location for a #GError
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_config_get_cmyk_color_profile (GimpColorConfig  *config,
                                          GError          **error)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (config->cmyk_profile)
    {
      GFile *file = gimp_file_new_for_config_path (config->cmyk_profile,
                                                   error);

      if (file)
        {
          profile = gimp_color_profile_new_from_file (file, error);

          if (profile && ! gimp_color_profile_is_cmyk (profile))
            {
              g_object_unref (profile);
              profile = NULL;

              g_set_error (error, GIMP_CONFIG_ERROR, 0,
                           _("Color profile '%s' is not for CMYK color space."),
                           gimp_file_get_utf8_name (file));
            }

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * gimp_color_config_get_display_color_profile:
 * @config: a #GimpColorConfig
 * @error:  return location for a #GError
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_config_get_display_color_profile (GimpColorConfig  *config,
                                             GError          **error)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (config->display_profile)
    {
      GFile *file = gimp_file_new_for_config_path (config->display_profile,
                                                   error);

      if (file)
        {
          profile = gimp_color_profile_new_from_file (file, error);

          g_object_unref (file);
        }
    }

  return profile;
}

/**
 * gimp_color_config_get_simulation_color_profile:
 * @config: a #GimpColorConfig
 * @error:  return location for a #GError
 *
 * Since: 2.10
 **/
GimpColorProfile *
gimp_color_config_get_simulation_color_profile (GimpColorConfig  *config,
                                                GError          **error)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (config->printer_profile)
    {
      GFile *file = gimp_file_new_for_config_path (config->printer_profile,
                                                   error);

      if (file)
        {
          profile = gimp_color_profile_new_from_file (file, error);

          g_object_unref (file);
        }
    }

  return profile;
}


/*  private functions  */

static void
gimp_color_config_set_rgb_profile (GimpColorConfig  *config,
                                   const gchar      *filename,
                                   GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = gimp_file_new_for_config_path (filename, error);

      if (file)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! gimp_color_profile_is_rgb (profile))
                {
                  g_set_error (error, GIMP_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for RGB "
                                 "color space."),
                               gimp_file_get_utf8_name (file));
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
      g_free (config->rgb_profile);
      config->rgb_profile = g_strdup (filename);
    }
}

static void
gimp_color_config_set_gray_profile (GimpColorConfig  *config,
                                    const gchar      *filename,
                                    GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = gimp_file_new_for_config_path (filename, error);

      if (file)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! gimp_color_profile_is_gray (profile))
                {
                  g_set_error (error, GIMP_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for GRAY "
                                 "color space."),
                               gimp_file_get_utf8_name (file));
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
      g_free (config->gray_profile);
      config->gray_profile = g_strdup (filename);
    }
}

static void
gimp_color_config_set_cmyk_profile (GimpColorConfig  *config,
                                    const gchar      *filename,
                                    GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = gimp_file_new_for_config_path (filename, error);

      if (file)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_file (file, error);

          if (profile)
            {
              if (! gimp_color_profile_is_cmyk (profile))
                {
                  g_set_error (error, GIMP_CONFIG_ERROR, 0,
                               _("Color profile '%s' is not for CMYK "
                                 "color space."),
                               gimp_file_get_utf8_name (file));
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
      g_free (config->cmyk_profile);
      config->cmyk_profile = g_strdup (filename);
    }
}

static void
gimp_color_config_set_display_profile (GimpColorConfig  *config,
                                       const gchar      *filename,
                                       GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = gimp_file_new_for_config_path (filename, error);

      if (file)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_file (file, error);

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
      g_free (config->display_profile);
      config->display_profile = g_strdup (filename);
    }
}

static void
gimp_color_config_set_simulation_profile (GimpColorConfig  *config,
                                          const gchar      *filename,
                                          GError          **error)
{
  gboolean success = TRUE;

  if (filename)
    {
      GFile *file = gimp_file_new_for_config_path (filename, error);

      if (file)
        {
          GimpColorProfile *profile;

          profile = gimp_color_profile_new_from_file (file, error);

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
      g_free (config->printer_profile);
      config->printer_profile = g_strdup (filename);
    }
}
