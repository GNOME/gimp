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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"
#include "gegl/gimptilehandlervalidate.h"

#include "gimp-memsize.h"
#include "gimp-utils.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawable-fill.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-floating-selection.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimpdrawable-shadow.h"
#include "gimpdrawable-transform.h"
#include "gimpdrawablefilter.h"
#include "gimpfilterstack.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo-push.h"
#include "gimpmarshal.h"
#include "gimppickable.h"
#include "gimpprogress.h"

#include "gimp-log.h"

#include "gimp-intl.h"


#define PAINT_UPDATE_CHUNK_WIDTH  32
#define PAINT_UPDATE_CHUNK_HEIGHT 32


enum
{
  UPDATE,
  FORMAT_CHANGED,
  ALPHA_CHANGED,
  BOUNDING_BOX_CHANGED,
  FILTERS_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void       gimp_color_managed_iface_init    (GimpColorManagedInterface *iface);
static void       gimp_pickable_iface_init         (GimpPickableInterface     *iface);

static void       gimp_drawable_dispose            (GObject           *object);
static void       gimp_drawable_finalize           (GObject           *object);
static void       gimp_drawable_set_property       (GObject           *object,
                                                    guint              property_id,
                                                    const GValue      *value,
                                                    GParamSpec        *pspec);
static void       gimp_drawable_get_property       (GObject           *object,
                                                    guint              property_id,
                                                    GValue            *value,
                                                    GParamSpec        *pspec);

static gint64     gimp_drawable_get_memsize        (GimpObject        *object,
                                                    gint64            *gui_size);

static void       gimp_drawable_size_changed       (GimpViewable      *viewable);
static gboolean   gimp_drawable_get_size           (GimpViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void       gimp_drawable_preview_freeze     (GimpViewable      *viewable);
static void       gimp_drawable_preview_thaw       (GimpViewable      *viewable);

static GeglNode * gimp_drawable_get_node           (GimpFilter        *filter);

static void       gimp_drawable_removed            (GimpItem          *item);
static GimpItem * gimp_drawable_duplicate          (GimpItem          *item,
                                                    GType              new_type);
static void       gimp_drawable_scale              (GimpItem          *item,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               new_offset_x,
                                                    gint               new_offset_y,
                                                    GimpInterpolationType interp_type,
                                                    GimpProgress      *progress);
static void       gimp_drawable_resize             (GimpItem          *item,
                                                    GimpContext       *context,
                                                    GimpFillType       fill_type,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               offset_x,
                                                    gint               offset_y);
static void       gimp_drawable_flip               (GimpItem          *item,
                                                    GimpContext       *context,
                                                    GimpOrientationType  flip_type,
                                                    gdouble            axis,
                                                    gboolean           clip_result);
static void       gimp_drawable_rotate             (GimpItem          *item,
                                                    GimpContext       *context,
                                                    GimpRotationType   rotate_type,
                                                    gdouble            center_x,
                                                    gdouble            center_y,
                                                    gboolean           clip_result);
static void       gimp_drawable_transform          (GimpItem          *item,
                                                    GimpContext       *context,
                                                    const GimpMatrix3 *matrix,
                                                    GimpTransformDirection direction,
                                                    GimpInterpolationType interpolation_type,
                                                    GimpTransformResize clip_result,
                                                    GimpProgress      *progress,
                                                    gboolean           push_undo);

static const guint8 *
                  gimp_drawable_get_icc_profile    (GimpColorManaged  *managed,
                                                    gsize             *len);
static GimpColorProfile *
                  gimp_drawable_get_color_profile  (GimpColorManaged  *managed);
static void       gimp_drawable_profile_changed    (GimpColorManaged  *managed);

static gboolean   gimp_drawable_get_pixel_at       (GimpPickable      *pickable,
                                                    gint               x,
                                                    gint               y,
                                                    const Babl        *format,
                                                    gpointer           pixel);
static void       gimp_drawable_get_pixel_average  (GimpPickable      *pickable,
                                                    const GeglRectangle *rect,
                                                    const Babl        *format,
                                                    gpointer           pixel);

static void       gimp_drawable_real_update        (GimpDrawable      *drawable,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

static void    gimp_drawable_real_filters_changed  (GimpDrawable      *drawable);
static gint64  gimp_drawable_real_estimate_memsize (GimpDrawable      *drawable,
                                                    GimpComponentType  component_type,
                                                    gint               width,
                                                    gint               height);

static void       gimp_drawable_real_update_all    (GimpDrawable      *drawable);

static GimpComponentMask
                gimp_drawable_real_get_active_mask (GimpDrawable      *drawable);

static gboolean   gimp_drawable_real_supports_alpha
                                                   (GimpDrawable     *drawable);

static void       gimp_drawable_real_convert_type  (GimpDrawable      *drawable,
                                                    GimpImage         *dest_image,
                                                    const Babl        *new_format,
                                                    GimpColorProfile  *src_profile,
                                                    GimpColorProfile  *dest_profile,
                                                    GeglDitherMethod   layer_dither_type,
                                                    GeglDitherMethod   mask_dither_type,
                                                    gboolean           push_undo,
                                                    GimpProgress      *progress);

static GeglBuffer * gimp_drawable_real_get_buffer  (GimpDrawable      *drawable);
static void       gimp_drawable_real_set_buffer    (GimpDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    const GeglRectangle *bounds);

static GeglRectangle gimp_drawable_real_get_bounding_box
                                                   (GimpDrawable      *drawable);

static void       gimp_drawable_real_push_undo     (GimpDrawable      *drawable,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);
static void       gimp_drawable_real_swap_pixels   (GimpDrawable      *drawable,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y);
static GeglNode * gimp_drawable_real_get_source_node (GimpDrawable    *drawable);

static void       gimp_drawable_format_changed     (GimpDrawable      *drawable);
static void       gimp_drawable_alpha_changed      (GimpDrawable      *drawable);


G_DEFINE_TYPE_WITH_CODE (GimpDrawable, gimp_drawable, GIMP_TYPE_ITEM,
                         G_ADD_PRIVATE (GimpDrawable)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_COLOR_MANAGED,
                                                gimp_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_pickable_iface_init))

#define parent_class gimp_drawable_parent_class

static guint gimp_drawable_signals[LAST_SIGNAL] = { 0 };


static void
gimp_drawable_class_init (GimpDrawableClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpFilterClass   *filter_class      = GIMP_FILTER_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);

  gimp_drawable_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, update),
                  NULL, NULL,
                  gimp_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  gimp_drawable_signals[FORMAT_CHANGED] =
    g_signal_new ("format-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, format_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_drawable_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_drawable_signals[BOUNDING_BOX_CHANGED] =
    g_signal_new ("bounding-box-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, bounding_box_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gimp_drawable_signals[FILTERS_CHANGED] =
    g_signal_new ("filters-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, filters_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose           = gimp_drawable_dispose;
  object_class->finalize          = gimp_drawable_finalize;
  object_class->set_property      = gimp_drawable_set_property;
  object_class->get_property      = gimp_drawable_get_property;

  gimp_object_class->get_memsize  = gimp_drawable_get_memsize;

  viewable_class->size_changed    = gimp_drawable_size_changed;
  viewable_class->get_size        = gimp_drawable_get_size;
  viewable_class->get_new_preview = gimp_drawable_get_new_preview;
  viewable_class->get_new_pixbuf  = gimp_drawable_get_new_pixbuf;
  viewable_class->preview_freeze  = gimp_drawable_preview_freeze;
  viewable_class->preview_thaw    = gimp_drawable_preview_thaw;

  filter_class->get_node          = gimp_drawable_get_node;

  item_class->removed             = gimp_drawable_removed;
  item_class->duplicate           = gimp_drawable_duplicate;
  item_class->scale               = gimp_drawable_scale;
  item_class->resize              = gimp_drawable_resize;
  item_class->flip                = gimp_drawable_flip;
  item_class->rotate              = gimp_drawable_rotate;
  item_class->transform           = gimp_drawable_transform;

  klass->update                   = gimp_drawable_real_update;
  klass->format_changed           = NULL;
  klass->alpha_changed            = NULL;
  klass->bounding_box_changed     = NULL;
  klass->filters_changed          = gimp_drawable_real_filters_changed;
  klass->estimate_memsize         = gimp_drawable_real_estimate_memsize;
  klass->update_all               = gimp_drawable_real_update_all;
  klass->invalidate_boundary      = NULL;
  klass->get_active_components    = NULL;
  klass->get_active_mask          = gimp_drawable_real_get_active_mask;
  klass->supports_alpha           = gimp_drawable_real_supports_alpha;
  klass->convert_type             = gimp_drawable_real_convert_type;
  klass->apply_buffer             = gimp_drawable_real_apply_buffer;
  klass->get_buffer               = gimp_drawable_real_get_buffer;
  klass->set_buffer               = gimp_drawable_real_set_buffer;
  klass->get_bounding_box         = gimp_drawable_real_get_bounding_box;
  klass->push_undo                = gimp_drawable_real_push_undo;
  klass->swap_pixels              = gimp_drawable_real_swap_pixels;
  klass->get_source_node          = gimp_drawable_real_get_source_node;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->private = gimp_drawable_get_instance_private (drawable);

  _gimp_drawable_filters_init (drawable);
}

/* sorry for the evil casts */

static void
gimp_color_managed_iface_init (GimpColorManagedInterface *iface)
{
  iface->get_icc_profile   = gimp_drawable_get_icc_profile;
  iface->get_color_profile = gimp_drawable_get_color_profile;
  iface->profile_changed   = gimp_drawable_profile_changed;
}

static void
gimp_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image               = (GimpImage     * (*) (GimpPickable *pickable)) gimp_item_get_image;
  iface->get_format              = (const Babl    * (*) (GimpPickable *pickable)) gimp_drawable_get_format;
  iface->get_format_with_alpha   = (const Babl    * (*) (GimpPickable *pickable)) gimp_drawable_get_format_with_alpha;
  iface->get_buffer              = (GeglBuffer    * (*) (GimpPickable *pickable)) gimp_drawable_get_buffer;
  iface->get_buffer_with_effects = (GeglBuffer    * (*) (GimpPickable *pickable)) gimp_drawable_get_buffer_with_effects;
  iface->get_pixel_at            = gimp_drawable_get_pixel_at;
  iface->get_pixel_average       = gimp_drawable_get_pixel_average;
}

static void
gimp_drawable_dispose (GObject *object)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  if (gimp_drawable_get_floating_sel (drawable))
    gimp_drawable_detach_floating_sel (drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_finalize (GObject *object)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  while (drawable->private->paint_count)
    gimp_drawable_end_paint (drawable);

  g_clear_object (&drawable->private->buffer);
  g_clear_object (&drawable->private->format_profile);

  gimp_drawable_free_shadow_buffer (drawable);

  g_clear_object (&drawable->private->source_node);
  g_clear_object (&drawable->private->buffer_source_node);

  _gimp_drawable_filters_finalize (drawable);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_drawable_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_BUFFER:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, drawable->private->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_drawable_get_memsize (GimpObject *object,
                           gint64     *gui_size)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (object);
  gint64        memsize  = 0;

  memsize += gimp_gegl_buffer_get_memsize (gimp_drawable_get_buffer (drawable));
  memsize += gimp_gegl_buffer_get_memsize (drawable->private->shadow);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_drawable_size_changed (GimpViewable *viewable)
{
  GList *list;
  gint   width;
  gint   height;

  if (GIMP_VIEWABLE_CLASS (parent_class)->size_changed)
    GIMP_VIEWABLE_CLASS (parent_class)->size_changed (viewable);

  width   = gimp_item_get_width (GIMP_ITEM (viewable));
  height  = gimp_item_get_height (GIMP_ITEM (viewable));

  for (list = GIMP_LIST (GIMP_DRAWABLE (viewable)->private->filter_stack)->queue->tail;
       list;
       list = g_list_previous (list))
    {
      if (GIMP_IS_DRAWABLE_FILTER (list->data))
        {
          GimpDrawableFilter *filter = list->data;
          GimpChannel        *mask   = GIMP_CHANNEL (gimp_drawable_filter_get_mask (filter));
          GeglRectangle       rect   = { 0, 0, width, height };

          /* Don't resize partial layer effects */
          if (gimp_channel_is_empty (mask))
            gimp_drawable_filter_refresh_crop (filter, &rect);
        }
    }
}

static gboolean
gimp_drawable_get_size (GimpViewable *viewable,
                        gint         *width,
                        gint         *height)
{
  GimpItem *item = GIMP_ITEM (viewable);

  *width  = gimp_item_get_width  (item);
  *height = gimp_item_get_height (item);

  return TRUE;
}

static void
gimp_drawable_preview_freeze (GimpViewable *viewable)
{
  GimpViewable *parent = gimp_viewable_get_parent (viewable);

  if (! parent && gimp_item_is_attached (GIMP_ITEM (viewable)))
    parent = GIMP_VIEWABLE (gimp_item_get_image (GIMP_ITEM (viewable)));

  if (parent)
    gimp_viewable_preview_freeze (parent);
}

static void
gimp_drawable_preview_thaw (GimpViewable *viewable)
{
  GimpViewable *parent = gimp_viewable_get_parent (viewable);

  if (! parent && gimp_item_is_attached (GIMP_ITEM (viewable)))
    parent = GIMP_VIEWABLE (gimp_item_get_image (GIMP_ITEM (viewable)));

  if (parent)
    gimp_viewable_preview_thaw (parent);
}

static GeglNode *
gimp_drawable_get_node (GimpFilter *filter)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *output;

  node = GIMP_FILTER_CLASS (parent_class)->get_node (filter);

  g_warn_if_fail (drawable->private->mode_node == NULL);

  drawable->private->mode_node =
    gegl_node_new_child (node,
                         "operation", "gimp:normal",
                         NULL);

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  gegl_node_link (input, drawable->private->mode_node);
  gegl_node_link (drawable->private->mode_node, output);

  return node;
}

static void
gimp_drawable_removed (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);

  gimp_drawable_free_shadow_buffer (drawable);

  if (GIMP_ITEM_CLASS (parent_class)->removed)
    GIMP_ITEM_CLASS (parent_class)->removed (item);
}

static GimpItem *
gimp_drawable_duplicate (GimpItem *item,
                         GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_DRAWABLE (new_item))
    {
      GimpDrawable  *drawable     = GIMP_DRAWABLE (item);
      GimpDrawable  *new_drawable = GIMP_DRAWABLE (new_item);
      GeglBuffer    *new_buffer;
      GimpContainer *filters;
      GList         *list;

      new_buffer = gimp_gegl_buffer_dup (gimp_drawable_get_buffer (drawable));

      gimp_drawable_set_buffer (new_drawable, FALSE, NULL, new_buffer);
      g_object_unref (new_buffer);

      filters = gimp_drawable_get_filters (drawable);

      for (list = GIMP_LIST (filters)->queue->tail;
           list;
           list = g_list_previous (list))
        {
          GimpDrawableFilter *filter = list->data;

          if (GIMP_IS_DRAWABLE_FILTER (filter))
            {
              GimpDrawableFilter *new_filter;

              new_filter = gimp_drawable_filter_duplicate (new_drawable,
                                                           filter);
              if (new_filter)
                {
                  gimp_drawable_filter_apply (new_filter, NULL);
                  gimp_drawable_filter_commit (new_filter, TRUE, NULL, FALSE);

                  gimp_drawable_filter_layer_mask_freeze (new_filter);
                  g_object_unref (new_filter);
                }
            }
        }
    }

