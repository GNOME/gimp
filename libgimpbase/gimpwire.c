/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include <libgimpcolor/gimpcolortypes.h>

#include "gimpwire.h"


typedef struct _GimpWireHandler  GimpWireHandler;

struct _GimpWireHandler
{
  guint32             type;
  GimpWireReadFunc    read_func;
  GimpWireWriteFunc   write_func;
  GimpWireDestroyFunc destroy_func;
};


static GHashTable        *wire_ht         = NULL;
static GimpWireIOFunc     wire_read_func  = NULL;
static GimpWireIOFunc     wire_write_func = NULL;
static GimpWireFlushFunc  wire_flush_func = NULL;
static gboolean           wire_error_val  = FALSE;


static void  gimp_wire_init (void);


void
gimp_wire_register (guint32             type,
                    GimpWireReadFunc    read_func,
                    GimpWireWriteFunc   write_func,
                    GimpWireDestroyFunc destroy_func)
{
  GimpWireHandler *handler;

  if (! wire_ht)
    gimp_wire_init ();

  handler = g_hash_table_lookup (wire_ht, &type);

  if (! handler)
    handler = g_slice_new0 (GimpWireHandler);

  handler->type         = type;
  handler->read_func    = read_func;
  handler->write_func   = write_func;
  handler->destroy_func = destroy_func;

  g_hash_table_insert (wire_ht, &handler->type, handler);
}

void
gimp_wire_set_reader (GimpWireIOFunc read_func)
{
  wire_read_func = read_func;
}

void
gimp_wire_set_writer (GimpWireIOFunc write_func)
{
  wire_write_func = write_func;
}

void
gimp_wire_set_flusher (GimpWireFlushFunc flush_func)
{
  wire_flush_func = flush_func;
}

gboolean
gimp_wire_read (GIOChannel *channel,
                guint8     *buf,
                gsize       count,
                gpointer    user_data)
{
  if (wire_read_func)
    {
      if (!(* wire_read_func) (channel, buf, count, user_data))
        {
          /* Gives a confusing error message most of the time, disable:
          g_warning ("%s: gimp_wire_read: error", g_get_prgname ());
           */
          wire_error_val = TRUE;
          return FALSE;
        }
    }
  else
    {
      GIOStatus  status;
      GError    *error = NULL;
      gsize      bytes;

      while (count > 0)
        {
          do
            {
              bytes = 0;
              status = g_io_channel_read_chars (channel,
                                                (gchar *) buf, count,
                                                &bytes,
                                                &error);
            }
          while (G_UNLIKELY (status == G_IO_STATUS_AGAIN));

          if (G_UNLIKELY (bytes == 0 && status == G_IO_STATUS_EOF))
            {
              g_warning ("%s: gimp_wire_read(): unexpected EOF",
                         g_get_prgname ());
              wire_error_val = TRUE;
              return FALSE;
            }
          else if (G_UNLIKELY (status != G_IO_STATUS_NORMAL))
            {
              if (error)
                {
                  g_warning ("%s: gimp_wire_read(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: gimp_wire_read(): error",
                             g_get_prgname ());
                }

              wire_error_val = TRUE;
              return FALSE;
            }

          count -= bytes;
          buf += bytes;
        }
    }

  return TRUE;
}

gboolean
gimp_wire_write (GIOChannel   *channel,
                 const guint8 *buf,
                 gsize         count,
                 gpointer      user_data)
{
  if (wire_write_func)
    {
      if (!(* wire_write_func) (channel, (guint8 *) buf, count, user_data))
        {
          g_warning ("%s: gimp_wire_write: error", g_get_prgname ());
          wire_error_val = TRUE;
          return FALSE;
        }
    }
  else
    {
      GIOStatus  status;
      GError    *error = NULL;
      gsize      bytes;

      while (count > 0)
        {
          do
            {
              bytes = 0;
              status = g_io_channel_write_chars (channel,
                                                 (const gchar *) buf, count,
                                                 &bytes,
                                                 &error);
            }
          while (G_UNLIKELY (status == G_IO_STATUS_AGAIN));

          if (G_UNLIKELY (status != G_IO_STATUS_NORMAL))
            {
              if (error)
                {
                  g_warning ("%s: gimp_wire_write(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: gimp_wire_write(): error",
                             g_get_prgname ());
                }

              wire_error_val = TRUE;
              return FALSE;
            }

          count -= bytes;
          buf += bytes;
        }
    }

  return TRUE;
}

gboolean
gimp_wire_flush (GIOChannel *channel,
                 gpointer    user_data)
{
  if (wire_flush_func)
    return (* wire_flush_func) (channel, user_data);

  return FALSE;
}

gboolean
gimp_wire_error (void)
{
  return wire_error_val;
}

void
gimp_wire_clear_error (void)
{
  wire_error_val = FALSE;
}

