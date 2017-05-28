/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Editable label base class
 * Copyright (C) 2015-2017 Jehan <jehan@gimp.org>
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


#ifndef __ANIMATION_EDITABLE_LABEL_H__
#define __ANIMATION_EDITABLE_LABEL_H__

#include <gtk/gtk.h>

#define ANIMATION_TYPE_EDITABLE_LABEL             (animation_editable_label_get_type ())
#define ANIMATION_EDITABLE_LABEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_EDITABLE_LABEL, AnimationEditableLabel))
#define ANIMATION_IS_EDITABLE_LABEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_EDITABLE_LABEL))
#define ANIMATION_EDITABLE_LABEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_EDITABLE_LABEL, AnimationEditableLabelClass))
#define ANIMATION_IS_EDITABLE_LABEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_EDITABLE_LABEL))
#define ANIMATION_EDITABLE_LABEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_EDITABLE_LABEL, AnimationEditableLabelClass))

typedef struct _AnimationEditableLabel        AnimationEditableLabel;
typedef struct _AnimationEditableLabelClass   AnimationEditableLabelClass;
typedef struct _AnimationEditableLabelPrivate AnimationEditableLabelPrivate;

struct _AnimationEditableLabel
{
  GtkFrame                       parent_instance;

  AnimationEditableLabelPrivate *priv;

  /* Protected variable to be used by the child classes. */
  GtkWidget                     *editing_widget;
};

struct _AnimationEditableLabelClass
{
  GtkFrameClass                  parent_class;
};

GType animation_editable_label_get_type (void);

void          animation_editable_label_set_editing (AnimationEditableLabel *label,
                                                    gboolean                editing);
void          animation_editable_label_show_icon   (AnimationEditableLabel *label,
                                                    gboolean                show_icon);

const gchar * animation_editable_label_get_text    (AnimationEditableLabel *label);

GtkWidget   * animation_editable_label_get_label   (AnimationEditableLabel *label);

#endif /* __ANIMATION_EDITABLE_LABEL_H__ */
