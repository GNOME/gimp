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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gegl-plugin.h> /* gegl_operation_invalidate() */
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp-memsize.h"
#include "gimp-utils.h"
#include "gimpchannel.h"
#include "gimpcontext.h"
#include "gimpdrawable-combine.h"
#include "gimpdrawable-filter.h"
#include "gimpdrawable-preview.h"
#include "gimpdrawable-private.h"
#include "gimpdrawable-shadow.h"
#include "gimpdrawable-transform.h"
#include "gimpfilterstack.h"
#include "gimpimage.h"
#include "gimpimage-colormap.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimpmarshal.h"
#include "gimppattern.h"
#include "gimppickable.h"
#include "gimpprogress.h"

#include "gimp-log.h"

#include "gimp-intl.h"


enum
{
  UPDATE,
  ALPHA_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void  gimp_drawable_pickable_iface_init (GimpPickableInterface *iface);

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

static gboolean   gimp_drawable_get_size           (GimpViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);

static GeglNode * gimp_drawable_get_node           (GimpFilter        *filter);

static void       gimp_drawable_removed            (GimpItem          *item);
static void       gimp_drawable_visibility_changed (GimpItem          *item);
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
                                                    GimpProgress      *progress);

static gboolean   gimp_drawable_get_pixel_at       (GimpPickable      *pickable,
                                                    gint               x,
                                                    gint               y,
                                                    const Babl        *format,
                                                    gpointer           pixel);
static void       gimp_drawable_real_update        (GimpDrawable      *drawable,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

static gint64  gimp_drawable_real_estimate_memsize (const GimpDrawable *drawable,
                                                    GimpComponentType  component_type,
                                                    gint               width,
                                                    gint               height);

static void       gimp_drawable_real_convert_type  (GimpDrawable      *drawable,
                                                    GimpImage         *dest_image,
                                                    const Babl        *new_format,
                                                    GimpImageBaseType  new_base_type,
                                                    GimpPrecision      new_precision,
                                                    gint               layer_dither_type,
                                                    gint               mask_dither_type,
                                                    gboolean           convert_profile,
                                                    gboolean           push_undo,
                                                    GimpProgress      *progress);

static GeglBuffer * gimp_drawable_real_get_buffer  (GimpDrawable      *drawable);
static void       gimp_drawable_real_set_buffer    (GimpDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    gint               offset_x,
                                                    gint               offset_y);

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

static void       gimp_drawable_sync_fs_filter     (GimpDrawable      *drawable,
                                                    gboolean           detach_fs);
static void       gimp_drawable_fs_notify          (GimpLayer         *fs,
                                                    const GParamSpec  *pspec,
                                                    GimpDrawable      *drawable);
static void       gimp_drawable_fs_affect_changed  (GimpImage         *image,
                                                    GimpChannelType    channel,
                                                    GimpDrawable      *drawable);
static void       gimp_drawable_fs_mask_changed    (GimpImage         *image,
                                                    GimpDrawable      *drawable);
static void       gimp_drawable_fs_update          (GimpLayer         *fs,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height,
                                                    GimpDrawable      *drawable);


G_DEFINE_TYPE_WITH_CODE (GimpDrawable, gimp_drawable, GIMP_TYPE_ITEM,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_drawable_pickable_iface_init))

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

  gimp_drawable_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableClass, alpha_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->dispose              = gimp_drawable_dispose;
  object_class->finalize             = gimp_drawable_finalize;
  object_class->set_property         = gimp_drawable_set_property;
  object_class->get_property         = gimp_drawable_get_property;

  gimp_object_class->get_memsize     = gimp_drawable_get_memsize;

  viewable_class->get_size           = gimp_drawable_get_size;
  viewable_class->get_new_preview    = gimp_drawable_get_new_preview;
  viewable_class->get_new_pixbuf     = gimp_drawable_get_new_pixbuf;

  filter_class->get_node             = gimp_drawable_get_node;

  item_class->removed                = gimp_drawable_removed;
  item_class->visibility_changed     = gimp_drawable_visibility_changed;
  item_class->duplicate              = gimp_drawable_duplicate;
  item_class->scale                  = gimp_drawable_scale;
  item_class->resize                 = gimp_drawable_resize;
  item_class->flip                   = gimp_drawable_flip;
  item_class->rotate                 = gimp_drawable_rotate;
  item_class->transform              = gimp_drawable_transform;

  klass->update                      = gimp_drawable_real_update;
  klass->alpha_changed               = NULL;
  klass->estimate_memsize            = gimp_drawable_real_estimate_memsize;
  klass->invalidate_boundary         = NULL;
  klass->get_active_components       = NULL;
  klass->get_active_mask             = NULL;
  klass->convert_type                = gimp_drawable_real_convert_type;
  klass->apply_buffer                = gimp_drawable_real_apply_buffer;
  klass->replace_buffer              = gimp_drawable_real_replace_buffer;
  klass->get_buffer                  = gimp_drawable_real_get_buffer;
  klass->set_buffer                  = gimp_drawable_real_set_buffer;
  klass->push_undo                   = gimp_drawable_real_push_undo;
  klass->swap_pixels                 = gimp_drawable_real_swap_pixels;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");

  g_type_class_add_private (klass, sizeof (GimpDrawablePrivate));
}

