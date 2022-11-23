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

#ifndef __LIGMA_COLOR_PANEL_H__
#define __LIGMA_COLOR_PANEL_H__


#define LIGMA_TYPE_COLOR_PANEL            (ligma_color_panel_get_type ())
#define LIGMA_COLOR_PANEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_PANEL, LigmaColorPanel))
#define LIGMA_COLOR_PANEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_PANEL, LigmaColorPanelClass))
#define LIGMA_IS_COLOR_PANEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_PANEL))
#define LIGMA_IS_COLOR_PANEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_PANEL))
#define LIGMA_COLOR_PANEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_PANEL, LigmaColorPanelClass))


typedef struct _LigmaColorPanelClass LigmaColorPanelClass;

struct _LigmaColorPanel
{
  LigmaColorButton  parent_instance;

  LigmaContext     *context;
  GtkWidget       *color_dialog;
};

struct _LigmaColorPanelClass
{
  LigmaColorButtonClass  parent_class;

  /*  signals  */
  void (* response) (LigmaColorPanel       *panel,
                     LigmaColorDialogState  state);
};


GType       ligma_color_panel_get_type        (void) G_GNUC_CONST;

GtkWidget * ligma_color_panel_new             (const gchar          *title,
                                              const LigmaRGB        *color,
                                              LigmaColorAreaType     type,
                                              gint                  width,
                                              gint                  height);

void        ligma_color_panel_set_context     (LigmaColorPanel       *panel,
                                              LigmaContext          *context);

void        ligma_color_panel_dialog_response (LigmaColorPanel       *panel,
                                              LigmaColorDialogState  state);


#endif  /*  __LIGMA_COLOR_PANEL_H__  */
