/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasettingseditor.h
 * Copyright (C) 2008-2011 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SETTINGS_EDITOR_H__
#define __LIGMA_SETTINGS_EDITOR_H__


#define LIGMA_TYPE_SETTINGS_EDITOR            (ligma_settings_editor_get_type ())
#define LIGMA_SETTINGS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_SETTINGS_EDITOR, LigmaSettingsEditor))
#define LIGMA_SETTINGS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_SETTINGS_EDITOR, LigmaSettingsEditorClass))
#define LIGMA_IS_SETTINGS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_SETTINGS_EDITOR))
#define LIGMA_IS_SETTINGS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_SETTINGS_EDITOR))
#define LIGMA_SETTINGS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_SETTINGS_EDITOR, LigmaSettingsEditorClass))


typedef struct _LigmaSettingsEditorClass LigmaSettingsEditorClass;

struct _LigmaSettingsEditor
{
  GtkBox  parent_instance;
};

struct _LigmaSettingsEditorClass
{
  GtkBoxClass  parent_class;
};


GType       ligma_settings_editor_get_type (void) G_GNUC_CONST;

GtkWidget * ligma_settings_editor_new      (Ligma          *ligma,
                                           GObject       *config,
                                           LigmaContainer *container);


#endif  /*  __LIGMA_SETTINGS_EDITOR_H__  */
