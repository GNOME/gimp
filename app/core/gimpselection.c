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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimpdrawable-edit.h"
#include "gimpdrawable-private.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-new.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimplayer.h"
#include "gimplayer-new.h"
#include "gimplayermask.h"
#include "gimplayer-floating-selection.h"
#include "gimppickable.h"
#include "gimpselection.h"

#include "gimp-intl.h"


static gboolean   gimp_selection_is_attached   (GimpItem            *item);
static GimpItemTree * gimp_selection_get_tree  (GimpItem            *item);
static void       gimp_selection_translate     (GimpItem            *item,
                                                gdouble              offset_x,
                                                gdouble              offset_y,
                                                gboolean             push_undo);
static void       gimp_selection_scale         (GimpItem            *item,
                                                gint                 new_width,
                                                gint                 new_height,
                                                gint                 new_offset_x,
                                                gint                 new_offset_y,
                                                GimpInterpolationType interp_type,
                                                GimpProgress        *progress);
static void       gimp_selection_resize        (GimpItem            *item,
                                                GimpContext         *context,
                                                GimpFillType         fill_type,
                                                gint                 new_width,
                                                gint                 new_height,
                                                gint                 offset_x,
                                                gint                 offset_y);
static void       gimp_selection_flip          (GimpItem            *item,
                                                GimpContext         *context,
                                                GimpOrientationType  flip_type,
                                                gdouble              axis,
                                                gboolean             clip_result);
static void       gimp_selection_rotate        (GimpItem            *item,
                                                GimpContext         *context,
                                                GimpRotationType     rotation_type,
                                                gdouble              center_x,
                                                gdouble              center_y,
                                                gboolean             clip_result);
static gboolean   gimp_selection_fill          (GimpItem            *item,
                                                GimpDrawable        *drawable,
                                                GimpFillOptions     *fill_options,
                                                gboolean             push_undo,
                                                GimpProgress        *progress,
                                                GError             **error);
static gboolean   gimp_selection_stroke        (GimpItem            *item,
                                                GimpDrawable        *drawable,
                                                GimpStrokeOptions   *stroke_options,
                                                gboolean             push_undo,
                                                GimpProgress        *progress,
                                                GError             **error);
static void       gimp_selection_convert_type  (GimpDrawable        *drawable,
                                                GimpImage           *dest_image,
                                                const Babl          *new_format,
                                                GimpColorProfile    *src_profile,
                                                GimpColorProfile    *dest_profile,
                                                GeglDitherMethod     layer_dither_type,
                                                GeglDitherMethod     mask_dither_type,
                                                gboolean             push_undo,
                                                GimpProgress        *progress);
static void gimp_selection_invalidate_boundary (GimpDrawable        *drawable);

static gboolean   gimp_selection_boundary      (GimpChannel         *channel,
                                                const GimpBoundSeg **segs_in,
                                                const GimpBoundSeg **segs_out,
                                                gint                *num_segs_in,
                                                gint                *num_segs_out,
                                                gint                 x1,
                                                gint                 y1,
                                                gint                 x2,
                                                gint                 y2);
