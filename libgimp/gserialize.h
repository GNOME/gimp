/* gserialize.h
 * Copyright (C) 1998 Jay Cox <jaycox@earthlink.net>
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

#ifndef __GSERIALIZE_H__
#define __GSERIALIZE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <glib.h>
#include <stdarg.h>

typedef enum {
  GSERIAL_END          = 0,           /* for internal use only */
  GSERIAL_INT8         = 1,
  GSERIAL_INT16        = 2,
  GSERIAL_INT32        = 3,
  GSERIAL_FLOAT        = 4,         /* 32 bit IEEE fp value */
  GSERIAL_DOUBLE       = 5,        /* 64 bit IEEE fp value */
  GSERIAL_STRING       = 101,
  GSERIAL_INT8ARRAY    = 102,
  GSERIAL_INT16ARRAY   = 103,
  GSERIAL_INT32ARRAY   = 104,
  GSERIAL_FLOATARRAY   = 105,
  GSERIAL_DOUBLEARRAY  = 106,
  GSERIAL_LAST_TYPE    = 107
} GSerialType;

typedef struct _GSerialItem GSerialItem;
typedef struct _GSerialDescription GSerialDescription;


GSerialItem *g_new_serial_item(GSerialType type, gulong offset,
			       gint32 length, gulong length_offset);

#define g_serial_item(type, struct_, member) \
  g_new_serial_item(type, G_STRUCT_OFFSET(struct_, member), 0, 0)

#define g_serial_array(type, struct_, member, length) \
  g_new_serial_item(type, G_STRUCT_OFFSET(struct_, member), length, 0)

#define g_serial_vlen_array(type, struct_, member, length_member) \
  g_new_serial_item(type, G_STRUCT_OFFSET(struct_, member), -1,   \
		    G_STRUCT_OFFSET(struct_, length_member))

GSerialDescription *g_new_serial_description(char *name, ...);
                       /* pass it g_serial_s.  null terminated */
void g_free_serial_description(GSerialDescription *);

long g_serialize(GSerialDescription *d, void **output, void *struct_data);
   /* returns the length of the serialized data.
      g_mallocs space for the data and stores a pointer to it in output */

long g_deserialize(GSerialDescription *d,  void *output, void *serial);
   /* output must be a preallocated area large enough to hold the
      deserialized struct. */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __GSERIALIZE_H__*/

