/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpaspectpreview.c
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"
#include "gimppdb-private.h"

#include "libgimp-intl.h"

#include "gimpaspectpreview.h"


/**
 * SECTION: gimpaspectpreview
 * @title: GimpAspectPreview
 * @short_description: A widget providing a preview with fixed aspect ratio.
 *
 * A widget providing a preview with fixed aspect ratio.
 **/


enum
{
  PROP_0,
  PROP_DRAWABLE
};

typedef struct _GimpAspectPreviewPrivate
{
  GimpDrawable *drawable;
} GimpAspectPreviewPrivate;

typedef struct
{
  gboolean  update;
} PreviewSettings;

#define GET_PRIVATE(obj) (gimp_aspect_preview_get_instance_private ((GimpAspectPreview *) (obj)))


static void  gimp_aspect_preview_constructed   (GObject         *object);
static void  gimp_aspect_preview_dispose       (GObject         *object);
static void  gimp_aspect_preview_get_property  (GObject         *object,
                                                guint            property_id,
                                                GValue          *value,
                                                GParamSpec      *pspec);
static void  gimp_aspect_preview_set_property  (GObject         *object,
                                                guint            property_id,
                                                const GValue    *value,
                                                GParamSpec      *pspec);

static void  gimp_aspect_preview_style_updated (GtkWidget       *widget);
static void  gimp_aspect_preview_draw          (GimpPreview     *preview);
static void  gimp_aspect_preview_draw_buffer   (GimpPreview     *preview,
                                                const guchar    *buffer,
                                                gint             rowstride);
static void  gimp_aspect_preview_transform     (GimpPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);
static void  gimp_aspect_preview_untransform   (GimpPreview     *preview,
                                                gint             src_x,
                                                gint             src_y,
                                                gint            *dest_x,
                                                gint            *dest_y);

static void  gimp_aspect_preview_set_drawable  (GimpAspectPreview *preview,
                                                GimpDrawable      *drawable);


G_DEFINE_TYPE_WITH_PRIVATE (GimpAspectPreview, gimp_aspect_preview,
                            GIMP_TYPE_PREVIEW)

#define parent_class gimp_aspect_preview_parent_class

static gint gimp_aspect_preview_counter = 0;


