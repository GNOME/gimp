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

#ifndef __GIMP_UNDO_H__
#define __GIMP_UNDO_H__


#include "gimpviewable.h"


struct _GimpUndoAccumulator
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


#define GIMP_TYPE_UNDO            (gimp_undo_get_type ())
#define GIMP_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_UNDO, GimpUndo))
#define GIMP_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_UNDO, GimpUndoClass))
#define GIMP_IS_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_UNDO))
#define GIMP_IS_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_UNDO))
#define GIMP_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_UNDO, GimpUndoClass))


typedef struct _GimpUndoClass GimpUndoClass;

struct _GimpUndo
{
  GimpViewable      parent_instance;

  GimpImage        *image;          /* the image this undo is part of     */
  guint             time;           /* time of undo step construction     */

  GimpUndoType      undo_type;      /* undo type                          */
  GimpDirtyMask     dirty_mask;     /* affected parts of the image        */

  GimpTempBuf      *preview;
  guint             preview_idle_id;
};

struct _GimpUndoClass
{
  GimpViewableClass  parent_class;

  void (* pop)  (GimpUndo            *undo,
                 GimpUndoMode         undo_mode,
                 GimpUndoAccumulator *accum);
  void (* free) (GimpUndo            *undo,
                 GimpUndoMode         undo_mode);
};


GType         gimp_undo_get_type        (void) G_GNUC_CONST;

void          gimp_undo_pop             (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode,
                                         GimpUndoAccumulator *accum);
void          gimp_undo_free            (GimpUndo            *undo,
                                         GimpUndoMode         undo_mode);

void          gimp_undo_create_preview  (GimpUndo            *undo,
                                         GimpContext         *context,
                                         gboolean             create_now);
void          gimp_undo_refresh_preview (GimpUndo            *undo,
                                         GimpContext         *context);

const gchar * gimp_undo_type_to_name    (GimpUndoType         type);

gboolean      gimp_undo_is_weak         (GimpUndo            *undo);
gint          gimp_undo_get_age         (GimpUndo            *undo);
void          gimp_undo_reset_age       (GimpUndo            *undo);


#endif /* __GIMP_UNDO_H__ */
