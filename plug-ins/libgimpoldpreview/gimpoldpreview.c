/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpmiscui.c
 * Contains all kinds of miscellaneous routines factored out from different
 * plug-ins. They stay here until their API has crystalized a bit and we can
 * put them into the file where they belong (Maurits Rijk
 * <lpeek.mrijk@consunet.nl> if you want to blame someone for this mess)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED
#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include "gimpoldpreview.h"

#include <libgimp/libgimp-intl.h>


#define PREVIEW_SIZE    128
#define PREVIEW_BPP     3

static void
gimp_old_preview_put_in_frame (GimpOldPreview* preview)
{
  GtkWidget *frame, *abox;

  preview->frame = gtk_frame_new (_("Preview"));
  gtk_widget_show (preview->frame);

  abox = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (abox), 4);
  gtk_container_add (GTK_CONTAINER (preview->frame), abox);
  gtk_widget_show (abox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (abox), frame);
  gtk_widget_show (frame);

  gtk_container_add (GTK_CONTAINER (frame), preview->widget);
}

GimpOldPreview*
gimp_old_preview_new (GimpDrawable *drawable,
                      gboolean      has_frame)
{
  GimpOldPreview *preview = g_new0 (GimpOldPreview, 1);

  preview->widget = gtk_preview_new (GTK_PREVIEW_COLOR);
  preview->is_gray = FALSE;

  if (drawable)
    gimp_old_preview_fill_with_thumb (preview, drawable->drawable_id);

  if (has_frame)
    gimp_old_preview_put_in_frame (preview);

  return preview;
}

void
gimp_old_preview_free (GimpOldPreview *preview)
{
  g_free (preview->cmap);
  g_free (preview->even);
  g_free (preview->odd);
  g_free (preview->cache);
  g_free (preview);
}

GimpOldPreview*
gimp_old_preview_new2 (GimpImageType drawable_type,
                       gboolean      has_frame)
{
  GimpOldPreview *preview;
  guchar         *buf = NULL;
  gint            y;

  preview = g_new0 (GimpOldPreview, 1);

  switch (drawable_type)
    {
    case GIMP_GRAY_IMAGE:
    case GIMP_GRAYA_IMAGE:
      preview->widget = gtk_preview_new (GTK_PREVIEW_GRAYSCALE);
      buf     = g_malloc0 (PREVIEW_SIZE);
      preview->is_gray = TRUE;
      break;

    case GIMP_RGB_IMAGE:
    case GIMP_RGBA_IMAGE:
      preview->widget = gtk_preview_new (GTK_PREVIEW_COLOR);
      buf     = g_malloc0 (PREVIEW_SIZE * 3);
      preview->is_gray = FALSE;
      break;

    default:
      g_assert_not_reached ();
      break;
    }

  gtk_preview_size (GTK_PREVIEW (preview->widget), PREVIEW_SIZE, PREVIEW_SIZE);

  for (y = 0; y < PREVIEW_SIZE; y++)
    gtk_preview_draw_row (GTK_PREVIEW (preview->widget), buf, 0, y,
                          PREVIEW_SIZE);

  g_free (buf);

  if (has_frame)
    gimp_old_preview_put_in_frame (preview);

  preview->buffer = GTK_PREVIEW (preview->widget)->buffer;
  preview->width  = GTK_PREVIEW (preview->widget)->buffer_width;
  preview->height = GTK_PREVIEW (preview->widget)->buffer_height;
  preview->rowstride = preview->width * ((preview->is_gray) ? 1 : 3);

  return preview;
}

void
gimp_old_preview_put_pixel (GimpOldPreview *preview,
                            gint            x,
                            gint            y,
                            const guchar   *pixel)
{
  guchar *dest;

  g_return_if_fail (x >= 0 && x < PREVIEW_SIZE);
  g_return_if_fail (y >= 0 && y < PREVIEW_SIZE);

  dest = preview->buffer + y * preview->rowstride;

  if (preview->is_gray)
    {
      dest += x;
      dest[0] = pixel[0];
    }
  else
    {
      dest += x * 3;
      dest[0] = pixel[0];
      dest[1] = pixel[1];
      dest[2] = pixel[2];
    }
}

void
gimp_old_preview_get_pixel (GimpOldPreview *preview,
                            gint            x,
                            gint            y,
                            guchar         *pixel)
{
  const guchar *src;

  g_return_if_fail (x >= 0 && x < PREVIEW_SIZE);
  g_return_if_fail (y >= 0 && y < PREVIEW_SIZE);

  src = preview->buffer + y * preview->rowstride;

  if (preview->is_gray)
    {
      src += x;
      pixel[0] = src[0];
    }
  else
    {
      src += x * 3;
      pixel[0] = src[0];
      pixel[1] = src[1];
      pixel[2] = src[2];
    }
}

