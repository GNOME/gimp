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
 * [type@Image] property.
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
 * [type@Item] property.
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
 * [type@Drawable] property.
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
 * [type@Layer] property.
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
 * GIMP_TYPE_PARAM_TEXT_LAYER
 */

static void   gimp_param_text_layer_class_init (GParamSpecClass *klass);
static void   gimp_param_text_layer_init       (GParamSpec      *pspec);

GType
gimp_param_text_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_text_layer_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecTextLayer),
        0,
        (GInstanceInitFunc) gimp_param_text_layer_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_LAYER,
                                     "GimpParamTextLayer", &info, 0);
    }

  return type;
}

static void
gimp_param_text_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_TEXT_LAYER;
}

static void
gimp_param_text_layer_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_text_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecTextLayer specifying a
 * [type@TextLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecTextLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_text_layer (const gchar *name,
                            const gchar *nick,
                            const gchar *blurb,
                            gboolean     none_ok,
                            GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_TEXT_LAYER,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}


/*
 * GIMP_TYPE_PARAM_GROUP_LAYER
 */

static void   gimp_param_group_layer_class_init (GParamSpecClass *klass);
static void   gimp_param_group_layer_init       (GParamSpec      *pspec);

GType
gimp_param_group_layer_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_group_layer_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecGroupLayer),
        0,
        (GInstanceInitFunc) gimp_param_group_layer_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_LAYER,
                                     "GimpParamGroupLayer", &info, 0);
    }

  return type;
}

static void
gimp_param_group_layer_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_GROUP_LAYER;
}

static void
gimp_param_group_layer_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_group_layer:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecGroupLayer specifying a
 * [type@GroupLayer] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecGroupLayer.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_group_layer (const gchar *name,
                             const gchar *nick,
                             const gchar *blurb,
                             gboolean     none_ok,
                             GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_GROUP_LAYER,
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
 * [type@Channel] property.
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
 * [type@LayerMask] property.
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
 * [type@Selection] property.
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
 * GIMP_TYPE_PARAM_PATH
 */

static void   gimp_param_path_class_init (GParamSpecClass *klass);
static void   gimp_param_path_init       (GParamSpec      *pspec);

GType
gimp_param_path_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_path_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecPath),
        0,
        (GInstanceInitFunc) gimp_param_path_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_ITEM,
                                     "GimpParamPath", &info, 0);
    }

  return type;
}

static void
gimp_param_path_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_PATH;
}

static void
gimp_param_path_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_path:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecPath specifying a
 * [type@Path] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecPath.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_path (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  GimpParamSpecItem *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_PATH,
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
 * [type@Display] property.
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


 /*
  * GIMP_TYPE_PARAM_RESOURCE
  */

 static void       gimp_param_resource_class_init (GParamSpecClass *klass);
 static void       gimp_param_resource_init       (GParamSpec      *pspec);
 static gboolean   gimp_param_resource_validate   (GParamSpec      *pspec,
                                                   GValue          *value);

 GType
 gimp_param_resource_get_type (void)
 {
   static GType type = 0;

   if (! type)
     {
       const GTypeInfo info =
       {
         sizeof (GParamSpecClass),
         NULL, NULL,
         (GClassInitFunc) gimp_param_resource_class_init,
         NULL, NULL,
         sizeof (GimpParamSpecResource),
         0,
         (GInstanceInitFunc) gimp_param_resource_init
       };

       type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                      "GimpParamResource", &info, 0);
     }

   return type;
 }

 static void
 gimp_param_resource_class_init (GParamSpecClass *klass)
 {
   klass->value_type     = GIMP_TYPE_RESOURCE;
   klass->value_validate = gimp_param_resource_validate;
 }

static void
gimp_param_resource_init (GParamSpec *pspec)
{
  GimpParamSpecResource *rspec = GIMP_PARAM_SPEC_RESOURCE (pspec);

  rspec->none_ok = FALSE;
}

