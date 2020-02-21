/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimpdrawable-filters.h"
#include "gimpgrouplayer.h"
#include "gimpgrouplayerundo.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayerstack.h"
#include "gimpobjectqueue.h"
#include "gimppickable.h"
#include "gimpprogress.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"

#include "gimp-intl.h"


typedef struct _GimpGroupLayerPrivate GimpGroupLayerPrivate;

struct _GimpGroupLayerPrivate
{
  GimpContainer  *children;
  GimpProjection *projection;
  GeglNode       *source_node;
  GeglNode       *parent_source_node;
  GeglNode       *graph;
  GeglNode       *offset_node;
  GeglRectangle   bounding_box;
  gint            suspend_resize;
  gint            suspend_mask;
  GeglBuffer     *suspended_mask_buffer;
  GeglRectangle   suspended_mask_bounds;
  gint            direct_update;
  gint            transforming;
  gboolean        expanded;
  gboolean        pass_through;

  /*  hackish temp states to make the projection/tiles stuff work  */
  const Babl     *convert_format;
  gboolean        reallocate_projection;
};

#define GET_PRIVATE(item) ((GimpGroupLayerPrivate *) gimp_group_layer_get_instance_private ((GimpGroupLayer *) (item)))


static void            gimp_projectable_iface_init   (GimpProjectableInterface  *iface);
static void            gimp_pickable_iface_init      (GimpPickableInterface     *iface);

static void            gimp_group_layer_finalize     (GObject         *object);
static void            gimp_group_layer_set_property (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void            gimp_group_layer_get_property (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);

static gint64          gimp_group_layer_get_memsize  (GimpObject      *object,
                                                      gint64          *gui_size);

static void        gimp_group_layer_ancestry_changed (GimpViewable    *viewable);
static gboolean        gimp_group_layer_get_size     (GimpViewable    *viewable,
                                                      gint            *width,
                                                      gint            *height);
static GimpContainer * gimp_group_layer_get_children (GimpViewable    *viewable);
static gboolean        gimp_group_layer_get_expanded (GimpViewable    *viewable);
static void            gimp_group_layer_set_expanded (GimpViewable    *viewable,
                                                      gboolean         expanded);

static gboolean  gimp_group_layer_is_position_locked (GimpItem        *item);
static GimpItem      * gimp_group_layer_duplicate    (GimpItem        *item,
                                                      GType            new_type);
static void            gimp_group_layer_convert      (GimpItem        *item,
                                                      GimpImage       *dest_image,
                                                      GType            old_type);
static void         gimp_group_layer_start_transform (GimpItem        *item,
                                                      gboolean         push_undo);
static void           gimp_group_layer_end_transform (GimpItem        *item,
                                                      gboolean         push_undo);
static void            gimp_group_layer_resize       (GimpItem        *item,
                                                      GimpContext     *context,
                                                      GimpFillType     fill_type,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             offset_x,
                                                      gint             offset_y);
static GimpTransformResize
                       gimp_group_layer_get_clip     (GimpItem        *item,
                                                      GimpTransformResize clip_result);

static gint64      gimp_group_layer_estimate_memsize (GimpDrawable      *drawable,
                                                      GimpComponentType  component_type,
                                                      gint               width,
                                                      gint               height);
static void            gimp_group_layer_update_all   (GimpDrawable      *drawable);

static void            gimp_group_layer_translate    (GimpLayer       *layer,
                                                      gint             offset_x,
                                                      gint             offset_y);
static void            gimp_group_layer_scale        (GimpLayer       *layer,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             new_offset_x,
                                                      gint             new_offset_y,
                                                      GimpInterpolationType  interp_type,
                                                      GimpProgress    *progress);
static void            gimp_group_layer_flip         (GimpLayer       *layer,
                                                      GimpContext     *context,
                                                      GimpOrientationType flip_type,
                                                      gdouble          axis,
                                                      gboolean         clip_result);
static void            gimp_group_layer_rotate       (GimpLayer       *layer,
                                                      GimpContext     *context,
                                                      GimpRotationType rotate_type,
                                                      gdouble          center_x,
                                                      gdouble          center_y,
                                                      gboolean         clip_result);
static void            gimp_group_layer_transform    (GimpLayer       *layer,
                                                      GimpContext     *context,
                                                      const GimpMatrix3 *matrix,
                                                      GimpTransformDirection direction,
                                                      GimpInterpolationType  interpolation_type,
                                                      GimpTransformResize clip_result,
                                                      GimpProgress      *progress);
static void            gimp_group_layer_convert_type (GimpLayer         *layer,
                                                      GimpImage         *dest_image,
                                                      const Babl        *new_format,
                                                      GimpColorProfile  *dest_profile,
                                                      GeglDitherMethod   layer_dither_type,
                                                      GeglDitherMethod   mask_dither_type,
                                                      gboolean           push_undo,
                                                      GimpProgress      *progress);
static GeglNode   * gimp_group_layer_get_source_node (GimpDrawable      *drawable);

static void         gimp_group_layer_opacity_changed (GimpLayer         *layer);
static void  gimp_group_layer_effective_mode_changed (GimpLayer         *layer);
static void
          gimp_group_layer_excludes_backdrop_changed (GimpLayer         *layer);
static GeglRectangle
                   gimp_group_layer_get_bounding_box (GimpLayer         *layer);
static void      gimp_group_layer_get_effective_mode (GimpLayer         *layer,
                                                      GimpLayerMode          *mode,
                                                      GimpLayerColorSpace    *blend_space,
                                                      GimpLayerColorSpace    *composite_space,
                                                      GimpLayerCompositeMode *composite_mode);
static gboolean
              gimp_group_layer_get_excludes_backdrop (GimpLayer         *layer);

static const Babl    * gimp_group_layer_get_format   (GimpProjectable *projectable);
static GeglRectangle
       gimp_group_layer_projectable_get_bounding_box (GimpProjectable *projectable);
static GeglNode      * gimp_group_layer_get_graph    (GimpProjectable *projectable);
static void            gimp_group_layer_begin_render (GimpProjectable *projectable);
static void            gimp_group_layer_end_render   (GimpProjectable *projectable);

static void          gimp_group_layer_pickable_flush (GimpPickable    *pickable);
static gdouble       gimp_group_layer_get_opacity_at (GimpPickable    *pickable,
                                                      gint             x,
                                                      gint             y);


static void            gimp_group_layer_child_add    (GimpContainer   *container,
                                                      GimpLayer       *child,
                                                      GimpGroupLayer  *group);
static void            gimp_group_layer_child_remove (GimpContainer   *container,
                                                      GimpLayer       *child,
                                                      GimpGroupLayer  *group);
static void            gimp_group_layer_child_move   (GimpLayer       *child,
                                                      GParamSpec      *pspec,
                                                      GimpGroupLayer  *group);
static void            gimp_group_layer_child_resize (GimpLayer       *child,
                                                      GimpGroupLayer  *group);
static void    gimp_group_layer_child_active_changed (GimpLayer       *child,
                                                      GimpGroupLayer  *group);
static void
       gimp_group_layer_child_effective_mode_changed (GimpLayer       *child,
                                                      GimpGroupLayer  *group);
static void
    gimp_group_layer_child_excludes_backdrop_changed (GimpLayer       *child,
                                                      GimpGroupLayer  *group);

static void            gimp_group_layer_flush        (GimpGroupLayer  *group);
static void            gimp_group_layer_update       (GimpGroupLayer  *group);
static void            gimp_group_layer_update_size  (GimpGroupLayer  *group);
static void        gimp_group_layer_update_mask_size (GimpGroupLayer  *group);
static void      gimp_group_layer_update_source_node (GimpGroupLayer  *group);
static void        gimp_group_layer_update_mode_node (GimpGroupLayer  *group);

static void            gimp_group_layer_stack_update (GimpDrawableStack *stack,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      GimpGroupLayer    *group);
static void            gimp_group_layer_proj_update  (GimpProjection    *proj,
                                                      gboolean           now,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      GimpGroupLayer    *group);


G_DEFINE_TYPE_WITH_CODE (GimpGroupLayer, gimp_group_layer, GIMP_TYPE_LAYER,
                         G_ADD_PRIVATE (GimpGroupLayer)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROJECTABLE,
                                                gimp_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_pickable_iface_init))


