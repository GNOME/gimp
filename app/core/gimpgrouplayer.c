/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaGroupLayer
 * Copyright (C) 2009  Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligmadrawable-filters.h"
#include "ligmagrouplayer.h"
#include "ligmagrouplayerundo.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmalayerstack.h"
#include "ligmaobjectqueue.h"
#include "ligmapickable.h"
#include "ligmaprogress.h"
#include "ligmaprojectable.h"
#include "ligmaprojection.h"

#include "ligma-intl.h"


typedef struct _LigmaGroupLayerPrivate LigmaGroupLayerPrivate;

struct _LigmaGroupLayerPrivate
{
  LigmaContainer  *children;
  LigmaProjection *projection;
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

#define GET_PRIVATE(item) ((LigmaGroupLayerPrivate *) ligma_group_layer_get_instance_private ((LigmaGroupLayer *) (item)))


static void            ligma_projectable_iface_init   (LigmaProjectableInterface  *iface);
static void            ligma_pickable_iface_init      (LigmaPickableInterface     *iface);

static void            ligma_group_layer_finalize     (GObject         *object);
static void            ligma_group_layer_set_property (GObject         *object,
                                                      guint            property_id,
                                                      const GValue    *value,
                                                      GParamSpec      *pspec);
static void            ligma_group_layer_get_property (GObject         *object,
                                                      guint            property_id,
                                                      GValue          *value,
                                                      GParamSpec      *pspec);

static gint64          ligma_group_layer_get_memsize  (LigmaObject      *object,
                                                      gint64          *gui_size);

static void        ligma_group_layer_ancestry_changed (LigmaViewable    *viewable);
static gboolean        ligma_group_layer_get_size     (LigmaViewable    *viewable,
                                                      gint            *width,
                                                      gint            *height);
static LigmaContainer * ligma_group_layer_get_children (LigmaViewable    *viewable);
static gboolean        ligma_group_layer_get_expanded (LigmaViewable    *viewable);
static void            ligma_group_layer_set_expanded (LigmaViewable    *viewable,
                                                      gboolean         expanded);

static gboolean  ligma_group_layer_is_position_locked (LigmaItem        *item,
                                                      LigmaItem       **locked_item,
                                                      gboolean         check_children);
static LigmaItem      * ligma_group_layer_duplicate    (LigmaItem        *item,
                                                      GType            new_type);
static void            ligma_group_layer_convert      (LigmaItem        *item,
                                                      LigmaImage       *dest_image,
                                                      GType            old_type);
static void         ligma_group_layer_start_transform (LigmaItem        *item,
                                                      gboolean         push_undo);
static void           ligma_group_layer_end_transform (LigmaItem        *item,
                                                      gboolean         push_undo);
static void            ligma_group_layer_resize       (LigmaItem        *item,
                                                      LigmaContext     *context,
                                                      LigmaFillType     fill_type,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             offset_x,
                                                      gint             offset_y);
static LigmaTransformResize
                       ligma_group_layer_get_clip     (LigmaItem        *item,
                                                      LigmaTransformResize clip_result);

static gint64      ligma_group_layer_estimate_memsize (LigmaDrawable      *drawable,
                                                      LigmaComponentType  component_type,
                                                      gint               width,
                                                      gint               height);
static void            ligma_group_layer_update_all   (LigmaDrawable      *drawable);

static void            ligma_group_layer_translate    (LigmaLayer       *layer,
                                                      gint             offset_x,
                                                      gint             offset_y);
static void            ligma_group_layer_scale        (LigmaLayer       *layer,
                                                      gint             new_width,
                                                      gint             new_height,
                                                      gint             new_offset_x,
                                                      gint             new_offset_y,
                                                      LigmaInterpolationType  interp_type,
                                                      LigmaProgress    *progress);
static void            ligma_group_layer_flip         (LigmaLayer       *layer,
                                                      LigmaContext     *context,
                                                      LigmaOrientationType flip_type,
                                                      gdouble          axis,
                                                      gboolean         clip_result);
static void            ligma_group_layer_rotate       (LigmaLayer       *layer,
                                                      LigmaContext     *context,
                                                      LigmaRotationType rotate_type,
                                                      gdouble          center_x,
                                                      gdouble          center_y,
                                                      gboolean         clip_result);
static void            ligma_group_layer_transform    (LigmaLayer       *layer,
                                                      LigmaContext     *context,
                                                      const LigmaMatrix3 *matrix,
                                                      LigmaTransformDirection direction,
                                                      LigmaInterpolationType  interpolation_type,
                                                      LigmaTransformResize clip_result,
                                                      LigmaProgress      *progress);
static void            ligma_group_layer_convert_type (LigmaLayer         *layer,
                                                      LigmaImage         *dest_image,
                                                      const Babl        *new_format,
                                                      LigmaColorProfile  *src_profile,
                                                      LigmaColorProfile  *dest_profile,
                                                      GeglDitherMethod   layer_dither_type,
                                                      GeglDitherMethod   mask_dither_type,
                                                      gboolean           push_undo,
                                                      LigmaProgress      *progress);
static GeglNode   * ligma_group_layer_get_source_node (LigmaDrawable      *drawable);

static void         ligma_group_layer_opacity_changed (LigmaLayer         *layer);
static void  ligma_group_layer_effective_mode_changed (LigmaLayer         *layer);
static void
          ligma_group_layer_excludes_backdrop_changed (LigmaLayer         *layer);
static GeglRectangle
                   ligma_group_layer_get_bounding_box (LigmaLayer         *layer);
static void      ligma_group_layer_get_effective_mode (LigmaLayer         *layer,
                                                      LigmaLayerMode          *mode,
                                                      LigmaLayerColorSpace    *blend_space,
                                                      LigmaLayerColorSpace    *composite_space,
                                                      LigmaLayerCompositeMode *composite_mode);
static gboolean
              ligma_group_layer_get_excludes_backdrop (LigmaLayer         *layer);

static const Babl    * ligma_group_layer_get_format   (LigmaProjectable *projectable);
static GeglRectangle
       ligma_group_layer_projectable_get_bounding_box (LigmaProjectable *projectable);
static GeglNode      * ligma_group_layer_get_graph    (LigmaProjectable *projectable);
static void            ligma_group_layer_begin_render (LigmaProjectable *projectable);
static void            ligma_group_layer_end_render   (LigmaProjectable *projectable);

static void          ligma_group_layer_pickable_flush (LigmaPickable    *pickable);
static gdouble       ligma_group_layer_get_opacity_at (LigmaPickable    *pickable,
                                                      gint             x,
                                                      gint             y);


static void            ligma_group_layer_child_add    (LigmaContainer   *container,
                                                      LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);
static void            ligma_group_layer_child_remove (LigmaContainer   *container,
                                                      LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);
static void            ligma_group_layer_child_move   (LigmaLayer       *child,
                                                      GParamSpec      *pspec,
                                                      LigmaGroupLayer  *group);
static void            ligma_group_layer_child_resize (LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);
static void    ligma_group_layer_child_active_changed (LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);
static void
       ligma_group_layer_child_effective_mode_changed (LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);
static void
    ligma_group_layer_child_excludes_backdrop_changed (LigmaLayer       *child,
                                                      LigmaGroupLayer  *group);

static void            ligma_group_layer_flush        (LigmaGroupLayer  *group);
static void            ligma_group_layer_update       (LigmaGroupLayer  *group);
static void            ligma_group_layer_update_size  (LigmaGroupLayer  *group);
static void        ligma_group_layer_update_mask_size (LigmaGroupLayer  *group);
static void      ligma_group_layer_update_source_node (LigmaGroupLayer  *group);
static void        ligma_group_layer_update_mode_node (LigmaGroupLayer  *group);

static void            ligma_group_layer_stack_update (LigmaDrawableStack *stack,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      LigmaGroupLayer    *group);
static void            ligma_group_layer_proj_update  (LigmaProjection    *proj,
                                                      gboolean           now,
                                                      gint               x,
                                                      gint               y,
                                                      gint               width,
                                                      gint               height,
                                                      LigmaGroupLayer    *group);


G_DEFINE_TYPE_WITH_CODE (LigmaGroupLayer, ligma_group_layer, LIGMA_TYPE_LAYER,
                         G_ADD_PRIVATE (LigmaGroupLayer)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PROJECTABLE,
                                                ligma_projectable_iface_init)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_PICKABLE,
                                                ligma_pickable_iface_init))