static void
gimp_drawable_init (GimpDrawable *drawable)
{
  drawable->private = G_TYPE_INSTANCE_GET_PRIVATE (drawable,
                                                   GIMP_TYPE_DRAWABLE,
                                                   GimpDrawablePrivate);

  drawable->private->filter_stack = gimp_filter_stack_new (GIMP_TYPE_FILTER);
}

/* sorry for the evil casts */

static void
gimp_drawable_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->get_image             = (GimpImage     * (*) (GimpPickable *pickable)) gimp_item_get_image;
  iface->get_format            = (const Babl    * (*) (GimpPickable *pickable)) gimp_drawable_get_format;
  iface->get_format_with_alpha = (const Babl    * (*) (GimpPickable *pickable)) gimp_drawable_get_format_with_alpha;
  iface->get_buffer            = (GeglBuffer    * (*) (GimpPickable *pickable)) gimp_drawable_get_buffer;
  iface->get_pixel_at          = gimp_drawable_get_pixel_at;
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

  if (drawable->private->buffer)
    {
      g_object_unref (drawable->private->buffer);
      drawable->private->buffer = NULL;
    }

  gimp_drawable_free_shadow_buffer (drawable);

  if (drawable->private->source_node)
    {
      g_object_unref (drawable->private->source_node);
      drawable->private->source_node = NULL;
    }

  if (drawable->private->filter_stack)
    {
      g_object_unref (drawable->private->filter_stack);
      drawable->private->filter_stack = NULL;
    }

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
                         "operation", "gimp:normal-mode",
                         NULL);

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  if (gimp_item_get_visible (GIMP_ITEM (drawable)))
    {
      gegl_node_connect_to (input,                        "output",
                            drawable->private->mode_node, "input");
      gegl_node_connect_to (drawable->private->mode_node, "output",
                            output,                       "input");
    }
  else
    {
      gegl_node_connect_to (input,  "output",
                            output, "input");
    }

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

static void
gimp_drawable_visibility_changed (GimpItem *item)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglNode     *node;

  /*  don't use gimp_filter_get_node() because that would create
   *  the node.
   */
  node = gimp_filter_peek_node (GIMP_FILTER (item));

  if (node)
    {
      GeglNode *input  = gegl_node_get_input_proxy  (node, "input");
      GeglNode *output = gegl_node_get_output_proxy (node, "output");

      if (gimp_item_get_visible (item))
        {
          gegl_node_connect_to (input,                        "output",
                                drawable->private->mode_node, "input");
          gegl_node_connect_to (drawable->private->mode_node, "output",
                                output,                       "input");
        }
      else
        {
          gegl_node_disconnect (drawable->private->mode_node, "input");

          gegl_node_connect_to (input,  "output",
                                output, "input");
        }

      /* FIXME: chain up again when above floating sel special case is gone */
      return;
    }

  GIMP_ITEM_CLASS (parent_class)->visibility_changed (item);
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

      if (new_drawable->private->buffer)
        g_object_unref (new_drawable->private->buffer);

      new_drawable->private->buffer =
        gegl_buffer_dup (gimp_drawable_get_buffer (drawable));
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
                                 new_offset_x, new_offset_y);
  g_object_unref (new_buffer);
}

