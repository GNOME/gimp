/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsettingseditor.h
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_SETTINGS_EDITOR_H__
#define __GIMP_SETTINGS_EDITOR_H__


#define GIMP_TYPE_SETTINGS_EDITOR            (gimp_settings_editor_get_type ())
#define GIMP_SETTINGS_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SETTINGS_EDITOR, GimpSettingsEditor))
#define GIMP_SETTINGS_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SETTINGS_EDITOR, GimpSettingsEditorClass))
#define GIMP_IS_SETTINGS_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_SETTINGS_EDITOR))
#define GIMP_IS_SETTINGS_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SETTINGS_EDITOR))
#define GIMP_SETTINGS_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SETTINGS_EDITOR, GimpSettingsEditorClass))


typedef struct _GimpSettingsEditorClass GimpSettingsEditorClass;

struct _GimpSettingsEditor
{
  GtkVBox        parent_instance;

  Gimp          *gimp;
  GObject       *config;
  GimpContainer *container;
  GObject       *selected_setting;

  GtkWidget     *view;
  GtkWidget     *import_button;
  GtkWidget     *export_button;
  GtkWidget     *delete_button;
};

struct _GimpSettingsEditorClass
{
  GtkVBoxClass  parent_class;
};


GType       gimp_settings_editor_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_settings_editor_new      (Gimp          *gimp,
                                           GObject       *config,
                                           GimpContainer *container);


#endif  /*  __GIMP_SETTINGS_EDITOR_H__  */