#define parent_class ligma_group_layer_parent_class


/* disable pass-through groups strength-reduction to normal groups.
 * see ligma_group_layer_get_effective_mode().
 */
static gboolean no_pass_through_strength_reduction = FALSE;


static void
ligma_group_layer_class_init (LigmaGroupLayerClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaItemClass     *item_class        = LIGMA_ITEM_CLASS (klass);
  LigmaDrawableClass *drawable_class    = LIGMA_DRAWABLE_CLASS (klass);
  LigmaLayerClass    *layer_class       = LIGMA_LAYER_CLASS (klass);

  object_class->set_property             = ligma_group_layer_set_property;
  object_class->get_property             = ligma_group_layer_get_property;
  object_class->finalize                 = ligma_group_layer_finalize;

  ligma_object_class->get_memsize         = ligma_group_layer_get_memsize;

  viewable_class->default_icon_name      = "ligma-group-layer";
  viewable_class->ancestry_changed       = ligma_group_layer_ancestry_changed;
  viewable_class->get_size               = ligma_group_layer_get_size;
  viewable_class->get_children           = ligma_group_layer_get_children;
  viewable_class->set_expanded           = ligma_group_layer_set_expanded;
  viewable_class->get_expanded           = ligma_group_layer_get_expanded;

  item_class->is_position_locked         = ligma_group_layer_is_position_locked;
  item_class->duplicate                  = ligma_group_layer_duplicate;
  item_class->convert                    = ligma_group_layer_convert;
  item_class->start_transform            = ligma_group_layer_start_transform;
  item_class->end_transform              = ligma_group_layer_end_transform;
  item_class->resize                     = ligma_group_layer_resize;
  item_class->get_clip                   = ligma_group_layer_get_clip;

  item_class->default_name               = _("Layer Group");
  item_class->rename_desc                = C_("undo-type", "Rename Layer Group");
  item_class->translate_desc             = C_("undo-type", "Move Layer Group");
  item_class->scale_desc                 = C_("undo-type", "Scale Layer Group");
  item_class->resize_desc                = C_("undo-type", "Resize Layer Group");
  item_class->flip_desc                  = C_("undo-type", "Flip Layer Group");
  item_class->rotate_desc                = C_("undo-type", "Rotate Layer Group");
  item_class->transform_desc             = C_("undo-type", "Transform Layer Group");

  drawable_class->estimate_memsize       = ligma_group_layer_estimate_memsize;
  drawable_class->update_all             = ligma_group_layer_update_all;
  drawable_class->get_source_node        = ligma_group_layer_get_source_node;

  layer_class->opacity_changed           = ligma_group_layer_opacity_changed;
  layer_class->effective_mode_changed    = ligma_group_layer_effective_mode_changed;
  layer_class->excludes_backdrop_changed = ligma_group_layer_excludes_backdrop_changed;
  layer_class->translate                 = ligma_group_layer_translate;
  layer_class->scale                     = ligma_group_layer_scale;
  layer_class->flip                      = ligma_group_layer_flip;
  layer_class->rotate                    = ligma_group_layer_rotate;
  layer_class->transform                 = ligma_group_layer_transform;
  layer_class->convert_type              = ligma_group_layer_convert_type;
  layer_class->get_bounding_box          = ligma_group_layer_get_bounding_box;
  layer_class->get_effective_mode        = ligma_group_layer_get_effective_mode;
  layer_class->get_excludes_backdrop     = ligma_group_layer_get_excludes_backdrop;

  if (g_getenv ("LIGMA_NO_PASS_THROUGH_STRENGTH_REDUCTION"))
    no_pass_through_strength_reduction = TRUE;
}

static void
ligma_projectable_iface_init (LigmaProjectableInterface *iface)
{
  iface->get_image          = (LigmaImage * (*) (LigmaProjectable *)) ligma_item_get_image;
  iface->get_format         = ligma_group_layer_get_format;
  iface->get_offset         = (void (*) (LigmaProjectable*, gint*, gint*)) ligma_item_get_offset;
  iface->get_bounding_box   = ligma_group_layer_projectable_get_bounding_box;
  iface->get_graph          = ligma_group_layer_get_graph;
  iface->begin_render       = ligma_group_layer_begin_render;
  iface->end_render         = ligma_group_layer_end_render;
  iface->invalidate_preview = (void (*) (LigmaProjectable*)) ligma_viewable_invalidate_preview;
}

static void
ligma_pickable_iface_init (LigmaPickableInterface *iface)
{
  iface->flush          = ligma_group_layer_pickable_flush;
  iface->get_opacity_at = ligma_group_layer_get_opacity_at;
}

static void
ligma_group_layer_init (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);

  private->children = ligma_layer_stack_new (LIGMA_TYPE_LAYER);
  private->expanded = TRUE;

  g_signal_connect (private->children, "add",
                    G_CALLBACK (ligma_group_layer_child_add),
                    group);
  g_signal_connect (private->children, "remove",
                    G_CALLBACK (ligma_group_layer_child_remove),
                    group);

  ligma_container_add_handler (private->children, "notify::offset-x",
                              G_CALLBACK (ligma_group_layer_child_move),
                              group);
  ligma_container_add_handler (private->children, "notify::offset-y",
                              G_CALLBACK (ligma_group_layer_child_move),
                              group);
  ligma_container_add_handler (private->children, "size-changed",
                              G_CALLBACK (ligma_group_layer_child_resize),
                              group);
  ligma_container_add_handler (private->children, "bounding-box-changed",
                              G_CALLBACK (ligma_group_layer_child_resize),
                              group);
  ligma_container_add_handler (private->children, "active-changed",
                              G_CALLBACK (ligma_group_layer_child_active_changed),
                              group);
  ligma_container_add_handler (private->children, "effective-mode-changed",
                              G_CALLBACK (ligma_group_layer_child_effective_mode_changed),
                              group);
  ligma_container_add_handler (private->children, "excludes-backdrop-changed",
                              G_CALLBACK (ligma_group_layer_child_excludes_backdrop_changed),
                              group);

  g_signal_connect (private->children, "update",
                    G_CALLBACK (ligma_group_layer_stack_update),
                    group);

  private->projection = ligma_projection_new (LIGMA_PROJECTABLE (group));
  ligma_projection_set_priority (private->projection, 1);

  g_signal_connect (private->projection, "update",
                    G_CALLBACK (ligma_group_layer_proj_update),
                    group);
}

