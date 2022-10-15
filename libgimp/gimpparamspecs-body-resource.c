
/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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


/*  this .c file is included by both
 *
 *  libgimp/gimpparamspecs.c
 *  app/core/gimpparamspecs.c
 */


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
   GimpParamSpecResource *ispec = GIMP_PARAM_SPEC_RESOURCE (pspec);

   ispec->none_ok = FALSE;
 }

 /* Return TRUE when value is invalid!!!
  * FIXME: should be named is_invalid.
  */
static gboolean
gimp_param_resource_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecResource *ispec = GIMP_PARAM_SPEC_RESOURCE (pspec);
  GimpResource          *resource = value->data[0].v_pointer;

  g_debug (">>>>>%s", G_STRFUNC);

  if (! ispec->none_ok && resource == NULL)
    /* NULL when NULL not allowed is invalid. */
    return TRUE;

  if (resource != NULL)
    {
      if (GIMP_IS_BRUSH (resource) && gimp_brush_is_valid (GIMP_BRUSH (resource)))
        return FALSE;
      else if (GIMP_IS_FONT (resource) && gimp_font_is_valid (GIMP_FONT (resource)))
        return FALSE;
      else if (GIMP_IS_GRADIENT (resource) && gimp_gradient_is_valid (GIMP_GRADIENT (resource)))
        return FALSE;
      else if (GIMP_IS_PALETTE (resource) && gimp_palette_is_valid (GIMP_PALETTE (resource)))
        return FALSE;
      else if (GIMP_IS_PATTERN (resource) && gimp_pattern_is_valid (GIMP_PATTERN (resource)))
        return FALSE;
      else
        {
          /* Not a resource type or is invalid reference to core resource.
           * Plugin might be using a resource that was uninstalled.
           */
          g_object_unref (resource);
          value->data[0].v_pointer = NULL;
          return TRUE;
        }
    }
  else
    {
      /* resource is NULL but null is valid.*/
      return FALSE;
    }
}

 /**
  * gimp_param_spec_resource:
  * @name:    Canonical name of the property specified.
  * @nick:    Nick name of the property specified.
  * @blurb:   Description of the property specified.
  * @none_ok: Whether no  is a valid value.
  * @flags:   Flags for the property specified.
  *
  * Creates a new #GimpParamSpecResource specifying a
  * #GIMP_TYPE_RESOURCE property.
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
   GimpParamSpecResource *ispec;

   ispec = g_param_spec_internal (GIMP_TYPE_PARAM_RESOURCE,
                                  name, nick, blurb, flags);

   g_return_val_if_fail (ispec, NULL);

   ispec->none_ok = none_ok ? TRUE : FALSE;

   return G_PARAM_SPEC (ispec);
 }




/*
 * GIMP_TYPE_PARAM_BRUSH
 */

static void       gimp_param_brush_class_init (GParamSpecClass *klass);
static void       gimp_param_brush_init       (GParamSpec      *pspec);
static gboolean   gimp_param_brush_validate   (GParamSpec      *pspec,
                                               GValue          *value);

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

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamBrush", &info, 0);
    }

  return type;
}

static void
gimp_param_brush_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_BRUSH;
  klass->value_validate = gimp_param_brush_validate;
}

static void
gimp_param_brush_init (GParamSpec *pspec)
{
  GimpParamSpecBrush *ispec = GIMP_PARAM_SPEC_BRUSH (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_brush_validate (GParamSpec *pspec,
                           GValue     *value)
{
  GimpParamSpecBrush *ispec = GIMP_PARAM_SPEC_BRUSH (pspec);
  GimpBrush          *brush = value->data[0].v_pointer;

  if (! ispec->none_ok && brush == NULL)
    return TRUE;

  if (brush && (! GIMP_IS_BRUSH (brush) ||
                ! gimp_brush_is_valid (brush)))
    {
      g_object_unref (brush);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_brush:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecBrush specifying a
 * #GIMP_TYPE_BRUSH property.
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
  GimpParamSpecBrush *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_BRUSH,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}



/*
 * GIMP_TYPE_PARAM_FONT
 */

static void       gimp_param_font_class_init (GParamSpecClass *klass);
static void       gimp_param_font_init       (GParamSpec      *pspec);
static gboolean   gimp_param_font_validate   (GParamSpec      *pspec,
                                              GValue          *value);

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

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamFont", &info, 0);
    }

  return type;
}

static void
gimp_param_font_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_FONT;
  klass->value_validate = gimp_param_font_validate;
}

