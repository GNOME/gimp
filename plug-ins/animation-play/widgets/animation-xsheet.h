/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * animation-xsheet.h
 * Copyright (C) 2015-2016 Jehan <jehan@gimp.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __ANIMATION_XSHEET_H__
#define __ANIMATION_XSHEET_H__

#define ANIMATION_TYPE_XSHEET            (animation_xsheet_get_type ())
#define ANIMATION_XSHEET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_XSHEET, AnimationXSheet))
#define ANIMATION_XSHEET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_XSHEET, AnimationXSheetClass))
#define ANIMATION_IS_XSHEET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_XSHEET))
#define ANIMATION_IS_XSHEET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_XSHEET))
#define ANIMATION_XSHEET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_XSHEET, AnimationXSheetClass))

typedef struct _AnimationXSheet        AnimationXSheet;
typedef struct _AnimationXSheetClass   AnimationXSheetClass;
typedef struct _AnimationXSheetPrivate AnimationXSheetPrivate;

struct _AnimationXSheet
{
  GtkScrolledWindow       parent_instance;

  AnimationXSheetPrivate *priv;
};

struct _AnimationXSheetClass
{
  GtkScrolledWindowClass  parent_class;
};

GType       animation_xsheet_get_type (void) G_GNUC_CONST;

GtkWidget * animation_xsheet_new      (AnimationCelAnimation *animation,
                                       AnimationPlayback     *playback,
                                       GtkWidget             *layer_view,
                                       GtkWidget             *keyframe_view);

#endif  /*  __ANIMATION_XSHEET_H__  */