static void
gimp_aspect_preview_class_init (GimpAspectPreviewClass *klass)
{
  GObjectClass     *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class  = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class = GIMP_PREVIEW_CLASS (klass);

  object_class->constructed   = gimp_aspect_preview_constructed;
  object_class->dispose       = gimp_aspect_preview_dispose;
  object_class->get_property  = gimp_aspect_preview_get_property;
  object_class->set_property  = gimp_aspect_preview_set_property;

  widget_class->style_updated = gimp_aspect_preview_style_updated;

  preview_class->draw         = gimp_aspect_preview_draw;
  preview_class->draw_buffer  = gimp_aspect_preview_draw_buffer;
  preview_class->transform    = gimp_aspect_preview_transform;
  preview_class->untransform  = gimp_aspect_preview_untransform;

  /**
   * GimpAspectPreview:drawable-id:
   *
   * The drawable the #GimpAspectPreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_object ("drawable",
                                                        "Drawable",
                                                        "The drawable this preview is attached to",
                                                        GIMP_TYPE_DRAWABLE,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_aspect_preview_init (GimpAspectPreview *preview)
{
  g_object_set (gimp_preview_get_area (GIMP_PREVIEW (preview)),
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                "check-custom-color1", gimp_check_custom_color1 (),
                "check-custom-color2", gimp_check_custom_color2 (),
                NULL);
}

static void
gimp_aspect_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;
  GBytes          *settings_bytes = NULL;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-aspect-preview-%d",
                               g_get_prgname (),
                               gimp_aspect_preview_counter++);

  if (gimp_pdb_get_data (data_name, &settings_bytes) &&
      g_bytes_get_size (settings_bytes) == sizeof (PreviewSettings))
    {
      settings = *((PreviewSettings *) g_bytes_get_data (settings_bytes, NULL));
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
    }
  g_bytes_unref (settings_bytes);

  g_object_set_data_full (object, "gimp-aspect-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
gimp_aspect_preview_dispose (GObject *object)
{
  GimpAspectPreviewPrivate *priv = GET_PRIVATE (object);
  const gchar              *data_name;

  data_name = g_object_get_data (G_OBJECT (object),
                                 "gimp-aspect-preview-data-name");
  if (data_name)
    {
      GimpPreview     *preview = GIMP_PREVIEW (object);
      GBytes          *bytes;
      PreviewSettings  settings;

      settings.update = gimp_preview_get_update (preview);

      bytes = g_bytes_new_static (&settings, sizeof (PreviewSettings));
      gimp_pdb_set_data (data_name, bytes);
      g_bytes_unref (bytes);
    }

  g_clear_object (&priv->drawable);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_aspect_preview_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpAspectPreview        *preview = GIMP_ASPECT_PREVIEW (object);
  GimpAspectPreviewPrivate *priv    = GET_PRIVATE (preview);

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
gimp_aspect_preview_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpAspectPreview *preview = GIMP_ASPECT_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      gimp_aspect_preview_set_drawable (preview, g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_aspect_preview_style_updated (GtkWidget *widget)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  GtkWidget   *area    = gimp_preview_get_area (preview);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (area)
    {
      GimpAspectPreviewPrivate *priv = GET_PRIVATE (preview);
      gint                      width;
      gint                      height;
      gint                      preview_width;
      gint                      preview_height;
      gint                      size;

      width  = gimp_drawable_get_width  (priv->drawable);
      height = gimp_drawable_get_height (priv->drawable);

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

      gimp_preview_set_size (preview, preview_width, preview_height);
    }
}

static void
gimp_aspect_preview_draw (GimpPreview *preview)
{
  GtkWidget *area = gimp_preview_get_area (preview);
  gint       width;
  gint       height;

  gimp_preview_get_size (preview, &width, &height);

  gimp_preview_area_fill (GIMP_PREVIEW_AREA (area),
                          0, 0,
                          width, height,
                          0, 0, 0);
}

static void
gimp_aspect_preview_draw_buffer (GimpPreview  *preview,
                                 const guchar *buffer,
                                 gint          rowstride)
{
  GimpAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  GtkWidget                *area = gimp_preview_get_area (preview);
  GimpImage                *image;
  gint                      width;
  gint                      height;

  gimp_preview_get_size (preview, &width, &height);

  image = gimp_item_get_image (GIMP_ITEM (priv->drawable));

  if (gimp_selection_is_empty (image))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (area),
                              0, 0,
                              width, height,
                              gimp_drawable_type (priv->drawable),
                              buffer,
                              rowstride);
    }
  else
    {
      GBytes        *sel;
      GBytes        *src;
      GimpSelection *selection;
      gint           w, h;
      gint           bpp;

      selection = gimp_image_get_selection (image);

      src = gimp_drawable_get_thumbnail_data (priv->drawable,
                                              width, height,
                                              &w, &h, &bpp);
      sel = gimp_drawable_get_thumbnail_data (GIMP_DRAWABLE (selection),
                                              width, height,
                                              &w, &h, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (area),
                              0, 0, width, height,
                              gimp_drawable_type (priv->drawable),
                              g_bytes_get_data (src, NULL),
                              width * gimp_drawable_get_bpp (priv->drawable),
                              buffer, rowstride,
                              g_bytes_get_data (sel, NULL), width);

      g_bytes_unref (sel);
      g_bytes_unref (src);
    }
}

static void
gimp_aspect_preview_transform (GimpPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  GimpAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      width;
  gint                      height;

  gimp_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * width  / gimp_drawable_get_width  (priv->drawable);
  *dest_y = (gdouble) src_y * height / gimp_drawable_get_height (priv->drawable);
}

static void
gimp_aspect_preview_untransform (GimpPreview *preview,
                                 gint         src_x,
                                 gint         src_y,
                                 gint        *dest_x,
                                 gint        *dest_y)
{
  GimpAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      width;
  gint                      height;

  gimp_preview_get_size (preview, &width, &height);

  *dest_x = (gdouble) src_x * gimp_drawable_get_width  (priv->drawable) / width;
  *dest_y = (gdouble) src_y * gimp_drawable_get_height (priv->drawable) / height;
}

static void
gimp_aspect_preview_set_drawable (GimpAspectPreview *preview,
                                  GimpDrawable      *drawable)
{
  GimpAspectPreviewPrivate *priv = GET_PRIVATE (preview);
  gint                      d_width;
  gint                      d_height;
  gint                      width;
  gint                      height;

  g_return_if_fail (priv->drawable == NULL);

  priv->drawable = drawable;

  d_width  = gimp_drawable_get_width  (priv->drawable);
  d_height = gimp_drawable_get_height (priv->drawable);

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

  gimp_preview_set_bounds (GIMP_PREVIEW (preview), 0, 0, width, height);

  if (height > 0)
    g_object_set (gimp_preview_get_frame (GIMP_PREVIEW (preview)),
                  "ratio",
                  (gdouble) d_width / (gdouble) d_height,
                  NULL);
}

/**
 * gimp_aspect_preview_new_from_drawable:
 * @drawable: (transfer none): a drawable
 *
 * Creates a new #GimpAspectPreview widget for @drawable_. See also
 * gimp_drawable_preview_new_from_drawable().
 *
 * Since: 2.10
 *
 * Returns: a new #GimpAspectPreview.
 **/
GtkWidget *
gimp_aspect_preview_new_from_drawable (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_valid (GIMP_ITEM (drawable)), NULL);

  return g_object_new (GIMP_TYPE_ASPECT_PREVIEW,
                       "drawable", drawable,
                       NULL);
}
