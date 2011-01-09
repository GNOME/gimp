/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdataeditor.c
 * Copyright (C) 2002-2004 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"
#include "core/gimpdatafactory.h"

#include "gimpdataeditor.h"
#include "gimpdocked.h"
#include "gimpmenufactory.h"
#include "gimpsessioninfo-aux.h"
#include "gimpuimanager.h"

#include "gimp-intl.h"


#define DEFAULT_MINIMAL_HEIGHT 96


enum
{
  PROP_0,
  PROP_DATA_FACTORY,
  PROP_CONTEXT,
  PROP_DATA
};


static void       gimp_data_editor_docked_iface_init (GimpDockedInterface *iface);

static void       gimp_data_editor_constructed       (GObject        *object);
static void       gimp_data_editor_dispose           (GObject        *object);
static void       gimp_data_editor_set_property      (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void       gimp_data_editor_get_property      (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);

static void       gimp_data_editor_style_updated     (GtkWidget      *widget);

static void       gimp_data_editor_set_context       (GimpDocked     *docked,
                                                      GimpContext    *context);
static void       gimp_data_editor_set_aux_info      (GimpDocked     *docked,
                                                      GList          *aux_info);
static GList    * gimp_data_editor_get_aux_info      (GimpDocked     *docked);
static gchar    * gimp_data_editor_get_title         (GimpDocked     *docked);

static void       gimp_data_editor_real_set_data     (GimpDataEditor *editor,
                                                      GimpData       *data);

static void       gimp_data_editor_data_changed      (GimpContext    *context,
                                                      GimpData       *data,
                                                      GimpDataEditor *editor);
static gboolean   gimp_data_editor_name_key_press    (GtkWidget      *widget,
                                                      GdkEventKey    *kevent,
                                                      GimpDataEditor *editor);
static void       gimp_data_editor_name_activate     (GtkWidget      *widget,
                                                      GimpDataEditor *editor);
static gboolean   gimp_data_editor_name_focus_out    (GtkWidget      *widget,
                                                      GdkEvent       *event,
                                                      GimpDataEditor *editor);

static void       gimp_data_editor_data_name_changed (GimpObject     *object,
                                                      GimpDataEditor *editor);

static void       gimp_data_editor_save_dirty        (GimpDataEditor *editor);


G_DEFINE_TYPE_WITH_CODE (GimpDataEditor, gimp_data_editor, GIMP_TYPE_EDITOR,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_DOCKED,
                                                gimp_data_editor_docked_iface_init))

#define parent_class gimp_data_editor_parent_class

static GimpDockedInterface *parent_docked_iface = NULL;


static void
gimp_data_editor_class_init (GimpDataEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed   = gimp_data_editor_constructed;
  object_class->set_property  = gimp_data_editor_set_property;
  object_class->get_property  = gimp_data_editor_get_property;
  object_class->dispose       = gimp_data_editor_dispose;

  widget_class->style_updated = gimp_data_editor_style_updated;

  klass->set_data             = gimp_data_editor_real_set_data;

  g_object_class_install_property (object_class, PROP_DATA_FACTORY,
                                   g_param_spec_object ("data-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DATA_FACTORY,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_DATA,
                                   g_param_spec_object ("data",
                                                        NULL, NULL,
                                                        GIMP_TYPE_DATA,
                                                        GIMP_PARAM_READWRITE));

  gtk_widget_class_install_style_property (widget_class,
                                           g_param_spec_int ("minimal-height",
                                                             NULL, NULL,
                                                             32,
                                                             G_MAXINT,
                                                             DEFAULT_MINIMAL_HEIGHT,
                                                             GIMP_PARAM_READABLE));
}

static void
gimp_data_editor_docked_iface_init (GimpDockedInterface *iface)
{
  parent_docked_iface = g_type_interface_peek_parent (iface);

  if (! parent_docked_iface)
    parent_docked_iface = g_type_default_interface_peek (GIMP_TYPE_DOCKED);

  iface->set_context  = gimp_data_editor_set_context;
  iface->set_aux_info = gimp_data_editor_set_aux_info;
  iface->get_aux_info = gimp_data_editor_get_aux_info;
  iface->get_title    = gimp_data_editor_get_title;
}

static void
gimp_data_editor_init (GimpDataEditor *editor)
{
  editor->data_factory  = NULL;
  editor->context       = NULL;
  editor->data          = NULL;
  editor->data_editable = FALSE;

  editor->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (editor), editor->name_entry, FALSE, FALSE, 0);
  gtk_widget_show (editor->name_entry);

  gtk_editable_set_editable (GTK_EDITABLE (editor->name_entry), FALSE);

  g_signal_connect (editor->name_entry, "key-press-event",
                    G_CALLBACK (gimp_data_editor_name_key_press),
                    editor);
  g_signal_connect (editor->name_entry, "activate",
                    G_CALLBACK (gimp_data_editor_name_activate),
                    editor);
  g_signal_connect (editor->name_entry, "focus-out-event",
                    G_CALLBACK (gimp_data_editor_name_focus_out),
                    editor);
}

