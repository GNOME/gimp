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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <glib-object.h>

#include "gimpconfig.h"
#include "gimpconfig-deserialize.h"
#include "gimpconfig-serialize.h"


enum
{
  PROP_0,
  PROP_MARCHING_ANTS_SPEED,
  PROP_PREVIEW_SIZE
};

#define GIMP_TYPE_PREVIEW_SIZE (gimp_preview_size_get_type ())

GType        gimp_preview_size_get_type (void) G_GNUC_CONST;

static void  gimp_config_class_init   (GimpConfigClass  *klass);
static void  gimp_config_set_property (GObject          *object,
                                       guint             property_id,
                                       const GValue     *value,
                                       GParamSpec       *pspec);
static void  gimp_config_get_property (GObject          *object,
                                       guint             property_id,
                                       GValue           *value,
                                       GParamSpec       *pspec);


static GimpObjectClass *parent_class = NULL;

static const GEnumValue gimp_preview_size_enum_values[] =
{
  { GIMP_PREVIEW_SIZE_NONE,        "none"        }, 
  { GIMP_PREVIEW_SIZE_TINY,        "tiny"        },
  { GIMP_PREVIEW_SIZE_EXTRA_SMALL, "extra-small" },
  { GIMP_PREVIEW_SIZE_SMALL,       "small"       },
  { GIMP_PREVIEW_SIZE_MEDIUM,      "medium"      },
  { GIMP_PREVIEW_SIZE_LARGE,       "large"       },
  { GIMP_PREVIEW_SIZE_EXTRA_LARGE, "extra-large" },
  { GIMP_PREVIEW_SIZE_HUGE,        "huge"        },
  { GIMP_PREVIEW_SIZE_ENORMOUS,    "enormous"    },
  { GIMP_PREVIEW_SIZE_GIGANTIC,    "gigantic"    }
};


GType
gimp_preview_size_get_type (void)
{
  static GType size_type = 0;

  if (! size_type)
    {
      size_type = g_enum_register_static ("GimpPreviewSize",
                                          gimp_preview_size_enum_values);
    }

  return size_type;
}

GType 
gimp_config_get_type (void)
{
  static GType config_type = 0;

  if (! config_type)
    {
      static const GTypeInfo config_info =
      {
        sizeof (GimpConfigClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_config_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (GimpConfig),
	0,              /* n_preallocs    */
	NULL            /* instance_init  */
      };

      config_type = g_type_register_static (GIMP_TYPE_OBJECT, 
                                            "GimpConfig", 
                                            &config_info, 0);
    }

  return config_type;
}

static void
gimp_config_class_init (GimpConfigClass *klass)
{
  GObjectClass *object_class;

  parent_class = g_type_class_peek_parent (klass);

  object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_config_set_property;
  object_class->get_property = gimp_config_get_property;

  g_object_class_install_property (object_class,
                                   PROP_MARCHING_ANTS_SPEED,
                                   g_param_spec_uint ("marching-ants-speed",
                                                      NULL, NULL,
                                                      50, G_MAXINT,
                                                      300,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class,
                                   PROP_PREVIEW_SIZE,
                                   g_param_spec_enum ("preview-size",
                                                      NULL, NULL,
                                                      GIMP_TYPE_PREVIEW_SIZE,
                                                      GIMP_PREVIEW_SIZE_MEDIUM,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
}

static void
gimp_config_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  GimpConfig *gimp_config;

  gimp_config = GIMP_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      gimp_config->marching_ants_speed = g_value_get_uint (value);
      break;
    case PROP_PREVIEW_SIZE:
      gimp_config->preview_size = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_config_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  GimpConfig *gimp_config;

  gimp_config = GIMP_CONFIG (object);

  switch (property_id)
    {
    case PROP_MARCHING_ANTS_SPEED:
      g_value_set_uint (value, gimp_config->marching_ants_speed);
      break;
    case PROP_PREVIEW_SIZE:
      g_value_set_enum (value, gimp_config->preview_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

void
gimp_config_serialize (GimpConfig *config)
{
  FILE *file;

  g_return_if_fail (GIMP_CONFIG (config));
  g_return_if_fail (GIMP_OBJECT (config)->name != NULL);

  file = fopen (GIMP_OBJECT (config)->name, "w");

  if (file)
    {
      gimp_config_serialize_properties (config, file);
      fclose (file);
    }
}

gboolean
gimp_config_deserialize (GimpConfig *config)
{
  gint      fd;
  GScanner *scanner;
  gboolean  success;

  g_return_val_if_fail (GIMP_CONFIG (config), FALSE);
  g_return_val_if_fail (GIMP_OBJECT (config)->name != NULL, FALSE);
  
  fd = open (GIMP_OBJECT (config)->name, O_RDONLY);

  if (fd == -1)
    return FALSE;

  scanner = g_scanner_new (NULL);

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z "-" G_CSET_A_2_Z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z "-" G_CSET_A_2_Z );

  g_scanner_input_file (scanner, fd);
  scanner->input_name = GIMP_OBJECT (config)->name;

  success = gimp_config_deserialize_properties (config, scanner);

  g_scanner_destroy (scanner);
  close (fd);

  return success;
}
