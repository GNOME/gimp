/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaparamspecs.h
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIBLIGMA_LIGMA_PARAM_SPECS_H__
#define __LIBLIGMA_LIGMA_PARAM_SPECS_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*
 * LIGMA_TYPE_PARAM_IMAGE
 */

#define LIGMA_VALUE_HOLDS_IMAGE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         LIGMA_TYPE_IMAGE))

#define LIGMA_TYPE_PARAM_IMAGE           (ligma_param_image_get_type ())
#define LIGMA_PARAM_SPEC_IMAGE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_IMAGE, LigmaParamSpecImage))
#define LIGMA_IS_PARAM_SPEC_IMAGE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_IMAGE))

typedef struct _LigmaParamSpecImage LigmaParamSpecImage;

struct _LigmaParamSpecImage
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        ligma_param_image_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_image     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_ITEM
 */

#define LIGMA_VALUE_HOLDS_ITEM(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                        LIGMA_TYPE_ITEM))

#define LIGMA_TYPE_PARAM_ITEM           (ligma_param_item_get_type ())
#define LIGMA_PARAM_SPEC_ITEM(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_ITEM, LigmaParamSpecItem))
#define LIGMA_IS_PARAM_SPEC_ITEM(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_ITEM))

typedef struct _LigmaParamSpecItem LigmaParamSpecItem;

