/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplayermask.h"
#include "gimpparamspecs.h"
#include "gimpselection.h"

#include "vectors/gimpvectors.h"


/*
 * GIMP_TYPE_INT32
 */

GType
gimp_int32_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpInt32", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT32
 */

static void   gimp_param_int32_class_init (GParamSpecClass *klass);
static void   gimp_param_int32_init       (GParamSpec      *pspec);

GType
gimp_param_int32_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int32_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecInt32),
        0,
        (GInstanceInitFunc) gimp_param_int32_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GimpParamInt32", &info, 0);
    }

  return type;
}

static void
gimp_param_int32_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT32;
}

static void
gimp_param_int32_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int32 (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gint         minimum,
                       gint         maximum,
                       gint         default_value,
                       GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (minimum >= G_MININT32, NULL);
  g_return_val_if_fail (maximum <= G_MAXINT32, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_INT32,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_INT16
 */

GType
gimp_int16_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpInt16", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT16
 */

static void   gimp_param_int16_class_init (GParamSpecClass *klass);
static void   gimp_param_int16_init       (GParamSpec      *pspec);

GType
gimp_param_int16_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int16_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecInt16),
        0,
        (GInstanceInitFunc) gimp_param_int16_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GimpParamInt16", &info, 0);
    }

  return type;
}

static void
gimp_param_int16_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT16;
}

static void
gimp_param_int16_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int16 (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gint         minimum,
                       gint         maximum,
                       gint         default_value,
                       GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (minimum >= G_MININT16, NULL);
  g_return_val_if_fail (maximum <= G_MAXINT16, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_INT16,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_INT8
 */

GType
gimp_int8_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_UINT, "GimpInt8", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT8
 */

static void   gimp_param_int8_class_init (GParamSpecClass *klass);
static void   gimp_param_int8_init       (GParamSpec      *pspec);

GType
gimp_param_int8_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int8_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecInt8),
        0,
        (GInstanceInitFunc) gimp_param_int8_init
      };

      type = g_type_register_static (G_TYPE_PARAM_UINT,
                                     "GimpParamInt8", &info, 0);
    }

  return type;
}

static void
gimp_param_int8_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT8;
}

static void
gimp_param_int8_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int8 (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      guint        minimum,
                      guint        maximum,
                      guint        default_value,
                      GParamFlags  flags)
{
  GParamSpecInt *ispec;

  g_return_val_if_fail (maximum <= G_MAXUINT8, NULL);
  g_return_val_if_fail (default_value >= minimum &&
                        default_value <= maximum, NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_INT8,
                                 name, nick, blurb, flags);

  ispec->minimum       = minimum;
  ispec->maximum       = maximum;
  ispec->default_value = default_value;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_STRING
 */

static void       gimp_param_string_class_init (GParamSpecClass *klass);
static void       gimp_param_string_init       (GParamSpec      *pspec);
static gboolean   gimp_param_string_validate   (GParamSpec      *pspec,
                                                GValue          *value);

static GParamSpecClass * gimp_param_string_parent_class = NULL;

GType
gimp_param_string_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_string_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecString),
        0,
        (GInstanceInitFunc) gimp_param_string_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "GimpParamString", &info, 0);
    }

  return type;
}

static void
gimp_param_string_class_init (GParamSpecClass *klass)
{
  gimp_param_string_parent_class = g_type_class_peek_parent (klass);

  klass->value_type     = G_TYPE_STRING;
  klass->value_validate = gimp_param_string_validate;
}

static void
gimp_param_string_init (GParamSpec *pspec)
{
  GimpParamSpecString *sspec = GIMP_PARAM_SPEC_STRING (pspec);

  G_PARAM_SPEC_STRING (pspec)->ensure_non_null = TRUE;

  sspec->allow_non_utf8 = FALSE;
  sspec->non_empty      = FALSE;
}

static gboolean
gimp_param_string_validate (GParamSpec *pspec,
                            GValue     *value)
{
  GimpParamSpecString *sspec  = GIMP_PARAM_SPEC_STRING (pspec);
  gchar               *string = value->data[0].v_pointer;

  if (gimp_param_string_parent_class->value_validate (pspec, value))
    return TRUE;

  if (string)
    {
      gchar *s;

      if (sspec->non_empty && ! string[0])
        {
          if (!(value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS))
            g_free (string);
          else
            value->data[1].v_uint &= ~G_VALUE_NOCOPY_CONTENTS;

          value->data[0].v_pointer = g_strdup ("none");
          return TRUE;
        }

      if (! sspec->allow_non_utf8 &&
          ! g_utf8_validate (string, -1, (const gchar **) &s))
        {
          if (value->data[1].v_uint & G_VALUE_NOCOPY_CONTENTS)
            {
              value->data[0].v_pointer = g_strdup (string);
              value->data[1].v_uint &= ~G_VALUE_NOCOPY_CONTENTS;
              string = value->data[0].v_pointer;
            }

          for (s = string; *s; s++)
            if (*s < ' ')
              *s = '?';

          return TRUE;
        }
    }
  else if (sspec->non_empty)
    {
      value->data[1].v_uint &= ~G_VALUE_NOCOPY_CONTENTS;
      value->data[0].v_pointer = g_strdup ("none");
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_string (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        gboolean     allow_non_utf8,
                        gboolean     null_ok,
                        gboolean     non_empty,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  GimpParamSpecString *sspec;

  g_return_val_if_fail (! (null_ok && non_empty), NULL);

  sspec = g_param_spec_internal (GIMP_TYPE_PARAM_STRING,
                                 name, nick, blurb, flags);

  if (sspec)
    {
      g_free (G_PARAM_SPEC_STRING (sspec)->default_value);
      G_PARAM_SPEC_STRING (sspec)->default_value = g_strdup (default_value);

      G_PARAM_SPEC_STRING (sspec)->ensure_non_null = null_ok ? FALSE : TRUE;

      sspec->allow_non_utf8 = allow_non_utf8 ? TRUE : FALSE;
      sspec->non_empty      = non_empty      ? TRUE : FALSE;
    }

  return G_PARAM_SPEC (sspec);
}


/*
 * GIMP_TYPE_PARAM_ENUM
 */

static void       gimp_param_enum_class_init (GParamSpecClass *klass);
static void       gimp_param_enum_init       (GParamSpec      *pspec);
static void       gimp_param_enum_finalize   (GParamSpec      *pspec);
static gboolean   gimp_param_enum_validate   (GParamSpec      *pspec,
                                              GValue          *value);

GType
gimp_param_enum_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_enum_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecEnum),
        0,
        (GInstanceInitFunc) gimp_param_enum_init
      };

      type = g_type_register_static (G_TYPE_PARAM_ENUM,
                                     "GimpParamEnum", &info, 0);
    }

  return type;
}

