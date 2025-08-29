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

/* This file contains the code necessary for generating on canvas
 * previews, by connecting a specified GEGL operation to do the
 * processing. It uses drawable filters that allow for non-destructive
 * manipulation of drawable data, with live preview on screen.
 *
 * To create a tool that uses this, see app/tools/gimpfiltertool.c for
 * the interface and e.g. app/tools/gimpcolorbalancetool.c for an
 * example of using that interface.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <gegl-plugin.h>
#include <gegl-paramspecs.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "operations/gimp-operation-config.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawablefilter.h"
#include "gimpdrawablefiltermask.h"
#include "gimperror.h"
#include "gimpidtable.h"
#include "gimpimage.h"
#include "gimplayer.h"
#include "gimplist.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  FLUSH,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_DRAWABLE,
  PROP_MASK,
  PROP_CUSTOM_NAME,
  PROP_TEMPORARY,
  PROP_TO_BE_MERGED,
  N_PROPS
};


struct _GimpDrawableFilter
{
  GimpFilter              parent_instance;

  gint                    ID;

  GimpDrawable           *drawable;
  GimpDrawableFilterMask *mask;
  GeglNode               *operation;

  gboolean                has_input;
  gboolean                has_custom_name;

  gboolean                clip;
  GimpFilterRegion        region;
  gboolean                crop_enabled;
  GeglRectangle           crop_rect;
  gboolean                preview_enabled;
  gboolean                preview_split_enabled;
  GimpAlignmentType       preview_split_alignment;
  gint                    preview_split_position;
  gdouble                 opacity;
  GimpLayerMode           paint_mode;
  GimpLayerColorSpace     blend_space;
  GimpLayerColorSpace     composite_space;
  GimpLayerCompositeMode  composite_mode;
  gboolean                add_alpha;
  gboolean                color_managed;

  gboolean                override_constraints;

  GeglRectangle           filter_area;
  gboolean                filter_clip;

  GeglNode               *translate;
  GeglNode               *crop_before;
  GeglNode               *crop_after;
  GimpApplicator         *applicator;

  gboolean                temporary;
  /* This is mirroring merge_filter option of GimpFilterOptions. */
  gboolean                to_be_merged;
};

static void       gimp_drawable_filter_set_property          (GObject             *object,
                                                              guint                property_id,
                                                              const GValue        *value,
                                                              GParamSpec          *pspec);

static void       gimp_drawable_filter_get_property          (GObject             *object,
                                                              guint                property_id,
                                                              GValue              *value,
                                                              GParamSpec          *pspec);
static void       gimp_drawable_filter_dispose               (GObject             *object);
static void       gimp_drawable_filter_finalize              (GObject             *object);

