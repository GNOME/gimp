/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "vectors/gimpvectors.h"

#include "gimplist.h"
#include "gimpimage.h"
#include "gimpimage-arrange.h"
#include "gimpimage-guides.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimpchannel.h"
#include "gimpdrawable.h"
#include "gimpguide.h"

#include "gimp-intl.h"

static GList * sort_by_offset        (GList             *list);
static GList * list_of_linked_items  (GimpImage         *image);
static void    compute_offsets       (GList             *list,
                                      GimpAlignmentType  alignment);
static void    compute_offset        (GObject           *object,
                                      GimpAlignmentType  alignment);
static gint    align_offset          (GObject           *object,
                                      GimpAlignmentType  alignment);
static gint    offset_compare        (gconstpointer      a,
                                      gconstpointer      b);
static void    gimp_image_align_list (GimpImage         *image,
                                      GList             *list,
                                      GimpAlignmentType  alignment,
                                      GObject           *reference,
                                      GimpAlignmentType  reference_alignment,
                                      gint               offset);

/**
 * gimp_image_distribute_linked_items:
 * @image:                The #GimpImage to which the items belong.
 * @alignment:            The point on each item to use for alignment.
 *
 * This function shifts the positions of the set of items (layer, paths, etc) that
 * are currently linked, to bring them into a specified type of alignment with a
 * reference object, which can be an item, guide, or image.  If the requested
 * alignment does not make sense, nothing happens and no error message is generated.
 *
 */
void
gimp_image_distribute_linked_items (GimpImage         *image,
                                    GimpAlignmentType  alignment)
{
  GList    *list;
  gboolean  do_x               = FALSE;
  gboolean  do_y               = FALSE;
  GList    *sorted_list;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  list = list_of_linked_items (image);

  /* get offsets used for sorting */
  switch (alignment)
    {
      /* order vertically for horizontal distribution */
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_HCENTER:
    case GIMP_ALIGN_RIGHT:
      do_x = TRUE;
      compute_offsets (list, alignment);
      break;
      /* order horizontally for vertical distribution */
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_VCENTER:
    case GIMP_ALIGN_BOTTOM:
      do_y = TRUE;
      compute_offsets (list, alignment);
      break;
    }

  sorted_list = sort_by_offset (list);
  g_list_free (list);

  if (sorted_list)
    {
      GList *l;
      gint   n;
      gint   n_items;
      gint   z0;
      gint   z;

      z0 = GPOINTER_TO_INT (g_object_get_data (sorted_list->data,
                                               "align-offset"));
      z = GPOINTER_TO_INT (g_object_get_data (g_list_last (sorted_list)->data,
                                              "align-offset"));
      n_items = g_list_length (sorted_list);

      if (n_items > 1)
        {
          gint offset = (z - z0) / (n_items - 1);

          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                       _("Distribute items"));

          for (l = sorted_list, n = 0; l; l = g_list_next (l), n++)
            {
              GObject *target          = G_OBJECT (l->data);
              gint     xtranslate      = 0;
              gint     ytranslate      = 0;
              gint     z1;

              z1 = GPOINTER_TO_INT (g_object_get_data (target,
                                                       "align-offset"));

              if (do_x)
                xtranslate = z0 - z1 + n * offset;

              if (do_y)
                ytranslate = z0 - z1 + n * offset;

              gimp_item_translate (GIMP_ITEM (target),
                                   xtranslate, ytranslate, TRUE);
            }
        }

      gimp_image_undo_group_end (image);
    }

  g_list_free (sorted_list);
}


/**
 * gimp_image_align_linked_items:
 * @image:                The #GimpImage to which the objects belong.
 * @alignment:            The point on each target object to bring into alignment.
 * @reference:            The #GObject to align the targets with, or #NULL.
 * @reference_alignment:  The point on the reference object to align the target item with..
 *
 * This function shifts the positions of the set of items (layer, paths, etc) that
 * are currently linked, to bring them into a specified type of alignment with a
 * reference object, which can be an item, guide, or image.  If the requested
 * alignment does not make sense, nothing happens and no error message is generated.
 *
 */
