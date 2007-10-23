/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_SELECTION_H__
#define __GIMP_SELECTION_H__


#include "gimpchannel.h"


#define GIMP_TYPE_SELECTION            (gimp_selection_get_type ())
#define GIMP_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SELECTION, GimpSelection))
#define GIMP_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SELECTION, GimpSelectionClass))
#define GIMP_IS_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SELECTION))
#define GIMP_IS_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SELECTION))
#define GIMP_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SELECTION, GimpSelectionClass))


typedef struct _GimpSelectionClass GimpSelectionClass;

struct _GimpSelection
{
  GimpChannel parent_instance;

  gboolean    stroking;
};

struct _GimpSelectionClass
{
  GimpChannelClass parent_class;
};


GType         gimp_selection_get_type   (void) G_GNUC_CONST;

GimpChannel * gimp_selection_new        (GimpImage    *image,
                                         gint          width,
                                         gint          height);

void          gimp_selection_load       (GimpChannel  *selection,
                                         GimpChannel  *channel);
GimpChannel * gimp_selection_save       (GimpChannel  *selection);

TileManager * gimp_selection_extract    (GimpChannel  *selection,
                                         GimpPickable *pickable,
                                         GimpContext  *context,
                                         gboolean      cut_image,
                                         gboolean      keep_indexed,
                                         gboolean      add_alpha);

GimpLayer   * gimp_selection_float      (GimpChannel  *selection,
                                         GimpDrawable *drawable,
                                         GimpContext  *context,
                                         gboolean      cut_image,
                                         gint          off_x,
                                         gint          off_y);


#endif /* __GIMP_SELECTION_H__ */