  return new_item;
}

static void
gimp_drawable_scale (GimpItem              *item,
                     gint                   new_width,
                     gint                   new_height,
                     gint                   new_offset_x,
                     gint                   new_offset_y,
                     GimpInterpolationType  interpolation_type,
                     GimpProgress          *progress)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglBuffer   *new_buffer;

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                new_width, new_height),
                                gimp_drawable_get_format (drawable));

  gimp_gegl_apply_scale (gimp_drawable_get_buffer (drawable),
                         progress, C_("undo-type", "Scale"),
                         new_buffer,
                         interpolation_type,
                         ((gdouble) new_width /
                          gimp_item_get_width  (item)),
                         ((gdouble) new_height /
                          gimp_item_get_height (item)));

  gimp_drawable_set_buffer_full (drawable, gimp_item_is_attached (item), NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);
}

static void
gimp_drawable_resize (GimpItem     *item,
                      GimpContext  *context,
                      GimpFillType  fill_type,
                      gint          new_width,
                      gint          new_height,
                      gint          offset_x,
                      gint          offset_y)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglBuffer   *new_buffer;
  gint          new_offset_x;
  gint          new_offset_y;
  gint          copy_x, copy_y;
  gint          copy_width, copy_height;
  gboolean      intersect;

  /*  if the size doesn't change, this is a nop  */
  if (new_width  == gimp_item_get_width  (item) &&
      new_height == gimp_item_get_height (item) &&
      offset_x   == 0                           &&
      offset_y   == 0)
    return;

  new_offset_x = gimp_item_get_offset_x (item) - offset_x;
  new_offset_y = gimp_item_get_offset_y (item) - offset_y;

  intersect = gimp_rectangle_intersect (gimp_item_get_offset_x (item),
                                        gimp_item_get_offset_y (item),
                                        gimp_item_get_width (item),
                                        gimp_item_get_height (item),
                                        new_offset_x,
                                        new_offset_y,
                                        new_width,
                                        new_height,
                                        &copy_x,
                                        &copy_y,
                                        &copy_width,
                                        &copy_height);

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                new_width, new_height),
                                gimp_drawable_get_format (drawable));

  if (! intersect              ||
      copy_width  != new_width ||
      copy_height != new_height)
    {
      /*  Clear the new buffer if needed  */

      GeglColor   *color;
      GimpPattern *pattern;

      if (gimp_get_fill_params (context, fill_type, &color, &pattern, NULL))
        gimp_drawable_fill_buffer (drawable, new_buffer,
                                   color, pattern, 0, 0);

      g_clear_object (&color);
    }

  if (intersect && copy_width && copy_height)
    {
      /*  Copy the pixels in the intersection  */
      gimp_gegl_buffer_copy (
        gimp_drawable_get_buffer (drawable),
        GEGL_RECTANGLE (copy_x - gimp_item_get_offset_x (item),
                        copy_y - gimp_item_get_offset_y (item),
                        copy_width,
                        copy_height), GEGL_ABYSS_NONE,
        new_buffer,
        GEGL_RECTANGLE (copy_x - new_offset_x,
                        copy_y - new_offset_y, 0, 0));
    }

  gimp_drawable_set_buffer_full (drawable,
                                 gimp_item_is_attached (item) &&
                                 drawable->private->push_resize_undo,
                                 NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);
}

