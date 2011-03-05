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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpfilloptions.h"
#include "gimpviewable.h"


enum
{
  PROP_0,
  PROP_STYLE,
  PROP_ANTIALIAS,
  PROP_PATTERN_VIEW_TYPE,
  PROP_PATTERN_VIEW_SIZE
};


typedef struct _GimpFillOptionsPrivate GimpFillOptionsPrivate;

struct _GimpFillOptionsPrivate
{
  GimpFillStyle style;

  gboolean      antialias;

  GimpViewType  pattern_view_type;
  GimpViewSize  pattern_view_size;
};

#define GET_PRIVATE(options) \
        G_TYPE_INSTANCE_GET_PRIVATE (options, \
                                     GIMP_TYPE_FILL_OPTIONS, \
                                     GimpFillOptionsPrivate)


static void   gimp_fill_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   gimp_fill_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (GimpFillOptions, gimp_fill_options, GIMP_TYPE_CONTEXT)


static void
gimp_fill_options_class_init (GimpFillOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_fill_options_set_property;
  object_class->get_property = gimp_fill_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_STYLE,
                                 "style", NULL,
                                 GIMP_TYPE_FILL_STYLE,
                                 GIMP_FILL_STYLE_SOLID,
                                 GIMP_PARAM_STATIC_STRINGS);

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                                    "antialias", NULL,
                                    TRUE,
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

  g_type_class_add_private (klass, sizeof (GimpFillOptionsPrivate));
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
      break;
    case PROP_ANTIALIAS:
      private->antialias = g_value_get_boolean (value);
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
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, private->antialias);
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


/*  public functions  */

GimpFillOptions *
gimp_fill_options_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_FILL_OPTIONS,
                       "gimp", gimp,
                       NULL);
}

GimpFillStyle
gimp_fill_options_get_style (GimpFillOptions *options)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), GIMP_FILL_STYLE_SOLID);

  return GET_PRIVATE (options)->style;
}

gboolean
gimp_fill_options_get_antialias (GimpFillOptions *options)
{
  g_return_val_if_fail (GIMP_IS_FILL_OPTIONS (options), FALSE);

  return GET_PRIVATE (options)->antialias;
}
