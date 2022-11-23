/* LIBLIGMA - The LIGMA Library
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

#include <libligmacolor/ligmacolortypes.h>

#include "ligmawire.h"


typedef struct _LigmaWireHandler  LigmaWireHandler;

struct _LigmaWireHandler
{
  guint32             type;
  LigmaWireReadFunc    read_func;
  LigmaWireWriteFunc   write_func;
  LigmaWireDestroyFunc destroy_func;
};


static GHashTable        *wire_ht         = NULL;
static LigmaWireIOFunc     wire_read_func  = NULL;
static LigmaWireIOFunc     wire_write_func = NULL;
static LigmaWireFlushFunc  wire_flush_func = NULL;
static gboolean           wire_error_val  = FALSE;


static void  ligma_wire_init (void);


void
ligma_wire_register (guint32             type,
                    LigmaWireReadFunc    read_func,
                    LigmaWireWriteFunc   write_func,
                    LigmaWireDestroyFunc destroy_func)
{
  LigmaWireHandler *handler;

  if (! wire_ht)
    ligma_wire_init ();

  handler = g_hash_table_lookup (wire_ht, &type);

  if (! handler)
    handler = g_slice_new0 (LigmaWireHandler);

  handler->type         = type;
  handler->read_func    = read_func;
  handler->write_func   = write_func;
  handler->destroy_func = destroy_func;

  g_hash_table_insert (wire_ht, &handler->type, handler);
}

void
ligma_wire_set_reader (LigmaWireIOFunc read_func)
{
  wire_read_func = read_func;
}

void
ligma_wire_set_writer (LigmaWireIOFunc write_func)
{
  wire_write_func = write_func;
}

void
ligma_wire_set_flusher (LigmaWireFlushFunc flush_func)
{
  wire_flush_func = flush_func;
}

gboolean
ligma_wire_read (GIOChannel *channel,
                guint8     *buf,
                gsize       count,
                gpointer    user_data)
{
  if (wire_read_func)
    {
      if (!(* wire_read_func) (channel, buf, count, user_data))
        {
          /* Gives a confusing error message most of the time, disable:
          g_warning ("%s: ligma_wire_read: error", g_get_prgname ());
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

          if (G_UNLIKELY (status != G_IO_STATUS_NORMAL))
            {
              if (error)
                {
                  g_warning ("%s: ligma_wire_read(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: ligma_wire_read(): error",
                             g_get_prgname ());
                }

              wire_error_val = TRUE;
              return FALSE;
            }

          if (G_UNLIKELY (bytes == 0))
            {
              g_warning ("%s: ligma_wire_read(): unexpected EOF",
                         g_get_prgname ());
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
ligma_wire_write (GIOChannel   *channel,
                 const guint8 *buf,
                 gsize         count,
                 gpointer      user_data)
{
  if (wire_write_func)
    {
      if (!(* wire_write_func) (channel, (guint8 *) buf, count, user_data))
        {
          g_warning ("%s: ligma_wire_write: error", g_get_prgname ());
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
                  g_warning ("%s: ligma_wire_write(): error: %s",
                             g_get_prgname (), error->message);
                  g_error_free (error);
                }
              else
                {
                  g_warning ("%s: ligma_wire_write(): error",
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
ligma_wire_flush (GIOChannel *channel,
                 gpointer    user_data)
{
  if (wire_flush_func)
    return (* wire_flush_func) (channel, user_data);

  return FALSE;
}

gboolean
ligma_wire_error (void)
{
  return wire_error_val;
}

void
ligma_wire_clear_error (void)
{
  wire_error_val = FALSE;
}

gboolean
ligma_wire_read_msg (GIOChannel      *channel,
                    LigmaWireMessage *msg,
                    gpointer         user_data)
{
  LigmaWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("ligma_wire_read_msg: the wire protocol has not been initialized");

  if (wire_error_val)
    return !wire_error_val;

  if (! _ligma_wire_read_int32 (channel, &msg->type, 1, user_data))
    return FALSE;

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("ligma_wire_read_msg: could not find handler for message: %d",
             msg->type);

  (* handler->read_func) (channel, msg, user_data);

  return !wire_error_val;
}

gboolean
ligma_wire_write_msg (GIOChannel      *channel,
                     LigmaWireMessage *msg,
                     gpointer         user_data)
{
  LigmaWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("ligma_wire_write_msg: the wire protocol has not been initialized");

  if (wire_error_val)
    return !wire_error_val;

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("ligma_wire_write_msg: could not find handler for message: %d",
             msg->type);

  if (! _ligma_wire_write_int32 (channel, &msg->type, 1, user_data))
    return FALSE;

  (* handler->write_func) (channel, msg, user_data);

  return !wire_error_val;
}

void
ligma_wire_destroy (LigmaWireMessage *msg)
{
  LigmaWireHandler *handler;

  if (G_UNLIKELY (! wire_ht))
    g_error ("ligma_wire_destroy: the wire protocol has not been initialized");

  handler = g_hash_table_lookup (wire_ht, &msg->type);

  if (G_UNLIKELY (! handler))
    g_error ("ligma_wire_destroy: could not find handler for message: %d\n",
             msg->type);

  (* handler->destroy_func) (msg);
}

gboolean
_ligma_wire_read_int64 (GIOChannel *channel,
                       guint64    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _ligma_wire_read_int8 (channel,
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
_ligma_wire_read_int32 (GIOChannel *channel,
                       guint32    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _ligma_wire_read_int8 (channel,
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
_ligma_wire_read_int16 (GIOChannel *channel,
                       guint16    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  if (count > 0)
    {
      if (! _ligma_wire_read_int8 (channel,
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
_ligma_wire_read_int8 (GIOChannel *channel,
                      guint8     *data,
                      gint        count,
                      gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return ligma_wire_read (channel, data, count, user_data);
}

gboolean
_ligma_wire_read_double (GIOChannel *channel,
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

      if (! _ligma_wire_read_int8 (channel, tmp, 8, user_data))
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
_ligma_wire_read_string (GIOChannel  *channel,
                        gchar      **data,
                        gint         count,
                        gpointer     user_data)
{
  gint i;

  g_return_val_if_fail (count >= 0, FALSE);

  for (i = 0; i < count; i++)
    {
      guint32 tmp;

      if (! _ligma_wire_read_int32 (channel, &tmp, 1, user_data))
        return FALSE;

      if (tmp > 0)
        {
          data[i] = g_try_new (gchar, tmp);

          if (! data[i])
            {
              g_printerr ("%s: failed to allocate %u bytes\n", G_STRFUNC, tmp);
              return FALSE;
            }

          if (! _ligma_wire_read_int8 (channel,
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
_ligma_wire_read_color (GIOChannel *channel,
                       LigmaRGB    *data,
                       gint        count,
                       gpointer    user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return _ligma_wire_read_double (channel,
                                 (gdouble *) data, 4 * count, user_data);
}

gboolean
_ligma_wire_write_int64 (GIOChannel    *channel,
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

          if (! _ligma_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 8, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_ligma_wire_write_int32 (GIOChannel    *channel,
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

          if (! _ligma_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 4, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_ligma_wire_write_int16 (GIOChannel    *channel,
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

          if (! _ligma_wire_write_int8 (channel,
                                       (const guint8 *) &tmp, 2, user_data))
            return FALSE;
        }
    }

  return TRUE;
}

gboolean
_ligma_wire_write_int8 (GIOChannel   *channel,
                       const guint8 *data,
                       gint          count,
                       gpointer      user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return ligma_wire_write (channel, data, count, user_data);
}

gboolean
_ligma_wire_write_double (GIOChannel    *channel,
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

      if (! _ligma_wire_write_int8 (channel, tmp, 8, user_data))
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
_ligma_wire_write_string (GIOChannel  *channel,
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

      if (! _ligma_wire_write_int32 (channel, &tmp, 1, user_data))
        return FALSE;

      if (tmp > 0)
        if (! _ligma_wire_write_int8 (channel,
                                     (const guint8 *) data[i], tmp, user_data))
          return FALSE;
    }

  return TRUE;
}

gboolean
_ligma_wire_write_color (GIOChannel    *channel,
                        const LigmaRGB *data,
                        gint           count,
                        gpointer       user_data)
{
  g_return_val_if_fail (count >= 0, FALSE);

  return _ligma_wire_write_double (channel,
                                  (gdouble *) data, 4 * count, user_data);
}

static guint
ligma_wire_hash (const guint32 *key)
{
  return *key;
}

static gboolean
ligma_wire_compare (const guint32 *a,
                   const guint32 *b)
{
  return (*a == *b);
}

static void
ligma_wire_init (void)
{
  if (! wire_ht)
    wire_ht = g_hash_table_new ((GHashFunc) ligma_wire_hash,
                                (GCompareFunc) ligma_wire_compare);
}
