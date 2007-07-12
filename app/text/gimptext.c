/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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

#include <string.h>

#include <glib-object.h>
#include <pango/pango.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"

#include "text-types.h"

#include "core/gimpstrokeoptions.h"
#include "core/gimp-utils.h"

#include "gimptext.h"


enum
{
  PROP_0,
  PROP_TEXT,
  PROP_FONT,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_HINTING,
  PROP_AUTOHINT,
  PROP_ANTIALIAS,
  PROP_KERNING,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_COLOR,
  PROP_OUTLINE,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,
  PROP_BOX_MODE,
  PROP_BOX_WIDTH,
  PROP_BOX_HEIGHT,
  PROP_BOX_UNIT,
  PROP_TRANSFORMATION,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_BORDER
};


static void     gimp_text_finalize     (GObject      *object);
static void     gimp_text_get_property (GObject      *object,
                                        guint         property_id,
                                        GValue       *value,
                                        GParamSpec   *pspec);
static void     gimp_text_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);
static gint64   gimp_text_get_memsize  (GimpObject   *object,
                                        gint64       *gui_size);


G_DEFINE_TYPE_WITH_CODE (GimpText, gimp_text, GIMP_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_CONFIG, NULL))

#define parent_class gimp_text_parent_class


static void
gimp_text_class_init (GimpTextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpRGB          black;
  GimpMatrix2      identity;
  gchar           *language;

  object_class->finalize         = gimp_text_finalize;
  object_class->get_property     = gimp_text_get_property;
  object_class->set_property     = gimp_text_set_property;

  gimp_object_class->get_memsize = gimp_text_get_memsize;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);
  gimp_matrix2_identity (&identity);

  language = gimp_get_default_language (NULL);

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_TEXT,
                                   "text", NULL,
                                   NULL,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_FONT,
                                   "font", NULL,
                                   "Sans",
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                                   "font-size", NULL,
                                   0.0, 8192.0, 18.0,
                                   GIMP_PARAM_STATIC_STRINGS);
  /*
   *  We use the name "font-size-unit" for backward compatibility.
   *  The unit is also used for other sizes in the text object.
   */
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_UNIT,
                                 "font-size-unit", NULL,
                                 TRUE, FALSE, GIMP_UNIT_PIXEL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_HINTING,
                                    "hinting", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_AUTOHINT,
                                    "autohint", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS |
                                    GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_KERNING,
                                    "kerning", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS |
                                    GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_LANGUAGE,
                                   "language", NULL,
                                   language,
                                   GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BASE_DIR,
                                "base-direction", NULL,
                                 GIMP_TYPE_TEXT_DIRECTION,
                                 GIMP_TEXT_DIRECTION_LTR,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_RGB (object_class, PROP_COLOR,
                                "color", NULL,
                                FALSE, &black,
                                GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_OUTLINE,
                                "outline", NULL,
                                 GIMP_TYPE_TEXT_OUTLINE,
                                 GIMP_TEXT_OUTLINE_NONE,
                                 GIMP_PARAM_STATIC_STRINGS |
                                 GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                                "justify", NULL,
                                 GIMP_TYPE_TEXT_JUSTIFICATION,
                                 GIMP_TEXT_JUSTIFY_LEFT,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_INDENTATION,
                                   "indent", NULL,
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                                   "line-spacing", NULL,
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                                   "letter-spacing", NULL,
                                   -8192.0, 8192.0, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BOX_MODE,
                                "box-mode",
                                 NULL,
                                 GIMP_TYPE_TEXT_BOX_MODE,
                                 GIMP_TEXT_BOX_DYNAMIC,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BOX_WIDTH,
                                   "box-width", NULL,
                                   0.0, GIMP_MAX_IMAGE_SIZE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BOX_HEIGHT,
                                   "box-height", NULL,
                                   0.0, GIMP_MAX_IMAGE_SIZE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_BOX_UNIT,
                                 "box-unit", NULL,
                                 TRUE, FALSE, GIMP_UNIT_PIXEL,
                                 GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_MATRIX2 (object_class, PROP_TRANSFORMATION,
                                    "transformation", NULL,
                                    &identity,
                                    GIMP_PARAM_STATIC_STRINGS |
                                    GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_OFFSET_X,
                                   "offset-x", NULL,
                                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_OFFSET_Y,
                                   "offset-y", NULL,
                                   -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                                   GIMP_PARAM_STATIC_STRINGS |
                                   GIMP_CONFIG_PARAM_DEFAULTS);

  /*  border does only exist to implement the old text API  */
  g_object_class_install_property (object_class, PROP_BORDER,
                                   g_param_spec_int ("border", NULL, NULL,
                                                     0, GIMP_MAX_IMAGE_SIZE, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     GIMP_PARAM_WRITABLE));

  g_free (language);
}

static void
gimp_text_init (GimpText *text)
{
}