void
gimp_image_align_linked_items (GimpImage         *image,
                               GimpAlignmentType  alignment,
                               GObject           *reference)
{
  GList    *list;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (G_IS_OBJECT (reference) || reference == NULL);

  list = list_of_linked_items (image);

  if (list)
    {
      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                   _("Align Items"));

      gimp_image_align_list (image, list, alignment, reference, alignment, 0);

      gimp_image_undo_group_end (image);

      g_list_free (list);
    }
}

/*
 * generic alignment function.  only a part of the capabilites
 * are exploited so far.  Note that this function does not
 * enclose the operations within an undo group.
 */
static void
gimp_image_align_list (GimpImage         *image,
                       GList             *list,
                       GimpAlignmentType  alignment,
                       GObject           *reference,
                       GimpAlignmentType  reference_alignment,
                       gint               offset)
{
  GList    *l;
  gboolean  do_x   = FALSE;
  gboolean  do_y   = FALSE;
  gint      z0     = 0;

  switch (alignment)
    {
    case GIMP_ALIGN_LEFT:
    case GIMP_ALIGN_HCENTER:
    case GIMP_ALIGN_RIGHT:
      do_x = TRUE;
      break;
    case GIMP_ALIGN_TOP:
    case GIMP_ALIGN_VCENTER:
    case GIMP_ALIGN_BOTTOM:
      do_y = TRUE;
      break;
    }

  if (reference == NULL)
    reference = G_OBJECT (image);

  z0 = align_offset (reference, reference_alignment);

  for (l = list; l; l = g_list_next (l))
    {
      GObject *target          = G_OBJECT (l->data);
      gint     xtranslate      = 0;
      gint     ytranslate      = 0;
      gint     z1;

      z1 = align_offset (target, alignment);

      if (do_x)
        xtranslate = z0 - z1 + offset;

      if (do_y)
        ytranslate = z0 - z1 + offset;

      /* now actually align the target object */
      if (GIMP_IS_ITEM (target))
        {
          gimp_item_translate (GIMP_ITEM (target),
                               xtranslate, ytranslate, TRUE);
        }
      else if (GIMP_IS_GUIDE (target))
        {
          GimpGuide *guide = GIMP_GUIDE (target);

          switch (gimp_guide_get_orientation (guide))
            {
            case GIMP_ORIENTATION_VERTICAL:
              gimp_image_move_guide (image, guide, z1 + xtranslate, TRUE);
              gimp_image_update_guide (image, guide);
              break;

            case GIMP_ORIENTATION_HORIZONTAL:
              gimp_image_move_guide (image, guide, z1 + ytranslate, TRUE);
              gimp_image_update_guide (image, guide);
              break;

            default:
              break;
            }
        }
    }
}

static GList *
sort_by_offset (GList *list)
{
  return g_list_sort (g_list_copy (list),
                      offset_compare);
}

static gint
offset_compare (gconstpointer a,
                gconstpointer b)
{
  gint offset1 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (a),
                                                     "align-offset"));
  gint offset2 = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b),
                                                     "align-offset"));

  return offset1 - offset2;
}

/*
 * this function computes the position of the alignment point
 * for each object in the list, and attaches it to the
 * object as object data.
 */
static void
compute_offsets (GList             *list,
                 GimpAlignmentType  alignment)
{
  GList *l;

  for (l = list; l; l = g_list_next (l))
    compute_offset (G_OBJECT (l->data), alignment);
}

static void
compute_offset (GObject *object,
                GimpAlignmentType  alignment)
{
  gint offset = align_offset (object, alignment);

  g_object_set_data (object, "align-offset",
                     GINT_TO_POINTER (offset));
}