static void
gimp_param_enum_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_ENUM;
  klass->finalize       = gimp_param_enum_finalize;
  klass->value_validate = gimp_param_enum_validate;
}

static void
gimp_param_enum_init (GParamSpec *pspec)
{
  GimpParamSpecEnum *espec = GIMP_PARAM_SPEC_ENUM (pspec);

  espec->excluded_values = NULL;
}

static void
gimp_param_enum_finalize (GParamSpec *pspec)
{
  GimpParamSpecEnum *espec = GIMP_PARAM_SPEC_ENUM (pspec);
  GParamSpecClass   *parent_class;

  parent_class = g_type_class_peek (g_type_parent (GIMP_TYPE_PARAM_ENUM));

  g_slist_free (espec->excluded_values);

  parent_class->finalize (pspec);
}

static gboolean
gimp_param_enum_validate (GParamSpec *pspec,
                          GValue     *value)
{
  GimpParamSpecEnum *espec  = GIMP_PARAM_SPEC_ENUM (pspec);
  GParamSpecClass   *parent_class;
  GSList            *list;

  parent_class = g_type_class_peek (g_type_parent (GIMP_TYPE_PARAM_ENUM));

  if (parent_class->value_validate (pspec, value))
    return TRUE;

  for (list = espec->excluded_values; list; list = g_slist_next (list))
    {
      if (GPOINTER_TO_INT (list->data) == value->data[0].v_long)
        {
          value->data[0].v_long = G_PARAM_SPEC_ENUM (pspec)->default_value;
          return TRUE;
        }
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_enum (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      GType        enum_type,
                      gint         default_value,
                      GParamFlags  flags)
{
  GimpParamSpecEnum *espec;
  GEnumClass        *enum_class;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  g_return_val_if_fail (g_enum_get_value (enum_class, default_value) != NULL,
                        NULL);

  espec = g_param_spec_internal (GIMP_TYPE_PARAM_ENUM,
                                 name, nick, blurb, flags);

  G_PARAM_SPEC_ENUM (espec)->enum_class    = enum_class;
  G_PARAM_SPEC_ENUM (espec)->default_value = default_value;
  G_PARAM_SPEC (espec)->value_type         = enum_type;

  return G_PARAM_SPEC (espec);
}

void
gimp_param_spec_enum_exclude_value (GimpParamSpecEnum *espec,
                                    gint               value)
{
  g_return_if_fail (GIMP_IS_PARAM_SPEC_ENUM (espec));
  g_return_if_fail (g_enum_get_value (G_PARAM_SPEC_ENUM (espec)->enum_class,
                                      value) != NULL);

  espec->excluded_values = g_slist_prepend (espec->excluded_values,
                                            GINT_TO_POINTER (value));
}


/*
 * GIMP_TYPE_IMAGE_ID
 */

GType
gimp_image_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpImageID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_IMAGE_ID
 */

static void       gimp_param_image_id_class_init  (GParamSpecClass *klass);
static void       gimp_param_image_id_init        (GParamSpec      *pspec);
static void       gimp_param_image_id_set_default (GParamSpec      *pspec,
                                                   GValue          *value);
static gboolean   gimp_param_image_id_validate    (GParamSpec      *pspec,
                                                   GValue          *value);
static gint       gimp_param_image_id_values_cmp  (GParamSpec      *pspec,
                                                   const GValue    *value1,
                                                   const GValue    *value2);

GType
gimp_param_image_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_image_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecImageID),
        0,
        (GInstanceInitFunc) gimp_param_image_id_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GimpParamImageID", &info, 0);
    }

  return type;
}

static void
gimp_param_image_id_class_init (GParamSpecClass *klass)
{
  klass->value_type        = GIMP_TYPE_IMAGE_ID;
  klass->value_set_default = gimp_param_image_id_set_default;
  klass->value_validate    = gimp_param_image_id_validate;
  klass->values_cmp        = gimp_param_image_id_values_cmp;
}