static gboolean   gimp_selection_is_empty      (GimpChannel         *channel);
static void       gimp_selection_feather       (GimpChannel         *channel,
                                                gdouble              radius_x,
                                                gdouble              radius_y,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       gimp_selection_sharpen       (GimpChannel         *channel,
                                                gboolean             push_undo);
static void       gimp_selection_clear         (GimpChannel         *channel,
                                                const gchar         *undo_desc,
                                                gboolean             push_undo);
static void       gimp_selection_all           (GimpChannel         *channel,
                                                gboolean             push_undo);
static void       gimp_selection_invert        (GimpChannel         *channel,
                                                gboolean             push_undo);
static void       gimp_selection_border        (GimpChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                GimpChannelBorderStyle style,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       gimp_selection_grow          (GimpChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                gboolean             push_undo);
static void       gimp_selection_shrink        (GimpChannel         *channel,
                                                gint                 radius_x,
                                                gint                 radius_y,
                                                gboolean             edge_lock,
                                                gboolean             push_undo);
static void       gimp_selection_flood         (GimpChannel         *channel,
                                                gboolean             push_undo);


G_DEFINE_TYPE (GimpSelection, gimp_selection, GIMP_TYPE_CHANNEL)

#define parent_class gimp_selection_parent_class


static void
gimp_selection_class_init (GimpSelectionClass *klass)
{
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class     = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class = GIMP_DRAWABLE_CLASS (klass);
  GimpChannelClass  *channel_class  = GIMP_CHANNEL_CLASS (klass);

  viewable_class->default_icon_name   = "gimp-selection";

  item_class->is_attached             = gimp_selection_is_attached;
  item_class->get_tree                = gimp_selection_get_tree;
  item_class->translate               = gimp_selection_translate;
  item_class->scale                   = gimp_selection_scale;
  item_class->resize                  = gimp_selection_resize;
  item_class->flip                    = gimp_selection_flip;
  item_class->rotate                  = gimp_selection_rotate;
  item_class->fill                    = gimp_selection_fill;
  item_class->stroke                  = gimp_selection_stroke;
  item_class->default_name            = _("Selection Mask");
  item_class->translate_desc          = C_("undo-type", "Move Selection");
  item_class->fill_desc               = C_("undo-type", "Fill Selection");
  item_class->stroke_desc             = C_("undo-type", "Stroke Selection");

  drawable_class->convert_type        = gimp_selection_convert_type;
  drawable_class->invalidate_boundary = gimp_selection_invalidate_boundary;

  channel_class->boundary             = gimp_selection_boundary;
  channel_class->is_empty             = gimp_selection_is_empty;
  channel_class->feather              = gimp_selection_feather;
  channel_class->sharpen              = gimp_selection_sharpen;
  channel_class->clear                = gimp_selection_clear;
  channel_class->all                  = gimp_selection_all;
  channel_class->invert               = gimp_selection_invert;
  channel_class->border               = gimp_selection_border;
  channel_class->grow                 = gimp_selection_grow;
  channel_class->shrink               = gimp_selection_shrink;
  channel_class->flood                = gimp_selection_flood;

  channel_class->feather_desc         = C_("undo-type", "Feather Selection");
  channel_class->sharpen_desc         = C_("undo-type", "Sharpen Selection");
  channel_class->clear_desc           = C_("undo-type", "Select None");
  channel_class->all_desc             = C_("undo-type", "Select All");
  channel_class->invert_desc          = C_("undo-type", "Invert Selection");
  channel_class->border_desc          = C_("undo-type", "Border Selection");
  channel_class->grow_desc            = C_("undo-type", "Grow Selection");
  channel_class->shrink_desc          = C_("undo-type", "Shrink Selection");
  channel_class->flood_desc           = C_("undo-type", "Remove Holes");
}

static void
gimp_selection_init (GimpSelection *selection)
{
}

static gboolean
gimp_selection_is_attached (GimpItem *item)
{
  return (GIMP_IS_IMAGE (gimp_item_get_image (item)) &&
          gimp_image_get_mask (gimp_item_get_image (item)) ==
          GIMP_CHANNEL (item));
}

static GimpItemTree *
gimp_selection_get_tree (GimpItem *item)
{
  return NULL;
}

static void
gimp_selection_translate (GimpItem *item,
                          gdouble   offset_x,
                          gdouble   offset_y,
                          gboolean  push_undo)
{
  GIMP_ITEM_CLASS (parent_class)->translate (item, offset_x, offset_y,
                                             push_undo);
}

static void
gimp_selection_scale (GimpItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      GimpInterpolationType  interp_type,
                      GimpProgress          *progress)
{
  GIMP_ITEM_CLASS (parent_class)->scale (item, new_width, new_height,
                                         new_offset_x, new_offset_y,
                                         interp_type, progress);

  gimp_item_set_offset (item, 0, 0);
}

static void
gimp_selection_resize (GimpItem     *item,
                       GimpContext  *context,
                       GimpFillType  fill_type,
                       gint          new_width,
                       gint          new_height,
                       gint          offset_x,
                       gint          offset_y)
{
  GIMP_ITEM_CLASS (parent_class)->resize (item, context, GIMP_FILL_TRANSPARENT,
                                          new_width, new_height,
                                          offset_x, offset_y);

  gimp_item_set_offset (item, 0, 0);
}

static void
gimp_selection_flip (GimpItem            *item,
                     GimpContext         *context,
                     GimpOrientationType  flip_type,
                     gdouble              axis,
                     gboolean             clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->flip (item, context, flip_type, axis, TRUE);
}

static void
gimp_selection_rotate (GimpItem         *item,
                       GimpContext      *context,
                       GimpRotationType  rotation_type,
                       gdouble           center_x,
                       gdouble           center_y,
                       gboolean          clip_result)
{
  GIMP_ITEM_CLASS (parent_class)->rotate (item, context, rotation_type,
                                          center_x, center_y,
                                          clip_result);
}

static gboolean
gimp_selection_fill (GimpItem         *item,
                     GimpDrawable     *drawable,
                     GimpFillOptions  *fill_options,
                     gboolean          push_undo,
                     GimpProgress     *progress,
                     GError          **error)
{
  GimpSelection      *selection = GIMP_SELECTION (item);
  const GimpBoundSeg *dummy_in;
  const GimpBoundSeg *dummy_out;
  gint                num_dummy_in;
  gint                num_dummy_out;
  gboolean            retval;

  if (! gimp_channel_boundary (GIMP_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("There is no selection to fill."));
      return FALSE;
    }

  gimp_selection_suspend (selection);

  retval = GIMP_ITEM_CLASS (parent_class)->fill (item, drawable,
                                                 fill_options,
                                                 push_undo, progress, error);

  gimp_selection_resume (selection);

  return retval;
}

static gboolean
gimp_selection_stroke (GimpItem           *item,
                       GimpDrawable       *drawable,
                       GimpStrokeOptions  *stroke_options,
                       gboolean            push_undo,
                       GimpProgress       *progress,
                       GError            **error)
{
  GimpSelection      *selection = GIMP_SELECTION (item);
  const GimpBoundSeg *dummy_in;
  const GimpBoundSeg *dummy_out;
  gint                num_dummy_in;
  gint                num_dummy_out;
  gboolean            retval;

  if (! gimp_channel_boundary (GIMP_CHANNEL (selection),
                               &dummy_in, &dummy_out,
                               &num_dummy_in, &num_dummy_out,
                               0, 0, 0, 0))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("There is no selection to stroke."));
      return FALSE;
    }

  gimp_selection_suspend (selection);

  retval = GIMP_ITEM_CLASS (parent_class)->stroke (item, drawable,
                                                   stroke_options,
                                                   push_undo, progress, error);

  gimp_selection_resume (selection);

  return retval;
}