#define parent_class gimp_group_layer_parent_class


/* disable pass-through groups strength-reduction to normal groups.
 * see gimp_group_layer_get_effective_mode().
 */
static gboolean no_pass_through_strength_reduction = FALSE;


static void
gimp_group_layer_class_init (GimpGroupLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);
  GimpLayerClass    *layer_class       = GIMP_LAYER_CLASS (klass);

  object_class->set_property             = gimp_group_layer_set_property;
  object_class->get_property             = gimp_group_layer_get_property;
  object_class->finalize                 = gimp_group_layer_finalize;

  gimp_object_class->get_memsize         = gimp_group_layer_get_memsize;

  viewable_class->default_icon_name      = "gimp-group-layer";
  viewable_class->ancestry_changed       = gimp_group_layer_ancestry_changed;
  viewable_class->get_size               = gimp_group_layer_get_size;
  viewable_class->get_children           = gimp_group_layer_get_children;
  viewable_class->set_expanded           = gimp_group_layer_set_expanded;
  viewable_class->get_expanded           = gimp_group_layer_get_expanded;

  item_class->is_position_locked         = gimp_group_layer_is_position_locked;
  item_class->duplicate                  = gimp_group_layer_duplicate;
  item_class->convert                    = gimp_group_layer_convert;
  item_class->start_transform            = gimp_group_layer_start_transform;
  item_class->end_transform              = gimp_group_layer_end_transform;
  item_class->resize                     = gimp_group_layer_resize;
  item_class->get_clip                   = gimp_group_layer_get_clip;

  item_class->default_name               = _("Layer Group");
  item_class->rename_desc                = C_("undo-type", "Rename Layer Group");
  item_class->translate_desc             = C_("undo-type", "Move Layer Group");
  item_class->scale_desc                 = C_("undo-type", "Scale Layer Group");
  item_class->resize_desc                = C_("undo-type", "Resize Layer Group");
  item_class->flip_desc                  = C_("undo-type", "Flip Layer Group");
  item_class->rotate_desc                = C_("undo-type", "Rotate Layer Group");
  item_class->transform_desc             = C_("undo-type", "Transform Layer Group");

  drawable_class->estimate_memsize       = gimp_group_layer_estimate_memsize;
  drawable_class->update_all             = gimp_group_layer_update_all;
  drawable_class->get_source_node        = gimp_group_layer_get_source_node;

  layer_class->opacity_changed           = gimp_group_layer_opacity_changed;
  layer_class->effective_mode_changed    = gimp_group_layer_effective_mode_changed;
  layer_class->excludes_backdrop_changed = gimp_group_layer_excludes_backdrop_changed;
  layer_class->translate                 = gimp_group_layer_translate;
  layer_class->scale                     = gimp_group_layer_scale;
  layer_class->flip                      = gimp_group_layer_flip;
  layer_class->rotate                    = gimp_group_layer_rotate;
  layer_class->transform                 = gimp_group_layer_transform;
  layer_class->convert_type              = gimp_group_layer_convert_type;
  layer_class->get_bounding_box          = gimp_group_layer_get_bounding_box;
  layer_class->get_effective_mode        = gimp_group_layer_get_effective_mode;
  layer_class->get_excludes_backdrop     = gimp_group_layer_get_excludes_backdrop;

  if (g_getenv ("GIMP_NO_PASS_THROUGH_STRENGTH_REDUCTION"))
    no_pass_through_strength_reduction = TRUE;
}

static void
gimp_projectable_iface_init (GimpProjectableInterface *iface)
{
  iface->get_image          = (GimpImage * (*) (GimpProjectable *)) gimp_item_get_image;
  iface->get_format         = gimp_group_layer_get_format;
  iface->get_offset         = (void (*) (GimpProjectable*, gint*, gint*)) gimp_item_get_offset;
  iface->get_bounding_box   = gimp_group_layer_projectable_get_bounding_box;
  iface->get_graph          = gimp_group_layer_get_graph;
  iface->begin_render       = gimp_group_layer_begin_render;
  iface->end_render         = gimp_group_layer_end_render;
  iface->invalidate_preview = (void (*) (GimpProjectable*)) gimp_viewable_invalidate_preview;
}

static void
gimp_pickable_iface_init (GimpPickableInterface *iface)
{
  iface->flush          = gimp_group_layer_pickable_flush;
  iface->get_opacity_at = gimp_group_layer_get_opacity_at;
}

static void
gimp_group_layer_init (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);

  private->children = gimp_layer_stack_new (GIMP_TYPE_LAYER);
  private->expanded = TRUE;

  g_signal_connect (private->children, "add",
                    G_CALLBACK (gimp_group_layer_child_add),
                    group);
  g_signal_connect (private->children, "remove",
                    G_CALLBACK (gimp_group_layer_child_remove),
                    group);

  gimp_container_add_handler (private->children, "notify::offset-x",
                              G_CALLBACK (gimp_group_layer_child_move),
                              group);
  gimp_container_add_handler (private->children, "notify::offset-y",
                              G_CALLBACK (gimp_group_layer_child_move),
                              group);
  gimp_container_add_handler (private->children, "size-changed",
                              G_CALLBACK (gimp_group_layer_child_resize),
                              group);
  gimp_container_add_handler (private->children, "bounding-box-changed",
                              G_CALLBACK (gimp_group_layer_child_resize),
                              group);
  gimp_container_add_handler (private->children, "active-changed",
                              G_CALLBACK (gimp_group_layer_child_active_changed),
                              group);
  gimp_container_add_handler (private->children, "effective-mode-changed",
                              G_CALLBACK (gimp_group_layer_child_effective_mode_changed),
                              group);
  gimp_container_add_handler (private->children, "excludes-backdrop-changed",
                              G_CALLBACK (gimp_group_layer_child_excludes_backdrop_changed),
                              group);

  g_signal_connect (private->children, "update",
                    G_CALLBACK (gimp_group_layer_stack_update),
                    group);

  private->projection = gimp_projection_new (GIMP_PROJECTABLE (group));
  gimp_projection_set_priority (private->projection, 1);

  g_signal_connect (private->projection, "update",
                    G_CALLBACK (gimp_group_layer_proj_update),
                    group);
}