static void
gimp_drawable_flip (GimpItem            *item,
                    GimpContext         *context,
                    GimpOrientationType  flip_type,
                    gdouble              axis,
                    gboolean             clip_result)
{
  GimpDrawable     *drawable = GIMP_DRAWABLE (item);
  GeglBuffer       *buffer;
  GimpColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_flip (drawable, context,
                                                gimp_drawable_get_buffer (drawable),
                                                off_x, off_y,
                                                flip_type, axis,
                                                clip_result,
                                                &buffer_profile,
                                                &new_off_x, &new_off_y);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, TRUE);
      g_object_unref (buffer);
    }
}

static void
gimp_drawable_rotate (GimpItem         *item,
                      GimpContext      *context,
                      GimpRotationType  rotate_type,
                      gdouble           center_x,
                      gdouble           center_y,
                      gboolean          clip_result)
{
  GimpDrawable     *drawable = GIMP_DRAWABLE (item);
  GeglBuffer       *buffer;
  GimpColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_rotate (drawable, context,
                                                  gimp_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  rotate_type, center_x, center_y,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, TRUE);
      g_object_unref (buffer);
    }
}

static void
gimp_drawable_transform (GimpItem               *item,
                         GimpContext            *context,
                         const GimpMatrix3      *matrix,
                         GimpTransformDirection  direction,
                         GimpInterpolationType   interpolation_type,
                         GimpTransformResize     clip_result,
                         GimpProgress           *progress,
                         gboolean                push_undo)
{
  GimpDrawable     *drawable = GIMP_DRAWABLE (item);
  GeglBuffer       *buffer;
  GimpColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_affine (drawable, context,
                                                  gimp_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  matrix, direction,
                                                  interpolation_type,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y,
                                                  progress);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE, push_undo);
      g_object_unref (buffer);
    }
}

