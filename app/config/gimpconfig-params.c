/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001-2003  Sven Neumann <sven@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "config-types.h"

#include "gimpconfig-params.h"
#include "gimpconfig-types.h"


/*
 * GIMP_TYPE_PARAM_RGB
 */

#define GIMP_PARAM_SPEC_RGB(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_RGB, GimpParamSpecRGB))

static void       gimp_param_rgb_class_init  (GParamSpecClass *class);
static void       gimp_param_rgb_init        (GParamSpec      *pspec);
static void       gimp_param_rgb_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gboolean   gimp_param_rgb_validate    (GParamSpec      *pspec,
                                              GValue          *value);
static gint       gimp_param_rgb_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _GimpParamSpecRGB GimpParamSpecRGB;

struct _GimpParamSpecRGB
{
  GParamSpecBoxed  parent_instance;

  GimpRGB          default_value;
};

GType
gimp_param_rgb_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_rgb_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecRGB),
        0,
        (GInstanceInitFunc) gimp_param_rgb_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "GimpParamRGB",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_rgb_class_init (GParamSpecClass *class)
{
  class->value_type        = GIMP_TYPE_RGB;
  class->value_set_default = gimp_param_rgb_set_default;
  class->value_validate    = gimp_param_rgb_validate;
  class->values_cmp        = gimp_param_rgb_values_cmp;
}

static void
gimp_param_rgb_init (GParamSpec *pspec)
{
  GimpParamSpecRGB *cspec = GIMP_PARAM_SPEC_RGB (pspec);

  gimp_rgba_set (&cspec->default_value, 0.0, 0.0, 0.0, 0.0);
}

static void
gimp_param_rgb_set_default (GParamSpec *pspec,
                            GValue     *value)
{
  GimpParamSpecRGB *cspec = GIMP_PARAM_SPEC_RGB (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gboolean
gimp_param_rgb_validate (GParamSpec *pspec,
                         GValue     *value)
{
  GimpRGB *rgb;

  rgb = value->data[0].v_pointer;

  if (rgb)
    {
      GimpRGB oval;

      oval = *rgb;

      gimp_rgb_clamp (rgb);

      return (oval.r != rgb->r ||
              oval.g != rgb->g ||
              oval.b != rgb->b ||
              oval.a != rgb->a);
    }

  return FALSE;
}

static gint
gimp_param_rgb_values_cmp (GParamSpec   *pspec,
                           const GValue *value1,
                           const GValue *value2)
{
  GimpRGB *rgb1;
  GimpRGB *rgb2;

  rgb1 = value1->data[0].v_pointer;
  rgb2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! rgb1)
    return rgb2 != NULL ? -1 : 0;
  else if (! rgb2)
    return rgb1 != NULL;
  else
    {
      guint32 int1, int2;

      gimp_rgba_get_uchar (rgb1,
                           ((guchar *) &int1) + 0,
                           ((guchar *) &int1) + 1,
                           ((guchar *) &int1) + 2,
                           ((guchar *) &int1) + 3);
      gimp_rgba_get_uchar (rgb2,
                           ((guchar *) &int2) + 0,
                           ((guchar *) &int2) + 1,
                           ((guchar *) &int2) + 2,
                           ((guchar *) &int2) + 3);

      return int1 - int2;
    }
}

GParamSpec *
gimp_param_spec_rgb (const gchar   *name,
                     const gchar   *nick,
                     const gchar   *blurb,
                     const GimpRGB *default_value,
                     GParamFlags    flags)
{
  GimpParamSpecRGB *cspec;

  g_return_val_if_fail (default_value != NULL, NULL);

  cspec = g_param_spec_internal (GIMP_TYPE_PARAM_RGB,
                                 name, nick, blurb, flags);

  cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


/*
 * GIMP_TYPE_PARAM_MATRIX2
 */

#define GIMP_PARAM_SPEC_MATRIX2(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_MATRIX2, GimpParamSpecMatrix2))

static void   gimp_param_matrix2_class_init  (GParamSpecClass *class);
static void   gimp_param_matrix2_init        (GParamSpec      *pspec);
static void   gimp_param_matrix2_set_default (GParamSpec      *pspec,
                                              GValue          *value);
static gint   gimp_param_matrix2_values_cmp  (GParamSpec      *pspec,
                                              const GValue    *value1,
                                              const GValue    *value2);

typedef struct _GimpParamSpecMatrix2 GimpParamSpecMatrix2;

struct _GimpParamSpecMatrix2
{
  GParamSpecBoxed      parent_instance;

  GimpMatrix2          default_value;
};

GType
gimp_param_matrix2_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_matrix2_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecMatrix2),
        0,
        (GInstanceInitFunc) gimp_param_matrix2_init
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                          "GimpParamMatrix2",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_matrix2_class_init (GParamSpecClass *class)
{
  class->value_type        = GIMP_TYPE_MATRIX2;
  class->value_set_default = gimp_param_matrix2_set_default;
  class->values_cmp        = gimp_param_matrix2_values_cmp;
}

static void
gimp_param_matrix2_init (GParamSpec *pspec)
{
  GimpParamSpecMatrix2 *cspec = GIMP_PARAM_SPEC_MATRIX2 (pspec);

  gimp_matrix2_identity (&cspec->default_value);
}

static void
gimp_param_matrix2_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecMatrix2 *cspec = GIMP_PARAM_SPEC_MATRIX2 (pspec);

  g_value_set_static_boxed (value, &cspec->default_value);
}

