/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 *   gimpvectorlayer.c
 *
 *   Copyright 2006 Hendrik Boom
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#include "path-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable-fill.h"
#include "core/gimpdrawable-stroke.h"
#include "core/gimpimage.h"
#include "core/gimpselection.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpstrokeoptions.h"
#include "core/gimpparasitelist.h"
#include "core/gimprasterizable.h"

#include "gimpvectorlayer.h"
#include "gimpvectorlayeroptions.h"
#include "gimppath.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_VECTOR_LAYER_OPTIONS,
};


/* local function declarations */

static void       gimp_vector_layer_rasterizable_iface_init
                                                    (GimpRasterizableInterface *iface);


static void       gimp_vector_layer_finalize        (GObject                *object);
static void       gimp_vector_layer_get_property    (GObject                *object,
                                                     guint                   property_id,
                                                     GValue                 *value,
                                                     GParamSpec             *pspec);
static void       gimp_vector_layer_set_property    (GObject                *object,
                                                     guint                   property_id,
                                                     const GValue           *value,
                                                     GParamSpec             *pspec);

static void       gimp_vector_layer_set_rasterized  (GimpRasterizable *rasterizable,
                                                     gboolean          rasterized);

static void       gimp_vector_layer_set_vector_options
                                                    (GimpVectorLayer        *layer,
                                                     GimpVectorLayerOptions *options);

static void       gimp_vector_layer_set_buffer      (GimpDrawable           *drawable,
                                                     gboolean                push_undo,
                                                     const gchar            *undo_desc,
                                                     GeglBuffer             *buffer,
                                                     const GeglRectangle    *bounds);
static void       gimp_vector_layer_push_undo       (GimpDrawable           *drawable,
                                                     const gchar            *undo_desc,
                                                     GeglBuffer             *buffer,
                                                     gint                    x,
                                                     gint                    y,
                                                     gint                    width,
                                                     gint                    height);

static gint64     gimp_vector_layer_get_memsize     (GimpObject             *object,
                                                     gint64                 *gui_size);

static GimpItem * gimp_vector_layer_duplicate       (GimpItem               *item,
                                                     GType                   new_type);

static void       gimp_vector_layer_translate       (GimpLayer              *layer,
                                                     gint                    offset_x,
                                                     gint                    offset_y);
static void       gimp_vector_layer_scale           (GimpItem               *item,
                                                     gint                    new_width,
                                                     gint                    new_height,
                                                     gint                    new_offset_x,
                                                     gint                    new_offset_y,
                                                     GimpInterpolationType   interp_type,
                                                     GimpProgress           *progress);
static void       gimp_vector_layer_flip            (GimpItem               *item,
                                                     GimpContext            *context,
                                                     GimpOrientationType     flip_type,
                                                     gdouble                 axis,
                                                     gboolean                clip_result);
static void       gimp_vector_layer_rotate          (GimpItem               *item,
                                                     GimpContext            *context,
                                                     GimpRotationType        rotate_type,
                                                     gdouble                 center_x,
                                                     gdouble                 center_y,
                                                     gboolean                clip_result);
static void       gimp_vector_layer_transform       (GimpItem               *item,
                                                     GimpContext            *context,
                                                     const GimpMatrix3      *matrix,
                                                     GimpTransformDirection  direction,
                                                     GimpInterpolationType   interp_type,
                                                     GimpTransformResize     clip_result,
                                                     GimpProgress           *progress,
                                                     gboolean                push_undo);

static gboolean   gimp_vector_layer_render          (GimpVectorLayer        *layer);
static void       gimp_vector_layer_render_path     (GimpVectorLayer        *layer);
static void       gimp_vector_layer_changed_options (GimpVectorLayer        *layer);

static void       gimp_vector_layer_removed         (GimpItem               *item);

static void       gimp_vector_layer_removed_options_path
                                                    (GimpVectorLayer        *layer);


G_DEFINE_TYPE_WITH_CODE (GimpVectorLayer, gimp_vector_layer, GIMP_TYPE_LAYER,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_RASTERIZABLE,
                                                gimp_vector_layer_rasterizable_iface_init))

#define parent_class gimp_vector_layer_parent_class