static gboolean
gimp_param_resource_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecResource *rspec    = GIMP_PARAM_SPEC_RESOURCE (pspec);
  GObject               *resource = value->data[0].v_pointer;

  if (! rspec->none_ok && resource == NULL)
    return TRUE;

  if (resource && (! g_type_is_a (G_OBJECT_TYPE (resource), pspec->value_type) ||
                   ! gimp_resource_is_valid ((gpointer) resource)))
    {
      g_object_unref (resource);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_resource:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecResource specifying a
 * [type@Resource] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecResource.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_resource (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_RESOURCE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}


/*
 * GIMP_TYPE_PARAM_BRUSH
 */

static void   gimp_param_brush_class_init (GParamSpecClass *klass);
static void   gimp_param_brush_init       (GParamSpec      *pspec);

GType
gimp_param_brush_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_brush_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecBrush),
        0,
        (GInstanceInitFunc) gimp_param_brush_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_RESOURCE,
                                     "GimpParamBrush", &info, 0);
    }

  return type;
}

static void
gimp_param_brush_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_BRUSH;
}

static void
gimp_param_brush_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_brush:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecBrush specifying a
 * [type@Brush] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecBrush.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_brush (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       gboolean     none_ok,
                       GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_BRUSH,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}


/*
 * GIMP_TYPE_PARAM_PATTERN
 */

static void   gimp_param_pattern_class_init (GParamSpecClass *klass);
static void   gimp_param_pattern_init       (GParamSpec      *pspec);

GType
gimp_param_pattern_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_pattern_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecPattern),
        0,
        (GInstanceInitFunc) gimp_param_pattern_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_RESOURCE,
                                     "GimpParamPattern", &info, 0);
    }

  return type;
}

static void
gimp_param_pattern_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_PATTERN;
}

static void
gimp_param_pattern_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_pattern:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecPattern specifying a
 * [type@Pattern] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecPattern.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_pattern (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_PATTERN,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}


/*
 * GIMP_TYPE_PARAM_GRADIENT
 */

static void   gimp_param_gradient_class_init (GParamSpecClass *klass);
static void   gimp_param_gradient_init       (GParamSpec      *pspec);

GType
gimp_param_gradient_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_gradient_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecGradient),
        0,
        (GInstanceInitFunc) gimp_param_gradient_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_RESOURCE,
                                     "GimpParamGradient", &info, 0);
    }

  return type;
}

static void
gimp_param_gradient_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_GRADIENT;
}

static void
gimp_param_gradient_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_gradient:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecGradient specifying a
 * [type@Gradient] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecGradient.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_gradient (const gchar *name,
                          const gchar *nick,
                          const gchar *blurb,
                          gboolean     none_ok,
                          GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_GRADIENT,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}


/*
 * GIMP_TYPE_PARAM_PALETTE
 */

static void   gimp_param_palette_class_init (GParamSpecClass *klass);
static void   gimp_param_palette_init       (GParamSpec      *pspec);

GType
gimp_param_palette_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_palette_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecPalette),
        0,
        (GInstanceInitFunc) gimp_param_palette_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_RESOURCE,
                                     "GimpParamPalette", &info, 0);
    }

  return type;
}

static void
gimp_param_palette_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_PALETTE;
}

static void
gimp_param_palette_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_palette:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecPalette specifying a
 * [type@Palette] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecPalette.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_palette (const gchar *name,
                         const gchar *nick,
                         const gchar *blurb,
                         gboolean     none_ok,
                         GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_PALETTE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}


/*
 * GIMP_TYPE_PARAM_FONT
 */

static void   gimp_param_font_class_init (GParamSpecClass *klass);
static void   gimp_param_font_init       (GParamSpec      *pspec);

GType
gimp_param_font_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) gimp_param_font_class_init,
        NULL, NULL,
        sizeof (GimpParamSpecFont),
        0,
        (GInstanceInitFunc) gimp_param_font_init
      };

      type = g_type_register_static (GIMP_TYPE_PARAM_RESOURCE,
                                     "GimpParamFont", &info, 0);
    }

  return type;
}

static void
gimp_param_font_class_init (GParamSpecClass *klass)
{
  klass->value_type = GIMP_TYPE_FONT;
}

static void
gimp_param_font_init (GParamSpec *pspec)
{
}

/**
 * gimp_param_spec_font:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether %NULL is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecFont specifying a
 * [type@Font] property.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer full): The newly created #GimpParamSpecFont.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_param_spec_font (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     none_ok,
                      GParamFlags  flags)
{
  GimpParamSpecResource *rspec;

  rspec = g_param_spec_internal (GIMP_TYPE_PARAM_FONT,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (rspec, NULL);

  rspec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (rspec);
}