static void       gimp_drawable_filter_sync_active           (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_clip             (GimpDrawableFilter  *filter,
                                                              gboolean             sync_region);
static void       gimp_drawable_filter_sync_region           (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_crop             (GimpDrawableFilter  *filter,
                                                              gboolean             old_crop_enabled,
                                                              const GeglRectangle *old_crop_rect,
                                                              gboolean             old_preview_split_enabled,
                                                              GimpAlignmentType    old_preview_split_alignment,
                                                              gint                 old_preview_split_position,
                                                              gboolean             update);
static void       gimp_drawable_filter_sync_opacity          (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_mode             (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_affect           (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_format           (GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_sync_mask             (GimpDrawableFilter  *filter);

static gboolean   gimp_drawable_filter_is_added              (GimpDrawableFilter  *filter);
static gboolean   gimp_drawable_filter_is_active             (GimpDrawableFilter  *filter);
static gboolean   gimp_drawable_filter_add_filter            (GimpDrawableFilter  *filter);
static gboolean   gimp_drawable_filter_remove_filter         (GimpDrawableFilter  *filter);

static void       gimp_drawable_filter_update_drawable       (GimpDrawableFilter  *filter,
                                                              const GeglRectangle *area);

static void       gimp_drawable_filter_affect_changed        (GimpImage           *image,
                                                              GimpChannelType      channel,
                                                              GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_mask_changed          (GimpImage           *image,
                                                              GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_lock_position_changed (GimpDrawable        *drawable,
                                                              GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_format_changed        (GimpDrawable        *drawable,
                                                              GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_drawable_removed      (GimpDrawable        *drawable,
                                                              GimpDrawableFilter  *filter);
static void       gimp_drawable_filter_lock_alpha_changed    (GimpLayer           *layer,
                                                              GimpDrawableFilter  *filter);

static void       gimp_drawable_filter_reorder               (GimpFilterStack    *stack,
                                                              GimpDrawableFilter *reordered_filter,
                                                              gint                old_index,
                                                              gint                new_index,
                                                              GimpDrawableFilter *filter);


G_DEFINE_TYPE (GimpDrawableFilter, gimp_drawable_filter, GIMP_TYPE_FILTER)

#define parent_class gimp_drawable_filter_parent_class

static guint       drawable_filter_signals[LAST_SIGNAL] = { 0, };
static GParamSpec *drawable_filter_props[N_PROPS]       = { NULL, };


static void
gimp_drawable_filter_class_init (GimpDrawableFilterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  drawable_filter_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpDrawableFilterClass, flush),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->set_property = gimp_drawable_filter_set_property;
  object_class->get_property = gimp_drawable_filter_get_property;
  object_class->dispose      = gimp_drawable_filter_dispose;
  object_class->finalize     = gimp_drawable_filter_finalize;

  drawable_filter_props[PROP_ID]       = g_param_spec_int ("id", NULL, NULL,
                                                           0, G_MAXINT, 0,
                                                           GIMP_PARAM_READABLE);

  drawable_filter_props[PROP_DRAWABLE] = g_param_spec_object ("drawable", NULL, NULL,
                                                              GIMP_TYPE_DRAWABLE,
                                                              GIMP_PARAM_READWRITE |
                                                              G_PARAM_CONSTRUCT_ONLY);
  /* The mask is in fact a GimpDrawableFilterMask but the property is a
   * GimpDrawable to allow setting any drawable. The set_property() code
   * will take care of creating a new object of the proper type.
   */
  drawable_filter_props[PROP_MASK]     = g_param_spec_object ("mask",
                                                              NULL, NULL,
                                                              GIMP_TYPE_DRAWABLE,
                                                              GIMP_PARAM_READWRITE);

  drawable_filter_props[PROP_CUSTOM_NAME] = g_param_spec_boolean ("custom-name",
                                                                  NULL, NULL,
                                                                  FALSE,
                                                                  GIMP_PARAM_READWRITE);

  drawable_filter_props[PROP_TEMPORARY] = g_param_spec_boolean ("temporary",
                                                                NULL, NULL,
                                                                FALSE,
                                                                GIMP_PARAM_READWRITE);

  drawable_filter_props[PROP_TO_BE_MERGED] = g_param_spec_boolean ("to-be-merged",
                                                                   NULL, NULL,
                                                                   FALSE,
                                                                   GIMP_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, drawable_filter_props);
}

static void
gimp_drawable_filter_init (GimpDrawableFilter *drawable_filter)
{
  drawable_filter->clip                    = TRUE;
  drawable_filter->region                  = GIMP_FILTER_REGION_SELECTION;
  drawable_filter->preview_enabled         = TRUE;
  drawable_filter->preview_split_enabled   = FALSE;
  drawable_filter->preview_split_alignment = GIMP_ALIGN_LEFT;
  drawable_filter->preview_split_position  = 0;
  drawable_filter->opacity                 = GIMP_OPACITY_OPAQUE;
  drawable_filter->paint_mode              = GIMP_LAYER_MODE_REPLACE;
  drawable_filter->blend_space             = GIMP_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_space         = GIMP_LAYER_COLOR_SPACE_AUTO;
  drawable_filter->composite_mode          = GIMP_LAYER_COMPOSITE_AUTO;
}

static void
gimp_drawable_filter_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpDrawableFilter     *filter = GIMP_DRAWABLE_FILTER (object);
  GimpDrawableFilterMask *mask   = NULL;
  GimpImage              *image;

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_set_object (&filter->drawable, g_value_get_object (value));
      break;

    case PROP_MASK:
      image = gimp_item_get_image (GIMP_ITEM (filter->drawable));

      if (g_value_get_object (value) != NULL)
        mask = GIMP_DRAWABLE_FILTER_MASK (gimp_item_convert (GIMP_ITEM (g_value_get_object (value)),
                                                             image, GIMP_TYPE_DRAWABLE_FILTER_MASK));
      g_set_object (&filter->mask, mask);
      g_clear_object (&mask);

      if (filter->mask)
        {
          gimp_drawable_filter_mask_set_filter (filter->mask, filter);
          gimp_drawable_filter_sync_mask (filter);
        }
      break;

    case PROP_CUSTOM_NAME:
      filter->has_custom_name = g_value_get_boolean (value);
      break;

    case PROP_TEMPORARY:
      filter->temporary = g_value_get_boolean (value);
      break;

    case PROP_TO_BE_MERGED:
      filter->to_be_merged = g_value_get_boolean (value);
      gimp_drawable_filter_sync_format (filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_filter_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpDrawableFilter *filter = GIMP_DRAWABLE_FILTER (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, filter->ID);
      break;
    case PROP_MASK:
      g_value_set_object (value, gimp_drawable_filter_get_mask (filter));
      break;
    case PROP_CUSTOM_NAME:
      g_value_set_boolean (value, filter->has_custom_name);
      break;
    case PROP_TEMPORARY:
      g_value_set_boolean (value, filter->temporary);
      break;
    case PROP_TO_BE_MERGED:
      g_value_set_boolean (value, filter->to_be_merged);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_filter_dispose (GObject *object)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    gimp_drawable_filter_remove_filter (drawable_filter);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_filter_finalize (GObject *object)
{
  GimpDrawableFilter *drawable_filter = GIMP_DRAWABLE_FILTER (object);

  if (drawable_filter->drawable)
    {
      GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable_filter->drawable));

      gimp_id_table_remove (image->gimp->drawable_filter_table, drawable_filter->ID);
    }

  g_clear_object (&drawable_filter->applicator);
  g_clear_object (&drawable_filter->drawable);
  g_clear_object (&drawable_filter->operation);
  g_clear_object (&drawable_filter->mask);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GimpDrawableFilter *
gimp_drawable_filter_new (GimpDrawable *drawable,
                          const gchar  *undo_desc,
                          GeglNode     *operation,
                          const gchar  *icon_name)
{
  GimpDrawableFilter *filter;
  GimpImage          *image;
  GeglNode           *node;
  GeglOperation      *op          = NULL;
  GeglOperationClass *opclass     = NULL;
  gboolean            custom_name = TRUE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (operation), NULL);
  g_return_val_if_fail (gegl_node_has_pad (operation, "output"), NULL);

  op = gegl_node_get_gegl_operation (operation);
  if (op != NULL)
    opclass = GEGL_OPERATION_GET_CLASS (op);

  if (undo_desc == NULL || strlen (undo_desc) == 0)
    {
      undo_desc   = gegl_operation_class_get_key (opclass, "title");
      custom_name = FALSE;
    }

  if (opclass &&
      ! g_strcmp0 (undo_desc, gegl_operation_class_get_key (opclass, "title")))
    custom_name = FALSE;

  filter = g_object_new (GIMP_TYPE_DRAWABLE_FILTER,
                         "name",        undo_desc,
                         "icon-name",   icon_name,
                         "custom-name", custom_name,
                         "drawable",    drawable,
                         "mask",        NULL,
                         NULL);

  filter->operation = g_object_ref (operation);

  image      = gimp_item_get_image (GIMP_ITEM (drawable));
  filter->ID = gimp_id_table_insert (image->gimp->drawable_filter_table, filter);
  g_object_notify_by_pspec (G_OBJECT (filter), drawable_filter_props[PROP_ID]);

  node = gimp_filter_get_node (GIMP_FILTER (filter));

  if (! gegl_node_get_parent (operation))
    {
      gegl_node_add_child (node, operation);
      gimp_gegl_node_set_underlying_operation (node, operation);
    }

  filter->applicator = gimp_applicator_new (node);

  gimp_filter_set_applicator (GIMP_FILTER (filter), filter->applicator);

  gimp_applicator_set_cache (filter->applicator, TRUE);

  filter->has_input = gegl_node_has_pad (filter->operation, "input");

  if (filter->has_input)
    {
      GeglNode *input;

      input = gegl_node_get_input_proxy (node, "input");

      filter->translate = gegl_node_new_child (node,
                                               "operation", "gegl:translate",
                                               NULL);

      filter->crop_before = gegl_node_new_child (node,
                                                 "operation", "gegl:crop",
                                                 NULL);

      gegl_node_link_many (input,
                           filter->translate,
                           filter->crop_before,
                           filter->operation,
                           NULL);
    }

  filter->crop_after = gegl_node_new_child (node,
                                            "operation", "gegl:crop",
                                            NULL);

  gegl_node_link_many (filter->operation,
                       filter->crop_after,
                       NULL);

  gegl_node_connect (filter->crop_after, "output", node, "aux");

  return filter;
}

GimpDrawableFilter *
gimp_drawable_filter_duplicate (GimpDrawable       *drawable,
                                GimpDrawableFilter *prior_filter)
{
  GimpImage          *image;
  GimpDrawableFilter *filter;
  GeglNode           *prior_node;
  GeglNode           *node = gegl_node_new ();
  gchar              *operation;
  const gchar        *undo_desc;
  const gchar        *icon_name;
  GParamSpec        **pspecs;
  guint               n_pspecs;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (prior_filter), NULL);

  prior_node = gimp_drawable_filter_get_operation (prior_filter);

  /* Don't duplicate tool-based filters (for now) or temporary filters */
  if (gimp_drawable_filter_get_mask (prior_filter) == NULL ||
      prior_node                                   == NULL ||
      ! strcmp (gegl_node_get_operation (prior_node), "GraphNode"))
    return NULL;

  g_object_get (prior_filter,
                "name",      &undo_desc,
                "icon-name", &icon_name,
                NULL);

  gegl_node_get (prior_node,
                 "operation", &operation,
                 NULL);

  gegl_node_set (node,
                 "operation", operation,
                 NULL);

  pspecs = gegl_operation_list_properties (operation, &n_pspecs);

  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      gegl_node_get_property (prior_node, pspec->name,
                              &value);

      gegl_node_set_property (node, pspec->name,
                              &value);
      g_value_unset (&value);
    }
  g_free (pspecs);

  filter = gimp_drawable_filter_new (drawable, undo_desc, node, icon_name);
  g_object_unref (node);

  gimp_drawable_filter_set_clip (filter, prior_filter->clip);
  gimp_drawable_filter_set_opacity (filter, prior_filter->opacity);
  gimp_drawable_filter_set_mode (filter,
                                 prior_filter->paint_mode,
                                 prior_filter->blend_space,
                                 prior_filter->composite_space,
                                 prior_filter->composite_mode);
  gimp_drawable_filter_set_region (filter,
                                   prior_filter->region);
  gimp_drawable_filter_set_add_alpha (filter,
                                      prior_filter->add_alpha);
  gimp_filter_set_active (GIMP_FILTER (filter),
                          gimp_filter_get_active (GIMP_FILTER (prior_filter)));
  gimp_filter_set_is_last_node (GIMP_FILTER (filter),
                                gimp_filter_get_is_last_node (GIMP_FILTER (prior_filter)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  if (image != NULL)
    g_object_set (filter,
                  "mask", prior_filter->mask,
                  NULL);

  g_free (operation);

  return filter;
}

gint
gimp_drawable_filter_get_id (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), -1);

  return filter->ID;
}

GimpDrawableFilter *
gimp_drawable_filter_get_by_id (Gimp *gimp,
                                gint  filter_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->drawable_filter_table == NULL)
    return NULL;

  return (GimpDrawableFilter *) gimp_id_table_lookup (gimp->drawable_filter_table, filter_id);
}

GimpDrawable *
gimp_drawable_filter_get_drawable (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->drawable;
}

GeglNode *
gimp_drawable_filter_get_operation (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->operation;
}

GimpDrawableFilterMask *
gimp_drawable_filter_get_mask (GimpDrawableFilter  *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), NULL);

  return filter->mask;
}

void
gimp_drawable_filter_set_temporary (GimpDrawableFilter *filter,
                                    gboolean            temporary)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  temporary = temporary ? TRUE : FALSE;

  if (temporary != filter->temporary)
    {
      g_object_set (filter,
                    "temporary", temporary,
                    NULL);
    }
}

gboolean
gimp_drawable_filter_get_temporary (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->temporary;
}

void
gimp_drawable_filter_set_opacity (GimpDrawableFilter *filter,
                                  gdouble             opacity)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      gimp_drawable_filter_sync_opacity (filter);

      if (gimp_drawable_filter_is_active (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

gdouble
gimp_drawable_filter_get_opacity (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0.0f);

  return filter->opacity;
}

void
gimp_drawable_filter_set_mode (GimpDrawableFilter     *filter,
                               GimpLayerMode           paint_mode,
                               GimpLayerColorSpace     blend_space,
                               GimpLayerColorSpace     composite_space,
                               GimpLayerCompositeMode  composite_mode)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      gimp_drawable_filter_sync_mode (filter);

      if (gimp_drawable_filter_is_active (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

GimpLayerMode
gimp_drawable_filter_get_paint_mode (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0);

  return filter->paint_mode;
}

GimpLayerColorSpace
gimp_drawable_filter_get_blend_space (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0);

  return filter->blend_space;
}

GimpLayerColorSpace
gimp_drawable_filter_get_composite_space (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0);

  return filter->composite_space;
}

GimpLayerCompositeMode
gimp_drawable_filter_get_composite_mode (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0);

  return filter->composite_mode;
}

void
gimp_drawable_filter_set_clip (GimpDrawableFilter *filter,
                               gboolean            clip)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (clip != filter->clip)
    {
      filter->clip = clip;

      gimp_drawable_filter_sync_region (filter);
      gimp_drawable_filter_sync_clip (filter, TRUE);
    }
}

