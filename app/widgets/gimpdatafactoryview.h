/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadatafactoryview.h
 * Copyright (C) 2001 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DATA_FACTORY_VIEW_H__
#define __LIGMA_DATA_FACTORY_VIEW_H__


#include "ligmacontainereditor.h"


#define LIGMA_TYPE_DATA_FACTORY_VIEW            (ligma_data_factory_view_get_type ())
#define LIGMA_DATA_FACTORY_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DATA_FACTORY_VIEW, LigmaDataFactoryView))
#define LIGMA_DATA_FACTORY_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DATA_FACTORY_VIEW, LigmaDataFactoryViewClass))
#define LIGMA_IS_DATA_FACTORY_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DATA_FACTORY_VIEW))
#define LIGMA_IS_DATA_FACTORY_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DATA_FACTORY_VIEW))
#define LIGMA_DATA_FACTORY_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DATA_FACTORY_VIEW, LigmaDataFactoryViewClass))


typedef struct _LigmaDataFactoryViewClass   LigmaDataFactoryViewClass;
typedef struct _LigmaDataFactoryViewPrivate LigmaDataFactoryViewPrivate;

struct _LigmaDataFactoryView
{
  LigmaContainerEditor         parent_instance;

  LigmaDataFactoryViewPrivate *priv;
};

struct _LigmaDataFactoryViewClass
{
  LigmaContainerEditorClass  parent_class;
};


GType             ligma_data_factory_view_get_type             (void) G_GNUC_CONST;

GtkWidget *       ligma_data_factory_view_new                  (LigmaViewType      view_type,
                                                               LigmaDataFactory  *factory,
                                                               LigmaContext      *context,
                                                               gint              view_size,
                                                               gint              view_border_width,
                                                               LigmaMenuFactory  *menu_factory,
                                                               const gchar      *menu_identifier,
                                                               const gchar      *ui_path,
                                                               const gchar      *action_group);

GtkWidget       * ligma_data_factory_view_get_edit_button      (LigmaDataFactoryView *factory_view);
GtkWidget       * ligma_data_factory_view_get_duplicate_button (LigmaDataFactoryView *factory_view);
LigmaDataFactory * ligma_data_factory_view_get_data_factory     (LigmaDataFactoryView *factory_view);
GType             ligma_data_factory_view_get_children_type    (LigmaDataFactoryView *factory_view);
gboolean          ligma_data_factory_view_has_data_new_func    (LigmaDataFactoryView *factory_view);
gboolean          ligma_data_factory_view_have                 (LigmaDataFactoryView *factory_view,
                                                               LigmaObject          *object);


#endif  /*  __LIGMA_DATA_FACTORY_VIEW_H__  */
