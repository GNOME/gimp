/* The GIMP -- an image manipulation program
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

#include <glib-object.h>

#include "libgimpbase/gimplimits.h"
#include "libgimpcolor/gimpcolor.h"

#include "text/text-types.h"

#include "config/gimpconfig.h"
#include "config/gimpconfig-params.h"

#include "gimptext.h"

enum
{
  PROP_0,
  PROP_TEXT,
  PROP_FONT,
  PROP_FONT_SIZE,
  PROP_FONT_SIZE_UNIT,
  PROP_COLOR,
  PROP_LETTER_SPACING,
  PROP_LINE_SPACING,
  PROP_FIXED_WIDTH,
  PROP_FIXED_HEIGHT,
  PROP_GRAVITY,
  PROP_BORDER
};

static void  gimp_text_class_init   (GimpTextClass *klass);
static void  gimp_text_finalize     (GObject       *object);
static void  gimp_text_get_property (GObject       *object,
                                     guint          property_id,
                                     GValue        *value,
                                     GParamSpec    *pspec);
static void  gimp_text_set_property (GObject       *object,
                                     guint          property_id,
                                     const GValue  *value,
                                     GParamSpec    *pspec);

static GObjectClass *parent_class = NULL;


GType
gimp_text_get_type (void)
{
  static GType text_type = 0;

  if (! text_type)
    {
      static const GTypeInfo text_info =
      {
        sizeof (GimpTextClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_text_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpText),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };
      static const GInterfaceInfo text_iface_info = 
      {
        NULL,           /* iface_init     */
        NULL,           /* iface_finalize */ 
        NULL            /* iface_data     */
      };

      text_type = g_type_register_static (G_TYPE_OBJECT,
                                          "GimpText", &text_info, 0);

      g_type_add_interface_static (text_type,
                                   GIMP_TYPE_CONFIG_INTERFACE,
                                   &text_iface_info);
    }

  return text_type;
}

static void
gimp_text_class_init (GimpTextClass *klass)
{
  GObjectClass *object_class;
  GParamSpec   *param_spec;
  GimpRGB       black;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_text_finalize;
  object_class->get_property = gimp_text_get_property;
  object_class->set_property = gimp_text_set_property;

  gimp_rgba_set (&black, 0.0, 0.0, 0.0, GIMP_OPACITY_OPAQUE);

  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_TEXT,
				   "text", NULL,
				   "GIMP",
				   0);
  GIMP_CONFIG_INSTALL_PROP_STRING (object_class, PROP_FONT,
				   "font", NULL,
				   "Sans",
				   0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
				   "font-size", NULL,
				   0.0, 8192.0, 18.0,
				   0);
  GIMP_CONFIG_INSTALL_PROP_UNIT (object_class, PROP_FONT_SIZE_UNIT,
				 "font-size-unit", NULL,
				 TRUE, GIMP_UNIT_PIXEL,
				 0);
  GIMP_CONFIG_INSTALL_PROP_COLOR (object_class, PROP_COLOR,
				  "color", NULL,
				  &black,
				  0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
				   "letter-spacing", NULL,
                                    0.0, 64.0, 1.0,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
				   "line-spacing", NULL,
                                   0.0, 64.0, 1.0,
                                   0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_FIXED_WIDTH,
                                "fixed-width", NULL,
                                0, GIMP_MAX_IMAGE_SIZE, 0,
                                0);
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_FIXED_HEIGHT,
                                "fixed-height", NULL,
                                0, GIMP_MAX_IMAGE_SIZE, 0,
                                0);
  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_GRAVITY,
                                "gravity", NULL,
                                 GIMP_TYPE_GRAVITY_TYPE, GIMP_GRAVITY_CENTER,
                                 0);

  /*  border does only exist to implement the old text API  */
  param_spec = g_param_spec_int ("border", NULL, NULL,
                                 0, GIMP_MAX_IMAGE_SIZE, 0,
                                 G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_BORDER, param_spec);

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
    case PROP_FONT_SIZE_UNIT:
      g_value_set_int (value, text->font_size_unit);
      break;
    case PROP_COLOR:
      g_value_set_boxed (value, &text->color);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, text->letter_spacing);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, text->line_spacing);
      break;
    case PROP_FIXED_WIDTH:
      g_value_set_int (value, text->fixed_width);
      break;
    case PROP_FIXED_HEIGHT:
      g_value_set_int (value, text->fixed_height);
      break;
    case PROP_GRAVITY:
      g_value_set_enum (value, text->gravity);
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
  GimpText *text = GIMP_TEXT (object);
  GimpRGB  *color;

  switch (property_id)
    {
    case PROP_TEXT:
      g_free (text->text);
      text->text = g_value_dup_string (value);
      break;
    case PROP_FONT:
      g_free (text->font);
      text->font = g_value_dup_string (value);
      break;
    case PROP_FONT_SIZE:
      text->font_size = g_value_get_double (value);
      break;
    case PROP_FONT_SIZE_UNIT:
      text->font_size_unit = g_value_get_int (value);
      break;
    case PROP_COLOR:
      color = g_value_get_boxed (value);
      text->color = *color;
      break;
    case PROP_LETTER_SPACING:
      text->letter_spacing = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      text->line_spacing = g_value_get_double (value);
      break;
    case PROP_FIXED_WIDTH:
      text->fixed_width = g_value_get_int (value);
      break;
    case PROP_FIXED_HEIGHT:
      text->fixed_height = g_value_get_int (value);
      break;
    case PROP_GRAVITY:
      text->gravity = g_value_get_enum (value);
      break;
    case PROP_BORDER:
      text->border = g_value_get_int (value);
      if (text->border > 0)
        text->gravity = GIMP_GRAVITY_CENTER;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
