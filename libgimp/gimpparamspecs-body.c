/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpparamspecs-body.c
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

/*  this file is included by both
 *
 *  libgimp/gimpparamspecs.c
 *  app/core/gimpparamspecs.c
 */

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

  if (image && (! GIMP_IS_IMAGE (image) ||
                ! gimp_image_is_valid (image)))
    {
      g_object_unref (image);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_image:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecImage specifying a
 * #GIMP_TYPE_IMAGE property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecImage.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

  if (item && (! g_type_is_a (G_OBJECT_TYPE (item), pspec->value_type) ||
               ! gimp_item_is_valid (item)))
    {
      g_object_unref (item);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_item:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecItem specifying a
 * #GIMP_TYPE_ITEM property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecItem.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_drawable:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecDrawable specifying a
 * #GIMP_TYPE_DRAWABLE property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecDrawable.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecLayer specifying a
 * #GIMP_TYPE_LAYER property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecLayer.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_channel:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecChannel specifying a
 * #GIMP_TYPE_CHANNEL property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecChannel.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_layer_mask:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecLayerMask specifying a
 * #GIMP_TYPE_LAYER_MASK property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecLayerMask.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_selection:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecSelection specifying a
 * #GIMP_TYPE_SELECTION property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecSelection.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

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

/**
 * gimp_param_spec_vectors:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecVectors specifying a
 * #GIMP_TYPE_VECTORS property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecVectors.
 *
 * Since: 3.0
 **/
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

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_DISPLAY
 */

static void       gimp_param_display_class_init (GParamSpecClass *klass);
static void       gimp_param_display_init       (GParamSpec      *pspec);
static gboolean   gimp_param_display_validate   (GParamSpec      *pspec,
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
  klass->value_type     = GIMP_TYPE_DISPLAY;
  klass->value_validate = gimp_param_display_validate;
}

static void
gimp_param_display_init (GParamSpec *pspec)
{
  GimpParamSpecDisplay *dspec = GIMP_PARAM_SPEC_DISPLAY (pspec);

  dspec->none_ok = FALSE;
}

static gboolean
gimp_param_display_validate (GParamSpec *pspec,
                             GValue     *value)
{
  GimpParamSpecDisplay *dspec   = GIMP_PARAM_SPEC_DISPLAY (pspec);
  GimpDisplay          *display = value->data[0].v_pointer;

  if (! dspec->none_ok && display == NULL)
    return TRUE;

  if (display && (! GIMP_IS_DISPLAY (display) ||
                  ! gimp_display_is_valid (display)))
    {
      g_object_unref (display);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_display:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecDisplay specifying a
 * #GIMP_TYPE_DISPLAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecDisplay.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_display (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecDisplay *dspec;

  dspec = g_param_spec_internal (GIMP_TYPE_PARAM_DISPLAY,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (dspec, NULL);

  dspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (dspec);
}