static const guint8 *
gimp_drawable_get_icc_profile (GimpColorManaged *managed,
                               gsize            *len)
{
  GimpColorProfile *profile = gimp_color_managed_get_color_profile (managed);

  return gimp_color_profile_get_icc_profile (profile, len);
}

static GimpColorProfile *
gimp_drawable_get_color_profile (GimpColorManaged *managed)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (managed);
  const Babl   *format   = gimp_drawable_get_format (drawable);

  if (! drawable->private->format_profile)
    drawable->private->format_profile =
      gimp_babl_format_get_color_profile (format);

  return drawable->private->format_profile;
}

static void
gimp_drawable_profile_changed (GimpColorManaged *managed)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (managed));
}

static gboolean
gimp_drawable_get_pixel_at (GimpPickable *pickable,
                            gint          x,
                            gint          y,
                            const Babl   *format,
                            gpointer      pixel)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (pickable);

  /* do not make this a g_return_if_fail() */
  if (x < 0 || x >= gimp_item_get_width  (GIMP_ITEM (drawable)) ||
      y < 0 || y >= gimp_item_get_height (GIMP_ITEM (drawable)))
    return FALSE;

  gegl_buffer_sample (gimp_drawable_get_buffer (drawable),
                      x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static void
gimp_drawable_get_pixel_average (GimpPickable        *pickable,
                                 const GeglRectangle *rect,
                                 const Babl          *format,
                                 gpointer             pixel)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (pickable);

  return gimp_gegl_average_color (gimp_drawable_get_buffer (drawable),
                                  rect, TRUE, GEGL_ABYSS_NONE, format, pixel);
}

static void
gimp_drawable_real_update (GimpDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

static void
gimp_drawable_real_filters_changed (GimpDrawable *drawable)
{
  gimp_drawable_update_bounding_box (drawable);
}

static gint64
gimp_drawable_real_estimate_memsize (GimpDrawable      *drawable,
                                     GimpComponentType  component_type,
                                     gint               width,
                                     gint               height)
{
  GimpImage   *image = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpTRCType  trc   = gimp_drawable_get_trc (drawable);
  const Babl  *format;

  format = gimp_image_get_format (image,
                                  gimp_drawable_get_base_type (drawable),
                                  gimp_babl_precision (component_type, trc),
                                  gimp_drawable_has_alpha (drawable),
                                  NULL);

  return (gint64) babl_format_get_bytes_per_pixel (format) * width * height;
}

static void
gimp_drawable_real_update_all (GimpDrawable *drawable)
{
  gimp_drawable_update (drawable, 0, 0, -1, -1);
}

static GimpComponentMask
gimp_drawable_real_get_active_mask (GimpDrawable *drawable)
{
  /*  Return all, because that skips the component mask op when painting  */
  return GIMP_COMPONENT_MASK_ALL;
}

static gboolean
gimp_drawable_real_supports_alpha (GimpDrawable *drawable)
{
  return FALSE;
}

/* FIXME: this default impl is currently unused because no subclass
 * chains up. the goal is to handle the almost identical subclass code
 * here again.
 */
static void
gimp_drawable_real_convert_type (GimpDrawable      *drawable,
                                 GimpImage         *dest_image,
                                 const Babl        *new_format,
                                 GimpColorProfile  *src_profile,
                                 GimpColorProfile  *dest_profile,
                                 GeglDitherMethod   layer_dither_type,
                                 GeglDitherMethod   mask_dither_type,
                                 gboolean           push_undo,
                                 GimpProgress      *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                                     gimp_item_get_height (GIMP_ITEM (drawable))),
                     new_format);

  gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL,
                         GEGL_ABYSS_NONE,
                         dest_buffer, NULL);

  gimp_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

static GeglBuffer *
gimp_drawable_real_get_buffer (GimpDrawable *drawable)
{
  return drawable->private->buffer;
}

static void
gimp_drawable_real_set_buffer (GimpDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds)
{
  GimpItem   *item          = GIMP_ITEM (drawable);
  const Babl *old_format    = NULL;
  gint        old_has_alpha = -1;

  g_object_freeze_notify (G_OBJECT (drawable));

  gimp_drawable_invalidate_boundary (drawable);

  if (push_undo)
    gimp_image_undo_push_drawable_mod (gimp_item_get_image (item), undo_desc,
                                       drawable, FALSE);

  if (drawable->private->buffer)
    {
      old_format    = gimp_drawable_get_format (drawable);
      old_has_alpha = gimp_drawable_has_alpha (drawable);
    }

  g_set_object (&drawable->private->buffer, buffer);

  if (gimp_drawable_is_painting (drawable))
    g_set_object (&drawable->private->paint_buffer, buffer);

  g_clear_object (&drawable->private->format_profile);

  if (drawable->private->buffer_source_node)
    gegl_node_set (drawable->private->buffer_source_node,
                   "buffer", gimp_drawable_get_buffer (drawable),
                   NULL);

  gimp_item_set_offset (item, bounds->x, bounds->y);
  gimp_item_set_size (item,
                      bounds->width  ? bounds->width :
                                       gegl_buffer_get_width (buffer),
                      bounds->height ? bounds->height :
                                       gegl_buffer_get_height (buffer));

  gimp_drawable_update_bounding_box (drawable);

  if (gimp_drawable_get_format (drawable) != old_format)
    gimp_drawable_format_changed (drawable);

  if (gimp_drawable_has_alpha (drawable) != old_has_alpha)
    gimp_drawable_alpha_changed (drawable);

  g_object_notify (G_OBJECT (drawable), "buffer");

  g_object_thaw_notify (G_OBJECT (drawable));
}

