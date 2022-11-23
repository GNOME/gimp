/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaaspectpreview.c
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

#include "libligma-intl.h"

#include "ligmaaspectpreview.h"


/**
 * SECTION: ligmaaspectpreview
 * @title: LigmaAspectPreview
 * @short_description: A widget providing a preview with fixed aspect ratio.
 *
 * A widget providing a preview with fixed aspect ratio.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE
};

struct _LigmaAspectPreviewPrivate
{
  LigmaDrawable *drawable;
};

typedef struct
{
  gboolean  update;
} PreviewSettings;

#define GET_PRIVATE(obj) (((LigmaAspectPreview *) (obj))->priv)


static void  ligma_aspect_preview_constructed   (GObject         *object);
static void  ligma_aspect_preview_dispose       (GObject         *object);
static void  ligma_aspect_preview_get_property  (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);
static void  ligma_aspect_preview_set_property  (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);

static void  ligma_aspect_preview_style_updated (GtkWidget       *widget);
static void  ligma_aspect_preview_draw          (LigmaPreview     *preview);
static void  ligma_aspect_preview_draw_buffer   (LigmaPreview     *preview,
                                                const guchar    *buffer,
                                                gint             rowstride);
static void  ligma_aspect_preview_transform     (LigmaPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);
static void  ligma_aspect_preview_untransform   (LigmaPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);

static void  ligma_aspect_preview_set_drawable  (LigmaAspectPreview *preview,
                                                LigmaDrawable      *drawable);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaAspectPreview, ligma_aspect_preview,
                            LIGMA_TYPE_PREVIEW)

#define parent_class ligma_aspect_preview_parent_class

static gint ligma_aspect_preview_counter = 0;


static void
ligma_aspect_preview_class_init (LigmaAspectPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  LigmaPreviewClass *preview_class = LIGMA_PREVIEW_CLASS (klass);

  object_class->constructed   = ligma_aspect_preview_constructed;
  object_class->dispose       = ligma_aspect_preview_dispose;
  object_class->get_property  = ligma_aspect_preview_get_property;
  object_class->set_property  = ligma_aspect_preview_set_property;

  widget_class->style_updated = ligma_aspect_preview_style_updated;

  preview_class->draw         = ligma_aspect_preview_draw;
  preview_class->draw_buffer  = ligma_aspect_preview_draw_buffer;
  preview_class->transform    = ligma_aspect_preview_transform;
  preview_class->untransform  = ligma_aspect_preview_untransform;

  /**
   * LigmaAspectPreview:drawable-id:
   *
   * The drawable the #LigmaAspectPreview is attached to.
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
ligma_aspect_preview_init (LigmaAspectPreview *preview)
{
  preview->priv = ligma_aspect_preview_get_instance_private (preview);

  g_object_set (ligma_preview_get_area (LIGMA_PREVIEW (preview)),
                "check-size", ligma_check_size (),
                "check-type", ligma_check_type (),
                "check-custom-color1", ligma_check_custom_color1 (),
                "check-custom-color2", ligma_check_custom_color2 (),
                NULL);
}

static void
ligma_aspect_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-aspect-preview-%d",
                               g_get_prgname (),
                               ligma_aspect_preview_counter++);

  if (ligma_get_data (data_name, &settings))
    {
      ligma_preview_set_update (LIGMA_PREVIEW (object), settings.update);
    }

  g_object_set_data_full (object, "ligma-aspect-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
ligma_aspect_preview_dispose (GObject *object)
{
  LigmaAspectPreviewPrivate *priv = GET_PRIVATE (object);
  const gchar              *data_name;

  data_name = g_object_get_data (G_OBJECT (object),
                                 "ligma-aspect-preview-data-name");
  if (data_name)
    {
      LigmaPreview     *preview = LIGMA_PREVIEW (object);
      PreviewSettings  settings;

      settings.update = ligma_preview_get_update (preview);

      ligma_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  g_clear_object (&priv->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_aspect_preview_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaAspectPreview        *preview = LIGMA_ASPECT_PREVIEW (object);
  LigmaAspectPreviewPrivate *priv    = GET_PRIVATE (preview);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_object (value, priv->drawable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_aspect_preview_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaAspectPreview *preview = LIGMA_ASPECT_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      ligma_aspect_preview_set_drawable (preview, g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_aspect_preview_style_updated (GtkWidget *widget)
{
  LigmaPreview *preview = LIGMA_PREVIEW (widget);
  GtkWidget   *area    = ligma_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      LigmaAspectPreviewPrivate *priv = GET_PRIVATE (preview);
      gint                      width;
      gint                      height;
      gint                      preview_width;
      gint                      preview_height;
      gint                      size;

      width  = ligma_drawable_get_width  (priv->drawable);
      height = ligma_drawable_get_height (priv->drawable);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      if (width > height)
        {
          preview_width  = MIN (width, size);
          preview_height = (height * preview_width) / width;
        }
      else
        {
          preview_height = MIN (height, size);
          preview_width  = (width * preview_height) / height;
        }

      ligma_preview_set_size (preview, preview_width, preview_height);
    }
}

static void
ligma_aspect_preview_draw (LigmaPreview *preview)
{
  GtkWidget *area = ligma_preview_get_area (preview);
  gint       width;
  gint       height;

  ligma_preview_get_size (preview, &width, &height);

  ligma_preview_area_fill (LIGMA_PREVIEW_AREA (area),
                          0, 0,
                          width, height,
                          0, 0, 0);
}

static void
ligma_aspect_preview_draw_buffer (LigmaPreview  *preview,
                                 const guchar *buffer,
                                 gint          rowstride)
{
  LigmaAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                *area = ligma_preview_get_area (preview);
  LigmaImage                *image;
  gint                      width;
  gint                      height;

  ligma_preview_get_size (preview, &width, &height);

  image = ligma_item_get_image (LIGMA_ITEM (priv->drawable));

  if (ligma_selection_is_empty (image))
    {
      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              ligma_drawable_type (priv->drawable),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar        *sel;
      guchar        *src;
      LigmaSelection *selection;
      gint           w, h;
      gint           bpp;

      selection = ligma_image_get_selection (image);

      w = width;
      h = height;

      src = ligma_drawable_get_thumbnail_data (priv->drawable,
                                              &w, &h, &bpp);
      sel = ligma_drawable_get_thumbnail_data (LIGMA_DRAWABLE (selection),
                                              &w, &h, &bpp);

      ligma_preview_area_mask (LIGMA_PREVIEW_AREA (area),
                              0, 0, width, height,
                              ligma_drawable_type (priv->drawable),
                              src, width * ligma_drawable_get_bpp (priv->drawable),
                              buffer, rowstride,
                              sel, width);

      g_free (sel);
      g_free (src);
    }
}

static void
ligma_aspect_preview_transform (LigmaPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  LigmaAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      width;
  gint                      height;

  ligma_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * width  / ligma_drawable_get_width  (priv->drawable);
  *dest_y = (gdouble) src_y * height / ligma_drawable_get_height (priv->drawable);
}

static void
ligma_aspect_preview_untransform (LigmaPreview *preview,
                                 gint         src_x,
                                 gint         src_y,
                                 gint        *dest_x,
                                 gint        *dest_y)
{
  LigmaAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      width;
  gint                      height;

  ligma_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * ligma_drawable_get_width  (priv->drawable) / width;
  *dest_y = (gdouble) src_y * ligma_drawable_get_height (priv->drawable) / height;
}

static void
ligma_aspect_preview_set_drawable (LigmaAspectPreview *preview,
                                  LigmaDrawable      *drawable)
{
  LigmaAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      d_width;
  gint                      d_height;
  gint                      width;
  gint                      height;

  g_return_if_fail (priv->drawable == NULL);

  priv->drawable = drawable;

  d_width  = ligma_drawable_get_width  (priv->drawable);
  d_height = ligma_drawable_get_height (priv->drawable);

  if (d_width > d_height)
    {
      width  = MIN (d_width, 512);
      height = (d_height * width) / d_width;
    }
  else
    {
      height = MIN (d_height, 512);
      width  = (d_width * height) / d_height;
    }

  ligma_preview_set_bounds (LIGMA_PREVIEW (preview), 0, 0, width, height);

  if (height > 0)
    g_object_set (ligma_preview_get_frame (LIGMA_PREVIEW (preview)),
                  "ratio",
                  (gdouble) d_width / (gdouble) d_height,
                  NULL);
}

/**
 * ligma_aspect_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #LigmaAspectPreview widget for @drawable_. See also
 * ligma_drawable_preview_new_from_drawable().
 *
 * Since: 2.10
 *
 * Returns: a new #LigmaAspectPreview.
 **/
GtkWidget *
ligma_aspect_preview_new_from_drawable (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_valid (LIGMA_ITEM (drawable)), NULL);

  return g_object_new (LIGMA_TYPE_ASPECT_PREVIEW,
                       "drawable", drawable,
                       NULL);
}