static void
gimp_vector_layer_class_init (GimpVectorLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);
  GimpLayerClass    *layer_class       = GIMP_LAYER_CLASS (klass);

  drawable_class->set_buffer        = gimp_vector_layer_set_buffer;
  drawable_class->push_undo         = gimp_vector_layer_push_undo;

  object_class->finalize            = gimp_vector_layer_finalize;
  object_class->set_property        = gimp_vector_layer_set_property;
  object_class->get_property        = gimp_vector_layer_get_property;

  gimp_object_class->get_memsize    = gimp_vector_layer_get_memsize;

  /* TODO: make a custom icon "gimp-vector-layer". */
  viewable_class->default_icon_name = "gimp-tool-path";
  viewable_class->default_name      = _("Vector Layer");

  layer_class->translate            = gimp_vector_layer_translate;

  item_class->removed               = gimp_vector_layer_removed;
  item_class->duplicate             = gimp_vector_layer_duplicate;
  item_class->scale                 = gimp_vector_layer_scale;
  item_class->flip                  = gimp_vector_layer_flip;
  item_class->rotate                = gimp_vector_layer_rotate;
  item_class->transform             = gimp_vector_layer_transform;
  item_class->rename_desc           = _("Rename Vector Layer");
  item_class->translate_desc        = _("Move Vector Layer");
  item_class->scale_desc            = _("Scale Vector Layer");
  item_class->resize_desc           = _("Resize Vector Layer");
  item_class->flip_desc             = _("Flip Vector Layer");
  item_class->rotate_desc           = _("Rotate Vector Layer");
  item_class->transform_desc        = _("Transform Vector Layer");

  GIMP_CONFIG_PROP_OBJECT (object_class, PROP_VECTOR_LAYER_OPTIONS,
                           "vector-layer-options", NULL, NULL,
                           GIMP_TYPE_VECTOR_LAYER_OPTIONS,
                           G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
}

static void
gimp_vector_layer_init (GimpVectorLayer *layer)
{
  layer->options  = NULL;
}

static void
gimp_vector_layer_rasterizable_iface_init (GimpRasterizableInterface *iface)
{
  iface->set_rasterized = gimp_vector_layer_set_rasterized;
}

static void
gimp_vector_layer_finalize (GObject *object)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (object);

  if (layer->options)
    {
      g_object_unref (layer->options);
      layer->options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vector_layer_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      g_value_set_object (value, vector_layer->options);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);

  switch (property_id)
    {
    case PROP_VECTOR_LAYER_OPTIONS:
      gimp_vector_layer_set_vector_options (vector_layer, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_vector_layer_set_rasterized (GimpRasterizable *rasterizable,
                                  gboolean          rasterized)
{
  if (! rasterized)
    gimp_vector_layer_render (GIMP_VECTOR_LAYER (rasterizable));
}

static void
gimp_vector_layer_set_vector_options (GimpVectorLayer        *layer,
                                      GimpVectorLayerOptions *options)
{
  if (layer->options)
    {
      g_signal_handlers_disconnect_by_func (layer->options,
                                            G_CALLBACK (gimp_vector_layer_changed_options),
                                            layer);

      if (layer->options->path)
        g_signal_handlers_disconnect_by_func (layer->options->path,
                                              G_CALLBACK (gimp_vector_layer_removed_options_path),
                                              layer);

      g_object_unref (layer->options);
      layer->options = NULL;
    }

  if (options)
    g_set_object (&layer->options, options);

  gimp_vector_layer_changed_options (layer);

  if (layer->options)
    {
      if (layer->options->path)
        g_signal_connect_object (layer->options->path, "removed",
                                 G_CALLBACK (gimp_vector_layer_removed_options_path),
                                 layer, G_CONNECT_SWAPPED);

      g_signal_connect_object (layer->options, "notify",
                               G_CALLBACK (gimp_vector_layer_changed_options),
                               layer, G_CONNECT_SWAPPED);
    }

  g_object_notify (G_OBJECT (layer), "vector-layer-options");
}

static void
gimp_vector_layer_set_buffer (GimpDrawable        *drawable,
                              gboolean             push_undo,
                              const gchar         *undo_desc,
                              GeglBuffer          *buffer,
                              const GeglRectangle *bounds)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (drawable);
  GimpImage       *image = gimp_item_get_image (GIMP_ITEM (layer));
  gboolean         is_rasterized;

  is_rasterized = gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (layer));

  if (push_undo && ! is_rasterized)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE_MOD,
                                 undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  if (push_undo && ! is_rasterized)
    {
      gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (layer));
      gimp_image_undo_group_end (image);
    }
}

