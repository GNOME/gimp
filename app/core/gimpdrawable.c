/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"
#include "gegl/ligma-gegl-utils.h"

#include "ligma-memsize.h"
#include "ligma-utils.h"
#include "ligmachannel.h"
#include "ligmacontext.h"
#include "ligmadrawable-combine.h"
#include "ligmadrawable-fill.h"
#include "ligmadrawable-floating-selection.h"
#include "ligmadrawable-preview.h"
#include "ligmadrawable-private.h"
#include "ligmadrawable-shadow.h"
#include "ligmadrawable-transform.h"
#include "ligmafilterstack.h"
#include "ligmaimage.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-undo-push.h"
#include "ligmamarshal.h"
#include "ligmapickable.h"
#include "ligmaprogress.h"

#include "ligma-log.h"

#include "ligma-intl.h"


#define PAINT_UPDATE_CHUNK_WIDTH  32
#define PAINT_UPDATE_CHUNK_HEIGHT 32


enum
{
  UPDATE,
  FORMAT_CHANGED,
  ALPHA_CHANGED,
  BOUNDING_BOX_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_BUFFER
};


/*  local function prototypes  */

static void       ligma_color_managed_iface_init    (LigmaColorManagedInterface *iface);
static void       ligma_pickable_iface_init         (LigmaPickableInterface     *iface);

static void       ligma_drawable_dispose            (GObject           *object);
static void       ligma_drawable_finalize           (GObject           *object);
static void       ligma_drawable_set_property       (GObject           *object,
                                                    guint              property_id,
                                                    const GValue      *value,
                                                    GParamSpec        *pspec);
static void       ligma_drawable_get_property       (GObject           *object,
                                                    guint              property_id,
                                                    GValue            *value,
                                                    GParamSpec        *pspec);

static gint64     ligma_drawable_get_memsize        (LigmaObject        *object,
                                                    gint64            *gui_size);

static gboolean   ligma_drawable_get_size           (LigmaViewable      *viewable,
                                                    gint              *width,
                                                    gint              *height);
static void       ligma_drawable_preview_freeze     (LigmaViewable      *viewable);
static void       ligma_drawable_preview_thaw       (LigmaViewable      *viewable);

static GeglNode * ligma_drawable_get_node           (LigmaFilter        *filter);

static void       ligma_drawable_removed            (LigmaItem          *item);
static LigmaItem * ligma_drawable_duplicate          (LigmaItem          *item,
                                                    GType              new_type);
static void       ligma_drawable_scale              (LigmaItem          *item,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               new_offset_x,
                                                    gint               new_offset_y,
                                                    LigmaInterpolationType interp_type,
                                                    LigmaProgress      *progress);
static void       ligma_drawable_resize             (LigmaItem          *item,
                                                    LigmaContext       *context,
                                                    LigmaFillType       fill_type,
                                                    gint               new_width,
                                                    gint               new_height,
                                                    gint               offset_x,
                                                    gint               offset_y);
static void       ligma_drawable_flip               (LigmaItem          *item,
                                                    LigmaContext       *context,
                                                    LigmaOrientationType  flip_type,
                                                    gdouble            axis,
                                                    gboolean           clip_result);
static void       ligma_drawable_rotate             (LigmaItem          *item,
                                                    LigmaContext       *context,
                                                    LigmaRotationType   rotate_type,
                                                    gdouble            center_x,
                                                    gdouble            center_y,
                                                    gboolean           clip_result);
static void       ligma_drawable_transform          (LigmaItem          *item,
                                                    LigmaContext       *context,
                                                    const LigmaMatrix3 *matrix,
                                                    LigmaTransformDirection direction,
                                                    LigmaInterpolationType interpolation_type,
                                                    LigmaTransformResize clip_result,
                                                    LigmaProgress      *progress);

static const guint8 *
                  ligma_drawable_get_icc_profile    (LigmaColorManaged  *managed,
                                                    gsize             *len);
static LigmaColorProfile *
                  ligma_drawable_get_color_profile  (LigmaColorManaged  *managed);
static void       ligma_drawable_profile_changed    (LigmaColorManaged  *managed);

static gboolean   ligma_drawable_get_pixel_at       (LigmaPickable      *pickable,
                                                    gint               x,
                                                    gint               y,
                                                    const Babl        *format,
                                                    gpointer           pixel);
static void       ligma_drawable_get_pixel_average  (LigmaPickable      *pickable,
                                                    const GeglRectangle *rect,
                                                    const Babl        *format,
                                                    gpointer           pixel);

static void       ligma_drawable_real_update        (LigmaDrawable      *drawable,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);

static gint64  ligma_drawable_real_estimate_memsize (LigmaDrawable      *drawable,
                                                    LigmaComponentType  component_type,
                                                    gint               width,
                                                    gint               height);

static void       ligma_drawable_real_update_all    (LigmaDrawable      *drawable);

static LigmaComponentMask
                ligma_drawable_real_get_active_mask (LigmaDrawable      *drawable);

static gboolean   ligma_drawable_real_supports_alpha
                                                   (LigmaDrawable     *drawable);

static void       ligma_drawable_real_convert_type  (LigmaDrawable      *drawable,
                                                    LigmaImage         *dest_image,
                                                    const Babl        *new_format,
                                                    LigmaColorProfile  *src_profile,
                                                    LigmaColorProfile  *dest_profile,
                                                    GeglDitherMethod   layer_dither_type,
                                                    GeglDitherMethod   mask_dither_type,
                                                    gboolean           push_undo,
                                                    LigmaProgress      *progress);

static GeglBuffer * ligma_drawable_real_get_buffer  (LigmaDrawable      *drawable);
static void       ligma_drawable_real_set_buffer    (LigmaDrawable      *drawable,
                                                    gboolean           push_undo,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    const GeglRectangle *bounds);

static GeglRectangle ligma_drawable_real_get_bounding_box
                                                   (LigmaDrawable      *drawable);

static void       ligma_drawable_real_push_undo     (LigmaDrawable      *drawable,
                                                    const gchar       *undo_desc,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y,
                                                    gint               width,
                                                    gint               height);
static void       ligma_drawable_real_swap_pixels   (LigmaDrawable      *drawable,
                                                    GeglBuffer        *buffer,
                                                    gint               x,
                                                    gint               y);
static GeglNode * ligma_drawable_real_get_source_node (LigmaDrawable    *drawable);