static void
gimp_selection_convert_type (GimpDrawable      *drawable,
                             GimpImage         *dest_image,
                             const Babl        *new_format,
                             GimpColorProfile  *src_profile,
                             GimpColorProfile  *dest_profile,
                             GeglDitherMethod   layer_dither_type,
                             GeglDitherMethod   mask_dither_type,
                             gboolean           push_undo,
                             GimpProgress      *progress)
{
  new_format =
    gimp_babl_mask_format (gimp_babl_format_get_precision (new_format));

  GIMP_DRAWABLE_CLASS (parent_class)->convert_type (drawable, dest_image,
                                                    new_format,
                                                    src_profile,
                                                    dest_profile,
                                                    layer_dither_type,
                                                    mask_dither_type,
                                                    push_undo,
                                                    progress);
}

static void
gimp_selection_invalidate_boundary (GimpDrawable *drawable)
{
  GimpImage *image = gimp_item_get_image (GIMP_ITEM (drawable));
  GList     *layers;

  /*  Turn the current selection off  */
  gimp_image_selection_invalidate (image);

  GIMP_DRAWABLE_CLASS (parent_class)->invalidate_boundary (drawable);

  /*  If there is a floating selection, update it's area...
   *  we need to do this since this selection mask can act as an additional
   *  mask in the composition of the floating selection
   */
  layers = gimp_image_get_selected_layers (image);

  if (g_list_length (layers) == 1 && gimp_layer_is_floating_sel (layers->data))
    {
      gimp_drawable_update (GIMP_DRAWABLE (layers->data), 0, 0, -1, -1);
    }

#if 0
  /*  invalidate the preview  */
  drawable->private->preview_valid = FALSE;
#endif
}

