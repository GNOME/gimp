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

/**
 * gimp_param_spec_string:
 * @name:           Canonical name of the property specified.
 * @nick:           Nick name of the property specified.
 * @blurb:          Description of the property specified.
 * @allow_non_utf8: Whether non-UTF-8 strings are allowed.
 * @null_ok:        Whether %NULL is allowed.
 * @non_empty:      Whether a non-½NULL value must be set.
 * @default_value:  The default value.
 * @flags:          Flags for the property specified.
 *
 * Creates a new #GimpParamSpecString specifying a
 * #G_TYPE_STRING property.
 *
 * If @allow_non_utf8 is %FALSE, non-valid UTF-8 strings will be
 * replaced by question marks.
 *
 * If @null_ok is %FALSE, %NULL strings will be replaced by an empty
 * string.
 *
 * If @non_empty is %TRUE, empty strings will be replaced by `"none"`.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecString.
 *
 * Since: 3.0
 **/
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
 * GIMP_TYPE_PARAM_IMAGE
 */

static void       gimp_param_image_class_init (GParamSpecClass *klass);
static void       gimp_param_image_init       (GParamSpec      *pspec);
static gboolean   gimp_param_image_validate   (GParamSpec      *pspec,
                                               GValue          *value);

GType
gimp_param_image_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_image_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecImage),
        0,
        (GInstanceInitFunc) gimp_param_image_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamImage", &info, 0);
    }

  return type;
}

static void
gimp_param_image_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_IMAGE;
  klass->value_validate = gimp_param_image_validate;
}

static void
gimp_param_image_init (GParamSpec *pspec)
{
  GimpParamSpecImage *ispec = GIMP_PARAM_SPEC_IMAGE (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_image_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecImage *ispec = GIMP_PARAM_SPEC_IMAGE (pspec);
  GimpImage          *image = value->data[0].v_pointer;

  if (! ispec->none_ok && image == NULL)
    return TRUE;

  if (image && ! GIMP_IS_IMAGE (image))
    {
      g_object_unref (image);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_image (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  GimpParamSpecImage *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_IMAGE,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_ITEM
 */

static void       gimp_param_item_class_init (GParamSpecClass *klass);
static void       gimp_param_item_init       (GParamSpec      *pspec);
static gboolean   gimp_param_item_validate   (GParamSpec      *pspec,
                                              GValue          *value);

GType
gimp_param_item_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_item_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecItem),
        0,
        (GInstanceInitFunc) gimp_param_item_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamItem", &info, 0);
    }

  return type;
}

static void
gimp_param_item_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_ITEM;
  klass->value_validate = gimp_param_item_validate;
}

static void
gimp_param_item_init (GParamSpec *pspec)
{
  GimpParamSpecItem *ispec = GIMP_PARAM_SPEC_ITEM (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_item_validate (GParamSpec *pspec,
                          GValue     *value)
{
  GimpParamSpecItem *ispec = GIMP_PARAM_SPEC_ITEM (pspec);
  GimpItem          *item  = value->data[0].v_pointer;

  if (! ispec->none_ok && item == NULL)
    return TRUE;

  if (item && ! g_type_is_a (G_OBJECT_TYPE (item), pspec->value_type))
    {
      g_object_unref (item);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_item (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_ITEM,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_DRAWABLE
 */

static void   gimp_param_drawable_class_init (GParamSpecClass *klass);
static void   gimp_param_drawable_init       (GParamSpec      *pspec);

GType
gimp_param_drawable_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_drawable_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecDrawable),
        0,
        (GInstanceInitFunc) gimp_param_drawable_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ITEM,
                                     "GimpParamDrawable", &info, 0);
    }

  return type;
}

static void
gimp_param_drawable_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_DRAWABLE;
}

static void
gimp_param_drawable_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_drawable (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DRAWABLE,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_LAYER
 */

static void   gimp_param_layer_class_init (GParamSpecClass *klass);
static void   gimp_param_layer_init       (GParamSpec      *pspec);

GType
gimp_param_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_layer_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecLayer),
        0,
        (GInstanceInitFunc) gimp_param_layer_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_DRAWABLE,
                                     "GimpParamLayer", &info, 0);
    }

  return type;
}

