/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimagefile.h
 *
 * Thumbnail handling according to the Thumbnail Managing Standard.
 * https://specifications.freedesktop.org/thumbnail-spec/
 *
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
 *                          Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_IMAGEFILE_H__
#define __LIGMA_IMAGEFILE_H__


#include "ligmaviewable.h"


#define LIGMA_TYPE_IMAGEFILE            (ligma_imagefile_get_type ())
#define LIGMA_IMAGEFILE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_IMAGEFILE, LigmaImagefile))
#define LIGMA_IMAGEFILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_IMAGEFILE, LigmaImagefileClass))
#define LIGMA_IS_IMAGEFILE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_IMAGEFILE))
#define LIGMA_IS_IMAGEFILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_IMAGEFILE))
#define LIGMA_IMAGEFILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_IMAGEFILE, LigmaImagefileClass))


typedef struct _LigmaImagefileClass LigmaImagefileClass;

struct _LigmaImagefile
{
  LigmaViewable  parent_instance;
};

struct _LigmaImagefileClass
{
  LigmaViewableClass   parent_class;

  void (* info_changed) (LigmaImagefile *imagefile);
};


GType           ligma_imagefile_get_type              (void) G_GNUC_CONST;

LigmaImagefile * ligma_imagefile_new                   (Ligma           *ligma,
                                                      GFile          *file);

GFile         * ligma_imagefile_get_file              (LigmaImagefile  *imagefile);
void            ligma_imagefile_set_file              (LigmaImagefile  *imagefile,
                                                      GFile          *file);

LigmaThumbnail * ligma_imagefile_get_thumbnail         (LigmaImagefile  *imagefile);
GIcon         * ligma_imagefile_get_gicon             (LigmaImagefile  *imagefile);

void            ligma_imagefile_set_mime_type         (LigmaImagefile  *imagefile,
                                                      const gchar    *mime_type);
void            ligma_imagefile_update                (LigmaImagefile  *imagefile);
gboolean        ligma_imagefile_create_thumbnail      (LigmaImagefile  *imagefile,
                                                      LigmaContext    *context,
                                                      LigmaProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace,
                                                      GError        **error);
void            ligma_imagefile_create_thumbnail_weak (LigmaImagefile  *imagefile,
                                                      LigmaContext    *context,
                                                      LigmaProgress   *progress,
                                                      gint            size,
                                                      gboolean        replace);
gboolean        ligma_imagefile_check_thumbnail       (LigmaImagefile  *imagefile);
gboolean        ligma_imagefile_save_thumbnail        (LigmaImagefile  *imagefile,
                                                      const gchar    *mime_type,
                                                      LigmaImage      *image,
                                                      GError        **error);
const gchar   * ligma_imagefile_get_desc_string       (LigmaImagefile  *imagefile);


#endif /* __LIGMA_IMAGEFILE_H__ */