static void
gimp_text_finalize (GObject *object)
{
  GimpText *text = GIMP_TEXT (object);

  if (text->text)
    {
      g_free (text->text);
      text->text = NULL;
    }
  if (text->font)
    {
      g_free (text->font);
      text->font = NULL;
    }
  if (text->language)
    {
      g_free (text->language);
      text->language = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  GimpText *text = GIMP_TEXT (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, text->text);
      break;
    case PROP_FONT:
      g_value_set_string (value, text->font);
      break;
    case PROP_FONT_SIZE:
      g_value_set_double (value, text->font_size);
      break;
    case PROP_UNIT:
      g_value_set_int (value, text->unit);
      break;
    case PROP_HINTING:
      g_value_set_boolean (value, text->hinting);
      break;
    case PROP_AUTOHINT:
      g_value_set_boolean (value, text->autohint);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, text->antialias);
      break;
    case PROP_KERNING:
      g_value_set_boolean (value, text->kerning);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, text->base_dir);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, text->language);
      break;
    case PROP_COLOR:
      g_value_set_boxed (value, &text->color);
      break;
    case PROP_OUTLINE:
      g_value_set_enum (value, text->outline);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, text->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, text->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, text->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, text->letter_spacing);
      break;
    case PROP_BOX_MODE:
      g_value_set_enum (value, text->box_mode);
      break;
    case PROP_BOX_WIDTH:
      g_value_set_double (value, text->box_width);
      break;
    case PROP_BOX_HEIGHT:
      g_value_set_double (value, text->box_height);
      break;
    case PROP_BOX_UNIT:
      g_value_set_int (value, text->unit);
      break;
    case PROP_TRANSFORMATION:
      g_value_set_boxed (value, &text->transformation);
      break;
    case PROP_OFFSET_X:
      g_value_set_double (value, text->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, text->offset_y);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_text_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpText    *text = GIMP_TEXT (object);
  GimpRGB     *color;
  GimpMatrix2 *matrix;

  switch (property_id)
    {
    case PROP_TEXT:
      g_free (text->text);
      text->text = g_value_dup_string (value);
      break;
    case PROP_FONT:
      {
        const gchar *font = g_value_get_string (value);
        gsize        len  = strlen (font);

        if (g_str_has_suffix (font, " Not-Rotated"))
          len -= strlen ( " Not-Rotated");

        g_free (text->font);
        text->font = g_strndup (font, len);
      }
      break;
    case PROP_FONT_SIZE:
      text->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      text->unit = g_value_get_int (value);
      break;
    case PROP_HINTING:
      text->hinting = g_value_get_boolean (value);
      break;
    case PROP_AUTOHINT:
      text->autohint = g_value_get_boolean (value);
      break;
    case PROP_ANTIALIAS:
      text->antialias = g_value_get_boolean (value);
      break;
    case PROP_KERNING:
      text->kerning = g_value_get_boolean (value);
      break;
    case PROP_LANGUAGE:
      g_free (text->language);
      text->language = g_value_dup_string (value);
      break;
    case PROP_BASE_DIR:
      text->base_dir = g_value_get_enum (value);
      break;
    case PROP_COLOR:
      color = g_value_get_boxed (value);
      text->color = *color;
      break;
    case PROP_OUTLINE:
      text->outline = g_value_get_enum (value);
      break;
    case PROP_JUSTIFICATION:
      text->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      text->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      text->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      text->letter_spacing = g_value_get_double (value);
      break;
    case PROP_BOX_MODE:
      text->box_mode = g_value_get_enum (value);
      break;
    case PROP_BOX_WIDTH:
      text->box_width = g_value_get_double (value);
      break;
    case PROP_BOX_HEIGHT:
      text->box_height = g_value_get_double (value);
      break;
    case PROP_BOX_UNIT:
      text->box_unit = g_value_get_int (value);
      break;
    case PROP_TRANSFORMATION:
      matrix = g_value_get_boxed (value);
      text->transformation = *matrix;
      break;
    case PROP_OFFSET_X:
      text->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      text->offset_y = g_value_get_double (value);
      break;
    case PROP_BORDER:
      text->border = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_text_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpText *text    = GIMP_TEXT (object);
  gint64    memsize = 0;

  if (text->text)
    memsize += strlen (text->text) + 1;

  if (text->font)
    memsize += strlen (text->font) + 1;

  if (text->language)
    memsize += strlen (text->language) + 1;

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
gimp_text_get_transformation (GimpText    *text,
                              GimpMatrix3 *matrix)
{
  g_return_if_fail (GIMP_IS_TEXT (text));
  g_return_if_fail (matrix != NULL);

  matrix->coeff[0][0] = text->transformation.coeff[0][0];
  matrix->coeff[0][1] = text->transformation.coeff[0][1];
  matrix->coeff[0][2] = text->offset_x;

  matrix->coeff[1][0] = text->transformation.coeff[1][0];
  matrix->coeff[1][1] = text->transformation.coeff[1][1];
  matrix->coeff[1][2] = text->offset_y;

  matrix->coeff[2][0] = 0.0;
  matrix->coeff[2][1] = 0.0;
  matrix->coeff[2][1] = 1.0;
}
