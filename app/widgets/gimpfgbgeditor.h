/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmafgbgeditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_FG_BG_EDITOR_H__
#define __LIGMA_FG_BG_EDITOR_H__


typedef enum
{
  LIGMA_FG_BG_TARGET_INVALID,
  LIGMA_FG_BG_TARGET_FOREGROUND,
  LIGMA_FG_BG_TARGET_BACKGROUND,
  LIGMA_FG_BG_TARGET_SWAP,
  LIGMA_FG_BG_TARGET_DEFAULT
} LigmaFgBgTarget;


#define LIGMA_TYPE_FG_BG_EDITOR            (ligma_fg_bg_editor_get_type ())
#define LIGMA_FG_BG_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_FG_BG_EDITOR, LigmaFgBgEditor))
#define LIGMA_FG_BG_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_FG_BG_EDITOR, LigmaFgBgEditorClass))
#define LIGMA_IS_FG_BG_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_FG_BG_EDITOR))
#define LIGMA_IS_FG_BG_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_FG_BG_EDITOR))
#define LIGMA_FG_BG_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_FG_BG_EDITOR, LigmaFgBgEditorClass))


typedef struct _LigmaFgBgEditorClass LigmaFgBgEditorClass;

struct _LigmaFgBgEditor
{
  GtkEventBox         parent_instance;

  LigmaContext        *context;
  LigmaColorConfig    *color_config;
  LigmaColorTransform *transform;

  LigmaActiveColor     active_color;

  LigmaImage          *active_image;

  GdkPixbuf          *default_icon;
  GdkPixbuf          *swap_icon;

  gint                rect_width;
  gint                rect_height;
  gint                click_target;
};

struct _LigmaFgBgEditorClass
{
  GtkEventBoxClass    parent_class;

  /*  signals  */

  void (* color_clicked)  (LigmaFgBgEditor  *editor,
                           LigmaActiveColor  color);
  void (* color_dropped)  (LigmaFgBgEditor  *editor,
                           LigmaActiveColor  color);
  void (* colors_swapped) (LigmaFgBgEditor  *editor);
  void (* colors_default) (LigmaFgBgEditor  *editor);

  void (* tooltip)        (LigmaFgBgEditor *editor,
                           LigmaFgBgTarget  target,
                           GtkTooltip      tooltip);
};


GType       ligma_fg_bg_editor_get_type    (void) G_GNUC_CONST;

GtkWidget * ligma_fg_bg_editor_new         (LigmaContext     *context);

void        ligma_fg_bg_editor_set_context (LigmaFgBgEditor  *editor,
                                           LigmaContext     *context);
void        ligma_fg_bg_editor_set_active  (LigmaFgBgEditor  *editor,
                                           LigmaActiveColor  active);


#endif  /*  __LIGMA_FG_BG_EDITOR_H__  */
