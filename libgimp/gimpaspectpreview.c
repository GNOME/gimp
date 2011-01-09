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
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

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
  PROP_DRAWABLE,
  PROP_DRAWABLE_ID
};

typedef struct
{
  gint32 drawable_ID;
} GimpAspectPreviewPrivate;

typedef struct
{
  gboolean  update;
} PreviewSettings;


#define GIMP_ASPECT_PREVIEW_GET_PRIVATE(obj) \
  G_TYPE_INSTANCE_GET_PRIVATE (preview, \
                               GIMP_TYPE_ASPECT_PREVIEW, \
                               GimpAspectPreviewPrivate)

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
static void  gimp_aspect_preview_set_drawable_id
                                               (GimpAspectPreview *preview,
                                                gint32             drawable_ID);


G_DEFINE_TYPE (GimpAspectPreview, gimp_aspect_preview, GIMP_TYPE_PREVIEW)

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

  g_type_class_add_private (object_class, sizeof (GimpAspectPreviewPrivate));

  /**
   * GimpAspectPreview:drawable:
   *
   * Deprecated: use the drawable-id property instead.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_pointer ("drawable", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  /**
   * GimpAspectPreview:drawable-id:
   *
   * The drawable the #GimpAspectPreview is attached to.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE_ID,
                                   g_param_spec_int ("drawable-id", NULL, NULL,
                                                     -1, G_MAXINT, -1,
                                                     GIMP_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_aspect_preview_init (GimpAspectPreview *preview)
{
  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static void
gimp_aspect_preview_constructed (GObject *object)
{
  gchar           *data_name;
  PreviewSettings  settings;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  data_name = g_strdup_printf ("%s-aspect-preview-%d",
                               g_get_prgname (),
                               gimp_aspect_preview_counter++);

  if (gimp_get_data (data_name, &settings))
    {
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
    }

  g_object_set_data_full (object, "gimp-aspect-preview-data-name",
                          data_name, (GDestroyNotify) g_free);
}

static void
gimp_aspect_preview_dispose (GObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                              "gimp-aspect-preview-data-name");

  if (data_name)
    {
      GimpPreview     *preview = GIMP_PREVIEW (object);
      PreviewSettings  settings;

      settings.update = gimp_preview_get_update (preview);

      gimp_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_aspect_preview_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpAspectPreview        *preview = GIMP_ASPECT_PREVIEW (object);
  GimpAspectPreviewPrivate *priv    = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_pointer (value, preview->drawable);
      break;

    case PROP_DRAWABLE_ID:
      g_value_set_int (value, priv->drawable_ID);
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
  GimpAspectPreview        *preview = GIMP_ASPECT_PREVIEW (object);
  GimpAspectPreviewPrivate *priv    = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_return_if_fail (priv->drawable_ID < 1);
      if (g_value_get_pointer (value))
        gimp_aspect_preview_set_drawable (preview,
                                          g_value_get_pointer (value));
      break;

    case PROP_DRAWABLE_ID:
      gimp_aspect_preview_set_drawable_id (preview,
                                           g_value_get_int (value));
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

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  if (preview->area)
    {
      GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);
      gint                      width;
      gint                      height;
      gint                      size;

      width  = gimp_drawable_width  (priv->drawable_ID);
      height = gimp_drawable_height (priv->drawable_ID);

      gtk_widget_style_get (widget,
                            "size", &size,
                            NULL);

      if (width > height)
        {
          preview->width  = MIN (width, size);
          preview->height = (height * preview->width) / width;
        }
      else
        {
          preview->height = MIN (height, size);
          preview->width  = (width * preview->height) / height;
        }

      gtk_widget_set_size_request (preview->area,
                                   preview->width, preview->height);
    }
}

static void
gimp_aspect_preview_draw (GimpPreview *preview)
{
  g_return_if_fail (GIMP_IS_ASPECT_PREVIEW (preview));

  gimp_preview_area_fill (GIMP_PREVIEW_AREA (preview->area),
                          0, 0,
                          preview->width,
                          preview->height,
                          0, 0, 0);
}

static void
gimp_aspect_preview_draw_buffer (GimpPreview  *preview,
                                 const guchar *buffer,
                                 gint          rowstride)
{
  GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);
  gint32                    image_ID;

  image_ID = gimp_item_get_image (priv->drawable_ID);

  if (gimp_selection_is_empty (image_ID))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                              0, 0,
                              preview->width, preview->height,
                              gimp_drawable_type (priv->drawable_ID),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar *sel;
      guchar *src;
      gint    selection_ID;
      gint    width, height;
      gint    bpp;

      selection_ID = gimp_image_get_selection (image_ID);

      width  = preview->width;
      height = preview->height;

      src = gimp_drawable_get_thumbnail_data (priv->drawable_ID,
                                              &width, &height, &bpp);
      sel = gimp_drawable_get_thumbnail_data (selection_ID,
                                              &width, &height, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (preview->area),
                              0, 0, preview->width, preview->height,
                              gimp_drawable_type (priv->drawable_ID),
                              src, width * gimp_drawable_bpp (priv->drawable_ID),
                              buffer, rowstride,
                              sel, width);

      g_free (sel);
      g_free (src);
    }
}

static void
gimp_aspect_preview_transform (GimpPreview *preview,
                               gint         src_x,
                               gint         src_y,
                               gint        *dest_x,
                               gint        *dest_y)
{
  GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);

  *dest_x = (gdouble) src_x * preview->width  / gimp_drawable_width  (priv->drawable_ID);
  *dest_y = (gdouble) src_y * preview->height / gimp_drawable_height (priv->drawable_ID);
}

static void
gimp_aspect_preview_untransform (GimpPreview *preview,
                                 gint         src_x,
                                 gint         src_y,
                                 gint        *dest_x,
                                 gint        *dest_y)
{
  GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);

  *dest_x = (gdouble) src_x * gimp_drawable_width  (priv->drawable_ID) / preview->width;
  *dest_y = (gdouble) src_y * gimp_drawable_height (priv->drawable_ID) / preview->height;
}

static void
gimp_aspect_preview_set_drawable (GimpAspectPreview *preview,
                                  GimpDrawable      *drawable)
{
  GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);

  g_return_if_fail (preview->drawable == NULL);
  g_return_if_fail (priv->drawable_ID < 1);

  preview->drawable = drawable;

  gimp_aspect_preview_set_drawable_id (preview, drawable->drawable_id);
}

static void
gimp_aspect_preview_set_drawable_id (GimpAspectPreview *preview,
                                     gint32             drawable_ID)
{
  GimpAspectPreviewPrivate *priv = GIMP_ASPECT_PREVIEW_GET_PRIVATE (preview);
  gint                      d_width;
  gint                      d_height;
  gint                      width;
  gint                      height;

  g_return_if_fail (priv->drawable_ID < 1);

  priv->drawable_ID = drawable_ID;

  d_width  = gimp_drawable_width  (priv->drawable_ID);
  d_height = gimp_drawable_height (priv->drawable_ID);

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
    g_object_set (GIMP_PREVIEW (preview)->frame,
                  "ratio",
                  (gdouble) d_width / (gdouble) d_height,
                  NULL);
}

/**
 * gimp_aspect_preview_new_from_drawable_id:
 * @drawable_ID: a drawable ID
 *
 * Creates a new #GimpAspectPreview widget for @drawable_ID. See also
 * gimp_drawable_preview_new_from_drawable_id().
 *
 * Since: 2.10
 *
 * Returns: a new #GimpAspectPreview.
 **/