static void
gimp_vector_layer_push_undo (GimpDrawable *drawable,
                             const gchar  *undo_desc,
                             GeglBuffer   *buffer,
                             gint          x,
                             gint          y,
                             gint          width,
                             gint          height)
{
  GimpVectorLayer *layer = GIMP_VECTOR_LAYER (drawable);
  GimpImage       *image = gimp_item_get_image (GIMP_ITEM (layer));
  gboolean         is_rasterized;

  is_rasterized = gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (layer));

  if (! is_rasterized)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_DRAWABLE, undo_desc);

  GIMP_DRAWABLE_CLASS (parent_class)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);

  if (! is_rasterized)
    {
      gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (layer));
      gimp_image_undo_group_end (image);
    }
}

static gint64
gimp_vector_layer_get_memsize (GimpObject *object,
                               gint64     *gui_size)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (object);
  gint64           memsize      = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (vector_layer->options),
                                      gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_vector_layer_duplicate (GimpItem *item,
                             GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_VECTOR_LAYER (new_item))
    {
      GimpVectorLayer *vector_layer     = GIMP_VECTOR_LAYER (item);
      GimpVectorLayer *new_vector_layer = GIMP_VECTOR_LAYER (new_item);

      if (vector_layer->options)
        {
          GimpVectorLayerOptions *new_options =
            gimp_config_duplicate (GIMP_CONFIG (vector_layer->options));

          if (vector_layer->options->path)
            {
              GimpPath *path = gimp_vector_layer_get_path (vector_layer);
              GimpPath *new_path;

              new_path = GIMP_PATH (gimp_item_duplicate (GIMP_ITEM (path),
                                                         G_TYPE_FROM_INSTANCE (GIMP_ITEM (path))));

              g_object_set (new_options, "path", new_path, NULL);
            }

          g_object_set (new_vector_layer,
                        "vector-layer-options", new_options,
                        NULL);
          g_object_unref (new_options);
        }

      if (gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (vector_layer)))
        gimp_rasterizable_set_undo_rasterized (GIMP_RASTERIZABLE (new_vector_layer), TRUE);
    }

  return new_item;
}

static void
gimp_vector_layer_translate (GimpLayer *layer,
                             gint       offset_x,
                             gint       offset_y)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (layer);

  if (vector_layer->options && vector_layer->options->path)
    {
      gimp_item_translate (GIMP_ITEM (vector_layer->options->path),
                           offset_x, offset_y, FALSE);
    }
  else
    {
      GIMP_LAYER_CLASS (parent_class)->translate (layer, offset_x, offset_y);
    }
}

static void
gimp_vector_layer_scale (GimpItem              *item,
                         gint                   new_width,
                         gint                   new_height,
                         gint                   new_offset_x,
                         gint                   new_offset_y,
                         GimpInterpolationType  interp_type,
                         GimpProgress          *progress)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    {
      gimp_item_scale (GIMP_ITEM (vector_layer->options->path),
                       new_width, new_height, new_offset_x, new_offset_y,
                       interp_type, progress);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                             new_offset_x, new_offset_y,
                                             interp_type, progress);
    }
}

static void
gimp_vector_layer_flip (GimpItem            *item,
                        GimpContext         *context,
                        GimpOrientationType  flip_type,
                        gdouble              axis,
                        gboolean             clip_result)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    {
      gimp_item_flip (GIMP_ITEM (vector_layer->options->path),
                      context, flip_type, axis, clip_result);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->flip (item, context, flip_type,
                                            axis, clip_result);
    }
}

static void
gimp_vector_layer_rotate (GimpItem         *item,
                          GimpContext      *context,
                          GimpRotationType  rotate_type,
                          gdouble           center_x,
                          gdouble           center_y,
                          gboolean          clip_result)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    {
      gimp_item_rotate (GIMP_ITEM (vector_layer->options->path),
                        context, rotate_type, center_x, center_y, clip_result);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->rotate (item, context, rotate_type,
                                              center_x, center_y, clip_result);
    }
}

