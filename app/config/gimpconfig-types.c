/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ParamSpecs for config objects
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
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

#include <stdio.h>
#include <stdlib.h>

#include <glib-object.h>

#include "gimpconfig-types.h"


static void  memsize_to_string (const GValue *src_value,
                                GValue       *dest_value);
static void  string_to_memsize (const GValue *src_value,
                                GValue       *dest_value);


GType
gimp_memsize_get_type (void)
{
  static GType memsize_type = 0;

  if (!memsize_type)
    {
      static const GTypeInfo type_info = { 0, };

      memsize_type = g_type_register_static (G_TYPE_UINT, "GimpMemsize", 
                                             &type_info, 0);

      g_value_register_transform_func (memsize_type, G_TYPE_STRING,
                                       memsize_to_string);
      g_value_register_transform_func (G_TYPE_STRING, memsize_type,
                                       string_to_memsize);
    }
  
  return memsize_type;
}

static void
memsize_to_string (const GValue *src_value,
                   GValue       *dest_value)
{
  guint size;

  size = src_value->data[0].v_uint;
  
  if (size > (1 << 30) && size % (1 << 30) == 0)
    dest_value->data[0].v_pointer = g_strdup_printf ("%uG", size >> 30);
  else if (size > (1 << 20) && size % (1 << 20) == 0)
    dest_value->data[0].v_pointer = g_strdup_printf ("%uM", size >> 20);
  else if (size > (1 << 10) && size % (1 << 10) == 0)
    dest_value->data[0].v_pointer = g_strdup_printf ("%uk", size >> 10);
  else
    dest_value->data[0].v_pointer = g_strdup_printf ("%u", size);
};

static void
string_to_memsize (const GValue *src_value,
                   GValue       *dest_value)
{
  gchar *end;
  guint  size;

  if (!src_value->data[0].v_pointer)
    goto error;

  size = strtoul (src_value->data[0].v_pointer, &end, 0);
  if (size == ULONG_MAX)
    goto error;

  if (end && *end)
    {
      guint shift;
      
      switch (g_ascii_tolower (*end))
        {
        case 'b':
          shift = 0;
          break;
        case 'k':
          shift = 10;
          break;
        case 'm':
          shift = 20;
          break;
        case 'g':
          shift = 30;
          break;
        default:
          goto error;
        }
      
      size <<= shift;
    }
  
  g_value_set_uint (dest_value, size);
  
  return;
  
 error:
  g_warning ("Can't convert string to memory size.");
};
