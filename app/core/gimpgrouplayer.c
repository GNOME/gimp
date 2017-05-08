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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"

#include "gimpgrouplayer.h"
#include "gimpimage.h"
#include "gimpimage-undo-push.h"
#include "gimplayerstack.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"

#include "gimp-intl.h"


typedef struct _GimpGroupLayerPrivate GimpGroupLayerPrivate;

struct _GimpGroupLayerPrivate
{
  GimpContainer  *children;
  GimpProjection *projection;
  GeglNode       *graph;
  GeglNode       *offset_node;
  gint            suspend_resize;
  gboolean        expanded;

  /*  hackish temp states to make the projection/tiles stuff work  */
  const Babl     *convert_format;
  gboolean        reallocate_projection;
  gint            reallocate_width;
  gint            reallocate_height;
};

#define GET_PRIVATE(item) G_TYPE_INSTANCE_GET_PRIVATE (item, \
                                                       GIMP_TYPE_GROUP_LAYER, \
                                                       GimpGroupLayerPrivate)


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
static void            gimp_group_layer_translate    (GimpItem        *item,
                                                      gint             offset_x,
                                                      gint             offset_y,
                                                      gboolean         push_undo);
static void            gimp_group_layer_scale        (GimpItem        *item,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             new_offset_x,
                                                      gint             new_offset_y,
                                                      GimpInterpolationType  interp_type,
                                                      GimpProgress    *progress);
static void            gimp_group_layer_resize       (GimpItem        *item,
                                                      GimpContext     *context,
                                                      GimpFillType     fill_type,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             offset_x,
                                                      gint             offset_y);
static void            gimp_group_layer_flip         (GimpItem        *item,
                                                      GimpContext     *context,
                                                      GimpOrientationType flip_type,
                                                      gdouble          axis,
                                                      gboolean         clip_result);
static void            gimp_group_layer_rotate       (GimpItem        *item,
                                                      GimpContext     *context,
                                                      GimpRotationType rotate_type,
                                                      gdouble          center_x,
                                                      gdouble          center_y,
                                                      gboolean         clip_result);
static void            gimp_group_layer_transform    (GimpItem        *item,
                                                      GimpContext     *context,
                                                      const GimpMatrix3 *matrix,
                                                      GimpTransformDirection direction,
                                                      GimpInterpolationType  interpolation_type,
                                                      GimpTransformResize clip_result,
                                                      GimpProgress    *progress);

static gint64      gimp_group_layer_estimate_memsize (GimpDrawable      *drawable,
                                                      GimpComponentType  component_type,
                                                      gint               width,
                                                      gint               height);
static void            gimp_group_layer_convert_type (GimpDrawable      *drawable,
                                                      GimpImage         *dest_image,
                                                      const Babl        *new_format,
                                                      GimpColorProfile  *dest_profile,
                                                      GeglDitherMethod   layer_dither_type,
                                                      GeglDitherMethod   mask_dither_type,
                                                      gboolean           push_undo,
                                                      GimpProgress      *progress);

static const Babl    * gimp_group_layer_get_format   (GimpProjectable *projectable);
static GeglNode      * gimp_group_layer_get_graph    (GimpProjectable *projectable);
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

static void            gimp_group_layer_update       (GimpGroupLayer  *group);
static void            gimp_group_layer_update_size  (GimpGroupLayer  *group);

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
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PROJECTABLE,
                                                gimp_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_PICKABLE,
                                                gimp_pickable_iface_init))


#define parent_class gimp_group_layer_parent_class


