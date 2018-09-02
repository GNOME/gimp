/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphighlightablebutton.h
 * Copyright (C) 2018 Ell
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

#ifndef __GIMP_HIGHLIGHTABLE_BUTTON_H__
#define __GIMP_HIGHLIGHTABLE_BUTTON_H__


#define GIMP_TYPE_HIGHLIGHTABLE_BUTTON            (gimp_highlightable_button_get_type ())
#define GIMP_HIGHLIGHTABLE_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_HIGHLIGHTABLE_BUTTON, GimpHighlightableButton))
#define GIMP_HIGHLIGHTABLE_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_HIGHLIGHTABLE_BUTTON, GimpHighlightableButtonClass))
#define GIMP_IS_HIGHLIGHTABLE_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_HIGHLIGHTABLE_BUTTON))
#define GIMP_IS_HIGHLIGHTABLE_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_HIGHLIGHTABLE_BUTTON))
#define GIMP_HIGHLIGHTABLE_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_HIGHLIGHTABLE_BUTTON, GimpHighlightableButtonClass))


#define GIMP_HIGHLIGHTABLE_BUTTON_COLOR_AFFIRMATIVE (&(const GimpRGB) {0.20, 0.70, 0.20, 0.65})
#define GIMP_HIGHLIGHTABLE_BUTTON_COLOR_NEGATIVE    (&(const GimpRGB) {0.80, 0.20, 0.20, 0.65})


typedef struct _GimpHighlightableButtonPrivate GimpHighlightableButtonPrivate;
typedef struct _GimpHighlightableButtonClass   GimpHighlightableButtonClass;

struct _GimpHighlightableButton
{
  GimpButton                      parent_instance;

  GimpHighlightableButtonPrivate *priv;
};

struct _GimpHighlightableButtonClass
{
  GimpButtonClass  parent_class;
};


GType       gimp_highlightable_button_get_type            (void) G_GNUC_CONST;

GtkWidget * gimp_highlightable_button_new                 (void);

void        gimp_highlightable_button_set_highlight       (GimpHighlightableButton *button,
                                                           gboolean                 highlight);
gboolean    gimp_highlightable_button_get_highlight       (GimpHighlightableButton *button);

void        gimp_highlightable_button_set_highlight_color (GimpHighlightableButton *button,
                                                           const GimpRGB           *color);
void        gimp_highlightable_button_get_highlight_color (GimpHighlightableButton *button,
                                                           GimpRGB                 *color);


#endif /* __GIMP_HIGHLIGHTABLE_BUTTON_H__ */
