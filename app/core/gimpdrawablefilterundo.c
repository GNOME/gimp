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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimp-gegl-utils.h"

#include "gimpchannel.h"
#include "gimpcontainer.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawablefilter.h"
#include "gimpdrawablefilterundo.h"
#include "gimpimage.h"
#include "gimpitem.h"


enum
{
  PROP_0,
  PROP_FILTER,
};


static void    gimp_drawable_filter_undo_constructed  (GObject             *object);
static void    gimp_drawable_filter_undo_set_property (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void    gimp_drawable_filter_undo_get_property (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);
static gint64  gimp_drawable_filter_undo_get_memsize  (GimpObject          *object,
                                                       gint64              *gui_size);
static void    gimp_drawable_filter_undo_pop          (GimpUndo            *undo,
                                                       GimpUndoMode         undo_mode,
                                                       GimpUndoAccumulator *accum);
static void    gimp_drawable_filter_undo_free         (GimpUndo            *undo,
                                                       GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpDrawableFilterUndo, gimp_drawable_filter_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_drawable_filter_undo_parent_class


static void
gimp_drawable_filter_undo_class_init (GimpDrawableFilterUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpUndoClass   *undo_class        = GIMP_UNDO_CLASS (klass);

  object_class->constructed          = gimp_drawable_filter_undo_constructed;
  object_class->set_property         = gimp_drawable_filter_undo_set_property;
  object_class->get_property         = gimp_drawable_filter_undo_get_property;

  gimp_object_class->get_memsize     = gimp_drawable_filter_undo_get_memsize;

  undo_class->pop                    = gimp_drawable_filter_undo_pop;
  undo_class->free                   = gimp_drawable_filter_undo_free;

  g_object_class_install_property (object_class, PROP_FILTER,
                                   g_param_spec_object ("filter", NULL, NULL,
                                                        GIMP_TYPE_DRAWABLE_FILTER,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_drawable_filter_undo_init (GimpDrawableFilterUndo *undo)
{
}

static void
gimp_drawable_filter_undo_constructed (GObject *object)
{
  GimpDrawableFilterUndo *df_undo = GIMP_DRAWABLE_FILTER_UNDO (object);
  GimpDrawable           *drawable;
  GimpContainer          *filter_stack;
  GeglNode               *op;
  gchar                  *op_name;
  GParamSpec             **pspecs;
  guint                    n_pspecs;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_DRAWABLE_FILTER (df_undo->filter));

  drawable = gimp_drawable_filter_get_drawable (df_undo->filter);
  if (drawable)
    {
      filter_stack = gimp_drawable_get_filters (drawable);

      df_undo->row_index =
        gimp_container_get_child_index (filter_stack,
                                        GIMP_OBJECT (df_undo->filter));
    }

  op = gimp_drawable_filter_get_operation (df_undo->filter);
  gegl_node_get (op, "operation", &op_name, NULL);

  df_undo->node = gegl_node_new ();
  gegl_node_set (df_undo->node,"operation", op_name, NULL);

  pspecs = gegl_operation_list_properties (op_name, &n_pspecs);
  for (gint i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, pspec->value_type);
      gegl_node_get_property (op, pspec->name,
                              &value);

      gegl_node_set_property (df_undo->node, pspec->name,
                              &value);
      g_value_unset (&value);
    }

  df_undo->opacity         = gimp_drawable_filter_get_opacity (df_undo->filter);
  df_undo->paint_mode      = gimp_drawable_filter_get_paint_mode (df_undo->filter);
  df_undo->blend_space     = gimp_drawable_filter_get_blend_space (df_undo->filter);
  df_undo->composite_space = gimp_drawable_filter_get_composite_space (df_undo->filter);
  df_undo->composite_mode  = gimp_drawable_filter_get_composite_mode (df_undo->filter);
  df_undo->region          = gimp_drawable_filter_get_region (df_undo->filter);

  g_free (pspecs);
  g_free (op_name);
}

static void
gimp_drawable_filter_undo_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  GimpDrawableFilterUndo *drawable_filter_undo = GIMP_DRAWABLE_FILTER_UNDO (object);

  switch (property_id)
    {
    case PROP_FILTER:
      drawable_filter_undo->filter = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_filter_undo_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  GimpDrawableFilterUndo *drawable_filter_undo = GIMP_DRAWABLE_FILTER_UNDO (object);

  switch (property_id)
    {
    case PROP_FILTER:
      g_value_set_object (value, drawable_filter_undo->filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_drawable_filter_undo_get_memsize (GimpObject *object,
                                       gint64     *gui_size)
{
  GimpDrawableFilterUndo *drawable_filter_undo = GIMP_DRAWABLE_FILTER_UNDO (object);
  gint64                  memsize              = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (drawable_filter_undo->filter),
                                      NULL);
  memsize += sizeof (guint32);     /* Row Index */
  memsize += sizeof (gdouble);     /* Opacity */
  memsize += sizeof (guint32) * 5; /* Composite and Region options */

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
gimp_drawable_filter_undo_pop (GimpUndo            *undo,
                               GimpUndoMode         undo_mode,
                               GimpUndoAccumulator *accum)
{
  GimpDrawableFilterUndo *df_undo      = GIMP_DRAWABLE_FILTER_UNDO (undo);
  GimpDrawableFilter     *filter       = GIMP_DRAWABLE_FILTER (df_undo->filter);
  GimpDrawable           *drawable     = gimp_drawable_filter_get_drawable (filter);
  GimpContainer          *filter_stack = gimp_drawable_get_filters (drawable);
  GimpImage              *image        = NULL;
  GimpChannel            *selection    = NULL;

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  if (drawable)
    image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (image)
    {
      /* Temporarily remove selection so filters can be applied correctly */
      selection =
        GIMP_CHANNEL (gimp_item_duplicate (GIMP_ITEM (gimp_image_get_mask (image)),
                                           GIMP_TYPE_CHANNEL));
      gimp_channel_clear (gimp_image_get_mask (image), NULL, FALSE);
    }

  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_FILTER_ADD) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_FILTER_REMOVE) )
    {
      if (drawable)
        {
          gimp_drawable_remove_filter (drawable, GIMP_FILTER (filter));

          gimp_item_refresh_filters (GIMP_ITEM (drawable));
        }
    }
  if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
       undo->undo_type == GIMP_UNDO_FILTER_REMOVE) ||
      (undo_mode       == GIMP_UNDO_MODE_REDO &&
       undo->undo_type == GIMP_UNDO_FILTER_ADD) )
    {
      if (drawable)
        {
          gimp_drawable_filter_apply (filter, NULL);
          gimp_container_reorder (filter_stack, GIMP_OBJECT (filter),
                                  df_undo->row_index);

          if (gimp_viewable_preview_is_frozen (GIMP_VIEWABLE (drawable)))
            gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
        }
    }
  else if (undo->undo_type == GIMP_UNDO_FILTER_REORDER)
    {
      gimp_container_reorder (filter_stack, GIMP_OBJECT (filter),
                              df_undo->row_index);
      gimp_drawable_filter_apply (filter, NULL);
    }
  else if (undo->undo_type == GIMP_UNDO_FILTER_MODIFIED)
    {
      GeglNode                *op;
      GParamSpec             **pspecs;
      guint                    n_pspecs;
      gchar                   *op_name = NULL;
      gdouble                  opacity;
      GimpLayerMode            paint_mode;
      GimpLayerColorSpace      blend_space;
      GimpLayerColorSpace      composite_space;
      GimpLayerCompositeMode   composite_mode;
      GimpFilterRegion         region;

      /* Swap values */
      opacity         = gimp_drawable_filter_get_opacity (filter);
      paint_mode      = gimp_drawable_filter_get_paint_mode (filter);
      blend_space     = gimp_drawable_filter_get_blend_space (filter);
      composite_space = gimp_drawable_filter_get_composite_space (filter);
      composite_mode  = gimp_drawable_filter_get_composite_mode (filter);
      region          = gimp_drawable_filter_get_region (filter);

      gimp_drawable_filter_set_opacity (filter, df_undo->opacity);
      gimp_drawable_filter_set_mode (filter, df_undo->paint_mode,
                                     df_undo->blend_space,
                                     df_undo->composite_space,
                                     df_undo->composite_mode);
      gimp_drawable_filter_set_region (filter, df_undo->region);

      df_undo->opacity         = opacity;
      df_undo->paint_mode      = paint_mode;
      df_undo->blend_space     = blend_space;
      df_undo->composite_space = composite_space;
      df_undo->composite_mode  = composite_mode;
      df_undo->region          = region;

      op = gimp_drawable_filter_get_operation (df_undo->filter);
      gegl_node_get (op, "operation", &op_name, NULL);

      pspecs = gegl_operation_list_properties (op_name, &n_pspecs);

      for (gint i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec  = pspecs[i];
          GValue      value1 = G_VALUE_INIT;
          GValue      value2 = G_VALUE_INIT;

          g_value_init (&value1, pspec->value_type);
          g_value_init (&value2, pspec->value_type);

          gegl_node_get_property (op, pspec->name, &value1);

          gegl_node_get_property (df_undo->node, pspec->name, &value2);
          gegl_node_set_property (op, pspec->name, &value2);

          gegl_node_set_property (df_undo->node, pspec->name, &value1);

          g_value_unset (&value1);
          g_value_unset (&value2);
        }

      g_free (pspecs);
      g_free (op_name);

      gimp_drawable_filter_apply (filter, NULL);
    }

  if (image)
    {
      GeglBuffer *buffer = NULL;

      /* Restore selection after undo or redo */
      buffer = gimp_gegl_buffer_dup (gimp_drawable_get_buffer (GIMP_DRAWABLE (selection)));
      gimp_drawable_set_buffer (GIMP_DRAWABLE (gimp_image_get_mask (image)),
                                FALSE, NULL, buffer);
      g_object_unref (buffer);
      g_object_unref (selection);
    }
}

static void
gimp_drawable_filter_undo_free (GimpUndo     *undo,
                                GimpUndoMode  undo_mode)
{
  GimpDrawableFilterUndo *drawable_filter_undo = GIMP_DRAWABLE_FILTER_UNDO (undo);

  if (drawable_filter_undo->filter)
    g_clear_object (&drawable_filter_undo->filter);

  if (drawable_filter_undo->node)
    g_object_unref (drawable_filter_undo->node);

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