static void
gimp_group_layer_class_init (GimpGroupLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  object_class->set_property        = gimp_group_layer_set_property;
  object_class->get_property        = gimp_group_layer_get_property;
  object_class->finalize            = gimp_group_layer_finalize;

  gimp_object_class->get_memsize    = gimp_group_layer_get_memsize;

  viewable_class->default_icon_name = "gimp-group-layer";
  viewable_class->get_size          = gimp_group_layer_get_size;
  viewable_class->get_children      = gimp_group_layer_get_children;
  viewable_class->set_expanded      = gimp_group_layer_set_expanded;
  viewable_class->get_expanded      = gimp_group_layer_get_expanded;

  item_class->is_position_locked    = gimp_group_layer_is_position_locked;
  item_class->duplicate             = gimp_group_layer_duplicate;
  item_class->convert               = gimp_group_layer_convert;
  item_class->translate             = gimp_group_layer_translate;
  item_class->scale                 = gimp_group_layer_scale;
  item_class->resize                = gimp_group_layer_resize;
  item_class->flip                  = gimp_group_layer_flip;
  item_class->rotate                = gimp_group_layer_rotate;
  item_class->transform             = gimp_group_layer_transform;

  item_class->default_name          = _("Layer Group");
  item_class->rename_desc           = C_("undo-type", "Rename Layer Group");
  item_class->translate_desc        = C_("undo-type", "Move Layer Group");
  item_class->scale_desc            = C_("undo-type", "Scale Layer Group");
  item_class->resize_desc           = C_("undo-type", "Resize Layer Group");
  item_class->flip_desc             = C_("undo-type", "Flip Layer Group");
  item_class->rotate_desc           = C_("undo-type", "Rotate Layer Group");
  item_class->transform_desc        = C_("undo-type", "Transform Layer Group");

  drawable_class->estimate_memsize  = gimp_group_layer_estimate_memsize;
  drawable_class->convert_type      = gimp_group_layer_convert_type;

  g_type_class_add_private (klass, sizeof (GimpGroupLayerPrivate));
}

static void
gimp_projectable_iface_init (GimpProjectableInterface *iface)
{
  iface->get_image          = (GimpImage * (*) (GimpProjectable *)) gimp_item_get_image;
  iface->get_format         = gimp_group_layer_get_format;
  iface->get_offset         = (void (*) (GimpProjectable*, gint*, gint*)) gimp_item_get_offset;
  iface->get_size           = (void (*) (GimpProjectable*, gint*, gint*)) gimp_viewable_get_size;
  iface->get_graph          = gimp_group_layer_get_graph;
  iface->invalidate_preview = (void (*) (GimpProjectable*)) gimp_viewable_invalidate_preview;
}

static void
gimp_pickable_iface_init (GimpPickableInterface *iface)
{
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

  g_signal_connect (private->children, "update",
                    G_CALLBACK (gimp_group_layer_stack_update),
                    group);

  private->projection = gimp_projection_new (GIMP_PROJECTABLE (group));

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

      g_object_unref (private->children);
      private->children = NULL;
    }

  if (private->projection)
    {
      g_object_unref (private->projection);
      private->projection = NULL;
    }

  if (private->graph)
    {
      g_object_unref (private->graph);
      private->graph = NULL;
    }

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

static gboolean
gimp_group_layer_get_size (GimpViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  GimpGroupLayerPrivate *private = GET_PRIVATE (viewable);

  if (private->reallocate_width  != 0 &&
      private->reallocate_height != 0)
    {
      *width  = private->reallocate_width;
      *height = private->reallocate_height;

      return TRUE;
    }

  /*  return the size only if there are children...  */
  if (gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children)))
    return GIMP_VIEWABLE_CLASS (parent_class)->get_size (viewable,
                                                         width, height);

  /*  ...otherwise return "no content"  */
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
  GimpGroupLayer *group = GIMP_GROUP_LAYER (viewable);

  GET_PRIVATE (group)->expanded = expanded;
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
gimp_group_layer_translate (GimpItem *item,
                            gint      offset_x,
                            gint      offset_y,
                            gboolean  push_undo)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GimpLayerMask         *mask;
  GList                 *list;

  /*  don't push an undo here because undo will call us again  */
  gimp_group_layer_suspend_resize (group, FALSE);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      gimp_item_translate (child, offset_x, offset_y, push_undo);
    }

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    {
      gint off_x, off_y;

      gimp_item_get_offset (item, &off_x, &off_y);
      gimp_item_set_offset (GIMP_ITEM (mask), off_x, off_y);

      gimp_viewable_invalidate_preview (GIMP_VIEWABLE (mask));
    }

  /*  don't push an undo here because undo will call us again  */
  gimp_group_layer_resume_resize (group, FALSE);
}