static void       ligma_drawable_format_changed     (LigmaDrawable      *drawable);
static void       ligma_drawable_alpha_changed      (LigmaDrawable      *drawable);


G_DEFINE_TYPE_WITH_CODE (LigmaDrawable, ligma_drawable, LIGMA_TYPE_ITEM,
                         G_ADD_PRIVATE (LigmaDrawable)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_COLOR_MANAGED,
                                                ligma_color_managed_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_pickable_iface_init))

#define parent_class ligma_drawable_parent_class

static guint ligma_drawable_signals[LAST_SIGNAL] = { 0 };


static void
ligma_drawable_class_init (LigmaDrawableClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaFilterClass   *filter_class      = LIGMA_FILTER_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);

  ligma_drawable_signals[UPDATE] =
    g_signal_new ("update",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableClass, update),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  ligma_drawable_signals[FORMAT_CHANGED] =
    g_signal_new ("format-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableClass, format_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_drawable_signals[ALPHA_CHANGED] =
    g_signal_new ("alpha-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableClass, alpha_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  ligma_drawable_signals[BOUNDING_BOX_CHANGED] =
    g_signal_new ("bounding-box-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaDrawableClass, bounding_box_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->dispose           = ligma_drawable_dispose;
  object_class->finalize          = ligma_drawable_finalize;
  object_class->set_property      = ligma_drawable_set_property;
  object_class->get_property      = ligma_drawable_get_property;

  ligma_object_class->get_memsize  = ligma_drawable_get_memsize;

  viewable_class->get_size        = ligma_drawable_get_size;
  viewable_class->get_new_preview = ligma_drawable_get_new_preview;
  viewable_class->get_new_pixbuf  = ligma_drawable_get_new_pixbuf;
  viewable_class->preview_freeze  = ligma_drawable_preview_freeze;
  viewable_class->preview_thaw    = ligma_drawable_preview_thaw;

  filter_class->get_node          = ligma_drawable_get_node;

  item_class->removed             = ligma_drawable_removed;
  item_class->duplicate           = ligma_drawable_duplicate;
  item_class->scale               = ligma_drawable_scale;
  item_class->resize              = ligma_drawable_resize;
  item_class->flip                = ligma_drawable_flip;
  item_class->rotate              = ligma_drawable_rotate;
  item_class->transform           = ligma_drawable_transform;

  klass->update                   = ligma_drawable_real_update;
  klass->format_changed           = NULL;
  klass->alpha_changed            = NULL;
  klass->bounding_box_changed     = NULL;
  klass->estimate_memsize         = ligma_drawable_real_estimate_memsize;
  klass->update_all               = ligma_drawable_real_update_all;
  klass->invalidate_boundary      = NULL;
  klass->get_active_components    = NULL;
  klass->get_active_mask          = ligma_drawable_real_get_active_mask;
  klass->supports_alpha           = ligma_drawable_real_supports_alpha;
  klass->convert_type             = ligma_drawable_real_convert_type;
  klass->apply_buffer             = ligma_drawable_real_apply_buffer;
  klass->get_buffer               = ligma_drawable_real_get_buffer;
  klass->set_buffer               = ligma_drawable_real_set_buffer;
  klass->get_bounding_box         = ligma_drawable_real_get_bounding_box;
  klass->push_undo                = ligma_drawable_real_push_undo;
  klass->swap_pixels              = ligma_drawable_real_swap_pixels;
  klass->get_source_node          = ligma_drawable_real_get_source_node;

  g_object_class_override_property (object_class, PROP_BUFFER, "buffer");
}

static void
ligma_drawable_init (LigmaDrawable *drawable)
{
  drawable->private = ligma_drawable_get_instance_private (drawable);

  drawable->private->filter_stack = ligma_filter_stack_new (LIGMA_TYPE_FILTER);
}

/* sorry for the evil casts */

static void
ligma_color_managed_iface_init (LigmaColorManagedInterface *iface)
{
  iface->get_icc_profile   = ligma_drawable_get_icc_profile;
  iface->get_color_profile = ligma_drawable_get_color_profile;
  iface->profile_changed   = ligma_drawable_profile_changed;
}

static void
ligma_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->get_image             = (LigmaImage     * (*) (LigmaPickable *pickable)) ligma_item_get_image;
  iface->get_format            = (const Babl    * (*) (LigmaPickable *pickable)) ligma_drawable_get_format;
  iface->get_format_with_alpha = (const Babl    * (*) (LigmaPickable *pickable)) ligma_drawable_get_format_with_alpha;
  iface->get_buffer            = (GeglBuffer    * (*) (LigmaPickable *pickable)) ligma_drawable_get_buffer;
  iface->get_pixel_at          = ligma_drawable_get_pixel_at;
  iface->get_pixel_average     = ligma_drawable_get_pixel_average;
}

static void
ligma_drawable_dispose (GObject *object)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (object);

  if (ligma_drawable_get_floating_sel (drawable))
    ligma_drawable_detach_floating_sel (drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_drawable_finalize (GObject *object)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (object);

  while (drawable->private->paint_count)
    ligma_drawable_end_paint (drawable);

  g_clear_object (&drawable->private->buffer);
  g_clear_object (&drawable->private->format_profile);

  ligma_drawable_free_shadow_buffer (drawable);

  g_clear_object (&drawable->private->source_node);
  g_clear_object (&drawable->private->buffer_source_node);
  g_clear_object (&drawable->private->filter_stack);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_drawable_set_property (GObject      *object,
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
ligma_drawable_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (object);

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
ligma_drawable_get_memsize (LigmaObject *object,
                           gint64     *gui_size)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (object);
  gint64        memsize  = 0;

  memsize += ligma_gegl_buffer_get_memsize (ligma_drawable_get_buffer (drawable));
  memsize += ligma_gegl_buffer_get_memsize (drawable->private->shadow);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_drawable_get_size (LigmaViewable *viewable,
                        gint         *width,
                        gint         *height)
{
  LigmaItem *item = LIGMA_ITEM (viewable);

  *width  = ligma_item_get_width  (item);
  *height = ligma_item_get_height (item);

  return TRUE;
}

static void
ligma_drawable_preview_freeze (LigmaViewable *viewable)
{
  LigmaViewable *parent = ligma_viewable_get_parent (viewable);

  if (! parent && ligma_item_is_attached (LIGMA_ITEM (viewable)))
    parent = LIGMA_VIEWABLE (ligma_item_get_image (LIGMA_ITEM (viewable)));

  if (parent)
    ligma_viewable_preview_freeze (parent);
}

static void
ligma_drawable_preview_thaw (LigmaViewable *viewable)
{
  LigmaViewable *parent = ligma_viewable_get_parent (viewable);

  if (! parent && ligma_item_is_attached (LIGMA_ITEM (viewable)))
    parent = LIGMA_VIEWABLE (ligma_item_get_image (LIGMA_ITEM (viewable)));

  if (parent)
    ligma_viewable_preview_thaw (parent);
}

static GeglNode *
ligma_drawable_get_node (LigmaFilter *filter)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (filter);
  GeglNode     *node;
  GeglNode     *input;
  GeglNode     *output;

  node = LIGMA_FILTER_CLASS (parent_class)->get_node (filter);

  g_warn_if_fail (drawable->private->mode_node == NULL);

  drawable->private->mode_node =
    gegl_node_new_child (node,
                         "operation", "ligma:normal",
                         NULL);

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  gegl_node_connect_to (input,                        "output",
                        drawable->private->mode_node, "input");
  gegl_node_connect_to (drawable->private->mode_node, "output",
                        output,                       "input");

  return node;
}

static void
ligma_drawable_removed (LigmaItem *item)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (item);

  ligma_drawable_free_shadow_buffer (drawable);

  if (LIGMA_ITEM_CLASS (parent_class)->removed)
    LIGMA_ITEM_CLASS (parent_class)->removed (item);
}

static LigmaItem *
ligma_drawable_duplicate (LigmaItem *item,
                         GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_DRAWABLE (new_item))
    {
      LigmaDrawable  *drawable     = LIGMA_DRAWABLE (item);
      LigmaDrawable  *new_drawable = LIGMA_DRAWABLE (new_item);
      GeglBuffer    *new_buffer;

      new_buffer = ligma_gegl_buffer_dup (ligma_drawable_get_buffer (drawable));

      ligma_drawable_set_buffer (new_drawable, FALSE, NULL, new_buffer);
      g_object_unref (new_buffer);
    }

  return new_item;
}