static void
gimp_group_layer_finalize (GObject *object)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (object);

  if (private->children)
    {
      g_signal_handlers_disconnect_by_func (private->children,
                                            gimp_group_layer_child_add,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            gimp_group_layer_child_remove,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            gimp_group_layer_stack_update,
                                            object);

      /* this is particularly important to avoid reallocating the projection
       * in response to a "bounding-box-changed" signal, which can be emitted
       * during layer destruction.  see issue #4584.
       */
      gimp_container_remove_handlers_by_data (private->children, object);

      g_clear_object (&private->children);
    }

  g_clear_object (&private->projection);
  g_clear_object (&private->source_node);
  g_clear_object (&private->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_group_layer_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_group_layer_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_group_layer_get_memsize (GimpObject *object,
                              gint64     *gui_size)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (object);
  gint64                 memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->children), gui_size);
  memsize += gimp_object_get_memsize (GIMP_OBJECT (private->projection), gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_group_layer_ancestry_changed (GimpViewable *viewable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (viewable);

  gimp_projection_set_priority (private->projection,
                                gimp_viewable_get_depth (viewable) + 1);

  GIMP_VIEWABLE_CLASS (parent_class)->ancestry_changed (viewable);
}

static gboolean
gimp_group_layer_get_size (GimpViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (viewable);

  /*  return the size only if there are children ...  */
  if (! gimp_container_is_empty (private->children))
    {
      return GIMP_VIEWABLE_CLASS (parent_class)->get_size (viewable,
                                                           width, height);
    }

  /*  ... otherwise, return "no content"  */
  return FALSE;
}

static GimpContainer *
gimp_group_layer_get_children (GimpViewable *viewable)
{
  return GET_PRIVATE (viewable)->children;
}

static gboolean
gimp_group_layer_get_expanded (GimpViewable *viewable)
{
  GimpGroupLayer *group = GIMP_GROUP_LAYER (viewable);

  return GET_PRIVATE (group)->expanded;
}

static void
gimp_group_layer_set_expanded (GimpViewable *viewable,
                               gboolean      expanded)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (viewable);

  if (private->expanded != expanded)
    {
      private->expanded = expanded;

      gimp_viewable_expanded_changed (viewable);
    }
}

static gboolean
gimp_group_layer_is_position_locked (GimpItem *item)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      if (gimp_item_is_position_locked (child))
        return TRUE;
    }

  return GIMP_ITEM_CLASS (parent_class)->is_position_locked (item);
}

static GimpItem *
gimp_group_layer_duplicate (GimpItem *item,
                            GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (GIMP_IS_GROUP_LAYER (new_item))
    {
      GimpGroupLayerPrivate *private     = GET_PRIVATE (item);
      GimpGroupLayer        *new_group   = GIMP_GROUP_LAYER (new_item);
      GimpGroupLayerPrivate *new_private = GET_PRIVATE (new_item);
      gint                   position    = 0;
      GList                 *list;

      gimp_group_layer_suspend_resize (new_group, FALSE);

      for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          GimpItem      *child = list->data;
          GimpItem      *new_child;
          GimpLayerMask *mask;

          new_child = gimp_item_duplicate (child, G_TYPE_FROM_INSTANCE (child));

          gimp_object_set_name (GIMP_OBJECT (new_child),
                                gimp_object_get_name (child));

          mask = gimp_layer_get_mask (GIMP_LAYER (child));

          if (mask)
            {
              GimpLayerMask *new_mask;

              new_mask = gimp_layer_get_mask (GIMP_LAYER (new_child));

              gimp_object_set_name (GIMP_OBJECT (new_mask),
                                    gimp_object_get_name (mask));
            }

          gimp_viewable_set_parent (GIMP_VIEWABLE (new_child),
                                    GIMP_VIEWABLE (new_group));

          gimp_container_insert (new_private->children,
                                 GIMP_OBJECT (new_child),
                                 position++);
        }

      /*  force the projection to reallocate itself  */
      GET_PRIVATE (new_group)->reallocate_projection = TRUE;

      gimp_group_layer_resume_resize (new_group, FALSE);
    }

  return new_item;
}

static void
gimp_group_layer_convert (GimpItem  *item,
                          GimpImage *dest_image,
                          GType      old_type)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      GIMP_ITEM_GET_CLASS (child)->convert (child, dest_image,
                                            G_TYPE_FROM_INSTANCE (child));
    }

  GIMP_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
gimp_group_layer_start_transform (GimpItem *item,
                             gboolean  push_undo)
{
  _gimp_group_layer_start_transform (GIMP_GROUP_LAYER (item), push_undo);

  if (GIMP_ITEM_CLASS (parent_class)->start_transform)
    GIMP_ITEM_CLASS (parent_class)->start_transform (item, push_undo);
}

static void
gimp_group_layer_end_transform (GimpItem *item,
                           gboolean  push_undo)
{
  if (GIMP_ITEM_CLASS (parent_class)->end_transform)
    GIMP_ITEM_CLASS (parent_class)->end_transform (item, push_undo);

  _gimp_group_layer_end_transform (GIMP_GROUP_LAYER (item), push_undo);
}

static void
gimp_group_layer_resize (GimpItem     *item,
                         GimpContext  *context,
                         GimpFillType  fill_type,
                         gint          new_width,
                         gint          new_height,
                         gint          offset_x,
                         gint          offset_y)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;
  gint                   x, y;

  /* we implement GimpItem::resize(), instead of GimpLayer::resize(), so that
   * GimpLayer doesn't resize the mask.  note that gimp_item_resize() calls
   * gimp_item_{start,end}_move(), and not gimp_item_{start,end}_transform(),
   * so that mask resizing is handled by gimp_group_layer_update_size().
   */

  x = gimp_item_get_offset_x (item) - offset_x;
  y = gimp_item_get_offset_y (item) - offset_y;

  gimp_group_layer_suspend_resize (group, TRUE);

  list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));

  while (list)
    {
      GimpItem *child = list->data;
      gint      child_width;
      gint      child_height;
      gint      child_x;
      gint      child_y;

      list = g_list_next (list);

      if (gimp_rectangle_intersect (x,
                                    y,
                                    new_width,
                                    new_height,
                                    gimp_item_get_offset_x (child),
                                    gimp_item_get_offset_y (child),
                                    gimp_item_get_width  (child),
                                    gimp_item_get_height (child),
                                    &child_x,
                                    &child_y,
                                    &child_width,
                                    &child_height))
        {
          gint child_offset_x = gimp_item_get_offset_x (child) - child_x;
          gint child_offset_y = gimp_item_get_offset_y (child) - child_y;

          gimp_item_resize (child, context, fill_type,
                            child_width, child_height,
                            child_offset_x, child_offset_y);
        }
      else if (gimp_item_is_attached (item))
        {
          gimp_image_remove_layer (gimp_item_get_image (item),
                                   GIMP_LAYER (child),
                                   TRUE, NULL);
        }
      else
        {
          gimp_container_remove (private->children, GIMP_OBJECT (child));
        }
    }

  gimp_group_layer_resume_resize (group, TRUE);
}

static GimpTransformResize
gimp_group_layer_get_clip (GimpItem            *item,
                           GimpTransformResize  clip_result)
{
  /* TODO: add clipping support, by clipping all sublayers as a unit, instead
   * of individually.
   */
  return GIMP_TRANSFORM_RESIZE_ADJUST;
}

static gint64
gimp_group_layer_estimate_memsize (GimpDrawable      *drawable,
                                   GimpComponentType  component_type,
                                   gint               width,
                                   gint               height)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;
  GimpImageBaseType      base_type;
  gint64                 memsize = 0;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpDrawable *child = list->data;
      gint          child_width;
      gint          child_height;

      child_width  = (gimp_item_get_width (GIMP_ITEM (child)) *
                      width /
                      gimp_item_get_width (GIMP_ITEM (drawable)));
      child_height = (gimp_item_get_height (GIMP_ITEM (child)) *
                      height /
                      gimp_item_get_height (GIMP_ITEM (drawable)));

      memsize += gimp_drawable_estimate_memsize (child,
                                                 component_type,
                                                 child_width,
                                                 child_height);
    }

  base_type = gimp_drawable_get_base_type (drawable);

  memsize += gimp_projection_estimate_memsize (base_type, component_type,
                                               width, height);

  return memsize +
         GIMP_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static void
gimp_group_layer_update_all (GimpDrawable *drawable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;

  /*  redirect stack updates to the drawable, rather than to the projection  */
  private->direct_update++;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpFilter *child = list->data;

      if (gimp_filter_get_active (child))
        gimp_drawable_update_all (GIMP_DRAWABLE (child));
    }

  /*  redirect stack updates back to the projection  */
  private->direct_update--;
}