GtkWidget *
gimp_aspect_preview_new_from_drawable_id (gint32 drawable_ID)
{
  g_return_val_if_fail (gimp_item_is_valid (drawable_ID), NULL);
  g_return_val_if_fail (gimp_item_is_drawable (drawable_ID), NULL);

  return g_object_new (GIMP_TYPE_ASPECT_PREVIEW,
                       "drawable-id", drawable_ID,
                       NULL);
}
/**
 * gimp_aspect_preview_new:
 * @drawable: a #GimpDrawable
 * @toggle:   unused
 *
 * Creates a new #GimpAspectPreview widget for @drawable. See also
 * gimp_drawable_preview_new().
 *
 * In GIMP 2.2 the @toggle parameter was provided to conviently access
 * the state of the "Preview" check-button. This is not any longer
 * necessary as the preview itself now stores this state, as well as
 * the scroll offset.
 *
 * Since: 2.2
 *
 * Returns: a new #GimpAspectPreview.
 **/
GtkWidget *
gimp_aspect_preview_new (GimpDrawable *drawable,
                         gboolean     *toggle)
{
  g_return_val_if_fail (drawable != NULL, NULL);

  return g_object_new (GIMP_TYPE_ASPECT_PREVIEW,
                       "drawable", drawable,
                       NULL);
}
