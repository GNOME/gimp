/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaparamspecs-body.c
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
 *  libligma/ligmaparamspecs.c
 *  app/core/ligmaparamspecs.c
 */

/*
 * LIGMA_TYPE_PARAM_IMAGE
 */

static void       ligma_param_image_class_init (GParamSpecClass *klass);
static void       ligma_param_image_init       (GParamSpec      *pspec);
static gboolean   ligma_param_image_validate   (GParamSpec      *pspec,
                                               GValue          *value);

GType
ligma_param_image_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_image_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecImage),
        0,
        (GInstanceInitFunc) ligma_param_image_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "LigmaParamImage", &info, 0);
    }

  return type;
}

static void
ligma_param_image_class_init (GParamSpecClass *klass)
{
  klass->value_type     = LIGMA_TYPE_IMAGE;
  klass->value_validate = ligma_param_image_validate;
}

static void
ligma_param_image_init (GParamSpec *pspec)
{
  LigmaParamSpecImage *ispec = LIGMA_PARAM_SPEC_IMAGE (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
ligma_param_image_validate (GParamSpec *pspec,
                           GValue     *value)
{
  LigmaParamSpecImage *ispec = LIGMA_PARAM_SPEC_IMAGE (pspec);
  LigmaImage          *image = value->data[0].v_pointer;

  if (! ispec->none_ok && image == NULL)
    return TRUE;

  if (image && (! LIGMA_IS_IMAGE (image) ||
                ! ligma_image_is_valid (image)))
    {
      g_object_unref (image);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_param_spec_image:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecImage specifying a
 * #LIGMA_TYPE_IMAGE property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecImage.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_image (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  LigmaParamSpecImage *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_IMAGE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_ITEM
 */

static void       ligma_param_item_class_init (GParamSpecClass *klass);
static void       ligma_param_item_init       (GParamSpec      *pspec);
static gboolean   ligma_param_item_validate   (GParamSpec      *pspec,
                                              GValue          *value);

GType
ligma_param_item_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_item_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecItem),
        0,
        (GInstanceInitFunc) ligma_param_item_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "LigmaParamItem", &info, 0);
    }

  return type;
}

static void
ligma_param_item_class_init (GParamSpecClass *klass)
{
  klass->value_type     = LIGMA_TYPE_ITEM;
  klass->value_validate = ligma_param_item_validate;
}

static void
ligma_param_item_init (GParamSpec *pspec)
{
  LigmaParamSpecItem *ispec = LIGMA_PARAM_SPEC_ITEM (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
ligma_param_item_validate (GParamSpec *pspec,
                          GValue     *value)
{
  LigmaParamSpecItem *ispec = LIGMA_PARAM_SPEC_ITEM (pspec);
  LigmaItem          *item  = value->data[0].v_pointer;

  if (! ispec->none_ok && item == NULL)
    return TRUE;

  if (item && (! g_type_is_a (G_OBJECT_TYPE (item), pspec->value_type) ||
               ! ligma_item_is_valid (item)))
    {
      g_object_unref (item);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_param_spec_item:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecItem specifying a
 * #LIGMA_TYPE_ITEM property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecItem.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_item (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_ITEM,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_DRAWABLE
 */

static void   ligma_param_drawable_class_init (GParamSpecClass *klass);
static void   ligma_param_drawable_init       (GParamSpec      *pspec);

GType
ligma_param_drawable_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_drawable_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecDrawable),
        0,
        (GInstanceInitFunc) ligma_param_drawable_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_ITEM,
                                     "LigmaParamDrawable", &info, 0);
    }

  return type;
}

static void
ligma_param_drawable_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_DRAWABLE;
}

static void
ligma_param_drawable_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_drawable:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecDrawable specifying a
 * #LIGMA_TYPE_DRAWABLE property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecDrawable.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_drawable (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_DRAWABLE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_LAYER
 */

static void   ligma_param_layer_class_init (GParamSpecClass *klass);
static void   ligma_param_layer_init       (GParamSpec      *pspec);

GType
ligma_param_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_layer_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecLayer),
        0,
        (GInstanceInitFunc) ligma_param_layer_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_DRAWABLE,
                                     "LigmaParamLayer", &info, 0);
    }

  return type;
}

static void
ligma_param_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_LAYER;
}

static void
ligma_param_layer_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecLayer specifying a
 * #LIGMA_TYPE_LAYER property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_layer (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_TEXT_LAYER
 */

static void   ligma_param_text_layer_class_init (GParamSpecClass *klass);
static void   ligma_param_text_layer_init       (GParamSpec      *pspec);

GType
ligma_param_text_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_text_layer_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecTextLayer),
        0,
        (GInstanceInitFunc) ligma_param_text_layer_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_LAYER,
                                     "LigmaParamTextLayer", &info, 0);
    }

  return type;
}

static void
ligma_param_text_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_TEXT_LAYER;
}

static void
ligma_param_text_layer_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_text_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecTextLayer specifying a
 * #LIGMA_TYPE_TEXT_LAYER property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecTextLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_text_layer (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_TEXT_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_CHANNEL
 */