gboolean
gimp_drawable_filter_get_clip (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->clip;
}

void
gimp_drawable_filter_set_region (GimpDrawableFilter *filter,
                                 GimpFilterRegion    region)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (region != filter->region)
    {
      filter->region = region;

      gimp_drawable_filter_sync_region (filter);

      if (gimp_drawable_filter_is_active (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

GimpFilterRegion
gimp_drawable_filter_get_region (GimpDrawableFilter  *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), 0);

  return filter->region;
}

void
gimp_drawable_filter_set_crop (GimpDrawableFilter  *filter,
                               const GeglRectangle *rect,
                               gboolean             update)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if ((rect != NULL) != filter->crop_enabled ||
      (rect && ! gegl_rectangle_equal (rect, &filter->crop_rect)))
    {
      gboolean      old_enabled = filter->crop_enabled;
      GeglRectangle old_rect    = filter->crop_rect;

      if (rect)
        {
          filter->crop_enabled = TRUE;
          filter->crop_rect    = *rect;
        }
      else
        {
          filter->crop_enabled = FALSE;
        }

      gimp_drawable_filter_sync_crop (filter,
                                      old_enabled,
                                      &old_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      update);
    }
}

void
gimp_drawable_filter_set_preview (GimpDrawableFilter *filter,
                                  gboolean            enabled)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (enabled != filter->preview_enabled)
    {
      filter->preview_enabled = enabled;

      gimp_drawable_filter_sync_active (filter);

      if (gimp_drawable_filter_is_added (filter))
        {
          gimp_drawable_update_bounding_box (filter->drawable);

          gimp_drawable_filter_update_drawable (filter, NULL);
        }
    }
}

void
gimp_drawable_filter_set_preview_split (GimpDrawableFilter  *filter,
                                        gboolean             enabled,
                                        GimpAlignmentType    alignment,
                                        gint                 position)
{
  GimpItem *item;

  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));
  g_return_if_fail (alignment == GIMP_ALIGN_LEFT  ||
                    alignment == GIMP_ALIGN_RIGHT ||
                    alignment == GIMP_ALIGN_TOP   ||
                    alignment == GIMP_ALIGN_BOTTOM);

  item = GIMP_ITEM (filter->drawable);

  switch (alignment)
    {
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_RIGHT:
      position = CLAMP (position, 0, gimp_item_get_width (item));
      break;

    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_BOTTOM:
      position = CLAMP (position, 0, gimp_item_get_height (item));
      break;

    default:
      g_return_if_reached ();
    }

  if (enabled   != filter->preview_split_enabled   ||
      alignment != filter->preview_split_alignment ||
      position  != filter->preview_split_position)
    {
      gboolean          old_enabled   = filter->preview_split_enabled;
      GimpAlignmentType old_alignment = filter->preview_split_alignment;
      gint              old_position  = filter->preview_split_position;

      filter->preview_split_enabled   = enabled;
      filter->preview_split_alignment = alignment;
      filter->preview_split_position  = position;

      gimp_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      old_enabled,
                                      old_alignment,
                                      old_position,
                                      TRUE);
    }
}