static void
gimp_param_font_init (GParamSpec *pspec)
{
  GimpParamSpecFont *ispec = GIMP_PARAM_SPEC_FONT (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_font_validate (GParamSpec *pspec,
                          GValue     *value)
{
  GimpParamSpecFont *ispec = GIMP_PARAM_SPEC_FONT (pspec);
  GimpFont          *font = value->data[0].v_pointer;

  if (! ispec->none_ok && font == NULL)
    return TRUE;

  if (font && (! GIMP_IS_FONT (font) ||
               ! gimp_font_is_valid (font)))
    {
      g_object_unref (font);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_font:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecFont specifying a
 * #GIMP_TYPE_FONT property.
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
  GimpParamSpecFont *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_FONT,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}



/*
 * GIMP_TYPE_PARAM_GRADIENT
 */

static void       gimp_param_gradient_class_init (GParamSpecClass *klass);
static void       gimp_param_gradient_init       (GParamSpec      *pspec);
static gboolean   gimp_param_gradient_validate   (GParamSpec      *pspec,
                                                  GValue          *value);

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

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamGradient", &info, 0);
    }

  return type;
}

static void
gimp_param_gradient_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_GRADIENT;
  klass->value_validate = gimp_param_gradient_validate;
}

static void
gimp_param_gradient_init (GParamSpec *pspec)
{
  GimpParamSpecGradient *ispec = GIMP_PARAM_SPEC_GRADIENT (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_gradient_validate (GParamSpec *pspec,
                              GValue     *value)
{
  GimpParamSpecGradient *ispec = GIMP_PARAM_SPEC_GRADIENT (pspec);
  GimpGradient          *gradient = value->data[0].v_pointer;

  if (! ispec->none_ok && gradient == NULL)
    return TRUE;

  if (gradient && (! GIMP_IS_GRADIENT (gradient) ||
                   ! gimp_gradient_is_valid (gradient)))
    {
      g_object_unref (gradient);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_gradient:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecGradient specifying a
 * #GIMP_TYPE_GRADIENT property.
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
  GimpParamSpecGradient *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_GRADIENT,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}



/*
 * GIMP_TYPE_PARAM_PALETTE
 */

static void       gimp_param_palette_class_init (GParamSpecClass *klass);
static void       gimp_param_palette_init       (GParamSpec      *pspec);
static gboolean   gimp_param_palette_validate   (GParamSpec      *pspec,
                                                 GValue          *value);

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

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamPalette", &info, 0);
    }

  return type;
}

static void
gimp_param_palette_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_PALETTE;
  klass->value_validate = gimp_param_palette_validate;
}

static void
gimp_param_palette_init (GParamSpec *pspec)
{
  GimpParamSpecPalette *ispec = GIMP_PARAM_SPEC_PALETTE (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_palette_validate (GParamSpec *pspec,
                             GValue     *value)
{
  GimpParamSpecPalette *ispec = GIMP_PARAM_SPEC_PALETTE (pspec);
  GimpPalette          *palette = value->data[0].v_pointer;

  if (! ispec->none_ok && palette == NULL)
    return TRUE;

  if (palette && (! GIMP_IS_PALETTE (palette) ||
                  ! gimp_palette_is_valid (palette)))
    {
      g_object_unref (palette);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_palette:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecPalette specifying a
 * #GIMP_TYPE_PALETTE property.
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
  GimpParamSpecPalette *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_PALETTE,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}



/*
 * GIMP_TYPE_PARAM_PATTERN
 */

static void       gimp_param_pattern_class_init (GParamSpecClass *klass);
static void       gimp_param_pattern_init       (GParamSpec      *pspec);
static gboolean   gimp_param_pattern_validate   (GParamSpec      *pspec,
                                                 GValue          *value);

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

      type = g_type_register_static (G_TYPE_PARAM_OBJECT,
                                     "GimpParamPattern", &info, 0);
    }

  return type;
}

static void
gimp_param_pattern_class_init (GParamSpecClass *klass)
{
  klass->value_type     = GIMP_TYPE_PATTERN;
  klass->value_validate = gimp_param_pattern_validate;
}

static void
gimp_param_pattern_init (GParamSpec *pspec)
{
  GimpParamSpecPattern *ispec = GIMP_PARAM_SPEC_PATTERN (pspec);

  ispec->none_ok = FALSE;
}

static gboolean
gimp_param_pattern_validate (GParamSpec *pspec,
                             GValue     *value)
{
  GimpParamSpecPattern *ispec = GIMP_PARAM_SPEC_PATTERN (pspec);
  GimpPattern          *pattern = value->data[0].v_pointer;

  if (! ispec->none_ok && pattern == NULL)
    return TRUE;

  if (pattern && (! GIMP_IS_PATTERN (pattern) ||
                  ! gimp_pattern_is_valid (pattern)))
    {
      g_object_unref (pattern);
      value->data[0].v_pointer = NULL;
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_param_spec_pattern:
 * @name:    Canonical name of the property specified.
 * @nick:    Nick name of the property specified.
 * @blurb:   Description of the property specified.
 * @none_ok: Whether no  is a valid value.
 * @flags:   Flags for the property specified.
 *
 * Creates a new #GimpParamSpecPattern specifying a
 * #GIMP_TYPE_PATTERN property.
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
  GimpParamSpecPattern *ispec;

  ispec = g_param_spec_internal (GIMP_TYPE_PARAM_PATTERN,
                                 name, nick, blurb, flags);

  g_return_val_if_fail (ispec, NULL);

  ispec->none_ok = none_ok ? TRUE : FALSE;

  return G_PARAM_SPEC (ispec);
}
