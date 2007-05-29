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

#include <stdlib.h>
#include <string.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-parasites.h"
#include "gimpdrawable.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpitem.h"
#include "gimpitem-preview.h"
#include "gimplist.h"
#include "gimpmarshal.h"
#include "gimpparasitelist.h"
#include "gimpprogress.h"
#include "gimpstrokedesc.h"

#include "gimp-intl.h"


enum
{
  REMOVED,
  VISIBILITY_CHANGED,
  LINKED_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT
};


/*  local function prototypes  */

static void       gimp_item_set_property      (GObject       *object,
                                               guint          property_id,
                                               const GValue  *value,
                                               GParamSpec    *pspec);
static void       gimp_item_get_property      (GObject       *object,
                                               guint          property_id,
                                               GValue        *value,
                                               GParamSpec    *pspec);
static void       gimp_item_finalize          (GObject       *object);

static gint64     gimp_item_get_memsize       (GimpObject    *object,
                                               gint64        *gui_size);

static GimpItem * gimp_item_real_duplicate    (GimpItem      *item,
                                               GType          new_type,
                                               gboolean       add_alpha);
static void       gimp_item_real_convert      (GimpItem      *item,
                                               GimpImage     *dest_image);
static gboolean   gimp_item_real_rename       (GimpItem      *item,
                                               const gchar   *new_name,
                                               const gchar   *undo_desc);
static void       gimp_item_real_translate    (GimpItem      *item,
                                               gint           offset_x,
                                               gint           offset_y,
                                               gboolean       push_undo);
static void       gimp_item_real_scale        (GimpItem      *item,
                                               gint           new_width,
                                               gint           new_height,
                                               gint           new_offset_x,
                                               gint           new_offset_y,
                                               GimpInterpolationType  interpolation,
                                               GimpProgress  *progress);
static void       gimp_item_real_resize       (GimpItem      *item,
                                               GimpContext   *context,
                                               gint           new_width,
                                               gint           new_height,
                                               gint           offset_x,
                                               gint           offset_y);