/* This function is **ONLY** for usage by libgimp API. The idea is to have
 * a single function which updates a bunch of settings in a single call
 * and in particular a single rendering update.
 *
 * Also it does some funky config object switch for custom operations
 * which is only needed libgimp-side.
 */
gboolean
gimp_drawable_filter_update (GimpDrawableFilter      *filter,
                             const gchar            **propnames,
                             const GimpValueArray    *values,
                             gdouble                  opacity,
                             GimpLayerMode            paint_mode,
                             GimpLayerColorSpace      blend_space,
                             GimpLayerColorSpace      composite_space,
                             GimpLayerCompositeMode   composite_mode,
                             const gchar            **auxinputnames,
                             GimpDrawable           **auxinputs,
                             GError                 **error)
{
  GimpImage    *image;
  GimpObject   *settings        = NULL;
  GeglNode     *node            = NULL;
  GParamSpec  **pspecs;
  gchar        *opname;
  guint         n_parent_pspecs = 0;
  guint         n_pspecs;
  gint          n_values;
  gint          n_auxinputs;
  gboolean      changed = FALSE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (error != NULL && *error == NULL, FALSE);

  n_values = gimp_value_array_length (values);

  if (n_values != g_strv_length ((gchar **) propnames))
    {
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   "%s: the number of property names and values differ.",
                   G_STRFUNC);
      return FALSE;
    }

  n_auxinputs = 0;
  while (auxinputs[n_auxinputs] != NULL)
    n_auxinputs++;

  if (n_auxinputs != g_strv_length ((gchar **) auxinputnames))
    {
      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   "%s: the number of aux input names and aux inputs differ.",
                   G_STRFUNC);
      return FALSE;
    }

  g_object_freeze_notify (G_OBJECT (filter));

  gegl_node_get (filter->operation, "operation", &opname, NULL);

  image = gimp_item_get_image (GIMP_ITEM (filter->drawable));
  node  = gimp_drawable_filter_get_operation (filter);
  if (gimp_operation_config_is_custom (image->gimp, opname))
    {
      GObjectClass *klass;
      GObjectClass *parent_klass;

      gegl_node_get (node,
                     "config", &settings,
                     NULL);
      klass        = G_OBJECT_GET_CLASS (settings);
      parent_klass = G_OBJECT_CLASS (g_type_class_peek_parent (klass));
      g_free (g_object_class_list_properties (parent_klass, &n_parent_pspecs));
      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (settings), &n_pspecs);
    }
  else
    {
      pspecs = gegl_operation_list_properties (opname, &n_pspecs);
    }

  for (gint i = n_parent_pspecs; i < n_pspecs; i++)
    {
      GParamSpec *target_pspec;
      GParamSpec *pspec     = pspecs[i];
      GValue      old_value = G_VALUE_INIT;
      gint        j;

      if (settings)
        target_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (settings), pspec->name);
      else
        target_pspec = gegl_node_find_property (node, pspec->name);
      if (! target_pspec)
        {
          /* If this ever happens, this is more likely a bug in our
           * PDB code, unless someone tried to call the PDB procedure
           * directly with bad data.
           */
          g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                       _("GEGL operation '%s' has been called with a "
                         "non-existent argument name '%s' (#%d)."),
                       opname, pspec->name, i);
          break;
        }

      if (settings)
        g_object_get_property (G_OBJECT (settings), pspec->name, &old_value);
      else
        gegl_node_get_property (node, pspec->name, &old_value);

      for (j = 0; j < n_values; j++)
        if (g_strcmp0 (pspec->name, propnames[j]) == 0)
          break;

      if (j < n_values)
        {
          GValue   *new_value;
          GValue    replaced_value        = G_VALUE_INIT;
          gboolean  new_value_initialized = FALSE;

          new_value = gimp_value_array_index (values, j);

          if (GEGL_IS_PARAM_SPEC_ENUM (pspec) && G_VALUE_HOLDS_STRING (new_value))
            {
              /* GEGL enum types are special-cased to be passed as
               * GimpChoice (string) param specs on libgimp.
               */
              GeglParamSpecEnum *gespec = GEGL_PARAM_SPEC_ENUM (pspec);
              GEnumClass        *enum_class;
              GEnumValue        *enum_value;
              const gchar       *new_str;

              new_str    = g_value_get_string (new_value);
              enum_class = g_type_class_ref (G_PARAM_SPEC_VALUE_TYPE (pspec));
              for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
                {
                  GSList *iter;

                  if (enum_value->value < enum_class->minimum || enum_value->value > enum_class->maximum)
                    continue;

                  for (iter = gespec->excluded_values; iter; iter = iter->next)
                    if (GPOINTER_TO_INT (iter->data) == enum_value->value)
                      break;

                  if (iter != NULL)
                    /* Excluded value. */
                    continue;

                  if (g_strcmp0 (enum_value->value_nick, new_str) == 0)
                    {
                      g_value_init (&replaced_value, G_PARAM_SPEC_VALUE_TYPE (pspec));
                      g_value_set_enum (&replaced_value, enum_value->value);
                      new_value = &replaced_value;
                      new_value_initialized = TRUE;
                    }
                }

              g_type_class_unref (enum_class);
            }
          else if (! G_TYPE_CHECK_VALUE_TYPE (new_value, G_PARAM_SPEC_VALUE_TYPE (pspec)))
            {
              g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                           _("GEGL operation '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           opname, pspec->name, i,
                           g_type_name (pspec->value_type),
                           g_type_name (G_VALUE_TYPE (new_value)));

              break;
            }

          if (g_param_values_cmp (pspec, new_value, &old_value) != 0)
            {
              if (settings)
                g_object_set_property (G_OBJECT (settings), pspec->name, new_value);
              else
                gegl_node_set_property (node, pspec->name, new_value);
              changed = TRUE;
            }

          if (new_value_initialized)
            g_value_unset (new_value);

          continue;
        }

      if (! g_param_value_defaults (pspec, &old_value))
        {
          GValue default_value = G_VALUE_INIT;

          g_value_init (&default_value, pspec->value_type);
          g_param_value_set_default (pspec, &default_value);
          if (settings)
            g_object_set_property (G_OBJECT (settings), pspec->name, &default_value);
          else
            gegl_node_set_property (node, pspec->name, &default_value);
          changed = TRUE;

          g_value_unset (&default_value);
        }

      g_value_unset (&old_value);
    }

  if (opacity != filter->opacity)
    {
      filter->opacity = opacity;

      gimp_drawable_filter_sync_opacity (filter);
      changed = TRUE;
    }

  if (paint_mode      != filter->paint_mode      ||
      blend_space     != filter->blend_space     ||
      composite_space != filter->composite_space ||
      composite_mode  != filter->composite_mode)
    {
      filter->paint_mode      = paint_mode;
      filter->blend_space     = blend_space;
      filter->composite_space = composite_space;
      filter->composite_mode  = composite_mode;

      gimp_drawable_filter_sync_mode (filter);
      changed = TRUE;
    }

  if (*error == NULL)
    {
      for (gint i = 0; auxinputnames[i]; i++)
        {
          GeglNode   *src_node;
          GeglBuffer *buffer;

          if (! gegl_node_has_pad (node, auxinputnames[i]))
            {
              g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                           _("GEGL operation '%s' has been called with an "
                             "invalid aux input name '%s'."),
                           opname, auxinputnames[i]);
              break;
            }


          buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (auxinputs[i]));
          g_object_ref (buffer);
          src_node = gegl_node_new_child (gegl_node_get_parent (node),
                                          "operation", "gegl:buffer-source",
                                          "buffer",    buffer,
                                          NULL);
          g_object_unref (buffer);

          gegl_node_connect (src_node, "output", node, auxinputnames[i]);
        }
    }

  if (settings)
    gegl_node_set (node, "config", settings, NULL);

  g_object_thaw_notify (G_OBJECT (filter));

  g_clear_object (&settings);
  g_free (pspecs);
  g_free (opname);

  if (changed && gimp_drawable_filter_is_active (filter))
    gimp_drawable_filter_update_drawable (filter, NULL);

  return (*error != NULL);
}

