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

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpimage.h"
#include "gimpitem.h"
#include "gimpparamspecs.h"


/*
 * GIMP_TYPE_PARAM_STRING
 */

static void       gimp_param_string_class_init  (GParamSpecClass *class);
static void       gimp_param_string_init        (GParamSpec      *pspec);
static gboolean   gimp_param_string_validate    (GParamSpec      *pspec,
                                                 GValue          *value);

GType
gimp_param_string_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo type_info =
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
                                     "GimpParamString",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_string_class_init (GParamSpecClass *class)
{
  class->value_type     = G_TYPE_STRING;
  class->value_validate = gimp_param_string_validate;
}

static void
gimp_param_string_init (GParamSpec *pspec)
{
  GimpParamSpecString *sspec = GIMP_PARAM_SPEC_STRING (pspec);

  sspec->no_validate = FALSE;
  sspec->null_ok     = FALSE;
}

static gboolean
gimp_param_string_validate (GParamSpec *pspec,
                            GValue     *value)
{
  GimpParamSpecString *sspec  = GIMP_PARAM_SPEC_STRING (pspec);
  gchar               *string = value->data[0].v_pointer;

  if (string)
    {
      gchar *s;

      if (! sspec->no_validate &&
          ! g_utf8_validate (string, -1, (const gchar **) &s))
        {
          for (; *s; s++)
            if (*s < ' ')
              *s = '?';

          return TRUE;
        }
    }
  else if (! sspec->null_ok)
    {
      value->data[0].v_pointer = g_strdup ("");
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_string (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        gboolean     no_validate,
                        gboolean     null_ok,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  GimpParamSpecString *sspec;

  sspec = g_param_spec_internal (GIMP_TYPE_PARAM_STRING,
                                 name, nick, blurb, flags);

  if (sspec)
    {
      g_free (G_PARAM_SPEC_STRING (sspec)->default_value);
      G_PARAM_SPEC_STRING (sspec)->default_value = g_strdup (default_value);

      sspec->no_validate = no_validate ? TRUE : FALSE;
      sspec->null_ok     = null_ok     ? TRUE : FALSE;
    }

  return G_PARAM_SPEC (sspec);
}


/*
 * GIMP_TYPE_PARAM_ENUM
 */

static void       gimp_param_enum_class_init  (GParamSpecClass *class);
static void       gimp_param_enum_init        (GParamSpec      *pspec);
static void       gimp_param_enum_finalize    (GParamSpec      *pspec);
static gboolean   gimp_param_enum_validate    (GParamSpec      *pspec,
                                               GValue          *value);

GType
gimp_param_enum_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo type_info =
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
                                     "GimpParamEnum",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_enum_class_init (GParamSpecClass *class)
{
  class->value_type     = G_TYPE_ENUM;
  class->finalize       = gimp_param_enum_finalize;
  class->value_validate = gimp_param_enum_validate;
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
 * GIMP_TYPE_PARAM_IMAGE_ID
 */

static void       gimp_param_image_id_class_init  (GParamSpecClass *class);
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
      static const GTypeInfo type_info =
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
                                     "GimpParamImageID",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_image_id_class_init (GParamSpecClass *class)
{
  class->value_type        = G_TYPE_INT;
  class->value_set_default = gimp_param_image_id_set_default;
  class->value_validate    = gimp_param_image_id_validate;
  class->values_cmp        = gimp_param_image_id_values_cmp;
}

static void
gimp_param_image_id_init (GParamSpec *pspec)
{
  GimpParamSpecImageID *ispec = GIMP_PARAM_SPEC_IMAGE_ID (pspec);

  ispec->gimp = NULL;
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
  GimpParamSpecImageID *ispec = GIMP_PARAM_SPEC_IMAGE_ID (pspec);
  gint                  image_id;
  GimpImage            *image;

  image_id = value->data[0].v_int;
  image    = gimp_image_get_by_ID (ispec->gimp, image_id);

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
  gint image_id1;
  gint image_id2;

  image_id1 = value1->data[0].v_int;
  image_id2 = value2->data[0].v_int;

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
                          GParamFlags  flags)
{
  GimpParamSpecImageID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_IMAGE_ID,
                                 name, nick, blurb, flags);

  if (ispec)
    ispec->gimp = gimp;

  return G_PARAM_SPEC (ispec);
}

GimpImage *
gimp_value_get_image (const GValue *value,
                      Gimp         *gimp)
{
  g_return_val_if_fail (G_VALUE_HOLDS_INT (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp_image_get_by_ID (gimp, value->data[0].v_int);
}

void
gimp_value_set_image (GValue    *value,
                      GimpImage *image)
{
  g_return_if_fail (G_VALUE_HOLDS_INT (value));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  value->data[0].v_int = image ? gimp_image_get_ID (image) : -1;
}


/*
 * GIMP_TYPE_PARAM_ITEM_ID
 */

static void       gimp_param_item_id_class_init  (GParamSpecClass *class);
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
      static const GTypeInfo type_info =
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
                                     "GimpParamItemID",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_item_id_class_init (GParamSpecClass *class)
{
  class->value_type        = G_TYPE_INT;
  class->value_set_default = gimp_param_item_id_set_default;
  class->value_validate    = gimp_param_item_id_validate;
  class->values_cmp        = gimp_param_item_id_values_cmp;
}

static void
gimp_param_item_id_init (GParamSpec *pspec)
{
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);

  ispec->gimp      = NULL;
  ispec->item_type = G_TYPE_NONE;
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
  GimpParamSpecItemID *ispec = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id;
  GimpItem            *item;

  item_id = value->data[0].v_int;
  item    = gimp_item_get_by_ID (ispec->gimp, item_id);

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
  gint item_id1;
  gint item_id2;

  item_id1 = value1->data[0].v_int;
  item_id2 = value2->data[0].v_int;

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
                         GType        item_type,
                         GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_ITEM_ID,
                                 name, nick, blurb, flags);

  if (ispec)
    {
      ispec->gimp      = gimp;
      ispec->item_type = item_type;
    }

  return G_PARAM_SPEC (ispec);
}

GimpItem *
gimp_value_get_item (const GValue *value,
                     Gimp         *gimp,
                     GType         item_type)
{
  GimpItem *item;

  g_return_val_if_fail (G_VALUE_HOLDS_INT (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (g_type_is_a (item_type, GIMP_TYPE_ITEM), NULL);

  item = gimp_item_get_by_ID (gimp, value->data[0].v_int);

  if (item && ! g_type_is_a (G_TYPE_FROM_INSTANCE (item), item_type))
    return NULL;

  return item;
}

void
gimp_value_set_item (GValue   *value,
                     GimpItem *item)
{
  g_return_if_fail (G_VALUE_HOLDS_INT (value));
  g_return_if_fail (item == NULL || GIMP_IS_ITEM (item));

  value->data[0].v_int = item ? gimp_item_get_ID (item) : -1;
}


/*
 * GIMP_TYPE_PARAM_DISPLAY_ID
 */

static void       gimp_param_display_id_class_init  (GParamSpecClass *class);
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
      static const GTypeInfo type_info =
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
                                     "GimpParamDisplayID",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_display_id_class_init (GParamSpecClass *class)
{
  class->value_type        = G_TYPE_INT;
  class->value_set_default = gimp_param_display_id_set_default;
  class->value_validate    = gimp_param_display_id_validate;
  class->values_cmp        = gimp_param_display_id_values_cmp;
}

static void
gimp_param_display_id_init (GParamSpec *pspec)
{
  GimpParamSpecDisplayID *ispec = GIMP_PARAM_SPEC_DISPLAY_ID (pspec);

  ispec->gimp = NULL;
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
  GimpParamSpecDisplayID *ispec = GIMP_PARAM_SPEC_DISPLAY_ID (pspec);
  gint                    display_id;
  GimpObject             *display;

  display_id = value->data[0].v_int;
  display    = gimp_get_display_by_ID (ispec->gimp, display_id);

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
  gint display_id1;
  gint display_id2;

  display_id1 = value1->data[0].v_int;
  display_id2 = value2->data[0].v_int;

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
                            GParamFlags  flags)
{
  GimpParamSpecDisplayID *ispec;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DISPLAY_ID,
                                 name, nick, blurb, flags);

  if (ispec)
    ispec->gimp = gimp;

  return G_PARAM_SPEC (ispec);
}

GimpObject *
gimp_value_get_display (const GValue *value,
                        Gimp         *gimp)
{
  g_return_val_if_fail (G_VALUE_HOLDS_INT (value), NULL);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp_get_display_by_ID (gimp, value->data[0].v_int);
}

void
gimp_value_set_display (GValue     *value,
                        GimpObject *display)
{
  gint id = -1;

  g_return_if_fail (G_VALUE_HOLDS_INT (value));
  g_return_if_fail (display == NULL || GIMP_IS_OBJECT (display));

  if (display)
    g_object_get (display, "id", &id, NULL);

  value->data[0].v_int = id;
}


/*
 * GIMP_TYPE_RGB
 */

void
gimp_value_get_rgb (const GValue *value,
                    GimpRGB      *rgb)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_RGB (value));
  g_return_if_fail (rgb != NULL);

  if (value->data[0].v_pointer)
    *rgb = *((GimpRGB *) value->data[0].v_pointer);
  else
    gimp_rgba_set (rgb, 0.0, 0.0, 0.0, 1.0);
}

void
gimp_value_set_rgb (GValue  *value,
                    GimpRGB *rgb)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_RGB (value));
  g_return_if_fail (rgb != NULL);

  g_value_set_boxed (value, rgb);
}