static GeglRectangle
gimp_drawable_real_get_bounding_box (GimpDrawable *drawable)
{
  return gegl_node_get_bounding_box (gimp_drawable_get_source_node (drawable));
}

static void
gimp_drawable_real_push_undo (GimpDrawable *drawable,
                              const gchar  *undo_desc,
                              GeglBuffer   *buffer,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height)
{
  GimpImage *image;

  if (! buffer)
    {
      GeglBuffer    *drawable_buffer = gimp_drawable_get_buffer (drawable);
      GeglRectangle  drawable_rect;

      gegl_rectangle_align_to_buffer (
        &drawable_rect,
        GEGL_RECTANGLE (x, y, width, height),
        drawable_buffer,
        GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      x      = drawable_rect.x;
      y      = drawable_rect.y;
      width  = drawable_rect.width;
      height = drawable_rect.height;

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                gimp_drawable_get_format (drawable));

      gimp_gegl_buffer_copy (
        drawable_buffer,
        &drawable_rect, GEGL_ABYSS_NONE,
        buffer,
        GEGL_RECTANGLE (0, 0, 0, 0));
    }
  else
    {
      g_object_ref (buffer);
    }

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  gimp_image_undo_push_drawable (image,
                                 undo_desc, drawable,
                                 buffer, x, y);

  g_object_unref (buffer);
}

static void
gimp_drawable_real_swap_pixels (GimpDrawable *drawable,
                                GeglBuffer   *buffer,
                                gint          x,
                                gint          y)
{
  GeglBuffer *tmp;
  gint        width  = gegl_buffer_get_width (buffer);
  gint        height = gegl_buffer_get_height (buffer);

  tmp = gimp_gegl_buffer_dup (buffer);

  gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, width, height), GEGL_ABYSS_NONE,
                         buffer,
                         GEGL_RECTANGLE (0, 0, 0, 0));
  gimp_gegl_buffer_copy (tmp,
                         GEGL_RECTANGLE (0, 0, width, height), GEGL_ABYSS_NONE,
                         gimp_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, 0, 0));

  g_object_unref (tmp);

  gimp_drawable_update (drawable, x, y, width, height);
}

static GeglNode *
gimp_drawable_real_get_source_node (GimpDrawable *drawable)
{
  g_warn_if_fail (drawable->private->buffer_source_node == NULL);

  drawable->private->buffer_source_node =
    gegl_node_new_child (NULL,
                         "operation", "gimp:buffer-source-validate",
                         "buffer",    gimp_drawable_get_buffer (drawable),
                         NULL);

  return g_object_ref (drawable->private->buffer_source_node);
}

static void
gimp_drawable_format_changed (GimpDrawable *drawable)
{
  g_signal_emit (drawable, gimp_drawable_signals[FORMAT_CHANGED], 0);
}

static void
gimp_drawable_alpha_changed (GimpDrawable *drawable)
{
  g_signal_emit (drawable, gimp_drawable_signals[ALPHA_CHANGED], 0);
}


/*  public functions  */

GimpDrawable *
gimp_drawable_new (GType          type,
                   GimpImage     *image,
                   const gchar   *name,
                   gint           offset_x,
                   gint           offset_y,
                   gint           width,
                   gint           height,
                   const Babl    *format)
{
  GimpDrawable *drawable;
  GeglBuffer   *buffer;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  drawable = GIMP_DRAWABLE (gimp_item_new (type,
                                           image, name,
                                           offset_x, offset_y,
                                           width, height));

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height), format);

  gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  gimp_drawable_enable_resize_undo (drawable);

  return drawable;
}

gint64
gimp_drawable_estimate_memsize (GimpDrawable      *drawable,
                                GimpComponentType  component_type,
                                gint               width,
                                gint               height)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  return GIMP_DRAWABLE_GET_CLASS (drawable)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

void
gimp_drawable_update (GimpDrawable *drawable,
                      gint          x,
                      gint          y,
                      gint          width,
                      gint          height)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (width < 0 || height < 0)
    {
      GeglRectangle bounding_box;

      bounding_box = gimp_drawable_get_bounding_box (drawable);

      if (width < 0)
        {
          x     = bounding_box.x;
          width = bounding_box.width;
        }

      if (height < 0)
        {
          y      = bounding_box.y;
          height = bounding_box.height;
        }
    }

  if (drawable->private->paint_count == 0)
    {
      g_signal_emit (drawable, gimp_drawable_signals[UPDATE], 0,
                     x, y, width, height);
    }
  else
    {
      GeglRectangle rect;

      if (gegl_rectangle_intersect (
            &rect,
            GEGL_RECTANGLE (x, y, width, height),
            GEGL_RECTANGLE (0, 0,
                            gimp_item_get_width  (GIMP_ITEM (drawable)),
                            gimp_item_get_height (GIMP_ITEM (drawable)))))
        {
          GeglRectangle aligned_rect;

          gegl_rectangle_align_to_buffer (&aligned_rect, &rect,
                                          gimp_drawable_get_buffer (drawable),
                                          GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          if (drawable->private->paint_copy_region)
            {
              cairo_region_union_rectangle (
                drawable->private->paint_copy_region,
                (const cairo_rectangle_int_t *) &aligned_rect);
            }
          else
            {
              drawable->private->paint_copy_region =
                cairo_region_create_rectangle (
                  (const cairo_rectangle_int_t *) &aligned_rect);
            }

          gegl_rectangle_align (&aligned_rect, &rect,
                                GEGL_RECTANGLE (0, 0,
                                                PAINT_UPDATE_CHUNK_WIDTH,
                                                PAINT_UPDATE_CHUNK_HEIGHT),
                                GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          if (drawable->private->paint_update_region)
            {
              cairo_region_union_rectangle (
                drawable->private->paint_update_region,
                (const cairo_rectangle_int_t *) &aligned_rect);
            }
          else
            {
              drawable->private->paint_update_region =
                cairo_region_create_rectangle (
                  (const cairo_rectangle_int_t *) &aligned_rect);
            }
        }
    }
}

void
gimp_drawable_update_all (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  GIMP_DRAWABLE_GET_CLASS (drawable)->update_all (drawable);
}

void
gimp_drawable_filters_changed (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  g_signal_emit (drawable, gimp_drawable_signals[FILTERS_CHANGED], 0);
}

void
gimp_drawable_invalidate_boundary (GimpDrawable *drawable)
{
  GimpDrawableClass *drawable_class;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->invalidate_boundary)
    drawable_class->invalidate_boundary (drawable);
}

void
gimp_drawable_get_active_components (GimpDrawable *drawable,
                                     gboolean     *active)
{
  GimpDrawableClass *drawable_class;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (active != NULL);

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->get_active_components)
    drawable_class->get_active_components (drawable, active);
}

