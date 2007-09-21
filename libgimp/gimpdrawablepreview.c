/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpdrawablepreview.c
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

#include "gimpdrawablepreview.h"


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


static GObject * gimp_drawable_preview_constructor (GType                  type,
                                                    guint                  n_params,
                                                    GObjectConstructParam *params);

static void  gimp_drawable_preview_get_property  (GObject         *object,
                                                  guint            property_id,
                                                  GValue          *value,
                                                  GParamSpec      *pspec);
static void  gimp_drawable_preview_set_property  (GObject         *object,
                                                  guint            property_id,
                                                  const GValue    *value,
                                                  GParamSpec      *pspec);
static void  gimp_drawable_preview_destroy       (GtkObject       *object);

static void  gimp_drawable_preview_style_set     (GtkWidget       *widget,
                                                  GtkStyle        *prev_style);

static void  gimp_drawable_preview_draw_original (GimpPreview     *preview);
static void  gimp_drawable_preview_draw_thumb    (GimpPreview     *preview,
                                                  GimpPreviewArea *area,
                                                  gint             width,
                                                  gint             height);
static void  gimp_drawable_preview_draw_buffer   (GimpPreview     *preview,
                                                  const guchar    *buffer,
                                                  gint             rowstride);

static void  gimp_drawable_preview_set_drawable (GimpDrawablePreview *preview,
                                                 GimpDrawable        *drawable);


G_DEFINE_TYPE (GimpDrawablePreview, gimp_drawable_preview,
               GIMP_TYPE_SCROLLED_PREVIEW)

#define parent_class gimp_drawable_preview_parent_class

static gint gimp_drawable_preview_counter = 0;


