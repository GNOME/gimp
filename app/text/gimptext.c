/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "text/text-types.h"

#include "config/gimpconfig-params.h"

#include "gimptext.h"

enum
{
  PROP_0,
  PROP_TEXT,
  PROP_FONT,
  PROP_SIZE,
  PROP_BORDER,
  PROP_UNIT,
  PROP_LETTER_SPACING,
  PROP_LINE_SPACING
};

static void  gimp_text_class_init   (GimpTextClass *klass);
static void  gimp_text_finalize     (GObject       *object);
static void  gimp_text_set_property (GObject        *object,
                                     guint           property_id,
                                     const GValue   *value,
                                     GParamSpec     *pspec);

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

      text_type = g_type_register_static (G_TYPE_OBJECT,
                                          "GimpText", &text_info, 0);
    }

  return text_type;
}

static void
gimp_text_class_init (GimpTextClass *klass)
{
  GObjectClass *object_class;
  GParamSpec   *param_spec;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = gimp_text_finalize;
  object_class->set_property = gimp_text_set_property;

  param_spec = g_param_spec_string ("text", NULL, NULL,
                                    NULL,
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_TEXT, param_spec);

  param_spec = g_param_spec_string ("font", NULL, NULL,
                                    "Sans",
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_FONT, param_spec);

  param_spec = g_param_spec_double ("size", NULL, NULL,
                                    0.0, G_MAXFLOAT, 18.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_SIZE, param_spec);

  param_spec = g_param_spec_double ("border", NULL, NULL,
                                    0.0, G_MAXFLOAT, 0.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_BORDER, param_spec);

  param_spec = gimp_param_spec_unit ("unit", NULL, NULL,
                                     TRUE, GIMP_UNIT_PIXEL,
                                     G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class, PROP_UNIT, param_spec);

  param_spec = g_param_spec_double ("letter-spacing", NULL, NULL,
                                    0.0, 64.0, 1.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class,
                                   PROP_LETTER_SPACING, param_spec);

  param_spec = g_param_spec_double ("line-spacing", NULL, NULL,
                                    0.0, 64.0, 1.0,
                                    G_PARAM_CONSTRUCT | G_PARAM_WRITABLE);
  g_object_class_install_property (object_class,
                                   PROP_LINE_SPACING, param_spec);
}

static void
gimp_text_finalize (GObject *object)
{
  GimpText *text = GIMP_TEXT (object);

  if (text->str)
    {
      g_free (text->str);
      text->str = NULL;
    }
  if (text->font)
    {
      g_free (text->font);
      text->font = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_text_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  GimpText *text = GIMP_TEXT (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_free (text->str);
      text->str = g_value_dup_string (value);
      break;
    case PROP_FONT:
      g_free (text->font);
      text->font = g_value_dup_string (value);
      break;
    case PROP_SIZE:
      text->size = g_value_get_double (value);
      break;
    case PROP_BORDER:
      text->border = g_value_get_double (value);
      break;
    case PROP_UNIT:
      text->unit = g_value_get_int (value);
      break;
    case PROP_LETTER_SPACING:
      text->letter_spacing = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      text->line_spacing = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
