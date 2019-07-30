/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimp.h"


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

  if (ispec->none_ok && (image_id == 0 || image_id == -1))
    return FALSE;

  if (! gimp_image_is_valid (image_id))
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
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecImageID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_IMAGE_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_image_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_IMAGE_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_image_id (GValue *value,
                         gint32  image_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_IMAGE_ID (value));

  value->data[0].v_int = image_id;
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

  ispec->none_ok = FALSE;
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

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_valid (item_id))
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
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_ITEM_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_item_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_ITEM_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_item_id (GValue *value,
                        gint32  item_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_ITEM_ID (value));

  value->data[0].v_int = item_id;
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

static void       gimp_param_drawable_id_class_init (GParamSpecClass *klass);
static void       gimp_param_drawable_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_drawable_id_validate   (GParamSpec      *pspec,
                                                     GValue          *value);

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
  klass->value_type     = GIMP_TYPE_DRAWABLE_ID;
  klass->value_validate = gimp_param_drawable_id_validate;
}

static void
gimp_param_drawable_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_drawable_id_validate (GParamSpec *pspec,
                                 GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_drawable (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_drawable_id (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             gboolean     none_ok,
                             GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DRAWABLE_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_drawable_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_DRAWABLE_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_drawable_id (GValue *value,
                            gint32  drawable_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DRAWABLE_ID (value));

  value->data[0].v_int = drawable_id;
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

static void       gimp_param_layer_id_class_init (GParamSpecClass *klass);
static void       gimp_param_layer_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_layer_id_validate   (GParamSpec      *pspec,
                                                  GValue          *value);

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
  klass->value_type     = GIMP_TYPE_LAYER_ID;
  klass->value_validate = gimp_param_layer_id_validate;
}

static void
gimp_param_layer_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_layer_id_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_layer (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_layer_id (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_layer_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_LAYER_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_layer_id (GValue *value,
                         gint32  layer_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_LAYER_ID (value));

  value->data[0].v_int = layer_id;
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

static void       gimp_param_channel_id_class_init (GParamSpecClass *klass);
static void       gimp_param_channel_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_channel_id_validate   (GParamSpec      *pspec,
                                                    GValue          *value);

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
  klass->value_type     = GIMP_TYPE_CHANNEL_ID;
  klass->value_validate = gimp_param_channel_id_validate;
}

static void
gimp_param_channel_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_channel_id_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_channel (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_channel_id (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_CHANNEL_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_channel_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_CHANNEL_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_channel_id (GValue *value,
                           gint32  channel_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_CHANNEL_ID (value));

  value->data[0].v_int = channel_id;
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

static void       gimp_param_layer_mask_id_class_init (GParamSpecClass *klass);
static void       gimp_param_layer_mask_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_layer_mask_id_validate   (GParamSpec      *pspec,
                                                       GValue          *value);

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
  klass->value_type     = GIMP_TYPE_LAYER_MASK_ID;
  klass->value_validate = gimp_param_layer_mask_id_validate;
}

static void
gimp_param_layer_mask_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_layer_mask_id_validate (GParamSpec *pspec,
                                   GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_layer_mask (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_layer_mask_id (const gchar *name,
                               const gchar *nick,
                               const gchar *blurb,
                               gboolean     none_ok,
                               GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_LAYER_MASK_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_layer_mask_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_LAYER_MASK_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_layer_mask_id (GValue *value,
                              gint32  layer_mask_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_LAYER_MASK_ID (value));

  value->data[0].v_int = layer_mask_id;
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

static void       gimp_param_selection_id_class_init (GParamSpecClass *klass);
static void       gimp_param_selection_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_selection_id_validate   (GParamSpec      *pspec,
                                                      GValue          *value);

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
  klass->value_type     = GIMP_TYPE_SELECTION_ID;
  klass->value_validate = gimp_param_selection_id_validate;
}

static void
gimp_param_selection_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_selection_id_validate (GParamSpec *pspec,
                                  GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_selection (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_selection_id (const gchar *name,
                              const gchar *nick,
                              const gchar *blurb,
                              gboolean     none_ok,
                              GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_SELECTION_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_selection_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_SELECTION_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_selection_id (GValue *value,
                             gint32  selection_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_SELECTION_ID (value));

  value->data[0].v_int = selection_id;
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

static void       gimp_param_vectors_id_class_init (GParamSpecClass *klass);
static void       gimp_param_vectors_id_init       (GParamSpec      *pspec);
static gboolean   gimp_param_vectors_id_validate   (GParamSpec      *pspec,
                                                    GValue          *value);

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
  klass->value_type     = GIMP_TYPE_VECTORS_ID;
  klass->value_validate = gimp_param_vectors_id_validate;
}

static void
gimp_param_vectors_id_init (GParamSpec *pspec)
{
}

static gboolean
gimp_param_vectors_id_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GimpParamSpecItemID *ispec   = GIMP_PARAM_SPEC_ITEM_ID (pspec);
  gint                 item_id = value->data[0].v_int;

  if (ispec->none_ok && (item_id == 0 || item_id == -1))
    return FALSE;

  if (! gimp_item_is_vectors (item_id))
    {
      value->data[0].v_int = -1;
      return TRUE;
    }

  return FALSE;
}

GParamSpec *
gimp_param_spec_vectors_id (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItemID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_VECTORS_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_vectors_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_VECTORS_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_vectors_id (GValue *value,
                           gint32  vectors_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_VECTORS_ID (value));

  value->data[0].v_int = vectors_id;
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

  if (ispec->none_ok && (display_id == 0 || display_id == -1))
    return FALSE;

  if (! gimp_display_is_valid (display_id))
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
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecDisplayID *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_DISPLAY_ID,
                                 name, nick, blurb, flags);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}

gint32
gimp_value_get_display_id (const GValue *value)
{
  g_return_val_if_fail (GIMP_VALUE_HOLDS_DISPLAY_ID (value), -1);

  return value->data[0].v_int;
}

void
gimp_value_set_display_id (GValue *value,
                           gint32  display_id)
{
  g_return_if_fail (GIMP_VALUE_HOLDS_DISPLAY_ID (value));

  value->data[0].v_int = display_id;
}