G_DEFINE_TYPE (GimpItem, gimp_item, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_item_parent_class

static guint gimp_item_signals[LAST_SIGNAL] = { 0 };


static void
gimp_item_class_init (GimpItemClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);

  gimp_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, removed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_item_signals[VISIBILITY_CHANGED] =
    g_signal_new ("visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, visibility_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_item_signals[LINKED_CHANGED] =
    g_signal_new ("linked-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpItemClass, linked_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->set_property       = gimp_item_set_property;
  object_class->get_property       = gimp_item_get_property;
  object_class->finalize           = gimp_item_finalize;

  gimp_object_class->get_memsize   = gimp_item_get_memsize;

  viewable_class->get_preview_size = gimp_item_get_preview_size;
  viewable_class->get_popup_size   = gimp_item_get_popup_size;

  klass->removed                   = NULL;
  klass->visibility_changed        = NULL;
  klass->linked_changed            = NULL;

  klass->is_attached               = NULL;
  klass->duplicate                 = gimp_item_real_duplicate;
  klass->convert                   = gimp_item_real_convert;
  klass->rename                    = gimp_item_real_rename;
  klass->translate                 = gimp_item_real_translate;
  klass->scale                     = gimp_item_real_scale;
  klass->resize                    = gimp_item_real_resize;
  klass->flip                      = NULL;
  klass->rotate                    = NULL;
  klass->transform                 = NULL;
  klass->stroke                    = NULL;

  klass->default_name              = NULL;
  klass->rename_desc               = NULL;
  klass->translate_desc            = NULL;
  klass->scale_desc                = NULL;
  klass->resize_desc               = NULL;
  klass->flip_desc                 = NULL;
  klass->rotate_desc               = NULL;
  klass->transform_desc            = NULL;

  g_object_class_install_property (object_class, PROP_ID,
                                   g_param_spec_int ("id", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_WIDTH,
                                   g_param_spec_int ("width", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", NULL, NULL,
                                                     1, GIMP_MAX_IMAGE_SIZE, 1,
                                                     GIMP_PARAM_READABLE));
}

static void
gimp_item_init (GimpItem *item)
{
  g_object_force_floating (G_OBJECT (item));

  item->ID        = 0;
  item->tattoo    = 0;
  item->image     = NULL;
  item->parasites = gimp_parasite_list_new ();
  item->width     = 0;
  item->height    = 0;
  item->offset_x  = 0;
  item->offset_y  = 0;
  item->visible   = TRUE;
  item->linked    = FALSE;
  item->removed   = FALSE;
}

static void
gimp_item_set_property (GObject      *object,
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
gimp_item_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  GimpItem *item = GIMP_ITEM (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, item->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, item->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, item->height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_item_finalize (GObject *object)
{
  GimpItem *item = GIMP_ITEM (object);

  if (item->image && item->image->gimp)
    {
      g_hash_table_remove (item->image->gimp->item_table,
                           GINT_TO_POINTER (item->ID));
      item->image = NULL;
    }

  if (item->parasites)
    {
      g_object_unref (item->parasites);
      item->parasites = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_item_get_memsize (GimpObject *object,
                       gint64     *gui_size)
{
  GimpItem *item    = GIMP_ITEM (object);
  gint64    memsize = 0;

  memsize += gimp_object_get_memsize (GIMP_OBJECT (item->parasites), gui_size);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static GimpItem *
gimp_item_real_duplicate (GimpItem *item,
                          GType     new_type,
                          gboolean  add_alpha)
{
  GimpItem *new_item;
  gchar    *new_name;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (item->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  /*  formulate the new name  */
  {
    const gchar *name;
    gchar       *ext;
    gint         number;
    gint         len;

    name = gimp_object_get_name (GIMP_OBJECT (item));

    g_return_val_if_fail (name != NULL, NULL);

    ext = strrchr (name, '#');
    len = strlen (_("copy"));

    if ((strlen (name) >= len &&
         strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
        (ext && (number = atoi (ext + 1)) > 0 &&
         ((int)(log10 (number) + 1)) == strlen (ext + 1)))
      {
        /* don't have redundant "copy"s */
        new_name = g_strdup (name);
      }
    else
      {
        new_name = g_strdup_printf (_("%s copy"), name);
      }
  }

  new_item = g_object_new (new_type, NULL);

  gimp_item_configure (new_item, gimp_item_get_image (item),
                       item->offset_x, item->offset_y,
                       item->width, item->height,
                       new_name);

  g_free (new_name);

  g_object_unref (new_item->parasites);
  new_item->parasites = gimp_parasite_list_copy (item->parasites);

  new_item->visible = item->visible;
  new_item->linked  = item->linked;

  return new_item;
}

static void
gimp_item_real_convert (GimpItem  *item,
                        GimpImage *dest_image)
{
  gimp_item_set_image (item, dest_image);
}

static gboolean
gimp_item_real_rename (GimpItem    *item,
                       const gchar *new_name,
                       const gchar *undo_desc)
{
  if (gimp_item_is_attached (item))
    gimp_image_undo_push_item_rename (item->image, undo_desc, item);

  gimp_object_set_name (GIMP_OBJECT (item), new_name);

  return TRUE;
}

static void
gimp_item_real_translate (GimpItem *item,
                          gint      offset_x,
                          gint      offset_y,
                          gboolean  push_undo)
{
  item->offset_x += offset_x;
  item->offset_y += offset_y;
}

static void
gimp_item_real_scale (GimpItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      GimpInterpolationType  interpolation,
                      GimpProgress          *progress)
{
  item->width    = new_width;
  item->height   = new_height;
  item->offset_x = new_offset_x;
  item->offset_y = new_offset_y;

  g_object_notify (G_OBJECT (item), "width");
  g_object_notify (G_OBJECT (item), "height");
}

static void
gimp_item_real_resize (GimpItem    *item,
                       GimpContext *context,
                       gint         new_width,
                       gint         new_height,
                       gint         offset_x,
                       gint         offset_y)
{
  item->offset_x = item->offset_x - offset_x;
  item->offset_y = item->offset_y - offset_y;
  item->width    = new_width;
  item->height   = new_height;

  g_object_notify (G_OBJECT (item), "width");
  g_object_notify (G_OBJECT (item), "height");
}

/**
 * gimp_item_remove:
 * @item: the #GimpItem to remove.
 *
 * This function sets the 'removed' flag on @item to #TRUE, and emits
 * a 'removed' signal on the item.
 */
void
gimp_item_removed (GimpItem *item)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  item->removed = TRUE;

  g_signal_emit (item, gimp_item_signals[REMOVED], 0);
}

/**
 * gimp_item_is_removed:
 * @item: the #GimpItem to check.
 *
 * Returns #TRUE if the 'removed' flag is set for @item, and
 * #FALSE otherwise.
 */
gboolean
gimp_item_is_removed (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return item->removed;
}

/**
 * gimp_item_configure:
 * @item:     The #GimpItem to configure.
 * @image:    The #GimpImage to which the item belongs.
 * @offset_x: The X offset to assign the item.
 * @offset_y: The Y offset to assign the item.
 * @width:    The width to assign the item.
 * @height:   The height to assign the item.
 * @name:     The name to assign the item.
 *
 * This function is used to configure a new item.  First, if the item
 * does not already have an ID, it is assigned the next available
 * one, and then inserted into the Item Hash Table.  Next, it is
 * given basic item properties as specified by the arguments.
 */
void
gimp_item_configure (GimpItem    *item,
                     GimpImage   *image,
                     gint         offset_x,
                     gint         offset_y,
                     gint         width,
                     gint         height,
                     const gchar *name)
{
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_IMAGE (image));

  g_object_freeze_notify (G_OBJECT (item));

  if (item->ID == 0)
    {
      do
        {
          item->ID = image->gimp->next_item_ID++;

          if (image->gimp->next_item_ID == G_MAXINT)
            image->gimp->next_item_ID = 1;
        }
      while (g_hash_table_lookup (image->gimp->item_table,
                                  GINT_TO_POINTER (item->ID)));

      g_hash_table_insert (image->gimp->item_table,
                           GINT_TO_POINTER (item->ID),
                           item);

      gimp_item_set_image (item, image);

      g_object_notify (G_OBJECT (item), "id");
    }

  item->width    = width;
  item->height   = height;
  item->offset_x = offset_x;
  item->offset_y = offset_y;

  g_object_notify (G_OBJECT (item), "width");
  g_object_notify (G_OBJECT (item), "height");

  if (name)
    gimp_object_set_name (GIMP_OBJECT (item), name);
  else
    gimp_object_set_static_name (GIMP_OBJECT (item), _("Unnamed"));

  g_object_thaw_notify (G_OBJECT (item));
}

/**
 * gimp_item_is_attached:
 * @item: The #GimpItem to check.
 *
 * Returns: %TRUE if the item is attached to an image, %FALSE otherwise.
 */
gboolean
gimp_item_is_attached (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return GIMP_ITEM_GET_CLASS (item)->is_attached (item);
}

/**
 * gimp_item_duplicate:
 * @item:      The #GimpItem to duplicate.
 * @new_type:  The type to make the new item.
 * @add_alpha: #TRUE if an alpha channel should be added to the new item.
 *
 * Returns: the newly created item.
 */
GimpItem *
gimp_item_duplicate (GimpItem *item,
                     GType     new_type,
                     gboolean  add_alpha)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (item->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  return GIMP_ITEM_GET_CLASS (item)->duplicate (item, new_type, add_alpha);
}

/**
 * gimp_item_convert:
 * @item:       The #GimpItem to convert.
 * @dest_image: The #GimpImage in which to place the converted item.
 * @new_type:   The type to convert the item to.
 * @add_alpha:  #TRUE if an alpha channel should be added to the converted item.
 *
 * Returns: the new item that results from the conversion.
 */
GimpItem *
gimp_item_convert (GimpItem  *item,
                   GimpImage *dest_image,
                   GType      new_type,
                   gboolean   add_alpha)
{
  GimpItem *new_item;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (item->image), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_ITEM), NULL);

  new_item = gimp_item_duplicate (item, new_type, add_alpha);

  if (new_item)
    GIMP_ITEM_GET_CLASS (new_item)->convert (new_item, dest_image);

  return new_item;
}

/**
 * gimp_item_rename:
 * @item:     The #GimpItem to rename.
 * @new_name: The new name to give the item.
 *
 * This function assigns a new name to the item, if the desired name is
 * different from the name it already has, and pushes an entry onto the
 * undo stack for the item's image.  If @new_name is NULL or empty, the
 * default name for the item's class is used.  If the name is changed,
 * the "name_changed" signal is emitted for the item.
 *
 * Returns: %TRUE if the @item could be renamed, %FALSE otherwise.
 */
gboolean
gimp_item_rename (GimpItem    *item,
                  const gchar *new_name)
{
  GimpItemClass *item_class;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  item_class = GIMP_ITEM_GET_CLASS (item);

  if (! new_name || ! *new_name)
    new_name = item_class->default_name;

  if (strcmp (new_name, gimp_object_get_name (GIMP_OBJECT (item))))
    return item_class->rename (item, new_name, item_class->rename_desc);

  return TRUE;
}

/**
 * gimp_item_width:
 * @item: The #GimpItem to check.
 *
 * Returns: The width of the item.
 */
gint
gimp_item_width (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return item->width;
}

/**
 * gimp_item_height:
 * @item: The #GimpItem to check.
 *
 * Returns: The height of the item.
 */
gint
gimp_item_height (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return item->height;
}

/**
 * gimp_item_offsets:
 * @item:     The #GimpItem to check.
 * @offset_x: Return location for the item's X offset.
 * @offset_y: Return location for the item's Y offset.
 *
 * Reveals the X and Y offsets of the item.
 */
void
gimp_item_offsets (const GimpItem *item,
                   gint           *offset_x,
                   gint           *offset_y)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  if (offset_x) *offset_x = item->offset_x;
  if (offset_y) *offset_y = item->offset_y;
}

/**
 * gimp_item_translate:
 * @item:      The #GimpItem to move.
 * @offset_x:  Increment to the X offset of the item.
 * @offset_y:  Increment to the Y offset of the item.
 * @push_undo: If #TRUE, create an entry in the image's undo stack
 *             for this action.
 *
 * Adds the specified increments to the X and Y offsets for the item.
 */
void
gimp_item_translate (GimpItem *item,
                     gint      offset_x,
                     gint      offset_y,
                     gboolean  push_undo)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_DISPLACE,
                                 item_class->translate_desc);

  item_class->translate (item, offset_x, offset_y, push_undo);

  if (push_undo)
    gimp_image_undo_group_end (image);
}

/**
 * gimp_item_check_scaling:
 * @item:       Item to check
 * @new_width:  proposed width of item, in pixels
 * @new_height: proposed height of item, in pixels
 *
 * Scales item dimensions, then snaps them to pixel centers
 *
 * Returns: #FALSE if any dimension reduces to zero as a result
 *          of this; otherwise, returns #TRUE.
 **/
gboolean
gimp_item_check_scaling (const GimpItem *item,
                         gint            new_width,
                         gint            new_height)
{
  GimpImage *image;
  gdouble    img_scale_w;
  gdouble    img_scale_h;
  gint       new_item_width;
  gint       new_item_height;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  image = gimp_item_get_image (item);

  img_scale_w     = (gdouble) new_width  / (gdouble) image->width;
  img_scale_h     = (gdouble) new_height / (gdouble) image->height;
  new_item_width  = ROUND (img_scale_w * (gdouble) item->width);
  new_item_height = ROUND (img_scale_h * (gdouble) item->height);

  return (new_item_width > 0 && new_item_height > 0);
}

void
gimp_item_scale (GimpItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 GimpInterpolationType  interpolation,
                 GimpProgress          *progress)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_SCALE,
                                 item_class->scale_desc);

  item_class->scale (item, new_width, new_height, new_offset_x, new_offset_y,
                     interpolation, progress);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_end (image);
}

/**
 * gimp_item_scale_by_factors:
 * @item:     Item to be transformed by explicit width and height factors.
 * @w_factor: scale factor to apply to width and horizontal offset
 * @h_factor: scale factor to apply to height and vertical offset
 * @interpolation:
 * @progress:
 *
 * Scales item dimensions and offsets by uniform width and
 * height factors.
 *
 * Use gimp_item_scale_by_factors() in circumstances when the same
 * width and height scaling factors are to be uniformly applied to a
 * set of items. In this context, the item's dimensions and offsets
 * from the sides of the containing image all change by these
 * predetermined factors. By fiat, the fixed point of the transform is
 * the upper left hand corner of the image. Returns #FALSE if a
 * requested scale factor is zero or if a scaling zero's out a item
 * dimension; returns #TRUE otherwise.
 *
 * Use gimp_item_scale() in circumstances where new item width
 * and height dimensions are predetermined instead.
 *
 * Side effects: Undo set created for item. Old item imagery
 *               scaled & painted to new item tiles.
 *
 * Returns: #TRUE, if the scaled item has positive dimensions
 *          #FALSE if the scaled item has at least one zero dimension
 **/
gboolean
gimp_item_scale_by_factors (GimpItem              *item,
                            gdouble                w_factor,
                            gdouble                h_factor,
                            GimpInterpolationType  interpolation,
                            GimpProgress          *progress)
{
  gint new_width, new_height;
  gint new_offset_x, new_offset_y;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  if (w_factor == 0.0 || h_factor == 0.0)
    {
      g_warning ("%s: requested width or height scale equals zero", G_STRFUNC);
      return FALSE;
    }

  new_offset_x = ROUND (w_factor * (gdouble) item->offset_x);
  new_offset_y = ROUND (h_factor * (gdouble) item->offset_y);
  new_width    = ROUND (w_factor * (gdouble) item->width);
  new_height   = ROUND (h_factor * (gdouble) item->height);

  if (new_width != 0 && new_height != 0)
    {
      gimp_item_scale (item,
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation, progress);
      return TRUE;
    }

  return FALSE;
}

/**
 * gimp_item_scale_by_origin:
 * @item:         The item to be transformed by width & height scale factors
 * @new_width:    The width that item will acquire
 * @new_height:   The height that the item will acquire
 * @interpolation:
 * @progress:
 * @local_origin: sets fixed point of the scaling transform. See below.
 *
 * Sets item dimensions to new_width and
 * new_height. Derives vertical and horizontal scaling
 * transforms from new width and height. If local_origin is
 * #TRUE, the fixed point of the scaling transform coincides
 * with the item's center point.  Otherwise, the fixed
 * point is taken to be [-item->offset_x, -item->offset_y].
 *
 * Since this function derives scale factors from new and
 * current item dimensions, these factors will vary from
 * item to item because of aliasing artifacts; factor
 * variations among items can be quite large where item
 * dimensions approach pixel dimensions. Use
 * gimp_item_scale_by_factors() where constant scales are to
 * be uniformly applied to a number of items.
 *
 * Side effects: undo set created for item.
 *               Old item imagery scaled
 *               & painted to new item tiles
 **/
void
gimp_item_scale_by_origin (GimpItem              *item,
                           gint                   new_width,
                           gint                   new_height,
                           GimpInterpolationType  interpolation,
                           GimpProgress          *progress,
                           gboolean               local_origin)
{
  gint new_offset_x, new_offset_y;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (new_width == 0 || new_height == 0)
    {
      g_warning ("%s: requested width or height equals zero", G_STRFUNC);
      return;
    }

  if (local_origin)
    {
      new_offset_x = item->offset_x + ((item->width  - new_width)  / 2.0);
      new_offset_y = item->offset_y + ((item->height - new_height) / 2.0);
    }
  else
    {
      new_offset_x = (gint) (((gdouble) new_width *
                              (gdouble) item->offset_x /
                              (gdouble) item->width));

      new_offset_y = (gint) (((gdouble) new_height *
                              (gdouble) item->offset_y /
                              (gdouble) item->height));
    }

  gimp_item_scale (item,
                   new_width, new_height,
                   new_offset_x, new_offset_y,
                   interpolation, progress);
}

void
gimp_item_resize (GimpItem    *item,
                  GimpContext *context,
                  gint         new_width,
                  gint         new_height,
                  gint         offset_x,
                  gint         offset_y)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_ITEM_RESIZE,
                                 item_class->resize_desc);

  item_class->resize (item, context, new_width, new_height, offset_x, offset_y);

  if (gimp_item_is_attached (item))
    gimp_image_undo_group_end (image);
}

