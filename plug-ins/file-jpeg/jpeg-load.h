/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __JPEG_LOAD_H__
#define __JPEG_LOAD_H__

/* Path record types */
typedef enum {
  PSD_PATH_CL_LEN    = 0, /* Closed sub-path length record */
  PSD_PATH_CL_LNK    = 1, /* Closed sub-path Bezier knot, linked */
  PSD_PATH_CL_UNLNK  = 2, /* Closed sub-path Bezier knot, unlinked */
  PSD_PATH_OP_LEN    = 3, /* Open sub-path length record */
  PSD_PATH_OP_LNK    = 4, /* Open sub-path Bezier knot, linked */
  PSD_PATH_OP_UNLNK  = 5, /* Open sub-path Bezier knot, unlinked */
  PSD_PATH_FILL_RULE = 6, /* Path fill rule record */
  PSD_PATH_CLIPBOARD = 7, /* Path clipboard record */
  PSD_PATH_FILL_INIT = 8  /* Path initial fill record */
} PSDpathtype;

GimpImage * load_image           (GFile        *file,
                                  GimpRunMode   runmode,
                                  gboolean      preview,
                                  gboolean     *resolution_loaded,
                                  GError      **error);

GimpImage * load_thumbnail_image (GFile         *file,
                                  gint          *width,
                                  gint          *height,
                                  GimpImageType *type,
                                  GError       **error);

#endif /* __JPEG_LOAD_H__ */