static void
gimp_vector_layer_transform (GimpItem               *item,
                             GimpContext            *context,
                             const GimpMatrix3      *matrix,
                             GimpTransformDirection  direction,
                             GimpInterpolationType   interp_type,
                             GimpTransformResize     clip_result,
                             GimpProgress           *progress,
                             gboolean                push_undo)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    {
      gimp_item_transform (GIMP_ITEM (vector_layer->options->path),
                           context, matrix, direction, interp_type,
                           clip_result, progress);
    }
  else
    {
      GIMP_ITEM_CLASS (parent_class)->transform (item, context, matrix,
                                                 direction, interp_type,
                                                 clip_result, progress, push_undo);
    }
}

static void
gimp_vector_layer_removed_options_path (GimpVectorLayer *layer)
{
  if (layer->options)
    {
      gimp_image_undo_push_vector_layer (gimp_item_get_image (GIMP_ITEM (layer)),
                                         _("Discard Vector Information"),
                                         layer, NULL);

      g_object_set (layer->options, "path", NULL, NULL);
    }
}

static void
gimp_vector_layer_removed (GimpItem *item)
{
  GimpVectorLayer *vector_layer = GIMP_VECTOR_LAYER (item);

  if (vector_layer->options && vector_layer->options->path)
    g_signal_handlers_disconnect_by_func (vector_layer->options->path,
                                          G_CALLBACK (gimp_vector_layer_removed_options_path),
                                          vector_layer);

  GIMP_ITEM_CLASS (parent_class)->removed (item);
}


/*  public functions  */

/**
 * gimp_vector_layer_new:
 * @image:   the #GimpImage the layer should belong to
 * @path:    the #GimpPath object the layer should render
 * @context: the #GimpContext from which to pull context properties
 *
 * Creates a new vector layer.
 *
 * Return value: a new #GimpVectorLayer or %NULL in case of a problem
 **/
GimpVectorLayer *
gimp_vector_layer_new (GimpImage   *image,
                       GimpPath    *path,
                       GimpContext *context)
{
  GimpVectorLayer        *layer;
  GimpVectorLayerOptions *options;
  gint                    x      = 0;
  gint                    y      = 0;
  gint                    width  = gimp_image_get_width (image);
  gint                    height = gimp_image_get_height (image);

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (GIMP_IS_PATH (path), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  options = gimp_vector_layer_options_new (image, path, context);

  gimp_item_bounds (GIMP_ITEM (path), &x, &y, &width, &height);

  /* Set boundaries to image size if it's a blank path */
  if (width == 0 || height == 0)
    {
      width  = gimp_image_get_width (image);
      height = gimp_image_get_height (image);
    }

  layer =
    GIMP_VECTOR_LAYER (gimp_drawable_new (GIMP_TYPE_VECTOR_LAYER,
                                          image, NULL,
                                          x, y, width, height,
                                          gimp_image_get_layer_format (image,
                                                                       TRUE)));

  gimp_object_set_name (GIMP_OBJECT (layer),
                        gimp_object_get_name (GIMP_OBJECT (path)));

  gimp_layer_set_mode (GIMP_LAYER (layer),
                       gimp_image_get_default_new_layer_mode (image),
                       FALSE);

  gimp_vector_layer_set_vector_options (layer, options);
  g_object_unref (options);

  return layer;
}

/**
 * gimp_vector_layer_get_path:
 * @layer: a #GimpVectorLayer
 *
 * Gets the path from @layer if one is associated with it.
 *
  * Return value: a #GimpPath or %NULL if no path is set.
 */
GimpPath *
gimp_vector_layer_get_path (GimpVectorLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER (layer), NULL);

  return layer->options->path;
}

/**
 * gimp_vector_layer_get_options:
 * @layer: a #GimpVectorLayer
 *
 * Gets the vector layer options from @layer if one is associated with it.
 *
  * Return value: a #GimpVectorLayerOptions or %NULL if no options are set.
 */
GimpVectorLayerOptions *
gimp_vector_layer_get_options (GimpVectorLayer *layer)
{
  g_return_val_if_fail (GIMP_IS_VECTOR_LAYER (layer), NULL);

  if (gimp_item_is_vector_layer (GIMP_ITEM (layer)))
    return layer->options;

  return NULL;
}

void
gimp_vector_layer_refresh (GimpVectorLayer *layer)
{
  if (layer->options)
    gimp_vector_layer_render (layer);
}

gboolean
gimp_item_is_vector_layer (GimpItem *item)
{
  return (GIMP_IS_VECTOR_LAYER (item) &&
          ! gimp_rasterizable_is_rasterized (GIMP_RASTERIZABLE (item)));
}


/* private functions  */