static gboolean
gimp_selection_boundary (GimpChannel         *channel,
                         const GimpBoundSeg **segs_in,
                         const GimpBoundSeg **segs_out,
                         gint                *num_segs_in,
                         gint                *num_segs_out,
                         gint                 unused1,
                         gint                 unused2,
                         gint                 unused3,
                         gint                 unused4)
{
  GimpImage    *image = gimp_item_get_image (GIMP_ITEM (channel));
  GimpLayer    *floating_selection;
  GList        *drawables;
  GList        *layers;
  gboolean      channel_selected;

  drawables = gimp_image_get_selected_drawables (image);
  channel_selected = (drawables && GIMP_IS_CHANNEL (drawables->data));
  g_list_free (drawables);

  if ((floating_selection = gimp_image_get_floating_selection (image)))
    {
      /*  If there is a floating selection, then
       *  we need to do some slightly different boundaries.
       *  Instead of inside and outside boundaries being defined
       *  by the extents of the layer, the inside boundary (the one
       *  that actually marches and is black/white) is the boundary of
       *  the floating selection.  The outside boundary (doesn't move,
       *  is black/gray) is defined as the normal selection mask
       */

      /*  Find the selection mask boundary  */
      GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                   segs_in, segs_out,
                                                   num_segs_in, num_segs_out,
                                                   0, 0, 0, 0);

      /*  Find the floating selection boundary  */
      *segs_in = floating_sel_boundary (floating_selection, num_segs_in);

      return TRUE;
    }
  else if (channel_selected)
    {
      /*  Otherwise, return the boundary...if a channels are selected  */

      return GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          0, 0,
                                                          gimp_image_get_width  (image),
                                                          gimp_image_get_height (image));
    }
  else if ((layers = gimp_image_get_selected_layers (image)))
    {
      /*  If layers are selected, we return multiple boundaries based
       *  on the extents
       */
      GList *iter;
      gint    x1, y1;
      gint    x2       = G_MININT;
      gint    y2       = G_MININT;
      gint    offset_x = G_MAXINT;
      gint    offset_y = G_MAXINT;

      for (iter = layers; iter; iter = iter->next)
        {
          gint item_off_x, item_off_y;
          gint item_x2, item_y2;

          gimp_item_get_offset (iter->data, &item_off_x, &item_off_y);
          offset_x = MIN (offset_x, item_off_x);
          offset_y = MIN (offset_y, item_off_y);

          item_x2 = item_off_x + gimp_item_get_width (GIMP_ITEM (iter->data));
          item_y2 = item_off_y + gimp_item_get_height (GIMP_ITEM (iter->data));
          x2 = MAX (x2, item_x2);
          y2 = MAX (y2, item_y2);
        }

      x1 = CLAMP (offset_x, 0, gimp_image_get_width  (image));
      y1 = CLAMP (offset_y, 0, gimp_image_get_height (image));
      x2 = CLAMP (x2, 0, gimp_image_get_width (image));
      y2 = CLAMP (y2, 0, gimp_image_get_height (image));

      return GIMP_CHANNEL_CLASS (parent_class)->boundary (channel,
                                                          segs_in, segs_out,
                                                          num_segs_in,
                                                          num_segs_out,
                                                          x1, y1, x2, y2);
    }

  *segs_in      = NULL;
  *segs_out     = NULL;
  *num_segs_in  = 0;
  *num_segs_out = 0;

  return FALSE;
}

