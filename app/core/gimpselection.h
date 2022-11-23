/* LIGMA - The GNU Image Manipulation Program
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

#ifndef __LIGMA_SELECTION_H__
#define __LIGMA_SELECTION_H__


#include "ligmachannel.h"


#define LIGMA_TYPE_SELECTION            (ligma_selection_get_type ())
#define LIGMA_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SELECTION, LigmaSelection))
#define LIGMA_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SELECTION, LigmaSelectionClass))
#define LIGMA_IS_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SELECTION))
#define LIGMA_IS_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SELECTION))
#define LIGMA_SELECTION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SELECTION, LigmaSelectionClass))


typedef struct _LigmaSelectionClass LigmaSelectionClass;

struct _LigmaSelection
{
  LigmaChannel parent_instance;

  gint        suspend_count;
};

struct _LigmaSelectionClass
{
  LigmaChannelClass parent_class;
};


GType         ligma_selection_get_type (void) G_GNUC_CONST;

LigmaChannel * ligma_selection_new      (LigmaImage     *image,
                                       gint           width,
                                       gint           height);

gint          ligma_selection_suspend  (LigmaSelection *selection);
gint          ligma_selection_resume   (LigmaSelection *selection);

GeglBuffer  * ligma_selection_extract  (LigmaSelection *selection,
                                       GList         *pickables,
                                       LigmaContext   *context,
                                       gboolean       cut_image,
                                       gboolean       keep_indexed,
                                       gboolean       add_alpha,
                                       gint          *offset_x,
                                       gint          *offset_y,
                                       GError       **error);

LigmaLayer   * ligma_selection_float    (LigmaSelection *selection,
                                       GList         *drawables,
                                       LigmaContext   *context,
                                       gboolean       cut_image,
                                       gint           off_x,
                                       gint           off_y,
                                       GError       **error);


#endif /* __LIGMA_SELECTION_H__ */