static void
gimp_param_image_id_init (GParamSpec *pspec)
{
  GimpParamSpecImageID *ispec = GIMP_PARAM_SPEC_IMAGE_ID (pspec);

  ispec->gimp    = NULL;
  ispec->none_ok = FALSE;
}

static void
gimp_param_image_id_set_default (GParamSpec *pspec,
                                 GValue     *value)
{
  value->data[0].v_int = -1;
}

static gboolean
gimp_param_image_id_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecImageID *ispec    = GIMP_PARAM_SPEC_IMAGE_ID (pspec);
  gint                  image_id = value->data[0].v_int;
  GimpImage            *image;

  if (ispec->none_ok && (image_id == 0 || image_id == -1))
    return FALSE;

  image = gimp_image_get_by_ID (ispec->gimp, image_id);

  if (! GIMP_IS_IMAGE (image))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

static gint
gimp_param_image_id_values_cmp (GParamSpec   *pspec,
                                const GValue *value1,
                                const GValue *value2)
{
  gint image_id1 = value1->data[0].v_int;
  gint image_id2 = value2->data[0].v_int;

  /*  try to return at least *something*, it's useless anyway...  */

  if (image_id1 < image_id2)
    return -1;
  else if (image_id1 > image_id2)
    return 1;
  else
    return 0;
}

GParamSpec *
gimp_param_spec_image_id (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          Gimp        *gimp,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecImageID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_IMAGE_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpImage *
gimp_value_get_image (const GValue *value,
                      Gimp         *gimp)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_IMAGE_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp_image_get_by_ID (gimp, value->data[0].v_int);
}

void
gimp_value_set_image (GValue    *value,
                      GimpImage *image)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_IMAGE_ID (value));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  value->data[0].v_int = image ? gimp_image_get_ID (image) : -1;
}


/*
 * GIMP_TYPE_ITEM_ID
 */

GType
gimp_item_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpItemID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_ITEM_ID
 */

static void       gimp_param_item_id_class_init  (GParamSpecClass *klass);
static void       gimp_param_item_id_init        (GParamSpec      *pspec);
static void       gimp_param_item_id_set_default (GParamSpec      *pspec,
                                                  GValue          *value);
static gboolean   gimp_param_item_id_validate    (GParamSpec      *pspec,
                                                  GValue          *value);
static gint       gimp_param_item_id_values_cmp  (GParamSpec      *pspec,
                                                  const GValue    *value1,
                                                  const GValue    *value2);

GType
gimp_param_item_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_item_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecItemID),
        0,
        (GInstanceInitFunc) gimp_param_item_id_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GimpParamItemID", &info, 0);
    }

  return type;
}

static void
gimp_param_item_id_class_init (GParamSpecClass *klass)
{
  klass->value_type        = GIMP_TYPE_ITEM_ID;
  klass->value_set_default = gimp_param_item_id_set_default;
  klass->value_validate    = gimp_param_item_id_validate;
  klass->values_cmp        = gimp_param_item_id_values_cmp;
}

static void
gimp_param_item_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->gimp      = NULL;
  ispec->item_type = GIMP_TYPE_ITEM;
  ispec->none_ok   = FALSE;
}

static void
gimp_param_item_id_set_default (GParamSpec *pspec,
                                GValue     *value)
{
  value->data[0].v_int = -1;
}

static gboolean
gimp_param_item_id_validate (GParamSpec *pspec,
                             GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;
  GimpItem            *item;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  item = gimp_item_get_by_ID (ispec->gimp, item_id);

  if (! item || ! g_type_is_a (G_TYPE_FROM_INSTANCE (item), ispec->item_type))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }
  else if (gimp_item_is_removed (item))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

static gint
gimp_param_item_id_values_cmp (GParamSpec   *pspec,
                               const GValue *value1,
                               const GValue *value2)
{
  gint item_id1 = value1->data[0].v_int;
  gint item_id2 = value2->data[0].v_int;

  /*  try to return at least *something*, it's useless anyway...  */

  if (item_id1 < item_id2)
    return -1;
  else if (item_id1 > item_id2)
    return 1;
  else
    return 0;
}

GParamSpec *
gimp_param_spec_item_id (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         Gimp        *gimp,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_ITEM_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok;

  return G_PARAM_SPEC (ispec);
}

GimpItem *
gimp_value_get_item (const GValue *value,
                     Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_ITEM_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_ITEM (item))
    return NULL;

  return item;
}

