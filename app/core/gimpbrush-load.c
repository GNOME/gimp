/* The GIMP -- an image manipulation program
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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include <glib-object.h>

#ifdef G_OS_WIN32 /* gets defined by glib.h */
#include <io.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "base/brush-scale.h"
#include "base/temp-buf.h"

#include "config/gimpbaseconfig.h"

#include "gimpbrush.h"
#include "gimpbrush-header.h"
#include "gimpbrushgenerated.h"
#include "gimpmarshal.h"

#include "gimp-intl.h"


enum
{
  SPACING_CHANGED,
  LAST_SIGNAL
};


static void        gimp_brush_class_init            (GimpBrushClass *klass);
static void        gimp_brush_init                  (GimpBrush      *brush);

static void        gimp_brush_finalize              (GObject        *object);

static gint64      gimp_brush_get_memsize           (GimpObject     *object,
                                                     gint64         *gui_size);

static gboolean    gimp_brush_get_popup_size        (GimpViewable   *viewable,
                                                     gint            width,
                                                     gint            height,
                                                     gboolean        dot_for_dot,
                                                     gint           *popup_width,
                                                     gint           *popup_height);
static TempBuf   * gimp_brush_get_new_preview       (GimpViewable   *viewable,
                                                     gint            width,
                                                     gint            height);
static gchar     * gimp_brush_get_description       (GimpViewable   *viewable,
                                                     gchar         **tooltip);
static gchar     * gimp_brush_get_extension         (GimpData       *data);

static GimpBrush * gimp_brush_real_select_brush     (GimpBrush      *brush,
                                                     GimpCoords     *last_coords,
                                                     GimpCoords     *cur_coords);
static gboolean    gimp_brush_real_want_null_motion (GimpBrush      *brush,
                                                     GimpCoords     *last_coords,
                                                     GimpCoords     *cur_coords);


static guint brush_signals[LAST_SIGNAL] = { 0 };

static GimpDataClass *parent_class = NULL;


GType
gimp_brush_get_type (void)
{
  static GType brush_type = 0;

  if (! brush_type)
    {
      static const GTypeInfo brush_info =
      {
        sizeof (GimpBrushClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_brush_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpBrush),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_brush_init,
      };

      brush_type = g_type_register_static (GIMP_TYPE_DATA,
                                           "GimpBrush",
                                           &brush_info, 0);
  }

  return brush_type;
}

