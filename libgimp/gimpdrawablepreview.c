/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmadrawablepreview.c
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
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "ligmauitypes.h"

#include "ligma.h"

#include "ligmadrawablepreview.h"


/**
 * SECTION: ligmadrawablepreview
 * @title: LigmaDrawablePreview
 * @short_description: A widget providing a preview of a #LigmaDrawable.
 *
 * A widget providing a preview of a #LigmaDrawable.
 **/


#define SELECTION_BORDER  8

enum
{
  PROP_0,
  PROP_DRAWABLE
};

typedef struct
{
  gint      x;
  gint      y;
  gboolean  update;
} PreviewSettings;


struct _LigmaDrawablePreviewPrivate
{
  LigmaDrawable *drawable;
};

#define GET_PRIVATE(obj) (((LigmaDrawablePreview *) (obj))->priv)


static void  ligma_drawable_preview_constructed   (GObject         *object);
static void  ligma_drawable_preview_dispose       (GObject         *object);
static void  ligma_drawable_preview_get_property  (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void  ligma_drawable_preview_set_property  (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);

static void  ligma_drawable_preview_style_updated (GtkWidget       *widget);

static void  ligma_drawable_preview_draw_original (LigmaPreview     *preview);
static void  ligma_drawable_preview_draw_thumb    (LigmaPreview     *preview,
                                                  LigmaPreviewArea *area,
                                                  gint             width,
                                                  gint             height);
static void  ligma_drawable_preview_draw_buffer   (LigmaPreview     *preview,
                                                  const guchar    *buffer,
                                                  gint             rowstride);

static void  ligma_drawable_preview_set_drawable  (LigmaDrawablePreview *preview,
                                                  LigmaDrawable        *drawable);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaDrawablePreview, ligma_drawable_preview,
                            LIGMA_TYPE_SCROLLED_PREVIEW)

#define parent_class ligma_drawable_preview_parent_class

static gint ligma_drawable_preview_counter = 0;