static void
gimp_drawable_resize (GimpItem    *item,
                      GimpContext *context,
                      gint         new_width,
                      gint         new_height,
                      gint         offset_x,
                      gint         offset_y)
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
      /*  Clear the new tiles if needed  */

      GimpRGB    bg;
      GeglColor *col;

      if (! gimp_drawable_has_alpha (drawable) && ! GIMP_IS_CHANNEL (drawable))
        gimp_context_get_background (context, &bg);
      else
        gimp_rgba_set (&bg, 0.0, 0.0, 0.0, 0.0);

      col = gimp_gegl_color_new (&bg);

      gegl_buffer_set_color (new_buffer, NULL, col);
      g_object_unref (col);
    }

  if (intersect && copy_width && copy_height)
    {
      /*  Copy the pixels in the intersection  */
      gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                        GEGL_RECTANGLE (copy_x - gimp_item_get_offset_x (item),
                                        copy_y - gimp_item_get_offset_y (item),
                                        copy_width,
                                        copy_height), GEGL_ABYSS_NONE,
                        new_buffer,
                        GEGL_RECTANGLE (copy_x - new_offset_x,
                                        copy_y - new_offset_y, 0, 0));
    }

  gimp_drawable_set_buffer_full (drawable, gimp_item_is_attached (item), NULL,
                                 new_buffer,
                                 new_offset_x, new_offset_y);
  g_object_unref (new_buffer);
}