void
gimp_old_preview_do_row (GimpOldPreview *preview,
                         gint            row,
                         gint            width,
                         const guchar   *src)
{
  gint    x;
  guchar *p0 = preview->even;
  guchar *p1 = preview->odd;
  gint    bpp = preview->bpp;
  gint r, g, b, a;
  gint c0, c1;

  for (x = 0; x < width; x++)
    {
      switch (bpp)
        {
        case 4:
          r = src[x * 4 + 0];
          g = src[x * 4 + 1];
          b = src[x * 4 + 2];
          a = src[x * 4 + 3];
          break;

        case 3:
          r = src[x*3 + 0];
          g = src[x*3 + 1];
          b = src[x*3 + 2];
          a = 255;
          break;

        default:
          if (preview->cmap)
            {
              gint index = MIN (src[x*bpp], preview->ncolors - 1);

              r = preview->cmap[index * 3 + 0];
              g = preview->cmap[index * 3 + 1];
              b = preview->cmap[index * 3 + 2];
            }
          else
            {
              g = b = r = src[x * bpp + 0];
            }

          if (bpp == 2)
            a = src[x*2 + 1];
          else
            a = 255;
          break;
        }

      if ((x / GIMP_CHECK_SIZE_SM) & 1)
        {
          c0 = GIMP_CHECK_LIGHT * 255;
          c1 = GIMP_CHECK_DARK * 255;
        }
      else
        {
          c0 = GIMP_CHECK_DARK * 255;
          c1 = GIMP_CHECK_LIGHT * 255;
        }

      *p0++ = c0 + (r - c0) * a / 255;
      *p0++ = c0 + (g - c0) * a / 255;
      *p0++ = c0 + (b - c0) * a / 255;

      *p1++ = c1 + (r - c1) * a / 255;
      *p1++ = c1 + (g - c1) * a / 255;
      *p1++ = c1 + (b - c1) * a / 255;
    }

  if ((row / GIMP_CHECK_SIZE_SM) & 1)
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview->widget),
                            preview->odd,  0, row, width);
    }
  else
    {
      gtk_preview_draw_row (GTK_PREVIEW (preview->widget),
                            preview->even, 0, row, width);
    }
}

void
gimp_old_preview_update (GimpOldPreview      *preview,
                         GimpOldPreviewFunc   func,
                         gpointer             data)
{
  guchar *buffer;
  gint    x, y;
  gint    bpp;

  bpp    = preview->bpp;
  buffer = g_new (guchar, preview->rowstride);

  for (y = 0; y < preview->height; y++)
    {
      const guchar *src  = preview->cache + y * preview->rowstride;
      guchar       *dest = buffer;

      for (x = 0; x < preview->width; x++)
        {
          func (src, dest, bpp, data);

          src += bpp;
          dest += bpp;
        }

      gimp_old_preview_do_row (preview, y, preview->width, buffer);
    }

  gtk_widget_queue_draw (preview->widget);

  g_free (buffer);
}

void
gimp_old_preview_fill_with_thumb (GimpOldPreview *preview,
                                  gint32          drawable_ID)
{
  const guchar *src;
  gint          bpp;
  gint          y;
  gint          width  = PREVIEW_SIZE;
  gint          height = PREVIEW_SIZE;

  preview->cache =
    gimp_drawable_get_thumbnail_data (drawable_ID, &width, &height, &bpp);

  if (width < 1 || height < 1)
    return;

  preview->rowstride = width * bpp;
  preview->bpp       = bpp;

  if (gimp_drawable_is_indexed (drawable_ID))
    {
      gint32 image_ID = gimp_drawable_get_image (drawable_ID);
      preview->cmap = gimp_image_get_cmap (image_ID, &preview->ncolors);
    }
  else
    {
      preview->cmap = NULL;
    }

  gtk_preview_size (GTK_PREVIEW (preview->widget), width, height);

  preview->scale_x =
    (gdouble) width / (gdouble) gimp_drawable_width (drawable_ID);
  preview->scale_y =
    (gdouble) height / (gdouble) gimp_drawable_height (drawable_ID);

  src  = preview->cache;
  preview->even = g_malloc (width * 3);
  preview->odd  = g_malloc (width * 3);

  for (y = 0; y < height; y++)
    {
      gimp_old_preview_do_row (preview, y, width, src);
      src += width * bpp;
    }

  preview->buffer = GTK_PREVIEW (preview->widget)->buffer;
  preview->width  = GTK_PREVIEW (preview->widget)->buffer_width;
  preview->height = GTK_PREVIEW (preview->widget)->buffer_height;
}

