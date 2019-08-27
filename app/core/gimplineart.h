/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2017 Sébastien Fourey & David Tchumperlé
 * Copyright (C) 2018 Jehan
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

#ifndef __GIMP_LINEART__
#define __GIMP_LINEART__


#include "gimpobject.h"

#define GIMP_TYPE_LINE_ART            (gimp_line_art_get_type ())
#define GIMP_LINE_ART(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_LINE_ART, GimpLineArt))
#define GIMP_LINE_ART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_LINE_ART, GimpLineArtClass))
#define GIMP_IS_LINE_ART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_LINE_ART))
#define GIMP_IS_LINE_ART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_LINE_ART))
#define GIMP_LINE_ART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_LINE_ART, GimpLineArtClass))


typedef struct _GimpLineArtClass   GimpLineArtClass;
typedef struct _GimpLineArtPrivate GimpLineArtPrivate;

struct _GimpLineArt
{
  GimpObject          parent_instance;

  GimpLineArtPrivate *priv;
};

struct _GimpLineArtClass
{
  GimpObjectClass     parent_class;

  /* Signals */

  void (* computing_start) (GimpLineArt *line_art);
  void (* computing_end)   (GimpLineArt *line_art);
};


GType                gimp_line_art_get_type         (void) G_GNUC_CONST;

GimpLineArt        * gimp_line_art_new              (void);

void                 gimp_line_art_bind_gap_length  (GimpLineArt  *line_art,
                                                     gboolean      bound);

void                 gimp_line_art_set_input        (GimpLineArt  *line_art,
                                                     GimpPickable *pickable);
GimpPickable       * gimp_line_art_get_input        (GimpLineArt  *line_art);
void                 gimp_line_art_freeze           (GimpLineArt  *line_art);
void                 gimp_line_art_thaw             (GimpLineArt  *line_art);
gboolean             gimp_line_art_is_frozen        (GimpLineArt  *line_art);

GeglBuffer         * gimp_line_art_get              (GimpLineArt  *line_art,
                                                     gfloat      **distmap);

#endif /* __GIMP_LINEART__ */
