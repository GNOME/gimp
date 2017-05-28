/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Editable label with entry
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


#ifndef __ANIMATION_EDITABLE_LABEL_STRING_H__
#define __ANIMATION_EDITABLE_LABEL_STRING_H__

#include <gtk/gtk.h>

#define ANIMATION_TYPE_EDITABLE_LABEL_STRING                  (animation_editable_label_string_get_type ())
#define ANIMATION_EDITABLE_LABEL_STRING(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), ANIMATION_TYPE_EDITABLE_LABEL_STRING, AnimationEditableLabelString))
#define ANIMATION_IS_EDITABLE_LABEL_STRING(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ANIMATION_TYPE_EDITABLE_LABEL_STRING))
#define ANIMATION_EDITABLE_LABEL_STRING_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), ANIMATION_TYPE_EDITABLE_LABEL_STRING, AnimationEditableLabelStringClass))
#define ANIMATION_IS_EDITABLE_LABEL_STRING_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), ANIMATION_TYPE_EDITABLE_LABEL_STRING))
#define ANIMATION_EDITABLE_LABEL_STRING_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), ANIMATION_TYPE_EDITABLE_LABEL_STRING, AnimationEditableLabelStringClass))

typedef struct _AnimationEditableLabelString        AnimationEditableLabelString;
typedef struct _AnimationEditableLabelStringClass   AnimationEditableLabelStringClass;
typedef struct _AnimationEditableLabelStringPrivate AnimationEditableLabelStringPrivate;

struct _AnimationEditableLabelString
{
  AnimationEditableLabel               parent_instance;

  AnimationEditableLabelStringPrivate *priv;
};

struct _AnimationEditableLabelStringClass
{
  AnimationEditableLabelClass parent_class;
};

GType animation_editable_label_string_get_type (void);

GtkWidget * animation_editable_label_string_new (const gchar *default_text);

#endif /* __ANIMATION_EDITABLE_LABEL_STRING_H__ */
