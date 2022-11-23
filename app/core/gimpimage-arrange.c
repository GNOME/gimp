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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligmaimage.h"
#include "ligmaimage-arrange.h"
#include "ligmaimage-guides.h"
#include "ligmaimage-undo.h"
#include "ligmaitem.h"
#include "ligmaguide.h"
#include "ligmapickable.h"
#include "ligmapickable-auto-shrink.h"

#include "ligma-intl.h"


static GList * sort_by_offset  (GList             *list);
static void    compute_offsets (GList             *list,
                                gboolean           use_x_offset,
                                gdouble            align_x,
                                gdouble            align_y,
                                gboolean           align_contents);
static void    compute_offset  (GObject           *object,
                                gboolean           use_x_offset,
                                gdouble            align_x,
                                gdouble            align_y,
                                gboolean           align_contents);
static gint    offset_compare  (gconstpointer      a,
                                gconstpointer      b);


/**
 * ligma_image_arrange_objects:
 * @image:                The #LigmaImage to which the objects belong.
 * @list:                 A #GList of objects to be aligned.
 * @align_x:              Relative X coordinate of point on each target object to bring into alignment.
 * @align_y:              Relative Y coordinate of point on each target object to bring into alignment.
 * @reference:            The #GObject to align the targets with, or %NULL.
 * @reference_alignment:  The point on the reference object to align the target item with..
 * @align_contents:       Take into account non-empty contents rather than item borders.
 *
 * This function shifts the positions of a set of target objects,
 * which can be "items" or guides, to bring them into a specified type
 * of alignment with a reference object, which can be an item, guide,
 * or image.  If the requested alignment does not make sense (i.e.,
 * trying to align a vertical guide vertically), nothing happens and
 * no error message is generated.
 *
 * The objects in the list are sorted into increasing order before
 * being arranged, where the order is defined by the type of alignment
 * being requested.  If the @reference argument is %NULL, then the
 * first object in the sorted list is used as reference.
 */
