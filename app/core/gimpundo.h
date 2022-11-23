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

#ifndef __LIGMA_UNDO_H__
#define __LIGMA_UNDO_H__


#include "ligmaviewable.h"


struct _LigmaUndoAccumulator
{
  gboolean mode_changed;
  gboolean precision_changed;

  gboolean size_changed;
  gint     previous_origin_x;
  gint     previous_origin_y;
  gint     previous_width;
  gint     previous_height;

  gboolean resolution_changed;

  gboolean unit_changed;
};


#define LIGMA_TYPE_UNDO            (ligma_undo_get_type ())
#define LIGMA_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_UNDO, LigmaUndo))
#define LIGMA_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_UNDO, LigmaUndoClass))
#define LIGMA_IS_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_UNDO))
#define LIGMA_IS_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_UNDO))
#define LIGMA_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_UNDO, LigmaUndoClass))


typedef struct _LigmaUndoClass LigmaUndoClass;

struct _LigmaUndo
{
  LigmaViewable      parent_instance;

  LigmaImage        *image;          /* the image this undo is part of     */
  guint             time;           /* time of undo step construction     */

  LigmaUndoType      undo_type;      /* undo type                          */
  LigmaDirtyMask     dirty_mask;     /* affected parts of the image        */

  LigmaTempBuf      *preview;
  guint             preview_idle_id;
};

struct _LigmaUndoClass
{
  LigmaViewableClass  parent_class;

  void (* pop)  (LigmaUndo            *undo,
                 LigmaUndoMode         undo_mode,
                 LigmaUndoAccumulator *accum);
  void (* free) (LigmaUndo            *undo,
                 LigmaUndoMode         undo_mode);
};


GType         ligma_undo_get_type        (void) G_GNUC_CONST;

void          ligma_undo_pop             (LigmaUndo            *undo,
                                         LigmaUndoMode         undo_mode,
                                         LigmaUndoAccumulator *accum);
void          ligma_undo_free            (LigmaUndo            *undo,
                                         LigmaUndoMode         undo_mode);

void          ligma_undo_create_preview  (LigmaUndo            *undo,
                                         LigmaContext         *context,
                                         gboolean             create_now);
void          ligma_undo_refresh_preview (LigmaUndo            *undo,
                                         LigmaContext         *context);

const gchar * ligma_undo_type_to_name    (LigmaUndoType         type);

gboolean      ligma_undo_is_weak         (LigmaUndo            *undo);
gint          ligma_undo_get_age         (LigmaUndo            *undo);
void          ligma_undo_reset_age       (LigmaUndo            *undo);


#endif /* __LIGMA_UNDO_H__ */