static void
gimp_brush_class_init (GimpBrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  brush_signals[SPACING_CHANGED] =
    g_signal_new ("spacing_changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpBrushClass, spacing_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize           = gimp_brush_finalize;

  gimp_object_class->get_memsize   = gimp_brush_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-paintbrush";
  viewable_class->get_popup_size   = gimp_brush_get_popup_size;
  viewable_class->get_new_preview  = gimp_brush_get_new_preview;
  viewable_class->get_description  = gimp_brush_get_description;

  data_class->get_extension        = gimp_brush_get_extension;

  klass->select_brush              = gimp_brush_real_select_brush;
  klass->want_null_motion          = gimp_brush_real_want_null_motion;
  klass->spacing_changed           = NULL;
}

static void
gimp_brush_init (GimpBrush *brush)
{
  brush->mask     = NULL;
  brush->pixmap   = NULL;

  brush->spacing  = 20;
  brush->x_axis.x = 15.0;
  brush->x_axis.y =  0.0;
  brush->y_axis.x =  0.0;
  brush->y_axis.y = 15.0;
}

static void
gimp_brush_finalize (GObject *object)
{
  GimpBrush *brush = GIMP_BRUSH (object);

  if (brush->mask)
    {
      temp_buf_free (brush->mask);
      brush->mask = NULL;
    }

  if (brush->pixmap)
    {
      temp_buf_free (brush->pixmap);
      brush->pixmap = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
gimp_brush_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpBrush *brush   = GIMP_BRUSH (object);
  gint64     memsize = 0;

  if (brush->mask)
    memsize += temp_buf_get_memsize (brush->mask);

  if (brush->pixmap)
    memsize += temp_buf_get_memsize (brush->pixmap);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_brush_get_popup_size (GimpViewable *viewable,
                           gint          width,
                           gint          height,
                           gboolean      dot_for_dot,
                           gint         *popup_width,
                           gint         *popup_height)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  if (brush->mask->width > width || brush->mask->height > height)
    {
      *popup_width  = brush->mask->width;
      *popup_height = brush->mask->height;

      return TRUE;
    }

  return FALSE;
}

static TempBuf *
gimp_brush_get_new_preview (GimpViewable *viewable,
                            gint          width,
                            gint          height)
{
  GimpBrush *brush      = GIMP_BRUSH (viewable);
  gint       brush_width;
  gint       brush_height;
  TempBuf   *mask_buf   = NULL;
  TempBuf   *pixmap_buf = NULL;
  TempBuf   *return_buf = NULL;
  guchar     transp[4] = { 0, 0, 0, 0 };
  guchar    *mask;
  guchar    *buf;
  gint       x, y;
  gboolean   scale = FALSE;

  mask_buf   = gimp_brush_get_mask (brush);
  pixmap_buf = gimp_brush_get_pixmap (brush);

  brush_width  = mask_buf->width;
  brush_height = mask_buf->height;

  if (brush_width > width || brush_height > height)
    {
      gdouble ratio_x = (gdouble) brush_width  / width;
      gdouble ratio_y = (gdouble) brush_height / height;

      brush_width  = (gdouble) brush_width  / MAX (ratio_x, ratio_y) + 0.5;
      brush_height = (gdouble) brush_height / MAX (ratio_x, ratio_y) + 0.5;

      if (brush_width <= 0)  brush_width  = 1;
      if (brush_height <= 0) brush_height = 1;

      mask_buf = brush_scale_mask (mask_buf, brush_width, brush_height);

      if (pixmap_buf)
        {
          /* TODO: the scale function should scale the pixmap and the
           *  mask in one run
           */
          pixmap_buf =
            brush_scale_pixmap (pixmap_buf, brush_width, brush_height);
        }

      scale = TRUE;
    }

  return_buf = temp_buf_new (brush_width, brush_height, 4, 0, 0, transp);

  mask = temp_buf_data (mask_buf);
  buf  = temp_buf_data (return_buf);

  if (pixmap_buf)
    {
      guchar *pixmap = temp_buf_data (pixmap_buf);

      for (y = 0; y < brush_height; y++)
        {
          for (x = 0; x < brush_width ; x++)
            {
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *pixmap++;
              *buf++ = *mask++;
            }
        }
    }
  else
    {
      for (y = 0; y < brush_height; y++)
        {
          for (x = 0; x < brush_width ; x++)
            {
              *buf++ = 0;
              *buf++ = 0;
              *buf++ = 0;
              *buf++ = *mask++;
            }
        }
    }

  if (scale)
    {
      temp_buf_free (mask_buf);

      if (pixmap_buf)
        temp_buf_free (pixmap_buf);
    }

  return return_buf;
}

static gchar *
gimp_brush_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpBrush *brush = GIMP_BRUSH (viewable);

  if (tooltip)
    *tooltip = NULL;

  return g_strdup_printf ("%s (%d x %d)",
                          GIMP_OBJECT (brush)->name,
                          brush->mask->width,
                          brush->mask->height);
}

static gchar *
gimp_brush_get_extension (GimpData *data)
{
  return GIMP_BRUSH_FILE_EXTENSION;
}

GimpData *
gimp_brush_new (const gchar *name,
                gboolean     stingy_memory_use)
{
  g_return_val_if_fail (name != NULL, NULL);

  return gimp_brush_generated_new (name,
                                   GIMP_BRUSH_GENERATED_CIRCLE,
                                   5.0, 2, 0.5, 1.0, 0.0,
                                   stingy_memory_use);
}

GimpData *
gimp_brush_get_standard (void)
{
  static GimpData *standard_brush = NULL;

  if (! standard_brush)
    {
      standard_brush = gimp_brush_new ("Standard", FALSE);

      standard_brush->dirty = FALSE;
      gimp_data_make_internal (standard_brush);

      /*  set ref_count to 2 --> never swap the standard brush  */
      g_object_ref (standard_brush);
    }

  return standard_brush;
}

GList *
gimp_brush_load (const gchar  *filename,
                 gboolean      stingy_memory_use,
                 GError      **error)
{
  GimpBrush *brush;
  gint       fd;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  fd = open (filename, O_RDONLY | _O_BINARY);
  if (fd == -1)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_OPEN,
                   _("Could not open '%s' for reading: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  brush = gimp_brush_load_brush (fd, filename, error);

  close (fd);

  if (! brush)
    return NULL;

  /*  Swap the brush to disk (if we're being stingy with memory) */
  if (stingy_memory_use)
    {
      temp_buf_swap (brush->mask);

      if (brush->pixmap)
        temp_buf_swap (brush->pixmap);
    }

  return g_list_prepend (NULL, brush);
}

GimpBrush *
gimp_brush_select_brush (GimpBrush  *brush,
                         GimpCoords *last_coords,
                         GimpCoords *cur_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (cur_coords != NULL, NULL);

  return GIMP_BRUSH_GET_CLASS (brush)->select_brush (brush,
                                                     last_coords,
                                                     cur_coords);
}

gboolean
gimp_brush_want_null_motion (GimpBrush  *brush,
                             GimpCoords *last_coords,
                             GimpCoords *cur_coords)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (cur_coords != NULL, FALSE);

  return GIMP_BRUSH_GET_CLASS (brush)->want_null_motion (brush,
                                                         last_coords,
                                                         cur_coords);
}

static GimpBrush *
gimp_brush_real_select_brush (GimpBrush  *brush,
                              GimpCoords *last_coords,
                              GimpCoords *cur_coords)
{
  return brush;
}

static gboolean
gimp_brush_real_want_null_motion (GimpBrush  *brush,
                                  GimpCoords *last_coords,
                                  GimpCoords *cur_coords)
{
  return TRUE;
}

TempBuf *
gimp_brush_get_mask (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->mask;
}

TempBuf *
gimp_brush_get_pixmap (const GimpBrush *brush)
{
  g_return_val_if_fail (brush != NULL, NULL);
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), NULL);

  return brush->pixmap;
}