static void
ligma_drawable_preview_class_init (LigmaDrawablePreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  LigmaPreviewClass *preview_class = LIGMA_PREVIEW_CLASS (klass);

  object_class->constructed   = ligma_drawable_preview_constructed;
  object_class->dispose       = ligma_drawable_preview_dispose;
  object_class->get_property  = ligma_drawable_preview_get_property;
  object_class->set_property  = ligma_drawable_preview_set_property;

  widget_class->style_updated = ligma_drawable_preview_style_updated;

  preview_class->draw         = ligma_drawable_preview_draw_original;
  preview_class->draw_thumb   = ligma_drawable_preview_draw_thumb;
  preview_class->draw_buffer  = ligma_drawable_preview_draw_buffer;

  /**
   * LigmaDrawablePreview:drawable-id:
   *
   * The drawable the #LigmaDrawablePreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "The drawable this preview is attached to",
                                                        LIGMA_TYPE_DRAWABLE,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

}

static void
ligma_drawable_preview_init (LigmaDrawablePreview *preview)
{
  preview->priv = ligma_drawable_preview_get_instance_private (preview);

  g_object_set (ligma_preview_get_area (LIGMA_PREVIEW (preview)),
                "check-size", ligma_check_size (),
                "check-type", ligma_check_type (),
                "check-custom-color1", ligma_check_custom_color1 (),
                "check-custom-color2", ligma_check_custom_color2 (),
                NULL);
}

static void
ligma_drawable_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-drawable-preview-%d",
                               g_get_prgname (),
                               ++ligma_drawable_preview_counter);

  if (ligma_get_data (data_name, &settings))
    {
      ligma_preview_set_update (LIGMA_PREVIEW (object), settings.update);
      ligma_scrolled_preview_set_position (LIGMA_SCROLLED_PREVIEW (object),
                                          settings.x, settings.y);
    }

  g_object_set_data_full (object, "ligma-drawable-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
ligma_drawable_preview_dispose (GObject *object)
{
  LigmaDrawablePreviewPrivate *priv = GET_PRIVATE (object);
  const gchar                *data_name;

  data_name = g_object_get_data (G_OBJECT (object),
                                 "ligma-drawable-preview-data-name");
  if (data_name)
    {
      LigmaPreview     *preview = LIGMA_PREVIEW (object);
      PreviewSettings  settings;

      ligma_preview_get_position (preview, &settings.x, &settings.y);
      settings.update = ligma_preview_get_update (preview);

      ligma_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  g_clear_object (&priv->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_drawable_preview_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaDrawablePreview *preview = LIGMA_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value,
                          ligma_drawable_preview_get_drawable (preview));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_preview_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaDrawablePreview *preview = LIGMA_DRAWABLE_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      ligma_drawable_preview_set_drawable (preview,
                                          g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_preview_style_updated (GtkWidget *widget)
{
  LigmaPreview *preview = LIGMA_PREVIEW (widget);
  GtkWidget   *area    = ligma_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      gint xmin, ymin;
      gint xmax, ymax;
      gint size;

      ligma_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      gtk_widget_set_size_request (area,
                                   MIN (xmax - xmin, size),
                                   MIN (ymax - ymin, size));
    }
}

static void
ligma_drawable_preview_draw_original (LigmaPreview *preview)
{
  LigmaDrawablePreviewPrivate *priv = GET_PRIVATE (preview);
  guchar                     *buffer;
  gint                        width, height;
  gint                        xoff, yoff;
  gint                        xmin, ymin;
  gint                        xmax, ymax;
  gint                        bpp;
  LigmaImageType               type;

  if (priv->drawable == NULL)
    return;

  ligma_preview_get_size (preview, &width, &height);
  ligma_preview_get_offsets (preview, &xoff, &yoff);
  ligma_preview_get_bounds (preview, &xmin, &ymin, &xmax, &ymax);

  xoff = CLAMP (xoff, 0, xmax - xmin - width);
  yoff = CLAMP (yoff, 0, ymax - ymin - height);

  ligma_preview_set_offsets (preview, xoff, yoff);

  buffer = ligma_drawable_get_sub_thumbnail_data (priv->drawable,
                                                 xoff + xmin,
                                                 yoff + ymin,
                                                 width, height,
                                                 &width, &height, &bpp);

  switch (bpp)
    {
    case 1: type = LIGMA_GRAY_IMAGE; break;
    case 2: type = LIGMA_GRAYA_IMAGE; break;
    case 3: type = LIGMA_RGB_IMAGE; break;
    case 4: type = LIGMA_RGBA_IMAGE; break;
    default:
      g_free (buffer);
      return;
    }

  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (ligma_preview_get_area (preview)),
                          0, 0, width, height, type, buffer, width * bpp);
  g_free (buffer);
}

static void
ligma_drawable_preview_draw_thumb (LigmaPreview     *preview,
                                  LigmaPreviewArea *area,
                                  gint             width,
                                  gint             height)
{
  LigmaDrawablePreviewPrivate *priv = GET_PRIVATE (preview);

  if (priv->drawable)
    _ligma_drawable_preview_area_draw_thumb (area, priv->drawable,
                                            width, height);
}

void
_ligma_drawable_preview_area_draw_thumb (LigmaPreviewArea *area,
                                        LigmaDrawable    *drawable,
                                        gint             width,
                                        gint             height)
{
  guchar *buffer;
  gint    x1, y1, x2, y2;
  gint    bpp;
  gint    size = 100;
  gint    nav_width, nav_height;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)));
  g_return_if_fail (ligma_item_is_drawable (LIGMA_ITEM (drawable)));

  if (_ligma_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;
    }
  else
    {
      width  = ligma_drawable_get_width  (drawable);
      height = ligma_drawable_get_height (drawable);
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

  if (_ligma_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      buffer = ligma_drawable_get_sub_thumbnail_data (drawable,
                                                     x1, y1, x2 - x1, y2 - y1,
                                                     &nav_width, &nav_height,
                                                     &bpp);
    }
  else
    {
      buffer = ligma_drawable_get_thumbnail_data (drawable,
                                                 &nav_width, &nav_height,
                                                 &bpp);
    }

  if (buffer)
    {
      LigmaImageType type;

      gtk_widget_set_size_request (GTK_WIDGET (area), nav_width, nav_height);
      gtk_widget_show (GTK_WIDGET (area));
      gtk_widget_realize (GTK_WIDGET (area));

      switch (bpp)
        {
        case 1:  type = LIGMA_GRAY_IMAGE;   break;
        case 2:  type = LIGMA_GRAYA_IMAGE;  break;
        case 3:  type = LIGMA_RGB_IMAGE;    break;
        case 4:  type = LIGMA_RGBA_IMAGE;   break;
        default:
          g_free (buffer);
          return;
        }

      ligma_preview_area_draw (area,
                              0, 0, nav_width, nav_height,
                              type, buffer, bpp * nav_width);
      g_free (buffer);
    }
}

static void
ligma_drawable_preview_draw_area (LigmaDrawablePreview *preview,
                                 gint                 x,
                                 gint                 y,
                                 gint                 width,
                                 gint                 height,
                                 const guchar        *buf,
                                 gint                 rowstride)
{
  LigmaDrawablePreviewPrivate *priv         = GET_PRIVATE (preview);
  LigmaPreview                *ligma_preview = LIGMA_PREVIEW (preview);
  GtkWidget                  *area         = ligma_preview_get_area (ligma_preview);
  LigmaImage                  *image;
  gint                        xmin, ymin;
  gint                        xoff, yoff;

  ligma_preview_get_bounds (ligma_preview, &xmin, &ymin, NULL, NULL);
  ligma_preview_get_offsets (ligma_preview, &xoff, &yoff);

  image = ligma_item_get_image (LIGMA_ITEM (priv->drawable));

  if (ligma_selection_is_empty (image))
    {
      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (area),
                              x - xoff - xmin,
                              y - yoff - ymin,
                              width,
                              height,
                              ligma_drawable_type (priv->drawable),
                              buf, rowstride);
    }
  else
    {
      gint offset_x, offset_y;
      gint mask_x, mask_y;
      gint mask_width, mask_height;
      gint draw_x, draw_y;
      gint draw_width, draw_height;

      ligma_drawable_get_offsets (priv->drawable, &offset_x, &offset_y);

      if (ligma_drawable_mask_intersect (priv->drawable,
                                        &mask_x, &mask_y,
                                        &mask_width, &mask_height) &&
          ligma_rectangle_intersect (mask_x, mask_y,
                                    mask_width, mask_height,
                                    x, y, width, height,
                                    &draw_x, &draw_y,
                                    &draw_width, &draw_height))
        {
          LigmaImageType   type;
          LigmaSelection  *selection;
          guchar         *src;
          guchar         *sel;
          gint            d_w, d_h, d_bpp;
          gint            s_w, s_h, s_bpp;

          d_w = draw_width;
          d_h = draw_height;

          s_w = draw_width;
          s_h = draw_height;

          selection = ligma_image_get_selection (image);

          src = ligma_drawable_get_sub_thumbnail_data (priv->drawable,
                                                      draw_x, draw_y,
                                                      draw_width, draw_height,
                                                      &d_w, &d_h,
                                                      &d_bpp);

          sel = ligma_drawable_get_sub_thumbnail_data (LIGMA_DRAWABLE (selection),
                                                      draw_x + offset_x,
                                                      draw_y + offset_y,
                                                      draw_width, draw_height,
                                                      &s_w, &s_h,
                                                      &s_bpp);

          switch (d_bpp)
            {
            case 1:  type = LIGMA_GRAY_IMAGE;   break;
            case 2:  type = LIGMA_GRAYA_IMAGE;  break;
            case 3:  type = LIGMA_RGB_IMAGE;    break;
            case 4:  type = LIGMA_RGBA_IMAGE;   break;
            default:
              g_free (sel);
              g_free (src);
              return;
            }

          ligma_preview_area_mask (LIGMA_PREVIEW_AREA (area),
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
ligma_drawable_preview_draw_buffer (LigmaPreview  *preview,
                                   const guchar *buffer,
                                   gint          rowstride)
{
  gint x, y;
  gint width, height;

  ligma_preview_get_position (preview, &x, &y);
  ligma_preview_get_size (preview, &width, &height);

  ligma_drawable_preview_draw_area (LIGMA_DRAWABLE_PREVIEW (preview),
                                   x, y,
                                   width, height,
                                   buffer, rowstride);
}

static void
ligma_drawable_preview_set_drawable (LigmaDrawablePreview *drawable_preview,
                                    LigmaDrawable        *drawable)
{
  LigmaPreview                *preview = LIGMA_PREVIEW (drawable_preview);
  LigmaDrawablePreviewPrivate *priv    = GET_PRIVATE (preview);
  gint                        x1, y1, x2, y2;

  g_return_if_fail (priv->drawable == NULL);

  priv->drawable = drawable;

  _ligma_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2);

  ligma_preview_set_bounds (preview, x1, y1, x2, y2);

  if (ligma_drawable_is_indexed (drawable))
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (drawable));
      GtkWidget *area  = ligma_preview_get_area (preview);
      guchar    *cmap;
      gint       num_colors;

      cmap = ligma_image_get_colormap (image, &num_colors);
      ligma_preview_area_set_colormap (LIGMA_PREVIEW_AREA (area),
                                      cmap, num_colors);
      g_free (cmap);
    }
}


#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

gboolean
_ligma_drawable_preview_get_bounds (LigmaDrawable *drawable,
                                   gint         *xmin,
                                   gint         *ymin,
                                   gint         *xmax,
                                   gint         *ymax)
{
  gint     width;
  gint     height;
  gint     offset_x;
  gint     offset_y;
  gint     x1, y1;
  gint     x2, y2;
  gboolean retval;

  g_return_val_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)), FALSE);
  g_return_val_if_fail (ligma_item_is_drawable (LIGMA_ITEM (drawable)), FALSE);

  width  = ligma_drawable_get_width (drawable);
  height = ligma_drawable_get_height (drawable);

  retval = ligma_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  ligma_drawable_get_offsets (drawable, &offset_x, &offset_y);

  *xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  *ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  *xmax = MIN3 (x2 + SELECTION_BORDER, width,  width  + offset_x);
  *ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  return retval;
}


/**
 * ligma_drawable_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #LigmaDrawablePreview widget for @drawable.
 *
 * Returns: A pointer to the new #LigmaDrawablePreview widget.
 *
 * Since: 2.10
 **/
GtkWidget *
ligma_drawable_preview_new_from_drawable (LigmaDrawable *drawable)
{
  g_return_val_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (ligma_item_is_drawable (LIGMA_ITEM (drawable)), NULL);

  return g_object_new (LIGMA_TYPE_DRAWABLE_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * ligma_drawable_preview_get_drawable:
 * @preview:   a #LigmaDrawablePreview widget
 *
 * Returns: (transfer none): the drawable that has been passed to
 *          ligma_drawable_preview_new_from_drawable().
 *
 * Since: 2.10
 **/
LigmaDrawable *
ligma_drawable_preview_get_drawable (LigmaDrawablePreview *preview)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE_PREVIEW (preview), NULL);

  return GET_PRIVATE (preview)->drawable;
}
