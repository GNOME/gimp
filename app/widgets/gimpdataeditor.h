/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadataeditor.h
 * Copyright (C) 2002-2004 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_DATA_EDITOR_H__
#define __LIGMA_DATA_EDITOR_H__


#include "ligmaeditor.h"


#define LIGMA_TYPE_DATA_EDITOR            (ligma_data_editor_get_type ())
#define LIGMA_DATA_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_DATA_EDITOR, LigmaDataEditor))
#define LIGMA_DATA_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_DATA_EDITOR, LigmaDataEditorClass))
#define LIGMA_IS_DATA_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_DATA_EDITOR))
#define LIGMA_IS_DATA_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_DATA_EDITOR))
#define LIGMA_DATA_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_DATA_EDITOR, LigmaDataEditorClass))


typedef struct _LigmaDataEditorClass LigmaDataEditorClass;

struct _LigmaDataEditor
{
  LigmaEditor       parent_instance;

  LigmaDataFactory *data_factory;
  LigmaContext     *context;
  gboolean         edit_active;

  LigmaData        *data;
  gboolean         data_editable;

  GtkWidget       *name_entry;

  GtkWidget       *view; /* filled by subclasses */
};

struct _LigmaDataEditorClass
{
  LigmaEditorClass  parent_class;

  /*  virtual functions  */
  void (* set_data) (LigmaDataEditor *editor,
                     LigmaData       *data);

  const gchar *title;
};


GType       ligma_data_editor_get_type        (void) G_GNUC_CONST;

void        ligma_data_editor_set_data        (LigmaDataEditor *editor,
                                              LigmaData       *data);
LigmaData  * ligma_data_editor_get_data        (LigmaDataEditor *editor);

void        ligma_data_editor_set_edit_active (LigmaDataEditor *editor,
                                              gboolean        edit_active);
gboolean    ligma_data_editor_get_edit_active (LigmaDataEditor *editor);


#endif  /*  __LIGMA_DATA_EDITOR_H__  */