static gint
gimp_param_matrix2_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  GimpMatrix2 *matrix1;
  GimpMatrix2 *matrix2;
  gint         i, j;

  matrix1 = value1->data[0].v_pointer;
  matrix2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! matrix1)
    return matrix2 != NULL ? -1 : 0;
  else if (! matrix2)
    return matrix1 != NULL;

  for (i = 0; i < 2; i++)
    for (j = 0; j < 2; j++)
      if (matrix1->coeff[i][j] != matrix2->coeff[i][j])
        return 1;

  return 0;
}

GParamSpec *
gimp_param_spec_matrix2 (const gchar       *name,
                         const gchar       *nick,
                         const gchar       *blurb,
                         const GimpMatrix2 *default_value,
                         GParamFlags        flags)
{
  GimpParamSpecMatrix2 *cspec;

  g_return_val_if_fail (default_value != NULL, NULL);

  cspec = g_param_spec_internal (GIMP_TYPE_PARAM_MATRIX2,
                                 name, nick, blurb, flags);

  cspec->default_value = *default_value;

  return G_PARAM_SPEC (cspec);
}


/*
 * GIMP_TYPE_PARAM_MEMSIZE
 */

static void  gimp_param_memsize_class_init (GParamSpecClass *class);

GType
gimp_param_memsize_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_memsize_class_init,
        NULL, NULL,
        sizeof (GParamSpecUInt64),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_UINT64,
                                          "GimpParamMemsize",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_memsize_class_init (GParamSpecClass *class)
{
  class->value_type = GIMP_TYPE_MEMSIZE;
}

GParamSpec *
gimp_param_spec_memsize (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         guint64      minimum,
                         guint64      maximum,
                         guint64      default_value,
                         GParamFlags  flags)
{
  GParamSpecUInt64 *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_MEMSIZE,
                                 name, nick, blurb, flags);

  pspec->minimum       = minimum;
  pspec->maximum       = maximum;
  pspec->default_value = default_value;

  return G_PARAM_SPEC (pspec);
}


/*
 * GIMP_TYPE_PARAM_PATH
 */

#define GIMP_PARAM_SPEC_PATH(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PATH, GimpParamSpecPath))

typedef struct _GimpParamSpecPath GimpParamSpecPath;

struct _GimpParamSpecPath
{
  GParamSpecString   parent_instance;

  GimpParamPathType  type;
};

static void  gimp_param_path_class_init (GParamSpecClass *class);

GType
gimp_param_path_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_path_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecPath),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_STRING,
                                          "GimpParamPath",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_path_class_init (GParamSpecClass *class)
{
  class->value_type = GIMP_TYPE_PATH;
}

GParamSpec *
gimp_param_spec_path (const gchar        *name,
                      const gchar        *nick,
                      const gchar        *blurb,
		      GimpParamPathType   type,
                      gchar              *default_value,
                      GParamFlags         flags)
{
  GParamSpecString *pspec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_PATH,
                                 name, nick, blurb, flags);


  pspec->default_value = default_value;

  GIMP_PARAM_SPEC_PATH (pspec)->type = type;

  return G_PARAM_SPEC (pspec);
}

GimpParamPathType
gimp_param_spec_path_type (GParamSpec *pspec)
{
  g_return_val_if_fail (GIMP_IS_PARAM_SPEC_PATH (pspec), 0);

  return GIMP_PARAM_SPEC_PATH (pspec)->type;
}


/*
 * GIMP_TYPE_PARAM_UNIT
 */

#define GIMP_PARAM_SPEC_UNIT(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_UNIT, GimpParamSpecUnit))

typedef struct _GimpParamSpecUnit GimpParamSpecUnit;

struct _GimpParamSpecUnit
{
  GParamSpecInt parent_instance;

  gboolean      allow_percent;
};

static void      gimp_param_unit_class_init     (GParamSpecClass *class);
static gboolean  gimp_param_unit_value_validate (GParamSpec      *pspec,
                                                 GValue          *value);

GType
gimp_param_unit_get_type (void)
{
  static GType spec_type = 0;

  if (!spec_type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_unit_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecUnit),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_INT,
                                          "GimpParamUnit",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
gimp_param_unit_class_init (GParamSpecClass *class)
{
  class->value_type     = GIMP_TYPE_UNIT;
  class->value_validate = gimp_param_unit_value_validate;
}

static gboolean
gimp_param_unit_value_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);
  GimpParamSpecUnit *uspec = GIMP_PARAM_SPEC_UNIT (pspec);
  gint               oval  = value->data[0].v_int;

  if (uspec->allow_percent && value->data[0].v_int == GIMP_UNIT_PERCENT)
    {
      value->data[0].v_int = value->data[0].v_int;
    }
  else
    {
      value->data[0].v_int = CLAMP (value->data[0].v_int,
                                    ispec->minimum,
                                    gimp_unit_get_number_of_units () - 1);
    }

  return value->data[0].v_int != oval;
}

GParamSpec *
gimp_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     allow_pixels,
                      gboolean     allow_percent,
                      GimpUnit     default_value,
                      GParamFlags  flags)
{
  GimpParamSpecUnit *pspec;
  GParamSpecInt     *ispec;

  pspec = g_param_spec_internal (GIMP_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  ispec = G_PARAM_SPEC_INT (pspec);

  ispec->default_value = default_value;
  ispec->minimum       = allow_pixels ? GIMP_UNIT_PIXEL : GIMP_UNIT_INCH;
  ispec->maximum       = GIMP_UNIT_PERCENT - 1;

  pspec->allow_percent = allow_percent;

  return G_PARAM_SPEC (pspec);
}