gboolean
gimp_drawable_filter_get_add_alpha (GimpDrawableFilter *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);

  return filter->add_alpha;
}

void
gimp_drawable_filter_set_add_alpha (GimpDrawableFilter *filter,
                                    gboolean            add_alpha)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (add_alpha != filter->add_alpha)
    {
      filter->add_alpha = add_alpha;
      gimp_drawable_filter_sync_format (filter);
    }
}

void
gimp_drawable_filter_set_override_constraints (GimpDrawableFilter *filter,
                                               gboolean            override_constraints)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (override_constraints != filter->override_constraints)
    {
      filter->override_constraints = override_constraints;

      gimp_drawable_filter_sync_affect (filter);
      gimp_drawable_filter_sync_format (filter);
      gimp_drawable_filter_sync_clip   (filter, TRUE);

      if (gimp_drawable_filter_is_active (filter))
        gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

const Babl *
gimp_drawable_filter_get_format (GimpDrawableFilter *filter)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), NULL);

  format = gimp_applicator_get_output_format (filter->applicator);

  if (! format)
    format = gimp_drawable_get_format (filter->drawable);

  return format;
}

void
gimp_drawable_filter_apply (GimpDrawableFilter  *filter,
                            const GeglRectangle *area)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  gimp_drawable_filter_add_filter (filter);

  gimp_drawable_filter_sync_clip (filter, TRUE);

  if (gimp_drawable_filter_is_active (filter))
    {
      gimp_drawable_update_bounding_box (filter->drawable);

      gimp_drawable_filter_update_drawable (filter, area);
    }
}

void
gimp_drawable_filter_apply_with_mask (GimpDrawableFilter  *filter,
                                      GimpChannel         *mask,
                                      const GeglRectangle *area)
{
  GimpImage   *image     = NULL;
  GimpChannel *selection = NULL;
  GeglBuffer  *buffer    = NULL;

  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (mask)
    {
      image = gimp_item_get_image (GIMP_ITEM (filter->drawable));
      selection =
        GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (gimp_image_get_mask (image)),
                                           GIMP_TYPE_CHANNEL));
      gimp_channel_clear (gimp_image_get_mask (image), NULL, FALSE);

      buffer =
        gimp_gegl_buffer_dup (gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)));
      gimp_drawable_set_buffer (GIMP_DRAWABLE (gimp_image_get_mask (image)),
                                FALSE, NULL, buffer);

      g_object_unref (buffer);
    }

  gimp_drawable_filter_apply (filter, area);

  if (mask)
    {
      buffer =
        gimp_gegl_buffer_dup (gimp_drawable_get_buffer (GIMP_DRAWABLE (selection)));
      gimp_drawable_set_buffer (GIMP_DRAWABLE (gimp_image_get_mask (image)),
                                FALSE, NULL, buffer);
      g_object_unref (buffer);
      g_object_unref (selection);
    }
}

gboolean
gimp_drawable_filter_commit (GimpDrawableFilter *filter,
                             gboolean            non_destructive,
                             GimpProgress       *progress,
                             gboolean            cancellable)
{
  gboolean success = TRUE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  if (gimp_drawable_filter_is_added (filter))
    {
      const Babl *format;

      g_object_add_weak_pointer (G_OBJECT (filter), (gpointer) &filter);
      format = gimp_drawable_filter_get_format (filter);

      gimp_drawable_filter_set_preview_split (filter, FALSE,
                                              filter->preview_split_alignment,
                                              filter->preview_split_position);
      gimp_drawable_filter_set_preview (filter, TRUE);

      /* Only commit if filter is applied destructively */
      if (! non_destructive)
        {
          success = gimp_drawable_merge_filter (filter->drawable,
                                                GIMP_FILTER (filter),
                                                progress,
                                                gimp_object_get_name (filter),
                                                format,
                                                filter->filter_clip,
                                                cancellable,
                                                FALSE);

          gimp_drawable_filter_remove_filter (filter);
        }
      else
        {
          GeglRectangle rect;

          if (gimp_viewable_preview_is_frozen (GIMP_VIEWABLE (filter->drawable)))
            gimp_viewable_preview_thaw (GIMP_VIEWABLE (filter->drawable));

          /* Update layer tree preview */
          rect = gimp_drawable_get_bounding_box (filter->drawable);
          gimp_drawable_update (filter->drawable, rect.x, rect.y,
                                rect.width, rect.height);
        }

      if (filter)
        {
          if (! success)
            gimp_drawable_filter_update_drawable (filter, NULL);

          g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
          g_object_remove_weak_pointer (G_OBJECT (filter), (gpointer) &filter);
        }
    }

  return success;
}

void
gimp_drawable_filter_abort (GimpDrawableFilter *filter)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (gimp_drawable_filter_remove_filter (filter))
    {
      gimp_drawable_filter_update_drawable (filter, NULL);
    }
}

void
gimp_drawable_filter_layer_mask_freeze (GimpDrawableFilter *filter)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));

  if (! filter->mask)
    g_object_set (filter,
                  "mask", GIMP_DRAWABLE (gimp_image_get_mask (image)),
                  NULL);

  g_signal_handlers_disconnect_by_func (image,
                                        gimp_drawable_filter_mask_changed,
                                        filter);
}

