/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdataeditor.c
 * Copyright (C) 2001 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "config/gimpconfig-path.h"

#include "gimpdataeditor.h"
#include "gimpdocked.h"
#include "gimpitemfactory.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo.h"

#include "gimp-intl.h"


static void       gimp_data_editor_class_init (GimpDataEditorClass *klass);
static void       gimp_data_editor_init       (GimpDataEditor      *view);

static void       gimp_data_editor_docked_iface_init (GimpDockedInterface *docked_iface);
static void       gimp_data_editor_set_aux_info      (GimpDocked     *docked,
                                                      GList          *aux_info);
static GList    * gimp_data_editor_get_aux_info      (GimpDocked     *docked);

static void       gimp_data_editor_dispose           (GObject        *object);

static void       gimp_data_editor_real_set_data     (GimpDataEditor *editor,
                                                      GimpData       *data);

static void       gimp_data_editor_name_activate     (GtkWidget      *widget,
                                                      GimpDataEditor *editor);
static gboolean   gimp_data_editor_name_focus_out    (GtkWidget      *widget,
                                                      GdkEvent       *event,
                                                      GimpDataEditor *editor);

static void       gimp_data_editor_data_name_changed (GimpObject     *object,
                                                      GimpDataEditor *editor);

static void       gimp_data_editor_save_clicked      (GtkWidget      *widget,
                                                      GimpDataEditor *editor);
static void       gimp_data_editor_revert_clicked    (GtkWidget      *widget,
                                                      GimpDataEditor *editor);


static GimpEditorClass *parent_class = NULL;


GType
gimp_data_editor_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo view_info =
      {
        sizeof (GimpDataEditorClass),
        NULL,           /* base_init */
        NULL,           /* base_finalize */
        (GClassInitFunc) gimp_data_editor_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GimpDataEditor),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gimp_data_editor_init,
      };
      static const GInterfaceInfo docked_iface_info =
      {
        (GInterfaceInitFunc) gimp_data_editor_docked_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (GIMP_TYPE_EDITOR,
                                     "GimpDataEditor",
                                     &view_info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_DOCKED,
                                   &docked_iface_info);
    }

  return type;
}

static void
gimp_data_editor_class_init (GimpDataEditorClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose  = gimp_data_editor_dispose;

  klass->set_data        = gimp_data_editor_real_set_data;
}

static void
gimp_data_editor_init (GimpDataEditor *editor)
{
  editor->data_factory  = NULL;
  editor->data          = NULL;
  editor->data_editable = FALSE;

  editor->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (editor), editor->name_entry,
		      FALSE, FALSE, 0);
  gtk_widget_show (editor->name_entry);

  g_signal_connect (editor->name_entry, "activate",
		    G_CALLBACK (gimp_data_editor_name_activate),
                    editor);
  g_signal_connect (editor->name_entry, "focus_out_event",
		    G_CALLBACK (gimp_data_editor_name_focus_out),
                    editor);

  editor->save_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_SAVE,
                            _("Save"), NULL,
                            G_CALLBACK (gimp_data_editor_save_clicked),
                            NULL,
                            editor);

  editor->revert_button =
    gimp_editor_add_button (GIMP_EDITOR (editor),
                            GTK_STOCK_REVERT_TO_SAVED,
                            _("Revert"), NULL,
                            G_CALLBACK (gimp_data_editor_revert_clicked),
                            NULL,
                            editor);
}

static void
gimp_data_editor_docked_iface_init (GimpDockedInterface *docked_iface)
{
  docked_iface->set_aux_info = gimp_data_editor_set_aux_info;
  docked_iface->get_aux_info = gimp_data_editor_get_aux_info;
}

#define AUX_INFO_CURRENT_DATA "current-data"

static void
gimp_data_editor_set_aux_info (GimpDocked *docked,
                               GList      *aux_info)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (docked);
  GList          *list;

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_CURRENT_DATA))
        {
          GimpData *data;

          data = (GimpData *)
            gimp_container_get_child_by_name (editor->data_factory->container,
                                              aux->value);

          if (data)
            gimp_data_editor_set_data (editor, data);
        }
    }
}