static void
gimp_drawable_preview_class_init (GimpDrawablePreviewClass *klass)
{
  GObjectClass     *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass   *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkWidgetClass   *widget_class     = GTK_WIDGET_CLASS (klass);
  GimpPreviewClass *preview_class    = GIMP_PREVIEW_CLASS (klass);

  object_class->constructor  = gimp_drawable_preview_constructor;
  object_class->get_property = gimp_drawable_preview_get_property;
  object_class->set_property = gimp_drawable_preview_set_property;

  gtk_object_class->destroy  = gimp_drawable_preview_destroy;

  widget_class->style_set    = gimp_drawable_preview_style_set;

  preview_class->draw        = gimp_drawable_preview_draw_original;
  preview_class->draw_thumb  = gimp_drawable_preview_draw_thumb;
  preview_class->draw_buffer = gimp_drawable_preview_draw_buffer;

  /**
   * GimpDrawablePreview:drawable:
   *
   * Since: GIMP 2.4
   */
  g_object_class_install_property (object_class, PROP_DRAWABLE,
                                   g_param_spec_pointer ("drawable", NULL, NULL,
                                                         GIMP_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_drawable_preview_init (GimpDrawablePreview *preview)
{
  g_object_set (GIMP_PREVIEW (preview)->area,
                "check-size", gimp_check_size (),
                "check-type", gimp_check_type (),
                NULL);
}

static GObject *
gimp_drawable_preview_constructor (GType                  type,
                                   guint                  n_params,
                                   GObjectConstructParam *params)
{
  GObject         *object;
  gchar           *data_name;
  PreviewSettings  settings;

  data_name = g_strdup_printf ("%s-drawable-preview-%d",
                               g_get_prgname (),
                               ++gimp_drawable_preview_counter);

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  if (gimp_get_data (data_name, &settings))
    {
      gimp_preview_set_update (GIMP_PREVIEW (object), settings.update);
      gimp_scrolled_preview_set_position (GIMP_SCROLLED_PREVIEW (object),
                                          settings.x, settings.y);
    }

  g_object_set_data_full (object, "gimp-drawable-preview-data-name",
                          data_name, (GDestroyNotify) g_free);

  return object;
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
    case PROP_DRAWABLE:
      g_value_set_pointer (value,
                           gimp_drawable_preview_get_drawable (preview));
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
    case PROP_DRAWABLE:
      gimp_drawable_preview_set_drawable (preview,
                                          g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_drawable_preview_destroy (GtkObject *object)
{
  const gchar *data_name = g_object_get_data (G_OBJECT (object),
                                              "gimp-drawable-preview-data-name");

  if (data_name)
    {
      GimpPreview     *preview = GIMP_PREVIEW (object);
      PreviewSettings  settings;

      settings.x      = preview->xoff + preview->xmin;
      settings.y      = preview->yoff + preview->ymin;
      settings.update = gimp_preview_get_update (preview);

      gimp_set_data (data_name, &settings, sizeof (PreviewSettings));
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_drawable_preview_style_set (GtkWidget *widget,
                                 GtkStyle  *prev_style)
{
  GimpPreview *preview = GIMP_PREVIEW (widget);
  gint         width   = preview->xmax - preview->xmin;
  gint         height  = preview->ymax - preview->ymin;
  gint         size;

  if (GTK_WIDGET_CLASS (parent_class)->style_set)
    GTK_WIDGET_CLASS (parent_class)->style_set (widget, prev_style);

  gtk_widget_style_get (widget,
                        "size", &size,
                        NULL);

  gtk_widget_set_size_request (GIMP_PREVIEW (preview)->area,
                               MIN (width, size), MIN (height, size));
}

static void
gimp_drawable_preview_draw_original (GimpPreview *preview)
{
  GimpDrawablePreview *drawable_preview = GIMP_DRAWABLE_PREVIEW (preview);
  GimpDrawable        *drawable         = drawable_preview->drawable;
  guchar              *buffer;
  GimpPixelRgn         srcPR;
  guint                rowstride;

  if (! drawable)
    return;

  rowstride = preview->width * drawable->bpp;
  buffer    = g_new (guchar, rowstride * preview->height);

  preview->xoff = CLAMP (preview->xoff,
                         0, preview->xmax - preview->xmin - preview->width);
  preview->yoff = CLAMP (preview->yoff,
                         0, preview->ymax - preview->ymin - preview->height);

  gimp_pixel_rgn_init (&srcPR, drawable,
                       preview->xoff + preview->xmin,
                       preview->yoff + preview->ymin,
                       preview->width, preview->height,
                       FALSE, FALSE);

  gimp_pixel_rgn_get_rect (&srcPR, buffer,
                           preview->xoff + preview->xmin,
                           preview->yoff + preview->ymin,
                           preview->width, preview->height);

  gimp_preview_area_draw (GIMP_PREVIEW_AREA (preview->area),
                          0, 0, preview->width, preview->height,
                          gimp_drawable_type (drawable->drawable_id),
                          buffer,
                          rowstride);
  g_free (buffer);
}

static void
gimp_drawable_preview_draw_thumb (GimpPreview     *preview,
                                  GimpPreviewArea *area,
                                  gint             width,
                                  gint             height)
{
  GimpDrawablePreview *drawable_preview = GIMP_DRAWABLE_PREVIEW (preview);
  GimpDrawable        *drawable         = drawable_preview->drawable;

  if (drawable)
    _gimp_drawable_preview_area_draw_thumb (area, drawable, width, height);
}

void
_gimp_drawable_preview_area_draw_thumb (GimpPreviewArea *area,
                                        GimpDrawable    *drawable,
                                        gint             width,
                                        gint             height)
{
  guchar *buffer;
  gint    x1, y1, x2, y2;
  gint    bpp;
  gint    size = 100;
  gint    nav_width, nav_height;

  g_return_if_fail (GIMP_IS_PREVIEW_AREA (area));
  g_return_if_fail (drawable != NULL);

  if (_gimp_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      width  = x2 - x1;
      height = y2 - y1;
    }
  else
    {
      width  = gimp_drawable_width  (drawable->drawable_id);
      height = gimp_drawable_height (drawable->drawable_id);
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

  if (_gimp_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2))
    {
      buffer = gimp_drawable_get_sub_thumbnail_data (drawable->drawable_id,
                                                     x1, y1, x2 - x1, y2 - y1,
                                                     &nav_width, &nav_height,
                                                     &bpp);
    }
  else
    {
      buffer = gimp_drawable_get_thumbnail_data (drawable->drawable_id,
                                                 &nav_width, &nav_height,
                                                 &bpp);
    }

  if (buffer)
    {
      GimpImageType  type;

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
  GimpPreview  *gimp_preview = GIMP_PREVIEW (preview);
  GimpDrawable *drawable     = preview->drawable;
  gint32        image_id;

  image_id = gimp_drawable_get_image (drawable->drawable_id);

  if (gimp_selection_is_empty (image_id))
    {
      gimp_preview_area_draw (GIMP_PREVIEW_AREA (gimp_preview->area),
                              x - gimp_preview->xoff - gimp_preview->xmin,
                              y - gimp_preview->yoff - gimp_preview->ymin,
                              width,
                              height,
                              gimp_drawable_type (drawable->drawable_id),
                              buf, rowstride);
    }
  else
    {
      gint offset_x, offset_y;
      gint mask_x, mask_y;
      gint mask_width, mask_height;
      gint draw_x, draw_y;
      gint draw_width, draw_height;

      gimp_drawable_offsets (drawable->drawable_id, &offset_x, &offset_y);

      if (gimp_drawable_mask_intersect (drawable->drawable_id,
                                        &mask_x, &mask_y,
                                        &mask_width, &mask_height) &&
          gimp_rectangle_intersect (mask_x, mask_y,
                                    mask_width, mask_height,
                                    x, y, width, height,
                                    &draw_x, &draw_y,
                                    &draw_width, &draw_height))
        {
          GimpDrawable *selection;
          GimpPixelRgn  drawable_rgn;
          GimpPixelRgn  selection_rgn;
          guchar       *src;
          guchar       *sel;

          selection = gimp_drawable_get (gimp_image_get_selection (image_id));

          gimp_pixel_rgn_init (&drawable_rgn, drawable,
                               draw_x, draw_y, draw_width, draw_height,
                               FALSE, FALSE);
          gimp_pixel_rgn_init (&selection_rgn, selection,
                               draw_x + offset_x, draw_y + offset_y,
                               draw_width, draw_height,
                               FALSE, FALSE);

          src = g_new (guchar, draw_width * draw_height * drawable->bpp);
          sel = g_new (guchar, draw_width * draw_height);

          gimp_pixel_rgn_get_rect (&drawable_rgn, src,
                                   draw_x, draw_y,
                                   draw_width, draw_height);
          gimp_pixel_rgn_get_rect (&selection_rgn, sel,
                                   draw_x + offset_x, draw_y + offset_y,
                                   draw_width, draw_height);

          gimp_preview_area_mask (GIMP_PREVIEW_AREA (gimp_preview->area),
                                  draw_x - gimp_preview->xoff - gimp_preview->xmin,
                                  draw_y - gimp_preview->yoff - gimp_preview->ymin,
                                  draw_width,
                                  draw_height,
                                  gimp_drawable_type (drawable->drawable_id),
                                  src, draw_width * drawable->bpp,
                                  (buf +
                                   (draw_x - x) * drawable->bpp +
                                   (draw_y - y) * rowstride),
                                  rowstride,
                                  sel, draw_width);

          g_free (sel);
          g_free (src);

          gimp_drawable_detach (selection);
        }
    }
}

static void
gimp_drawable_preview_draw_buffer (GimpPreview  *preview,
                                   const guchar *buffer,
                                   gint          rowstride)
{
  gimp_drawable_preview_draw_area (GIMP_DRAWABLE_PREVIEW (preview),
                                   preview->xmin + preview->xoff,
                                   preview->ymin + preview->yoff,
                                   preview->width,
                                   preview->height,
                                   buffer, rowstride);
}

static void
gimp_drawable_preview_set_drawable (GimpDrawablePreview *drawable_preview,
                                    GimpDrawable        *drawable)
{
  GimpPreview *preview = GIMP_PREVIEW (drawable_preview);
  gint         x1, y1, x2, y2;

  drawable_preview->drawable = drawable;

  _gimp_drawable_preview_get_bounds (drawable, &x1, &y1, &x2, &y2);

  gimp_preview_set_bounds (preview, x1, y1, x2, y2);

  if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      guint32  image = gimp_drawable_get_image (drawable->drawable_id);
      guchar  *cmap;
      gint     num_colors;

      cmap = gimp_image_get_colormap (image, &num_colors);
      gimp_preview_area_set_colormap (GIMP_PREVIEW_AREA (preview->area),
                                      cmap, num_colors);
      g_free (cmap);
    }
}


#define MAX3(a, b, c)  (MAX (MAX ((a), (b)), (c)))
#define MIN3(a, b, c)  (MIN (MIN ((a), (b)), (c)))

gboolean
_gimp_drawable_preview_get_bounds (GimpDrawable *drawable,
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

  g_return_val_if_fail (drawable != NULL, FALSE);

  width  = gimp_drawable_width (drawable->drawable_id);
  height = gimp_drawable_height (drawable->drawable_id);

  retval = gimp_drawable_mask_bounds (drawable->drawable_id,
                                      &x1, &y1, &x2, &y2);

  gimp_drawable_offsets (drawable->drawable_id, &offset_x, &offset_y);

  *xmin = MAX3 (x1 - SELECTION_BORDER, 0, - offset_x);
  *ymin = MAX3 (y1 - SELECTION_BORDER, 0, - offset_y);
  *xmax = MIN3 (x2 + SELECTION_BORDER, width,  width  + offset_x);
  *ymax = MIN3 (y2 + SELECTION_BORDER, height, height + offset_y);

  return retval;
}


/**
 * gimp_drawable_preview_new:
 * @drawable: a #GimpDrawable
 * @toggle:   unused
 *
 * Creates a new #GimpDrawablePreview widget for @drawable.
 *
 * In GIMP 2.2 the @toggle parameter was provided to conviently access
 * the state of the "Preview" check-button. This is not any longer
 * necessary as the preview itself now stores this state, as well as
 * the scroll offset.
 *
 * Returns: A pointer to the new #GimpDrawablePreview widget.
 *
 * Since: GIMP 2.2
 **/
GtkWidget *
gimp_drawable_preview_new (GimpDrawable *drawable,
                           gboolean     *toggle)
{
  g_return_val_if_fail (drawable != NULL, NULL);

  return g_object_new (GIMP_TYPE_DRAWABLE_PREVIEW,
                       "drawable", drawable,
                       NULL);
}

/**
 * gimp_drawable_preview_get_drawable:
 * @preview:   a #GimpDrawablePreview widget
 *
 * Return value: the #GimpDrawable that has been passed to
 *               gimp_drawable_preview_new().
 *
 * Since: GIMP 2.2
 **/
GimpDrawable *
gimp_drawable_preview_get_drawable (GimpDrawablePreview *preview)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview), NULL);

  return preview->drawable;
}

/**
 * gimp_drawable_preview_draw_region:
 * @preview: a #GimpDrawablePreview widget
 * @region:  a #GimpPixelRgn
 *
 * Since: GIMP 2.2
 **/
void
gimp_drawable_preview_draw_region (GimpDrawablePreview *preview,
                                   const GimpPixelRgn  *region)
{
  g_return_if_fail (GIMP_IS_DRAWABLE_PREVIEW (preview));
  g_return_if_fail (preview->drawable != NULL);
  g_return_if_fail (region != NULL);

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
