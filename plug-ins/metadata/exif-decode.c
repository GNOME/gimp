/* exif-decode.c - decodes exif data and converts it to XMP
 *
 * Copyright (C) 2004-2005, Róman Joost <romanofski@gimp.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include <glib.h>

#include <libgimp/gimp.h>

#include <libexif/exif-data.h>

#include "xmp-model.h"
#include "xmp-schemas.h"

#include "exif-decode.h"


static void         exif_foreach_content_cb     (ExifContent *content,
                                                 XMPModel    *xmp_model);
static void         exif_foreach_entry_cb       (ExifEntry   *entry,
                                                 XMPModel    *xmp_model);

/**
 * xmp_merge_from_exifbuffer:
 * @xmp_model: pointer to the #XMPModel in which the results will be stored
 * @image_ID: id of the image where the exif data parasite is attached to
 * @error: return location for a #GErrror
 *
 * Load the Exif data, which is attached to the image as a parasite. The
 * parsed Exif data is merged into the XMP model.
 *
 * Return value: %TRUE on success, %FALSE if an error occured during
 * reading/writing
 *
 **/
gboolean
xmp_merge_from_exifbuffer (XMPModel     *xmp_model,
                           gint32        image_ID,
                           GError       **error)
{
   ExifData *exif_data;
   GimpParasite *parasite = gimp_image_parasite_find (image_ID, "exif-data");

   if (!parasite)
     return FALSE;

   exif_data = exif_data_new_from_data (gimp_parasite_data (parasite),
                                        gimp_parasite_data_size (parasite));
   if (exif_data) {
     exif_data_foreach_content (exif_data,
                                (void *) exif_foreach_content_cb,
                                xmp_model);
   } else {
     return FALSE;
   }

   return TRUE;
}

static void
exif_foreach_content_cb (ExifContent *content,
                         XMPModel    *xmp_model)
{
   exif_content_foreach_entry (content, (void *) exif_foreach_entry_cb, xmp_model);
}

static void
exif_foreach_entry_cb (ExifEntry *entry,
                       XMPModel  *xmp_model)
{
   char value[1024];

   xmp_model_set_scalar_property (xmp_model,
                                  XMP_SCHEMA_EXIF,
                                  exif_tag_get_name (entry->tag),
                                  exif_entry_get_value (entry, value, sizeof (value)));
}
