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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdata.h"

#include "gimpdataeditor.h"
#include "gimpitemfactory.h"
#include "gimpmenufactory.h"

#include "gimp-intl.h"


static void       gimp_data_editor_class_init (GimpDataEditorClass *klass);
static void       gimp_data_editor_init       (GimpDataEditor      *view);

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
  static GType view_type = 0;

  if (! view_type)
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

      view_type = g_type_register_static (GIMP_TYPE_EDITOR,
                                          "GimpDataEditor",
                                          &view_info, 0);
    }

  return view_type;
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
  editor->data_type     = G_TYPE_NONE;
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
                            Gimp            *gimp,
                            GType            data_type,
                            GimpMenuFactory *menu_factory,
                            const gchar     *menu_identifier)
{
  GimpData *data;

  g_return_val_if_fail (GIMP_IS_DATA_EDITOR (editor), FALSE);
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (g_type_is_a (data_type, GIMP_TYPE_DATA), FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), FALSE);
  g_return_val_if_fail (menu_factory == NULL ||
                        menu_identifier != NULL, FALSE);

  editor->gimp         = gimp;
  editor->data_type    = data_type;

  if (menu_factory && menu_identifier)
    gimp_editor_create_menu (GIMP_EDITOR (editor),
                             menu_factory, menu_identifier, editor);

  data = (GimpData *)
    gimp_context_get_by_type (gimp_get_user_context (gimp), data_type);

  gimp_data_editor_set_data (editor, data);

  return TRUE;
}

void
gimp_data_editor_set_data (GimpDataEditor *editor,
                           GimpData       *data)
{
  g_return_if_fail (GIMP_IS_DATA_EDITOR (editor));
  g_return_if_fail (data == NULL || GIMP_IS_DATA (data));
  g_return_if_fail (data == NULL || g_type_is_a (G_TYPE_FROM_INSTANCE (data),
                                                 editor->data_type));

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
  g_print ("TODO: implement save\n");
}

static void
gimp_data_editor_revert_clicked (GtkWidget      *widget,
                                 GimpDataEditor *editor)
{
  g_print ("TODO: implement revert\n");
}