static gint
align_offset (GObject *object,
              GimpAlignmentType  alignment)
{
  gint object_offset_x = 0;
  gint object_offset_y = 0;
  gint object_height   = 0;
  gint object_width    = 0;
  gint offset          = 0;

  if (GIMP_IS_IMAGE (object))
    {
      GimpImage *image = GIMP_IMAGE (object);

      object_offset_x = 0;
      object_offset_y = 0;
      object_height   = gimp_image_get_height (image);
      object_width    = gimp_image_get_width (image);
    }
  else if (GIMP_IS_CHANNEL (object))
    {
      /* for channels, we use the bounds of the visible area, not
         the layer bounds.  This includes the selection channel */

      GimpChannel *channel = GIMP_CHANNEL (object);

      if (gimp_channel_is_empty (channel))
        {
          /* fall back on using the offsets instead */
          GimpItem *item = GIMP_ITEM (object);

          gimp_item_offsets (item, &object_offset_x, &object_offset_y);
          object_height   = gimp_item_height (item);
          object_width    = gimp_item_width (item);
        }
      else
        {
          gint x1, x2, y1, y2;

          gimp_channel_bounds (channel, &x1, &y1, &x2, &y2);
          object_offset_x = x1;
          object_offset_y = y1;
          object_height   = y2 - y1;
          object_width    = x2 - x1;
        }
    }
  else if (GIMP_IS_ITEM (object))
    {
      GimpItem *item = GIMP_ITEM (object);

      if (GIMP_IS_VECTORS (object))
        {
          gdouble x1_f, y1_f, x2_f, y2_f;

          gimp_vectors_bounds (GIMP_VECTORS (item),
                               &x1_f, &y1_f,
                               &x2_f, &y2_f);

          object_offset_x = ROUND (x1_f);
          object_offset_y = ROUND (y1_f);
          object_width    = ROUND (x2_f - x1_f);
          object_height   = ROUND (y2_f - y1_f);
        }
      else
        {
          gimp_item_offsets (item, &object_offset_x, &object_offset_y);
          object_height = gimp_item_height (item);
          object_width  = gimp_item_width (item);
        }
    }
  else if (GIMP_IS_GUIDE (object))
    {
      GimpGuide *guide = GIMP_GUIDE (object);

      switch (gimp_guide_get_orientation (guide))
        {
        case GIMP_ORIENTATION_VERTICAL:
          object_offset_x = gimp_guide_get_position (guide);
          object_width = 0;
          break;

        case GIMP_ORIENTATION_HORIZONTAL:
          object_offset_y = gimp_guide_get_position (guide);
          object_height = 0;
          break;

        default:
          break;
        }
    }
  else
    {
      g_printerr ("Alignment object is not an image, item or guide.\n");
    }

  switch (alignment)
    {
    case GIMP_ALIGN_LEFT:
      offset = object_offset_x;
      break;
    case GIMP_ALIGN_HCENTER:
      offset = object_offset_x + object_width/2;
      break;
    case GIMP_ALIGN_RIGHT:
      offset = object_offset_x + object_width;
      break;
    case GIMP_ALIGN_TOP:
      offset = object_offset_y;
      break;
    case GIMP_ALIGN_VCENTER:
      offset = object_offset_y + object_height/2;
      break;
    case GIMP_ALIGN_BOTTOM:
      offset = object_offset_y + object_height;
      break;
    default:
      g_assert_not_reached ();
    }

  return offset;
}

static GList *
list_of_linked_items (GimpImage *image)
{
  GList *list = NULL;
  GList *l;

  for (l = GIMP_LIST (image->layers)->list; l; l = g_list_next (l))
    {
      GimpItem *item = l->data;

      if (gimp_item_get_visible (item) && gimp_item_get_linked (item))
        list = g_list_append (list, item);
    }

  for (l = GIMP_LIST (image->channels)->list; l; l = g_list_next (l))
    {
      GimpItem *item = l->data;

      if (gimp_item_get_visible (item) && gimp_item_get_linked (item))
        list = g_list_append (list, item);
    }

  for (l = GIMP_LIST (image->vectors)->list; l; l = g_list_next (l))
    {
      GimpItem *item = l->data;

      if (gimp_item_get_visible (item) && gimp_item_get_linked (item))
        list = g_list_append (list, item);
    }

  return list;
}