static gboolean
gimp_selection_is_empty (GimpChannel *channel)
{
  GimpSelection *selection = GIMP_SELECTION (channel);

  /*  in order to allow stroking of selections, we need to pretend here
   *  that the selection mask is empty so that it doesn't mask the paint
   *  during the stroke operation.
   */
  if (selection->suspend_count > 0)
    return TRUE;

  return GIMP_CHANNEL_CLASS (parent_class)->is_empty (channel);
}

static void
gimp_selection_feather (GimpChannel *channel,
                        gdouble      radius_x,
                        gdouble      radius_y,
                        gboolean     edge_lock,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->feather (channel, radius_x, radius_y,
                                              edge_lock, push_undo);
}

static void
gimp_selection_sharpen (GimpChannel *channel,
                        gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->sharpen (channel, push_undo);
}

static void
gimp_selection_clear (GimpChannel *channel,
                      const gchar *undo_desc,
                      gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->clear (channel, undo_desc, push_undo);
}

static void
gimp_selection_all (GimpChannel *channel,
                    gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->all (channel, push_undo);
}

static void
gimp_selection_invert (GimpChannel *channel,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->invert (channel, push_undo);
}

static void
gimp_selection_border (GimpChannel            *channel,
                       gint                    radius_x,
                       gint                    radius_y,
                       GimpChannelBorderStyle  style,
                       gboolean                edge_lock,
                       gboolean                push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->border (channel,
                                             radius_x, radius_y,
                                             style, edge_lock,
                                             push_undo);
}

static void
gimp_selection_grow (GimpChannel *channel,
                     gint         radius_x,
                     gint         radius_y,
                     gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->grow (channel,
                                           radius_x, radius_y,
                                           push_undo);
}

static void
gimp_selection_shrink (GimpChannel *channel,
                       gint         radius_x,
                       gint         radius_y,
                       gboolean     edge_lock,
                       gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->shrink (channel,
                                             radius_x, radius_y, edge_lock,
                                             push_undo);
}

static void
gimp_selection_flood (GimpChannel *channel,
                      gboolean     push_undo)
{
  GIMP_CHANNEL_CLASS (parent_class)->flood (channel, push_undo);
}


/*  public functions  */

GimpChannel *
gimp_selection_new (GimpImage *image,
                    gint       width,
                    gint       height)
{
  GeglColor   *black = gegl_color_new ("black");
  GimpChannel *channel;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  gimp_color_set_alpha (black, 0.5);
  channel = GIMP_CHANNEL (gimp_drawable_new (GIMP_TYPE_SELECTION,
                                             image, NULL,
                                             0, 0, width, height,
                                             gimp_image_get_mask_format (image)));

  gimp_channel_set_color (channel, black, FALSE);
  gimp_channel_set_show_masked (channel, TRUE);

  channel->x2 = width;
  channel->y2 = height;

  g_object_unref (black);

  return channel;
}

gint
gimp_selection_suspend (GimpSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_SELECTION (selection), 0);

  selection->suspend_count++;

  return selection->suspend_count;
}

gint
gimp_selection_resume (GimpSelection *selection)
{
  g_return_val_if_fail (GIMP_IS_SELECTION (selection), 0);
  g_return_val_if_fail (selection->suspend_count > 0, 0);

  selection->suspend_count--;

  return selection->suspend_count;
}