static void
gimp_group_layer_scale (GimpItem              *item,
                        gint                   new_width,
                        gint                   new_height,
                        gint                   new_offset_x,
                        gint                   new_offset_y,
                        GimpInterpolationType  interpolation_type,
                        GimpProgress          *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GimpLayerMask         *mask;
  GList                 *list;
  gdouble                width_factor;
  gdouble                height_factor;
  gint                   old_offset_x;
  gint                   old_offset_y;

  width_factor  = (gdouble) new_width  / (gdouble) gimp_item_get_width  (item);
  height_factor = (gdouble) new_height / (gdouble) gimp_item_get_height (item);

  old_offset_x = gimp_item_get_offset_x (item);
  old_offset_y = gimp_item_get_offset_y (item);

  gimp_group_layer_suspend_resize (group, TRUE);

  list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));

  while (list)
    {
      GimpItem *child = list->data;
      gint      child_width;
      gint      child_height;
      gint      child_offset_x;
      gint      child_offset_y;

      list = g_list_next (list);

      child_width    = ROUND (width_factor  * gimp_item_get_width  (child));
      child_height   = ROUND (height_factor * gimp_item_get_height (child));
      child_offset_x = ROUND (width_factor  * (gimp_item_get_offset_x (child) -
                                               old_offset_x));
      child_offset_y = ROUND (height_factor * (gimp_item_get_offset_y (child) -
                                               old_offset_y));

      child_offset_x += new_offset_x;
      child_offset_y += new_offset_y;

      if (child_width > 0 && child_height > 0)
        {
          gimp_item_scale (child,
                           child_width, child_height,
                           child_offset_x, child_offset_y,
                           interpolation_type, progress);
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

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    gimp_item_scale (GIMP_ITEM (mask),
                     new_width, new_height,
                     new_offset_x, new_offset_y,
                     interpolation_type, progress);

  gimp_group_layer_resume_resize (group, TRUE);
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
  GimpLayerMask         *mask;
  GList                 *list;
  gint                   x, y;

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

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    gimp_item_resize (GIMP_ITEM (mask), context, GIMP_FILL_TRANSPARENT,
                      new_width, new_height, offset_x, offset_y);

  gimp_group_layer_resume_resize (group, TRUE);
}

static void
gimp_group_layer_flip (GimpItem            *item,
                       GimpContext         *context,
                       GimpOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GimpLayerMask         *mask;
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

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    gimp_item_flip (GIMP_ITEM (mask), context,
                    flip_type, axis, clip_result);

  gimp_group_layer_resume_resize (group, TRUE);
}

static void
gimp_group_layer_rotate (GimpItem         *item,
                         GimpContext      *context,
                         GimpRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GimpLayerMask         *mask;
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

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    gimp_item_rotate (GIMP_ITEM (mask), context,
                      rotate_type, center_x, center_y, clip_result);

  gimp_group_layer_resume_resize (group, TRUE);
}

static void
gimp_group_layer_transform (GimpItem               *item,
                            GimpContext            *context,
                            const GimpMatrix3      *matrix,
                            GimpTransformDirection  direction,
                            GimpInterpolationType   interpolation_type,
                            GimpTransformResize     clip_result,
                            GimpProgress           *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (item);
  GimpGroupLayerPrivate *private = GET_PRIVATE (item);
  GimpLayerMask         *mask;
  GList                 *list;

  gimp_group_layer_suspend_resize (group, TRUE);

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;

      gimp_item_transform (child, context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask)
    gimp_item_transform (GIMP_ITEM (mask), context,
                         matrix, direction,
                         interpolation_type,
                         clip_result, progress);

  gimp_group_layer_resume_resize (group, TRUE);
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

  g_assert_not_reached ();

  return NULL;
}

static void
gimp_group_layer_convert_type (GimpDrawable     *drawable,
                               GimpImage        *dest_image,
                               const Babl       *new_format,
                               GimpColorProfile *dest_profile,
                               GeglDitherMethod  layer_dither_type,
                               GeglDitherMethod  mask_dither_type,
                               gboolean          push_undo,
                               GimpProgress     *progress)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (drawable);
  GimpGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GimpLayerMask         *mask;
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
    get_projection_format (GIMP_PROJECTABLE (drawable),
                           gimp_babl_format_get_base_type (new_format),
                           gimp_babl_format_get_precision (new_format));
  gimp_projectable_structure_changed (GIMP_PROJECTABLE (drawable));
  gimp_pickable_flush (GIMP_PICKABLE (private->projection));

  buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));

  gimp_drawable_set_buffer_full (drawable,
                                 FALSE, NULL,
                                 buffer,
                                 gimp_item_get_offset_x (GIMP_ITEM (drawable)),
                                 gimp_item_get_offset_y (GIMP_ITEM (drawable)));

  /*  reset, the actual format is right now  */
  private->convert_format = NULL;

  mask = gimp_layer_get_mask (GIMP_LAYER (group));

  if (mask &&
      gimp_babl_format_get_precision (new_format) !=
      gimp_drawable_get_precision (GIMP_DRAWABLE (mask)))
    {
      gimp_drawable_convert_type (GIMP_DRAWABLE (mask), dest_image,
                                  GIMP_GRAY,
                                  gimp_babl_format_get_precision (new_format),
                                  gimp_drawable_has_alpha (GIMP_DRAWABLE (mask)),
                                  NULL,
                                  layer_dither_type, mask_dither_type,
                                  push_undo, progress);
    }
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