void
gimp_item_flip (GimpItem            *item,
                GimpContext         *context,
                GimpOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->flip_desc);

  item_class->flip (item, context, flip_type, axis, clip_result);

  gimp_image_undo_group_end (image);
}

void
gimp_item_rotate (GimpItem         *item,
                  GimpContext      *context,
                  GimpRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->rotate_desc);

  item_class->rotate (item, context, rotate_type, center_x, center_y,
                      clip_result);

  gimp_image_undo_group_end (image);
}

void
gimp_item_transform (GimpItem               *item,
                     GimpContext            *context,
                     const GimpMatrix3      *matrix,
                     GimpTransformDirection  direction,
                     GimpInterpolationType   interpolation,
                     gboolean                supersample,
                     gint                    recursion_level,
                     GimpTransformResize     clip_result,
                     GimpProgress           *progress)
{
  GimpItemClass *item_class;
  GimpImage     *image;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (gimp_item_is_attached (item));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  item_class = GIMP_ITEM_GET_CLASS (item);
  image = gimp_item_get_image (item);

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_TRANSFORM,
                               item_class->transform_desc);

  item_class->transform (item, context, matrix, direction, interpolation,
                         supersample, recursion_level,
                         clip_result, progress);

  gimp_image_undo_group_end (image);
}