struct _LigmaParamSpecItem
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        ligma_param_item_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_item     (const gchar  *name,
                                       const gchar  *nick,
                                       const gchar  *blurb,
                                       gboolean      none_ok,
                                       GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_DRAWABLE
 */

#define LIGMA_VALUE_HOLDS_DRAWABLE(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                            LIGMA_TYPE_DRAWABLE))

#define LIGMA_TYPE_PARAM_DRAWABLE           (ligma_param_drawable_get_type ())
#define LIGMA_PARAM_SPEC_DRAWABLE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_DRAWABLE, LigmaParamSpecDrawable))
#define LIGMA_IS_PARAM_SPEC_DRAWABLE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_DRAWABLE))

typedef struct _LigmaParamSpecDrawable LigmaParamSpecDrawable;

struct _LigmaParamSpecDrawable
{
  LigmaParamSpecItem parent_instance;
};

GType        ligma_param_drawable_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_drawable     (const gchar  *name,
                                           const gchar  *nick,
                                           const gchar  *blurb,
                                           gboolean      none_ok,
                                           GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_LAYER
 */

#define LIGMA_VALUE_HOLDS_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                         LIGMA_TYPE_LAYER))

#define LIGMA_TYPE_PARAM_LAYER           (ligma_param_layer_get_type ())
#define LIGMA_PARAM_SPEC_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_LAYER, LigmaParamSpecLayer))
#define LIGMA_IS_PARAM_SPEC_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_LAYER))

typedef struct _LigmaParamSpecLayer LigmaParamSpecLayer;

struct _LigmaParamSpecLayer
{
  LigmaParamSpecDrawable parent_instance;
};

GType        ligma_param_layer_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_layer     (const gchar  *name,
                                        const gchar  *nick,
                                        const gchar  *blurb,
                                        gboolean      none_ok,
                                        GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_TEXT_LAYER
 */

#define LIGMA_VALUE_HOLDS_TEXT_LAYER(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              LIGMA_TYPE_TEXT_LAYER))

#define LIGMA_TYPE_PARAM_TEXT_LAYER           (ligma_param_text_layer_get_type ())
#define LIGMA_PARAM_SPEC_TEXT_LAYER(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_TEXT_LAYER, LigmaParamSpecTextLayer))
#define LIGMA_IS_PARAM_SPEC_TEXT_LAYER(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_TEXT_LAYER))

typedef struct _LigmaParamSpecTextLayer LigmaParamSpecTextLayer;

struct _LigmaParamSpecTextLayer
{
  LigmaParamSpecLayer parent_instance;
};

GType        ligma_param_text_layer_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_text_layer     (const gchar  *name,
                                             const gchar  *nick,
                                             const gchar  *blurb,
                                             gboolean      none_ok,
                                             GParamFlags   flags);

/*
 * LIGMA_TYPE_PARAM_CHANNEL
 */

#define LIGMA_VALUE_HOLDS_CHANNEL(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           LIGMA_TYPE_CHANNEL))

#define LIGMA_TYPE_PARAM_CHANNEL           (ligma_param_channel_get_type ())
#define LIGMA_PARAM_SPEC_CHANNEL(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_CHANNEL, LigmaParamSpecChannel))
#define LIGMA_IS_PARAM_SPEC_CHANNEL(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_CHANNEL))

typedef struct _LigmaParamSpecChannel LigmaParamSpecChannel;

struct _LigmaParamSpecChannel
{
  LigmaParamSpecDrawable parent_instance;
};

GType        ligma_param_channel_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_channel     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_LAYER_MASK
 */

#define LIGMA_VALUE_HOLDS_LAYER_MASK(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                              LIGMA_TYPE_LAYER_MASK))

#define LIGMA_TYPE_PARAM_LAYER_MASK           (ligma_param_layer_mask_get_type ())
#define LIGMA_PARAM_SPEC_LAYER_MASK(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_LAYER_MASK, LigmaParamSpecLayerMask))
#define LIGMA_IS_PARAM_SPEC_LAYER_MASK(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_LAYER_MASK))

typedef struct _LigmaParamSpecLayerMask LigmaParamSpecLayerMask;

struct _LigmaParamSpecLayerMask
{
  LigmaParamSpecChannel parent_instance;
};

GType        ligma_param_layer_mask_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_layer_mask     (const gchar   *name,
                                             const gchar   *nick,
                                             const gchar   *blurb,
                                             gboolean       none_ok,
                                             GParamFlags    flags);


/*
 * LIGMA_TYPE_PARAM_SELECTION
 */

#define LIGMA_VALUE_HOLDS_SELECTION(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                             LIGMA_TYPE_SELECTION))

#define LIGMA_TYPE_PARAM_SELECTION           (ligma_param_selection_get_type ())
#define LIGMA_PARAM_SPEC_SELECTION(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_SELECTION, LigmaParamSpecSelection))
#define LIGMA_IS_PARAM_SPEC_SELECTION(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_SELECTION))

typedef struct _LigmaParamSpecSelection LigmaParamSpecSelection;

struct _LigmaParamSpecSelection
{
  LigmaParamSpecChannel parent_instance;
};

GType        ligma_param_selection_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_selection     (const gchar   *name,
                                            const gchar   *nick,
                                            const gchar   *blurb,
                                            gboolean       none_ok,
                                            GParamFlags    flags);


/*
 * LIGMA_TYPE_PARAM_VECTORS
 */

#define LIGMA_VALUE_HOLDS_VECTORS(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           LIGMA_TYPE_VECTORS))

#define LIGMA_TYPE_PARAM_VECTORS           (ligma_param_vectors_get_type ())
#define LIGMA_PARAM_SPEC_VECTORS(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_VECTORS, LigmaParamSpecVectors))
#define LIGMA_IS_PARAM_SPEC_VECTORS(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_VECTORS))

typedef struct _LigmaParamSpecVectors LigmaParamSpecVectors;

struct _LigmaParamSpecVectors
{
  LigmaParamSpecItem parent_instance;
};

GType        ligma_param_vectors_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_vectors     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


/*
 * LIGMA_TYPE_PARAM_DISPLAY
 */

#define LIGMA_VALUE_HOLDS_DISPLAY(value)   (G_TYPE_CHECK_VALUE_TYPE ((value),\
                                           LIGMA_TYPE_DISPLAY))

#define LIGMA_TYPE_PARAM_DISPLAY           (ligma_param_display_get_type ())
#define LIGMA_PARAM_SPEC_DISPLAY(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_DISPLAY, LigmaParamSpecDisplay))
#define LIGMA_IS_PARAM_SPEC_DISPLAY(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), LIGMA_TYPE_PARAM_DISPLAY))

typedef struct _LigmaParamSpecDisplay LigmaParamSpecDisplay;

struct _LigmaParamSpecDisplay
{
  GParamSpecObject  parent_instance;

  gboolean          none_ok;
};

GType        ligma_param_display_get_type (void) G_GNUC_CONST;

GParamSpec * ligma_param_spec_display     (const gchar  *name,
                                          const gchar  *nick,
                                          const gchar  *blurb,
                                          gboolean      none_ok,
                                          GParamFlags   flags);


G_END_DECLS

#endif  /*  __LIBLIGMA_LIGMA_PARAM_SPECS_H__  */
