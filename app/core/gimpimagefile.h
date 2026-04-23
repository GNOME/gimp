/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimagefile.h
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *                          Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "gimpviewable.h"


#define GIMP_TYPE_IMAGEFILE (gimp_imagefile_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpImagefile,
                          gimp_imagefile,
                          GIMP, IMAGEFILE,
                          GimpViewable)


struct _GimpImagefileClass
{
  GimpViewableClass  parent_class;

  void (* info_changed) (GimpImagefile *imagefile);
};


GimpImagefile * gimp_imagefile_new                   (Gimp           *gimp,
                                                      GFile          *file);

GFile         * gimp_imagefile_get_file              (GimpImagefile  *imagefile);
void            gimp_imagefile_set_file              (GimpImagefile  *imagefile,
                                                      GFile          *file);

GimpThumbnail * gimp_imagefile_get_thumbnail         (GimpImagefile  *imagefile);
GIcon         * gimp_imagefile_get_gicon             (GimpImagefile  *imagefile);

void            gimp_imagefile_set_mime_type         (GimpImagefile  *imagefile,
                                                      const gchar    *mime_type);
void            gimp_imagefile_update                (GimpImagefile  *imagefile);
gboolean        gimp_imagefile_create_thumbnail      (GimpImagefile  *imagefile,
                                                      GimpContext    *context,
                                                      GimpProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace,
                                                      GError        **error);
void            gimp_imagefile_create_thumbnail_weak (GimpImagefile  *imagefile,
                                                      GimpContext    *context,
                                                      GimpProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace);
gboolean        gimp_imagefile_check_thumbnail       (GimpImagefile  *imagefile);
gboolean        gimp_imagefile_save_thumbnail        (GimpImagefile  *imagefile,
                                                      const gchar    *mime_type,
                                                      GimpImage      *image,
                                                      GError        **error);
const gchar   * gimp_imagefile_get_desc_string       (GimpImagefile  *imagefile);