GeglBuffer *
gimp_selection_extract (GimpSelection *selection,
                        GList         *pickables,
                        GimpContext   *context,
                        gboolean       cut_image,
                        gboolean       keep_indexed,
                        gboolean       add_alpha,
                        gint          *offset_x,
                        gint          *offset_y,
                        GError       **error)
{
  GimpImage    *image      = NULL;
  GimpImage    *temp_image = NULL;
  GimpPickable *pickable   = NULL;
  GeglBuffer   *src_buffer;
  GeglBuffer   *dest_buffer;
  GList        *iter;
  const Babl   *src_format;
  const Babl   *dest_format;
  gint          x1, y1, x2, y2;
  gboolean      non_empty;
  gint          off_x, off_y;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  g_return_val_if_fail (pickables != NULL, NULL);

  for (iter = pickables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_PICKABLE (iter->data), NULL);

      if (GIMP_IS_ITEM (iter->data))
        g_return_val_if_fail (gimp_item_is_attached (iter->data), NULL);

      if (! image)
        image = gimp_pickable_get_image (iter->data);
      else
        g_return_val_if_fail (image == gimp_pickable_get_image (iter->data), NULL);
    }

  if (g_list_length (pickables) == 1)
    {
      pickable = pickables->data;
    }
  else
    {
      for (iter = pickables; iter; iter = iter->next)
        g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);

      temp_image = gimp_image_new_from_drawables (image->gimp, pickables, TRUE, FALSE);
      selection  = GIMP_SELECTION (gimp_image_get_mask (temp_image));

      pickable   = GIMP_PICKABLE (temp_image);

      /* Don't cut from the temporary image. */
      cut_image = FALSE;
    }

  /*  If there are no bounds, then just extract the entire image
   *  This may not be the correct behavior, but after getting rid
   *  of floating selections, it's still tempting to "cut" or "copy"
   *  a small layer and expect it to work, even though there is no
   *  actual selection mask
   */
  if (GIMP_IS_DRAWABLE (pickable))
    {
      non_empty = gimp_item_mask_bounds (GIMP_ITEM (pickable),
                                         &x1, &y1, &x2, &y2);

      gimp_item_get_offset (GIMP_ITEM (pickable), &off_x, &off_y);
    }
  else
    {
      non_empty = gimp_item_bounds (GIMP_ITEM (selection),
                                    &x1, &y1, &x2, &y2);
      x2 += x1;
      y2 += y1;

      off_x = 0;
      off_y = 0;

      /* can't cut from non-drawables, fall back to copy */
      cut_image = FALSE;
    }

  if (non_empty && ((x1 == x2) || (y1 == y2)))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Unable to cut or copy because the "
                             "selected region is empty."));

      if (temp_image)
        g_object_unref (temp_image);

      return NULL;
    }

  /*  If there is a selection, we must add alpha because the selection
   *  could have any shape.
   */
  if (non_empty)
    add_alpha = TRUE;

  src_format = gimp_pickable_get_format (pickable);

  /*  How many bytes in the temp buffer?  */
  if (babl_format_is_palette (src_format) && ! keep_indexed)
    {
      dest_format = gimp_image_get_format (image, GIMP_RGB,
                                           gimp_image_get_precision (image),
                                           add_alpha ||
                                           babl_format_has_alpha (src_format),
                                           babl_format_get_space (src_format));
    }
  else
    {
      if (add_alpha)
        dest_format = gimp_pickable_get_format_with_alpha (pickable);
      else
        dest_format = src_format;
    }

  gimp_pickable_flush (pickable);

  src_buffer = gimp_pickable_get_buffer (pickable);

  /*  Allocate the temp buffer  */
  dest_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, x2 - x1, y2 - y1),
                                 dest_format);

  /*  First, copy the pixels, possibly doing INDEXED->RGB and adding alpha  */
  gimp_gegl_buffer_copy (src_buffer,  GEGL_RECTANGLE (x1, y1, x2 - x1, y2 - y1),
                         GEGL_ABYSS_NONE,
                         dest_buffer, GEGL_RECTANGLE (0, 0, 0, 0));

  if (non_empty)
    {
      /*  If there is a selection, mask the dest_buffer with it  */

      GeglBuffer *mask_buffer;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (selection));

      gimp_gegl_apply_opacity (dest_buffer, NULL, NULL, dest_buffer,
                               mask_buffer,
                               - (off_x + x1),
                               - (off_y + y1),
                               1.0);

      if (cut_image)
        {
          gimp_drawable_edit_clear (GIMP_DRAWABLE (pickable), context);
        }
    }
  else if (cut_image)
    {
      /*  If we're cutting without selection, remove either the layer
       *  (or floating selection), the layer mask, or the channel
       */
      if (GIMP_IS_LAYER (pickable))
        {
          gimp_image_remove_layer (image, GIMP_LAYER (pickable),
                                   TRUE, NULL);
        }
      else if (GIMP_IS_LAYER_MASK (pickable))
        {
          gimp_layer_apply_mask (gimp_layer_mask_get_layer (GIMP_LAYER_MASK (pickable)),
                                 GIMP_MASK_DISCARD, TRUE);
        }
      else if (GIMP_IS_CHANNEL (pickable))
        {
          gimp_image_remove_channel (image, GIMP_CHANNEL (pickable),
                                     TRUE, NULL);
        }
    }

  *offset_x = x1 + off_x;
  *offset_y = y1 + off_y;

  if (temp_image)
    g_object_unref (temp_image);

  return dest_buffer;
}

