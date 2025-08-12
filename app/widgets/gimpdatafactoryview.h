/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdatafactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "gimpcontainereditor.h"


#define GIMP_TYPE_DATA_FACTORY_VIEW            (gimp_data_factory_view_get_type ())
#define GIMP_DATA_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_DATA_FACTORY_VIEW, GimpDataFactoryView))
#define GIMP_DATA_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_DATA_FACTORY_VIEW, GimpDataFactoryViewClass))
#define GIMP_IS_DATA_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_DATA_FACTORY_VIEW))
#define GIMP_IS_DATA_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_DATA_FACTORY_VIEW))
#define GIMP_DATA_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_DATA_FACTORY_VIEW, GimpDataFactoryViewClass))


typedef struct _GimpDataFactoryViewClass   GimpDataFactoryViewClass;
typedef struct _GimpDataFactoryViewPrivate GimpDataFactoryViewPrivate;

struct _GimpDataFactoryView
{
  GimpContainerEditor         parent_instance;

  GimpDataFactoryViewPrivate *priv;
};

struct _GimpDataFactoryViewClass
{
  GimpContainerEditorClass  parent_class;
};


GType             gimp_data_factory_view_get_type                 (void) G_GNUC_CONST;

GtkWidget *       gimp_data_factory_view_new                      (GimpViewType         view_type,
                                                                   GimpDataFactory     *factory,
                                                                   GimpContext         *context,
                                                                   gint                 view_size,
                                                                   gint                 view_border_width,
                                                                   GimpMenuFactory     *menu_factory,
                                                                   const gchar         *menu_identifier,
                                                                   const gchar         *ui_path,
                                                                   const gchar         *action_group);


void              gimp_data_factory_view_show_follow_theme_toggle (GimpDataFactoryView *view,
                                                                   gboolean             show);

GtkWidget       * gimp_data_factory_view_get_edit_button          (GimpDataFactoryView *factory_view);
GtkWidget       * gimp_data_factory_view_get_duplicate_button     (GimpDataFactoryView *factory_view);
GimpDataFactory * gimp_data_factory_view_get_data_factory         (GimpDataFactoryView *factory_view);
GType             gimp_data_factory_view_get_child_type           (GimpDataFactoryView *factory_view);
gboolean          gimp_data_factory_view_has_data_new_func        (GimpDataFactoryView *factory_view);
gboolean          gimp_data_factory_view_have                     (GimpDataFactoryView *factory_view,
                                                                   GimpObject          *object);