static void
ligma_drawable_scale (LigmaItem              *item,
                     gint                   new_width,
                     gint                   new_height,
                     gint                   new_offset_x,
                     gint                   new_offset_y,
                     LigmaInterpolationType  interpolation_type,
                     LigmaProgress          *progress)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (item);
  GeglBuffer   *new_buffer;

  new_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                new_width, new_height),
                                ligma_drawable_get_format (drawable));

  ligma_gegl_apply_scale (ligma_drawable_get_buffer (drawable),
                         progress, C_("undo-type", "Scale"),
                         new_buffer,
                         interpolation_type,
                         ((gdouble) new_width /
                          ligma_item_get_width  (item)),
                         ((gdouble) new_height /
                          ligma_item_get_height (item)));

  ligma_drawable_set_buffer_full (drawable, ligma_item_is_attached (item), NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);
}

static void
ligma_drawable_resize (LigmaItem     *item,
                      LigmaContext  *context,
                      LigmaFillType  fill_type,
                      gint          new_width,
                      gint          new_height,
                      gint          offset_x,
                      gint          offset_y)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (item);
  GeglBuffer   *new_buffer;
  gint          new_offset_x;
  gint          new_offset_y;
  gint          copy_x, copy_y;
  gint          copy_width, copy_height;
  gboolean      intersect;

  /*  if the size doesn't change, this is a nop  */
  if (new_width  == ligma_item_get_width  (item) &&
      new_height == ligma_item_get_height (item) &&
      offset_x   == 0                           &&
      offset_y   == 0)
    return;

  new_offset_x = ligma_item_get_offset_x (item) - offset_x;
  new_offset_y = ligma_item_get_offset_y (item) - offset_y;

  intersect = ligma_rectangle_intersect (ligma_item_get_offset_x (item),
                                        ligma_item_get_offset_y (item),
                                        ligma_item_get_width (item),
                                        ligma_item_get_height (item),
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
                                ligma_drawable_get_format (drawable));

  if (! intersect              ||
      copy_width  != new_width ||
      copy_height != new_height)
    {
      /*  Clear the new buffer if needed  */

      LigmaRGB      color;
      LigmaPattern *pattern;

      ligma_get_fill_params (context, fill_type, &color, &pattern, NULL);
      ligma_drawable_fill_buffer (drawable, new_buffer,
                                 &color, pattern, 0, 0);
    }

  if (intersect && copy_width && copy_height)
    {
      /*  Copy the pixels in the intersection  */
      ligma_gegl_buffer_copy (
        ligma_drawable_get_buffer (drawable),
        GEGL_RECTANGLE (copy_x - ligma_item_get_offset_x (item),
                        copy_y - ligma_item_get_offset_y (item),
                        copy_width,
                        copy_height), GEGL_ABYSS_NONE,
        new_buffer,
        GEGL_RECTANGLE (copy_x - new_offset_x,
                        copy_y - new_offset_y, 0, 0));
    }

  ligma_drawable_set_buffer_full (drawable, ligma_item_is_attached (item), NULL,
                                 new_buffer,
                                 GEGL_RECTANGLE (new_offset_x, new_offset_y,
                                                 0,            0),
                                 TRUE);
  g_object_unref (new_buffer);
}

static void
ligma_drawable_flip (LigmaItem            *item,
                    LigmaContext         *context,
                    LigmaOrientationType  flip_type,
                    gdouble              axis,
                    gboolean             clip_result)
{
  LigmaDrawable     *drawable = LIGMA_DRAWABLE (item);
  GeglBuffer       *buffer;
  LigmaColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  ligma_item_get_offset (item, &off_x, &off_y);

  buffer = ligma_drawable_transform_buffer_flip (drawable, context,
                                                ligma_drawable_get_buffer (drawable),
                                                off_x, off_y,
                                                flip_type, axis,
                                                clip_result,
                                                &buffer_profile,
                                                &new_off_x, &new_off_y);

  if (buffer)
    {
      ligma_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE);
      g_object_unref (buffer);
    }
}