/*
 * GIMP_TYPE_PARASITE
 */

GType
gimp_parasite_get_type (void)
{
  static GType type = 0;

  if (! type)
    type = g_boxed_type_register_static ("GimpParasite",
                                         (GBoxedCopyFunc) gimp_parasite_copy,
                                         (GBoxedFreeFunc) gimp_parasite_free);

  return type;
}


/*
 * GIMP_TYPE_PARAM_PARASITE
 */

static void       gimp_param_parasite_class_init  (GParamSpecClass *class);
static void       gimp_param_parasite_init        (GParamSpec      *pspec);
static void       gimp_param_parasite_set_default (GParamSpec      *pspec,
                                                   GValue          *value);
static gboolean   gimp_param_parasite_validate    (GParamSpec      *pspec,
                                                   GValue          *value);
static gint       gimp_param_parasite_values_cmp  (GParamSpec      *pspec,
                                                   const GValue    *value1,
                                                   const GValue    *value2);

GType
gimp_param_parasite_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_parasite_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecParasite),
        0,
        (GInstanceInitFunc) gimp_param_parasite_init
      };

      type = g_type_register_static (G_TYPE_PARAM_BOXED,
                                     "GimpParamParasite",
                                     &type_info, 0);
    }

  return type;
}

static void
gimp_param_parasite_class_init (GParamSpecClass *class)
{
  class->value_type        = GIMP_TYPE_PARASITE;
  class->value_set_default = gimp_param_parasite_set_default;
  class->value_validate    = gimp_param_parasite_validate;
  class->values_cmp        = gimp_param_parasite_values_cmp;
}

