/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_LINEART__
#define __LIGMA_LINEART__


#include "ligmaobject.h"

#define LIGMA_TYPE_LINE_ART            (ligma_line_art_get_type ())
#define LIGMA_LINE_ART(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_LINE_ART, LigmaLineArt))
#define LIGMA_LINE_ART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_LINE_ART, LigmaLineArtClass))
#define LIGMA_IS_LINE_ART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_LINE_ART))
#define LIGMA_IS_LINE_ART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_LINE_ART))
#define LIGMA_LINE_ART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_LINE_ART, LigmaLineArtClass))


typedef struct _LigmaLineArtClass   LigmaLineArtClass;
typedef struct _LigmaLineArtPrivate LigmaLineArtPrivate;

struct _LigmaLineArt
{
  LigmaObject          parent_instance;

  LigmaLineArtPrivate *priv;
};

struct _LigmaLineArtClass
{
  LigmaObjectClass     parent_class;

  /* Signals */

  void (* computing_start) (LigmaLineArt *line_art);
  void (* computing_end)   (LigmaLineArt *line_art);
};


GType                ligma_line_art_get_type         (void) G_GNUC_CONST;

LigmaLineArt        * ligma_line_art_new              (void);

void                 ligma_line_art_bind_gap_length  (LigmaLineArt  *line_art,
                                                     gboolean      bound);

void                 ligma_line_art_set_input        (LigmaLineArt  *line_art,
                                                     LigmaPickable *pickable);
LigmaPickable       * ligma_line_art_get_input        (LigmaLineArt  *line_art);
void                 ligma_line_art_freeze           (LigmaLineArt  *line_art);
void                 ligma_line_art_thaw             (LigmaLineArt  *line_art);
gboolean             ligma_line_art_is_frozen        (LigmaLineArt  *line_art);

GeglBuffer         * ligma_line_art_get              (LigmaLineArt  *line_art,
                                                     gfloat      **distmap);

#endif /* __LIGMA_LINEART__ */
