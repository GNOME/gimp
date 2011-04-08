/* metadata.h - defines for the metadata editor
 *
 * Copyright (C) 2004-2005, Raphaël Quinet <raphael@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef __METADATA_H__
#define __METADATA_H__


#define EDITOR_PROC         "plug-in-metadata-editor"
#define DECODE_XMP_PROC     "plug-in-metadata-decode-xmp"
#define ENCODE_XMP_PROC     "plug-in-metadata-encode-xmp"
#define DECODE_EXIF_PROC    "plug-in-metadata-decode-exif"
#define ENCODE_EXIF_PROC    "plug-in-metadata-encode-exif"
#define GET_PROC            "plug-in-metadata-get"
#define SET_PROC            "plug-in-metadata-set"
#define GET_SIMPLE_PROC     "plug-in-metadata-get-simple"
#define SET_SIMPLE_PROC     "plug-in-metadata-set-simple"
#define IMPORT_PROC         "plug-in-metadata-import"
#define EXPORT_PROC         "plug-in-metadata-export"
#define PLUG_IN_BINARY      "metadata"
#define PLUG_IN_ROLE        "gimp-metadata"


#endif /* __METADATA_H__ */