static void
ligma_drawable_rotate (LigmaItem         *item,
                      LigmaContext      *context,
                      LigmaRotationType  rotate_type,
                      gdouble           center_x,
                      gdouble           center_y,
                      gboolean          clip_result)
{
  LigmaDrawable     *drawable = LIGMA_DRAWABLE (item);
  GeglBuffer       *buffer;
  LigmaColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  ligma_item_get_offset (item, &off_x, &off_y);

  buffer = ligma_drawable_transform_buffer_rotate (drawable, context,
                                                  ligma_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  rotate_type, center_x, center_y,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y);

  if (buffer)
    {
      ligma_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE);
      g_object_unref (buffer);
    }
}

static void
ligma_drawable_transform (LigmaItem               *item,
                         LigmaContext            *context,
                         const LigmaMatrix3      *matrix,
                         LigmaTransformDirection  direction,
                         LigmaInterpolationType   interpolation_type,
                         LigmaTransformResize     clip_result,
                         LigmaProgress           *progress)
{
  LigmaDrawable     *drawable = LIGMA_DRAWABLE (item);
  GeglBuffer       *buffer;
  LigmaColorProfile *buffer_profile;
  gint              off_x, off_y;
  gint              new_off_x, new_off_y;

  ligma_item_get_offset (item, &off_x, &off_y);

  buffer = ligma_drawable_transform_buffer_affine (drawable, context,
                                                  ligma_drawable_get_buffer (drawable),
                                                  off_x, off_y,
                                                  matrix, direction,
                                                  interpolation_type,
                                                  clip_result,
                                                  &buffer_profile,
                                                  &new_off_x, &new_off_y,
                                                  progress);

  if (buffer)
    {
      ligma_drawable_transform_paste (drawable, buffer, buffer_profile,
                                     new_off_x, new_off_y, FALSE);
      g_object_unref (buffer);
    }
}

static const guint8 *
ligma_drawable_get_icc_profile (LigmaColorManaged *managed,
                               gsize            *len)
{
  LigmaColorProfile *profile = ligma_color_managed_get_color_profile (managed);

  return ligma_color_profile_get_icc_profile (profile, len);
}

static LigmaColorProfile *
ligma_drawable_get_color_profile (LigmaColorManaged *managed)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (managed);
  const Babl   *format   = ligma_drawable_get_format (drawable);

  if (! drawable->private->format_profile)
    drawable->private->format_profile =
      ligma_babl_format_get_color_profile (format);

  return drawable->private->format_profile;
}

static void
ligma_drawable_profile_changed (LigmaColorManaged *managed)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (managed));
}

static gboolean
ligma_drawable_get_pixel_at (LigmaPickable *pickable,
                            gint          x,
                            gint          y,
                            const Babl   *format,
                            gpointer      pixel)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (pickable);

  /* do not make this a g_return_if_fail() */
  if (x < 0 || x >= ligma_item_get_width  (LIGMA_ITEM (drawable)) ||
      y < 0 || y >= ligma_item_get_height (LIGMA_ITEM (drawable)))
    return FALSE;

  gegl_buffer_sample (ligma_drawable_get_buffer (drawable),
                      x, y, NULL, pixel, format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);

  return TRUE;
}

static void
ligma_drawable_get_pixel_average (LigmaPickable        *pickable,
                                 const GeglRectangle *rect,
                                 const Babl          *format,
                                 gpointer             pixel)
{
  LigmaDrawable *drawable = LIGMA_DRAWABLE (pickable);

  return ligma_gegl_average_color (ligma_drawable_get_buffer (drawable),
                                  rect, TRUE, GEGL_ABYSS_NONE, format, pixel);
}

static void
ligma_drawable_real_update (LigmaDrawable *drawable,
                           gint          x,
                           gint          y,
                           gint          width,
                           gint          height)
{
  ligma_viewable_invalidate_preview (LIGMA_VIEWABLE (drawable));
}

static gint64
ligma_drawable_real_estimate_memsize (LigmaDrawable      *drawable,
                                     LigmaComponentType  component_type,
                                     gint               width,
                                     gint               height)
{
  LigmaImage   *image = ligma_item_get_image (LIGMA_ITEM (drawable));
  LigmaTRCType  trc   = ligma_drawable_get_trc (drawable);
  const Babl  *format;

  format = ligma_image_get_format (image,
                                  ligma_drawable_get_base_type (drawable),
                                  ligma_babl_precision (component_type, trc),
                                  ligma_drawable_has_alpha (drawable),
                                  NULL);

  return (gint64) babl_format_get_bytes_per_pixel (format) * width * height;
}

static void
ligma_drawable_real_update_all (LigmaDrawable *drawable)
{
  ligma_drawable_update (drawable, 0, 0, -1, -1);
}

static LigmaComponentMask
ligma_drawable_real_get_active_mask (LigmaDrawable *drawable)
{
  /*  Return all, because that skips the component mask op when painting  */
  return LIGMA_COMPONENT_MASK_ALL;
}

static gboolean
ligma_drawable_real_supports_alpha (LigmaDrawable *drawable)
{
  return FALSE;
}

/* FIXME: this default impl is currently unused because no subclass
 * chains up. the goal is to handle the almost identical subclass code
 * here again.
 */
static void
ligma_drawable_real_convert_type (LigmaDrawable      *drawable,
                                 LigmaImage         *dest_image,
                                 const Babl        *new_format,
                                 LigmaColorProfile  *src_profile,
                                 LigmaColorProfile  *dest_profile,
                                 GeglDitherMethod   layer_dither_type,
                                 GeglDitherMethod   mask_dither_type,
                                 gboolean           push_undo,
                                 LigmaProgress      *progress)
{
  GeglBuffer *dest_buffer;

  dest_buffer =
    gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                     ligma_item_get_width  (LIGMA_ITEM (drawable)),
                                     ligma_item_get_height (LIGMA_ITEM (drawable))),
                     new_format);

  ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable), NULL,
                         GEGL_ABYSS_NONE,
                         dest_buffer, NULL);

  ligma_drawable_set_buffer (drawable, push_undo, NULL, dest_buffer);
  g_object_unref (dest_buffer);
}

static GeglBuffer *
ligma_drawable_real_get_buffer (LigmaDrawable *drawable)
{
  return drawable->private->buffer;
}