static void
ligma_group_layer_finalize (GObject *object)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (object);

  if (private->children)
    {
      g_signal_handlers_disconnect_by_func (private->children,
                                            ligma_group_layer_child_add,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            ligma_group_layer_child_remove,
                                            object);
      g_signal_handlers_disconnect_by_func (private->children,
                                            ligma_group_layer_stack_update,
                                            object);

      /* this is particularly important to avoid reallocating the projection
       * in response to a "bounding-box-changed" signal, which can be emitted
       * during layer destruction.  see issue #4584.
       */
      ligma_container_remove_handlers_by_data (private->children, object);

      g_clear_object (&private->children);
    }

  g_clear_object (&private->projection);
  g_clear_object (&private->source_node);
  g_clear_object (&private->graph);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_group_layer_set_property (GObject      *object,
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
ligma_group_layer_get_property (GObject    *object,
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
ligma_group_layer_get_memsize (LigmaObject *object,
                              gint64     *gui_size)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (object);
  gint64                 memsize = 0;

  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->children), gui_size);
  memsize += ligma_object_get_memsize (LIGMA_OBJECT (private->projection), gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_group_layer_ancestry_changed (LigmaViewable *viewable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (viewable);

  ligma_projection_set_priority (private->projection,
                                ligma_viewable_get_depth (viewable) + 1);

  LIGMA_VIEWABLE_CLASS (parent_class)->ancestry_changed (viewable);
}

static gboolean
ligma_group_layer_get_size (LigmaViewable *viewable,
                           gint         *width,
                           gint         *height)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (viewable);

  /*  return the size only if there are children ...  */
  if (! ligma_container_is_empty (private->children))
    {
      return LIGMA_VIEWABLE_CLASS (parent_class)->get_size (viewable,
                                                           width, height);
    }

  /*  ... otherwise, return "no content"  */
  return FALSE;
}

static LigmaContainer *
ligma_group_layer_get_children (LigmaViewable *viewable)
{
  return GET_PRIVATE (viewable)->children;
}

static gboolean
ligma_group_layer_get_expanded (LigmaViewable *viewable)
{
  LigmaGroupLayer *group = LIGMA_GROUP_LAYER (viewable);

  return GET_PRIVATE (group)->expanded;
}

static void
ligma_group_layer_set_expanded (LigmaViewable *viewable,
                               gboolean      expanded)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (viewable);

  if (private->expanded != expanded)
    {
      private->expanded = expanded;

      ligma_viewable_expanded_changed (viewable);
    }
}

static gboolean
ligma_group_layer_is_position_locked (LigmaItem  *item,
                                     LigmaItem **locked_item,
                                     gboolean   check_children)
{
  /* Lock position is particular because a locked child locks the group
   * too.
   */
  if (check_children)
    {
      LigmaGroupLayerPrivate *private = GET_PRIVATE (item);
      GList                 *list;

      for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          LigmaItem *child = list->data;

          if (ligma_item_get_lock_position (child))
            {
              if (locked_item)
                *locked_item = child;

              return TRUE;
            }
          else if (LIGMA_IS_GROUP_LAYER (child) &&
                   ligma_group_layer_is_position_locked (child,
                                                        locked_item,
                                                        TRUE))
            {
              return TRUE;
            }
        }
    }

  /* And a locked parent locks the group too! Which is handled by parent
   * implementation of the method.
   */
  return LIGMA_ITEM_CLASS (parent_class)->is_position_locked (item,
                                                             locked_item,
                                                             FALSE);
}

static LigmaItem *
ligma_group_layer_duplicate (LigmaItem *item,
                            GType     new_type)
{
  LigmaItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, LIGMA_TYPE_DRAWABLE), NULL);

  new_item = LIGMA_ITEM_CLASS (parent_class)->duplicate (item, new_type);

  if (LIGMA_IS_GROUP_LAYER (new_item))
    {
      LigmaGroupLayerPrivate *private     = GET_PRIVATE (item);
      LigmaGroupLayer        *new_group   = LIGMA_GROUP_LAYER (new_item);
      LigmaGroupLayerPrivate *new_private = GET_PRIVATE (new_item);
      gint                   position    = 0;
      GList                 *list;

      ligma_group_layer_suspend_resize (new_group, FALSE);

      for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          LigmaItem      *child = list->data;
          LigmaItem      *new_child;
          LigmaLayerMask *mask;

          new_child = ligma_item_duplicate (child, G_TYPE_FROM_INSTANCE (child));

          ligma_object_set_name (LIGMA_OBJECT (new_child),
                                ligma_object_get_name (child));

          mask = ligma_layer_get_mask (LIGMA_LAYER (child));

          if (mask)
            {
              LigmaLayerMask *new_mask;

              new_mask = ligma_layer_get_mask (LIGMA_LAYER (new_child));

              ligma_object_set_name (LIGMA_OBJECT (new_mask),
                                    ligma_object_get_name (mask));
            }

          ligma_viewable_set_parent (LIGMA_VIEWABLE (new_child),
                                    LIGMA_VIEWABLE (new_group));

          ligma_container_insert (new_private->children,
                                 LIGMA_OBJECT (new_child),
                                 position++);
        }

      /*  force the projection to reallocate itself  */
      GET_PRIVATE (new_group)->reallocate_projection = TRUE;

      ligma_group_layer_resume_resize (new_group, FALSE);
    }

  return new_item;
}

static void
ligma_group_layer_convert (LigmaItem  *item,
                          LigmaImage *dest_image,
                          GType      old_type)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem *child = list->data;

      LIGMA_ITEM_GET_CLASS (child)->convert (child, dest_image,
                                            G_TYPE_FROM_INSTANCE (child));
    }

  LIGMA_ITEM_CLASS (parent_class)->convert (item, dest_image, old_type);
}

static void
ligma_group_layer_start_transform (LigmaItem *item,
                             gboolean  push_undo)
{
  _ligma_group_layer_start_transform (LIGMA_GROUP_LAYER (item), push_undo);

  if (LIGMA_ITEM_CLASS (parent_class)->start_transform)
    LIGMA_ITEM_CLASS (parent_class)->start_transform (item, push_undo);
}

static void
ligma_group_layer_end_transform (LigmaItem *item,
                           gboolean  push_undo)
{
  if (LIGMA_ITEM_CLASS (parent_class)->end_transform)
    LIGMA_ITEM_CLASS (parent_class)->end_transform (item, push_undo);

  _ligma_group_layer_end_transform (LIGMA_GROUP_LAYER (item), push_undo);
}

static void
ligma_group_layer_resize (LigmaItem     *item,
                         LigmaContext  *context,
                         LigmaFillType  fill_type,
                         gint          new_width,
                         gint          new_height,
                         gint          offset_x,
                         gint          offset_y)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (item);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (item);
  GList                 *list;
  gint                   x, y;

  /* we implement LigmaItem::resize(), instead of LigmaLayer::resize(), so that
   * LigmaLayer doesn't resize the mask.  note that ligma_item_resize() calls
   * ligma_item_{start,end}_move(), and not ligma_item_{start,end}_transform(),
   * so that mask resizing is handled by ligma_group_layer_update_size().
   */

  x = ligma_item_get_offset_x (item) - offset_x;
  y = ligma_item_get_offset_y (item) - offset_y;

  ligma_group_layer_suspend_resize (group, TRUE);

  list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));

  while (list)
    {
      LigmaItem *child = list->data;
      gint      child_width;
      gint      child_height;
      gint      child_x;
      gint      child_y;

      list = g_list_next (list);

      if (ligma_rectangle_intersect (x,
                                    y,
                                    new_width,
                                    new_height,
                                    ligma_item_get_offset_x (child),
                                    ligma_item_get_offset_y (child),
                                    ligma_item_get_width  (child),
                                    ligma_item_get_height (child),
                                    &child_x,
                                    &child_y,
                                    &child_width,
                                    &child_height))
        {
          gint child_offset_x = ligma_item_get_offset_x (child) - child_x;
          gint child_offset_y = ligma_item_get_offset_y (child) - child_y;

          ligma_item_resize (child, context, fill_type,
                            child_width, child_height,
                            child_offset_x, child_offset_y);
        }
      else if (ligma_item_is_attached (item))
        {
          ligma_image_remove_layer (ligma_item_get_image (item),
                                   LIGMA_LAYER (child),
                                   TRUE, NULL);
        }
      else
        {
          ligma_container_remove (private->children, LIGMA_OBJECT (child));
        }
    }

  ligma_group_layer_resume_resize (group, TRUE);
}