gboolean
gimp_item_stroke (GimpItem       *item,
                  GimpDrawable   *drawable,
                  GimpContext    *context,
                  GimpStrokeDesc *stroke_desc,
                  gboolean        use_default_values,
                  GimpProgress   *progress)
{
  GimpItemClass *item_class;
  gboolean       retval = FALSE;

  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (item), FALSE);
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), FALSE);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (GIMP_IS_STROKE_DESC (stroke_desc), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  item_class = GIMP_ITEM_GET_CLASS (item);

  if (item_class->stroke)
    {
      GimpImage *image = gimp_item_get_image (item);

      gimp_stroke_desc_prepare (stroke_desc, context, use_default_values);

      gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_PAINT,
                                   item_class->stroke_desc);

      retval = item_class->stroke (item, drawable, stroke_desc, progress);

      gimp_image_undo_group_end (image);

      gimp_stroke_desc_finish (stroke_desc);
    }

  return retval;
}

gint
gimp_item_get_ID (GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), -1);

  return item->ID;
}

GimpItem *
gimp_item_get_by_ID (Gimp *gimp,
                     gint  item_id)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  if (gimp->item_table == NULL)
    return NULL;

  return (GimpItem *) g_hash_table_lookup (gimp->item_table,
                                           GINT_TO_POINTER (item_id));
}