static GList *
gimp_data_editor_get_aux_info (GimpDocked *docked)
{
  GimpDataEditor *editor   = GIMP_DATA_EDITOR (docked);
  GList          *aux_info = NULL;

  if (editor->data)
    {
      GimpSessionInfoAux *aux;
      const gchar        *value;

      value = gimp_object_get_name (GIMP_OBJECT (editor->data));

      aux = gimp_session_info_aux_new (AUX_INFO_CURRENT_DATA, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static void
gimp_data_editor_dispose (GObject *object)
{
  GimpDataEditor *editor;

  editor = GIMP_DATA_EDITOR (object);

  if (editor->data)
    gimp_data_editor_set_data (editor, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_data_editor_real_set_data (GimpDataEditor *editor,
                                GimpData       *data)
{
  if (editor->data)
    {
      g_signal_handlers_disconnect_by_func (editor->data,
                                            gimp_data_editor_data_name_changed,
                                            editor);

      g_object_unref (editor->data);
    }

  editor->data = data;

  if (editor->data)
    {
      g_object_ref (editor->data);

      g_signal_connect (editor->data, "name_changed",
                        G_CALLBACK (gimp_data_editor_data_name_changed),
                        editor);

      gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
                          gimp_object_get_name (GIMP_OBJECT (editor->data)));
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (editor->name_entry), "");
    }

  editor->data_editable = (editor->data            &&
                           editor->data->writeable &&
                           ! editor->data->internal);

  gtk_widget_set_sensitive (editor->name_entry, editor->data_editable);
}

gboolean
gimp_data_editor_construct (GimpDataEditor  *editor,
                            GimpDataFactory *data_factory,
                            GimpMenuFactory *menu_factory,
                            const gchar     *menu_identifier)
{
  GimpData *data;

  g_return_val_if_fail (GIMP_IS_DATA_EDITOR (editor), FALSE);
  g_return_val_if_fail (GIMP_IS_DATA_FACTORY (data_factory), FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        menu_identifier != NULL, FALSE);

  editor->data_factory = data_factory;

  if (menu_factory && menu_identifier)
    gimp_editor_create_menu (GIMP_EDITOR (editor),
                             menu_factory, menu_identifier, editor);

  data = (GimpData *)
    gimp_context_get_by_type (gimp_get_user_context (data_factory->gimp),
                              data_factory->container->children_type);

  gimp_data_editor_set_data (editor, data);

  return TRUE;
}

void
gimp_data_editor_set_data (GimpDataEditor *editor,
                           GimpData       *data)
{
  g_return_if_fail (GIMP_IS_DATA_EDITOR (editor));
  g_return_if_fail (data == NULL || GIMP_IS_DATA (data));
  g_return_if_fail (data == NULL ||
                    g_type_is_a (G_TYPE_FROM_INSTANCE (data),
                                 editor->data_factory->container->children_type));

  if (editor->data != data)
    GIMP_DATA_EDITOR_GET_CLASS (editor)->set_data (editor, data);
}

GimpData *
gimp_data_editor_get_data (GimpDataEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_DATA_EDITOR (editor), NULL);

  return editor->data;
}


/*  private functions  */

static void
gimp_data_editor_name_activate (GtkWidget      *widget,
                                GimpDataEditor *editor)
{
  if (editor->data)
    {
      const gchar *entry_text;

      entry_text = gtk_entry_get_text (GTK_ENTRY (widget));

      gimp_object_set_name (GIMP_OBJECT (editor->data), entry_text);
    }
}

static gboolean
gimp_data_editor_name_focus_out (GtkWidget      *widget,
                                 GdkEvent       *event,
                                 GimpDataEditor *editor)
{
  gimp_data_editor_name_activate (widget, editor);

  return FALSE;
}

static void
gimp_data_editor_data_name_changed (GimpObject     *object,
                                    GimpDataEditor *editor)
{
  gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
		      gimp_object_get_name (object));
}

static void
gimp_data_editor_save_clicked (GtkWidget      *widget,
                               GimpDataEditor *editor)
{
  gchar *path = NULL;
  GimpData *data;

  g_object_get (editor->data_factory->gimp->config,
                editor->data_factory->path_property_name, &path,
                NULL);

  if (path && strlen (path))
    {
      gchar    *tmp;

      tmp = gimp_config_path_expand (path, TRUE, NULL);
      g_free (path);
      path = tmp;

      data = editor->data;

      if (! data->filename)
        gimp_data_create_filename (data, GIMP_OBJECT (data)->name, path);

      if (data->dirty)
        {
          GError *error = NULL;

          if (! gimp_data_save (data, &error))
            {
              /*  check if there actually was an error (no error
               *  means the data class does not implement save)
               */
              if (error)
                {
                  g_message (_("Warning: Failed to save data:\n%s"),
                             error->message);
                  g_clear_error (&error);
                }
            }
        }
    }
  g_free (path);
}

static void
gimp_data_editor_revert_clicked (GtkWidget      *widget,
                                 GimpDataEditor *editor)
{
  g_print ("TODO: implement revert\n");
}
