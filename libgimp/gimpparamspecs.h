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

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __LIBGIMP_GIMP_PARAM_SPECS_H__
#define __LIBGIMP_GIMP_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * GIMP_TYPE_PARAM_IMAGE
 */

#define GIMP_VALUE_HOLDS_IMAGE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         GIMP_TYPE_IMAGE))

#define GIMP_TYPE_PARAM_IMAGE           (gimp_param_image_get_type ())
#define GIMP_PARAM_SPEC_IMAGE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_IMAGE, GimpParamSpecImage))
#define GIMP_IS_PARAM_SPEC_IMAGE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_IMAGE))

typedef struct _GimpParamSpecImage GimpParamSpecImage;

struct _GimpParamSpecImage
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_image_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_image     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_ITEM
 */

#define GIMP_VALUE_HOLDS_ITEM(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        GIMP_TYPE_ITEM))

#define GIMP_TYPE_PARAM_ITEM           (gimp_param_item_get_type ())
#define GIMP_PARAM_SPEC_ITEM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_ITEM, GimpParamSpecItem))
#define GIMP_IS_PARAM_SPEC_ITEM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_ITEM))

typedef struct _GimpParamSpecItem GimpParamSpecItem;

struct _GimpParamSpecItem
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_item_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_item     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_DRAWABLE
 */

#define GIMP_VALUE_HOLDS_DRAWABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            GIMP_TYPE_DRAWABLE))

#define GIMP_TYPE_PARAM_DRAWABLE           (gimp_param_drawable_get_type ())
#define GIMP_PARAM_SPEC_DRAWABLE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DRAWABLE, GimpParamSpecDrawable))
#define GIMP_IS_PARAM_SPEC_DRAWABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DRAWABLE))

typedef struct _GimpParamSpecDrawable GimpParamSpecDrawable;

struct _GimpParamSpecDrawable
{
  GimpParamSpecItem parent_instance;
};

GType        gimp_param_drawable_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_drawable     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_LAYER
 */

#define GIMP_VALUE_HOLDS_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         GIMP_TYPE_LAYER))

#define GIMP_TYPE_PARAM_LAYER           (gimp_param_layer_get_type ())
#define GIMP_PARAM_SPEC_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER, GimpParamSpecLayer))
#define GIMP_IS_PARAM_SPEC_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER))

typedef struct _GimpParamSpecLayer GimpParamSpecLayer;

struct _GimpParamSpecLayer
{
  GimpParamSpecDrawable parent_instance;
};

GType        gimp_param_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_TEXT_LAYER
 */

#define GIMP_VALUE_HOLDS_TEXT_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              GIMP_TYPE_TEXT_LAYER))

#define GIMP_TYPE_PARAM_TEXT_LAYER           (gimp_param_text_layer_get_type ())
#define GIMP_PARAM_SPEC_TEXT_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_TEXT_LAYER, GimpParamSpecTextLayer))
#define GIMP_IS_PARAM_SPEC_TEXT_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_TEXT_LAYER))

typedef struct _GimpParamSpecTextLayer GimpParamSpecTextLayer;

struct _GimpParamSpecTextLayer
{
  GimpParamSpecLayer parent_instance;
};

GType        gimp_param_text_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_text_layer     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             gboolean      none_ok,
                                             GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_GROUP_LAYER
 */

#define GIMP_VALUE_HOLDS_GROUP_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                               GIMP_TYPE_GROUP_LAYER))

#define GIMP_TYPE_PARAM_GROUP_LAYER           (gimp_param_group_layer_get_type ())
#define GIMP_PARAM_SPEC_GROUP_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_GROUP_LAYER, GimpParamSpecGroupLayer))
#define GIMP_IS_PARAM_SPEC_GROUP_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_GROUP_LAYER))

typedef struct _GimpParamSpecGroupLayer GimpParamSpecGroupLayer;

struct _GimpParamSpecGroupLayer
{
  GimpParamSpecLayer parent_instance;
};