GimpTattoo
gimp_item_get_tattoo (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), 0);

  return item->tattoo;
}

void
gimp_item_set_tattoo (GimpItem   *item,
                      GimpTattoo  tattoo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  item->tattoo = tattoo;
}

GimpImage *
gimp_item_get_image (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return item->image;
}

void
gimp_item_set_image (GimpItem  *item,
                     GimpImage *image)
{
  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (! gimp_item_is_attached (item));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));

  if (image == NULL)
    {
      item->tattoo = 0;
    }
  else if (item->tattoo == 0 || item->image != image)
    {
      item->tattoo = gimp_image_get_new_tattoo (image);
    }

  item->image = image;
}

void
gimp_item_parasite_attach (GimpItem           *item,
                           const GimpParasite *parasite)
{
  GimpParasite  copy;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (parasite != NULL);

  /*  make a temporary copy of the GimpParasite struct because
   *  gimp_parasite_shift_parent() changes it
   */
  copy = *parasite;

  if (gimp_item_is_attached (item))
    {
      /*  only set the dirty bit manually if we can be saved and the new
       *  parasite differs from the current one and we aren't undoable
       */
      if (gimp_parasite_is_undoable (&copy))
        {
          /* do a group in case we have attach_parent set */
          gimp_image_undo_group_start (item->image,
                                       GIMP_UNDO_GROUP_PARASITE_ATTACH,
                                       _("Attach Parasite"));

          gimp_image_undo_push_item_parasite (item->image, NULL, item, &copy);
        }
      else if (gimp_parasite_is_persistent (&copy) &&
               ! gimp_parasite_compare (&copy,
                                        gimp_item_parasite_find
                                        (item, gimp_parasite_name (&copy))))
        {
          gimp_image_undo_push_cantundo (item->image,
                                         _("Attach Parasite to Item"));
        }
    }

  gimp_parasite_list_add (item->parasites, &copy);

  if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_PARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_image_parasite_attach (item->image, &copy);
    }
  else if (gimp_parasite_has_flag (&copy, GIMP_PARASITE_ATTACH_GRANDPARENT))
    {
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_shift_parent (&copy);
      gimp_parasite_attach (item->image->gimp, &copy);
    }

  if (gimp_item_is_attached (item) &&
      gimp_parasite_is_undoable (&copy))
    {
      gimp_image_undo_group_end (item->image);
    }
}