static void
gimp_drawable_flip (GimpItem            *item,
                    GimpContext         *context,
                    GimpOrientationType  flip_type,
                    gdouble              axis,
                    gboolean             clip_result)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglBuffer   *buffer;
  gint          off_x, off_y;
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_flip (drawable, context,
                                                gimp_drawable_get_buffer (drawable),
                                                off_x, off_y,
                                                flip_type, axis,
                                                clip_result,
                                                &new_off_x, &new_off_y);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer,
                                     new_off_x, new_off_y, FALSE);
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
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglBuffer   *buffer;
  gint          off_x, off_y;
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_rotate (drawable, context,
                                                  gimp_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  rotate_type, center_x, center_y,
                                                  clip_result,
                                                  &new_off_x, &new_off_y);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer,
                                     new_off_x, new_off_y, FALSE);
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
                         GimpProgress           *progress)
{
  GimpDrawable *drawable = GIMP_DRAWABLE (item);
  GeglBuffer   *buffer;
  gint          off_x, off_y;
  gint          new_off_x, new_off_y;

  gimp_item_get_offset (item, &off_x, &off_y);

  buffer = gimp_drawable_transform_buffer_affine (drawable, context,
                                                  gimp_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  matrix, direction,
                                                  interpolation_type,
                                                  clip_result,
                                                  &new_off_x, &new_off_y,
                                                  progress);

  if (buffer)
    {
      gimp_drawable_transform_paste (drawable, buffer,
                                     new_off_x, new_off_y, FALSE);
      g_object_unref (buffer);
    }
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
gimp_drawable_real_update (GimpDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  gimp_viewable_invalidate_preview (GIMP_VIEWABLE (drawable));
}

static gint64
gimp_drawable_real_estimate_memsize (const GimpDrawable *drawable,
                                     GimpComponentType   component_type,
                                     gint                width,
                                     gint                height)
{
  GimpImage  *image  = gimp_item_get_image (GIMP_ITEM (drawable));
  gboolean    linear = gimp_drawable_get_linear (drawable);
  const Babl *format;

  format = gimp_image_get_format (image,
                                  gimp_drawable_get_base_type (drawable),
                                  gimp_babl_precision (component_type, linear),
                                  gimp_drawable_has_alpha (drawable));

  return (gint64) babl_format_get_bytes_per_pixel (format) * width * height;
}

/* FIXME: this default impl is currently unused because no subclass
 * chins up. the goal is to handle the almost identical subclass code
 * here again.
 */
static void
gimp_drawable_real_convert_type (GimpDrawable      *drawable,
                                 GimpImage         *dest_image,
                                 const Babl        *new_format,
                                 GimpImageBaseType  new_base_type,
                                 GimpPrecision      new_precision,
                                 gint               layer_dither_type,
                                 gint               mask_dither_type,
                                 gboolean           convert_type,
                                 gboolean           push_undo,
                                 GimpProgress      *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     gimp_item_get_width  (GIMP_ITEM (drawable)),
                                     gimp_item_get_height (GIMP_ITEM (drawable))),
                     new_format);

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable), NULL, GEGL_ABYSS_NONE,
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
gimp_drawable_real_set_buffer (GimpDrawable *drawable,
                               gboolean      push_undo,
                               const gchar  *undo_desc,
                               GeglBuffer   *buffer,
                               gint          offset_x,
                               gint          offset_y)
{
  GimpItem *item = GIMP_ITEM (drawable);
  gboolean  old_has_alpha;

  old_has_alpha = gimp_drawable_has_alpha (drawable);

  g_object_freeze_notify (G_OBJECT (drawable));

  gimp_drawable_invalidate_boundary (drawable);

  if (push_undo)
    gimp_image_undo_push_drawable_mod (gimp_item_get_image (item), undo_desc,
                                       drawable, FALSE);

  /*  ref new before unrefing old, they might be the same  */
  g_object_ref (buffer);

  if (drawable->private->buffer)
    g_object_unref (drawable->private->buffer);

  drawable->private->buffer = buffer;

  if (drawable->private->buffer_source_node)
    gegl_node_set (drawable->private->buffer_source_node,
                   "buffer", gimp_drawable_get_buffer (drawable),
                   NULL);

  gimp_item_set_offset (item, offset_x, offset_y);
  gimp_item_set_size (item,
                      gegl_buffer_get_width  (buffer),
                      gegl_buffer_get_height (buffer));

  if (old_has_alpha != gimp_drawable_has_alpha (drawable))
    gimp_drawable_alpha_changed (drawable);

  g_object_notify (G_OBJECT (drawable), "buffer");

  g_object_thaw_notify (G_OBJECT (drawable));
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
  if (! buffer)
    {
      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height),
                                gimp_drawable_get_format (drawable));

      gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                        GEGL_RECTANGLE (x, y, width, height), GEGL_ABYSS_NONE,
                        buffer,
                        GEGL_RECTANGLE (0, 0, 0, 0));
    }
  else
    {
      g_object_ref (buffer);
    }

  gimp_image_undo_push_drawable (gimp_item_get_image (GIMP_ITEM (drawable)),
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

  tmp = gegl_buffer_dup (buffer);

  gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                    GEGL_RECTANGLE (x, y, width, height), GEGL_ABYSS_NONE,
                    buffer,
                    GEGL_RECTANGLE (0, 0, 0, 0));
  gegl_buffer_copy (tmp,
                    GEGL_RECTANGLE (0, 0, width, height), GEGL_ABYSS_NONE,
                    gimp_drawable_get_buffer (drawable),
                    GEGL_RECTANGLE (x, y, 0, 0));

  g_object_unref (tmp);

  gimp_drawable_update (drawable, x, y, width, height);
}

