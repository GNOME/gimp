/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablepreview.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "gimpdrawablepreview.h"


/**
 * SECTION: gimpdrawablepreview
 * @title: GimpDrawablePreview
 * @short_description: A widget providing a preview of a #GimpDrawable.
 *
 * A widget providing a preview of a #GimpDrawable.
 **/


#define SELECTION_BORDER  8

enum
{
  PROP_0,
  PROP_DRAWABLE_ID
};

typedef struct
{
  gint      x;
  gint      y;
  gboolean  update;
} PreviewSettings;


struct _GimpDrawablePreviewPrivate
{
  gint32 drawable_ID;
};

#define GET_PRIVATE(obj) (((GimpDrawablePreview *) (obj))->priv)


static void  gimp_drawable_preview_constructed   (GObject         *object);
static void  gimp_drawable_preview_dispose       (GObject         *object);
static void  gimp_drawable_preview_get_property  (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void  gimp_drawable_preview_set_property  (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);

static void  gimp_drawable_preview_style_updated (GtkWidget       *widget);

static void  gimp_drawable_preview_draw_original (GimpPreview     *preview);
static void  gimp_drawable_preview_draw_thumb    (GimpPreview     *preview,
                                                  GimpPreviewArea *area,
                                                  gint             width,
                                                  gint             height);
static void  gimp_drawable_preview_draw_buffer   (GimpPreview     *preview,
                                                  const guchar    *buffer,
                                                  gint             rowstride);

static void  gimp_drawable_preview_set_drawable_id
                                                (GimpDrawablePreview *preview,
                                                 gint32               drawable_ID);


G_DEFINE_TYPE (GimpDrawablePreview, gimp_drawable_preview,
               GIMP_TYPE_SCROLLED_PREVIEW)

#define parent_class gimp_drawable_preview_parent_class

static gint gimp_drawable_preview_counter = 0;


static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->constructed   = gimp_drawable_preview_constructed;
  object_class->dispose       = gimp_drawable_preview_dispose;
  object_class->get_property  = gimp_drawable_preview_get_property;
  object_class->set_property  = gimp_drawable_preview_set_property;

  widget_class->style_updated = gimp_drawable_preview_style_updated;

  preview_class->draw         = gimp_drawable_preview_draw_original;
  preview_class->draw_thumb   = gimp_drawable_preview_draw_thumb;
  preview_class->draw_buffer  = gimp_drawable_preview_draw_buffer;

  g_type_class_add_private (object_class, sizeof (GimpDrawablePreviewPrivate));

  /**
   * GimpDrawablePreview:drawable-id:
   *
   * The drawable the #GimpDrawablePreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE_ID,
                                   g_param_spec_int ("drawable-id",
                                                     "Drawable ID",
                                                     "The drawable this preview is attached to",
                                                     -1, G_MAXINT, -1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

}

static void
gimp_drawable_preview_init (GimpDrawablePreview *preview)
{
  preview->priv = G_TYPE_INSTANCE_GET_PRIVATE (preview,
                                               GIMP_TYPE_DRAWABLE_PREVIEW,
                                               GimpDrawablePreviewPrivate);

  g_object_set (gimp_preview_get_area (GIMP_PREVIEW (preview)),
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static void
gimp_drawable_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-drawable-preview-%d",
                               g_get_prgname (),
                               ++gimp_drawable_preview_counter);

  if (gimp_get_data (data_name, &settings))
    {
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
      gimp_scrolled_preview_set_position (GIMP_SCROLLED_PREVIEW (object),
                                          settings.x, settings.y);
    }

  g_object_set_data_full (object, "gimp-drawable-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
gimp_drawable_preview_dispose (GObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                              "gimp-drawable-preview-data-name");

  if (data_name)
    {
      GimpPreview     *preview = GIMP_PREVIEW (object);
      PreviewSettings  settings;

      gimp_preview_get_position (preview, &settings.x, &settings.y);
      settings.update = gimp_preview_get_update (preview);

      gimp_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_drawable_preview_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpDrawablePreview *preview = GIMP_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE_ID:
      g_value_set_int (value,
                       gimp_drawable_preview_get_drawable_id (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_preview_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpDrawablePreview *preview = GIMP_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE_ID:
      gimp_drawable_preview_set_drawable_id (preview,
                                             g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_preview_style_updated (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  GtkWidget   *area    = gimp_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint xmin, ymin;
      gint xmax, ymax;
      gint size;

      gimp_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      gtk_widget_set_size_request (area,
                                   MIN (xmax - xmin, size),
                                   MIN (ymax - ymin, size));
    }
}

static void
gimp_drawable_preview_draw_original (GimpPreview *preview)
{
  GimpDrawablePreviewPrivate *priv = GET_PRIVATE (preview);
  guchar                     *buffer;
  gint                        width, height;
  gint                        xoff, yoff;
  gint                        xmin, ymin;
  gint                        xmax, ymax;
  gint                        bpp;
  GimpImageType               type;

  if (priv->drawable_ID < 1)
    return;

  gimp_preview_get_size (preview, &width, &height);
  gimp_preview_get_offsets (preview, &xoff, &yoff);
  gimp_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

  xoff = CLAMP (xoff, 0, xmax - xmin - width);
  yoff = CLAMP (yoff, 0, ymax - ymin - height);

  gimp_preview_set_offsets (preview, xoff, yoff);

  buffer = gimp_drawable_get_sub_thumbnail_data (priv->drawable_ID,
                                                 xoff + xmin,
                                                 yoff + ymin,
                                                 width, height,
                                                 &width, &height, &bpp);

  switch (bpp)
    {
    case 1: type = GIMP_GRAY_IMAGE; break;
    case 2: type = GIMP_GRAYA_IMAGE; break;
    case 3: type = GIMP_RGB_IMAGE; break;
    case 4: type = GIMP_RGBA_IMAGE; break;
    default:
      g_free (buffer);
      return;
    }

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview_get_area (preview)),
                          0, 0, width, height, type, buffer, width * bpp);
  g_free (buffer);
}

static void
gimp_drawable_preview_draw_thumb (GimpPreview     *preview,
                                  GimpPreviewArea *area,
                                  gint             width,
                                  gint             height)
{
  GimpDrawablePreviewPrivate *priv = GET_PRIVATE (preview);

  if (priv->drawable_ID > 0)
    _gimp_drawable_preview_area_draw_thumb (area, priv->drawable_ID,
                                            width, height);
}

void
_gimp_drawable_preview_area_draw_thumb (GimpPreviewArea *area,
                                        gint32           drawable_ID,
                                        gint             width,
                                        gint             height)
{
  guchar *buffer;
  gint    x1, y1, x2, y2;
  gint    bpp;
  gint    size = 100;
  gint    nav_width, nav_height;

  g_return_if_fail (GIMP_IS_PREVIEW_AREA (area));
  g_return_if_fail (gimp_item_is_valid (drawable_ID));
  g_return_if_fail (gimp_item_is_drawable (drawable_ID));

  if (_gimp_drawable_preview_get_bounds (drawable_ID, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;
    }
  else
    {
      width  = gimp_drawable_width  (drawable_ID);
      height = gimp_drawable_height (drawable_ID);
    }

  if (width > height)
    {
      nav_width  = MIN (width, size);
      nav_height = (height * nav_width) / width;
    }
  else
    {
      nav_height = MIN (height, size);
      nav_width  = (width * nav_height) / height;
    }

  if (_gimp_drawable_preview_get_bounds (drawable_ID, &x1, &y1, &x2, &y2))
    {
      buffer = gimp_drawable_get_sub_thumbnail_data (drawable_ID,
                                                     x1, y1, x2 - x1, y2 - y1,
                                                     &nav_width, &nav_height,
                                                     &bpp);
    }
  else
    {
      buffer = gimp_drawable_get_thumbnail_data (drawable_ID,
                                                 &nav_width, &nav_height,
                                                 &bpp);
    }

  if (buffer)
    {
      GimpImageType type;

      gtk_widget_set_size_request (GTK_WIDGET (area), nav_width, nav_height);
      gtk_widget_show (GTK_WIDGET (area));
      gtk_widget_realize (GTK_WIDGET (area));

      switch (bpp)
        {
        case 1:  type = GIMP_GRAY_IMAGE;   break;
        case 2:  type = GIMP_GRAYA_IMAGE;  break;
        case 3:  type = GIMP_RGB_IMAGE;    break;
        case 4:  type = GIMP_RGBA_IMAGE;   break;
        default:
          g_free (buffer);
          return;
        }

      gimp_preview_area_draw (area,
                              0, 0, nav_width, nav_height,
                              type, buffer, bpp * nav_width);
      g_free (buffer);
    }
}

static void
gimp_drawable_preview_draw_area (GimpDrawablePreview *preview,
                                 gint                 x,
                                 gint                 y,
                                 gint                 width,
                                 gint                 height,
                                 const guchar        *buf,
                                 gint                 rowstride)
{
  GimpDrawablePreviewPrivate *priv         = GET_PRIVATE (preview);
  GimpPreview                *gimp_preview = GIMP_PREVIEW (preview);
  GtkWidget                  *area         = gimp_preview_get_area (gimp_preview);
  gint                        xmin, ymin;
  gint                        xoff, yoff;
  gint32                      image_ID;

  gimp_preview_get_bounds (gimp_preview, &xmin, &ymin, NULL, NULL);
  gimp_preview_get_offsets (gimp_preview, &xoff, &yoff);

  image_ID = gimp_item_get_image (priv->drawable_ID);

  if (gimp_selection_is_empty (image_ID))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              x - xoff - xmin,
                              y - yoff - ymin,
                              width,
                              height,
                              gimp_drawable_type (priv->drawable_ID),
                              buf, rowstride);
    }
  else
    {
      gint offset_x, offset_y;
      gint mask_x, mask_y;
      gint mask_width, mask_height;
      gint draw_x, draw_y;
      gint draw_width, draw_height;

      gimp_drawable_offsets (priv->drawable_ID, &offset_x, &offset_y);

      if (gimp_drawable_mask_intersect (priv->drawable_ID,
                                        &mask_x, &mask_y,
                                        &mask_width, &mask_height) &&
          gimp_rectangle_intersect (mask_x, mask_y,
                                    mask_width, mask_height,
                                    x, y, width, height,
                                    &draw_x, &draw_y,
                                    &draw_width, &draw_height))
        {
          GimpImageType  type;
          gint32         selection_ID;
          guchar        *src;
          guchar        *sel;
          gint           d_w, d_h, d_bpp;
          gint           s_w, s_h, s_bpp;

          d_w = draw_width;
          d_h = draw_height;

          s_w = draw_width;
          s_h = draw_height;

          selection_ID = gimp_image_get_selection (image_ID);

          src = gimp_drawable_get_sub_thumbnail_data (priv->drawable_ID,
                                                      draw_x, draw_y,
                                                      draw_width, draw_height,
                                                      &d_w, &d_h,
                                                      &d_bpp);

          sel = gimp_drawable_get_sub_thumbnail_data (selection_ID,
                                                      draw_x + offset_x,
                                                      draw_y + offset_y,
                                                      draw_width, draw_height,
                                                      &s_w, &s_h,
                                                      &s_bpp);

          switch (d_bpp)
            {
            case 1:  type = GIMP_GRAY_IMAGE;   break;
            case 2:  type = GIMP_GRAYA_IMAGE;  break;
            case 3:  type = GIMP_RGB_IMAGE;    break;
            case 4:  type = GIMP_RGBA_IMAGE;   break;
            default:
              g_free (sel);
              g_free (src);
              return;
            }

          gimp_preview_area_mask (GIMP_PREVIEW_AREA (area),
                                  draw_x - xoff - xmin,
                                  draw_y - yoff - ymin,
                                  draw_width,
                                  draw_height,
                                  type,
                                  src, draw_width * d_bpp,
                                  (buf +
                                   (draw_x - x) * d_bpp +
                                   (draw_y - y) * d_w * d_bpp),
                                  rowstride,
                                  sel, s_w);

          g_free (sel);
          g_free (src);
        }
    }
}

static void
gimp_drawable_preview_draw_buffer (GimpPreview  *preview,
                                   const guchar *buffer,
                                   gint          rowstride)
{
  gint x, y;
  gint width, height;

  gimp_preview_get_position (preview, &x, &y);
  gimp_preview_get_size (preview, &width, &height);

  gimp_drawable_preview_draw_area (GIMP_DRAWABLE_PREVIEW (preview),
                                   x, y,
                                   width, height,
                                   buffer, rowstride);
}

static void
gimp_drawable_preview_set_drawable_id (GimpDrawablePreview *drawable_preview,
                                       gint32               drawable_ID)
{
  GimpPreview                *preview = GIMP_PREVIEW (drawable_preview);
  GimpDrawablePreviewPrivate *priv    = GET_PRIVATE (preview);
  gint                        x1, y1, x2, y2;

  g_return_if_fail (priv->drawable_ID < 1);

  priv->drawable_ID = drawable_ID;

  _gimp_drawable_preview_get_bounds (drawable_ID, &x1, &y1, &x2, &y2);

  gimp_preview_set_bounds (preview, x1, y1, x2, y2);

  if (gimp_drawable_is_indexed (drawable_ID))
    {
      guint32    image_ID = gimp_item_get_image (drawable_ID);
      GtkWidget *area     = gimp_preview_get_area (preview);
      guchar    *cmap;
      gint       num_colors;

      cmap = gimp_image_get_colormap (image_ID, &num_colors);
      gimp_preview_area_set_colormap (GIMP_PREVIEW_AREA (area),
                                      cmap, num_colors);
      g_free (cmap);
    }
}


#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

gboolean
_gimp_drawable_preview_get_bounds (gint32  drawable_ID,
                                   gint   *xmin,
                                   gint   *ymin,
                                   gint   *xmax,
                                   gint   *ymax)
{
  gint     width;
  gint     height;
  gint     offset_x;
  gint     offset_y;
  gint     x1, y1;
  gint     x2, y2;
  gboolean retval;

  g_return_val_if_fail (gimp_item_is_valid (drawable_ID), FALSE);
  g_return_val_if_fail (gimp_item_is_drawable (drawable_ID), FALSE);

  width  = gimp_drawable_width (drawable_ID);
  height = gimp_drawable_height (drawable_ID);

  retval = gimp_drawable_mask_bounds (drawable_ID, &x1, &y1, &x2, &y2);

  gimp_drawable_offsets (drawable_ID, &offset_x, &offset_y);

  *xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  *ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  *xmax = MIN3 (x2 + SELECTION_BORDER, width,  width  + offset_x);
  *ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  return retval;
}


/**
 * gimp_drawable_preview_new_from_drawable_id:
 * @drawable_ID: a drawable ID
 *
 * Creates a new #GimpDrawablePreview widget for @drawable_ID.
 *
 * Returns: A pointer to the new #GimpDrawablePreview widget.
 *
 * Since: 2.10
 **/
GtkWidget *
gimp_drawable_preview_new_from_drawable_id (gint32 drawable_ID)
{
  g_return_val_if_fail (gimp_item_is_valid (drawable_ID), NULL);
  g_return_val_if_fail (gimp_item_is_drawable (drawable_ID), NULL);

  return g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW,
                       "drawable-id", drawable_ID,
                       NULL);
}