gboolean
gimp_wire_read_msg (GIOChannel      *channel,
                    GimpWireMessage *msg,
                    gpointer         user_data)
{
  GimpWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("gimp_wire_read_msg: the wire protocol has not been initialized");

  if (wire_error_val)
    return !wire_error_val;

  if (! _gimp_wire_read_int32 (channel, &msg->type, 1, user_data))
    return FALSE;

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("gimp_wire_read_msg: could not find handler for message: %d",
             msg->type);

  (* handler->read_func) (channel, msg, user_data);

  return !wire_error_val;
}

gboolean
gimp_wire_write_msg (GIOChannel      *channel,
                     GimpWireMessage *msg,
                     gpointer         user_data)
{
  GimpWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("gimp_wire_write_msg: the wire protocol has not been initialized");

  if (wire_error_val)
    return !wire_error_val;

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("gimp_wire_write_msg: could not find handler for message: %d",
             msg->type);

  if (! _gimp_wire_write_int32 (channel, &msg->type, 1, user_data))
    return FALSE;

  (* handler->write_func) (channel, msg, user_data);

  return !wire_error_val;
}

void
gimp_wire_destroy (GimpWireMessage *msg)
{
  GimpWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("gimp_wire_destroy: the wire protocol has not been initialized");

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("gimp_wire_destroy: could not find handler for message: %d\n",
             msg->type);

  (* handler->destroy_func) (msg);
}