static void
gimp_param_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_LAYER;
}

static void
gimp_param_layer_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_layer (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_CHANNEL
 */

static void   gimp_param_channel_class_init (GParamSpecClass *klass);
static void   gimp_param_channel_init       (GParamSpec      *pspec);

GType
gimp_param_channel_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_channel_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecChannel),
        0,
        (GInstanceInitFunc) gimp_param_channel_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_DRAWABLE,
                                     "GimpParamChannel", &info, 0);
    }

  return type;
}

static void
gimp_param_channel_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_CHANNEL;
}

static void
gimp_param_channel_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_channel (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_CHANNEL,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_LAYER_MASK
 */

static void   gimp_param_layer_mask_class_init (GParamSpecClass *klass);
static void   gimp_param_layer_mask_init       (GParamSpec      *pspec);

GType
gimp_param_layer_mask_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_layer_mask_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecLayerMask),
        0,
        (GInstanceInitFunc) gimp_param_layer_mask_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_CHANNEL,
                                     "GimpParamLayerMask", &info, 0);
    }

  return type;
}

static void
gimp_param_layer_mask_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_LAYER_MASK;
}

static void
gimp_param_layer_mask_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_layer_mask (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER_MASK,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_SELECTION
 */

static void   gimp_param_selection_class_init (GParamSpecClass *klass);
static void   gimp_param_selection_init       (GParamSpec      *pspec);

GType
gimp_param_selection_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_selection_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecSelection),
        0,
        (GInstanceInitFunc) gimp_param_selection_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_CHANNEL,
                                     "GimpParamSelection", &info, 0);
    }

  return type;
}

static void
gimp_param_selection_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_SELECTION;
}

static void
gimp_param_selection_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_selection (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           gboolean     none_ok,
                           GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_SELECTION,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_VECTORS
 */

static void   gimp_param_vectors_class_init (GParamSpecClass *klass);
static void   gimp_param_vectors_init       (GParamSpec      *pspec);

GType
gimp_param_vectors_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_vectors_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecVectors),
        0,
        (GInstanceInitFunc) gimp_param_vectors_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ITEM,
                                     "GimpParamVectors", &info, 0);
    }

  return type;
}

static void
gimp_param_vectors_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_VECTORS;
}

static void
gimp_param_vectors_init (GParamSpec *pspec)
{
}

GParamSpec *
gimp_param_spec_vectors (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_VECTORS,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_DISPLAY
 */

static void       gimp_param_display_class_init  (GParamSpecClass *klass);
static void       gimp_param_display_init        (GParamSpec      *pspec);
static gboolean   gimp_param_display_validate    (GParamSpec      *pspec,
                                                     GValue          *value);

GType
gimp_param_display_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_display_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecDisplay),
        0,
        (GInstanceInitFunc) gimp_param_display_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamDisplay", &info, 0);
    }

  return type;
}

static void
gimp_param_display_class_init (GParamSpecClass *klass)
{
  klass->value_type = g_type_from_name ("GimpDisplay");

  g_assert (klass->value_type != G_TYPE_NONE);

  klass->value_validate = gimp_param_display_validate;
}

static void
gimp_param_display_init (GParamSpec *pspec)
{
  GimpParamSpecDisplay *ispec = GIMP_PARAM_SPEC_DISPLAY (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_display_validate (GParamSpec *pspec,
                             GValue     *value)
{
  GimpParamSpecDisplay *ispec   = GIMP_PARAM_SPEC_DISPLAY (pspec);
  GObject              *display = value->data[0].v_pointer;

  if (! ispec->none_ok && display == NULL)
    return TRUE;

  if (display && ! g_value_type_compatible (G_OBJECT_TYPE (display),
                                            G_PARAM_SPEC_VALUE_TYPE (pspec)))
    {
      g_object_unref (display);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_display (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecDisplay *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DISPLAY,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}
