/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *             
 * This library is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _MSC_VER
#include <process.h>
#include <io.h>
#endif

#include "gimpwire.h"


typedef struct _WireHandler  WireHandler;

struct _WireHandler
{
  guint32 type;
  WireReadFunc read_func;
  WireWriteFunc write_func;
  WireDestroyFunc destroy_func;
};


static void  wire_init    (void);
static guint wire_hash    (guint32 *key);
static gint  wire_compare (guint32 *a,
			   guint32 *b);


static GHashTable *wire_ht = NULL;
static WireIOFunc wire_read_func = NULL;
static WireIOFunc wire_write_func = NULL;
static WireFlushFunc wire_flush_func = NULL;
static int wire_error_val = FALSE;


void
wire_register (guint32         type,
	       WireReadFunc    read_func,
	       WireWriteFunc   write_func,
	       WireDestroyFunc destroy_func)
{
  WireHandler *handler;

  if (!wire_ht)
    wire_init ();

  handler = g_hash_table_lookup (wire_ht, &type);
  if (!handler)
    handler = g_new (WireHandler, 1);

  handler->type = type;
  handler->read_func = read_func;
  handler->write_func = write_func;
  handler->destroy_func = destroy_func;

  g_hash_table_insert (wire_ht, &handler->type, handler);
}

void
wire_set_reader (WireIOFunc read_func)
{
  wire_read_func = read_func;
}

void
wire_set_writer (WireIOFunc write_func)
{
  wire_write_func = write_func;
}

void
wire_set_flusher (WireFlushFunc flush_func)
{
  wire_flush_func = flush_func;
}

int
wire_read (GIOChannel *channel,
	   guint8     *buf,
	   gulong      count)
{
  if (wire_read_func)
    {
      if (!(* wire_read_func) (channel, buf, count))
	{
	  g_warning ("wire_read: error");
	  wire_error_val = TRUE;
	  return FALSE;
	}
    }
  else
    {
      GIOError error;
      int bytes;

      while (count > 0)
	{
	  do {
	    bytes = 0;
	    error = g_io_channel_read (channel, (char*) buf, count, &bytes);
	  } while (error == G_IO_ERROR_AGAIN);

	  if (error != G_IO_ERROR_NONE)
	    {
	      g_warning ("wire_read: error");
	      wire_error_val = TRUE;
	      return FALSE;
	    }

	  if (bytes == 0) 
	    {
	      g_warning ("wire_read: unexpected EOF (plug-in crashed?)");
	      wire_error_val = TRUE;
	      return FALSE;
	    }

	  count -= bytes;
	  buf += bytes;
	}
    }

  return TRUE;
}

int
wire_write (GIOChannel *channel,
	    guint8     *buf,
	    gulong      count)
{
  if (wire_write_func)
    {
      if (!(* wire_write_func) (channel, buf, count))
	{
	  g_warning ("wire_write: error");
	  wire_error_val = TRUE;
	  return FALSE;
	}
    }
  else
    {
      GIOError error;
      int bytes;

      while (count > 0)
	{
	  do {
	    bytes = 0;
	    error = g_io_channel_write (channel, (char*) buf, count, &bytes);
	  } while (error == G_IO_ERROR_AGAIN);

	  if (error != G_IO_ERROR_NONE)
	    {
	      g_warning ("wire_write: error");
	      wire_error_val = TRUE;
	      return FALSE;
	    }

	  count -= bytes;
	  buf += bytes;
	}
    }

  return TRUE;
}

int
wire_flush (GIOChannel *channel)
{
  if (wire_flush_func)
    return (* wire_flush_func) (channel);
  return FALSE;
}

int
wire_error ()
{
  return wire_error_val;
}

void
wire_clear_error ()
{
  wire_error_val = FALSE;
}

int
wire_read_msg (GIOChannel  *channel,
	       WireMessage *msg)
{
  WireHandler *handler;

  if (wire_error_val)
    return !wire_error_val;

  if (!wire_read_int32 (channel, &msg->type, 1))
    return FALSE;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d", msg->type);

  (* handler->read_func) (channel, msg);

  return !wire_error_val;
}

int
wire_write_msg (GIOChannel  *channel,
		WireMessage *msg)
{
  WireHandler *handler;

  if (wire_error_val)
    return !wire_error_val;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d", msg->type);

  if (!wire_write_int32 (channel, &msg->type, 1))
    return FALSE;

  (* handler->write_func) (channel, msg);

  return !wire_error_val;
}

void
wire_destroy (WireMessage *msg)
{
  WireHandler *handler;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d\n", msg->type);

  (* handler->destroy_func) (msg);
}

int
wire_read_int32 (GIOChannel *channel,
		 guint32    *data,
		 gint        count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (channel, (guint8*) data, count * 4))
	return FALSE;

      while (count--)
	{
	  *data = g_ntohl (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int16 (GIOChannel *channel,
		 guint16    *data,
		 gint        count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (channel, (guint8*) data, count * 2))
	return FALSE;

      while (count--)
	{
	  *data = g_ntohs (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int8 (GIOChannel *channel,
		guint8     *data,
		gint        count)
{
  return wire_read (channel, data, count);
}

int
wire_read_double (GIOChannel *channel,
		  gdouble    *data,
		  gint        count)
{
  char *str;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_string (channel, &str, 1))
	return FALSE;
      sscanf (str, "%le", &data[i]);
      g_free (str);
    }

  return TRUE;
}

int
wire_read_string (GIOChannel  *channel,
		  gchar      **data,
		  gint         count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_int32 (channel, &tmp, 1))
	return FALSE;

      if (tmp > 0)
	{
	  data[i] = g_new (gchar, tmp);
	  if (!wire_read_int8 (channel, (guint8*) data[i], tmp))
	    {
	      g_free (data[i]);
	      return FALSE;
	    }
	}
      else
	{
	  data[i] = NULL;
	}
    }

  return TRUE;
}

int
wire_write_int32 (GIOChannel *channel,
		  guint32    *data,
		  gint        count)
{
  guint32 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = g_htonl (data[i]);
	  if (!wire_write_int8 (channel, (guint8*) &tmp, 4))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int16 (GIOChannel *channel,
		  guint16    *data,
		  gint        count)
{
  guint16 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = g_htons (data[i]);
	  if (!wire_write_int8 (channel, (guint8*) &tmp, 2))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int8 (GIOChannel *channel,
		 guint8     *data,
		 gint        count)
{
  return wire_write (channel, data, count);
}

int
wire_write_double (GIOChannel *channel,
		   gdouble    *data,
		   gint        count)
{
  gchar *t, buf[128];
  int i;

  t = buf;
  for (i = 0; i < count; i++)
    {
      sprintf (buf, "%0.50e", data[i]);
      if (!wire_write_string (channel, &t, 1))
	return FALSE;
    }

  return TRUE;
}

int
wire_write_string (GIOChannel  *channel,
		   gchar      **data,
		   gint         count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (data[i])
	tmp = strlen (data[i]) + 1;
      else
	tmp = 0;

      if (!wire_write_int32 (channel, &tmp, 1))
	return FALSE;
      if (tmp > 0)
	if (!wire_write_int8 (channel, (guint8*) data[i], tmp))
	  return FALSE;
    }

  return TRUE;
}

static void
wire_init ()
{
  if (!wire_ht)
    {
      wire_ht = g_hash_table_new ((GHashFunc) wire_hash,
				  (GCompareFunc) wire_compare);
    }
}

static guint
wire_hash (guint32 *key)
{
  return *key;
}

static gint
wire_compare (guint32 *a,
	      guint32 *b)
{
  return (*a == *b);
}