static void
gimp_drawable_sync_fs_filter (GimpDrawable *drawable,
                              gboolean      detach_fs)
{
  GimpDrawablePrivate *private = drawable->private;
  GimpImage           *image   = gimp_item_get_image (GIMP_ITEM (drawable));
  GimpLayer           *fs      = gimp_drawable_get_floating_sel (drawable);

  if (! private->source_node)
    return;

  if (fs && ! detach_fs)
    {
      GimpImage   *image = gimp_item_get_image (GIMP_ITEM (drawable));
      GimpChannel *mask  = gimp_image_get_mask (image);
      gint         off_x, off_y;
      gint         fs_off_x, fs_off_y;

      if (! private->fs_filter)
        {
          GeglNode *node;
          GeglNode *fs_source;
          gboolean  linear;

          private->fs_filter = gimp_filter_new ("Floating Selection");
          gimp_viewable_set_icon_name (GIMP_VIEWABLE (private->fs_filter),
                                       "gimp-floating-selection");

          node = gimp_filter_get_node (private->fs_filter);

          fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));
          linear    = gimp_drawable_get_linear (GIMP_DRAWABLE (fs));

          /* rip the fs' source node out of its graph */
          if (fs->layer_offset_node)
            {
              gegl_node_disconnect (fs->layer_offset_node, "input");
              gegl_node_remove_child (gimp_filter_get_node (GIMP_FILTER (fs)),
                                      fs_source);
            }

          gegl_node_add_child (node, fs_source);

          private->fs_applicator = gimp_applicator_new (node, linear,
                                                        FALSE, FALSE);

          private->fs_crop_node =
            gegl_node_new_child (node,
                                 "operation", "gegl:crop",
                                 NULL);

          gegl_node_connect_to (fs_source,             "output",
                                private->fs_crop_node, "input");
          gegl_node_connect_to (private->fs_crop_node, "output",
                                node,                  "aux");

          gimp_drawable_add_filter (drawable, private->fs_filter);

          g_signal_connect (fs, "notify",
                            G_CALLBACK (gimp_drawable_fs_notify),
                            drawable);
          g_signal_connect (image, "component-active-changed",
                            G_CALLBACK (gimp_drawable_fs_affect_changed),
                            drawable);
          g_signal_connect (image, "mask-changed",
                            G_CALLBACK (gimp_drawable_fs_mask_changed),
                            drawable);
        }

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);

      gegl_node_set (private->fs_crop_node,
                     "x",      (gdouble) (off_x - fs_off_x),
                     "y",      (gdouble) (off_y - fs_off_y),
                     "width",  (gdouble) gimp_item_get_width  (GIMP_ITEM (drawable)),
                     "height", (gdouble) gimp_item_get_height (GIMP_ITEM (drawable)),
                     NULL);

      gimp_applicator_set_apply_offset (private->fs_applicator,
                                        fs_off_x - off_x,
                                        fs_off_y - off_y);

      if (gimp_channel_is_empty (mask))
        {
          gimp_applicator_set_mask_buffer (private->fs_applicator, NULL);
        }
      else
        {
          GeglBuffer *buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

          gimp_applicator_set_mask_buffer (private->fs_applicator, buffer);
          gimp_applicator_set_mask_offset (private->fs_applicator,
                                           -off_x, -off_y);
        }

      gimp_applicator_set_mode (private->fs_applicator,
                                gimp_layer_get_opacity (fs),
                                gimp_layer_get_mode (fs));
      gimp_applicator_set_affect (private->fs_applicator,
                                  gimp_drawable_get_active_mask (drawable));
    }
  else
    {
      if (private->fs_filter)
        {
          GeglNode *node;
          GeglNode *fs_source;

          g_signal_handlers_disconnect_by_func (fs,
                                                gimp_drawable_fs_notify,
                                                drawable);
          g_signal_handlers_disconnect_by_func (image,
                                                gimp_drawable_fs_affect_changed,
                                                drawable);
          g_signal_handlers_disconnect_by_func (image,
                                                gimp_drawable_fs_mask_changed,
                                                drawable);

          gimp_drawable_remove_filter (drawable, private->fs_filter);

          node = gimp_filter_get_node (private->fs_filter);

          fs_source = gimp_drawable_get_source_node (GIMP_DRAWABLE (fs));

          gegl_node_remove_child (node, fs_source);

          /* plug the fs' source node back into its graph */
          if (fs->layer_offset_node)
            {
              gegl_node_add_child (gimp_filter_get_node (GIMP_FILTER (fs)),
                                   fs_source);
              gegl_node_connect_to (fs_source,             "output",
                                    fs->layer_offset_node, "input");
            }

          g_object_unref (private->fs_filter);
          private->fs_filter = NULL;

          g_object_unref (private->fs_applicator);
          private->fs_applicator = NULL;

          private->fs_crop_node = NULL;
        }
    }
}

static void
gimp_drawable_fs_notify (GimpLayer        *fs,
                         const GParamSpec *pspec,
                         GimpDrawable     *drawable)
{
  if (! strcmp (pspec->name, "offset-x") ||
      ! strcmp (pspec->name, "offset-y") ||
      ! strcmp (pspec->name, "visible")  ||
      ! strcmp (pspec->name, "mode")     ||
      ! strcmp (pspec->name, "opacity"))
    {
      gimp_drawable_sync_fs_filter (drawable, FALSE);
    }
}