static void
ligma_drawable_real_set_buffer (LigmaDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds)
{
  LigmaItem   *item          = LIGMA_ITEM (drawable);
  const Babl *old_format    = NULL;
  gint        old_has_alpha = -1;

  g_object_freeze_notify (G_OBJECT (drawable));

  ligma_drawable_invalidate_boundary (drawable);

  if (push_undo)
    ligma_image_undo_push_drawable_mod (ligma_item_get_image (item), undo_desc,
                                       drawable, FALSE);

  if (drawable->private->buffer)
    {
      old_format    = ligma_drawable_get_format (drawable);
      old_has_alpha = ligma_drawable_has_alpha (drawable);
    }

  g_set_object (&drawable->private->buffer, buffer);
  g_clear_object (&drawable->private->format_profile);

  if (drawable->private->buffer_source_node)
    gegl_node_set (drawable->private->buffer_source_node,
                   "buffer", ligma_drawable_get_buffer (drawable),
                   NULL);

  ligma_item_set_offset (item, bounds->x, bounds->y);
  ligma_item_set_size (item,
                      bounds->width  ? bounds->width :
                                       gegl_buffer_get_width (buffer),
                      bounds->height ? bounds->height :
                                       gegl_buffer_get_height (buffer));

  ligma_drawable_update_bounding_box (drawable);

  if (ligma_drawable_get_format (drawable) != old_format)
    ligma_drawable_format_changed (drawable);

  if (ligma_drawable_has_alpha (drawable) != old_has_alpha)
    ligma_drawable_alpha_changed (drawable);

  g_object_notify (G_OBJECT (drawable), "buffer");

  g_object_thaw_notify (G_OBJECT (drawable));
}

static GeglRectangle
ligma_drawable_real_get_bounding_box (LigmaDrawable *drawable)
{
  return gegl_node_get_bounding_box (ligma_drawable_get_source_node (drawable));
}

static void
ligma_drawable_real_push_undo (LigmaDrawable *drawable,
                              const gchar  *undo_desc,
                              GeglBuffer   *buffer,
                              gint          x,
                              gint          y,
                              gint          width,
                              gint          height)
{
  LigmaImage *image;

  if (! buffer)
    {
      GeglBuffer    *drawable_buffer = ligma_drawable_get_buffer (drawable);
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
                                ligma_drawable_get_format (drawable));

      ligma_gegl_buffer_copy (
        drawable_buffer,
        &drawable_rect, GEGL_ABYSS_NONE,
        buffer,
        GEGL_RECTANGLE (0, 0, 0, 0));
    }
  else
    {
      g_object_ref (buffer);
    }

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  ligma_image_undo_push_drawable (image,
                                 undo_desc, drawable,
                                 buffer, x, y);

  g_object_unref (buffer);
}

static void
ligma_drawable_real_swap_pixels (LigmaDrawable *drawable,
                                GeglBuffer   *buffer,
                                gint          x,
                                gint          y)
{
  GeglBuffer *tmp;
  gint        width  = gegl_buffer_get_width (buffer);
  gint        height = gegl_buffer_get_height (buffer);

  tmp = ligma_gegl_buffer_dup (buffer);

  ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, width, height), GEGL_ABYSS_NONE,
                         buffer,
                         GEGL_RECTANGLE (0, 0, 0, 0));
  ligma_gegl_buffer_copy (tmp,
                         GEGL_RECTANGLE (0, 0, width, height), GEGL_ABYSS_NONE,
                         ligma_drawable_get_buffer (drawable),
                         GEGL_RECTANGLE (x, y, 0, 0));

  g_object_unref (tmp);

  ligma_drawable_update (drawable, x, y, width, height);
}

static GeglNode *
ligma_drawable_real_get_source_node (LigmaDrawable *drawable)
{
  g_warn_if_fail (drawable->private->buffer_source_node == NULL);

  drawable->private->buffer_source_node =
    gegl_node_new_child (NULL,
                         "operation", "ligma:buffer-source-validate",
                         "buffer",    ligma_drawable_get_buffer (drawable),
                         NULL);

  return g_object_ref (drawable->private->buffer_source_node);
}

static void
ligma_drawable_format_changed (LigmaDrawable *drawable)
{
  g_signal_emit (drawable, ligma_drawable_signals[FORMAT_CHANGED], 0);
}

static void
ligma_drawable_alpha_changed (LigmaDrawable *drawable)
{
  g_signal_emit (drawable, ligma_drawable_signals[ALPHA_CHANGED], 0);
}


/*  public functions  */

LigmaDrawable *
ligma_drawable_new (GType          type,
                   LigmaImage     *image,
                   const gchar   *name,
                   gint           offset_x,
                   gint           offset_y,
                   gint           width,
                   gint           height,
                   const Babl    *format)
{
  LigmaDrawable *drawable;
  GeglBuffer   *buffer;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (g_type_is_a (type, LIGMA_TYPE_DRAWABLE), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);
  g_return_val_if_fail (format != NULL, NULL);

  drawable = LIGMA_DRAWABLE (ligma_item_new (type,
                                           image, name,
                                           offset_x, offset_y,
                                           width, height));

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, width, height), format);

  ligma_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);

  return drawable;
}

gint64
ligma_drawable_estimate_memsize (LigmaDrawable      *drawable,
                                LigmaComponentType  component_type,
                                gint               width,
                                gint               height)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), 0);

  return LIGMA_DRAWABLE_GET_CLASS (drawable)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

