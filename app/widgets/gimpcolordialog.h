/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacolordialog.h
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

#ifndef __LIGMA_COLOR_DIALOG_H__
#define __LIGMA_COLOR_DIALOG_H__


#include "ligmaviewabledialog.h"


#define LIGMA_COLOR_DIALOG_HISTORY_SIZE 12


#define LIGMA_TYPE_COLOR_DIALOG            (ligma_color_dialog_get_type ())
#define LIGMA_COLOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_DIALOG, LigmaColorDialog))
#define LIGMA_COLOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_DIALOG, LigmaColorDialogClass))
#define LIGMA_IS_COLOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_DIALOG))
#define LIGMA_IS_COLOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_DIALOG))
#define LIGMA_COLOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_DIALOG, LigmaColorDialogClass))


typedef struct _LigmaColorDialogClass LigmaColorDialogClass;

struct _LigmaColorDialog
{
  LigmaViewableDialog   parent_instance;

  gboolean             wants_updates;
  gboolean             user_context_aware;

  GtkWidget           *stack;
  GtkWidget           *selection;
  GtkWidget           *colormap_selection;

  LigmaImage           *active_image;
  gboolean             colormap_editing;
};

struct _LigmaColorDialogClass
{
  LigmaViewableDialogClass  parent_class;

  void (* update) (LigmaColorDialog      *dialog,
                   const LigmaRGB        *color,
                   LigmaColorDialogState  state);
};


GType       ligma_color_dialog_get_type  (void) G_GNUC_CONST;

GtkWidget * ligma_color_dialog_new       (LigmaViewable      *viewable,
                                         LigmaContext       *context,
                                         gboolean           context_aware,
                                         const gchar       *title,
                                         const gchar       *icon_name,
                                         const gchar       *desc,
                                         GtkWidget         *parent,
                                         LigmaDialogFactory *dialog_factory,
                                         const gchar       *dialog_identifier,
                                         const LigmaRGB     *color,
                                         gboolean           wants_update,
                                         gboolean           show_alpha);

void        ligma_color_dialog_set_color (LigmaColorDialog   *dialog,
                                         const LigmaRGB     *color);
void        ligma_color_dialog_get_color (LigmaColorDialog   *dialog,
                                         LigmaRGB           *color);


#endif /* __LIGMA_COLOR_DIALOG_H__ */