static void
gimp_drawable_fs_affect_changed (GimpImage       *image,
                                 GimpChannelType  channel,
                                 GimpDrawable    *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable, FALSE);

  gimp_drawable_update (GIMP_DRAWABLE (fs),
                        0, 0,
                        gimp_item_get_width  (GIMP_ITEM (fs)),
                        gimp_item_get_height (GIMP_ITEM (fs)));
}

static void
gimp_drawable_fs_mask_changed (GimpImage       *image,
                               GimpDrawable    *drawable)
{
  GimpLayer *fs = gimp_drawable_get_floating_sel (drawable);

  gimp_drawable_sync_fs_filter (drawable, FALSE);

  gimp_drawable_update (GIMP_DRAWABLE (fs),
                        0, 0,
                        gimp_item_get_width  (GIMP_ITEM (fs)),
                        gimp_item_get_height (GIMP_ITEM (fs)));
}

static void
gimp_drawable_fs_update (GimpLayer    *fs,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height,
                         GimpDrawable *drawable)
{
  gint fs_off_x, fs_off_y;
  gint off_x, off_y;
  gint dr_x, dr_y, dr_width, dr_height;

  gimp_item_get_offset (GIMP_ITEM (fs), &fs_off_x, &fs_off_y);
  gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

  if (gimp_rectangle_intersect (x + fs_off_x,
                                y + fs_off_y,
                                width,
                                height,
                                off_x,
                                off_y,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &dr_x,
                                &dr_y,
                                &dr_width,
                                &dr_height))
    {
      gimp_drawable_update (drawable,
                            dr_x - off_x, dr_y - off_y,
                            dr_width, dr_height);
    }
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

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (type, GIMP_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  drawable = GIMP_DRAWABLE (gimp_item_new (type,
                                           image, name,
                                           offset_x, offset_y,
                                           width, height));

  drawable->private->buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                               width, height),
                                               format);

  return drawable;
}

gint64
gimp_drawable_estimate_memsize (const GimpDrawable *drawable,
                                GimpComponentType   component_type,
                                gint                width,
                                gint                height)
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

  g_signal_emit (drawable, gimp_drawable_signals[UPDATE], 0,
                 x, y, width, height);
}

void
gimp_drawable_alpha_changed (GimpDrawable *drawable)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));

  g_signal_emit (drawable, gimp_drawable_signals[ALPHA_CHANGED], 0);
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
gimp_drawable_get_active_components (const GimpDrawable *drawable,
                                     gboolean           *active)
{
  GimpDrawableClass *drawable_class;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (active != NULL);

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->get_active_components)
    drawable_class->get_active_components (drawable, active);
}

GimpComponentMask
gimp_drawable_get_active_mask (const GimpDrawable *drawable)
{
  GimpDrawableClass *drawable_class;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), 0);

  drawable_class = GIMP_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->get_active_mask)
    return drawable_class->get_active_mask (drawable);

  return 0;
}

void
gimp_drawable_convert_type (GimpDrawable      *drawable,
                            GimpImage         *dest_image,
                            GimpImageBaseType  new_base_type,
                            GimpPrecision      new_precision,
                            gint               layer_dither_type,
                            gint               mask_dither_type,
                            gboolean           convert_profile,
                            gboolean           push_undo,
                            GimpProgress      *progress)
{
  const Babl *new_format;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_IMAGE (dest_image));
  g_return_if_fail (new_base_type != gimp_drawable_get_base_type (drawable) ||
                    new_precision != gimp_drawable_get_precision (drawable) ||
                    convert_profile);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  new_format = gimp_image_get_format (dest_image,
                                      new_base_type,
                                      new_precision,
                                      gimp_drawable_has_alpha (drawable));

  GIMP_DRAWABLE_GET_CLASS (drawable)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    new_base_type,
                                                    new_precision,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    convert_profile,
                                                    push_undo,
                                                    progress);

  if (progress)
    gimp_progress_set_value (progress, 1.0);
}

void
gimp_drawable_apply_buffer (GimpDrawable         *drawable,
                            GeglBuffer           *buffer,
                            const GeglRectangle  *buffer_region,
                            gboolean              push_undo,
                            const gchar          *undo_desc,
                            gdouble               opacity,
                            GimpLayerModeEffects  mode,
                            GeglBuffer           *base_buffer,
                            gint                  base_x,
                            gint                  base_y)
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
                                                    base_buffer,
                                                    base_x, base_y);
}