void
ligma_drawable_update (LigmaDrawable *drawable,
                      gint          x,
                      gint          y,
                      gint          width,
                      gint          height)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  if (width < 0)
    {
      GeglRectangle bounding_box;

      bounding_box = ligma_drawable_get_bounding_box (drawable);

      x     = bounding_box.x;
      width = bounding_box.width;
    }

  if (height < 0)
    {
      GeglRectangle bounding_box;

      bounding_box = ligma_drawable_get_bounding_box (drawable);

      y      = bounding_box.y;
      height = bounding_box.height;
    }

  if (drawable->private->paint_count == 0)
    {
      g_signal_emit (drawable, ligma_drawable_signals[UPDATE], 0,
                     x, y, width, height);
    }
  else
    {
      GeglRectangle rect;

      if (gegl_rectangle_intersect (
            &rect,
            GEGL_RECTANGLE (x, y, width, height),
            GEGL_RECTANGLE (0, 0,
                            ligma_item_get_width  (LIGMA_ITEM (drawable)),
                            ligma_item_get_height (LIGMA_ITEM (drawable)))))
        {
          GeglRectangle aligned_rect;

          gegl_rectangle_align_to_buffer (&aligned_rect, &rect,
                                          ligma_drawable_get_buffer (drawable),
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
ligma_drawable_update_all (LigmaDrawable *drawable)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  LIGMA_DRAWABLE_GET_CLASS (drawable)->update_all (drawable);
}

void
ligma_drawable_invalidate_boundary (LigmaDrawable *drawable)
{
  LigmaDrawableClass *drawable_class;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  drawable_class = LIGMA_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->invalidate_boundary)
    drawable_class->invalidate_boundary (drawable);
}

void
ligma_drawable_get_active_components (LigmaDrawable *drawable,
                                     gboolean     *active)
{
  LigmaDrawableClass *drawable_class;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (active != NULL);

  drawable_class = LIGMA_DRAWABLE_GET_CLASS (drawable);

  if (drawable_class->get_active_components)
    drawable_class->get_active_components (drawable, active);
}

LigmaComponentMask
ligma_drawable_get_active_mask (LigmaDrawable *drawable)
{
  LigmaComponentMask mask;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), 0);

  mask = LIGMA_DRAWABLE_GET_CLASS (drawable)->get_active_mask (drawable);

  /* if the drawable doesn't have an alpha channel, the value of the mask's
   * alpha-bit doesn't matter, however, we'd like to have a fully-clear or
   * fully-set mask whenever possible, since it allows us to skip component
   * masking altogether.  we therefore set or clear the alpha bit, depending on
   * the state of the other bits, so that it never gets in the way of a uniform
   * mask.
   */
  if (! ligma_drawable_has_alpha (drawable))
    {
      if (mask & ~LIGMA_COMPONENT_MASK_ALPHA)
        mask |= LIGMA_COMPONENT_MASK_ALPHA;
      else
        mask &= ~LIGMA_COMPONENT_MASK_ALPHA;
    }

  return mask;
}

gboolean
ligma_drawable_supports_alpha (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  return LIGMA_DRAWABLE_GET_CLASS (drawable)->supports_alpha (drawable);
}

void
ligma_drawable_convert_type (LigmaDrawable      *drawable,
                            LigmaImage         *dest_image,
                            LigmaImageBaseType  new_base_type,
                            LigmaPrecision      new_precision,
                            gboolean           new_has_alpha,
                            LigmaColorProfile  *src_profile,
                            LigmaColorProfile  *dest_profile,
                            GeglDitherMethod   layer_dither_type,
                            GeglDitherMethod   mask_dither_type,
                            gboolean           push_undo,
                            LigmaProgress      *progress)
{
  const Babl *old_format;
  const Babl *new_format;
  gint        old_bits;
  gint        new_bits;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_IMAGE (dest_image));
  g_return_if_fail (new_base_type != ligma_drawable_get_base_type (drawable) ||
                    new_precision != ligma_drawable_get_precision (drawable) ||
                    new_has_alpha != ligma_drawable_has_alpha (drawable)     ||
                    dest_profile);
  g_return_if_fail (src_profile == NULL || LIGMA_IS_COLOR_PROFILE (src_profile));
  g_return_if_fail (dest_profile == NULL || LIGMA_IS_COLOR_PROFILE (dest_profile));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  if (! ligma_item_is_attached (LIGMA_ITEM (drawable)))
    push_undo = FALSE;

  old_format = ligma_drawable_get_format (drawable);
  new_format = ligma_image_get_format (dest_image,
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

  LIGMA_DRAWABLE_GET_CLASS (drawable)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);

  if (progress)
    ligma_progress_set_value (progress, 1.0);
}

void
ligma_drawable_apply_buffer (LigmaDrawable           *drawable,
                            GeglBuffer             *buffer,
                            const GeglRectangle    *buffer_region,
                            gboolean                push_undo,
                            const gchar            *undo_desc,
                            gdouble                 opacity,
                            LigmaLayerMode           mode,
                            LigmaLayerColorSpace     blend_space,
                            LigmaLayerColorSpace     composite_space,
                            LigmaLayerCompositeMode  composite_mode,
                            GeglBuffer             *base_buffer,
                            gint                    base_x,
                            gint                    base_y)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));
  g_return_if_fail (buffer_region != NULL);
  g_return_if_fail (base_buffer == NULL || GEGL_IS_BUFFER (base_buffer));

  LIGMA_DRAWABLE_GET_CLASS (drawable)->apply_buffer (drawable, buffer,
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
ligma_drawable_get_buffer (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->paint_count == 0)
    return LIGMA_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);
  else
    return drawable->private->paint_buffer;
}

void
ligma_drawable_set_buffer (LigmaDrawable *drawable,
                          gboolean      push_undo,
                          const gchar  *undo_desc,
                          GeglBuffer   *buffer)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  if (! ligma_item_is_attached (LIGMA_ITEM (drawable)))
    push_undo = FALSE;

  ligma_drawable_set_buffer_full (drawable, push_undo, undo_desc, buffer, NULL,
                                 TRUE);
}

void
ligma_drawable_set_buffer_full (LigmaDrawable        *drawable,
                               gboolean             push_undo,
                               const gchar         *undo_desc,
                               GeglBuffer          *buffer,
                               const GeglRectangle *bounds,
                               gboolean             update)
{
  LigmaItem      *item;
  GeglRectangle  curr_bounds;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  item = LIGMA_ITEM (drawable);

  if (! ligma_item_is_attached (LIGMA_ITEM (drawable)))
    push_undo = FALSE;

  if (! bounds)
    {
      ligma_item_get_offset (LIGMA_ITEM (drawable),
                            &curr_bounds.x, &curr_bounds.y);

      curr_bounds.width  = 0;
      curr_bounds.height = 0;

      bounds = &curr_bounds;
    }

  if (update && ligma_drawable_get_buffer (drawable))
    {
      GeglBuffer    *old_buffer = ligma_drawable_get_buffer (drawable);
      GeglRectangle  old_extent;
      GeglRectangle  new_extent;

      old_extent = *gegl_buffer_get_extent (old_buffer);
      old_extent.x += ligma_item_get_offset_x (item);
      old_extent.y += ligma_item_get_offset_x (item);

      new_extent = *gegl_buffer_get_extent (buffer);
      new_extent.x += bounds->x;
      new_extent.y += bounds->y;

      if (! gegl_rectangle_equal (&old_extent, &new_extent))
        ligma_drawable_update (drawable, 0, 0, -1, -1);
    }

  g_object_freeze_notify (G_OBJECT (drawable));

  LIGMA_DRAWABLE_GET_CLASS (drawable)->set_buffer (drawable,
                                                  push_undo, undo_desc,
                                                  buffer, bounds);

  g_object_thaw_notify (G_OBJECT (drawable));

  if (update)
    ligma_drawable_update (drawable, 0, 0, -1, -1);
}