static void
gimp_group_layer_translate (GimpLayer *layer,
                            gint       offset_x,
                            gint       offset_y)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  gint                   x, y;
  GList                 *list;

  /*  don't use gimp_group_layer_suspend_resize(), but rather increment
   *  private->suspend_resize directly, since we're translating the group layer
   *  here, rather than relying on gimp_group_layer_update_size() to do it.
   */
  private->suspend_resize++;

  /*  redirect stack updates to the drawable, rather than to the projection  */
  private->direct_update++;

  /*  translate the child layers *before* updating the group's offset, so that,
   *  if this is a nested group, the parent's bounds still reflect the original
   *  layer positions.  This prevents the original area of the child layers,
   *  which is updated as part of their translation, from being clipped to the
   *  post-translation parent bounds (see issue #3484).  The new area of the
   *  child layers, which is likewise updated as part of translation, *may* get
   *  clipped to the old parent bounds, but the corresponding region will be
   *  updated anyway when the parent is resized, once we update the group's
   *  offset.
   */
  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      /*  don't push an undo here because undo will call us again  */
      gimp_item_translate (child, offset_x, offset_y, FALSE);
    }

  gimp_item_get_offset (GIMP_ITEM (group), &x, &y);

  x += offset_x;
  y += offset_y;

  /*  update the offset node  */
  if (private->offset_node)
    gegl_node_set (private->offset_node,
                   "x", (gdouble) -x,
                   "y", (gdouble) -y,
                   NULL);

  /*  invalidate the selection boundary because of a layer modification  */
  gimp_drawable_invalidate_boundary (GIMP_DRAWABLE (layer));

  /*  update the group layer offset  */
  gimp_item_set_offset (GIMP_ITEM (group), x, y);

  /*  redirect stack updates back to the projection  */
  private->direct_update--;

  /*  don't use gimp_group_layer_resume_resize(), but rather decrement
   *  private->suspend_resize directly, so that gimp_group_layer_update_size()
   *  isn't called.
   */
  private->suspend_resize--;
}

static void
gimp_group_layer_scale (GimpLayer             *layer,
                        gint                   new_width,
                        gint                   new_height,
                        gint                   new_offset_x,
                        gint                   new_offset_y,
                        GimpInterpolationType  interpolation_type,
                        GimpProgress          *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  GimpItem              *item    = GIMP_ITEM (layer);
  GimpObjectQueue       *queue   = NULL;
  GList                 *list;
  gdouble                width_factor;
  gdouble                height_factor;
  gint                   old_offset_x;
  gint                   old_offset_y;

  width_factor  = (gdouble) new_width  / (gdouble) gimp_item_get_width  (item);
  height_factor = (gdouble) new_height / (gdouble) gimp_item_get_height (item);

  old_offset_x = gimp_item_get_offset_x (item);
  old_offset_y = gimp_item_get_offset_y (item);

  if (progress)
    {
      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      gimp_object_queue_push_container (queue, private->children);
    }

  gimp_group_layer_suspend_resize (group, TRUE);

  list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));

  while (list)
    {
      GimpItem *child = list->data;

      list = g_list_next (list);

      if (queue)
        gimp_object_queue_pop (queue);

      if (! gimp_item_scale_by_factors_with_origin (child,
                                                    width_factor, height_factor,
                                                    old_offset_x, old_offset_y,
                                                    new_offset_x, new_offset_y,
                                                    interpolation_type,
                                                    progress))
        {
          /* new width or height are 0; remove item */
          if (gimp_item_is_attached (item))
            {
              gimp_image_remove_layer (gimp_item_get_image (item),
                                       GIMP_LAYER (child),
                                       TRUE, NULL);
            }
          else
            {
              gimp_container_remove (private->children, GIMP_OBJECT (child));
            }
        }
    }

  gimp_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static void
gimp_group_layer_flip (GimpLayer           *layer,
                       GimpContext         *context,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  gimp_group_layer_suspend_resize (group, TRUE);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      gimp_item_flip (child, context,
                      flip_type, axis, clip_result);
    }

  gimp_group_layer_resume_resize (group, TRUE);
}

static void
gimp_group_layer_rotate (GimpLayer        *layer,
                         GimpContext      *context,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  gimp_group_layer_suspend_resize (group, TRUE);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      gimp_item_rotate (child, context,
                        rotate_type, center_x, center_y, clip_result);
    }

  gimp_group_layer_resume_resize (group, TRUE);
}

static void
gimp_group_layer_transform (GimpLayer              *layer,
                            GimpContext            *context,
                            const GimpMatrix3      *matrix,
                            GimpTransformDirection  direction,
                            GimpInterpolationType   interpolation_type,
                            GimpTransformResize     clip_result,
                            GimpProgress           *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  GimpObjectQueue       *queue   = NULL;
  GList                 *list;

  if (progress)
    {
      queue    = gimp_object_queue_new (progress);
      progress = GIMP_PROGRESS (queue);

      gimp_object_queue_push_container (queue, private->children);
    }

  gimp_group_layer_suspend_resize (group, TRUE);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      if (queue)
        gimp_object_queue_pop (queue);

      gimp_item_transform (child, context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  gimp_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static const Babl *
get_projection_format (GimpProjectable   *projectable,
                       GimpImageBaseType  base_type,
                       GimpPrecision      precision)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (projectable));

  switch (base_type)
    {
    case GIMP_RGB:
    case GIMP_INDEXED:
      return gimp_image_get_format (image, GIMP_RGB, precision, TRUE);

    case GIMP_GRAY:
      return gimp_image_get_format (image, GIMP_GRAY, precision, TRUE);
    }

  g_return_val_if_reached (NULL);
}

static void
gimp_group_layer_convert_type (GimpLayer        *layer,
                               GimpImage        *dest_image,
                               const Babl       *new_format,
                               GimpColorProfile *dest_profile,
                               GeglDitherMethod  layer_dither_type,
                               GeglDitherMethod  mask_dither_type,
                               gboolean          push_undo,
                               GimpProgress     *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);
  GeglBuffer            *buffer;

  if (push_undo)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (group));

      gimp_image_undo_push_group_layer_convert (image, NULL, group);
    }

  /*  Need to temporarily set the projectable's format to the new
   *  values so the projection will create its tiles with the right
   *  depth
   */
  private->convert_format =
    get_projection_format (GIMP_PROJECTABLE (group),
                           gimp_babl_format_get_base_type (new_format),
                           gimp_babl_format_get_precision (new_format));
  gimp_projectable_structure_changed (GIMP_PROJECTABLE (group));
  gimp_group_layer_flush (group);

  buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));

  gimp_drawable_set_buffer_full (GIMP_DRAWABLE (group),
                                 FALSE, NULL,
                                 buffer, NULL,
                                 TRUE);

  /*  reset, the actual format is right now  */
  private->convert_format = NULL;
}

static GeglNode *
gimp_group_layer_get_source_node (GimpDrawable *drawable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GeglNode              *input;

  g_warn_if_fail (private->source_node == NULL);

  private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (private->source_node, "input");

  private->parent_source_node =
    GIMP_DRAWABLE_CLASS (parent_class)->get_source_node (drawable);

  gegl_node_add_child (private->source_node, private->parent_source_node);

  g_object_unref (private->parent_source_node);

  if (gegl_node_has_pad (private->parent_source_node, "input"))
    {
      gegl_node_connect_to (input,                       "output",
                            private->parent_source_node, "input");
    }

  /* make sure we have a graph */
  (void) gimp_group_layer_get_graph (GIMP_PROJECTABLE (drawable));

  gegl_node_add_child (private->source_node, private->graph);

  gimp_group_layer_update_source_node (GIMP_GROUP_LAYER (drawable));

  return g_object_ref (private->source_node);
}

static void
gimp_group_layer_opacity_changed (GimpLayer *layer)
{
  gimp_layer_update_effective_mode (layer);

  if (GIMP_LAYER_CLASS (parent_class)->opacity_changed)
    GIMP_LAYER_CLASS (parent_class)->opacity_changed (layer);
}