static void   ligma_param_channel_class_init (GParamSpecClass *klass);
static void   ligma_param_channel_init       (GParamSpec      *pspec);

GType
ligma_param_channel_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_channel_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecChannel),
        0,
        (GInstanceInitFunc) ligma_param_channel_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_DRAWABLE,
                                     "LigmaParamChannel", &info, 0);
    }

  return type;
}

static void
ligma_param_channel_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_CHANNEL;
}

static void
ligma_param_channel_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_channel:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecChannel specifying a
 * #LIGMA_TYPE_CHANNEL property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecChannel.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_channel (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_CHANNEL,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_LAYER_MASK
 */

static void   ligma_param_layer_mask_class_init (GParamSpecClass *klass);
static void   ligma_param_layer_mask_init       (GParamSpec      *pspec);

GType
ligma_param_layer_mask_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_layer_mask_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecLayerMask),
        0,
        (GInstanceInitFunc) ligma_param_layer_mask_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_CHANNEL,
                                     "LigmaParamLayerMask", &info, 0);
    }

  return type;
}

static void
ligma_param_layer_mask_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_LAYER_MASK;
}

static void
ligma_param_layer_mask_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_layer_mask:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecLayerMask specifying a
 * #LIGMA_TYPE_LAYER_MASK property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecLayerMask.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_layer_mask (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_LAYER_MASK,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_SELECTION
 */

static void   ligma_param_selection_class_init (GParamSpecClass *klass);
static void   ligma_param_selection_init       (GParamSpec      *pspec);

GType
ligma_param_selection_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_selection_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecSelection),
        0,
        (GInstanceInitFunc) ligma_param_selection_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_CHANNEL,
                                     "LigmaParamSelection", &info, 0);
    }

  return type;
}

static void
ligma_param_selection_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_SELECTION;
}

static void
ligma_param_selection_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_selection:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecSelection specifying a
 * #LIGMA_TYPE_SELECTION property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecSelection.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_selection (const gchar *name,
                           const gchar *nick,
                           const gchar *blurb,
                           gboolean     none_ok,
                           GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_SELECTION,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_VECTORS
 */

static void   ligma_param_vectors_class_init (GParamSpecClass *klass);
static void   ligma_param_vectors_init       (GParamSpec      *pspec);

GType
ligma_param_vectors_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_vectors_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecVectors),
        0,
        (GInstanceInitFunc) ligma_param_vectors_init
      };

      type = g_type_register_static (LIGMA_TYPE_PARAM_ITEM,
                                     "LigmaParamVectors", &info, 0);
    }

  return type;
}

static void
ligma_param_vectors_class_init (GParamSpecClass *klass)
{
  klass->value_type = LIGMA_TYPE_VECTORS;
}

static void
ligma_param_vectors_init (GParamSpec *pspec)
{
}

/**
 * ligma_param_spec_vectors:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecVectors specifying a
 * #LIGMA_TYPE_VECTORS property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecVectors.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_vectors (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  LigmaParamSpecItem *ispec;

  ispec = g_param_spec_internal (LIGMA_TYPE_PARAM_VECTORS,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * LIGMA_TYPE_PARAM_DISPLAY
 */

static void       ligma_param_display_class_init (GParamSpecClass *klass);
static void       ligma_param_display_init       (GParamSpec      *pspec);
static gboolean   ligma_param_display_validate   (GParamSpec      *pspec,
                                                 GValue          *value);

GType
ligma_param_display_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_display_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecDisplay),
        0,
        (GInstanceInitFunc) ligma_param_display_init
      };

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "LigmaParamDisplay", &info, 0);
    }

  return type;
}

static void
ligma_param_display_class_init (GParamSpecClass *klass)
{
  klass->value_type     = LIGMA_TYPE_DISPLAY;
  klass->value_validate = ligma_param_display_validate;
}

static void
ligma_param_display_init (GParamSpec *pspec)
{
  LigmaParamSpecDisplay *dspec = LIGMA_PARAM_SPEC_DISPLAY (pspec);

  dspec->none_ok = FALSE;
}

static gboolean
ligma_param_display_validate (GParamSpec *pspec,
                             GValue     *value)
{
  LigmaParamSpecDisplay *dspec   = LIGMA_PARAM_SPEC_DISPLAY (pspec);
  LigmaDisplay          *display = value->data[0].v_pointer;

  if (! dspec->none_ok && display == NULL)
    return TRUE;

  if (display && (! LIGMA_IS_DISPLAY (display) ||
                  ! ligma_display_is_valid (display)))
    {
      g_object_unref (display);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_param_spec_display:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #LigmaParamSpecDisplay specifying a
 * #LIGMA_TYPE_DISPLAY property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #LigmaParamSpecDisplay.
 *
 * Since: 3.0
 **/
GParamSpec *
ligma_param_spec_display (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  LigmaParamSpecDisplay *dspec;

  dspec = g_param_spec_internal (LIGMA_TYPE_PARAM_DISPLAY,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (dspec, NULL);

  dspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (dspec);
}