void
ligma_image_arrange_objects (LigmaImage         *image,
                            GList             *list,
                            gdouble            align_x,
                            gdouble            align_y,
                            GObject           *reference,
                            LigmaAlignmentType  reference_alignment,
                            gboolean           align_contents)
{
  gdouble  reference_x      = 0.0;
  gdouble  reference_y      = 0.0;
  gboolean use_ref_x_offset = FALSE;
  gboolean use_obj_x_offset = FALSE;
  gboolean do_x             = FALSE;
  gboolean do_y             = FALSE;
  gint     z0               = 0;
  GList   *object_list;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (G_IS_OBJECT (reference) || reference == NULL);

  /* get offsets used for sorting */
  switch (reference_alignment)
    {
      /* order vertically for horizontal alignment */
    case LIGMA_ALIGN_LEFT:
    case LIGMA_ALIGN_HCENTER:
    case LIGMA_ALIGN_RIGHT:
      if (LIGMA_IS_GUIDE (reference) &&
          ligma_guide_get_orientation (LIGMA_GUIDE (reference)) == LIGMA_ORIENTATION_HORIZONTAL)
        return;

      use_obj_x_offset = TRUE;

      use_ref_x_offset = TRUE;
      if (reference_alignment == LIGMA_ALIGN_HCENTER)
        reference_x = 0.5;
      else if (reference_alignment == LIGMA_ALIGN_RIGHT)
        reference_x = 1.0;

      do_x = TRUE;
      break;

      /* order horizontally for horizontal arrangement */
    case LIGMA_ARRANGE_HFILL:
      if (g_list_length (list) <= 2)
        return;
      use_obj_x_offset = TRUE;
      use_ref_x_offset = TRUE;
      do_x = TRUE;
      break;

      /* order horizontally for vertical alignment */
    case LIGMA_ALIGN_TOP:
    case LIGMA_ALIGN_VCENTER:
    case LIGMA_ALIGN_BOTTOM:
      if (LIGMA_IS_GUIDE (reference) &&
          ligma_guide_get_orientation (LIGMA_GUIDE (reference)) == LIGMA_ORIENTATION_VERTICAL)
        return;

      if (reference_alignment == LIGMA_ALIGN_VCENTER)
        reference_y = 0.5;
      else if (reference_alignment == LIGMA_ALIGN_BOTTOM)
        reference_y = 1.0;

      do_y = TRUE;
      break;

      /* order vertically for vertical arrangement */
    case LIGMA_ARRANGE_VFILL:
      if (g_list_length (list) <= 2)
        return;
      do_y = TRUE;
      break;

    case LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP:
      use_obj_x_offset = TRUE;
      do_x             = TRUE;
    case LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP:
      if (g_list_length (list) <= 2)
        return;

      if (! do_x)
        do_y = TRUE;

      /* Gap distribution does not use the anchor point. */
      align_x = align_y = 0.0;
      break;

    default:
      g_return_if_reached ();
    }
  /* get offsets used for sorting */
  compute_offsets (list, use_obj_x_offset, align_x, align_y, align_contents);

  /* Sort. */
  object_list = sort_by_offset (list);

  /* now get offsets used for aligning */
  compute_offsets (list, use_obj_x_offset, align_x, align_y, align_contents);

  if (object_list)
    {
      GList   *list;
      gint     n;
      gdouble  fill_offset = 0;

      if (reference_alignment == LIGMA_ARRANGE_HFILL                  ||
          reference_alignment == LIGMA_ARRANGE_VFILL                  ||
          reference_alignment == LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP ||
          reference_alignment == LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP)
        {
          /* Distribution does not use the reference. Extreme coordinate items
           * are used instead.
           */
          GList *last_object = g_list_last (object_list);
          gint   distr_length;

          z0 = GPOINTER_TO_INT (g_object_get_data (object_list->data, "align-offset"));
          distr_length = GPOINTER_TO_INT (g_object_get_data (last_object->data, "align-offset")) - z0;

          if (reference_alignment == LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP ||
              reference_alignment == LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP)
            {
              for (list = object_list; list && list->next; list = g_list_next (list))
                {
                  gint obj_len;

                  if (reference_alignment == LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP)
                    obj_len = GPOINTER_TO_INT (g_object_get_data (list->data, "align-width"));
                  else
                    obj_len = GPOINTER_TO_INT (g_object_get_data (list->data, "align-height"));
                  distr_length -= obj_len;

                  /* Initial offset is at end of first object. */
                  if (list == object_list)
                    z0 += obj_len;
                }
              fill_offset = distr_length / (gdouble) (g_list_length (object_list) - 1);
            }
          else
            {
              fill_offset = distr_length / (gdouble) (g_list_length (object_list) - 1);
            }

          /* Removing first and last objects. These stay unmoved. */
          object_list = g_list_delete_link (object_list, last_object);
          object_list = g_list_delete_link (object_list, object_list);
        }
      else
        {
          if (reference == NULL)
            {
              reference = G_OBJECT (object_list->data);
              object_list = g_list_delete_link (object_list, object_list);
            }

          /* Compute the offset for the reference (for alignment). */
          compute_offset (reference, use_ref_x_offset, reference_x, reference_y, FALSE);
          z0 = GPOINTER_TO_INT (g_object_get_data (reference, "align-offset"));
        }

      /* FIXME: undo group type is wrong */
      ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_ITEM_DISPLACE,
                                   C_("undo-type", "Arrange Objects"));

      for (list = object_list, n = 1;
           list;
           list = g_list_next (list), n++)
        {
          GObject *target     = list->data;
          gint     xtranslate = 0;
          gint     ytranslate = 0;
          gint     z1;

          z1 = GPOINTER_TO_INT (g_object_get_data (target, "align-offset"));

          if (reference_alignment == LIGMA_ARRANGE_HFILL)
            {
              xtranslate = ROUND (z0 - z1 + n * fill_offset);
            }
          else if (reference_alignment == LIGMA_ARRANGE_VFILL)
            {
              ytranslate =  ROUND (z0 - z1 + n * fill_offset);
            }
          else if (reference_alignment == LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP)
            {
              gint obj_len;

              xtranslate = ROUND (z0 + fill_offset - z1);

              obj_len = GPOINTER_TO_INT (g_object_get_data (target, "align-width"));
              z0 = z0 + fill_offset + obj_len;
            }
          else if (reference_alignment == LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP)
            {
              gint obj_len;

              ytranslate = ROUND (z0 + fill_offset - z1);

              obj_len = GPOINTER_TO_INT (g_object_get_data (target, "align-height"));
              z0 = z0 + fill_offset + obj_len;
            }
          else /* the normal computing, when we don't depend on the
                * width or height of the reference object
                */
            {
              if (do_x)
                xtranslate = z0 - z1;

              if (do_y)
                ytranslate = z0 - z1;
            }

          /* now actually align the target object */
          if (LIGMA_IS_ITEM (target))
            {
              ligma_item_translate (LIGMA_ITEM (target),
                                   xtranslate, ytranslate, TRUE);
            }
          else if (LIGMA_IS_GUIDE (target))
            {
              LigmaGuide *guide = LIGMA_GUIDE (target);

              switch (ligma_guide_get_orientation (guide))
                {
                case LIGMA_ORIENTATION_VERTICAL:
                  ligma_image_move_guide (image, guide, z1 + xtranslate, TRUE);
                  break;

                case LIGMA_ORIENTATION_HORIZONTAL:
                  ligma_image_move_guide (image, guide, z1 + ytranslate, TRUE);
                  break;

                default:
                  break;
                }
            }
        }

      ligma_image_undo_group_end (image);
    }

  g_list_free (object_list);
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