GType        gimp_param_group_layer_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_group_layer     (const gchar  *name,
                                              const gchar  *nick,
                                              const gchar  *blurb,
                                              gboolean      none_ok,
                                              GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_CHANNEL
 */

#define GIMP_VALUE_HOLDS_CHANNEL(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_CHANNEL))

#define GIMP_TYPE_PARAM_CHANNEL           (gimp_param_channel_get_type ())
#define GIMP_PARAM_SPEC_CHANNEL(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_CHANNEL, GimpParamSpecChannel))
#define GIMP_IS_PARAM_SPEC_CHANNEL(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_CHANNEL))

typedef struct _GimpParamSpecChannel GimpParamSpecChannel;

struct _GimpParamSpecChannel
{
  GimpParamSpecDrawable parent_instance;
};

GType        gimp_param_channel_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_channel     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_LAYER_MASK
 */

#define GIMP_VALUE_HOLDS_LAYER_MASK(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              GIMP_TYPE_LAYER_MASK))

#define GIMP_TYPE_PARAM_LAYER_MASK           (gimp_param_layer_mask_get_type ())
#define GIMP_PARAM_SPEC_LAYER_MASK(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_LAYER_MASK, GimpParamSpecLayerMask))
#define GIMP_IS_PARAM_SPEC_LAYER_MASK(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_LAYER_MASK))

typedef struct _GimpParamSpecLayerMask GimpParamSpecLayerMask;

struct _GimpParamSpecLayerMask
{
  GimpParamSpecChannel parent_instance;
};

GType        gimp_param_layer_mask_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_layer_mask     (const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags);


/*
 * GIMP_TYPE_PARAM_SELECTION
 */

#define GIMP_VALUE_HOLDS_SELECTION(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                             GIMP_TYPE_SELECTION))

#define GIMP_TYPE_PARAM_SELECTION           (gimp_param_selection_get_type ())
#define GIMP_PARAM_SPEC_SELECTION(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_SELECTION, GimpParamSpecSelection))
#define GIMP_IS_PARAM_SPEC_SELECTION(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_SELECTION))

typedef struct _GimpParamSpecSelection GimpParamSpecSelection;

struct _GimpParamSpecSelection
{
  GimpParamSpecChannel parent_instance;
};

GType        gimp_param_selection_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_selection     (const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags);


/*
 * GIMP_TYPE_PARAM_PATH
 */

#define GIMP_VALUE_HOLDS_PATH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        GIMP_TYPE_PATH))

#define GIMP_TYPE_PARAM_PATH           (gimp_param_path_get_type ())
#define GIMP_PARAM_SPEC_PATH(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PATH, GimpParamSpecPath))
#define GIMP_IS_PARAM_SPEC_PATH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATH))

typedef struct _GimpParamSpecPath GimpParamSpecPath;

struct _GimpParamSpecPath
{
  GimpParamSpecItem parent_instance;
};

GType        gimp_param_path_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_path     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_DISPLAY
 */

#define GIMP_VALUE_HOLDS_DISPLAY(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           GIMP_TYPE_DISPLAY))

#define GIMP_TYPE_PARAM_DISPLAY           (gimp_param_display_get_type ())
#define GIMP_PARAM_SPEC_DISPLAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_DISPLAY, GimpParamSpecDisplay))
#define GIMP_IS_PARAM_SPEC_DISPLAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_DISPLAY))

typedef struct _GimpParamSpecDisplay GimpParamSpecDisplay;

struct _GimpParamSpecDisplay
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_display_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_display     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_RESOURCE
 */

#define GIMP_VALUE_HOLDS_RESOURCE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                            GIMP_TYPE_RESOURCE))

#define GIMP_TYPE_PARAM_RESOURCE           (gimp_param_resource_get_type ())
#define GIMP_PARAM_SPEC_RESOURCE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_RESOURCE, GimpParamSpecResource))
#define GIMP_IS_PARAM_SPEC_RESOURCE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_RESOURCE))

typedef struct _GimpParamSpecResource GimpParamSpecResource;

struct _GimpParamSpecResource
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        gimp_param_resource_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_resource     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_BRUSH
 */

#define GIMP_VALUE_HOLDS_BRUSH(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                         GIMP_TYPE_BRUSH))

#define GIMP_TYPE_PARAM_BRUSH           (gimp_param_brush_get_type ())
#define GIMP_PARAM_SPEC_BRUSH(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_BRUSH, GimpParamSpecBrush))
#define GIMP_IS_PARAM_SPEC_BRUSH(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_BRUSH))

typedef struct _GimpParamSpecBrush GimpParamSpecBrush;

struct _GimpParamSpecBrush
{
  GimpParamSpecResource  parent_instance;
};

GType        gimp_param_brush_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_brush (const gchar  *name,
                                    const gchar  *nick,
                                    const gchar  *blurb,
                                    gboolean      none_ok,
                                    GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_PATTERN
 */

#define GIMP_VALUE_HOLDS_PATTERN(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           GIMP_TYPE_PATTERN))

#define GIMP_TYPE_PARAM_PATTERN           (gimp_param_pattern_get_type ())
#define GIMP_PARAM_SPEC_PATTERN(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PATTERN, GimpParamSpecPattern))
#define GIMP_IS_PARAM_SPEC_PATTERN(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PATTERN))

typedef struct _GimpParamSpecPattern GimpParamSpecPattern;

struct _GimpParamSpecPattern
{
  GimpParamSpecResource  parent_instance;
};

GType        gimp_param_pattern_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_pattern (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_GRADIENT
 */

#define GIMP_VALUE_HOLDS_GRADIENT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                            GIMP_TYPE_GRADIENT))

#define GIMP_TYPE_PARAM_GRADIENT           (gimp_param_gradient_get_type ())
#define GIMP_PARAM_SPEC_GRADIENT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_GRADIENT, GimpParamSpecGradient))
#define GIMP_IS_PARAM_SPEC_GRADIENT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_GRADIENT))

typedef struct _GimpParamSpecGradient GimpParamSpecGradient;

struct _GimpParamSpecGradient
{
  GimpParamSpecResource  parent_instance;
};

GType        gimp_param_gradient_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_gradient (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_PALETTE
 */

#define GIMP_VALUE_HOLDS_PALETTE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                           GIMP_TYPE_PALETTE))

#define GIMP_TYPE_PARAM_PALETTE           (gimp_param_palette_get_type ())
#define GIMP_PARAM_SPEC_PALETTE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_PALETTE, GimpParamSpecPalette))
#define GIMP_IS_PARAM_SPEC_PALETTE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_PALETTE))

typedef struct _GimpParamSpecPalette GimpParamSpecPalette;

struct _GimpParamSpecPalette
{
  GimpParamSpecResource  parent_instance;
};

GType        gimp_param_palette_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_palette (const gchar  *name,
                                      const gchar  *nick,
                                      const gchar  *blurb,
                                      gboolean      none_ok,
                                      GParamFlags   flags);


/*
 * GIMP_TYPE_PARAM_FONT
 */

#define GIMP_VALUE_HOLDS_FONT(value)   (G_TYPE_CHECK_VALUE_TYPE ((value), \
                                        GIMP_TYPE_FONT))

#define GIMP_TYPE_PARAM_FONT           (gimp_param_font_get_type ())
#define GIMP_PARAM_SPEC_FONT(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), GIMP_TYPE_PARAM_FONT, GimpParamSpecFont))
#define GIMP_IS_PARAM_SPEC_FONT(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GIMP_TYPE_PARAM_FONT))

typedef struct _GimpParamSpecFont GimpParamSpecFont;

struct _GimpParamSpecFont
{
  GimpParamSpecResource  parent_instance;
};

GType        gimp_param_font_get_type (void) G_GNUC_CONST;

GParamSpec * gimp_param_spec_font (const gchar  *name,
                                   const gchar  *nick,
                                   const gchar  *blurb,
                                   gboolean      none_ok,
                                   GParamFlags   flags);


G_END_DECLS

#endif  /*  __LIBGIMP_GIMP_PARAM_SPECS_H__  */