gint
gimp_brush_get_spacing (const GimpBrush *brush)
{
  g_return_val_if_fail (GIMP_IS_BRUSH (brush), 0);

  return brush->spacing;
}

void
gimp_brush_set_spacing (GimpBrush *brush,
                        gint       spacing)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  if (brush->spacing != spacing)
    {
      brush->spacing = spacing;

      gimp_brush_spacing_changed (brush);
    }
}

void
gimp_brush_spacing_changed (GimpBrush *brush)
{
  g_return_if_fail (GIMP_IS_BRUSH (brush));

  g_signal_emit (brush, brush_signals[SPACING_CHANGED], 0);
}

GimpBrush *
gimp_brush_load_brush (gint          fd,
                       const gchar  *filename,
                       GError      **error)
{
  GimpBrush   *brush;
  gint         bn_size;
  BrushHeader  header;
  gchar       *name = NULL;
  guchar      *pixmap;
  guchar      *mask;
  gssize       i, size;
  gboolean     success = TRUE;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (fd != -1, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  /*  Read in the header size  */
  if (read (fd, &header, sizeof (header)) != sizeof (header))
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Could not read %d bytes from '%s': %s"),
                   (gint) sizeof (header),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      return NULL;
    }

  /*  rearrange the bytes in each unsigned int  */
  header.header_size  = g_ntohl (header.header_size);
  header.version      = g_ntohl (header.version);
  header.width        = g_ntohl (header.width);
  header.height       = g_ntohl (header.height);
  header.bytes        = g_ntohl (header.bytes);
  header.magic_number = g_ntohl (header.magic_number);
  header.spacing      = g_ntohl (header.spacing);

  /*  Check for correct file format */

  if (header.width == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Width = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  if (header.height == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Height = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  if (header.bytes == 0)
    {
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Bytes = 0."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  switch (header.version)
    {
    case 1:
      /*  If this is a version 1 brush, set the fp back 8 bytes  */
      lseek (fd, -8, SEEK_CUR);
      header.header_size += 8;
      /*  spacing is not defined in version 1  */
      header.spacing = 25;
      break;

    case 3:  /*  cinepaint brush  */
      if (header.bytes == 18  /* FLOAT16_GRAY_GIMAGE */)
        {
          header.bytes = 2;
        }
      else
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "Unknown depth %d."),
                       gimp_filename_to_utf8 (filename), header.bytes);
          return NULL;
        }
      /*  fallthrough  */

    case 2:
      if (header.magic_number == GBRUSH_MAGIC)
        break;

    default:
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unknown version %d."),
                   gimp_filename_to_utf8 (filename), header.version);
      return NULL;
    }

  /*  Read in the brush name  */
  if ((bn_size = (header.header_size - sizeof (header))))
    {
      gchar *utf8;

      name = g_new (gchar, bn_size);

      if ((read (fd, name, bn_size)) < bn_size)
        {
          g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                       _("Fatal parse error in brush file '%s': "
                         "File appears truncated."),
                       gimp_filename_to_utf8 (filename));
          g_free (name);
          return NULL;
        }

      utf8 = gimp_any_to_utf8 (name, -1,
                               _("Invalid UTF-8 string in brush file '%s'."),
                               gimp_filename_to_utf8 (filename));
      g_free (name);
      name = utf8;
    }

  if (!name)
    name = g_strdup (_("Unnamed"));

  brush = g_object_new (GIMP_TYPE_BRUSH,
                        "name", name,
                        NULL);

  g_free (name);

  brush->mask = temp_buf_new (header.width, header.height, 1, 0, 0, NULL);

  mask = temp_buf_data (brush->mask);
  size = header.width * header.height * header.bytes;

  switch (header.bytes)
    {
    case 1:
      success = (read (fd, mask, size) == size);
      break;

    case 2:  /*  cinepaint brush, 16 bit floats  */
      {
        guchar buf[8 * 1024];

        for (i = 0; success && i < size;)
          {
            gssize  bytes = MIN (size - i, sizeof (buf));

            success = (read (fd, buf, bytes) == bytes);

            if (success)
              {
                guint16 *b = (guint16 *) buf;

                i += bytes;

                for (; bytes > 0; bytes -= 2, mask++, b++)
                  {
                    union
                    {
                      guint16 u[2];
                      gfloat  f;
                    } short_float;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
                    short_float.u[0] = 0;
                    short_float.u[1] = GUINT16_FROM_BE (*b);
#else
                    short_float.u[0] = GUINT16_FROM_BE (*b);
                    short_float.u[1] = 0;
#endif

                    *mask = (guchar) (short_float.f * 255.0 + 0.5);
                  }
              }
          }
      }
      break;

    case 4:
      {
        guchar buf[8 * 1024];

        brush->pixmap = temp_buf_new (header.width, header.height,
                                      3, 0, 0, NULL);
        pixmap = temp_buf_data (brush->pixmap);

        for (i = 0; success && i < size;)
          {
            gssize  bytes = MIN (size - i, sizeof (buf));

            success = (read (fd, buf, bytes) == bytes);

            if (success)
              {
                guchar *b = buf;

                i += bytes;

                for (; bytes > 0; bytes -= 4, pixmap += 3, mask++, b += 4)
                  {
                    pixmap[0] = b[0];
                    pixmap[1] = b[1];
                    pixmap[2] = b[2];

                    mask[0]   = b[3];
                  }
              }
          }
      }
      break;

    default:
      g_object_unref (brush);
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "Unsupported brush depth %d\n"
                     "GIMP brushes must be GRAY or RGBA."),
                   gimp_filename_to_utf8 (filename), header.bytes);
      return NULL;
    }

  if (! success)
    {
      g_object_unref (brush);
      g_set_error (error, GIMP_DATA_ERROR, GIMP_DATA_ERROR_READ,
                   _("Fatal parse error in brush file '%s': "
                     "File appears truncated."),
                   gimp_filename_to_utf8 (filename));
      return NULL;
    }

  brush->spacing  = header.spacing;
  brush->x_axis.x = header.width  / 2.0;
  brush->x_axis.y = 0.0;
  brush->y_axis.x = 0.0;
  brush->y_axis.y = header.height / 2.0;

  return brush;
}