static LigmaTransformResize
ligma_group_layer_get_clip (LigmaItem            *item,
                           LigmaTransformResize  clip_result)
{
  /* TODO: add clipping support, by clipping all sublayers as a unit, instead
   * of individually.
   */
  return LIGMA_TRANSFORM_RESIZE_ADJUST;
}

static gint64
ligma_group_layer_estimate_memsize (LigmaDrawable      *drawable,
                                   LigmaComponentType  component_type,
                                   gint               width,
                                   gint               height)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;
  LigmaImageBaseType      base_type;
  gint64                 memsize = 0;

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaDrawable *child = list->data;
      gint          child_width;
      gint          child_height;

      child_width  = (ligma_item_get_width (LIGMA_ITEM (child)) *
                      width /
                      ligma_item_get_width (LIGMA_ITEM (drawable)));
      child_height = (ligma_item_get_height (LIGMA_ITEM (child)) *
                      height /
                      ligma_item_get_height (LIGMA_ITEM (drawable)));

      memsize += ligma_drawable_estimate_memsize (child,
                                                 component_type,
                                                 child_width,
                                                 child_height);
    }

  base_type = ligma_drawable_get_base_type (drawable);

  memsize += ligma_projection_estimate_memsize (base_type, component_type,
                                               width, height);

  return memsize +
         LIGMA_DRAWABLE_CLASS (parent_class)->estimate_memsize (drawable,
                                                               component_type,
                                                               width, height);
}

static void
ligma_group_layer_update_all (LigmaDrawable *drawable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GList                 *list;

  /*  redirect stack updates to the drawable, rather than to the projection  */
  private->direct_update++;

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaFilter *child = list->data;

      if (ligma_filter_get_active (child))
        ligma_drawable_update_all (LIGMA_DRAWABLE (child));
    }

  /*  redirect stack updates back to the projection  */
  private->direct_update--;
}

static void
ligma_group_layer_translate (LigmaLayer *layer,
                            gint       offset_x,
                            gint       offset_y)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  gint                   x, y;
  GList                 *list;

  /*  don't use ligma_group_layer_suspend_resize(), but rather increment
   *  private->suspend_resize directly, since we're translating the group layer
   *  here, rather than relying on ligma_group_layer_update_size() to do it.
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
  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem *child = list->data;

      /*  don't push an undo here because undo will call us again  */
      ligma_item_translate (child, offset_x, offset_y, FALSE);
    }

  ligma_item_get_offset (LIGMA_ITEM (group), &x, &y);

  x += offset_x;
  y += offset_y;

  /*  update the offset node  */
  if (private->offset_node)
    gegl_node_set (private->offset_node,
                   "x", (gdouble) -x,
                   "y", (gdouble) -y,
                   NULL);

  /*  invalidate the selection boundary because of a layer modification  */
  ligma_drawable_invalidate_boundary (LIGMA_DRAWABLE (layer));

  /*  update the group layer offset  */
  ligma_item_set_offset (LIGMA_ITEM (group), x, y);

  /*  redirect stack updates back to the projection  */
  private->direct_update--;

  /*  don't use ligma_group_layer_resume_resize(), but rather decrement
   *  private->suspend_resize directly, so that ligma_group_layer_update_size()
   *  isn't called.
   */
  private->suspend_resize--;
}

static void
ligma_group_layer_scale (LigmaLayer             *layer,
                        gint                   new_width,
                        gint                   new_height,
                        gint                   new_offset_x,
                        gint                   new_offset_y,
                        LigmaInterpolationType  interpolation_type,
                        LigmaProgress          *progress)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  LigmaItem              *item    = LIGMA_ITEM (layer);
  LigmaObjectQueue       *queue   = NULL;
  GList                 *list;
  gdouble                width_factor;
  gdouble                height_factor;
  gint                   old_offset_x;
  gint                   old_offset_y;

  width_factor  = (gdouble) new_width  / (gdouble) ligma_item_get_width  (item);
  height_factor = (gdouble) new_height / (gdouble) ligma_item_get_height (item);

  old_offset_x = ligma_item_get_offset_x (item);
  old_offset_y = ligma_item_get_offset_y (item);

  if (progress)
    {
      queue    = ligma_object_queue_new (progress);
      progress = LIGMA_PROGRESS (queue);

      ligma_object_queue_push_container (queue, private->children);
    }

  ligma_group_layer_suspend_resize (group, TRUE);

  list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));

  while (list)
    {
      LigmaItem *child = list->data;

      list = g_list_next (list);

      if (queue)
        ligma_object_queue_pop (queue);

      if (! ligma_item_scale_by_factors_with_origin (child,
                                                    width_factor, height_factor,
                                                    old_offset_x, old_offset_y,
                                                    new_offset_x, new_offset_y,
                                                    interpolation_type,
                                                    progress))
        {
          /* new width or height are 0; remove item */
          if (ligma_item_is_attached (item))
            {
              ligma_image_remove_layer (ligma_item_get_image (item),
                                       LIGMA_LAYER (child),
                                       TRUE, NULL);
            }
          else
            {
              ligma_container_remove (private->children, LIGMA_OBJECT (child));
            }
        }
    }

  ligma_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static void
ligma_group_layer_flip (LigmaLayer           *layer,
                       LigmaContext         *context,
                       LigmaOrientationType  flip_type,
                       gdouble              axis,
                       gboolean             clip_result)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  ligma_group_layer_suspend_resize (group, TRUE);

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem *child = list->data;

      ligma_item_flip (child, context,
                      flip_type, axis, clip_result);
    }

  ligma_group_layer_resume_resize (group, TRUE);
}

static void
ligma_group_layer_rotate (LigmaLayer        *layer,
                         LigmaContext      *context,
                         LigmaRotationType  rotate_type,
                         gdouble           center_x,
                         gdouble           center_y,
                         gboolean          clip_result)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  GList                 *list;

  ligma_group_layer_suspend_resize (group, TRUE);

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem *child = list->data;

      ligma_item_rotate (child, context,
                        rotate_type, center_x, center_y, clip_result);
    }

  ligma_group_layer_resume_resize (group, TRUE);
}

