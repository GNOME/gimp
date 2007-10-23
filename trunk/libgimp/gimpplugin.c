/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include "gimp.h"

gboolean
gimp_plugin_icon_register (const gchar  *procedure_name,
                           GimpIconType  icon_type,
                           const guint8 *icon_data)
{
  gint icon_data_length;

  g_return_val_if_fail (procedure_name != NULL, FALSE);
  g_return_val_if_fail (icon_data != NULL, FALSE);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_STOCK_ID:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      icon_data_length = strlen ((const gchar *) icon_data) + 1;
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      g_return_val_if_fail (g_ntohl (*((gint32 *) icon_data)) == 0x47646b50,
                            FALSE);

      icon_data_length = g_ntohl (*((gint32 *) (icon_data + 4)));
      break;

    default:
      g_return_val_if_reached (FALSE);
    }

  return _gimp_plugin_icon_register (procedure_name,
                                     icon_type, icon_data_length, icon_data);
}