void
gimp_value_set_item (GValue   *value,
                     GimpItem *item)
{
  g_return_if_fail (item == NULL || GIMP_IS_ITEM (item));

  /* FIXME remove hack as soon as bug #375864 is fixed */

  if (GIMP_VALUE_HOLDS_ITEM_ID (value))
    {
      value->data[0].v_int = item ? gimp_item_get_ID (item) : -1;
    }
  else if (GIMP_VALUE_HOLDS_DRAWABLE_ID (value) &&
           (item == NULL || GIMP_IS_DRAWABLE (item)))
    {
      gimp_value_set_drawable (value, GIMP_DRAWABLE (item));
    }
  else if (GIMP_VALUE_HOLDS_LAYER_ID (value) &&
           (item == NULL || GIMP_IS_LAYER (item)))
    {
      gimp_value_set_layer (value, GIMP_LAYER (item));
    }
  else if (GIMP_VALUE_HOLDS_CHANNEL_ID (value) &&
           (item == NULL || GIMP_IS_CHANNEL (item)))
    {
      gimp_value_set_channel (value, GIMP_CHANNEL (item));
    }
  else if (GIMP_VALUE_HOLDS_LAYER_MASK_ID (value) &&
           (item == NULL || GIMP_IS_LAYER_MASK (item)))
    {
      gimp_value_set_layer_mask (value, GIMP_LAYER_MASK (item));
    }
  else if (GIMP_VALUE_HOLDS_SELECTION_ID (value) &&
           (item == NULL || GIMP_IS_SELECTION (item)))
    {
      gimp_value_set_selection (value, GIMP_SELECTION (item));
    }
  else if (GIMP_VALUE_HOLDS_VECTORS_ID (value) &&
           (item == NULL || GIMP_IS_VECTORS (item)))
    {
      gimp_value_set_vectors (value, GIMP_VECTORS (item));
    }
  else
    {
      g_return_if_reached ();
    }
}


/*
 * GIMP_TYPE_DRAWABLE_ID
 */

GType
gimp_drawable_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpDrawableID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_DRAWABLE_ID
 */

static void   gimp_param_drawable_id_class_init (GParamSpecClass *klass);
static void   gimp_param_drawable_id_init       (GParamSpec      *pspec);

GType
gimp_param_drawable_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_drawable_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecDrawableID),
        0,
        (GInstanceInitFunc) gimp_param_drawable_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ITEM_ID,
                                     "GimpParamDrawableID", &info, 0);
    }

  return type;
}

static void
gimp_param_drawable_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_DRAWABLE_ID;
}

static void
gimp_param_drawable_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_DRAWABLE;
}

GParamSpec *
gimp_param_spec_drawable_id (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             Gimp        *gimp,
                             gboolean     none_ok,
                             GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DRAWABLE_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpDrawable *
gimp_value_get_drawable (const GValue *value,
                         Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_DRAWABLE_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_DRAWABLE (item))
    return NULL;

  return GIMP_DRAWABLE (item);
}

void
gimp_value_set_drawable (GValue       *value,
                         GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DRAWABLE_ID (value));
  g_return_if_fail (drawable == NULL || GIMP_IS_DRAWABLE (drawable));

  value->data[0].v_int = drawable ? gimp_item_get_ID (GIMP_ITEM (drawable)) : -1;
}


/*
 * GIMP_TYPE_LAYER_ID
 */

GType
gimp_layer_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpLayerID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_LAYER_ID
 */

static void   gimp_param_layer_id_class_init (GParamSpecClass *klass);
static void   gimp_param_layer_id_init       (GParamSpec      *pspec);

GType
gimp_param_layer_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_layer_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecLayerID),
        0,
        (GInstanceInitFunc) gimp_param_layer_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_DRAWABLE_ID,
                                     "GimpParamLayerID", &info, 0);
    }

  return type;
}

static void
gimp_param_layer_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_LAYER_ID;
}

static void
gimp_param_layer_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_LAYER;
}

GParamSpec *
gimp_param_spec_layer_id (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          Gimp        *gimp,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpLayer *
gimp_value_get_layer (const GValue *value,
                      Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_LAYER_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_LAYER (item))
    return NULL;

  return GIMP_LAYER (item);
}

void
gimp_value_set_layer (GValue    *value,
                      GimpLayer *layer)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_LAYER_ID (value));
  g_return_if_fail (layer == NULL || GIMP_IS_LAYER (layer));

  value->data[0].v_int = layer ? gimp_item_get_ID (GIMP_ITEM (layer)) : -1;
}


/*
 * GIMP_TYPE_CHANNEL_ID
 */

GType
gimp_channel_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpChannelID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_CHANNEL_ID
 */

static void   gimp_param_channel_id_class_init (GParamSpecClass *klass);
static void   gimp_param_channel_id_init       (GParamSpec      *pspec);

GType
gimp_param_channel_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_channel_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecChannelID),
        0,
        (GInstanceInitFunc) gimp_param_channel_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_DRAWABLE_ID,
                                     "GimpParamChannelID", &info, 0);
    }

  return type;
}

static void
gimp_param_channel_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_CHANNEL_ID;
}

static void
gimp_param_channel_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_CHANNEL;
}

GParamSpec *
gimp_param_spec_channel_id (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            Gimp        *gimp,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_CHANNEL_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpChannel *
gimp_value_get_channel (const GValue *value,
                        Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_CHANNEL_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_CHANNEL (item))
    return NULL;

  return GIMP_CHANNEL (item);
}

void
gimp_value_set_channel (GValue      *value,
                        GimpChannel *channel)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_CHANNEL_ID (value));
  g_return_if_fail (channel == NULL || GIMP_IS_CHANNEL (channel));

  value->data[0].v_int = channel ? gimp_item_get_ID (GIMP_ITEM (channel)) : -1;
}


/*
 * GIMP_TYPE_LAYER_MASK_ID
 */

GType
gimp_layer_mask_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpLayerMaskID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_LAYER_MASK_ID
 */

static void   gimp_param_layer_mask_id_class_init (GParamSpecClass *klass);
static void   gimp_param_layer_mask_id_init       (GParamSpec      *pspec);