/**
 * gimp_drawable_preview_get_drawable_id:
 * @preview:   a #GimpDrawablePreview widget
 *
 * Return value: the drawable_ID that has been passed to
 *               gimp_drawable_preview_new_from_drawable_id().
 *
 * Since: 2.10
 **/
gint32
gimp_drawable_preview_get_drawable_id (GimpDrawablePreview *preview)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview), -1);

  return GET_PRIVATE (preview)->drawable_ID;
}

/**
 * gimp_drawable_preview_draw_region:
 * @preview: a #GimpDrawablePreview widget
 * @region:  a #GimpPixelRgn
 *
 * Since: 2.2
 **/
void
gimp_drawable_preview_draw_region (GimpDrawablePreview *preview,
                                   const GimpPixelRgn  *region)
{
  GimpDrawablePreviewPrivate *priv;

  g_return_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview));
  g_return_if_fail (region != NULL);

  priv = GET_PRIVATE (preview);

  g_return_if_fail (priv->drawable_ID > 0);

  /*  If the data field is initialized, this region is currently being
   *  processed and we can access it directly.
   */
  if (region->data)
    {
      gimp_drawable_preview_draw_area (preview,
                                       region->x,
                                       region->y,
                                       region->w,
                                       region->h,
                                       region->data,
                                       region->rowstride);
    }
  else
    {
      GimpPixelRgn  src = *region;
      gpointer      iter;

      src.dirty = FALSE; /* we don't dirty the tiles, just read them */

      for (iter = gimp_pixel_rgns_register (1, &src);
           iter != NULL;
           iter = gimp_pixel_rgns_process (iter))
        {
          gimp_drawable_preview_draw_area (preview,
                                           src.x,
                                           src.y,
                                           src.w,
                                           src.h,
                                           src.data,
                                           src.rowstride);
        }
    }
}