GimpLayer *
gimp_selection_float (GimpSelection *selection,
                      GList         *drawables,
                      GimpContext   *context,
                      gboolean       cut_image,
                      gint           off_x,
                      gint           off_y,
                      GError       **error)
{
  GimpImage        *image;
  GimpLayer        *layer;
  GeglBuffer       *buffer;
  GimpColorProfile *profile;
  GimpImage        *temp_image = NULL;
  const Babl       *format     = NULL;
  GList            *iter;
  gint              x1, y1;
  gint              x2, y2;

  g_return_val_if_fail (GIMP_IS_SELECTION (selection), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (GIMP_IS_DRAWABLE (iter->data), NULL);
      g_return_val_if_fail (gimp_item_is_attached (iter->data), NULL);

      if (! format)
        format = gimp_drawable_get_format_with_alpha (iter->data);
      else
        g_return_val_if_fail (format == gimp_drawable_get_format_with_alpha (iter->data),
                              NULL);
    }

  image = gimp_item_get_image (GIMP_ITEM (selection));

  /*  Make sure there is a region to float...  */
  for (iter = drawables; iter; iter = iter->next)
    {
      if (gimp_item_mask_bounds (iter->data, &x1, &y1, &x2, &y2) &&
          x1 != x2 && y1 != y2)
        break;
    }
  if (iter == NULL)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("Cannot float selection because the selected "
                             "region is empty."));
      return NULL;
    }

  /*  Start an undo group  */
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_FS_FLOAT,
                               C_("undo-type", "Float Selection"));

  /*  Cut or copy the selected region  */
  buffer = gimp_selection_extract (selection, drawables, context,
                                   cut_image, FALSE, TRUE,
                                   &x1, &y1, NULL);

  profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (drawables->data));

  /*  Clear the selection  */
  gimp_channel_clear (GIMP_CHANNEL (selection), NULL, TRUE);

  /* Create a new layer from the buffer, using the drawables' type
   *  because it may be different from the image's type if we cut from
   *  a channel or layer mask
   */
  layer = gimp_layer_new_from_gegl_buffer (buffer, image, format,
                                           _("Floated Layer"),
                                           GIMP_OPACITY_OPAQUE,
                                           gimp_image_get_default_new_layer_mode (image),
                                           profile);

  /*  Set the offsets  */
  gimp_item_set_offset (GIMP_ITEM (layer), x1 + off_x, y1 + off_y);

  /*  Free the temp buffer  */
  g_object_unref (buffer);

  /*  Add the floating layer to the image  */
  floating_sel_attach (layer, drawables->data);

  /*  End an undo group  */
  gimp_image_undo_group_end (image);

  /*  invalidate the image's boundary variables  */
  GIMP_CHANNEL (selection)->boundary_known = FALSE;

  if (temp_image)
    g_object_unref (temp_image);

  return layer;
}
