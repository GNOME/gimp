/* gserialize.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
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

#include <string.h>

#include "gserialize.h"

struct _GSerialDescription
{
  char *struct_name;
  GSList *list;
};


struct _GSerialItem
{
  GSerialType type;          /* the type of this data member                 */
  gulong      offset;        /* the offset into the struct of this item      */
  glong       length;        /* the number of elements (if this is an array) */
		             /* or -1 if this is a variable length array     */
  gulong      length_offset; /* offset to the length of the array            */
};

#define g_serial_copy_from_n g_serial_copy_to_n

static long g_serial_copy_to_n(char *dest, char *source, long data_size,
			       long n_items)
{
  int i;
  int length = n_items*data_size;
#if G_BYTE_ORDER == G_BIG_ENDIAN
  memcpy(dest, source, length);
#else
  switch (data_size)
  {
   case 1: memcpy(dest, source, n_items);
     break;
   case 2: 
     for (i = 0; i < length; i+=2)
     {
       dest[i]   = source[i + 1];
       dest[i+1] = source[i];
     } break;
   case 4: 
     for (i = 0; i < length; i+=4)
     {
       dest[i]   = source[i + 3];
       dest[i+1] = source[i + 2];
       dest[i+2] = source[i + 1];
       dest[i+3] = source[i];
     } break;
   case 8: 
     for (i = 0; i < length; i+=8)
     {
       dest[i]   = source[i+7];
       dest[i+1] = source[i+6];
       dest[i+2] = source[i+5];
       dest[i+3] = source[i+4];
       dest[i+4] = source[i+3];
       dest[i+5] = source[i+2];
       dest[i+6] = source[i+1];
       dest[i+7] = source[i];
     } break;
   default:
     g_assert_not_reached();
  }
#endif /* G_BYTE_ORDER != G_BIG_ENDIAN */
  return length;
}

GSerialItem *g_new_serial_item(GSerialType type, gulong offset,
			       gint32 length, gulong length_offset)
{
  GSerialItem *item = g_new(GSerialItem, 1);
  item->type          = type;
  item->offset        = offset;
  item->length        = length;
  item->length_offset = length_offset;
  return item;
}

GSerialDescription *g_new_serial_description(char *name, ...)
{
  va_list argp;
  void *tmp;
  GSerialDescription *d = g_new(GSerialDescription, 1);

  d->struct_name = g_strdup(name);
  d->list = 0;

  va_start(argp, name);
  while ((tmp =  va_arg (argp, void*)))
    d->list = g_slist_append(d->list, tmp);
  va_end(argp);
  return d;
}

void g_free_serial_description(GSerialDescription *d)
{
  g_free (d->struct_name);
  while (d->list)
  {
    g_free(d->list->data);
    d->list = g_slist_remove_link(d->list, d->list);
  }
}

static int g_serial_item_is_array(GSerialItem *item)
{
  if (item->type < GSERIAL_STRING)
    return 0;
  return 1;
}

static long g_serial_item_data_size(GSerialItem *item)
{
  static long sizes[] = {0, 1, 2, 4, 4, 8, 1, 1, 2, 4, 4, 8};
  if (item->type >= GSERIAL_STRING)
    return sizes[item->type - 95];
  return sizes[item->type];
}

static long g_serial_item_n_items(GSerialItem *item, void *struct_data)
{
  if (item->type < GSERIAL_STRING)
    return 1;
  if (item->type == GSERIAL_STRING)
    return (strlen(G_STRUCT_MEMBER(char*, struct_data, item->offset)) + 1);
  if (item->length >= 0)
    return item->length;
  return (G_STRUCT_MEMBER(gint32, struct_data, item->length_offset));
}

static long g_serial_item_compute_length(GSerialItem *item, void *struct_data)
{
  int length;
  length = g_serial_item_n_items(item, struct_data) * g_serial_item_data_size(item);
  length += g_serial_item_is_array(item) * 4;
  return length + 1;
}

static long g_serial_description_compute_length(GSerialDescription *d,
						void *struct_data)
{
  long length = 0;
  GSList *list;
  list = d->list;
  while (list)
  {
    length += g_serial_item_compute_length((GSerialItem *) list->data, struct_data);
    list = list->next;
  }
  return length;
}

static long g_serial_item_serialize(GSerialItem *item, char *buffer,
				    void *struct_data)
{
  char *buf = buffer;
  gint32 tmp;
  if (item->type >= GSERIAL_LAST_TYPE ||
      (item->type > GSERIAL_DOUBLE && item->type < GSERIAL_STRING))
  {
    g_warning("Error serializing: Unknown serial item type.\n");
    return 0;
  }
  *buf++ = item->type;
  if (g_serial_item_is_array(item))
  {
    tmp = g_serial_item_n_items(item, struct_data);
    buf += g_serial_copy_to_n(buf, (char *)&tmp, 4, 1);
    buf += g_serial_copy_to_n(buf,
			      G_STRUCT_MEMBER(char*, struct_data,item->offset),
			      g_serial_item_data_size(item),
			      tmp);
  }
  else
  {
    buf += g_serial_copy_to_n(buf,
			      G_STRUCT_MEMBER_P(struct_data, item->offset),
			      g_serial_item_data_size(item), 1);
  }
  return (buf - buffer);
}

static long g_serial_item_deserialize(GSerialItem *item, void *struct_data, 
				      char *buffer)
{
  char *buf = buffer;
  gint32 n_items;
  if (*buf != item->type)
  {
    g_warning("Error deserializing: item types do not match: %d vs %d.\n",
	      *buf, item->type);
    return 0;
  }
  buf++;
  if (g_serial_item_is_array(item))
  {
    buf += g_serial_copy_from_n((char *)&n_items, buf, 4, 1);
    if (item->length < 0)
      G_STRUCT_MEMBER(gint32, struct_data, item->length_offset) = n_items;
    G_STRUCT_MEMBER(void*, struct_data, item->offset) 
      = g_malloc(n_items*g_serial_item_data_size(item));
    buf += g_serial_copy_from_n(G_STRUCT_MEMBER(void *, struct_data,
						item->offset),
				buf,
				g_serial_item_data_size(item),
				n_items);
  }
  else
  {
    buf += g_serial_copy_from_n(G_STRUCT_MEMBER_P(struct_data, item->offset),
				buf,
				g_serial_item_data_size(item),
				1);
  }
  return (buf - buffer);
}

long g_serialize(GSerialDescription *d,  void **output, void *struct_data)
{
  int length = g_serial_description_compute_length(d, struct_data);
  char *outbuf;
  GSList *list;
  long item_length;
  outbuf = (char *)g_malloc(length);
  *output = outbuf;

  list = d->list;
  while (list)
  {
    item_length = g_serial_item_serialize((GSerialItem *)list->data, outbuf,
					  struct_data);
    if (item_length == 0)
      g_error("Error serializing %s\n", d->struct_name);
    outbuf += item_length;
    list = list->next;
  }
  return length;
}

long g_deserialize(GSerialDescription *d,  void *struct_data, void *serial)
{
  GSList *list;
  char *in_buf = serial;
  char *out_buf = struct_data;
  list = d->list;
  while (list)
  {
    in_buf += g_serial_item_deserialize((GSerialItem *)list->data, 
					out_buf, in_buf);
    list = list->next;
  }
  return (in_buf - (char *)serial);
}