static void
ligma_group_layer_transform (LigmaLayer              *layer,
                            LigmaContext            *context,
                            const LigmaMatrix3      *matrix,
                            LigmaTransformDirection  direction,
                            LigmaInterpolationType   interpolation_type,
                            LigmaTransformResize     clip_result,
                            LigmaProgress           *progress)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  LigmaObjectQueue       *queue   = NULL;
  GList                 *list;

  if (progress)
    {
      queue    = ligma_object_queue_new (progress);
      progress = LIGMA_PROGRESS (queue);

      ligma_object_queue_push_container (queue, private->children);
    }

  ligma_group_layer_suspend_resize (group, TRUE);

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem *child = list->data;

      if (queue)
        ligma_object_queue_pop (queue);

      ligma_item_transform (child, context,
                           matrix, direction,
                           interpolation_type,
                           clip_result, progress);
    }

  ligma_group_layer_resume_resize (group, TRUE);

  g_clear_object (&queue);
}

static const Babl *
get_projection_format (LigmaProjectable   *projectable,
                       LigmaImageBaseType  base_type,
                       LigmaPrecision      precision)
{
  LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (projectable));

  switch (base_type)
    {
    case LIGMA_RGB:
    case LIGMA_INDEXED:
      return ligma_image_get_format (image, LIGMA_RGB, precision, TRUE,
                                    ligma_image_get_layer_space (image));

    case LIGMA_GRAY:
      return ligma_image_get_format (image, LIGMA_GRAY, precision, TRUE,
                                    ligma_image_get_layer_space (image));
    }

  g_return_val_if_reached (NULL);
}

static void
ligma_group_layer_convert_type (LigmaLayer        *layer,
                               LigmaImage        *dest_image,
                               const Babl       *new_format,
                               LigmaColorProfile *src_profile,
                               LigmaColorProfile *dest_profile,
                               GeglDitherMethod  layer_dither_type,
                               GeglDitherMethod  mask_dither_type,
                               gboolean          push_undo,
                               LigmaProgress     *progress)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);
  GeglBuffer            *buffer;

  if (push_undo)
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (group));

      ligma_image_undo_push_group_layer_convert (image, NULL, group);
    }

  /*  Need to temporarily set the projectable's format to the new
   *  values so the projection will create its tiles with the right
   *  depth
   */
  private->convert_format =
    get_projection_format (LIGMA_PROJECTABLE (group),
                           ligma_babl_format_get_base_type (new_format),
                           ligma_babl_format_get_precision (new_format));
  ligma_projectable_structure_changed (LIGMA_PROJECTABLE (group));
  ligma_group_layer_flush (group);

  buffer = ligma_pickable_get_buffer (LIGMA_PICKABLE (private->projection));

  ligma_drawable_set_buffer_full (LIGMA_DRAWABLE (group),
                                 FALSE, NULL,
                                 buffer, NULL,
                                 TRUE);

  /*  reset, the actual format is right now  */
  private->convert_format = NULL;
}

static GeglNode *
ligma_group_layer_get_source_node (LigmaDrawable *drawable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (drawable);
  GeglNode              *input;

  g_warn_if_fail (private->source_node == NULL);

  private->source_node = gegl_node_new ();

  input = gegl_node_get_input_proxy (private->source_node, "input");

  private->parent_source_node =
    LIGMA_DRAWABLE_CLASS (parent_class)->get_source_node (drawable);

  gegl_node_add_child (private->source_node, private->parent_source_node);

  g_object_unref (private->parent_source_node);

  if (gegl_node_has_pad (private->parent_source_node, "input"))
    {
      gegl_node_connect_to (input,                       "output",
                            private->parent_source_node, "input");
    }

  /* make sure we have a graph */
  (void) ligma_group_layer_get_graph (LIGMA_PROJECTABLE (drawable));

  gegl_node_add_child (private->source_node, private->graph);

  ligma_group_layer_update_source_node (LIGMA_GROUP_LAYER (drawable));

  return g_object_ref (private->source_node);
}

static void
ligma_group_layer_opacity_changed (LigmaLayer *layer)
{
  ligma_layer_update_effective_mode (layer);

  if (LIGMA_LAYER_CLASS (parent_class)->opacity_changed)
    LIGMA_LAYER_CLASS (parent_class)->opacity_changed (layer);
}

static void
ligma_group_layer_effective_mode_changed (LigmaLayer *layer)
{
  LigmaGroupLayer        *group               = LIGMA_GROUP_LAYER (layer);
  LigmaGroupLayerPrivate *private             = GET_PRIVATE (layer);
  LigmaLayerMode          mode;
  gboolean               pass_through;
  gboolean               update_bounding_box = FALSE;

  ligma_layer_get_effective_mode (layer, &mode, NULL, NULL, NULL);

  pass_through = (mode == LIGMA_LAYER_MODE_PASS_THROUGH);

  if (pass_through != private->pass_through)
    {
      if (private->pass_through && ! pass_through)
        {
          /* when switching from pass-through mode to a non-pass-through mode,
           * flush the pickable in order to make sure the projection's buffer
           * gets properly invalidated synchronously, so that it can be used
           * as a source for the rest of the composition.
           */
          ligma_pickable_flush (LIGMA_PICKABLE (private->projection));
        }

      private->pass_through = pass_through;

      update_bounding_box = TRUE;
    }

  ligma_group_layer_update_source_node (group);
  ligma_group_layer_update_mode_node (group);

  if (update_bounding_box)
    ligma_drawable_update_bounding_box (LIGMA_DRAWABLE (group));

  if (LIGMA_LAYER_CLASS (parent_class)->effective_mode_changed)
    LIGMA_LAYER_CLASS (parent_class)->effective_mode_changed (layer);
}

static void
ligma_group_layer_excludes_backdrop_changed (LigmaLayer *layer)
{
  LigmaGroupLayer *group = LIGMA_GROUP_LAYER (layer);

  ligma_group_layer_update_source_node (group);
  ligma_group_layer_update_mode_node (group);

  if (LIGMA_LAYER_CLASS (parent_class)->excludes_backdrop_changed)
    LIGMA_LAYER_CLASS (parent_class)->excludes_backdrop_changed (layer);
}

static GeglRectangle
ligma_group_layer_get_bounding_box (LigmaLayer *layer)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);

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
    return LIGMA_LAYER_CLASS (parent_class)->get_bounding_box (layer);
}

static void
ligma_group_layer_get_effective_mode (LigmaLayer              *layer,
                                     LigmaLayerMode          *mode,
                                     LigmaLayerColorSpace    *blend_space,
                                     LigmaLayerColorSpace    *composite_space,
                                     LigmaLayerCompositeMode *composite_mode)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);

  /* try to strength-reduce pass-through groups to normal groups, which are
   * cheaper.
   */
  if (ligma_layer_get_mode (layer) == LIGMA_LAYER_MODE_PASS_THROUGH &&
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

      *mode            = LIGMA_LAYER_MODE_NORMAL;
      *blend_space     = ligma_layer_get_real_blend_space (layer);
      *composite_space = ligma_layer_get_real_composite_space (layer);
      *composite_mode  = ligma_layer_get_real_composite_mode (layer);

      for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          LigmaLayer *child = list->data;

          if (! ligma_filter_get_active (LIGMA_FILTER (child)))
            continue;

          if (first)
            {
              ligma_layer_get_effective_mode (child,
                                             mode,
                                             blend_space,
                                             composite_space,
                                             composite_mode);

              if (*mode == LIGMA_LAYER_MODE_NORMAL_LEGACY)
                *mode = LIGMA_LAYER_MODE_NORMAL;

              first = FALSE;
            }
          else
            {
              LigmaLayerMode          other_mode;
              LigmaLayerColorSpace    other_blend_space;
              LigmaLayerColorSpace    other_composite_space;
              LigmaLayerCompositeMode other_composite_mode;

              if (*mode           != LIGMA_LAYER_MODE_NORMAL ||
                  *composite_mode != LIGMA_LAYER_COMPOSITE_UNION)
                {
                  reduce = FALSE;

                  break;
                }

              ligma_layer_get_effective_mode (child,
                                             &other_mode,
                                             &other_blend_space,
                                             &other_composite_space,
                                             &other_composite_mode);

              if (other_mode == LIGMA_LAYER_MODE_NORMAL_LEGACY)
                other_mode = LIGMA_LAYER_MODE_NORMAL;

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

          has_mask = ligma_layer_get_mask (layer) &&
                     ligma_layer_get_apply_mask (layer);

          if (first                                                  ||
              (ligma_layer_get_opacity (layer) == LIGMA_OPACITY_OPAQUE &&
               ! has_mask)                                           ||
              *composite_space == ligma_layer_get_real_composite_space (layer))
            {
              /* strength reduction succeeded! */
              return;
            }
        }
    }

  /* strength-reduction failed.  chain up. */
  LIGMA_LAYER_CLASS (parent_class)->get_effective_mode (layer,
                                                       mode,
                                                       blend_space,
                                                       composite_space,
                                                       composite_mode);
}