static void
gimp_group_layer_effective_mode_changed (GimpLayer *layer)
{
  GimpGroupLayer        *group               = GIMP_GROUP_LAYER (layer);
  GimpGroupLayerPrivate *private             = GET_PRIVATE (layer);
  GimpLayerMode          mode;
  gboolean               pass_through;
  gboolean               update_bounding_box = FALSE;

  gimp_layer_get_effective_mode (layer, &mode, NULL, NULL, NULL);

  pass_through = (mode == GIMP_LAYER_MODE_PASS_THROUGH);

  if (pass_through != private->pass_through)
    {
      if (private->pass_through && ! pass_through)
        {
          /* when switching from pass-through mode to a non-pass-through mode,
           * flush the pickable in order to make sure the projection's buffer
           * gets properly invalidated synchronously, so that it can be used
           * as a source for the rest of the composition.
           */
          gimp_pickable_flush (GIMP_PICKABLE (private->projection));
        }

      private->pass_through = pass_through;

      update_bounding_box = TRUE;
    }

  gimp_group_layer_update_source_node (group);
  gimp_group_layer_update_mode_node (group);

  if (update_bounding_box)
    gimp_drawable_update_bounding_box (GIMP_DRAWABLE (group));

  if (GIMP_LAYER_CLASS (parent_class)->effective_mode_changed)
    GIMP_LAYER_CLASS (parent_class)->effective_mode_changed (layer);
}

static void
gimp_group_layer_excludes_backdrop_changed (GimpLayer *layer)
{
  GimpGroupLayer *group = GIMP_GROUP_LAYER (layer);

  gimp_group_layer_update_source_node (group);
  gimp_group_layer_update_mode_node (group);

  if (GIMP_LAYER_CLASS (parent_class)->excludes_backdrop_changed)
    GIMP_LAYER_CLASS (parent_class)->excludes_backdrop_changed (layer);
}

static GeglRectangle
gimp_group_layer_get_bounding_box (GimpLayer *layer)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);

  /* for pass-through groups, use the group's calculated bounding box, instead
   * of the source-node's bounding box, since we don't update the bounding box
   * on all events that may affect the latter, and since it includes the
   * bounding box of the backdrop.  this means we can't attach filters that may
   * affect the bounding box to a pass-through group (since their effect weon't
   * be reflected by the group's bounding box), but attaching filters to pass-
   * through groups makes little sense anyway.
   */
  if (private->pass_through)
    return private->bounding_box;
  else
    return GIMP_LAYER_CLASS (parent_class)->get_bounding_box (layer);
}

static void
gimp_group_layer_get_effective_mode (GimpLayer              *layer,
                                     GimpLayerMode          *mode,
                                     GimpLayerColorSpace    *blend_space,
                                     GimpLayerColorSpace    *composite_space,
                                     GimpLayerCompositeMode *composite_mode)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);

  /* try to strength-reduce pass-through groups to normal groups, which are
   * cheaper.
   */
  if (gimp_layer_get_mode (layer) == GIMP_LAYER_MODE_PASS_THROUGH &&
      ! no_pass_through_strength_reduction)
    {
      /* we perform the strength-reduction if:
       *
       *   - the group has no active children;
       *
       *   or,
       *
       *     - the group has a single active child; or,
       *
       *     - the effective mode of all the active children is normal, their
       *       effective composite mode is UNION, and their effective blend and
       *       composite spaces are equal;
       *
       *   - and,
       *
       *     - the group's opacity is 100%, and it has no mask (or the mask
       *       isn't applied); or,
       *
       *     - the group's composite space equals the active children's
       *       composite space.
       */

      GList    *list;
      gboolean  reduce = TRUE;
      gboolean  first  = TRUE;

      *mode            = GIMP_LAYER_MODE_NORMAL;
      *blend_space     = gimp_layer_get_real_blend_space (layer);
      *composite_space = gimp_layer_get_real_composite_space (layer);
      *composite_mode  = gimp_layer_get_real_composite_mode (layer);

      for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          GimpLayer *child = list->data;

          if (! gimp_filter_get_active (GIMP_FILTER (child)))
            continue;

          if (first)
            {
              gimp_layer_get_effective_mode (child,
                                             mode,
                                             blend_space,
                                             composite_space,
                                             composite_mode);

              if (*mode == GIMP_LAYER_MODE_NORMAL_LEGACY)
                *mode = GIMP_LAYER_MODE_NORMAL;

              first = FALSE;
            }
          else
            {
              GimpLayerMode          other_mode;
              GimpLayerColorSpace    other_blend_space;
              GimpLayerColorSpace    other_composite_space;
              GimpLayerCompositeMode other_composite_mode;

              if (*mode           != GIMP_LAYER_MODE_NORMAL ||
                  *composite_mode != GIMP_LAYER_COMPOSITE_UNION)
                {
                  reduce = FALSE;

                  break;
                }

              gimp_layer_get_effective_mode (child,
                                             &other_mode,
                                             &other_blend_space,
                                             &other_composite_space,
                                             &other_composite_mode);

              if (other_mode == GIMP_LAYER_MODE_NORMAL_LEGACY)
                other_mode = GIMP_LAYER_MODE_NORMAL;

              if (other_mode            != *mode            ||
                  other_blend_space     != *blend_space     ||
                  other_composite_space != *composite_space ||
                  other_composite_mode  != *composite_mode)
                {
                  reduce = FALSE;

                  break;
                }
            }
        }

      if (reduce)
        {
          gboolean has_mask;

          has_mask = gimp_layer_get_mask (layer) &&
                     gimp_layer_get_apply_mask (layer);

          if (first                                                  ||
              (gimp_layer_get_opacity (layer) == GIMP_OPACITY_OPAQUE &&
               ! has_mask)                                           ||
              *composite_space == gimp_layer_get_real_composite_space (layer))
            {
              /* strength reduction succeeded! */
              return;
            }
        }
    }

  /* strength-reduction failed.  chain up. */
  GIMP_LAYER_CLASS (parent_class)->get_effective_mode (layer,
                                                       mode,
                                                       blend_space,
                                                       composite_space,
                                                       composite_mode);
}

static gboolean
gimp_group_layer_get_excludes_backdrop (GimpLayer *layer)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (layer);

  if (private->pass_through)
    {
      GList *list;

      for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          GimpFilter *child = list->data;

          if (gimp_filter_get_active (child) &&
              gimp_layer_get_excludes_backdrop (GIMP_LAYER (child)))
            return TRUE;
        }

      return FALSE;
    }
  else
    return GIMP_LAYER_CLASS (parent_class)->get_excludes_backdrop (layer);
}

static const Babl *
gimp_group_layer_get_format (GimpProjectable *projectable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);
  GimpImageBaseType      base_type;
  GimpPrecision          precision;

  if (private->convert_format)
    return private->convert_format;

  base_type = gimp_drawable_get_base_type (GIMP_DRAWABLE (projectable));
  precision = gimp_drawable_get_precision (GIMP_DRAWABLE (projectable));

  return get_projection_format (projectable, base_type, precision);
}

static GeglRectangle
gimp_group_layer_projectable_get_bounding_box (GimpProjectable *projectable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);

  return private->bounding_box;
}