static GeglNode *
gimp_group_layer_get_graph (GimpProjectable *projectable)
{
  GimpGroupLayer        *group   = GIMP_GROUP_LAYER (projectable);
  GimpGroupLayerPrivate *private = GET_PRIVATE (projectable);
  GeglNode              *layers_node;
  GeglNode              *output;
  gint                   off_x;
  gint                   off_y;

  if (private->graph)
    return private->graph;

  private->graph = gegl_node_new ();

  layers_node =
    gimp_filter_stack_get_graph (GIMP_FILTER_STACK (private->children));

  gegl_node_add_child (private->graph, layers_node);

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
    gimp_image_undo_push_group_layer_suspend (gimp_item_get_image (item),
                                              NULL, group);

  GET_PRIVATE (group)->suspend_resize++;
}

void
gimp_group_layer_resume_resize (GimpGroupLayer *group,
                                gboolean        push_undo)
{
  GimpGroupLayerPrivate *private;
  GimpItem              *item;

  g_return_if_fail (GIMP_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_resize > 0);

  item = GIMP_ITEM (group);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_group_layer_resume (gimp_item_get_image (item),
                                             NULL, group);

  private->suspend_resize--;

  if (private->suspend_resize == 0)
    {
      gimp_group_layer_update_size (group);
    }
}


/*  private functions  */

static void
gimp_group_layer_child_add (GimpContainer  *container,
                            GimpLayer      *child,
                            GimpGroupLayer *group)
{
  gimp_group_layer_update (group);
}

