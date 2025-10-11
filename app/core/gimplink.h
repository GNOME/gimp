/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpLink
 * Copyright (C) 2019 Jehan
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

#include "gimpitem.h"


#define GIMP_TYPE_LINK            (gimp_link_get_type ())
#define GIMP_LINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LINK, GimpLink))
#define GIMP_LINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LINK, GimpLinkClass))
#define GIMP_IS_LINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LINK))
#define GIMP_IS_LINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LINK))
#define GIMP_LINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LINK, GimpLinkClass))


typedef struct _GimpLinkClass   GimpLinkClass;
typedef struct _GimpLinkPrivate GimpLinkPrivate;

struct _GimpLink
{
  GimpObject         parent_instance;

  GimpLinkPrivate   *p;
};

struct _GimpLinkClass
{
  GimpObjectClass    parent_class;

  void   (* changed)              (GimpLink  *link);
};


GType                 gimp_link_get_type          (void) G_GNUC_CONST;

GimpLink            * gimp_link_new               (Gimp           *gimp,
                                                   GFile          *file,
                                                   gint            vector_width,
                                                   gint            vector_height,
                                                   gboolean        keep_ratio,
                                                   GimpProgress  *progress,
                                                   GError       **error);
GimpLink            * gimp_link_duplicate         (GimpLink       *link);

GFile               * gimp_link_get_file          (GimpLink       *link,
                                                   GFile          *parent,
                                                   gchar         **path);
void                  gimp_link_set_file          (GimpLink       *layer,
                                                   GFile          *file,
                                                   gint            vector_width,
                                                   gint            vector_height,
                                                   gboolean        keep_ratio,
                                                   GimpProgress   *progress,
                                                   GError        **error);
gboolean              gimp_link_get_absolute_path (GimpLink       *link);
void                  gimp_link_set_absolute_path (GimpLink       *link,
                                                   gboolean        absolute_path);

const gchar         * gimp_link_get_mime_type     (GimpLink       *link);

void                  gimp_link_freeze            (GimpLink       *link);
void                  gimp_link_thaw              (GimpLink       *link);
gboolean              gimp_link_is_monitored      (GimpLink       *link);

gboolean              gimp_link_is_broken         (GimpLink       *link);

void                  gimp_link_set_size          (GimpLink       *link,
                                                   gint            width,
                                                   gint            height,
                                                   gboolean        keep_ratio);
void                  gimp_link_get_size          (GimpLink       *link,
                                                   gint           *width,
                                                   gint           *height);
GimpImageBaseType     gimp_link_get_base_type     (GimpLink      *link);
GimpPrecision         gimp_link_get_precision     (GimpLink      *link);
GimpPlugInProcedure * gimp_link_get_load_proc     (GimpLink      *link);

gboolean              gimp_link_is_vector         (GimpLink       *link);

GeglBuffer          * gimp_link_get_buffer        (GimpLink       *link);