static gboolean
gimp_vector_layer_render (GimpVectorLayer *layer)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (layer);
  GeglBuffer   *buffer   = NULL;
  GimpItem     *item     = GIMP_ITEM (layer);
  GimpImage    *image    = gimp_item_get_image (item);
  gint          layer_x  = 0;
  gint          layer_y  = 0;
  gint          x        = 0;
  gint          y        = 0;
  gint          width    = gimp_image_get_width (image);
  gint          height   = gimp_image_get_height (image);
  gdouble       stroke   = 0;

  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);

  g_object_freeze_notify (G_OBJECT (drawable));

  if (layer->options->enable_stroke)
    stroke = gimp_stroke_options_get_width (layer->options->stroke_options);

  /* Resize layer according to path size */
  gimp_item_get_offset (GIMP_ITEM (layer), &layer_x, &layer_y);
  gimp_item_bounds (GIMP_ITEM (layer->options->path), &x, &y, &width, &height);

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            (gint) ceil (width + stroke),
                                            (gint) ceil (height + stroke)),
                            gimp_drawable_get_format (drawable));
  gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  gimp_item_set_offset (GIMP_ITEM (layer), x - (stroke / 2), y - (stroke / 2));

  /* make the layer background transparent */
  gimp_drawable_fill (GIMP_DRAWABLE (layer),
                      gimp_get_user_context (image->gimp),
                      GIMP_FILL_TRANSPARENT);

  /* render path to the layer */
  gimp_vector_layer_render_path (layer);

  g_object_thaw_notify (G_OBJECT (drawable));

  return TRUE;
}

static void
gimp_vector_layer_render_path (GimpVectorLayer *layer)
{
  GimpImage              *image     = gimp_item_get_image (GIMP_ITEM (layer));
  GimpVectorLayerOptions *options   = layer->options;
  GimpPath               *path      = NULL;
  GimpChannel            *selection = gimp_image_get_mask (image);
  GList                  *drawables;
  GimpCustomStyle         style;

  if (options)
    path = options->path;

  /* Don't mask these fill/stroke operations  */
  gimp_selection_suspend (GIMP_SELECTION (selection));

  /* Convert from custom to standard styles */
  style = gimp_fill_options_get_custom_style (options->fill_options);
  if (style == GIMP_CUSTOM_STYLE_SOLID_COLOR ||
      ! gimp_context_get_pattern (GIMP_CONTEXT (options->fill_options)))
    gimp_fill_options_set_style (options->fill_options,
                                 GIMP_FILL_STYLE_FG_COLOR);
  else
    gimp_fill_options_set_style (options->fill_options,
                                 GIMP_FILL_STYLE_PATTERN);

  style =
    gimp_fill_options_get_custom_style (GIMP_FILL_OPTIONS (options->stroke_options));
  if (style == GIMP_CUSTOM_STYLE_SOLID_COLOR ||
      ! gimp_context_get_pattern (GIMP_CONTEXT (options->stroke_options)))
    gimp_fill_options_set_style (GIMP_FILL_OPTIONS (options->stroke_options),
                                 GIMP_FILL_STYLE_FG_COLOR);
  else
    gimp_fill_options_set_style (GIMP_FILL_OPTIONS (options->stroke_options),
                                 GIMP_FILL_STYLE_PATTERN);

  /* Fill the path object onto the layer */
  if (options->enable_fill)
    gimp_drawable_fill_path (GIMP_DRAWABLE (layer),
                             options->fill_options,
                             path, FALSE, NULL);

  drawables = g_list_prepend (NULL, GIMP_DRAWABLE (layer));
  /* stroke the path object onto the layer */
  if (options->enable_stroke && gimp_item_is_attached (GIMP_ITEM (path)))
    gimp_item_stroke (GIMP_ITEM (path), drawables,
                      gimp_get_user_context (image->gimp),
                      options->stroke_options,
                      FALSE, FALSE,
                      NULL, NULL);

  g_list_free (drawables);

  gimp_selection_resume (GIMP_SELECTION (selection));
}

static void
gimp_vector_layer_changed_options (GimpVectorLayer *layer)
{
  GimpItem *item = GIMP_ITEM (layer);

  if (layer->options && ! layer->options->path)
    gimp_rasterizable_rasterize (GIMP_RASTERIZABLE (layer));
  else if (gimp_item_is_attached (item))
    gimp_vector_layer_refresh (layer);
}
