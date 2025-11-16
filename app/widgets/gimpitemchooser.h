/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpitemchooser.h
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


#define GIMP_TYPE_ITEM_CHOOSER            (gimp_item_chooser_get_type ())
#define GIMP_ITEM_CHOOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_ITEM_CHOOSER, GimpItemChooser))
#define GIMP_ITEM_CHOOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_ITEM_CHOOSER, GimpItemChooserClass))
#define GIMP_IS_ITEM_CHOOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_ITEM_CHOOSER))
#define GIMP_IS_ITEM_CHOOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_ITEM_CHOOSER))
#define GIMP_ITEM_CHOOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_ITEM_CHOOSER, GimpItemChooserClass))


typedef struct _GimpItemChooserPrivate GimpItemChooserPrivate;
typedef struct _GimpItemChooserClass   GimpItemChooserClass;

struct _GimpItemChooser
{
  GtkFrame                    parent_instance;

  GimpItemChooserPrivate *priv;
};

struct _GimpItemChooserClass
{
  GtkFrameClass               parent_instance;

  /* Signals. */

  void     (* activate)      (GimpItemChooser *view,
                              GimpItem        *item);
};


GType          gimp_item_chooser_get_type     (void) G_GNUC_CONST;

GtkWidget    * gimp_item_chooser_new          (GimpContext     *context,
                                               GType            item_type,
                                               gint             view_size,
                                               gint             view_border_width);

GimpItem *     gimp_item_chooser_get_item     (GimpItemChooser *chooser);
void           gimp_item_chooser_set_item     (GimpItemChooser *chooser,
                                               GimpItem        *item);