GimpComponentMask
gimp_drawable_get_active_mask (GimpDrawable *drawable)
{
  GimpComponentMask mask;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  mask = GIMP_DRAWABLE_GET_CLASS (drawable)->get_active_mask (drawable);

  /* if the drawable doesn't have an alpha channel, the value of the mask's
   * alpha-bit doesn't matter, however, we'd like to have a fully-clear or
   * fully-set mask whenever possible, since it allows us to skip component
   * masking altogether.  we therefore set or clear the alpha bit, depending on
   * the state of the other bits, so that it never gets in the way of a uniform
   * mask.
   */
  if (! gimp_drawable_has_alpha (drawable))
    {
      if (mask & ~GIMP_COMPONENT_MASK_ALPHA)
        mask |= GIMP_COMPONENT_MASK_ALPHA;
      else
        mask &= ~GIMP_COMPONENT_MASK_ALPHA;
    }

  return mask;
}

gboolean
gimp_drawable_supports_alpha (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return GIMP_DRAWABLE_GET_CLASS (drawable)->supports_alpha (drawable);
}

void
gimp_drawable_convert_type (GimpDrawable      *drawable,
                            GimpImage         *dest_image,
                            GimpImageBaseType  new_base_type,
                            GimpPrecision      new_precision,
                            gboolean           new_has_alpha,
                            GimpColorProfile  *src_profile,
                            GimpColorProfile  *dest_profile,
                            GeglDitherMethod   layer_dither_type,
                            GeglDitherMethod   mask_dither_type,
                            gboolean           push_undo,
                            GimpProgress      *progress)
{
  const Babl    *old_format;
  const Babl    *new_format;
  gint           old_bits;
  gint           new_bits;
  GimpContainer *filters;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (new_base_type != gimp_drawable_get_base_type (drawable) ||
                    new_precision != gimp_drawable_get_precision (drawable) ||
                    new_has_alpha != gimp_drawable_has_alpha (drawable)     ||
                    dest_profile);
  g_return_if_fail (src_profile == NULL || GIMP_IS_COLOR_PROFILE (src_profile));
  g_return_if_fail (dest_profile == NULL || GIMP_IS_COLOR_PROFILE (dest_profile));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  old_format = gimp_drawable_get_format (drawable);
  new_format = gimp_image_get_format (dest_image,
                                      new_base_type,
                                      new_precision,
                                      new_has_alpha,
                                      NULL /* handled by layer */);

  old_bits = (babl_format_get_bytes_per_pixel (old_format) * 8 /
              babl_format_get_n_components (old_format));
  new_bits = (babl_format_get_bytes_per_pixel (new_format) * 8 /
              babl_format_get_n_components (new_format));

  if (old_bits <= new_bits || new_bits > 16)
    {
      /*  don't dither if we are converting to a higher bit depth,
       *  or to more than 16 bits (gegl:dither only does
       *  16 bits).
       */
      layer_dither_type = GEGL_DITHER_NONE;
      mask_dither_type  = GEGL_DITHER_NONE;
    }

  GIMP_DRAWABLE_GET_CLASS (drawable)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);

  /* Update the masks of any filters */
  /* TODO: Move to gimp_drawable_real_convert_type () once it's updated
   * to run for all GimpDrawable child classes */
  filters = gimp_drawable_get_filters (drawable);
  if (gimp_container_get_n_children (filters) > 0)
    {
      const Babl    *mask_format;
      GList         *filter_list;
      GimpPrecision  new_mask_precision;

      mask_format        = gimp_image_get_mask_format (dest_image);
      new_mask_precision = gimp_babl_format_get_precision (mask_format);

      for (filter_list = GIMP_LIST (filters)->queue->tail;
           filter_list;
           filter_list = g_list_previous (filter_list))
        {
          if (GIMP_IS_DRAWABLE_FILTER (filter_list->data))
            {
              GimpDrawableFilter     *filter = filter_list->data;
              GimpDrawableFilterMask *mask;

              mask = gimp_drawable_filter_get_mask (filter);

              if (new_mask_precision == gimp_drawable_get_precision (GIMP_DRAWABLE (mask)))
                /* The only change which may happen for masks is the bit-depth. */
                break;

              gimp_drawable_convert_type (GIMP_DRAWABLE (mask), dest_image,
                                          GIMP_GRAY, new_mask_precision,
                                          FALSE,
                                          NULL, NULL,
                                          layer_dither_type, mask_dither_type,
                                          push_undo, progress);
            }
        }
    }

  if (progress)
    gimp_progress_set_value (progress, 1.0);
}

void
gimp_drawable_apply_buffer (GimpDrawable           *drawable,
                            GeglBuffer             *buffer,
                            const GeglRectangle    *buffer_region,
                            gboolean                push_undo,
                            const gchar            *undo_desc,
                            gdouble                 opacity,
                            GimpLayerMode           mode,
                            GimpLayerColorSpace     blend_space,
                            GimpLayerColorSpace     composite_space,
                            GimpLayerCompositeMode  composite_mode,
                            GeglBuffer             *base_buffer,
                            gint                    base_x,
                            gint                    base_y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_region != NULL);
  g_return_if_fail (base_buffer == NULL || GEGL_IS_BUFFER (base_buffer));

  GIMP_DRAWABLE_GET_CLASS (drawable)->apply_buffer (drawable, buffer,
                                                    buffer_region,
                                                    push_undo, undo_desc,
                                                    opacity, mode,
                                                    blend_space,
                                                    composite_space,
                                                    composite_mode,
                                                    base_buffer,
                                                    base_x, base_y);
}

GeglBuffer *
gimp_drawable_get_buffer (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->paint_count == 0)
    return GIMP_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);
  else
    return drawable->private->paint_buffer;
}

void
gimp_drawable_set_buffer (GimpDrawable *drawable,
                          gboolean      push_undo,
                          const gchar  *undo_desc,
                          GeglBuffer   *buffer)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  gimp_drawable_set_buffer_full (drawable, push_undo, undo_desc, buffer, NULL,
                                 TRUE);
}

