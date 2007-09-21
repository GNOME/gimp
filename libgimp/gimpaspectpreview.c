/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpaspectpreview.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gimpuitypes.h"

#include "gimp.h"

#include "libgimp-intl.h"

#include "gimpaspectpreview.h"


enum
{
  PROP_0,
  PROP_DRAWABLE
};

typedef struct
{
  gboolean  update;
} PreviewSettings;


static GObject * gimp_aspect_preview_constructor (GType                  type,
                                                  guint                  n_params,
                                                  GObjectConstructParam *params);


static void  gimp_aspect_preview_get_property (GObject         *object,
                                               guint            property_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);
static void  gimp_aspect_preview_set_property (GObject         *object,
                                               guint            property_id,
                                               const GValue    *value,
                                               GParamSpec      *pspec);
static void  gimp_aspect_preview_destroy      (GtkObject       *object);
static void  gimp_aspect_preview_style_set    (GtkWidget       *widget,
                                               GtkStyle        *prev_style);
static void  gimp_aspect_preview_draw         (GimpPreview     *preview);
static void  gimp_aspect_preview_draw_buffer  (GimpPreview     *preview,
                                               const guchar    *buffer,
                                               gint             rowstride);
static void  gimp_aspect_preview_transform    (GimpPreview     *preview,
                                               gint             src_x,
                                               gint             src_y,
                                               gint            *dest_x,
                                               gint            *dest_y);
static void  gimp_aspect_preview_untransform  (GimpPreview     *preview,
                                               gint             src_x,
                                               gint             src_y,
                                               gint            *dest_x,
                                               gint            *dest_y);

static void  gimp_aspect_preview_set_drawable (GimpAspectPreview *preview,
                                               GimpDrawable      *drawable);


G_DEFINE_TYPE (GimpAspectPreview, gimp_aspect_preview, GIMP_TYPE_PREVIEW)

#define parent_class gimp_aspect_preview_parent_class

static gint gimp_aspect_preview_counter = 0;


static void
gimp_aspect_preview_class_init (GimpAspectPreviewClass *klass)
{
  GObjectClass     *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass   *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class    = GIMP_PREVIEW_CLASS (klass);

  object_class->constructor  = gimp_aspect_preview_constructor;
  object_class->get_property = gimp_aspect_preview_get_property;
  object_class->set_property = gimp_aspect_preview_set_property;

  gtk_object_class->destroy  = gimp_aspect_preview_destroy;

  widget_class->style_set    = gimp_aspect_preview_style_set;

  preview_class->draw        = gimp_aspect_preview_draw;
  preview_class->draw_buffer = gimp_aspect_preview_draw_buffer;
  preview_class->transform   = gimp_aspect_preview_transform;
  preview_class->untransform = gimp_aspect_preview_untransform;

  /**
   * GimpAspectPreview:drawable:
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_pointer ("drawable", NULL, NULL,
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

static GObject *
gimp_aspect_preview_constructor (GType                  type,
                                 guint                  n_params,
                                 GObjectConstructParam *params)
{
  GObject         *object;
  gchar           *data_name;
  PreviewSettings  settings;

  data_name = g_strdup_printf ("%s-aspect-preview-%d",
                               g_get_prgname (),
                               gimp_aspect_preview_counter++);

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  if (gimp_get_data (data_name, &settings))
    {
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
    }

  g_object_set_data_full (object, "gimp-aspect-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  return object;
}

static void
gimp_aspect_preview_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpAspectPreview *preview = GIMP_ASPECT_PREVIEW (object);

  switch (property_id)
    {
    case PROP_DRAWABLE:
      g_value_set_pointer (value, preview->drawable);
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
      gimp_aspect_preview_set_drawable (preview,
                                        g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_aspect_preview_destroy (GtkObject *object)
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

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_aspect_preview_style_set (GtkWidget *widget,
                               GtkStyle  *prev_style)
{
  GimpPreview  *preview  = GIMP_PREVIEW (widget);
  GimpDrawable *drawable = GIMP_ASPECT_PREVIEW (preview)->drawable;
  gint          size;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "size", &size,
                        NULL);

  if (drawable->width > drawable->height)
    {
      preview->width  = MIN (drawable->width, size);
      preview->height = (drawable->height * preview->width) / drawable->width;
    }
  else
    {
      preview->height = MIN (drawable->height, size);
      preview->width  = (drawable->width * preview->height) / drawable->height;
    }

  gtk_widget_set_size_request (preview->area,
                               preview->width, preview->height);
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
  GimpDrawable *drawable = GIMP_ASPECT_PREVIEW (preview)->drawable;
  gint32        image_id;

  image_id = gimp_drawable_get_image (drawable->drawable_id);

  if (gimp_selection_is_empty (image_id))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                              0, 0,
                              preview->width, preview->height,
                              gimp_drawable_type (drawable->drawable_id),
                              buffer,
                              rowstride);
    }
  else
    {
      guchar *sel;
      guchar *src;
      gint    selection_id;
      gint    width, height;
      gint    bpp;

      selection_id = gimp_image_get_selection (image_id);

      width  = preview->width;
      height = preview->height;

      src = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                              &width, &height, &bpp);
      sel = gimp_drawable_get_thumbnail_data (selection_id,
                                              &width, &height, &bpp);

      gimp_preview_area_mask (GIMP_PREVIEW_AREA (preview->area),
                              0, 0, preview->width, preview->height,
                              gimp_drawable_type (drawable->drawable_id),
                              src, width * drawable->bpp,
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
  GimpDrawable *drawable = GIMP_ASPECT_PREVIEW (preview)->drawable;

  *dest_x = (gdouble) src_x * preview->width / drawable->width;
  *dest_y = (gdouble) src_y * preview->height / drawable->height;
}

static void
gimp_aspect_preview_untransform (GimpPreview *preview,
                                 gint         src_x,
                                 gint         src_y,
                                 gint        *dest_x,
                                 gint        *dest_y)
{
  GimpDrawable *drawable = GIMP_ASPECT_PREVIEW (preview)->drawable;

  *dest_x = (gdouble) src_x * drawable->width / preview->width;
  *dest_y = (gdouble) src_y * drawable->height / preview->height;
}

static void
gimp_aspect_preview_set_drawable (GimpAspectPreview *preview,
                                  GimpDrawable      *drawable)
{
  gint width;
  gint height;

  preview->drawable = drawable;

  if (drawable->width > drawable->height)
    {
      width  = MIN (drawable->width, 512);
      height = (drawable->height * width) / drawable->width;
    }
  else
    {
      height = MIN (drawable->height, 512);
      width  = (drawable->width * height) / drawable->height;
    }
  gimp_preview_set_bounds (GIMP_PREVIEW (preview), 0, 0, width, height);

  if (height > 0)
    g_object_set (GIMP_PREVIEW (preview)->frame,
                  "ratio",
                  (gdouble) drawable->width / (gdouble) drawable->height,
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
 * Since: GIMP 2.2
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