GType
gimp_param_layer_mask_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_layer_mask_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecLayerMaskID),
        0,
        (GInstanceInitFunc) gimp_param_layer_mask_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_CHANNEL_ID,
                                     "GimpParamLayerMaskID", &info, 0);
    }

  return type;
}

static void
gimp_param_layer_mask_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_LAYER_MASK_ID;
}

static void
gimp_param_layer_mask_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_LAYER_MASK;
}

GParamSpec *
gimp_param_spec_layer_mask_id (const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               Gimp        *gimp,
                               gboolean     none_ok,
                               GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER_MASK_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpLayerMask *
gimp_value_get_layer_mask (const GValue *value,
                           Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_LAYER_MASK_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_LAYER_MASK (item))
    return NULL;

  return GIMP_LAYER_MASK (item);
}

void
gimp_value_set_layer_mask (GValue        *value,
                           GimpLayerMask *layer_mask)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_LAYER_MASK_ID (value));
  g_return_if_fail (layer_mask == NULL || GIMP_IS_LAYER_MASK (layer_mask));

  value->data[0].v_int = layer_mask ? gimp_item_get_ID (GIMP_ITEM (layer_mask)) : -1;
}


/*
 * GIMP_TYPE_SELECTION_ID
 */

GType
gimp_selection_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpSelectionID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_SELECTION_ID
 */

static void   gimp_param_selection_id_class_init (GParamSpecClass *klass);
static void   gimp_param_selection_id_init       (GParamSpec      *pspec);

GType
gimp_param_selection_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_selection_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecSelectionID),
        0,
        (GInstanceInitFunc) gimp_param_selection_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_CHANNEL_ID,
                                     "GimpParamSelectionID", &info, 0);
    }

  return type;
}

static void
gimp_param_selection_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_SELECTION_ID;
}

static void
gimp_param_selection_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_SELECTION;
}

GParamSpec *
gimp_param_spec_selection_id (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              Gimp        *gimp,
                              gboolean     none_ok,
                              GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_SELECTION_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpSelection *
gimp_value_get_selection (const GValue *value,
                          Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_SELECTION_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_SELECTION (item))
    return NULL;

  return GIMP_SELECTION (item);
}

void
gimp_value_set_selection (GValue        *value,
                          GimpSelection *selection)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_SELECTION_ID (value));
  g_return_if_fail (selection == NULL || GIMP_IS_SELECTION (selection));

  value->data[0].v_int = selection ? gimp_item_get_ID (GIMP_ITEM (selection)) : -1;
}


/*
 * GIMP_TYPE_VECTORS_ID
 */

GType
gimp_vectors_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpVectorsID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_VECTORS_ID
 */

static void   gimp_param_vectors_id_class_init (GParamSpecClass *klass);
static void   gimp_param_vectors_id_init       (GParamSpec      *pspec);

GType
gimp_param_vectors_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_vectors_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecVectorsID),
        0,
        (GInstanceInitFunc) gimp_param_vectors_id_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ITEM_ID,
                                     "GimpParamVectorsID", &info, 0);
    }

  return type;
}

static void
gimp_param_vectors_id_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_VECTORS_ID;
}

static void
gimp_param_vectors_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->item_type = GIMP_TYPE_VECTORS;
}

GParamSpec *
gimp_param_spec_vectors_id (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            Gimp        *gimp,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_VECTORS_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpVectors *
gimp_value_get_vectors (const GValue *value,
                        Gimp         *gimp)
{
  GimpItem *item;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_VECTORS_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! GIMP_IS_VECTORS (item))
    return NULL;

  return GIMP_VECTORS (item);
}

void
gimp_value_set_vectors (GValue      *value,
                        GimpVectors *vectors)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_VECTORS_ID (value));
  g_return_if_fail (vectors == NULL || GIMP_IS_VECTORS (vectors));

  value->data[0].v_int = vectors ? gimp_item_get_ID (GIMP_ITEM (vectors)) : -1;
}


/*
 * GIMP_TYPE_DISPLAY_ID
 */

GType
gimp_display_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info = { 0, };

      type = g_type_register_static (G_TYPE_INT, "GimpDisplayID", &info, 0);
    }

  return type;
}


/*
 * GIMP_TYPE_PARAM_DISPLAY_ID
 */

static void       gimp_param_display_id_class_init  (GParamSpecClass *klass);
static void       gimp_param_display_id_init        (GParamSpec      *pspec);
static void       gimp_param_display_id_set_default (GParamSpec      *pspec,
                                                     GValue          *value);
static gboolean   gimp_param_display_id_validate    (GParamSpec      *pspec,
                                                     GValue          *value);
static gint       gimp_param_display_id_values_cmp  (GParamSpec      *pspec,
                                                     const GValue    *value1,
                                                     const GValue    *value2);

GType
gimp_param_display_id_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_display_id_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecDisplayID),
        0,
        (GInstanceInitFunc) gimp_param_display_id_init
      };

      type = g_type_register_static (G_TYPE_PARAM_INT,
                                     "GimpParamDisplayID", &info, 0);
    }

  return type;
}

static void
gimp_param_display_id_class_init (GParamSpecClass *klass)
{
  klass->value_type        = GIMP_TYPE_DISPLAY_ID;
  klass->value_set_default = gimp_param_display_id_set_default;
  klass->value_validate    = gimp_param_display_id_validate;
  klass->values_cmp        = gimp_param_display_id_values_cmp;
}