gboolean
_gimp_wire_read_int64 (GIOChannel *channel,
                       guint64    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _gimp_wire_read_int8 (channel,
                                  (guint8 *) data, count * 8, user_data))
        return FALSE;

      while (count--)
        {
          *data = GUINT64_FROM_BE (*data);
          data++;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_read_int32 (GIOChannel *channel,
                       guint32    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _gimp_wire_read_int8 (channel,
                                  (guint8 *) data, count * 4, user_data))
        return FALSE;

      while (count--)
        {
          *data = g_ntohl (*data);
          data++;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_read_int16 (GIOChannel *channel,
                       guint16    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _gimp_wire_read_int8 (channel,
                                  (guint8 *) data, count * 2, user_data))
        return FALSE;

      while (count--)
        {
          *data = g_ntohs (*data);
          data++;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_read_int8 (GIOChannel *channel,
                      guint8     *data,
                      gint        count,
                      gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return gimp_wire_read (channel, data, count, user_data);
}

gboolean
_gimp_wire_read_double (GIOChannel *channel,
                        gdouble    *data,
                        gint        count,
                        gpointer    user_data)
{
  gdouble *t;
  guint8   tmp[8];
  gint     i;

  g_return_val_if_fail (count >= 0, FALSE);

  t = (gdouble *) tmp;

  for (i = 0; i < count; i++)
    {
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
      gint j;
#endif

      if (! _gimp_wire_read_int8 (channel, tmp, 8, user_data))
        return FALSE;

#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
      for (j = 0; j < 4; j++)
        {
          guint8 swap;

          swap       = tmp[j];
          tmp[j]     = tmp[7 - j];
          tmp[7 - j] = swap;
        }
#endif

      data[i] = *t;
    }

  return TRUE;
}

gboolean
_gimp_wire_read_string (GIOChannel  *channel,
                        gchar      **data,
                        gint         count,
                        gpointer     user_data)
{
  gint i;

  g_return_val_if_fail (count >= 0, FALSE);

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      if (! _gimp_wire_read_int32 (channel, &tmp, 1, user_data))
        return FALSE;

      if (tmp > 0)
        {
          data[i] = g_try_new (gchar, tmp);

          if (! data[i])
            {
              g_printerr ("%s: failed to allocate %u bytes\n", G_STRFUNC, tmp);
              return FALSE;
            }

          if (! _gimp_wire_read_int8 (channel,
                                      (guint8 *) data[i], tmp, user_data))
            {
              g_free (data[i]);
              return FALSE;
            }

          /*  make sure that the string is NULL-terminated  */
          data[i][tmp - 1] = '\0';
        }
      else
        {
          data[i] = NULL;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_read_gegl_color (GIOChannel  *channel,
                            GBytes     **pixel_data,
                            GBytes     **icc_data,
                            gchar      **encoding,
                            gint         count,
                            gpointer     user_data)
{
  gint i;

  g_return_val_if_fail (count >= 0, FALSE);

  for (i = 0; i < count; i++)
    {
      guint32 size;
      guint8  pixel[40];
      guint32 icc_length;

      if (! _gimp_wire_read_int32 (channel,
                                   &size, 1,
                                   user_data)                              ||
          size > 40                                                        ||
          ! _gimp_wire_read_int8 (channel, pixel, size, user_data)         ||
          ! _gimp_wire_read_string (channel, &(encoding[i]), 1, user_data) ||
          ! _gimp_wire_read_int32 (channel, &icc_length, 1, user_data))
        {
          g_clear_pointer (&(encoding[i]), g_free);
          return FALSE;
        }
      pixel_data[i] = (size > 0 ? g_bytes_new (pixel, size) : NULL);

      /* Read space (profile data). */

      icc_data[i] = NULL;
      if (icc_length > 0)
        {
          guint8 *icc;

          icc = g_new0 (guint8, icc_length);

          if (! _gimp_wire_read_int8 (channel, icc, icc_length, user_data))
            {
              g_clear_pointer (&(encoding[i]), g_free);
              g_clear_pointer (&icc, g_free);
              return FALSE;
            }

          icc_data[i] = g_bytes_new_take (icc, icc_length);
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_write_int64 (GIOChannel    *channel,
                        const guint64 *data,
                        gint           count,
                        gpointer       user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      gint i;

      for (i = 0; i < count; i++)
        {
          guint64 tmp = GUINT64_TO_BE (data[i]);

          if (! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 8, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_write_int32 (GIOChannel    *channel,
                        const guint32 *data,
                        gint           count,
                        gpointer       user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      gint i;

      for (i = 0; i < count; i++)
        {
          guint32 tmp = g_htonl (data[i]);

          if (! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 4, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_write_int16 (GIOChannel    *channel,
                        const guint16 *data,
                        gint           count,
                        gpointer       user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      gint i;

      for (i = 0; i < count; i++)
        {
          guint16 tmp = g_htons (data[i]);

          if (! _gimp_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 2, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_gimp_wire_write_int8 (GIOChannel   *channel,
                       const guint8 *data,
                       gint          count,
                       gpointer      user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return gimp_wire_write (channel, data, count, user_data);
}

gboolean
_gimp_wire_write_double (GIOChannel    *channel,
                         const gdouble *data,
                         gint           count,
                         gpointer       user_data)
{
  gdouble *t;
  guint8   tmp[8];
  gint     i;
#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
  gint     j;
#endif

  g_return_val_if_fail (count >= 0, FALSE);

  t = (gdouble *) tmp;

  for (i = 0; i < count; i++)
    {
      *t = data[i];

#if (G_BYTE_ORDER == G_LITTLE_ENDIAN)
      for (j = 0; j < 4; j++)
        {
          guint8 swap;

          swap       = tmp[j];
          tmp[j]     = tmp[7 - j];
          tmp[7 - j] = swap;
        }
#endif

      if (! _gimp_wire_write_int8 (channel, tmp, 8, user_data))
        return FALSE;

#if 0
      {
        gint k;

        g_print ("Wire representation of %f:\t", data[i]);

        for (k = 0; k < 8; k++)
          g_print ("%02x ", tmp[k]);

        g_print ("\n");
      }
#endif
    }

  return TRUE;
}

gboolean
_gimp_wire_write_string (GIOChannel  *channel,
                         gchar      **data,
                         gint         count,
                         gpointer     user_data)
{
  gint i;

  g_return_val_if_fail (count >= 0, FALSE);

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      if (data[i])
        tmp = strlen (data[i]) + 1;
      else
        tmp = 0;

      if (! _gimp_wire_write_int32 (channel, &tmp, 1, user_data))
        return FALSE;

      if (tmp > 0)
        if (! _gimp_wire_write_int8 (channel,
                                     (const guint8 *) data[i], tmp, user_data))
          return FALSE;
    }

  return TRUE;
}

gboolean
_gimp_wire_write_gegl_color (GIOChannel  *channel,
                             GBytes     **pixel_data,
                             GBytes     **icc_data,
                             gchar      **encoding,
                             gint         count,
                             gpointer     user_data)
{
  gint i;

  g_return_val_if_fail (count >= 0, FALSE);

  for (i = 0; i < count; i++)
    {
      const guint8 *pixel      = NULL;
      gsize         bpp        = 0;
      const guint8 *icc        = NULL;
      gsize         icc_length = 0;

      if (pixel_data[i])
        pixel = g_bytes_get_data (pixel_data[i], &bpp);
      if (icc_data[i])
        icc = g_bytes_get_data (icc_data[i], &icc_length);

      if (! _gimp_wire_write_int32 (channel, (const guint32 *) &bpp, 1, user_data))
        return FALSE;

      if (bpp > 0 && ! _gimp_wire_write_int8 (channel, pixel, bpp, user_data))
        return FALSE;

      if (! _gimp_wire_write_string (channel, &(encoding[i]), 1, user_data) ||
          ! _gimp_wire_write_int32 (channel, (const guint32 *) &icc_length, 1, user_data))
        return FALSE;

      if (icc_length > 0 && ! _gimp_wire_write_int8 (channel, icc, icc_length, user_data))
        return FALSE;
    }

  return TRUE;
}

static guint
gimp_wire_hash (const guint32 *key)
{
  return *key;
}

static gboolean
gimp_wire_compare (const guint32 *a,
                   const guint32 *b)
{
  return (*a == *b);
}

static void
gimp_wire_init (void)
{
  if (! wire_ht)
    wire_ht = g_hash_table_new ((GHashFunc) gimp_wire_hash,
                                (GCompareFunc) gimp_wire_compare);
}