void
gimp_item_parasite_detach (GimpItem    *item,
                           const gchar *name)
{
  const GimpParasite *parasite;

  g_return_if_fail (GIMP_IS_ITEM (item));
  g_return_if_fail (name != NULL);

  parasite = gimp_parasite_list_find (item->parasites, name);

  if (! parasite)
    return;

  if (gimp_parasite_is_undoable (parasite))
    {
      gimp_image_undo_push_item_parasite_remove (item->image,
                                                 _("Remove Parasite from Item"),
                                                 item,
                                                 gimp_parasite_name (parasite));
    }
  else if (gimp_parasite_is_persistent (parasite))
    {
      gimp_image_undo_push_cantundo (item->image,
                                     _("Remove Parasite from Item"));
    }

  gimp_parasite_list_remove (item->parasites, name);
}

const GimpParasite *
gimp_item_parasite_find (const GimpItem *item,
                         const gchar    *name)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);

  return gimp_parasite_list_find (item->parasites, name);
}

static void
gimp_item_parasite_list_foreach_func (gchar          *name,
                                      GimpParasite   *parasite,
                                      gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (name);
}

gchar **
gimp_item_parasite_list (const GimpItem *item,
                         gint           *count)
{
  gchar **list;
  gchar **cur;

  g_return_val_if_fail (GIMP_IS_ITEM (item), NULL);
  g_return_val_if_fail (count != NULL, NULL);

  *count = gimp_parasite_list_length (item->parasites);

  cur = list = g_new (gchar *, *count);

  gimp_parasite_list_foreach (item->parasites,
                              (GHFunc) gimp_item_parasite_list_foreach_func,
                              &cur);

  return list;
}