void gimp_drawable_filter_refresh_crop (GimpDrawableFilter *filter,
                                        GeglRectangle      *rect)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_FILTER (filter));

  if (rect)
    {
      /* Some filters have built-in width/height properties that limit
       * the buffer size. If they have one in those roles, we can update
       * their size here. */
      GParamSpec *gegl_pspec_width  = NULL;
      GParamSpec *gegl_pspec_height = NULL;

      gegl_pspec_width  = gegl_node_find_property (filter->operation, "width");
      gegl_pspec_height = gegl_node_find_property (filter->operation, "height");

      if (gegl_pspec_width != NULL)
        {
          if (gimp_gegl_param_spec_has_key (gegl_pspec_width,
                                            "role",
                                            "output-extent"))
            {
              gegl_node_set (filter->operation, "width", rect->width, NULL);
              filter->filter_area.width  = rect->width;
            }
        }
      if (gegl_pspec_height != NULL)
        {
          if (gimp_gegl_param_spec_has_key (gegl_pspec_height,
                                            "role",
                                            "output-extent"))
            {
              gegl_node_set (filter->operation, "height", rect->height, NULL);
              filter->filter_area.height = rect->height;
            }
        }

      gimp_drawable_filter_set_clip (filter, TRUE);
      gimp_drawable_filter_set_clip (filter, FALSE);
      gimp_drawable_filter_set_region (filter, GIMP_FILTER_REGION_SELECTION);
      gimp_drawable_filter_set_region (filter, GIMP_FILTER_REGION_DRAWABLE);
      gimp_drawable_filter_set_crop (filter, NULL, FALSE);
      gimp_drawable_filter_set_crop (filter, rect, FALSE);
    }
}

/*  private functions  */

static void
gimp_drawable_filter_sync_active (GimpDrawableFilter *filter)
{
  gimp_applicator_set_active (filter->applicator, filter->preview_enabled);
}

static void
gimp_drawable_filter_sync_clip (GimpDrawableFilter *filter,
                                gboolean            sync_region)
{
  gboolean clip;

  if (filter->override_constraints)
    clip = filter->clip;
  else
    clip = gimp_item_get_clip (GIMP_ITEM (filter->drawable), filter->clip);

  if (! clip)
    {
      GeglRectangle bounding_box;

      bounding_box = gegl_node_get_bounding_box (filter->operation);

      if (gegl_rectangle_is_infinite_plane (&bounding_box))
        clip = TRUE;
    }

  if (clip != filter->filter_clip)
    {
      filter->filter_clip = clip;

      if (sync_region)
        gimp_drawable_filter_sync_region (filter);
    }
}

