/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacolorhistory.h
 * Copyright (C) 2015 Jehan <jehan@girinstud.io>
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

#ifndef __LIGMA_COLOR_HISTORY_H__
#define __LIGMA_COLOR_HISTORY_H__


#define LIGMA_TYPE_COLOR_HISTORY            (ligma_color_history_get_type ())
#define LIGMA_COLOR_HISTORY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_HISTORY, LigmaColorHistory))
#define LIGMA_COLOR_HISTORY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_HISTORY, LigmaColorHistoryClass))
#define LIGMA_IS_COLOR_HISTORY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_HISTORY))
#define LIGMA_IS_COLOR_HISTORY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_HISTORY))
#define LIGMA_COLOR_HISTORY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_HISTORY, LigmaColorHistoryClass))


typedef struct _LigmaColorHistoryClass LigmaColorHistoryClass;

struct _LigmaColorHistory
{
  GtkGrid       parent_instance;

  LigmaContext  *context;
  LigmaImage    *active_image;

  GtkWidget   **color_areas;
  GtkWidget   **buttons;
  gint          history_size;
  gint          n_rows;
};

struct _LigmaColorHistoryClass
{
  GtkGridClass  parent_class;

  /*  signals  */
  void   (* color_selected) (LigmaColorHistory *history,
                             const LigmaRGB    *rgb);
};


GType       ligma_color_history_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_color_history_new      (LigmaContext     *context,
                                         gint             history_size);

#endif /* __LIGMA_COLOR_HISTORY_H__ */