static void
gimp_group_layer_child_remove (GimpContainer  *container,
                               GimpLayer      *child,
                               GimpGroupLayer *group)
{
  gimp_group_layer_update (group);
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
  GimpGroupLayerPrivate *private    = GET_PRIVATE (group);
  GimpItem              *item       = GIMP_ITEM (group);
  gint                   old_x      = gimp_item_get_offset_x (item);
  gint                   old_y      = gimp_item_get_offset_y (item);
  gint                   old_width  = gimp_item_get_width  (item);
  gint                   old_height = gimp_item_get_height (item);
  gint                   x          = 0;
  gint                   y          = 0;
  gint                   width      = 1;
  gint                   height     = 1;
  gboolean               first      = TRUE;
  GList                 *list;

  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = list->data;
      gint      child_width;
      gint      child_height;

      if (! gimp_viewable_get_size (GIMP_VIEWABLE (child),
                                    &child_width, &child_height))
        {
          /*  ignore children without content (empty group layers);
           *  see bug 777017
           */
          continue;
        }

      if (first)
        {
          x      = gimp_item_get_offset_x (child);
          y      = gimp_item_get_offset_y (child);
          width  = child_width;
          height = child_height;

          first = FALSE;
        }
      else
        {
          gimp_rectangle_union (x, y, width, height,
                                gimp_item_get_offset_x (child),
                                gimp_item_get_offset_y (child),
                                child_width,
                                child_height,
                                &x, &y, &width, &height);
        }
    }

  if (private->reallocate_projection ||
      x      != old_x                ||
      y      != old_y                ||
      width  != old_width            ||
      height != old_height)
    {
      /* set the offset first, so the graph is in the right state when
       * the projection is reallocated, see bug #730550.
       */
      if (private->offset_node)
        gegl_node_set (private->offset_node,
                       "x", (gdouble) -x,
                       "y", (gdouble) -y,
                       NULL);

      if (private->reallocate_projection ||
          width  != old_width            ||
          height != old_height)
        {
          GeglBuffer *buffer;

          private->reallocate_projection = FALSE;

          /*  temporarily change the return values of gimp_viewable_get_size()
           *  so the projection allocates itself correctly
           */
          private->reallocate_width  = width;
          private->reallocate_height = height;

          gimp_projectable_structure_changed (GIMP_PROJECTABLE (group));
          gimp_pickable_flush (GIMP_PICKABLE (private->projection));

          buffer = gimp_pickable_get_buffer (GIMP_PICKABLE (private->projection));

          gimp_drawable_set_buffer_full (GIMP_DRAWABLE (group),
                                         FALSE, NULL,
                                         buffer,
                                         x, y);

          /*  reset, the actual size is correct now  */
          private->reallocate_width  = 0;
          private->reallocate_height = 0;
        }
      else
        {
          gimp_item_set_offset (item, x, y);

          /*  invalidate the entire projection since the position of
           *  the children relative to each other might have changed
           *  in a way that happens to leave the group's width and
           *  height the same
           */
          gimp_projectable_invalidate (GIMP_PROJECTABLE (group),
                                       x, y, width, height);

          /*  see comment in gimp_group_layer_stack_update() below  */
          gimp_pickable_flush (GIMP_PICKABLE (private->projection));
        }
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
#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, gimp_object_get_name (group),
              x, y, width, height);
#endif

  /*  the layer stack's update signal speaks in image coordinates,
   *  pass to the projection as-is.
   */
  gimp_projectable_invalidate (GIMP_PROJECTABLE (group),
                               x, y, width, height);

  /*  flush the pickable not the projectable because flushing the
   *  pickable will finish all invalidation on the projection so it
   *  can be used as source (note that it will still be constructed
   *  when the actual read happens, so this it not a performance
   *  problem)
   */
  gimp_pickable_flush (GIMP_PICKABLE (GET_PRIVATE (group)->projection));
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
#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, gimp_object_get_name (group),
              x, y, width, height);
#endif

  /*  the projection speaks in image coordinates, transform to layer
   *  coordinates when emitting our own update signal.
   */
  gimp_drawable_update (GIMP_DRAWABLE (group),
                        x - gimp_item_get_offset_x (GIMP_ITEM (group)),
                        y - gimp_item_get_offset_y (GIMP_ITEM (group)),
                        width, height);
}