static void
gimp_param_parasite_init (GParamSpec *pspec)
{
}

static void
gimp_param_parasite_set_default (GParamSpec *pspec,
                                 GValue     *value)
{
}

static gboolean
gimp_param_parasite_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParasite *parasite;

  parasite = value->data[0].v_pointer;

  if (! parasite)
    {
      return TRUE;
    }
  else if (parasite->name == NULL                          ||
           ! g_utf8_validate (parasite->name, -1, NULL)    ||
           (parasite->size == 0 && parasite->data != NULL) ||
           (parasite->size >  0 && parasite->data == NULL))
    {
      g_value_set_boxed (value, NULL);
      return TRUE;
    }

  return TRUE;
}

static gint
gimp_param_parasite_values_cmp (GParamSpec   *pspec,
                                const GValue *value1,
                                const GValue *value2)
{
  GimpParasite *parasite1;
  GimpParasite *parasite2;

  parasite1 = value1->data[0].v_pointer;
  parasite2 = value2->data[0].v_pointer;

  /*  try to return at least *something*, it's useless anyway...  */

  if (! parasite1)
    return parasite2 != NULL ? -1 : 0;
  else if (! parasite2)
    return parasite1 != NULL;
  else
    return gimp_parasite_compare (parasite1, parasite2);
}

GParamSpec *
gimp_param_spec_parasite (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          GParamFlags  flags)
{
  GimpParamSpecParasite *parasite_spec;

  parasite_spec = g_param_spec_internal (GIMP_TYPE_PARAM_PARASITE,
                                         name, nick, blurb, flags);

  return G_PARAM_SPEC (parasite_spec);
}