static void
gimp_drawable_filter_sync_region (GimpDrawableFilter *filter)
{
  GimpContainer *filters;
  gboolean       first_filter = FALSE;

  filters = gimp_drawable_get_filters (filter->drawable);

  /* The first test is because the filter might not be added yet. */
  if (GIMP_LIST (filters)->queue->tail != NULL &&
      filter == GIMP_LIST (filters)->queue->tail->data)
    {
      GimpDrawableFilter *next_filter = NULL;

      if (GIMP_LIST (filters)->queue->head->next)
        next_filter = GIMP_LIST (filters)->queue->tail->prev->data;

      if (next_filter)
        /* If the current filter became the first after a reorder, we
         * want to re-sync the next filter which was the first filter
         * just before.
         */
        gimp_drawable_filter_sync_region (next_filter);

      first_filter = TRUE;
    }

  if (filter->region == GIMP_FILTER_REGION_SELECTION)
    {
      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) -filter->filter_area.x,
                         "y", (gdouble) -filter->filter_area.y,
                         NULL);

          if (first_filter)
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:crop",
                           "width",     (gdouble) filter->filter_area.width,
                           "height",    (gdouble) filter->filter_area.height,
                           NULL);
          else
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:nop",
                           NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         0.0,
                         "y",         0.0,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      gimp_applicator_set_apply_offset (filter->applicator,
                                        filter->filter_area.x,
                                        filter->filter_area.y);
    }
  else
    {
      GimpItem *item   = GIMP_ITEM (filter->drawable);
      gdouble   width  = gimp_item_get_width (item);
      gdouble   height = gimp_item_get_height (item);

      if (filter->has_input)
        {
          gegl_node_set (filter->translate,
                         "x", (gdouble) 0.0,
                         "y", (gdouble) 0.0,
                         NULL);

          if (first_filter)
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:crop",
                           "width",     width,
                           "height",    height,
                           NULL);
          else
            gegl_node_set (filter->crop_before,
                           "operation", "gegl:nop",
                           NULL);
        }

      if (filter->filter_clip)
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:crop",
                         "x",         (gdouble) filter->filter_area.x,
                         "y",         (gdouble) filter->filter_area.y,
                         "width",     (gdouble) filter->filter_area.width,
                         "height",    (gdouble) filter->filter_area.height,
                         NULL);
        }
      else
        {
          gegl_node_set (filter->crop_after,
                         "operation", "gegl:nop",
                         NULL);
        }

      gimp_applicator_set_apply_offset (filter->applicator, 0, 0);
    }

  if (gimp_drawable_filter_is_active (filter))
    {
      if (gimp_drawable_update_bounding_box (filter->drawable))
        g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static gboolean
gimp_drawable_filter_get_crop_rect (GimpDrawableFilter  *filter,
                                    gboolean             crop_enabled,
                                    const GeglRectangle *crop_rect,
                                    gboolean             preview_split_enabled,
                                    GimpAlignmentType    preview_split_alignment,
                                    gint                 preview_split_position,
                                    GeglRectangle       *rect)
{
  GeglRectangle bounds;
  gint          x1, x2;
  gint          y1, y2;

  bounds = gegl_rectangle_infinite_plane ();

  x1 = bounds.x;
  x2 = bounds.x + bounds.width;

  y1 = bounds.y;
  y2 = bounds.y + bounds.height;

  if (preview_split_enabled)
    {
      switch (preview_split_alignment)
        {
        case GIMP_ALIGN_LEFT:
          x2 = preview_split_position;
          break;

        case GIMP_ALIGN_RIGHT:
          x1 = preview_split_position;
          break;

        case GIMP_ALIGN_TOP:
          y2 = preview_split_position;
          break;

        case GIMP_ALIGN_BOTTOM:
          y1 = preview_split_position;
          break;

        default:
          g_return_val_if_reached (FALSE);
        }
    }

  gegl_rectangle_set (rect, x1, y1, x2 - x1, y2 - y1);

  if (crop_enabled)
    gegl_rectangle_intersect (rect, rect, crop_rect);

  return ! gegl_rectangle_equal (rect, &bounds);
}

static void
gimp_drawable_filter_sync_crop (GimpDrawableFilter  *filter,
                                gboolean             old_crop_enabled,
                                const GeglRectangle *old_crop_rect,
                                gboolean             old_preview_split_enabled,
                                GimpAlignmentType    old_preview_split_alignment,
                                gint                 old_preview_split_position,
                                gboolean             update)
{
  GeglRectangle old_rect;
  GeglRectangle new_rect;
  gboolean      enabled;

  gimp_drawable_filter_get_crop_rect (filter,
                                      old_crop_enabled,
                                      old_crop_rect,
                                      old_preview_split_enabled,
                                      old_preview_split_alignment,
                                      old_preview_split_position,
                                      &old_rect);

  enabled = gimp_drawable_filter_get_crop_rect (filter,
                                                filter->crop_enabled,
                                                &filter->crop_rect,
                                                filter->preview_split_enabled,
                                                filter->preview_split_alignment,
                                                filter->preview_split_position,
                                                &new_rect);

  gimp_applicator_set_crop (filter->applicator, enabled ? &new_rect : NULL);

  if (update                                  &&
      gimp_drawable_filter_is_active (filter) &&
      ! gegl_rectangle_equal (&old_rect, &new_rect))
    {
      GeglRectangle diff_rects[4];
      gint          n_diff_rects;
      gint          i;

      gimp_drawable_update_bounding_box (filter->drawable);

      n_diff_rects = gegl_rectangle_xor (diff_rects, &old_rect, &new_rect);

      for (i = 0; i < n_diff_rects; i++)
        gimp_drawable_filter_update_drawable (filter, &diff_rects[i]);
    }
}

static void
gimp_drawable_filter_sync_opacity (GimpDrawableFilter *filter)
{
  gimp_applicator_set_opacity (filter->applicator,
                               filter->opacity);
}

static void
gimp_drawable_filter_sync_mode (GimpDrawableFilter *filter)
{
  GimpLayerMode paint_mode = filter->paint_mode;

  if (! filter->has_input && paint_mode == GIMP_LAYER_MODE_REPLACE)
    {
      /* if the filter's op has no input, use NORMAL instead of REPLACE, so
       * that we composite the op's output on top of the input, instead of
       * completely replacing it.
       */
      paint_mode = GIMP_LAYER_MODE_NORMAL;
    }

  gimp_applicator_set_mode (filter->applicator,
                            paint_mode,
                            filter->blend_space,
                            filter->composite_space,
                            filter->composite_mode);
}

static void
gimp_drawable_filter_sync_affect (GimpDrawableFilter *filter)
{
  gimp_applicator_set_affect (
    filter->applicator,
    filter->override_constraints ?

      GIMP_COMPONENT_MASK_RED   |
      GIMP_COMPONENT_MASK_GREEN |
      GIMP_COMPONENT_MASK_BLUE  |
      GIMP_COMPONENT_MASK_ALPHA :

      gimp_drawable_get_active_mask (filter->drawable));
}

static void
gimp_drawable_filter_sync_format (GimpDrawableFilter *filter)
{
  const Babl *format = NULL;
  gboolean    changed;

  /* We only convert back to drawable format when the filter is planned
   * to be merged, to simulate how it would look like once it happens.
   *
   * On the other hand, when a filter is meant to stay on the stack,
   * non-destructively, the output might be higher bit depth and there
   * is no reason to demote it back.
   *
   * XXX: actually we might want to do this after the last layer mode
   * node, no? Otherwise the display render might be better in some case
   * than when the whole image is actually flattened into a single
   * buffer. But maybe that's what some people would want?
   */
  if (filter->to_be_merged)
    {
      if (filter->add_alpha                                &&
          (gimp_drawable_supports_alpha (filter->drawable) ||
           filter->override_constraints))
        format = gimp_drawable_get_format_with_alpha (filter->drawable);
      else
        format = gimp_drawable_get_format (filter->drawable);
    }

  changed = gimp_applicator_set_output_format (filter->applicator, format);

  if (changed && gimp_drawable_filter_is_active (filter))
    gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_sync_mask (GimpDrawableFilter *filter)
{
  GimpImage   *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));
  GimpChannel *mask  = NULL;

  if (! filter->mask)
    mask = gimp_image_get_mask (image);
  else
    mask = GIMP_CHANNEL (filter->mask);

  if (! mask || gimp_channel_is_empty (mask))
    {
      gimp_applicator_set_mask_buffer (filter->applicator, NULL);

      gimp_item_mask_intersect (GIMP_ITEM (filter->drawable),
                                &filter->filter_area.x,
                                &filter->filter_area.y,
                                &filter->filter_area.width,
                                &filter->filter_area.height);
    }
  else
    {
      GeglBuffer *mask_buffer;
      gint        offset_x, offset_y;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));
      gimp_item_get_offset (GIMP_ITEM (filter->drawable),
                            &offset_x, &offset_y);

      gimp_applicator_set_mask_buffer (filter->applicator, mask_buffer);
      gimp_applicator_set_mask_offset (filter->applicator,
                                       -offset_x, -offset_y);

      /* Update filter crop */
      filter->filter_area.x      = mask->x1;
      filter->filter_area.y      = mask->y1;
      filter->filter_area.width  = mask->x2 - mask->x1;
      filter->filter_area.height = mask->y2 - mask->y1;

      gimp_item_mask_intersect (GIMP_ITEM (filter->drawable),
                                &filter->filter_area.x,
                                &filter->filter_area.y,
                                &filter->filter_area.width,
                                &filter->filter_area.height);

      gimp_drawable_filter_sync_region (filter);
    }
}

static gboolean
gimp_drawable_filter_is_added (GimpDrawableFilter *filter)
{
  return gimp_drawable_has_filter (filter->drawable,
                                   GIMP_FILTER (filter));
}

static gboolean
gimp_drawable_filter_is_active (GimpDrawableFilter *filter)
{
  return gimp_drawable_filter_is_added (filter) &&
         filter->preview_enabled;
}