static void
gimp_data_editor_constructed (GObject *object)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_DATA_FACTORY (editor->data_factory));
  g_assert (GIMP_IS_CONTEXT (editor->context));

  gimp_data_editor_set_edit_active (editor, TRUE);
}

static void
gimp_data_editor_dispose (GObject *object)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (object);

  if (editor->data)
    {
      /* Save dirty data before we clear out */
      gimp_data_editor_save_dirty (editor);
      gimp_data_editor_set_data (editor, NULL);
    }

  if (editor->context)
    gimp_docked_set_context (GIMP_DOCKED (editor), NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_data_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      editor->data_factory = g_value_get_object (value);
      break;
    case PROP_CONTEXT:
      gimp_docked_set_context (GIMP_DOCKED (object),
                               g_value_get_object (value));
      break;
    case PROP_DATA:
      gimp_data_editor_set_data (editor, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (object);

  switch (property_id)
    {
    case PROP_DATA_FACTORY:
      g_value_set_object (value, editor->data_factory);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, editor->context);
      break;
    case PROP_DATA:
      g_value_set_object (value, editor->data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_data_editor_style_updated (GtkWidget *widget)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (widget);
  gint            minimal_height;

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
                        "minimal-height", &minimal_height,
                        NULL);

  if (editor->view)
    gtk_widget_set_size_request (editor->view, -1, minimal_height);
}

static void
gimp_data_editor_set_context (GimpDocked  *docked,
                              GimpContext *context)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (docked);

  if (context == editor->context)
    return;

  if (parent_docked_iface->set_context)
    parent_docked_iface->set_context (docked, context);

  if (editor->context)
    {
      g_signal_handlers_disconnect_by_func (editor->context,
                                            gimp_data_editor_data_changed,
                                            editor);

      g_object_unref (editor->context);
    }

  editor->context = context;

  if (editor->context)
    {
      GType     data_type;
      GimpData *data;

      g_object_ref (editor->context);

      data_type = gimp_data_factory_get_data_type (editor->data_factory);
      data = GIMP_DATA (gimp_context_get_by_type (editor->context, data_type));

      g_signal_connect (editor->context,
                        gimp_context_type_to_signal_name (data_type),
                        G_CALLBACK (gimp_data_editor_data_changed),
                        editor);

      gimp_data_editor_data_changed (editor->context, data, editor);
    }
}

#define AUX_INFO_EDIT_ACTIVE  "edit-active"
#define AUX_INFO_CURRENT_DATA "current-data"

static void
gimp_data_editor_set_aux_info (GimpDocked *docked,
                               GList      *aux_info)
{
  GimpDataEditor *editor = GIMP_DATA_EDITOR (docked);
  GList          *list;

  parent_docked_iface->set_aux_info (docked, aux_info);

  for (list = aux_info; list; list = g_list_next (list))
    {
      GimpSessionInfoAux *aux = list->data;

      if (! strcmp (aux->name, AUX_INFO_EDIT_ACTIVE))
        {
          gboolean edit_active;

          edit_active = ! g_ascii_strcasecmp (aux->value, "true");

          gimp_data_editor_set_edit_active (editor, edit_active);
        }
      else if (! strcmp (aux->name, AUX_INFO_CURRENT_DATA))
        {
          if (! editor->edit_active)
            {
              GimpData *data;

              data = (GimpData *)
                gimp_container_get_child_by_name (gimp_data_factory_get_container (editor->data_factory),
                                                  aux->value);

              if (data)
                gimp_data_editor_set_data (editor, data);
            }
        }
    }
}

static GList *
gimp_data_editor_get_aux_info (GimpDocked *docked)
{
  GimpDataEditor     *editor = GIMP_DATA_EDITOR (docked);
  GList              *aux_info;
  GimpSessionInfoAux *aux;

  aux_info = parent_docked_iface->get_aux_info (docked);

  aux = gimp_session_info_aux_new (AUX_INFO_EDIT_ACTIVE,
                                   editor->edit_active ? "true" : "false");
  aux_info = g_list_append (aux_info, aux);

  if (editor->data)
    {
      const gchar *value;

      value = gimp_object_get_name (editor->data);

      aux = gimp_session_info_aux_new (AUX_INFO_CURRENT_DATA, value);
      aux_info = g_list_append (aux_info, aux);
    }

  return aux_info;
}

static gchar *
gimp_data_editor_get_title (GimpDocked *docked)
{
  GimpDataEditor      *editor       = GIMP_DATA_EDITOR (docked);
  GimpDataEditorClass *editor_class = GIMP_DATA_EDITOR_GET_CLASS (editor);

  if (editor->data_editable)
    return g_strdup (editor_class->title);
  else
    return g_strdup_printf (_("%s (read only)"), editor_class->title);
}