static gboolean
ligma_group_layer_get_excludes_backdrop (LigmaLayer *layer)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (layer);

  if (private->pass_through)
    {
      GList *list;

      for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
           list;
           list = g_list_next (list))
        {
          LigmaFilter *child = list->data;

          if (ligma_filter_get_active (child) &&
              ligma_layer_get_excludes_backdrop (LIGMA_LAYER (child)))
            return TRUE;
        }

      return FALSE;
    }
  else
    return LIGMA_LAYER_CLASS (parent_class)->get_excludes_backdrop (layer);
}

static const Babl *
ligma_group_layer_get_format (LigmaProjectable *projectable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (projectable);
  LigmaImageBaseType      base_type;
  LigmaPrecision          precision;

  if (private->convert_format)
    return private->convert_format;

  base_type = ligma_drawable_get_base_type (LIGMA_DRAWABLE (projectable));
  precision = ligma_drawable_get_precision (LIGMA_DRAWABLE (projectable));

  return get_projection_format (projectable, base_type, precision);
}

static GeglRectangle
ligma_group_layer_projectable_get_bounding_box (LigmaProjectable *projectable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (projectable);

  return private->bounding_box;
}

static GeglNode *
ligma_group_layer_get_graph (LigmaProjectable *projectable)
{
  LigmaGroupLayer        *group   = LIGMA_GROUP_LAYER (projectable);
  LigmaGroupLayerPrivate *private = GET_PRIVATE (projectable);
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
    ligma_filter_stack_get_graph (LIGMA_FILTER_STACK (private->children));

  gegl_node_add_child (private->graph, layers_node);

  gegl_node_connect_to (input,       "output",
                        layers_node, "input");

  ligma_item_get_offset (LIGMA_ITEM (group), &off_x, &off_y);

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
ligma_group_layer_begin_render (LigmaProjectable *projectable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (projectable);

  if (private->source_node == NULL)
    return;

  if (private->pass_through)
    gegl_node_disconnect (private->graph, "input");
}

static void
ligma_group_layer_end_render (LigmaProjectable *projectable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (projectable);

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
ligma_group_layer_pickable_flush (LigmaPickable *pickable)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (pickable);

  ligma_pickable_flush (LIGMA_PICKABLE (private->projection));
}

static gdouble
ligma_group_layer_get_opacity_at (LigmaPickable *pickable,
                                 gint          x,
                                 gint          y)
{
  /* Only consider child layers as having content */

  return LIGMA_OPACITY_TRANSPARENT;
}


/*  public functions  */

LigmaLayer *
ligma_group_layer_new (LigmaImage *image)
{
  LigmaGroupLayer *group;
  const Babl     *format;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  format = ligma_image_get_layer_format (image, TRUE);

  group = LIGMA_GROUP_LAYER (ligma_drawable_new (LIGMA_TYPE_GROUP_LAYER,
                                               image, NULL,
                                               0, 0, 1, 1,
                                               format));

  ligma_layer_set_mode (LIGMA_LAYER (group),
                       ligma_image_get_default_new_layer_mode (image),
                       FALSE);

  return LIGMA_LAYER (group);
}

LigmaProjection *
ligma_group_layer_get_projection (LigmaGroupLayer *group)
{
  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);

  return GET_PRIVATE (group)->projection;
}

void
ligma_group_layer_suspend_resize (LigmaGroupLayer *group,
                                 gboolean        push_undo)
{
  LigmaItem *item;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  item = LIGMA_ITEM (group);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_group_layer_suspend_resize (ligma_item_get_image (item),
                                                     NULL, group);

  GET_PRIVATE (group)->suspend_resize++;
}

void
ligma_group_layer_resume_resize (LigmaGroupLayer *group,
                                gboolean        push_undo)
{
  LigmaGroupLayerPrivate *private;
  LigmaItem              *item;
  LigmaItem              *mask = NULL;
  GeglBuffer            *mask_buffer;
  GeglRectangle          mask_bounds;
  LigmaUndo              *undo;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_resize > 0);

  item = LIGMA_ITEM (group);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      undo =
        ligma_image_undo_push_group_layer_resume_resize (ligma_item_get_image (item),
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
          mask = LIGMA_ITEM (ligma_layer_get_mask (LIGMA_LAYER (group)));

          if (mask)
            {
              mask_buffer =
                g_object_ref (ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)));

              mask_bounds.x      = ligma_item_get_offset_x (mask);
              mask_bounds.y      = ligma_item_get_offset_y (mask);
              mask_bounds.width  = ligma_item_get_width    (mask);
              mask_bounds.height = ligma_item_get_height   (mask);
            }
        }
    }

  private->suspend_resize--;

  if (private->suspend_resize == 0)
    {
      ligma_group_layer_update_size (group);

      if (mask)
        {
          /* if the mask changed, make sure it's restored during undo, as per
           * the comment above.
           */
          if (ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)) != mask_buffer)
            {
              g_return_if_fail (undo != NULL);

              LIGMA_GROUP_LAYER_UNDO (undo)->mask_buffer = mask_buffer;
              LIGMA_GROUP_LAYER_UNDO (undo)->mask_bounds = mask_bounds;
            }
          else
            {
              g_object_unref (mask_buffer);
            }
        }
    }
}

void
ligma_group_layer_suspend_mask (LigmaGroupLayer *group,
                               gboolean        push_undo)
{
  LigmaGroupLayerPrivate *private;
  LigmaItem              *item;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = LIGMA_ITEM (group);

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
  if (! ligma_item_is_attached (item) || private->suspend_mask > 0)
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_group_layer_suspend_mask (ligma_item_get_image (item),
                                                   NULL, group);

  if (private->suspend_mask == 0)
    {
      if (ligma_layer_get_mask (LIGMA_LAYER (group)))
        {
          LigmaItem *mask = LIGMA_ITEM (ligma_layer_get_mask (LIGMA_LAYER (group)));

          private->suspended_mask_buffer =
            g_object_ref (ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)));

          private->suspended_mask_bounds.x      = ligma_item_get_offset_x (mask);
          private->suspended_mask_bounds.y      = ligma_item_get_offset_y (mask);
          private->suspended_mask_bounds.width  = ligma_item_get_width    (mask);
          private->suspended_mask_bounds.height = ligma_item_get_height   (mask);
        }
      else
        {
          private->suspended_mask_buffer = NULL;
        }
    }

  private->suspend_mask++;
}

