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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */                                                                             
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
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
wire_read (int     fd,
	   guint8 *buf,
	   gulong  count)
{
  if (wire_read_func)
    {
      if (!(* wire_read_func) (fd, buf, count))
	{
	  g_print ("wire_read: error\n");
	  wire_error_val = TRUE;
	  return FALSE;
	}
    }
  else
    {
      int bytes;

      while (count > 0)
	{
	  do {
	    bytes = read (fd, (char*) buf, count);
	  } while ((bytes == -1) && ((errno == EAGAIN) || (errno == EINTR)));

	  if (bytes == -1)
	    {
	      g_print ("wire_read: error\n");
	      wire_error_val = TRUE;
	      return FALSE;
	    }

	  if (bytes == 0) 
	    {
	      g_print ("wire_read: unexpected EOF\n");
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
wire_write (int     fd,
	    guint8 *buf,
	    gulong  count)
{
  if (wire_write_func)
    {
      if (!(* wire_write_func) (fd, buf, count))
	{
	  g_print ("wire_write: error\n");
	  wire_error_val = TRUE;
	  return FALSE;
	}
    }
  else
    {
      int bytes;

      while (count > 0)
	{
	  do {
	    bytes = write (fd, (char*) buf, count);
	  } while ((bytes == -1) && ((errno == EAGAIN) || (errno == EINTR)));

	  if (bytes == -1)
	    {
	      g_print ("wire_write: error\n");
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
wire_flush (int fd)
{
  if (wire_flush_func)
    return (* wire_flush_func) (fd);
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
wire_read_msg (int          fd,
	       WireMessage *msg)
{
  WireHandler *handler;

  if (wire_error_val)
    return !wire_error_val;

  if (!wire_read_int32 (fd, &msg->type, 1))
    return FALSE;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d\n", msg->type);

  (* handler->read_func) (fd, msg);

  return !wire_error_val;
}

int
wire_write_msg (int          fd,
		WireMessage *msg)
{
  WireHandler *handler;

  if (wire_error_val)
    return !wire_error_val;

  handler = g_hash_table_lookup (wire_ht, &msg->type);
  if (!handler)
    g_error ("could not find handler for message: %d\n", msg->type);

  if (!wire_write_int32 (fd, &msg->type, 1))
    return FALSE;

  (* handler->write_func) (fd, msg);

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
wire_read_int32 (int      fd,
		 guint32 *data,
		 gint     count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (fd, (guint8*) data, count * 4))
	return FALSE;

      while (count--)
	{
	  *data = ntohl (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int16 (int      fd,
		 guint16 *data,
		 gint     count)
{
  if (count > 0)
    {
      if (!wire_read_int8 (fd, (guint8*) data, count * 2))
	return FALSE;

      while (count--)
	{
	  *data = ntohs (*data);
	  data++;
	}
    }

  return TRUE;
}

int
wire_read_int8 (int     fd,
		guint8 *data,
		gint    count)
{
  return wire_read (fd, data, count);
}

int
wire_read_double (int      fd,
		  gdouble *data,
		  gint     count)
{
  char *str;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_string (fd, &str, 1))
	return FALSE;
      sscanf (str, "%le", &data[i]);
      g_free (str);
    }

  return TRUE;
}

int
wire_read_string (int     fd,
		  gchar **data,
		  gint    count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (!wire_read_int32 (fd, &tmp, 1))
	return FALSE;

      if (tmp > 0)
	{
	  data[i] = g_new (gchar, tmp);
	  if (!wire_read_int8 (fd, (guint8*) data[i], tmp))
	    return FALSE;
	}
      else
	{
	  data[i] = NULL;
	}
    }

  return TRUE;
}

int
wire_write_int32 (int      fd,
		  guint32 *data,
		  gint     count)
{
  guint32 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = htonl (data[i]);
	  if (!wire_write_int8 (fd, (guint8*) &tmp, 4))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int16 (int      fd,
		  guint16 *data,
		  gint     count)
{
  guint16 tmp;
  int i;

  if (count > 0)
    {
      for (i = 0; i < count; i++)
	{
	  tmp = htons (data[i]);
	  if (!wire_write_int8 (fd, (guint8*) &tmp, 2))
	    return FALSE;
	}
    }

  return TRUE;
}

int
wire_write_int8 (int     fd,
		 guint8 *data,
		 gint    count)
{
  return wire_write (fd, data, count);
}

int
wire_write_double (int      fd,
		   gdouble *data,
		   gint     count)
{
  gchar *t, buf[128];
  int i;

  t = buf;
  for (i = 0; i < count; i++)
    {
      sprintf (buf, "%0.50e", data[i]);
      if (!wire_write_string (fd, &t, 1))
	return FALSE;
    }

  return TRUE;
}

int
wire_write_string (int     fd,
		   gchar **data,
		   gint    count)
{
  guint32 tmp;
  int i;

  for (i = 0; i < count; i++)
    {
      if (data[i])
	tmp = strlen (data[i]) + 1;
      else
	tmp = 0;

      if (!wire_write_int32 (fd, &tmp, 1))
	return FALSE;
      if (tmp > 0)
	if (!wire_write_int8 (fd, (guint8*) data[i], tmp))
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