static void
gimp_data_editor_real_set_data (GimpDataEditor *editor,
                                GimpData       *data)
{
  gboolean editable;

  if (editor->data)
    {
      gimp_data_editor_save_dirty (editor);

      g_signal_handlers_disconnect_by_func (editor->data,
                                            gimp_data_editor_data_name_changed,
                                            editor);

      g_object_unref (editor->data);
    }

  editor->data = data;

  if (editor->data)
    {
      g_object_ref (editor->data);

      g_signal_connect (editor->data, "name-changed",
                        G_CALLBACK (gimp_data_editor_data_name_changed),
                        editor);

      gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
                          gimp_object_get_name (editor->data));
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (editor->name_entry), "");
    }

  editable = (editor->data && gimp_data_is_writable (editor->data));

  if (editor->data_editable != editable)
    {
      editor->data_editable = editable;

      gtk_editable_set_editable (GTK_EDITABLE (editor->name_entry), editable);
      gimp_docked_title_changed (GIMP_DOCKED (editor));
    }
}

void
gimp_data_editor_set_data (GimpDataEditor *editor,
                           GimpData       *data)
{
  g_return_if_fail (GIMP_IS_DATA_EDITOR (editor));
  g_return_if_fail (data == NULL || GIMP_IS_DATA (data));
  g_return_if_fail (data == NULL ||
                    g_type_is_a (G_TYPE_FROM_INSTANCE (data),
                                 gimp_data_factory_get_data_type (editor->data_factory)));

  if (editor->data != data)
    {
      GIMP_DATA_EDITOR_GET_CLASS (editor)->set_data (editor, data);

      g_object_notify (G_OBJECT (editor), "data");

      if (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)))
        gimp_ui_manager_update (gimp_editor_get_ui_manager (GIMP_EDITOR (editor)),
                                gimp_editor_get_popup_data (GIMP_EDITOR (editor)));
    }
}

GimpData *
gimp_data_editor_get_data (GimpDataEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_DATA_EDITOR (editor), NULL);

  return editor->data;
}

void
gimp_data_editor_set_edit_active (GimpDataEditor *editor,
                                  gboolean        edit_active)
{
  g_return_if_fail (GIMP_IS_DATA_EDITOR (editor));

  if (editor->edit_active != edit_active)
    {
      editor->edit_active = edit_active;

      if (editor->edit_active && editor->context)
        {
          GType     data_type;
          GimpData *data;

          data_type = gimp_data_factory_get_data_type (editor->data_factory);
          data = GIMP_DATA (gimp_context_get_by_type (editor->context,
                                                      data_type));

          gimp_data_editor_set_data (editor, data);
        }
    }
}

gboolean
gimp_data_editor_get_edit_active (GimpDataEditor *editor)
{
  g_return_val_if_fail (GIMP_IS_DATA_EDITOR (editor), FALSE);

  return editor->edit_active;
}


/*  private functions  */

static void
gimp_data_editor_data_changed (GimpContext    *context,
                               GimpData       *data,
                               GimpDataEditor *editor)
{
  if (editor->edit_active)
    gimp_data_editor_set_data (editor, data);
}

static gboolean
gimp_data_editor_name_key_press (GtkWidget      *widget,
                                 GdkEventKey    *kevent,
                                 GimpDataEditor *editor)
{
  if (kevent->keyval == GDK_KEY_Escape)
    {
      gtk_entry_set_text (GTK_ENTRY (editor->name_entry),
                          gimp_object_get_name (editor->data));
      return TRUE;
    }

  return FALSE;
}

static void
gimp_data_editor_name_activate (GtkWidget      *widget,
                                GimpDataEditor *editor)
{
  if (editor->data)
    {
      gchar *new_name;

      new_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (widget)));
      new_name = g_strstrip (new_name);

      if (strlen (new_name) &&
          g_strcmp0 (new_name, gimp_object_get_name (editor->data)))
        {
          gimp_object_take_name (GIMP_OBJECT (editor->data), new_name);
        }
      else
        {
          gtk_entry_set_text (GTK_ENTRY (widget),
                              gimp_object_get_name (editor->data));
          g_free (new_name);
        }
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
gimp_data_editor_save_dirty (GimpDataEditor *editor)
{
  GimpData *data = editor->data;

  if (data                      &&
      gimp_data_is_dirty (data) &&
      gimp_data_is_writable (data))
    {
      GError *error = NULL;

      if (! gimp_data_factory_data_save_single (editor->data_factory, data,
                                                &error))
        {
          gimp_message_literal (gimp_data_factory_get_gimp (editor->data_factory),
                                G_OBJECT (editor),
                                GIMP_MESSAGE_ERROR,
                                error->message);
          g_clear_error (&error);
        }
    }
}
