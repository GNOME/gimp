/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpColorConfig class
 * Copyright (C) 2004  Stefan Döhla <stefan@doehla.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpcolor/gimpcolor.h"

#include "gimpconfigtypes.h"

#include "gimpcolorconfig-enums.h"

#include "gimpcolorconfig.h"
#include "gimpconfig-iface.h"
#include "gimpconfig-params.h"
#include "gimpconfig-path.h"

#include "libgimp/libgimp-intl.h"


#define COLOR_MANAGEMENT_MODE_BLURB \
  N_("Mode of operation for color management.")
#define DISPLAY_PROFILE_BLURB \
  N_("The color profile of your (primary) monitor.")
#define DISPLAY_PROFILE_FROM_GDK_BLURB \
  N_("When enabled, GIMP will try to use the display color profile from " \
     "the windowing system.  The configured monitor profile is then only " \
     "used as a fallback.")
#define RGB_PROFILE_BLURB \
  N_("The default RGB working space color profile.")
#define CMYK_PROFILE_BLURB \
  N_("The CMYK color profile used to convert between RGB and CMYK.")
#define PRINTER_PROFILE_BLURB \
  N_("The color profile used for simulating a printed version (softproof).")
#define DISPLAY_RENDERING_INTENT_BLURB \
  N_("Sets how colors are mapped for your display.")
#define SIMULATION_RENDERING_INTENT_BLURB \
  N_("Sets how colors are converted from RGB working space to the " \
     "print simulation device.")
#define SIMULATION_GAMUT_CHECK_BLURB \
  N_("When enabled, the print simulation will mark colors which can not be " \
     "represented in the target color space.")
#define OUT_OF_GAMUT_COLOR_BLURB \
  N_("The color to use for marking colors which are out of gamut.")


enum
{
  PROP_0,
  PROP_MODE,
  PROP_RGB_PROFILE,
  PROP_CMYK_PROFILE,
  PROP_DISPLAY_PROFILE,
  PROP_DISPLAY_PROFILE_FROM_GDK,
  PROP_PRINTER_PROFILE,
  PROP_DISPLAY_RENDERING_INTENT,
  PROP_SIMULATION_RENDERING_INTENT,
  PROP_SIMULATION_GAMUT_CHECK,
  PROP_OUT_OF_GAMUT_COLOR,
  PROP_DISPLAY_MODULE
};


static void  gimp_color_config_finalize     (GObject      *object);
static void  gimp_color_config_set_property (GObject      *object,
                                             guint         property_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void  gimp_color_config_get_property (GObject      *object,
                                             guint         property_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);


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

  gimp_rgba_set_uchar (&color, 0x80, 0x80, 0x80, 0xff);

  object_class->finalize     = gimp_color_config_finalize;
  object_class->set_property = gimp_color_config_set_property;
  object_class->get_property = gimp_color_config_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_MODE,
                                 "mode", COLOR_MANAGEMENT_MODE_BLURB,
                                 GIMP_TYPE_COLOR_MANAGEMENT_MODE,
                                 GIMP_COLOR_MANAGEMENT_DISPLAY,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_RGB_PROFILE,
                                 "rgb-profile", RGB_PROFILE_BLURB,
                                 GIMP_CONFIG_PATH_FILE, NULL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_CMYK_PROFILE,
                                 "cmyk-profile", CMYK_PROFILE_BLURB,
                                 GIMP_CONFIG_PATH_FILE, NULL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_DISPLAY_PROFILE,
                                 "display-profile", DISPLAY_PROFILE_BLURB,
                                 GIMP_CONFIG_PATH_FILE, NULL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_DISPLAY_PROFILE_FROM_GDK,
                                    "display-profile-from-gdk",
                                    DISPLAY_PROFILE_FROM_GDK_BLURB,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PRINTER_PROFILE,
                                 "printer-profile", PRINTER_PROFILE_BLURB,
                                 GIMP_CONFIG_PATH_FILE, NULL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_DISPLAY_RENDERING_INTENT,
                                 "display-rendering-intent",
                                 DISPLAY_RENDERING_INTENT_BLURB,
                                 GIMP_TYPE_COLOR_RENDERING_INTENT,
                                 GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_SIMULATION_RENDERING_INTENT,
                                 "simulation-rendering-intent",
                                 SIMULATION_RENDERING_INTENT_BLURB,
                                 GIMP_TYPE_COLOR_RENDERING_INTENT,
                                 GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SIMULATION_GAMUT_CHECK,
                                    "simulation-gamut-check",
                                    SIMULATION_GAMUT_CHECK_BLURB,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_RGB (object_class, PROP_OUT_OF_GAMUT_COLOR,
                                "out-of-gamut-color",
                                OUT_OF_GAMUT_COLOR_BLURB,
                                FALSE, &color,
                                GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_DISPLAY_MODULE,
                                   "display-module", NULL,
                                   "CdisplayLcms",
                                   GIMP_PARAM_STATIC_STRINGS);
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

  if (color_config->cmyk_profile)
    g_free (color_config->cmyk_profile);

  if (color_config->display_profile)
    g_free (color_config->display_profile);

  if (color_config->printer_profile)
    g_free (color_config->printer_profile);

  if (color_config->display_module)
    g_free (color_config->display_module);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_color_config_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpColorConfig *color_config = GIMP_COLOR_CONFIG (object);

  switch (property_id)
    {
    case PROP_MODE:
      color_config->mode = g_value_get_enum (value);
      break;
    case PROP_RGB_PROFILE:
      g_free (color_config->rgb_profile);
      color_config->rgb_profile = g_value_dup_string (value);
      break;
    case PROP_CMYK_PROFILE:
      g_free (color_config->cmyk_profile);
      color_config->cmyk_profile = g_value_dup_string (value);
      break;
    case PROP_DISPLAY_PROFILE:
      g_free (color_config->display_profile);
      color_config->display_profile = g_value_dup_string (value);
      break;
    case PROP_DISPLAY_PROFILE_FROM_GDK:
      color_config->display_profile_from_gdk = g_value_get_boolean (value);
      break;
    case PROP_PRINTER_PROFILE:
      g_free (color_config->printer_profile);
      color_config->printer_profile = g_value_dup_string (value);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      color_config->display_intent = g_value_get_enum (value);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      color_config->simulation_intent = g_value_get_enum (value);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      color_config->simulation_gamut_check = g_value_get_boolean (value);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      color_config->out_of_gamut_color = *(GimpRGB *) g_value_get_boxed (value);
      break;
    case PROP_DISPLAY_MODULE:
      g_free (color_config->display_module);
      color_config->display_module = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_config_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpColorConfig *color_config = GIMP_COLOR_CONFIG (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, color_config->mode);
      break;
    case PROP_RGB_PROFILE:
      g_value_set_string (value, color_config->rgb_profile);
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
    case PROP_PRINTER_PROFILE:
      g_value_set_string (value, color_config->printer_profile);
      break;
    case PROP_DISPLAY_RENDERING_INTENT:
      g_value_set_enum (value, color_config->display_intent);
      break;
    case PROP_SIMULATION_RENDERING_INTENT:
      g_value_set_enum (value, color_config->simulation_intent);
      break;
    case PROP_SIMULATION_GAMUT_CHECK:
      g_value_set_boolean (value, color_config->simulation_gamut_check);
      break;
    case PROP_OUT_OF_GAMUT_COLOR:
      g_value_set_boxed (value, &color_config->out_of_gamut_color);
      break;
    case PROP_DISPLAY_MODULE:
      g_value_set_string (value, color_config->display_module);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