void
ligma_group_layer_resume_mask (LigmaGroupLayer *group,
                              gboolean        push_undo)
{
  LigmaGroupLayerPrivate *private;
  LigmaItem              *item;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);

  g_return_if_fail (private->suspend_mask > 0);

  item = LIGMA_ITEM (group);

  /* avoid pushing an undo step if this is a nested resume_mask() call.  see
   * the comment in ligma_group_layer_suspend_mask().
   */
  if (! ligma_item_is_attached (item) || private->suspend_mask > 1)
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_group_layer_resume_mask (ligma_item_get_image (item),
                                                  NULL, group);

  private->suspend_mask--;

  if (private->suspend_mask == 0)
    g_clear_object (&private->suspended_mask_buffer);
}


/*  protected functions  */

void
_ligma_group_layer_set_suspended_mask (LigmaGroupLayer      *group,
                                      GeglBuffer          *buffer,
                                      const GeglRectangle *bounds)
{
  LigmaGroupLayerPrivate *private;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));
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
_ligma_group_layer_get_suspended_mask (LigmaGroupLayer *group,
                                      GeglRectangle  *bounds)
{
  LigmaGroupLayerPrivate *private;
  LigmaLayerMask         *mask;

  g_return_val_if_fail (LIGMA_IS_GROUP_LAYER (group), NULL);
  g_return_val_if_fail (bounds != NULL, NULL);

  private = GET_PRIVATE (group);
  mask    = ligma_layer_get_mask (LIGMA_LAYER (group));

  g_return_val_if_fail (private->suspend_mask > 0, NULL);

  if (mask &&
      ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)) !=
      private->suspended_mask_buffer)
    {
      *bounds = private->suspended_mask_bounds;

      return private->suspended_mask_buffer;
    }

  return NULL;
}

void
_ligma_group_layer_start_transform (LigmaGroupLayer *group,
                                   gboolean        push_undo)
{
  LigmaGroupLayerPrivate *private;
  LigmaItem              *item;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = LIGMA_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_group_layer_start_transform (ligma_item_get_image (item),
                                                      NULL, group);

  private->transforming++;
}

void
_ligma_group_layer_end_transform (LigmaGroupLayer *group,
                                 gboolean        push_undo)
{
  LigmaGroupLayerPrivate *private;
  LigmaItem              *item;

  g_return_if_fail (LIGMA_IS_GROUP_LAYER (group));

  private = GET_PRIVATE (group);
  item    = LIGMA_ITEM (group);

  g_return_if_fail (private->suspend_mask == 0);
  g_return_if_fail (private->transforming > 0);

  if (! ligma_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    ligma_image_undo_push_group_layer_end_transform (ligma_item_get_image (item),
                                                    NULL, group);

  private->transforming--;

  if (private->transforming == 0)
    ligma_group_layer_update_mask_size (LIGMA_GROUP_LAYER (item));
}


/*  private functions  */

static void
ligma_group_layer_child_add (LigmaContainer  *container,
                            LigmaLayer      *child,
                            LigmaGroupLayer *group)
{
  ligma_group_layer_update (group);

  if (ligma_filter_get_active (LIGMA_FILTER (child)))
    {
      ligma_layer_update_effective_mode (LIGMA_LAYER (group));

      if (ligma_layer_get_excludes_backdrop (child))
        ligma_layer_update_excludes_backdrop (LIGMA_LAYER (group));
    }
}

static void
ligma_group_layer_child_remove (LigmaContainer  *container,
                               LigmaLayer      *child,
                               LigmaGroupLayer *group)
{
  ligma_group_layer_update (group);

  if (ligma_filter_get_active (LIGMA_FILTER (child)))
    {
      ligma_layer_update_effective_mode (LIGMA_LAYER (group));

      if (ligma_layer_get_excludes_backdrop (child))
        ligma_layer_update_excludes_backdrop (LIGMA_LAYER (group));
    }
}

static void
ligma_group_layer_child_move (LigmaLayer      *child,
                             GParamSpec     *pspec,
                             LigmaGroupLayer *group)
{
  ligma_group_layer_update (group);
}

static void
ligma_group_layer_child_resize (LigmaLayer      *child,
                               LigmaGroupLayer *group)
{
  ligma_group_layer_update (group);
}

static void
ligma_group_layer_child_active_changed (LigmaLayer      *child,
                                       LigmaGroupLayer *group)
{
  ligma_layer_update_effective_mode (LIGMA_LAYER (group));

  if (ligma_layer_get_excludes_backdrop (child))
    ligma_layer_update_excludes_backdrop (LIGMA_LAYER (group));
}

static void
ligma_group_layer_child_effective_mode_changed (LigmaLayer      *child,
                                               LigmaGroupLayer *group)
{
  if (ligma_filter_get_active (LIGMA_FILTER (child)))
    ligma_layer_update_effective_mode (LIGMA_LAYER (group));
}

static void
ligma_group_layer_child_excludes_backdrop_changed (LigmaLayer      *child,
                                                  LigmaGroupLayer *group)
{
  if (ligma_filter_get_active (LIGMA_FILTER (child)))
    ligma_layer_update_excludes_backdrop (LIGMA_LAYER (group));
}

static void
ligma_group_layer_flush (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);

  if (private->pass_through)
    {
      /*  flush the projectable, not the pickable, because the source
       *  node of pass-through groups doesn't use the projection's
       *  buffer, hence there's no need to invalidate it synchronously.
       */
      ligma_projectable_flush (LIGMA_PROJECTABLE (group), TRUE);
    }
  else
    {
      /* make sure we have a buffer, and stop any idle rendering, which is
       * initiated when a new buffer is allocated.  the call to
       * ligma_pickable_flush() below causes any pending idle rendering to
       * finish synchronously, so this needs to happen before.
       */
      ligma_pickable_get_buffer (LIGMA_PICKABLE (private->projection));
      ligma_projection_stop_rendering (private->projection);

      /*  flush the pickable not the projectable because flushing the
       *  pickable will finish all invalidation on the projection so it
       *  can be used as source (note that it will still be constructed
       *  when the actual read happens, so this it not a performance
       *  problem)
       */
      ligma_pickable_flush (LIGMA_PICKABLE (private->projection));
    }
}

static void
ligma_group_layer_update (LigmaGroupLayer *group)
{
  if (GET_PRIVATE (group)->suspend_resize == 0)
    {
      ligma_group_layer_update_size (group);
    }
}