static GeglNode *
gimp_group_layer_get_graph (GimpProjectable *projectable)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (projectable);
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);
  GeglNode              *input;
  GeglNode              *layers_node;
  GeglNode              *output;
  gint                   off_x;
  gint                   off_y;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  input = gegl_node_get_input_proxy (private->graph, "input");

  layers_node =
    gimp_filter_stack_get_graph (GIMP_FILTER_STACK (private->children));

  gegl_node_add_child (private->graph, layers_node);

  gegl_node_connect_to (input,       "output",
                        layers_node, "input");

  gimp_item_get_offset (GIMP_ITEM (group), &off_x, &off_y);

  private->offset_node = gegl_node_new_child (private->graph,
                                              "operation", "gegl:translate",
                                              "x",         (gdouble) -off_x,
                                              "y",         (gdouble) -off_y,
                                              NULL);

  gegl_node_connect_to (layers_node,          "output",
                        private->offset_node, "input");

  output = gegl_node_get_output_proxy (private->graph, "output");

  gegl_node_connect_to (private->offset_node, "output",
                        output,               "input");

  return private->graph;
}

static void
gimp_group_layer_begin_render (GimpProjectable *projectable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);

  if (private->source_node == NULL)
    return;

  if (private->pass_through)
    gegl_node_disconnect (private->graph, "input");
}

static void
gimp_group_layer_end_render (GimpProjectable *projectable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);

  if (private->source_node == NULL)
    return;

  if (private->pass_through)
    {
      GeglNode *input;

      input = gegl_node_get_input_proxy (private->source_node, "input");

      gegl_node_connect_to (input,          "output",
                            private->graph, "input");
    }
}

static void
gimp_group_layer_pickable_flush (GimpPickable *pickable)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (pickable);

  gimp_pickable_flush (GIMP_PICKABLE (private->projection));
}

static gdouble
gimp_group_layer_get_opacity_at (GimpPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  /* Only consider child layers as having content */

  return GIMP_OPACITY_TRANSPARENT;
}


/*  public functions  */

GimpLayer *
gimp_group_layer_new (GimpImage *image)
{
  GimpGroupLayer *group;
  const Babl     *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  format = gimp_image_get_layer_format (image, TRUE);

  group = GIMP_GROUP_LAYER (gimp_drawable_new (GIMP_TYPE_GROUP_LAYER,
                                               image, NULL,
                                               0, 0, 1, 1,
                                               format));

  gimp_layer_set_mode (GIMP_LAYER (group),
                       gimp_image_get_default_new_layer_mode (image),
                       FALSE);

  return GIMP_LAYER (group);
}

GimpProjection *
gimp_group_layer_get_projection (GimpGroupLayer *group)
{
  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);

  return GET_PRIVATE (group)->projection;
}

void
gimp_group_layer_suspend_resize (GimpGroupLayer *group,
                                 gboolean        push_undo)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  item = GIMP_ITEM (group);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_suspend_resize (gimp_item_get_image (item),
                                                     NULL, group);

  GET_PRIVATE (group)->suspend_resize++;
}

void
gimp_group_layer_resume_resize (GimpGroupLayer *group,
                                gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;
  GimpItem              *mask = NULL;
  GeglBuffer            *mask_buffer;
  GeglRectangle          mask_bounds;
  GimpUndo              *undo;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_resize > 0);

  item = GIMP_ITEM (group);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      undo =
        gimp_image_undo_push_group_layer_resume_resize (gimp_item_get_image (item),
                                                        NULL, group);

      /* if there were any {suspend,resume}_mask() calls during the time the
       * group's size was suspended, the resume_mask() calls will not have seen
       * any changes to the mask, and will therefore won't restore the mask
       * during undo.  if the group's bounding box did change while resize was
       * suspended, and if there are no other {suspend,resume}_mask() blocks
       * that will see the resized mask, we have to restore the mask during the
       * resume_resize() undo.
       *
       * we ref the mask buffer here, and compare it to the mask buffer after
       * updating the size.
       */
      if (private->suspend_resize == 1 && private->suspend_mask == 0)
        {
          mask = GIMP_ITEM (gimp_layer_get_mask (GIMP_LAYER (group)));

          if (mask)
            {
              mask_buffer =
                g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)));

              mask_bounds.x      = gimp_item_get_offset_x (mask);
              mask_bounds.y      = gimp_item_get_offset_y (mask);
              mask_bounds.width  = gimp_item_get_width    (mask);
              mask_bounds.height = gimp_item_get_height   (mask);
            }
        }
    }

  private->suspend_resize--;

  if (private->suspend_resize == 0)
    {
      gimp_group_layer_update_size (group);

      if (mask)
        {
          /* if the mask changed, make sure it's restored during undo, as per
           * the comment above.
           */
          if (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)) != mask_buffer)
            {
              g_return_if_fail (undo != NULL);

              GIMP_GROUP_LAYER_UNDO (undo)->mask_buffer = mask_buffer;
              GIMP_GROUP_LAYER_UNDO (undo)->mask_bounds = mask_bounds;
            }
          else
            {
              g_object_unref (mask_buffer);
            }
        }
    }
}

void
gimp_group_layer_suspend_mask (GimpGroupLayer *group,
                               gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = GIMP_ITEM (group);

  /* avoid pushing an undo step if this is a nested suspend_mask() call, since
   * the value of 'push_undo' in nested calls should be the same as that passed
   * to the outermost call, and only pushing an undo step for the outermost
   * call in this case is enough.  we can't support cases where the values of
   * 'push_undo' in nested calls are different in a meaningful way, and
   * avoiding undo steps for nested calls prevents us from storing multiple
   * references to the suspend mask buffer on the undo stack.  while storing
   * multiple references to the buffer doesn't waste any memory (since all the
   * references are to the same buffer), it does cause the undo stack memory-
   * usage estimation to overshoot, potentially resulting in undo steps being
   * dropped unnecessarily.
   */
  if (! gimp_item_is_attached (item) || private->suspend_mask > 0)
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_suspend_mask (gimp_item_get_image (item),
                                                   NULL, group);

  if (private->suspend_mask == 0)
    {
      if (gimp_layer_get_mask (GIMP_LAYER (group)))
        {
          GimpItem *mask = GIMP_ITEM (gimp_layer_get_mask (GIMP_LAYER (group)));

          private->suspended_mask_buffer =
            g_object_ref (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)));

          private->suspended_mask_bounds.x      = gimp_item_get_offset_x (mask);
          private->suspended_mask_bounds.y      = gimp_item_get_offset_y (mask);
          private->suspended_mask_bounds.width  = gimp_item_get_width    (mask);
          private->suspended_mask_bounds.height = gimp_item_get_height   (mask);
        }
      else
        {
          private->suspended_mask_buffer = NULL;
        }
    }

  private->suspend_mask++;
}

void
gimp_group_layer_resume_mask (GimpGroupLayer *group,
                              gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_mask > 0);

  item = GIMP_ITEM (group);

  /* avoid pushing an undo step if this is a nested resume_mask() call.  see
   * the comment in gimp_group_layer_suspend_mask().
   */
  if (! gimp_item_is_attached (item) || private->suspend_mask > 1)
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_resume_mask (gimp_item_get_image (item),
                                                  NULL, group);

  private->suspend_mask--;

  if (private->suspend_mask == 0)
    g_clear_object (&private->suspended_mask_buffer);
}


/*  protected functions  */

void
_gimp_group_layer_set_suspended_mask (GimpGroupLayer      *group,
                                      GeglBuffer          *buffer,
                                      const GeglRectangle *bounds)
{
  GimpGroupLayerPrivate *private;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (bounds != NULL);

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_mask > 0);

  g_object_ref (buffer);

  g_clear_object (&private->suspended_mask_buffer);

  private->suspended_mask_buffer = buffer;
  private->suspended_mask_bounds = *bounds;
}

GeglBuffer *
_gimp_group_layer_get_suspended_mask (GimpGroupLayer *group,
                                      GeglRectangle  *bounds)
{
  GimpGroupLayerPrivate *private;
  GimpLayerMask         *mask;

  g_return_val_if_fail (GIMP_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (bounds != NULL, NULL);

  private = GET_PRIVATE (group);
  mask    = gimp_layer_get_mask (GIMP_LAYER (group));

  g_return_val_if_fail (private->suspend_mask > 0, NULL);

  if (mask &&
      gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)) !=
      private->suspended_mask_buffer)
    {
      *bounds = private->suspended_mask_bounds;

      return private->suspended_mask_buffer;
    }

  return NULL;
}