void
gimp_drawable_replace_buffer (GimpDrawable        *drawable,
                              GeglBuffer          *buffer,
                              const GeglRectangle *buffer_region,
                              gboolean             push_undo,
                              const gchar         *undo_desc,
                              gdouble              opacity,
                              GeglBuffer          *mask,
                              const GeglRectangle *mask_region,
                              gint                 x,
                              gint                 y)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (GEGL_IS_BUFFER (mask));

  GIMP_DRAWABLE_GET_CLASS (drawable)->replace_buffer (drawable, buffer,
                                                      buffer_region,
                                                      push_undo, undo_desc,
                                                      opacity,
                                                      mask, mask_region,
                                                      x, y);
}

GeglBuffer *
gimp_drawable_get_buffer (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return GIMP_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);
}

void
gimp_drawable_set_buffer (GimpDrawable *drawable,
                          gboolean      push_undo,
                          const gchar  *undo_desc,
                          GeglBuffer   *buffer)
{
  gint offset_x, offset_y;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  gimp_item_get_offset (GIMP_ITEM (drawable), &offset_x, &offset_y);

  gimp_drawable_set_buffer_full (drawable, push_undo, undo_desc, buffer,
                                 offset_x, offset_y);
}

void
gimp_drawable_set_buffer_full (GimpDrawable *drawable,
                               gboolean      push_undo,
                               const gchar  *undo_desc,
                               GeglBuffer   *buffer,
                               gint          offset_x,
                               gint          offset_y)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  item = GIMP_ITEM (drawable);

  if (! gimp_item_is_attached (GIMP_ITEM (drawable)))
    push_undo = FALSE;

  if (gimp_item_get_width  (item)   != gegl_buffer_get_width (buffer)  ||
      gimp_item_get_height (item)   != gegl_buffer_get_height (buffer) ||
      gimp_item_get_offset_x (item) != offset_x                        ||
      gimp_item_get_offset_y (item) != offset_y)
    {
      gimp_drawable_update (drawable,
                            0, 0,
                            gimp_item_get_width  (item),
                            gimp_item_get_height (item));
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  GIMP_DRAWABLE_GET_CLASS (drawable)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer,
                                                  offset_x, offset_y);

  g_object_thaw_notify (G_OBJECT (drawable));

  gimp_drawable_update (drawable,
                        0, 0,
                        gimp_item_get_width  (item),
                        gimp_item_get_height (item));
}

GeglNode *
gimp_drawable_get_source_node (GimpDrawable *drawable)
{
  GeglNode *filter;
  GeglNode *output;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->source_node)
    return drawable->private->source_node;

  drawable->private->source_node = gegl_node_new ();

  drawable->private->buffer_source_node =
    gegl_node_new_child (drawable->private->source_node,
                         "operation", "gegl:buffer-source",
                         "buffer",    gimp_drawable_get_buffer (drawable),
                         NULL);

  filter = gimp_filter_stack_get_graph (GIMP_FILTER_STACK (drawable->private->filter_stack));

  gegl_node_add_child (drawable->private->source_node, filter);

  gegl_node_connect_to (drawable->private->buffer_source_node, "output",
                        filter,                                "input");

  output = gegl_node_get_output_proxy (drawable->private->source_node, "output");

  gegl_node_connect_to (filter, "output",
                        output, "input");

  gimp_drawable_sync_fs_filter (drawable, FALSE);

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

const Babl *
gimp_drawable_get_format (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gegl_buffer_get_format (drawable->private->buffer);
}

const Babl *
gimp_drawable_get_format_with_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gimp_image_get_format (gimp_item_get_image (GIMP_ITEM (drawable)),
                                gimp_drawable_get_base_type (drawable),
                                gimp_drawable_get_precision (drawable),
                                TRUE);
}

const Babl *
gimp_drawable_get_format_without_alpha (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return gimp_image_get_format (gimp_item_get_image (GIMP_ITEM (drawable)),
                                gimp_drawable_get_base_type (drawable),
                                gimp_drawable_get_precision (drawable),
                                FALSE);
}

gboolean
gimp_drawable_get_linear (const GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_linear (format);
}

gboolean
gimp_drawable_has_alpha (const GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return babl_format_has_alpha (format);
}