static gboolean
gimp_drawable_filter_add_filter (GimpDrawableFilter *filter)
{
  if (! gimp_drawable_filter_is_added (filter))
    {
      GimpImage     *image = gimp_item_get_image (GIMP_ITEM (filter->drawable));
      GimpContainer *filters;

      gimp_viewable_preview_freeze (GIMP_VIEWABLE (filter->drawable));

      gimp_drawable_filter_sync_active (filter);
      gimp_drawable_filter_sync_mask (filter);
      gimp_drawable_filter_sync_clip (filter, FALSE);
      gimp_drawable_filter_sync_region (filter);
      gimp_drawable_filter_sync_crop (filter,
                                      filter->crop_enabled,
                                      &filter->crop_rect,
                                      filter->preview_split_enabled,
                                      filter->preview_split_alignment,
                                      filter->preview_split_position,
                                      TRUE);
      gimp_drawable_filter_sync_opacity (filter);
      gimp_drawable_filter_sync_mode (filter);
      gimp_drawable_filter_sync_affect (filter);

      gimp_drawable_add_filter (filter->drawable,
                                GIMP_FILTER (filter));
      gimp_drawable_filter_sync_format (filter);

      gimp_drawable_update_bounding_box (filter->drawable);

      g_signal_connect (image, "component-active-changed",
                        G_CALLBACK (gimp_drawable_filter_affect_changed),
                        filter);
      if (! filter->mask)
        g_signal_connect_object (image, "mask-changed",
                                 G_CALLBACK (gimp_drawable_filter_mask_changed),
                                 filter, 0);
      g_signal_connect_object (filter->drawable, "lock-position-changed",
                               G_CALLBACK (gimp_drawable_filter_lock_position_changed),
                               filter, 0);
      g_signal_connect_object (filter->drawable, "format-changed",
                               G_CALLBACK (gimp_drawable_filter_format_changed),
                               filter, 0);
      g_signal_connect_object (filter->drawable, "removed",
                               G_CALLBACK (gimp_drawable_filter_drawable_removed),
                               filter, 0);

      if (GIMP_IS_LAYER (filter->drawable))
        {
          g_signal_connect_object (filter->drawable, "lock-alpha-changed",
                                   G_CALLBACK (gimp_drawable_filter_lock_alpha_changed),
                                   filter, 0);
        }

      filters = gimp_drawable_get_filters (filter->drawable);
      g_signal_connect_object (G_OBJECT (filters), "reorder",
                               G_CALLBACK (gimp_drawable_filter_reorder),
                               filter, 0);

      g_signal_connect_object (G_OBJECT (filter->operation), "notify",
                               G_CALLBACK (gimp_drawable_filters_changed),
                               filter->drawable, G_CONNECT_SWAPPED);
      g_signal_connect_object (G_OBJECT (filter), "active-changed",
                               G_CALLBACK (gimp_drawable_filters_changed),
                               filter->drawable, G_CONNECT_SWAPPED);

      return TRUE;
    }

  return FALSE;
}

static gboolean
gimp_drawable_filter_remove_filter (GimpDrawableFilter *filter)
{
  if (gimp_drawable_filter_is_added (filter))
    {
      GimpImage     *image    = gimp_item_get_image (GIMP_ITEM (filter->drawable));
      GimpDrawable  *drawable = filter->drawable;
      GimpContainer *filters;

      g_signal_handlers_disconnect_by_func (G_OBJECT (filter),
                                            G_CALLBACK (gimp_drawable_filters_changed),
                                            filter->drawable);

      filters = gimp_drawable_get_filters (filter->drawable);
      g_signal_handlers_disconnect_by_func (filters,
                                            G_CALLBACK (gimp_drawable_filter_reorder),
                                            filter);

      if (GIMP_IS_LAYER (drawable))
        g_signal_handlers_disconnect_by_func (drawable,
                                              gimp_drawable_filter_lock_alpha_changed,
                                              filter);

      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_filter_drawable_removed,
                                            filter);
      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_filter_format_changed,
                                            filter);
      g_signal_handlers_disconnect_by_func (drawable,
                                            gimp_drawable_filter_lock_position_changed,
                                            filter);

      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_filter_affect_changed,
                                            filter);

      gimp_drawable_remove_filter (drawable, GIMP_FILTER (filter));

      gimp_drawable_update_bounding_box (drawable);

      if (gimp_viewable_preview_is_frozen (GIMP_VIEWABLE (drawable)))
        gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));

      return TRUE;
    }

  return FALSE;
}

static void
gimp_drawable_filter_update_drawable (GimpDrawableFilter  *filter,
                                      const GeglRectangle *area)
{
  GeglRectangle bounding_box;
  GeglRectangle update_area;

  bounding_box = gimp_drawable_get_bounding_box (filter->drawable);

  if (area)
    {
      if (! gegl_rectangle_intersect (&update_area,
                                      area, &bounding_box))
        {
          return;
        }
    }
  else
    {
      gimp_drawable_filter_get_crop_rect (filter,
                                          filter->crop_enabled,
                                          &filter->crop_rect,
                                          filter->preview_split_enabled,
                                          filter->preview_split_alignment,
                                          filter->preview_split_position,
                                          &update_area);

      if (! gegl_rectangle_intersect (&update_area,
                                      &update_area, &bounding_box))
        {
          return;
        }
    }

  if (update_area.width  > 0 &&
      update_area.height > 0)
    {
      gimp_drawable_update (filter->drawable,
                            update_area.x,
                            update_area.y,
                            update_area.width,
                            update_area.height);

      g_signal_emit (filter, drawable_filter_signals[FLUSH], 0);
    }
}

static void
gimp_drawable_filter_affect_changed (GimpImage          *image,
                                     GimpChannelType     channel,
                                     GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_affect (filter);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_mask_changed (GimpImage          *image,
                                   GimpDrawableFilter *filter)
{
  if (! filter->mask)
    {
      gimp_drawable_filter_update_drawable (filter, NULL);

      gimp_drawable_filter_sync_mask (filter);
      gimp_drawable_filter_sync_clip (filter, FALSE);
      gimp_drawable_filter_sync_region (filter);

      gimp_drawable_filter_update_drawable (filter, NULL);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (image,
                                            gimp_drawable_filter_mask_changed,
                                            filter);
    }
}

static void
gimp_drawable_filter_lock_position_changed (GimpDrawable       *drawable,
                                            GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_clip (filter, TRUE);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_format_changed (GimpDrawable       *drawable,
                                     GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_format (filter);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_drawable_removed (GimpDrawable       *drawable,
                                       GimpDrawableFilter *filter)
{
  if (filter)
    gimp_drawable_filter_remove_filter (filter);
}

static void
gimp_drawable_filter_lock_alpha_changed (GimpLayer          *layer,
                                         GimpDrawableFilter *filter)
{
  gimp_drawable_filter_sync_affect (filter);
  gimp_drawable_filter_update_drawable (filter, NULL);
}

static void
gimp_drawable_filter_reorder (GimpFilterStack    *stack,
                              GimpDrawableFilter *reordered_filter,
                              gint                old_index,
                              gint                new_index,
                              GimpDrawableFilter *filter)
{
  if (reordered_filter == filter)
    {
      gimp_drawable_filter_sync_format (filter);

      g_return_if_fail (GIMP_LIST (stack)->queue->head != NULL);

      if (GIMP_LIST (stack)->queue->head->data != filter &&
          /* When there is a floating selection, there will be a
           * GimpFilter (but not a drawable filter) in the stack.
           * XXX For now, let's fix the crash (#12851) but eventually we
           * really want to clean up this list and understand better how
           * it's organized.
           */
          GIMP_IS_DRAWABLE_FILTER (GIMP_LIST (stack)->queue->head->data))
        {
          gimp_drawable_filter_sync_format (GIMP_LIST (stack)->queue->head->data);
        }
    }
}