static void
gimp_param_display_id_init (GParamSpec *pspec)
{
  GimpParamSpecDisplayID *ispec = GIMP_PARAM_SPEC_DISPLAY_ID (pspec);

  ispec->gimp    = NULL;
  ispec->none_ok = FALSE;
}

static void
gimp_param_display_id_set_default (GParamSpec *pspec,
                                   GValue     *value)
{
  value->data[0].v_int = -1;
}

static gboolean
gimp_param_display_id_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecDisplayID *ispec      = GIMP_PARAM_SPEC_DISPLAY_ID (pspec);
  gint                    display_id = value->data[0].v_int;
  GimpObject             *display;

  if (ispec->none_ok && (display_id == 0 || display_id == -1))
    return FALSE;

  display = gimp_get_display_by_ID (ispec->gimp, display_id);

  if (! GIMP_IS_OBJECT (display))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

static gint
gimp_param_display_id_values_cmp (GParamSpec   *pspec,
                                  const GValue *value1,
                                  const GValue *value2)
{
  gint display_id1 = value1->data[0].v_int;
  gint display_id2 = value2->data[0].v_int;

  /*  try to return at least *something*, it's useless anyway...  */

  if (display_id1 < display_id2)
    return -1;
  else if (display_id1 > display_id2)
    return 1;
  else
    return 0;
}

GParamSpec *
gimp_param_spec_display_id (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            Gimp        *gimp,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecDisplayID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DISPLAY_ID,
                                 name, nick, blurb, flags);

  ispec->gimp    = gimp;
  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

GimpObject *
gimp_value_get_display (const GValue *value,
                        Gimp         *gimp)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_DISPLAY_ID (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp_get_display_by_ID (gimp, value->data[0].v_int);
}

void
gimp_value_set_display (GValue     *value,
                        GimpObject *display)
{
  gint id = -1;

  g_return_if_fail (GIMP_VALUE_HOLDS_DISPLAY_ID (value));
  g_return_if_fail (display == NULL || GIMP_IS_OBJECT (display));

  if (display)
    g_object_get (display, "id", &id, NULL);

  value->data[0].v_int = id;
}


/*
 * GIMP_TYPE_ARRAY
 */

GimpArray *
gimp_array_new (const guint8 *data,
                gsize         length,
                gboolean      static_data)
{
  GimpArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (GimpArray);

  array->data        = static_data ? (guint8 *) data : g_memdup (data, length);
  array->length      = length;
  array->static_data = static_data;

  return array;
}

GimpArray *
gimp_array_copy (const GimpArray *array)
{
  if (array)
    return gimp_array_new (array->data, array->length, FALSE);

  return NULL;
}

void
gimp_array_free (GimpArray *array)
{
  if (array)
    {
      if (! array->static_data)
        g_free (array->data);

      g_slice_free (GimpArray, array);
    }
}

GType
gimp_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_ARRAY
 */

static void       gimp_param_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_array_validate    (GParamSpec      *pspec,
                                                GValue          *value);
static gint       gimp_param_array_values_cmp  (GParamSpec      *pspec,
                                                const GValue    *value1,
                                                const GValue    *value2);

GType
gimp_param_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamArray", &info, 0);
    }

  return type;
}

static void
gimp_param_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_ARRAY;
  klass->value_validate = gimp_param_array_validate;
  klass->values_cmp     = gimp_param_array_values_cmp;
}

static void
gimp_param_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_array_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    {
      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

static gint
gimp_param_array_values_cmp (GParamSpec   *pspec,
                             const GValue *value1,
                             const GValue *value2)
{
  GimpArray *array1 = value1->data[0].v_pointer;
  GimpArray *array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (array1->length < array2->length)
    return -1;
  else if (array1->length > array2->length)
    return 1;

  return 0;
}

GParamSpec *
gimp_param_spec_array (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

static const guint8 *
gimp_value_get_array (const GValue *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    return array->data;

  return NULL;
}

static guint8 *
gimp_value_dup_array (const GValue *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    return g_memdup (array->data, array->length);

  return NULL;
}

static void
gimp_value_set_array (GValue       *value,
                      const guint8 *data,
                      gsize         length)
{
  GimpArray *array = gimp_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

static void
gimp_value_set_static_array (GValue       *value,
                             const guint8 *data,
                             gsize         length)
{
  GimpArray *array = gimp_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

static void
gimp_value_take_array (GValue *value,
                       guint8 *data,
                       gsize   length)
{
  GimpArray *array = gimp_array_new (data, length, TRUE);

  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * GIMP_TYPE_INT8_ARRAY
 */

GType
gimp_int8_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpInt8Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT8_ARRAY
 */

static void   gimp_param_int8_array_class_init (GParamSpecClass *klass);
static void   gimp_param_int8_array_init       (GParamSpec      *pspec);

GType
gimp_param_int8_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int8_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_int8_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamInt8Array", &info, 0);
    }

  return type;
}

static void
gimp_param_int8_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT8_ARRAY;
}

static void
gimp_param_int8_array_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int8_array (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT8_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const guint8 *
gimp_value_get_int8array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT8_ARRAY (value), NULL);

  return gimp_value_get_array (value);
}

guint8 *
gimp_value_dup_int8array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT8_ARRAY (value), NULL);

  return gimp_value_dup_array (value);
}