void
gimp_drawable_set_buffer_full (GimpDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds,
                               gboolean             update)
{
  GimpItem      *item;
  GeglRectangle  curr_bounds;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  item = GIMP_ITEM (drawable);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  if (! bounds)
    {
      gimp_item_get_offset (GIMP_ITEM (drawable),
                            &curr_bounds.x, &curr_bounds.y);

      curr_bounds.width  = 0;
      curr_bounds.height = 0;

      bounds = &curr_bounds;
    }

  if (update && gimp_drawable_get_buffer (drawable))
    {
      GeglBuffer    *old_buffer = gimp_drawable_get_buffer (drawable);
      GeglRectangle  old_extent;
      GeglRectangle  new_extent;

      old_extent = *gegl_buffer_get_extent (old_buffer);
      old_extent.x += gimp_item_get_offset_x (item);
      old_extent.y += gimp_item_get_offset_x (item);

      new_extent = *gegl_buffer_get_extent (buffer);
      new_extent.x += bounds->x;
      new_extent.y += bounds->y;

      if (! gegl_rectangle_equal (&old_extent, &new_extent))
        gimp_drawable_update (drawable, 0, 0, -1, -1);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  GIMP_DRAWABLE_GET_CLASS (drawable)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  g_object_thaw_notify (G_OBJECT (drawable));

  if (update)
    gimp_drawable_update (drawable, 0, 0, -1, -1);
}

GeglBuffer *
gimp_drawable_get_buffer_with_effects (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->paint_count == 0)
    {
      if (gimp_drawable_has_visible_filters (drawable))
        {
          GeglNode                *source = NULL;
          GeglBuffer              *buffer;
          GimpTileHandlerValidate *validate;

          source = gimp_drawable_get_source_node (drawable);
          buffer = GIMP_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);

          if (source)
            {
              buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                                        gegl_buffer_get_format (buffer));

              validate =
                GIMP_TILE_HANDLER_VALIDATE (gimp_tile_handler_validate_new (source));

              gimp_tile_handler_validate_assign (validate, buffer);

              g_object_unref (validate);

              gimp_tile_handler_validate_invalidate (validate,
                                                     gegl_buffer_get_extent (buffer));

              return buffer;
            }
          else
            {
              return g_object_ref (buffer);
            }
        }
      else
        {
          return g_object_ref (GIMP_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable));
        }
    }
  else
    {
      return g_object_ref (drawable->private->paint_buffer);
    }
}

void
gimp_drawable_steal_buffer (GimpDrawable *drawable,
                            GimpDrawable *src_drawable)
{
  GeglBuffer *buffer;
  GeglBuffer *replacement_buffer;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_DRAWABLE (src_drawable));

  buffer = gimp_drawable_get_buffer (src_drawable);

  g_return_if_fail (buffer != NULL);

  g_object_ref (buffer);

  replacement_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 1, 1),
                                        gegl_buffer_get_format (buffer));

  gimp_drawable_set_buffer (src_drawable, FALSE, NULL, replacement_buffer);
  gimp_drawable_set_buffer (drawable,     FALSE, NULL, buffer);

  g_object_unref (replacement_buffer);
  g_object_unref (buffer);
}

void
gimp_drawable_set_format (GimpDrawable *drawable,
                          const Babl   *format,
                          gboolean      copy_buffer,
                          gboolean      push_undo)
{
  GimpItem   *item;
  GeglBuffer *buffer;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (format != NULL);
  g_return_if_fail (format != gimp_drawable_get_format (drawable));
  g_return_if_fail (gimp_babl_format_get_base_type (format) ==
                    gimp_drawable_get_base_type (drawable));
  g_return_if_fail (gimp_babl_format_get_component_type (format) ==
                    gimp_drawable_get_component_type (drawable));
  g_return_if_fail (babl_format_has_alpha (format) ==
                    gimp_drawable_has_alpha (drawable));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  item = GIMP_ITEM (drawable);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_drawable_format (gimp_item_get_image (item),
                                          NULL, drawable);

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            gimp_item_get_width  (item),
                                            gimp_item_get_height (item)),
                            format);

  if (copy_buffer)
    {
      gegl_buffer_set_format (buffer, gimp_drawable_get_format (drawable));

      gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                             NULL, GEGL_ABYSS_NONE,
                             buffer, NULL);

      gegl_buffer_set_format (buffer, NULL);
    }

  gimp_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);
}

GeglNode *
gimp_drawable_get_source_node (GimpDrawable *drawable)
{
  GeglNode *input;
  GeglNode *source;
  GeglNode *filter;
  GeglNode *output;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->source_node)
    return drawable->private->source_node;

  drawable->private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (drawable->private->source_node, "input");

  source = GIMP_DRAWABLE_GET_CLASS (drawable)->get_source_node (drawable);

  gegl_node_add_child (drawable->private->source_node, source);

  g_object_unref (source);

  if (gegl_node_has_pad (source, "input"))
    {
      gegl_node_link (input, source);
    }

  filter = gimp_filter_stack_get_graph (GIMP_FILTER_STACK (drawable->private->filter_stack));

  gegl_node_add_child (drawable->private->source_node, filter);

  gegl_node_link (source, filter);

  output = gegl_node_get_output_proxy (drawable->private->source_node, "output");

  gegl_node_link (filter, output);

  if (gimp_drawable_get_floating_sel (drawable))
    _gimp_drawable_add_floating_sel_filter (drawable);

  return drawable->private->source_node;
}

GeglNode *
gimp_drawable_get_mode_node (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (! drawable->private->mode_node)
    gimp_filter_get_node (GIMP_FILTER (drawable));

  return drawable->private->mode_node;
}

GeglRectangle
gimp_drawable_get_bounding_box (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  if (gegl_rectangle_is_empty (&drawable->private->bounding_box))
    gimp_drawable_update_bounding_box (drawable);

  return drawable->private->bounding_box;
}

gboolean
gimp_drawable_update_bounding_box (GimpDrawable *drawable)
{
  GeglRectangle bounding_box;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  bounding_box =
    GIMP_DRAWABLE_GET_CLASS (drawable)->get_bounding_box (drawable);

  if (! gegl_rectangle_equal (&bounding_box, &drawable->private->bounding_box))
    {
      GeglRectangle old_bounding_box = drawable->private->bounding_box;
      GeglRectangle diff_rects[4];
      gint          n_diff_rects;
      gint          i;

      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &old_bounding_box,
                                              &bounding_box);

      for (i = 0; i < n_diff_rects; i++)
        {
          gimp_drawable_update (drawable,
                                diff_rects[i].x,
                                diff_rects[i].y,
                                diff_rects[i].width,
                                diff_rects[i].height);
        }

      drawable->private->bounding_box = bounding_box;

      g_signal_emit (drawable, gimp_drawable_signals[BOUNDING_BOX_CHANGED], 0);

      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &bounding_box,
                                              &old_bounding_box);

      for (i = 0; i < n_diff_rects; i++)
        {
          gimp_drawable_update (drawable,
                                diff_rects[i].x,
                                diff_rects[i].y,
                                diff_rects[i].width,
                                diff_rects[i].height);
        }

      return TRUE;
    }

  return FALSE;
}