GimpImageBaseType
gimp_drawable_get_base_type (const GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_base_type (format);
}

GimpComponentType
gimp_drawable_get_component_type (const GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_component_type (format);
}

GimpPrecision
gimp_drawable_get_precision (const GimpDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return gimp_babl_format_get_precision (format);
}

gboolean
gimp_drawable_is_rgb (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_RGB);
}

gboolean
gimp_drawable_is_gray (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_GRAY);
}

gboolean
gimp_drawable_is_indexed (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  return (gimp_drawable_get_base_type (drawable) == GIMP_INDEXED);
}

const Babl *
gimp_drawable_get_component_format (const GimpDrawable *drawable,
                                    GimpChannelType     channel)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         RED);

    case GIMP_GREEN_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         GREEN);

    case GIMP_BLUE_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         BLUE);

    case GIMP_ALPHA_CHANNEL:
      return gimp_babl_component_format (GIMP_RGB,
                                         gimp_drawable_get_precision (drawable),
                                         ALPHA);

    case GIMP_GRAY_CHANNEL:
      return gimp_babl_component_format (GIMP_GRAY,
                                         gimp_drawable_get_precision (drawable),
                                         GRAY);

    case GIMP_INDEXED_CHANNEL:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
gimp_drawable_get_component_index (const GimpDrawable *drawable,
                                   GimpChannelType     channel)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), -1);

  switch (channel)
    {
    case GIMP_RED_CHANNEL:     return RED;
    case GIMP_GREEN_CHANNEL:   return GREEN;
    case GIMP_BLUE_CHANNEL:    return BLUE;
    case GIMP_GRAY_CHANNEL:    return GRAY;
    case GIMP_INDEXED_CHANNEL: return INDEXED;
    case GIMP_ALPHA_CHANNEL:
      switch (gimp_drawable_get_base_type (drawable))
        {
        case GIMP_RGB:     return ALPHA;
        case GIMP_GRAY:    return ALPHA_G;
        case GIMP_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

const guchar *
gimp_drawable_get_colormap (const GimpDrawable *drawable)
{
  GimpImage *image;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  return image ? gimp_image_get_colormap (image) : NULL;
}

GimpLayer *
gimp_drawable_get_floating_sel (const GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->private->floating_selection;
}

void
gimp_drawable_attach_floating_sel (GimpDrawable *drawable,
                                   GimpLayer    *fs)
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) == NULL);
  g_return_if_fail (GIMP_IS_LAYER (fs));

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  drawable->private->floating_selection = fs;
  gimp_image_set_floating_selection (image, fs);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fs));

  gimp_drawable_sync_fs_filter (drawable, FALSE);

  g_signal_connect (fs, "update",
                    G_CALLBACK (gimp_drawable_fs_update),
                    drawable);

  gimp_drawable_fs_update (fs,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (fs)),
                           gimp_item_get_height (GIMP_ITEM (fs)),
                           drawable);
}

void
gimp_drawable_detach_floating_sel (GimpDrawable *drawable)
{
  GimpImage *image;
  GimpLayer *fs;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_get_floating_sel (drawable) != NULL);

  GIMP_LOG (FLOATING_SELECTION, "%s", G_STRFUNC);

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  fs    = drawable->private->floating_selection;

  gimp_drawable_sync_fs_filter (drawable, TRUE);

  g_signal_handlers_disconnect_by_func (fs,
                                        gimp_drawable_fs_update,
                                        drawable);

  gimp_drawable_fs_update (fs,
                           0, 0,
                           gimp_item_get_width  (GIMP_ITEM (fs)),
                           gimp_item_get_height (GIMP_ITEM (fs)),
                           drawable);

  /*  clear the selection  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (fs));

  gimp_image_set_floating_selection (image, NULL);
  drawable->private->floating_selection = NULL;
}

GimpFilter *
gimp_drawable_get_floating_sel_filter (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_drawable_get_floating_sel (drawable) != NULL, NULL);

  /*
   * Ensure that the graph is construced before the filter is used.
   * Otherwise, we rely on the projection to cause the graph to be constructed,
   * which fails for images that aren't displayed.
   */
  gimp_filter_get_node (GIMP_FILTER (drawable));

  return drawable->private->fs_filter;
}