/* this function computes the position of the alignment point
 * for each object in the list, and attaches it to the
 * object as object data.
 */
static void
compute_offsets (GList    *list,
                 gboolean  use_x_offset,
                 gdouble   align_x,
                 gdouble   align_y,
                 gboolean  align_contents)
{
  GList *l;

  for (l = list; l; l = g_list_next (l))
    compute_offset (l->data, use_x_offset, align_x, align_y, align_contents);
}

static void
compute_offset (GObject  *object,
                gboolean  use_x_offset,
                gdouble   align_x,
                gdouble   align_y,
                gboolean  align_contents)
{
  gint object_offset_x = 0;
  gint object_offset_y = 0;
  gint object_height   = 0;
  gint object_width    = 0;
  gint offset          = 0;

  if (LIGMA_IS_IMAGE (object))
    {
      LigmaImage *image = LIGMA_IMAGE (object);

      object_offset_x = 0;
      object_offset_y = 0;
      object_height   = ligma_image_get_height (image);
      object_width    = ligma_image_get_width (image);
    }
  else if (LIGMA_IS_ITEM (object))
    {
      LigmaItem *item = LIGMA_ITEM (object);
      gint      off_x, off_y;

      ligma_item_bounds (item,
                        &object_offset_x,
                        &object_offset_y,
                        &object_width,
                        &object_height);

      if (align_contents && LIGMA_IS_PICKABLE (object))
        {
          gint x;
          gint y;
          gint width;
          gint height;

          if (ligma_pickable_auto_shrink (LIGMA_PICKABLE (object),
                                         0, 0,
                                         ligma_item_get_width  (LIGMA_ITEM (object)),
                                         ligma_item_get_height (LIGMA_ITEM (object)),
                                         &x, &y, &width, &height) == LIGMA_AUTO_SHRINK_SHRINK)
            {
              object_offset_x += x;
              object_offset_y += y;
              object_width     = width;
              object_height    = height;
            }
        }

      ligma_item_get_offset (item, &off_x, &off_y);

      object_offset_x += off_x;
      object_offset_y += off_y;
    }
  else if (LIGMA_IS_GUIDE (object))
    {
      LigmaGuide *guide = LIGMA_GUIDE (object);

      switch (ligma_guide_get_orientation (guide))
        {
        case LIGMA_ORIENTATION_VERTICAL:
          object_offset_x = ligma_guide_get_position (guide);
          object_width = 0;
          break;

        case LIGMA_ORIENTATION_HORIZONTAL:
          object_offset_y = ligma_guide_get_position (guide);
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

  if (use_x_offset)
    offset = object_offset_x + object_width * align_x;
  else
    offset = object_offset_y + object_height * align_y;

  g_object_set_data (object, "align-offset",
                     GINT_TO_POINTER (offset));

  /* These are only used for HFILL and VFILL, but since the call to
   * ligma_image_arrange_objects allows for two different alignments
   * (object and reference_alignment) we better be on the safe side in
   * case they differ.  (the current implementation of the align tool
   * always pass the same value to both parameters)
   */
  g_object_set_data (object, "align-width",
                     GINT_TO_POINTER (object_width));

  g_object_set_data (object, "align-height",
                     GINT_TO_POINTER (object_height));
}
