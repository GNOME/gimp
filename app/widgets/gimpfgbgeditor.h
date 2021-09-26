/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfgbgeditor.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_FG_BG_EDITOR_H__
#define __GIMP_FG_BG_EDITOR_H__


typedef enum
{
  GIMP_FG_BG_TARGET_INVALID,
  GIMP_FG_BG_TARGET_FOREGROUND,
  GIMP_FG_BG_TARGET_BACKGROUND,
  GIMP_FG_BG_TARGET_SWAP,
  GIMP_FG_BG_TARGET_DEFAULT
} GimpFgBgTarget;


#define GIMP_TYPE_FG_BG_EDITOR            (gimp_fg_bg_editor_get_type ())
#define GIMP_FG_BG_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_FG_BG_EDITOR, GimpFgBgEditor))
#define GIMP_FG_BG_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_FG_BG_EDITOR, GimpFgBgEditorClass))
#define GIMP_IS_FG_BG_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_FG_BG_EDITOR))
#define GIMP_IS_FG_BG_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_FG_BG_EDITOR))
#define GIMP_FG_BG_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_FG_BG_EDITOR, GimpFgBgEditorClass))


typedef struct _GimpFgBgEditorClass GimpFgBgEditorClass;

struct _GimpFgBgEditor
{
  GtkEventBox         parent_instance;

  GimpContext        *context;
  GimpColorConfig    *color_config;
  GimpColorTransform *transform;

  GimpActiveColor     active_color;

  GimpImage          *active_image;

  GdkPixbuf          *default_icon;
  GdkPixbuf          *swap_icon;

  gint                rect_width;
  gint                rect_height;
  gint                click_target;
};

struct _GimpFgBgEditorClass
{
  GtkEventBoxClass    parent_class;

  /*  signals  */

  void (* color_clicked)  (GimpFgBgEditor  *editor,
                           GimpActiveColor  color);
  void (* color_dropped)  (GimpFgBgEditor  *editor,
                           GimpActiveColor  color);
  void (* colors_swapped) (GimpFgBgEditor  *editor);
  void (* colors_default) (GimpFgBgEditor  *editor);

  void (* tooltip)        (GimpFgBgEditor *editor,
                           GimpFgBgTarget  target,
                           GtkTooltip      tooltip);
};


GType       gimp_fg_bg_editor_get_type    (void) G_GNUC_CONST;

GtkWidget * gimp_fg_bg_editor_new         (GimpContext     *context);

void        gimp_fg_bg_editor_set_context (GimpFgBgEditor  *editor,
                                           GimpContext     *context);
void        gimp_fg_bg_editor_set_active  (GimpFgBgEditor  *editor,
                                           GimpActiveColor  active);


#endif  /*  __GIMP_FG_BG_EDITOR_H__  */