void
gimp_drawable_swap_pixels (GimpDrawable *drawable,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  GIMP_DRAWABLE_GET_CLASS (drawable)->swap_pixels (drawable, buffer, x, y);
}

void
gimp_drawable_push_undo (GimpDrawable *drawable,
                         const gchar  *undo_desc,
                         GeglBuffer   *buffer,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (buffer == NULL || GEGL_IS_BUFFER (buffer));

  item = GIMP_ITEM (drawable);

  g_return_if_fail (gimp_item_is_attached (item));

  if (! buffer &&
      ! gimp_rectangle_intersect (x, y,
                                  width, height,
                                  0, 0,
                                  gimp_item_get_width (item),
                                  gimp_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      g_warning ("%s: tried to push empty region", G_STRFUNC);
      return;
    }

  GIMP_DRAWABLE_GET_CLASS (drawable)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);
}

void
gimp_drawable_disable_resize_undo (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable->private->push_resize_undo = FALSE;
}

void
gimp_drawable_enable_resize_undo (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  drawable->private->push_resize_undo = TRUE;
}

const Babl *
gimp_drawable_get_space (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return babl_format_get_space (gimp_drawable_get_format (drawable));
}

const Babl *
gimp_drawable_get_format (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gegl_buffer_get_format (drawable->private->buffer);
}

const Babl *
gimp_drawable_get_format_with_alpha (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gimp_image_get_format (gimp_item_get_image (GIMP_ITEM (drawable)),
                                gimp_drawable_get_base_type (drawable),
                                gimp_drawable_get_precision (drawable),
                                TRUE,
                                gimp_drawable_get_space (drawable));
}

const Babl *
gimp_drawable_get_format_without_alpha (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gimp_image_get_format (gimp_item_get_image (GIMP_ITEM (drawable)),
                                gimp_drawable_get_base_type (drawable),
                                gimp_drawable_get_precision (drawable),
                                FALSE,
                                gimp_drawable_get_space (drawable));
}

GimpTRCType
gimp_drawable_get_trc (GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_trc (format);
}

gboolean
gimp_drawable_has_alpha (GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return babl_format_has_alpha (format);
}

GimpImageBaseType
gimp_drawable_get_base_type (GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_base_type (format);
}

GimpComponentType
gimp_drawable_get_component_type (GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_component_type (format);
}

GimpPrecision
gimp_drawable_get_precision (GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_precision (format);
}

gboolean
gimp_drawable_is_rgb (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_RGB);
}

gboolean
gimp_drawable_is_gray (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_GRAY);
}

gboolean
gimp_drawable_is_indexed (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_INDEXED);
}

const Babl *
gimp_drawable_get_component_format (GimpDrawable    *drawable,
                                    GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  switch (channel)
    {
    case GIMP_CHANNEL_RED:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         RED);

    case GIMP_CHANNEL_GREEN:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         GREEN);

    case GIMP_CHANNEL_BLUE:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         BLUE);

    case GIMP_CHANNEL_ALPHA:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         ALPHA);

    case GIMP_CHANNEL_GRAY:
      return gimp_babl_component_format (GIMP_GRAY,
                                         gimp_drawable_get_precision (drawable),
                                         GRAY);

    case GIMP_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
gimp_drawable_get_component_index (GimpDrawable    *drawable,
                                   GimpChannelType  channel)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  switch (channel)
    {
    case GIMP_CHANNEL_RED:     return RED;
    case GIMP_CHANNEL_GREEN:   return GREEN;
    case GIMP_CHANNEL_BLUE:    return BLUE;
    case GIMP_CHANNEL_GRAY:    return GRAY;
    case GIMP_CHANNEL_INDEXED: return INDEXED;
    case GIMP_CHANNEL_ALPHA:
      switch (gimp_drawable_get_base_type (drawable))
        {
        case GIMP_RGB:     return ALPHA;
        case GIMP_GRAY:    return ALPHA_G;
        case GIMP_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

void
gimp_drawable_start_paint (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  if (drawable->private->paint_count == 0)
    {
      GeglBuffer *buffer = gimp_drawable_get_buffer (drawable);

      g_return_if_fail (buffer != NULL);
      g_return_if_fail (drawable->private->paint_buffer == NULL);
      g_return_if_fail (drawable->private->paint_copy_region == NULL);
      g_return_if_fail (drawable->private->paint_update_region == NULL);

      drawable->private->paint_buffer = gimp_gegl_buffer_dup (buffer);
    }

  drawable->private->paint_count++;
}

gboolean
gimp_drawable_end_paint (GimpDrawable *drawable)
{
  gboolean result = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_count == 1)
    {
      result = gimp_drawable_flush_paint (drawable);

      g_clear_object (&drawable->private->paint_buffer);
    }

  drawable->private->paint_count--;

  /* Refresh filters after painting */
  if (gimp_drawable_has_visible_filters (drawable) &&
      drawable->private->paint_count == 0)
    {
      gimp_drawable_update (drawable, 0, 0, -1, -1);
    }

  return result;
}

gboolean
gimp_drawable_flush_paint (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_copy_region)
    {
      GeglBuffer *buffer;
      gint        n_rects;
      gint        i;

      buffer = GIMP_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);

      g_return_val_if_fail (buffer != NULL, FALSE);
      g_return_val_if_fail (drawable->private->paint_buffer != NULL, FALSE);

      n_rects = cairo_region_num_rectangles (
        drawable->private->paint_copy_region);

      for (i = 0; i < n_rects; i++)
        {
          GeglRectangle rect;

          cairo_region_get_rectangle (drawable->private->paint_copy_region,
                                      i, (cairo_rectangle_int_t *) &rect);

          gimp_gegl_buffer_copy (
            drawable->private->paint_buffer, &rect, GEGL_ABYSS_NONE,
            buffer, NULL);
        }

      g_clear_pointer (&drawable->private->paint_copy_region,
                       cairo_region_destroy);

      n_rects = cairo_region_num_rectangles (
        drawable->private->paint_update_region);

      for (i = 0; i < n_rects; i++)
        {
          GeglRectangle rect;

          cairo_region_get_rectangle (drawable->private->paint_update_region,
                                      i, (cairo_rectangle_int_t *) &rect);

          g_signal_emit (drawable, gimp_drawable_signals[UPDATE], 0,
                         rect.x, rect.y, rect.width, rect.height);
        }

      g_clear_pointer (&drawable->private->paint_update_region,
                       cairo_region_destroy);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_drawable_is_painting (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return drawable->private->paint_count > 0;
}
