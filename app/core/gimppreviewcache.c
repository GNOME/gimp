/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1999 Andy Thomas alt@gimp.org
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

#include <glib-object.h>

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimppreviewcache.h"


#define MAX_CACHE_PREVIEWS 5


typedef struct
{
  TempBuf *buf;
  gint     width;
  gint     height;
} PreviewNearest;


static gint
preview_cache_compare (gconstpointer  a,
                       gconstpointer  b)
{
  const TempBuf *buf1 = a;
  const TempBuf *buf2 = b;

  if (buf1->width > buf2->width && buf1->height > buf2->height)
    return -1;

  return 1;
}

static void
preview_cache_find_exact (gpointer data,
                          gpointer udata)
{
  TempBuf        *buf     = data;
  PreviewNearest *nearest = udata;

  if (nearest->buf)
    return;

  if (buf->width == nearest->width && buf->height == nearest->height)
    {
      nearest->buf = buf;
      return;
    }
}

static void
preview_cache_find_biggest (gpointer data,
                            gpointer udata)
{
  TempBuf        *buf     = data;
  PreviewNearest *nearest = udata;

  if (buf->width >= nearest->width && buf->height >= nearest->height)
    {
      /* Ok we could make the preview out of this one...
       * If we already have it are these bigger dimensions?
       */
      if (nearest->buf)
        {
          if (nearest->buf->width > buf->width &&
              nearest->buf->height > buf->height)
            return;
        }

      nearest->buf = buf;
    }
}

static void
preview_cache_remove_smallest (GSList **plist)
{
  GSList  *list;
  TempBuf *smallest = NULL;

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("preview_cache_remove_smallest\n");
#endif

  for (list = *plist; list; list = list->next)
    {
      if (!smallest)
        {
          smallest = list->data;
        }
      else
        {
          TempBuf *this = list->data;

          if (smallest->height * smallest->width > this->height * this->width)
            {
              smallest = this;
            }
        }
    }

  if (smallest)
    {
      *plist = g_slist_remove (*plist, smallest);

#ifdef PREVIEW_CACHE_DEBUG
      g_print ("preview_cache_remove_smallest: removed %d x %d\n",
               smallest->width, smallest->height);
#endif

      temp_buf_free (smallest);
    }
}

#ifdef PREVIEW_CACHE_DEBUG
static void
preview_cache_print (GSList *plist)
{
  GSList  *list;

  g_print ("preview cache dump:\n");

  for (list = plist; list; list = list->next)
    {
      TempBuf *buf = (TempBuf *) list->data;

      g_print ("\tvalue w,h [%d,%d]\n", buf->width, buf->height);
    }
}
#endif  /* PREVIEW_CACHE_DEBUG */

void
gimp_preview_cache_invalidate (GSList **plist)
{
#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_invalidate\n");
  preview_cache_print (*plist);
#endif

  g_slist_foreach (*plist, (GFunc) temp_buf_free, NULL);

  g_slist_free (*plist);
  *plist = NULL;
}

void
gimp_preview_cache_add (GSList  **plist,
                        TempBuf  *buf)
{
#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_add: %d x %d\n", buf->width, buf->height);
  preview_cache_print (*plist);
#endif

  if (g_slist_length (*plist) >= MAX_CACHE_PREVIEWS)
    {
      preview_cache_remove_smallest (plist);
    }

  *plist = g_slist_insert_sorted (*plist, buf, preview_cache_compare);
}

TempBuf *
gimp_preview_cache_get (GSList **plist,
                        gint     width,
                        gint     height)
{
  PreviewNearest pn;

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_get: %d x %d\n", width, height);
  preview_cache_print (*plist);
#endif

  pn.buf    = NULL;
  pn.width  = width;
  pn.height = height;

  g_slist_foreach (*plist, preview_cache_find_exact, &pn);

  if (pn.buf)
    {
#ifdef PREVIEW_CACHE_DEBUG
      g_print ("gimp_preview_cache_get: found exact match %d x %d\n",
               pn.buf->width, pn.buf->height);
#endif

      return pn.buf;
    }

  g_slist_foreach (*plist, preview_cache_find_biggest, &pn);

  if (pn.buf)
    {
      TempBuf *preview;
      gint     pwidth;
      gint     pheight;
      gdouble  x_ratio;
      gdouble  y_ratio;
      guchar  *src_data;
      guchar  *dest_data;
      gint     loop1;
      gint     loop2;

#ifdef PREVIEW_CACHE_DEBUG
      g_print ("gimp_preview_cache_get: nearest value: %d x %d\n",
               pn.buf->width, pn.buf->height);
#endif

      /* Make up new preview from the large one... */
      pwidth  = pn.buf->width;
      pheight = pn.buf->height;

      /* Now get the real one and add to cache */
      preview = temp_buf_new (width, height, pn.buf->bytes, 0, 0, NULL);

      /* preview from nearest bigger one */
      if (width)
        x_ratio = (gdouble) pwidth / (gdouble) width;
      else
        x_ratio = 0.0;

      if (height)
        y_ratio = (gdouble) pheight / (gdouble) height;
      else
        y_ratio = 0.0;

      src_data  = temp_buf_data (pn.buf);
      dest_data = temp_buf_data (preview);

      for (loop1 = 0 ; loop1 < height ; loop1++)
        for (loop2 = 0 ; loop2 < width ; loop2++)
          {
            gint    i;
            guchar *src_pixel;
            guchar *dest_pixel;

            src_pixel = src_data +
              ((gint) (loop2 * x_ratio)) * preview->bytes +
              ((gint) (loop1 * y_ratio)) * pwidth * preview->bytes;

            dest_pixel = dest_data +
              (loop2 + loop1 * width) * preview->bytes;

            for (i = 0; i < preview->bytes; i++)
              *dest_pixel++ = *src_pixel++;
          }

      gimp_preview_cache_add (plist, preview);

      return preview;
    }

#ifdef PREVIEW_CACHE_DEBUG
  g_print ("gimp_preview_cache_get returning NULL\n");
#endif

  return NULL;
}

gsize
gimp_preview_cache_get_memsize (GSList *cache)
{
  GSList *list;
  gsize   memsize = 0;

  g_return_val_if_fail (cache != NULL, 0);

  for (list = cache; list; list = list->next)
    memsize += sizeof (GSList) + temp_buf_get_memsize ((TempBuf *) list->data);

  return memsize;
}