void
gimp_value_set_int8array (GValue       *value,
                          const guint8 *data,
                          gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT8_ARRAY (value));

  gimp_value_set_array (value, data, length);
}

void
gimp_value_set_static_int8array (GValue       *value,
                                 const guint8 *data,
                                 gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT8_ARRAY (value));

  gimp_value_set_static_array (value, data, length);
}

void
gimp_value_take_int8array (GValue *value,
                           guint8 *data,
                           gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT8_ARRAY (value));

  gimp_value_take_array (value, data, length);
}


/*
 * GIMP_TYPE_INT16_ARRAY
 */

GType
gimp_int16_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpInt16Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT16_ARRAY
 */

static void   gimp_param_int16_array_class_init (GParamSpecClass *klass);
static void   gimp_param_int16_array_init       (GParamSpec      *pspec);

GType
gimp_param_int16_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int16_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_int16_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamInt16Array", &info, 0);
    }

  return type;
}

static void
gimp_param_int16_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT16_ARRAY;
}

static void
gimp_param_int16_array_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int16_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT16_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gint16 *
gimp_value_get_int16array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value), NULL);

  return (const gint16 *) gimp_value_get_array (value);
}

gint16 *
gimp_value_dup_int16array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value), NULL);

  return (gint16 *) gimp_value_dup_array (value);
}

void
gimp_value_set_int16array (GValue       *value,
                           const gint16 *data,
                           gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint16));
}

void
gimp_value_set_static_int16array (GValue       *value,
                                  const gint16 *data,
                                  gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint16));
}

void
gimp_value_take_int16array (GValue *value,
                            gint16 *data,
                            gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT16_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint16));
}


/*
 * GIMP_TYPE_INT32_ARRAY
 */

GType
gimp_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpInt32Array",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_INT32_ARRAY
 */

static void   gimp_param_int32_array_class_init (GParamSpecClass *klass);
static void   gimp_param_int32_array_init       (GParamSpec      *pspec);

GType
gimp_param_int32_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_int32_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_int32_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamInt32Array", &info, 0);
    }

  return type;
}

static void
gimp_param_int32_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_INT32_ARRAY;
}

static void
gimp_param_int32_array_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_int32_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_INT32_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gint32 *
gimp_value_get_int32array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (const gint32 *) gimp_value_get_array (value);
}

gint32 *
gimp_value_dup_int32array (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value), NULL);

  return (gint32 *) gimp_value_dup_array (value);
}

void
gimp_value_set_int32array (GValue       *value,
                           const gint32 *data,
                           gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gint32));
}

void
gimp_value_set_static_int32array (GValue       *value,
                                  const gint32 *data,
                                  gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gint32));
}

void
gimp_value_take_int32array (GValue *value,
                            gint32 *data,
                            gsize   length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_INT32_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gint32));
}


/*
 * GIMP_TYPE_FLOAT_ARRAY
 */

GType
gimp_float_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpFloatArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_FLOAT_ARRAY
 */

static void   gimp_param_float_array_class_init (GParamSpecClass *klass);
static void   gimp_param_float_array_init       (GParamSpec      *pspec);

GType
gimp_param_float_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_float_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_float_array_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ARRAY,
                                     "GimpParamFloatArray", &info, 0);
    }

  return type;
}

static void
gimp_param_float_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_FLOAT_ARRAY;
}

static void
gimp_param_float_array_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_float_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_FLOAT_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gdouble *
gimp_value_get_floatarray (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (const gdouble *) gimp_value_get_array (value);
}

gdouble *
gimp_value_dup_floatarray (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value), NULL);

  return (gdouble *) gimp_value_dup_array (value);
}

void
gimp_value_set_floatarray (GValue        *value,
                           const gdouble *data,
                           gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (gdouble));
}

void
gimp_value_set_static_floatarray (GValue        *value,
                                  const gdouble *data,
                                  gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (gdouble));
}

void
gimp_value_take_floatarray (GValue  *value,
                            gdouble *data,
                            gsize    length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_FLOAT_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (gdouble));
}


/*
 * GIMP_TYPE_STRING_ARRAY
 */

GimpArray *
gimp_string_array_new (const gchar **data,
                       gsize         length,
                       gboolean      static_data)
{
  GimpArray *array;

  g_return_val_if_fail ((data == NULL && length == 0) ||
                        (data != NULL && length  > 0), NULL);

  array = g_slice_new0 (GimpArray);

  if (! static_data)
    {
      gchar **tmp = g_new (gchar *, length);
      gint    i;

      for (i = 0; i < length; i++)
        tmp[i] = g_strdup (data[i]);

      array->data = (guint8 *) tmp;
    }
  else
    {
      array->data = (guint8 *) data;
    }

  array->length      = length;
  array->static_data = static_data;

  return array;
}

GimpArray *
gimp_string_array_copy (const GimpArray *array)
{
  if (array)
    return gimp_string_array_new ((const gchar **) array->data,
                                  array->length, FALSE);

  return NULL;
}

void
gimp_string_array_free (GimpArray *array)
{
  if (array)
    {
      if (! array->static_data)
        {
          gchar **tmp = (gchar **) array->data;
          gint    i;

          for (i = 0; i < array->length; i++)
            g_free (tmp[i]);

          g_free (array->data);
        }

      g_slice_free (GimpArray, array);
    }
}