void
_gimp_group_layer_start_transform (GimpGroupLayer *group,
                                   gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = GIMP_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_start_transform (gimp_item_get_image (item),
                                                      NULL, group);

  private->transforming++;
}

void
_gimp_group_layer_end_transform (GimpGroupLayer *group,
                                 gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = GIMP_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);
  g_return_if_fail (private->transforming > 0);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_end_transform (gimp_item_get_image (item),
                                                    NULL, group);

  private->transforming--;

  if (private->transforming == 0)
    gimp_group_layer_update_mask_size (GIMP_GROUP_LAYER (item));
}


/*  private functions  */

static void
gimp_group_layer_child_add (GimpContainer  *container,
                            GimpLayer      *child,
                            GimpGroupLayer *group)
{
  gimp_group_layer_update (group);

  if (gimp_filter_get_active (GIMP_FILTER (child)))
    {
      gimp_layer_update_effective_mode (GIMP_LAYER (group));

      if (gimp_layer_get_excludes_backdrop (child))
        gimp_layer_update_excludes_backdrop (GIMP_LAYER (group));
    }
}

static void
gimp_group_layer_child_remove (GimpContainer  *container,
                               GimpLayer      *child,
                               GimpGroupLayer *group)
{
  gimp_group_layer_update (group);

  if (gimp_filter_get_active (GIMP_FILTER (child)))
    {
      gimp_layer_update_effective_mode (GIMP_LAYER (group));

      if (gimp_layer_get_excludes_backdrop (child))
        gimp_layer_update_excludes_backdrop (GIMP_LAYER (group));
    }
}

static void
gimp_group_layer_child_move (GimpLayer      *child,
                             GParamSpec     *pspec,
                             GimpGroupLayer *group)
{
  gimp_group_layer_update (group);
}

static void
gimp_group_layer_child_resize (GimpLayer      *child,
                               GimpGroupLayer *group)
{
  gimp_group_layer_update (group);
}

static void
gimp_group_layer_child_active_changed (GimpLayer      *child,
                                       GimpGroupLayer *group)
{
  gimp_layer_update_effective_mode (GIMP_LAYER (group));

  if (gimp_layer_get_excludes_backdrop (child))
    gimp_layer_update_excludes_backdrop (GIMP_LAYER (group));
}

static void
gimp_group_layer_child_effective_mode_changed (GimpLayer      *child,
                                               GimpGroupLayer *group)
{
  if (gimp_filter_get_active (GIMP_FILTER (child)))
    gimp_layer_update_effective_mode (GIMP_LAYER (group));
}

static void
gimp_group_layer_child_excludes_backdrop_changed (GimpLayer      *child,
                                                  GimpGroupLayer *group)
{
  if (gimp_filter_get_active (GIMP_FILTER (child)))
    gimp_layer_update_excludes_backdrop (GIMP_LAYER (group));
}

static void
gimp_group_layer_flush (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);

  if (private->pass_through)
    {
      /*  flush the projectable, not the pickable, because the source
       *  node of pass-through groups doesn't use the projection's
       *  buffer, hence there's no need to invalidate it synchronously.
       */
      gimp_projectable_flush (GIMP_PROJECTABLE (group), TRUE);
    }
  else
    {
      /* make sure we have a buffer, and stop any idle rendering, which is
       * initiated when a new buffer is allocated.  the call to
       * gimp_pickable_flush() below causes any pending idle rendering to
       * finish synchronously, so this needs to happen before.
       */
      gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));
      gimp_projection_stop_rendering (private->projection);

      /*  flush the pickable not the projectable because flushing the
       *  pickable will finish all invalidation on the projection so it
       *  can be used as source (note that it will still be constructed
       *  when the actual read happens, so this it not a performance
       *  problem)
       */
      gimp_pickable_flush (GIMP_PICKABLE (private->projection));
    }
}

static void
gimp_group_layer_update (GimpGroupLayer *group)
{
  if (GET_PRIVATE (group)->suspend_resize == 0)
    {
      gimp_group_layer_update_size (group);
    }
}

static void
gimp_group_layer_update_size (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);
  GimpItem              *item    = GIMP_ITEM (group);
  GimpLayer             *layer   = GIMP_LAYER (group);
  GimpItem              *mask    = GIMP_ITEM (gimp_layer_get_mask (layer));
  GeglRectangle          old_bounds;
  GeglRectangle          bounds;
  GeglRectangle          old_bounding_box;
  GeglRectangle          bounding_box;
  gboolean               first   = TRUE;
  gboolean               size_changed;
  gboolean               resize_mask;
  GList                 *list;

  old_bounds.x      = gimp_item_get_offset_x (item);
  old_bounds.y      = gimp_item_get_offset_y (item);
  old_bounds.width  = gimp_item_get_width    (item);
  old_bounds.height = gimp_item_get_height   (item);

  bounds.x          = 0;
  bounds.y          = 0;
  bounds.width      = 1;
  bounds.height     = 1;

  old_bounding_box  = private->bounding_box;
  bounding_box      = bounds;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem      *child = list->data;
      GeglRectangle  child_bounds;
      GeglRectangle  child_bounding_box;

      if (! gimp_viewable_get_size (GIMP_VIEWABLE (child),
                                    &child_bounds.width, &child_bounds.height))
        {
          /*  ignore children without content (empty group layers);
           *  see bug 777017
           */
          continue;
        }

      gimp_item_get_offset (child, &child_bounds.x, &child_bounds.y);

      child_bounding_box =
        gimp_drawable_get_bounding_box (GIMP_DRAWABLE (child));

      child_bounding_box.x += child_bounds.x;
      child_bounding_box.y += child_bounds.y;

      if (first)
        {
          bounds       = child_bounds;
          bounding_box = child_bounding_box;

          first = FALSE;
        }
      else
        {
          gegl_rectangle_bounding_box (&bounds,
                                       &bounds, &child_bounds);
          gegl_rectangle_bounding_box (&bounding_box,
                                       &bounding_box, &child_bounding_box);
        }
    }

  bounding_box.x -= bounds.x;
  bounding_box.y -= bounds.y;

  size_changed = ! (gegl_rectangle_equal (&bounds,       &old_bounds) &&
                    gegl_rectangle_equal (&bounding_box, &old_bounding_box));

  resize_mask = mask && ! gegl_rectangle_equal (&bounds, &old_bounds);

  /* if we show the mask, invalidate the old mask area */
  if (resize_mask && gimp_layer_get_show_mask (layer))
    {
      gimp_drawable_update (GIMP_DRAWABLE (group),
                            gimp_item_get_offset_x (mask) - old_bounds.x,
                            gimp_item_get_offset_y (mask) - old_bounds.y,
                            gimp_item_get_width    (mask),
                            gimp_item_get_height   (mask));
    }

  if (private->reallocate_projection || size_changed)
    {
      GeglBuffer *buffer;

      /* if the graph is already constructed, set the offset node's
       * coordinates first, so the graph is in the right state when
       * the projection is reallocated, see bug #730550.
       */
      if (private->offset_node)
        gegl_node_set (private->offset_node,
                       "x", (gdouble) -bounds.x,
                       "y", (gdouble) -bounds.y,
                       NULL);

      /* update our offset *before* calling gimp_pickable_get_buffer(), so
       * that if our graph isn't constructed yet, the offset node picks
       * up the right coordinates in gimp_group_layer_get_graph().
       */
      gimp_item_set_offset (item, bounds.x, bounds.y);

      /* update the bounding box before updating the projection, so that it
       * picks up the right size.
       */
      private->bounding_box = bounding_box;

      if (private->reallocate_projection)
        {
          private->reallocate_projection = FALSE;

          gimp_projectable_structure_changed (GIMP_PROJECTABLE (group));
        }
      else
        {
          /* when there's no need to reallocate the projection, we call
           * gimp_projectable_bounds_changed(), rather than structure_chaned(),
           * so that the projection simply copies the old content over to the
           * new buffer with an offset, rather than re-renders the graph.
           */
          gimp_projectable_bounds_changed (GIMP_PROJECTABLE (group),
                                           old_bounds.x, old_bounds.y);
        }

      buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));

      gimp_drawable_set_buffer_full (GIMP_DRAWABLE (group),
                                     FALSE, NULL,
                                     buffer, &bounds,
                                     FALSE /* don't update the drawable, the
                                            * flush() below will take care of
                                            * that.
                                            */);

      gimp_group_layer_flush (group);
    }

  /* resize the mask if not transforming (in which case, GimpLayer takes care
   * of the mask)
   */
  if (resize_mask && ! private->transforming)
    gimp_group_layer_update_mask_size (group);

  /* if we show the mask, invalidate the new mask area */
  if (resize_mask && gimp_layer_get_show_mask (layer))
    {
      gimp_drawable_update (GIMP_DRAWABLE (group),
                            gimp_item_get_offset_x (mask) - bounds.x,
                            gimp_item_get_offset_y (mask) - bounds.y,
                            gimp_item_get_width    (mask),
                            gimp_item_get_height   (mask));
    }
}

