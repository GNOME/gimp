/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpcolordialog.h
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

#include "gimpviewabledialog.h"


#define GIMP_COLOR_DIALOG_HISTORY_SIZE 12


#define GIMP_TYPE_COLOR_DIALOG            (gimp_color_dialog_get_type ())
#define GIMP_COLOR_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_COLOR_DIALOG, GimpColorDialog))
#define GIMP_COLOR_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_COLOR_DIALOG, GimpColorDialogClass))
#define GIMP_IS_COLOR_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_COLOR_DIALOG))
#define GIMP_IS_COLOR_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_COLOR_DIALOG))
#define GIMP_COLOR_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_COLOR_DIALOG, GimpColorDialogClass))


typedef struct _GimpColorDialogClass GimpColorDialogClass;

struct _GimpColorDialog
{
  GimpViewableDialog   parent_instance;

  gboolean             wants_updates;
  gboolean             user_context_aware;

  GtkWidget           *stack;
  GtkWidget           *selection;
  GtkWidget           *colormap_selection;

  GimpImage           *active_image;
  gboolean             colormap_editing;
};

struct _GimpColorDialogClass
{
  GimpViewableDialogClass  parent_class;

  void (* update) (GimpColorDialog      *dialog,
                   GeglColor            *color,
                   GimpColorDialogState  state);
};


GType       gimp_color_dialog_get_type  (void) G_GNUC_CONST;

GtkWidget * gimp_color_dialog_new       (GimpViewable      *viewable,
                                         GimpContext       *context,
                                         gboolean           context_aware,
                                         const gchar       *title,
                                         const gchar       *icon_name,
                                         const gchar       *desc,
                                         GtkWidget         *parent,
                                         GimpDialogFactory *dialog_factory,
                                         const gchar       *dialog_identifier,
                                         GeglColor         *color,
                                         gboolean           wants_update,
                                         gboolean           show_alpha);

void        gimp_color_dialog_set_color (GimpColorDialog   *dialog,
                                         GeglColor         *color);
GeglColor * gimp_color_dialog_get_color (GimpColorDialog   *dialog);