GType
gimp_string_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpStringArray",
                                         (GBoxedCopyFunc) gimp_string_array_copy,
                                         (GBoxedFreeFunc) gimp_string_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_STRING_ARRAY
 */

static void       gimp_param_string_array_class_init  (GParamSpecClass *klass);
static void       gimp_param_string_array_init        (GParamSpec      *pspec);
static gboolean   gimp_param_string_array_validate    (GParamSpec      *pspec,
                                                       GValue          *value);
static gint       gimp_param_string_array_values_cmp  (GParamSpec      *pspec,
                                                       const GValue    *value1,
                                                       const GValue    *value2);

GType
gimp_param_string_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_string_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_string_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamStringArray", &info, 0);
    }

  return type;
}

static void
gimp_param_string_array_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_STRING_ARRAY;
  klass->value_validate = gimp_param_string_array_validate;
  klass->values_cmp     = gimp_param_string_array_values_cmp;
}

static void
gimp_param_string_array_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_string_array_validate (GParamSpec *pspec,
                                  GValue     *value)
{
  GimpArray *array = value->data[0].v_pointer;

  if (array)
    {
      if ((array->data == NULL && array->length != 0) ||
          (array->data != NULL && array->length == 0))
        {
          g_value_set_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}

static gint
gimp_param_string_array_values_cmp (GParamSpec   *pspec,
                                    const GValue *value1,
                                    const GValue *value2)
{
  GimpArray *array1 = value1->data[0].v_pointer;
  GimpArray *array2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! array1)
    return array2 != NULL ? -1 : 0;
  else if (! array2)
    return array1 != NULL ? 1 : 0;
  else if (array1->length < array2->length)
    return -1;
  else if (array1->length > array2->length)
    return 1;

  return 0;
}

GParamSpec *
gimp_param_spec_string_array (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              GParamFlags  flags)
{
  GimpParamSpecStringArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_STRING_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const gchar **
gimp_value_get_stringarray (const GValue *value)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    return (const gchar **) array->data;

  return NULL;
}

gchar **
gimp_value_dup_stringarray (const GValue *value)
{
  GimpArray *array;

  g_return_val_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value), NULL);

  array = value->data[0].v_pointer;

  if (array)
    {
      gchar **ret = g_memdup (array->data, array->length * sizeof (gchar *));
      gint    i;

      for (i = 0; i < array->length; i++)
        ret[i] = g_strdup (ret[i]);

      return ret;
    }

  return NULL;
}

void
gimp_value_set_stringarray (GValue       *value,
                            const gchar **data,
                            gsize         length)
{
  GimpArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new (data, length, FALSE);

  g_value_take_boxed (value, array);
}

void
gimp_value_set_static_stringarray (GValue       *value,
                                   const gchar **data,
                                   gsize         length)
{
  GimpArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new (data, length, TRUE);

  g_value_take_boxed (value, array);
}

void
gimp_value_take_stringarray (GValue  *value,
                             gchar  **data,
                             gsize    length)
{
  GimpArray *array;

  g_return_if_fail (GIMP_VALUE_HOLDS_STRING_ARRAY (value));

  array = gimp_string_array_new ((const gchar **) data, length, TRUE);
  array->static_data = FALSE;

  g_value_take_boxed (value, array);
}


/*
 * GIMP_TYPE_COLOR_ARRAY
 */

GType
gimp_color_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpColorArray",
                                         (GBoxedCopyFunc) gimp_array_copy,
                                         (GBoxedFreeFunc) gimp_array_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_COLOR_ARRAY
 */

static void  gimp_param_color_array_class_init (GParamSpecClass *klass);
static void  gimp_param_color_array_init       (GParamSpec      *pspec);

GType
gimp_param_color_array_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_color_array_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecArray),
        0,
        (GInstanceInitFunc) gimp_param_color_array_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamColorArray", &info, 0);
    }

  return type;
}

static void
gimp_param_color_array_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_COLOR_ARRAY;
}

static void
gimp_param_color_array_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_color_array (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             GParamFlags  flags)
{
  GimpParamSpecColorArray *array_spec;

  array_spec = g_param_spec_internal (GIMP_TYPE_PARAM_COLOR_ARRAY,
                                      name, nick, blurb, flags);

  return G_PARAM_SPEC (array_spec);
}

const GimpRGB *
gimp_value_get_colorarray (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_COLOR_ARRAY (value), NULL);

  return (const GimpRGB *) gimp_value_get_array (value);
}

GimpRGB *
gimp_value_dup_colorarray (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_COLOR_ARRAY (value), NULL);

  return (GimpRGB *) gimp_value_dup_array (value);
}

void
gimp_value_set_colorarray (GValue        *value,
                           const GimpRGB *data,
                           gsize         length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_COLOR_ARRAY (value));

  gimp_value_set_array (value, (const guint8 *) data,
                        length * sizeof (GimpRGB));
}

void
gimp_value_set_static_colorarray (GValue        *value,
                                  const GimpRGB *data,
                                  gsize          length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_COLOR_ARRAY (value));

  gimp_value_set_static_array (value, (const guint8 *) data,
                               length * sizeof (GimpRGB));
}

void
gimp_value_take_colorarray (GValue  *value,
                            GimpRGB *data,
                            gsize    length)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_COLOR_ARRAY (value));

  gimp_value_take_array (value, (guint8 *) data,
                         length * sizeof (GimpRGB));
}
