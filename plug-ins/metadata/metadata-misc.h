/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2016, 2017 Ben Touchette
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

#ifndef __METADATA_MISC_H__
#define __METADATA_MISC_H__

typedef struct
{
  GtkWidget      *dialog;
  GHashTable     *widgets;
  GExiv2Metadata *metadata;
  GimpImage      *image;
  gchar          *filename;
} metadata_editor;

typedef enum
{
  MODE_SINGLE,
  MODE_MULTI,
  MODE_COMBO,
  MODE_LIST,
} MetadataMode;

typedef struct
{
  gchar        *tag;
  MetadataMode  mode;
  gint32        other_tag_index;
  gint32        tag_type;
  gint32        xmp_type;
} metadata_tag;

typedef struct
{
  gchar        *tag;
  MetadataMode  mode;
  gint32        default_tag_index;
  gint32        exif_tag_index;
} iptc_tag_info;

typedef struct
{
  gint32        xmp_equivalent_index;
  gchar        *tag;
  MetadataMode  mode;
} exif_tag_info;

typedef struct
{
  gchar  *data;
  gchar  *display;
} combobox_str_tag;

typedef struct
{
  gint32  data;
  gchar  *display;
} combobox_int_tag;

typedef struct
{
  gchar        *id;
  gchar        *tag;
  MetadataMode  mode;
  gint32        other_tag_index;
  gint32        tag_type;
} TranslateTag;

#endif /* __METADATA_MISC_H__ */