void
ligma_drawable_steal_buffer (LigmaDrawable *drawable,
                            LigmaDrawable *src_drawable)
{
  GeglBuffer *buffer;
  GeglBuffer *replacement_buffer;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_DRAWABLE (src_drawable));

  buffer = ligma_drawable_get_buffer (src_drawable);

  g_return_if_fail (buffer != NULL);

  g_object_ref (buffer);

  replacement_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, 1, 1),
                                        gegl_buffer_get_format (buffer));

  ligma_drawable_set_buffer (src_drawable, FALSE, NULL, replacement_buffer);
  ligma_drawable_set_buffer (drawable,     FALSE, NULL, buffer);

  g_object_unref (replacement_buffer);
  g_object_unref (buffer);
}

void
ligma_drawable_set_format (LigmaDrawable *drawable,
                          const Babl   *format,
                          gboolean      copy_buffer,
                          gboolean      push_undo)
{
  LigmaItem   *item;
  GeglBuffer *buffer;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (format != NULL);
  g_return_if_fail (format != ligma_drawable_get_format (drawable));
  g_return_if_fail (ligma_babl_format_get_base_type (format) ==
                    ligma_drawable_get_base_type (drawable));
  g_return_if_fail (ligma_babl_format_get_component_type (format) ==
                    ligma_drawable_get_component_type (drawable));
  g_return_if_fail (babl_format_has_alpha (format) ==
                    ligma_drawable_has_alpha (drawable));
  g_return_if_fail (push_undo == FALSE || copy_buffer == TRUE);

  item = LIGMA_ITEM (drawable);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_drawable_format (ligma_item_get_image (item),
                                          NULL, drawable);

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                            ligma_item_get_width  (item),
                                            ligma_item_get_height (item)),
                            format);

  if (copy_buffer)
    {
      gegl_buffer_set_format (buffer, ligma_drawable_get_format (drawable));

      ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable),
                             NULL, GEGL_ABYSS_NONE,
                             buffer, NULL);

      gegl_buffer_set_format (buffer, NULL);
    }

  ligma_drawable_set_buffer (drawable, FALSE, NULL, buffer);
  g_object_unref (buffer);
}

GeglNode *
ligma_drawable_get_source_node (LigmaDrawable *drawable)
{
  GeglNode *input;
  GeglNode *source;
  GeglNode *filter;
  GeglNode *output;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  if (drawable->private->source_node)
    return drawable->private->source_node;

  drawable->private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (drawable->private->source_node, "input");

  source = LIGMA_DRAWABLE_GET_CLASS (drawable)->get_source_node (drawable);

  gegl_node_add_child (drawable->private->source_node, source);

  g_object_unref (source);

  if (gegl_node_has_pad (source, "input"))
    {
      gegl_node_connect_to (input,  "output",
                            source, "input");
    }

  filter = ligma_filter_stack_get_graph (LIGMA_FILTER_STACK (drawable->private->filter_stack));

  gegl_node_add_child (drawable->private->source_node, filter);

  gegl_node_connect_to (source, "output",
                        filter, "input");

  output = gegl_node_get_output_proxy (drawable->private->source_node, "output");

  gegl_node_connect_to (filter, "output",
                        output, "input");

  if (ligma_drawable_get_floating_sel (drawable))
    _ligma_drawable_add_floating_sel_filter (drawable);

  return drawable->private->source_node;
}

GeglNode *
ligma_drawable_get_mode_node (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  if (! drawable->private->mode_node)
    ligma_filter_get_node (LIGMA_FILTER (drawable));

  return drawable->private->mode_node;
}

GeglRectangle
ligma_drawable_get_bounding_box (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable),
                        *GEGL_RECTANGLE (0, 0, 0, 0));

  if (gegl_rectangle_is_empty (&drawable->private->bounding_box))
    ligma_drawable_update_bounding_box (drawable);

  return drawable->private->bounding_box;
}

gboolean
ligma_drawable_update_bounding_box (LigmaDrawable *drawable)
{
  GeglRectangle bounding_box;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  bounding_box =
    LIGMA_DRAWABLE_GET_CLASS (drawable)->get_bounding_box (drawable);

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
          ligma_drawable_update (drawable,
                                diff_rects[i].x,
                                diff_rects[i].y,
                                diff_rects[i].width,
                                diff_rects[i].height);
        }

      drawable->private->bounding_box = bounding_box;

      g_signal_emit (drawable, ligma_drawable_signals[BOUNDING_BOX_CHANGED], 0);

      n_diff_rects = gegl_rectangle_subtract (diff_rects,
                                              &bounding_box,
                                              &old_bounding_box);

      for (i = 0; i < n_diff_rects; i++)
        {
          ligma_drawable_update (drawable,
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
ligma_drawable_swap_pixels (LigmaDrawable *drawable,
                           GeglBuffer   *buffer,
                           gint          x,
                           gint          y)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (GEGL_IS_BUFFER (buffer));

  LIGMA_DRAWABLE_GET_CLASS (drawable)->swap_pixels (drawable, buffer, x, y);
}

void
ligma_drawable_push_undo (LigmaDrawable *drawable,
                         const gchar  *undo_desc,
                         GeglBuffer   *buffer,
                         gint          x,
                         gint          y,
                         gint          width,
                         gint          height)
{
  LigmaItem *item;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (buffer == NULL || GEGL_IS_BUFFER (buffer));

  item = LIGMA_ITEM (drawable);

  g_return_if_fail (ligma_item_is_attached (item));

  if (! buffer &&
      ! ligma_rectangle_intersect (x, y,
                                  width, height,
                                  0, 0,
                                  ligma_item_get_width (item),
                                  ligma_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      g_warning ("%s: tried to push empty region", G_STRFUNC);
      return;
    }

  LIGMA_DRAWABLE_GET_CLASS (drawable)->push_undo (drawable, undo_desc,
                                                 buffer,
                                                 x, y, width, height);
}

const Babl *
ligma_drawable_get_space (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return babl_format_get_space (ligma_drawable_get_format (drawable));
}

const Babl *
ligma_drawable_get_format (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return gegl_buffer_get_format (drawable->private->buffer);
}

const Babl *
ligma_drawable_get_format_with_alpha (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return ligma_image_get_format (ligma_item_get_image (LIGMA_ITEM (drawable)),
                                ligma_drawable_get_base_type (drawable),
                                ligma_drawable_get_precision (drawable),
                                TRUE,
                                ligma_drawable_get_space (drawable));
}

const Babl *
ligma_drawable_get_format_without_alpha (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return ligma_image_get_format (ligma_item_get_image (LIGMA_ITEM (drawable)),
                                ligma_drawable_get_base_type (drawable),
                                ligma_drawable_get_precision (drawable),
                                FALSE,
                                ligma_drawable_get_space (drawable));
}

LigmaTRCType
ligma_drawable_get_trc (LigmaDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return ligma_babl_format_get_trc (format);
}

gboolean
ligma_drawable_has_alpha (LigmaDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return babl_format_has_alpha (format);
}

LigmaImageBaseType
ligma_drawable_get_base_type (LigmaDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return ligma_babl_format_get_base_type (format);
}

LigmaComponentType
ligma_drawable_get_component_type (LigmaDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return ligma_babl_format_get_component_type (format);
}

LigmaPrecision
ligma_drawable_get_precision (LigmaDrawable *drawable)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), -1);

  format = gegl_buffer_get_format (drawable->private->buffer);

  return ligma_babl_format_get_precision (format);
}