gboolean
gimp_item_get_visible (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return item->visible;
}

void
gimp_item_set_visible (GimpItem *item,
                       gboolean  visible,
                       gboolean  push_undo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  if (item->visible != visible)
    {
      if (push_undo && gimp_item_is_attached (item))
        {
          GimpImage *image = gimp_item_get_image (item);

          if (image)
            gimp_image_undo_push_item_visibility (image, NULL, item);
        }

      item->visible = visible ? TRUE : FALSE;

      g_signal_emit (item, gimp_item_signals[VISIBILITY_CHANGED], 0);
    }
}

void
gimp_item_set_linked (GimpItem *item,
                      gboolean  linked,
                      gboolean  push_undo)
{
  g_return_if_fail (GIMP_IS_ITEM (item));

  if (item->linked != linked)
    {
      if (push_undo && gimp_item_is_attached (item))
        {
          GimpImage *image = gimp_item_get_image (item);

          if (image)
            gimp_image_undo_push_item_linked (image, NULL, item);
        }

      item->linked = linked ? TRUE : FALSE;

      g_signal_emit (item, gimp_item_signals[LINKED_CHANGED], 0);
    }
}

gboolean
gimp_item_get_linked (const GimpItem *item)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  return item->linked;
}

gboolean
gimp_item_is_in_set (GimpItem    *item,
                     GimpItemSet  set)
{
  g_return_val_if_fail (GIMP_IS_ITEM (item), FALSE);

  switch (set)
    {
    case GIMP_ITEM_SET_NONE:
      return FALSE;

    case GIMP_ITEM_SET_ALL:
      return TRUE;

    case GIMP_ITEM_SET_IMAGE_SIZED:
      return (item->width  == item->image->width &&
              item->height == item->image->height);

    case GIMP_ITEM_SET_VISIBLE:
      return gimp_item_get_visible (item);

    case GIMP_ITEM_SET_LINKED:
      return gimp_item_get_linked (item);
    }

  return FALSE;
}