void
gimp_old_preview_fill (GimpOldPreview *preview,
                       GimpDrawable   *drawable)
{
  GimpPixelRgn  srcPR;
  gint          width;
  gint          height;
  gint          x1, x2, y1, y2;
  gint          bpp;
  gint          y;
  guchar       *src;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  if (x2 - x1 > PREVIEW_SIZE)
    x2 = x1 + PREVIEW_SIZE;

  if (y2 - y1 > PREVIEW_SIZE)
    y2 = y1 + PREVIEW_SIZE;

  width  = x2 - x1;
  height = y2 - y1;
  bpp    = gimp_drawable_bpp (drawable->drawable_id);

  if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      gint32 image_ID = gimp_drawable_get_image (drawable->drawable_id);
      preview->cmap = gimp_image_get_cmap (image_ID, &preview->ncolors);
    }
  else
    {
      preview->cmap = NULL;
    }

  gtk_preview_size (GTK_PREVIEW (preview->widget), width, height);

  gimp_pixel_rgn_init (&srcPR, drawable, x1, y1, width, height, FALSE, FALSE);

  preview->even = g_malloc (width * 3);
  preview->odd  = g_malloc (width * 3);
  src = g_malloc (width * bpp);
  preview->cache = g_malloc(width * bpp * height);
  preview->rowstride = width * bpp;
  preview->bpp = bpp;

  for (y = 0; y < height; y++)
    {
      gimp_pixel_rgn_get_row (&srcPR, src, x1, y + y1, width);
      memcpy(preview->cache + (y * width * bpp), src, width * bpp);
    }

  for (y = 0; y < height; y++)
    {
      gimp_old_preview_do_row (preview, y, width,
                               preview->cache + (y * width * bpp));
    }

  preview->buffer = GTK_PREVIEW (preview->widget)->buffer;
  preview->width  = GTK_PREVIEW (preview->widget)->buffer_width;
  preview->height = GTK_PREVIEW (preview->widget)->buffer_height;

  g_free (src);
}

void
gimp_old_preview_fill_scaled (GimpOldPreview *preview,
                              GimpDrawable   *drawable)
{
  gint     bpp;
  gint     x1, y1, x2, y2;
  gint     sel_width, sel_height;
  gint     width, height;
  gdouble  px, py;
  gdouble  dx, dy;
  gint     x, y;
  guchar  *dest;
  GimpPixelFetcher *pft;

  gimp_drawable_mask_bounds (drawable->drawable_id, &x1, &y1, &x2, &y2);

  sel_width  = x2 - x1;
  sel_height = y2 - y1;

  /* Calculate preview size */
  if (sel_width > sel_height)
    {
      width  = MIN (sel_width, PREVIEW_SIZE);
      height = sel_height * width / sel_width;
    }
  else
    {
      height = MIN(sel_height, PREVIEW_SIZE);
      width  = sel_width * height / sel_height;
    }

  if (width < 2) width = 2;
  if (height < 2) height = 2;

  bpp = gimp_drawable_bpp (drawable->drawable_id);

  if (gimp_drawable_is_indexed (drawable->drawable_id))
    {
      gint32 image_ID = gimp_drawable_get_image (drawable->drawable_id);
      preview->cmap = gimp_image_get_cmap (image_ID, &preview->ncolors);
    }
  else
    {
      preview->cmap = NULL;
    }

  gtk_preview_size (GTK_PREVIEW (preview->widget), width, height);

  preview->even      = g_malloc (width * 3);
  preview->odd       = g_malloc (width * 3);
  preview->cache     = g_malloc (width * bpp * height);
  preview->rowstride = width * bpp;
  preview->bpp       = bpp;

  dx = (gdouble) (x2 - x1 - 1) / (width - 1);
  dy = (gdouble) (y2 - y1 - 1) / (height - 1);

  py = y1;

  pft = gimp_pixel_fetcher_new (drawable);

  for (y = 0; y < height; y++)
    {
      dest = preview->cache + y * preview->rowstride;
      px = x1;
      for (x = 0; x < width; x++)
        {
          gimp_pixel_fetcher_get_pixel (pft, (gint) px, (gint) py, dest);
          dest += bpp;
          px += dx;
        }
      gimp_old_preview_do_row (preview, y, width, dest);
      py += dy;
    }
  gimp_pixel_fetcher_destroy (pft);

  preview->buffer = GTK_PREVIEW (preview->widget)->buffer;
  preview->width  = GTK_PREVIEW (preview->widget)->buffer_width;
  preview->height = GTK_PREVIEW (preview->widget)->buffer_height;
}

gchar *
gimp_plug_in_get_path (const gchar *path_name,
                       const gchar *dir_name)
{
  gchar *path;

  g_return_val_if_fail (path_name != NULL, NULL);
  g_return_val_if_fail (dir_name != NULL, NULL);

  path = gimp_gimprc_query (path_name);

  if (! path)
    {
      gchar *gimprc = gimp_personal_rc_file ("gimprc");
      gchar *full_path;
      gchar *esc_path;

      full_path =
        g_strconcat ("${gimp_dir}", G_DIR_SEPARATOR_S, dir_name,
                     G_SEARCHPATH_SEPARATOR_S,
                     "${gimp_data_dir}", G_DIR_SEPARATOR_S, dir_name,
                     NULL);
      esc_path = g_strescape (full_path, NULL);
      g_free (full_path);

      g_message (_("No %s in gimprc:\n"
                   "You need to add an entry like\n"
                   "(%s \"%s\")\n"
                   "to your %s file."),
                 path_name, path_name, esc_path, gimprc);

      g_free (gimprc);
      g_free (esc_path);
    }

  return path;
}