gboolean
ligma_drawable_is_rgb (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  return (ligma_drawable_get_base_type (drawable) == LIGMA_RGB);
}

gboolean
ligma_drawable_is_gray (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  return (ligma_drawable_get_base_type (drawable) == LIGMA_GRAY);
}

gboolean
ligma_drawable_is_indexed (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  return (ligma_drawable_get_base_type (drawable) == LIGMA_INDEXED);
}

const Babl *
ligma_drawable_get_component_format (LigmaDrawable    *drawable,
                                    LigmaChannelType  channel)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  switch (channel)
    {
    case LIGMA_CHANNEL_RED:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_drawable_get_precision (drawable),
                                         RED);

    case LIGMA_CHANNEL_GREEN:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_drawable_get_precision (drawable),
                                         GREEN);

    case LIGMA_CHANNEL_BLUE:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_drawable_get_precision (drawable),
                                         BLUE);

    case LIGMA_CHANNEL_ALPHA:
      return ligma_babl_component_format (LIGMA_RGB,
                                         ligma_drawable_get_precision (drawable),
                                         ALPHA);

    case LIGMA_CHANNEL_GRAY:
      return ligma_babl_component_format (LIGMA_GRAY,
                                         ligma_drawable_get_precision (drawable),
                                         GRAY);

    case LIGMA_CHANNEL_INDEXED:
      return babl_format ("Y u8"); /* will extract grayscale, the best
                                    * we can do here */
    }

  return NULL;
}

gint
ligma_drawable_get_component_index (LigmaDrawable    *drawable,
                                   LigmaChannelType  channel)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), -1);

  switch (channel)
    {
    case LIGMA_CHANNEL_RED:     return RED;
    case LIGMA_CHANNEL_GREEN:   return GREEN;
    case LIGMA_CHANNEL_BLUE:    return BLUE;
    case LIGMA_CHANNEL_GRAY:    return GRAY;
    case LIGMA_CHANNEL_INDEXED: return INDEXED;
    case LIGMA_CHANNEL_ALPHA:
      switch (ligma_drawable_get_base_type (drawable))
        {
        case LIGMA_RGB:     return ALPHA;
        case LIGMA_GRAY:    return ALPHA_G;
        case LIGMA_INDEXED: return ALPHA_I;
        }
    }

  return -1;
}

guchar *
ligma_drawable_get_colormap (LigmaDrawable *drawable)
{
  LigmaImage *image;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  image = ligma_item_get_image (LIGMA_ITEM (drawable));

  return image ? ligma_image_get_colormap (image) : NULL;
}

void
ligma_drawable_start_paint (LigmaDrawable *drawable)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));

  if (drawable->private->paint_count == 0)
    {
      GeglBuffer *buffer = ligma_drawable_get_buffer (drawable);

      g_return_if_fail (buffer != NULL);
      g_return_if_fail (drawable->private->paint_buffer == NULL);
      g_return_if_fail (drawable->private->paint_copy_region == NULL);
      g_return_if_fail (drawable->private->paint_update_region == NULL);

      drawable->private->paint_buffer = ligma_gegl_buffer_dup (buffer);
    }

  drawable->private->paint_count++;
}

gboolean
ligma_drawable_end_paint (LigmaDrawable *drawable)
{
  gboolean result = FALSE;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_count == 1)
    {
      result = ligma_drawable_flush_paint (drawable);

      g_clear_object (&drawable->private->paint_buffer);
    }

  drawable->private->paint_count--;

  return result;
}

gboolean
ligma_drawable_flush_paint (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (drawable->private->paint_count > 0, FALSE);

  if (drawable->private->paint_copy_region)
    {
      GeglBuffer *buffer;
      gint        n_rects;
      gint        i;

      buffer = LIGMA_DRAWABLE_GET_CLASS (drawable)->get_buffer (drawable);

      g_return_val_if_fail (buffer != NULL, FALSE);
      g_return_val_if_fail (drawable->private->paint_buffer != NULL, FALSE);

      n_rects = cairo_region_num_rectangles (
        drawable->private->paint_copy_region);

      for (i = 0; i < n_rects; i++)
        {
          GeglRectangle rect;

          cairo_region_get_rectangle (drawable->private->paint_copy_region,
                                      i, (cairo_rectangle_int_t *) &rect);

          ligma_gegl_buffer_copy (
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

          g_signal_emit (drawable, ligma_drawable_signals[UPDATE], 0,
                         rect.x, rect.y, rect.width, rect.height);
        }

      g_clear_pointer (&drawable->private->paint_update_region,
                       cairo_region_destroy);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_drawable_is_painting (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  return drawable->private->paint_count > 0;
}