static void
gimp_group_layer_update_mask_size (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);
  GimpItem              *item    = GIMP_ITEM (group);
  GimpItem              *mask;
  GeglBuffer            *buffer;
  GeglBuffer            *mask_buffer;
  GeglRectangle          bounds;
  GeglRectangle          mask_bounds;
  GeglRectangle          copy_bounds;
  gboolean               intersect;

  mask = GIMP_ITEM (gimp_layer_get_mask (GIMP_LAYER (group)));

  if (! mask)
    return;

  bounds.x           = gimp_item_get_offset_x (item);
  bounds.y           = gimp_item_get_offset_y (item);
  bounds.width       = gimp_item_get_width    (item);
  bounds.height      = gimp_item_get_height   (item);

  mask_bounds.x      = gimp_item_get_offset_x (mask);
  mask_bounds.y      = gimp_item_get_offset_y (mask);
  mask_bounds.width  = gimp_item_get_width    (mask);
  mask_bounds.height = gimp_item_get_height   (mask);

  if (gegl_rectangle_equal (&bounds, &mask_bounds))
    return;

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, bounds.width, bounds.height),
                            gimp_drawable_get_format (GIMP_DRAWABLE (mask)));

  if (private->suspended_mask_buffer)
    {
      /* copy the suspended mask into the new mask */
      mask_buffer = private->suspended_mask_buffer;
      mask_bounds = private->suspended_mask_bounds;
    }
  else
    {
      /* copy the old mask into the new mask */
      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
    }

  intersect = gimp_rectangle_intersect (bounds.x,
                                        bounds.y,
                                        bounds.width,
                                        bounds.height,
                                        mask_bounds.x,
                                        mask_bounds.y,
                                        mask_bounds.width,
                                        mask_bounds.height,
                                        &copy_bounds.x,
                                        &copy_bounds.y,
                                        &copy_bounds.width,
                                        &copy_bounds.height);

  if (intersect)
    {
      gimp_gegl_buffer_copy (mask_buffer,
                             GEGL_RECTANGLE (copy_bounds.x - mask_bounds.x,
                                             copy_bounds.y - mask_bounds.y,
                                             copy_bounds.width,
                                             copy_bounds.height),
                             GEGL_ABYSS_NONE,
                             buffer,
                             GEGL_RECTANGLE (copy_bounds.x - bounds.x,
                                             copy_bounds.y - bounds.y,
                                             copy_bounds.width,
                                             copy_bounds.height));
    }

  gimp_drawable_set_buffer_full (GIMP_DRAWABLE (mask),
                                 FALSE, NULL,
                                 buffer, &bounds,
                                 TRUE);

  g_object_unref (buffer);
}

static void
gimp_group_layer_update_source_node (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);
  GeglNode              *input;
  GeglNode              *output;

  if (private->source_node == NULL)
    return;

  input  = gegl_node_get_input_proxy  (private->source_node, "input");
  output = gegl_node_get_output_proxy (private->source_node, "output");

  if (private->pass_through)
    {
      gegl_node_connect_to (input,          "output",
                            private->graph, "input");
      gegl_node_connect_to (private->graph, "output",
                            output,         "input");
    }
  else
    {
      gegl_node_disconnect (private->graph, "input");

      gegl_node_connect_to (private->parent_source_node, "output",
                            output,                      "input");
    }
}

static void
gimp_group_layer_update_mode_node (GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);
  GeglNode              *node;
  GeglNode              *input;
  GeglNode              *mode_node;

  node      = gimp_filter_get_node (GIMP_FILTER (group));
  input     = gegl_node_get_input_proxy (node, "input");
  mode_node = gimp_drawable_get_mode_node (GIMP_DRAWABLE (group));

  if (private->pass_through &&
      gimp_layer_get_excludes_backdrop (GIMP_LAYER (group)))
    {
      gegl_node_disconnect (mode_node, "input");
    }
  else
    {
      gegl_node_connect_to (input,     "output",
                            mode_node, "input");
    }
}

static void
gimp_group_layer_stack_update (GimpDrawableStack *stack,
                               gint               x,
                               gint               y,
                               gint               width,
                               gint               height,
                               GimpGroupLayer    *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, gimp_object_get_name (group),
              x, y, width, height);
#endif

  if (! private->direct_update)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  pass to the projection as-is.
       */
      gimp_projectable_invalidate (GIMP_PROJECTABLE (group),
                                   x, y, width, height);

      gimp_group_layer_flush (group);
    }

  if (private->direct_update || private->pass_through)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  transform to layer coordinates when emitting our own update signal.
       */
      gimp_drawable_update (GIMP_DRAWABLE (group),
                            x - gimp_item_get_offset_x (GIMP_ITEM (group)),
                            y - gimp_item_get_offset_y (GIMP_ITEM (group)),
                            width, height);
    }
}

static void
gimp_group_layer_proj_update (GimpProjection *proj,
                              gboolean        now,
                              gint            x,
                              gint            y,
                              gint            width,
                              gint            height,
                              GimpGroupLayer *group)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, gimp_object_get_name (group),
              x, y, width, height);
#endif

  if (! private->pass_through)
    {
      /* TODO: groups can currently have a gegl:transform op attached as a filter
       * when using a transform tool, in which case the updated region needs
       * undergo the same transformation.  more generally, when a drawable has
       * filters they may influence the area affected by drawable updates.
       *
       * this needs to be addressed much more generally at some point, but for now
       * we just resort to updating the entire group when it has a filter (i.e.,
       * when it's being used with a transform tool).  we restrict this to groups,
       * and don't do this more generally in gimp_drawable_update(), because this
       * negatively impacts the performance of the warp tool, which does perform
       * accurate drawable updates while using a filter.
       */
      if (gimp_drawable_has_filters (GIMP_DRAWABLE (group)))
        {
          width  = -1;
          height = -1;
        }

      /*  the projection speaks in image coordinates, transform to layer
       *  coordinates when emitting our own update signal.
       */
      gimp_drawable_update (GIMP_DRAWABLE (group),
                            x - gimp_item_get_offset_x (GIMP_ITEM (group)),
                            y - gimp_item_get_offset_y (GIMP_ITEM (group)),
                            width, height);
    }
}