static void
ligma_group_layer_update_size (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);
  LigmaItem              *item    = LIGMA_ITEM (group);
  LigmaLayer             *layer   = LIGMA_LAYER (group);
  LigmaItem              *mask    = LIGMA_ITEM (ligma_layer_get_mask (layer));
  GeglRectangle          old_bounds;
  GeglRectangle          bounds;
  GeglRectangle          old_bounding_box;
  GeglRectangle          bounding_box;
  gboolean               first   = TRUE;
  gboolean               size_changed;
  gboolean               resize_mask;
  GList                 *list;

  old_bounds.x      = ligma_item_get_offset_x (item);
  old_bounds.y      = ligma_item_get_offset_y (item);
  old_bounds.width  = ligma_item_get_width    (item);
  old_bounds.height = ligma_item_get_height   (item);

  bounds.x          = 0;
  bounds.y          = 0;
  bounds.width      = 1;
  bounds.height     = 1;

  old_bounding_box  = private->bounding_box;
  bounding_box      = bounds;

  for (list = ligma_item_stack_get_item_iter (LIGMA_ITEM_STACK (private->children));
       list;
       list = g_list_next (list))
    {
      LigmaItem      *child = list->data;
      GeglRectangle  child_bounds;
      GeglRectangle  child_bounding_box;

      if (! ligma_viewable_get_size (LIGMA_VIEWABLE (child),
                                    &child_bounds.width, &child_bounds.height))
        {
          /*  ignore children without content (empty group layers);
           *  see bug 777017
           */
          continue;
        }

      ligma_item_get_offset (child, &child_bounds.x, &child_bounds.y);

      child_bounding_box =
        ligma_drawable_get_bounding_box (LIGMA_DRAWABLE (child));

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
  if (resize_mask && ligma_layer_get_show_mask (layer))
    {
      ligma_drawable_update (LIGMA_DRAWABLE (group),
                            ligma_item_get_offset_x (mask) - old_bounds.x,
                            ligma_item_get_offset_y (mask) - old_bounds.y,
                            ligma_item_get_width    (mask),
                            ligma_item_get_height   (mask));
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

      /* update our offset *before* calling ligma_pickable_get_buffer(), so
       * that if our graph isn't constructed yet, the offset node picks
       * up the right coordinates in ligma_group_layer_get_graph().
       */
      ligma_item_set_offset (item, bounds.x, bounds.y);

      /* update the bounding box before updating the projection, so that it
       * picks up the right size.
       */
      private->bounding_box = bounding_box;

      if (private->reallocate_projection)
        {
          private->reallocate_projection = FALSE;

          ligma_projectable_structure_changed (LIGMA_PROJECTABLE (group));
        }
      else
        {
          /* when there's no need to reallocate the projection, we call
           * ligma_projectable_bounds_changed(), rather than structure_chaned(),
           * so that the projection simply copies the old content over to the
           * new buffer with an offset, rather than re-renders the graph.
           */
          ligma_projectable_bounds_changed (LIGMA_PROJECTABLE (group),
                                           old_bounds.x, old_bounds.y);
        }

      buffer = ligma_pickable_get_buffer (LIGMA_PICKABLE (private->projection));

      ligma_drawable_set_buffer_full (LIGMA_DRAWABLE (group),
                                     FALSE, NULL,
                                     buffer, &bounds,
                                     FALSE /* don't update the drawable, the
                                            * flush() below will take care of
                                            * that.
                                            */);

      ligma_group_layer_flush (group);
    }

  /* resize the mask if not transforming (in which case, LigmaLayer takes care
   * of the mask)
   */
  if (resize_mask && ! private->transforming)
    ligma_group_layer_update_mask_size (group);

  /* if we show the mask, invalidate the new mask area */
  if (resize_mask && ligma_layer_get_show_mask (layer))
    {
      ligma_drawable_update (LIGMA_DRAWABLE (group),
                            ligma_item_get_offset_x (mask) - bounds.x,
                            ligma_item_get_offset_y (mask) - bounds.y,
                            ligma_item_get_width    (mask),
                            ligma_item_get_height   (mask));
    }
}

static void
ligma_group_layer_update_mask_size (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);
  LigmaItem              *item    = LIGMA_ITEM (group);
  LigmaItem              *mask;
  GeglBuffer            *buffer;
  GeglBuffer            *mask_buffer;
  GeglRectangle          bounds;
  GeglRectangle          mask_bounds;
  GeglRectangle          copy_bounds;
  gboolean               intersect;

  mask = LIGMA_ITEM (ligma_layer_get_mask (LIGMA_LAYER (group)));

  if (! mask)
    return;

  bounds.x           = ligma_item_get_offset_x (item);
  bounds.y           = ligma_item_get_offset_y (item);
  bounds.width       = ligma_item_get_width    (item);
  bounds.height      = ligma_item_get_height   (item);

  mask_bounds.x      = ligma_item_get_offset_x (mask);
  mask_bounds.y      = ligma_item_get_offset_y (mask);
  mask_bounds.width  = ligma_item_get_width    (mask);
  mask_bounds.height = ligma_item_get_height   (mask);

  if (gegl_rectangle_equal (&bounds, &mask_bounds))
    return;

  buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, bounds.width, bounds.height),
                            ligma_drawable_get_format (LIGMA_DRAWABLE (mask)));

  if (private->suspended_mask_buffer)
    {
      /* copy the suspended mask into the new mask */
      mask_buffer = private->suspended_mask_buffer;
      mask_bounds = private->suspended_mask_bounds;
    }
  else
    {
      /* copy the old mask into the new mask */
      mask_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));
    }

  intersect = ligma_rectangle_intersect (bounds.x,
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
      ligma_gegl_buffer_copy (mask_buffer,
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

  ligma_drawable_set_buffer_full (LIGMA_DRAWABLE (mask),
                                 FALSE, NULL,
                                 buffer, &bounds,
                                 TRUE);

  g_object_unref (buffer);
}

static void
ligma_group_layer_update_source_node (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);
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
ligma_group_layer_update_mode_node (LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);
  GeglNode              *node;
  GeglNode              *input;
  GeglNode              *mode_node;

  node      = ligma_filter_get_node (LIGMA_FILTER (group));
  input     = gegl_node_get_input_proxy (node, "input");
  mode_node = ligma_drawable_get_mode_node (LIGMA_DRAWABLE (group));

  if (private->pass_through &&
      ligma_layer_get_excludes_backdrop (LIGMA_LAYER (group)))
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
ligma_group_layer_stack_update (LigmaDrawableStack *stack,
                               gint               x,
                               gint               y,
                               gint               width,
                               gint               height,
                               LigmaGroupLayer    *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, ligma_object_get_name (group),
              x, y, width, height);
#endif

  if (! private->direct_update)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  pass to the projection as-is.
       */
      ligma_projectable_invalidate (LIGMA_PROJECTABLE (group),
                                   x, y, width, height);

      ligma_group_layer_flush (group);
    }

  if (private->direct_update || private->pass_through)
    {
      /*  the layer stack's update signal speaks in image coordinates,
       *  transform to layer coordinates when emitting our own update signal.
       */
      ligma_drawable_update (LIGMA_DRAWABLE (group),
                            x - ligma_item_get_offset_x (LIGMA_ITEM (group)),
                            y - ligma_item_get_offset_y (LIGMA_ITEM (group)),
                            width, height);
    }
}

static void
ligma_group_layer_proj_update (LigmaProjection *proj,
                              gboolean        now,
                              gint            x,
                              gint            y,
                              gint            width,
                              gint            height,
                              LigmaGroupLayer *group)
{
  LigmaGroupLayerPrivate *private = GET_PRIVATE (group);

#if 0
  g_printerr ("%s (%s) %d, %d (%d, %d)\n",
              G_STRFUNC, ligma_object_get_name (group),
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
       * and don't do this more generally in ligma_drawable_update(), because this
       * negatively impacts the performance of the warp tool, which does perform
       * accurate drawable updates while using a filter.
       */
      if (ligma_drawable_has_filters (LIGMA_DRAWABLE (group)))
        {
          width  = -1;
          height = -1;
        }

      /*  the projection speaks in image coordinates, transform to layer
       *  coordinates when emitting our own update signal.
       */
      ligma_drawable_update (LIGMA_DRAWABLE (group),
                            x - ligma_item_get_offset_x (LIGMA_ITEM (group)),
                            y - ligma_item_get_offset_y (LIGMA_ITEM (group)),
                            width, height);
    }
}
