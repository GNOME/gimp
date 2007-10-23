/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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
#include "core/gimpcontext.h"

#include "pdb/gimppdb.h"

#include "gimpmenufactory.h"
#include "gimppdbdialog.h"
#include "gimpwidgets-utils.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PDB,
  PROP_CONTEXT,
  PROP_SELECT_TYPE,
  PROP_INITIAL_OBJECT,
  PROP_CALLBACK_NAME,
  PROP_MENU_FACTORY
};


static void      gimp_pdb_dialog_class_init     (GimpPdbDialogClass *klass);
static void      gimp_pdb_dialog_init           (GimpPdbDialog      *dialog,
                                                 GimpPdbDialogClass *klass);

static GObject * gimp_pdb_dialog_constructor    (GType               type,
                                                 guint               n_params,
                                                 GObjectConstructParam *params);
static void      gimp_pdb_dialog_dispose        (GObject            *object);
static void      gimp_pdb_dialog_set_property   (GObject            *object,
                                                 guint               property_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);

static void      gimp_pdb_dialog_destroy        (GtkObject          *object);

static void      gimp_pdb_dialog_response       (GtkDialog          *dialog,
                                                 gint                response_id);

static void     gimp_pdb_dialog_context_changed (GimpContext        *context,
                                                 GimpObject         *object,
                                                 GimpPdbDialog      *dialog);
static void     gimp_pdb_dialog_plug_in_closed  (GimpPlugInManager  *manager,
                                                 GimpPlugIn         *plug_in,
                                                 GimpPdbDialog      *dialog);


static GimpDialogClass *parent_class = NULL;


GType
gimp_pdb_dialog_get_type (void)
{
  static GType dialog_type = 0;

  if (! dialog_type)
    {
      const GTypeInfo dialog_info =
      {
        sizeof (GimpPdbDialogClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_pdb_dialog_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPdbDialog),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_pdb_dialog_init,
      };

      dialog_type = g_type_register_static (GIMP_TYPE_DIALOG,
                                            "GimpPdbDialog",
                                            &dialog_info,
                                            G_TYPE_FLAG_ABSTRACT);
    }

  return dialog_type;
}

static void
gimp_pdb_dialog_class_init (GimpPdbDialogClass *klass)
{
  GObjectClass   *object_class     = G_OBJECT_CLASS (klass);
  GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class     = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructor  = gimp_pdb_dialog_constructor;
  object_class->dispose      = gimp_pdb_dialog_dispose;
  object_class->set_property = gimp_pdb_dialog_set_property;
  object_class->set_property = gimp_pdb_dialog_set_property;

  gtk_object_class->destroy  = gimp_pdb_dialog_destroy;

  dialog_class->response     = gimp_pdb_dialog_response;

  klass->run_callback        = NULL;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        GIMP_TYPE_PDB,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SELECT_TYPE,
                                   g_param_spec_gtype ("select-type",
                                                       NULL, NULL,
                                                       GIMP_TYPE_OBJECT,
                                                       GIMP_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_OBJECT,
                                   g_param_spec_object ("initial-object",
                                                        NULL, NULL,
                                                        GIMP_TYPE_OBJECT,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MENU_FACTORY,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_pdb_dialog_init (GimpPdbDialog      *dialog,
                      GimpPdbDialogClass *klass)
{
  klass->dialogs = g_list_prepend (klass->dialogs, dialog);

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
}

static GObject *
gimp_pdb_dialog_constructor (GType                  type,
                             guint                  n_params,
                             GObjectConstructParam *params)
{
  GObject       *object;
  GimpPdbDialog *dialog;
  const gchar   *signal_name;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  dialog = GIMP_PDB_DIALOG (object);

  g_assert (GIMP_IS_PDB (dialog->pdb));
  g_assert (GIMP_IS_CONTEXT (dialog->caller_context));
  g_assert (g_type_is_a (dialog->select_type, GIMP_TYPE_OBJECT));

  dialog->context = gimp_context_new (dialog->caller_context->gimp,
                                      g_type_name (type),
                                      NULL);

  gimp_context_set_by_type (dialog->context, dialog->select_type,
                            dialog->initial_object);

  dialog->initial_object = NULL;

  signal_name = gimp_context_type_to_signal_name (dialog->select_type);

  g_signal_connect_object (dialog->context, signal_name,
                           G_CALLBACK (gimp_pdb_dialog_context_changed),
                           dialog, 0);
  g_signal_connect_object (dialog->context->gimp->plug_in_manager,
                           "plug-in-closed",
                           G_CALLBACK (gimp_pdb_dialog_plug_in_closed),
                           dialog, 0);

  return object;
}

static void
gimp_pdb_dialog_dispose (GObject *object)
{
  GimpPdbDialogClass *klass = GIMP_PDB_DIALOG_GET_CLASS (object);

  klass->dialogs = g_list_remove (klass->dialogs, object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pdb_dialog_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (object);

  switch (property_id)
    {
    case PROP_PDB:
      dialog->pdb = GIMP_PDB (g_value_dup_object (value));
      break;

    case PROP_CONTEXT:
      dialog->caller_context = GIMP_CONTEXT (g_value_dup_object (value));
      break;

    case PROP_SELECT_TYPE:
      dialog->select_type = g_value_get_gtype (value);
      break;

    case PROP_INITIAL_OBJECT:
      /* don't ref, see constructor */
      dialog->initial_object = g_value_get_object (value);
      break;

    case PROP_CALLBACK_NAME:
      if (dialog->callback_name)
        g_free (dialog->callback_name);
      dialog->callback_name = g_value_dup_string (value);
      break;

    case PROP_MENU_FACTORY:
      dialog->menu_factory = (GimpMenuFactory *) g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pdb_dialog_destroy (GtkObject *object)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (object);

  if (dialog->pdb)
    {
      g_object_unref (dialog->pdb);
      dialog->pdb = NULL;
    }

  if (dialog->caller_context)
    {
      g_object_unref (dialog->caller_context);
      dialog->caller_context = NULL;
    }

  if (dialog->context)
    {
      g_object_unref (dialog->context);
      dialog->context = NULL;
    }

  if (dialog->callback_name)
    {
      g_free (dialog->callback_name);
      dialog->callback_name = NULL;
    }

  if (dialog->menu_factory)
    {
      g_object_unref (dialog->menu_factory);
      dialog->menu_factory = NULL;
    }

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
gimp_pdb_dialog_response (GtkDialog *gtk_dialog,
                          gint       response_id)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (gtk_dialog);

  gimp_pdb_dialog_run_callback (dialog, TRUE);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
gimp_pdb_dialog_run_callback (GimpPdbDialog *dialog,
                              gboolean       closing)
{
  GimpPdbDialogClass *klass = GIMP_PDB_DIALOG_GET_CLASS (dialog);
  GimpObject         *object;

  object = gimp_context_get_by_type (dialog->context, dialog->select_type);

  if (object                &&
      klass->run_callback   &&
      dialog->callback_name &&
      ! dialog->callback_busy)
    {
      dialog->callback_busy = TRUE;

      if (gimp_pdb_lookup_procedure (dialog->pdb, dialog->callback_name))
        {
          GValueArray *return_vals;

          return_vals = klass->run_callback (dialog, object, closing);

          if (g_value_get_enum (&return_vals->values[0]) != GIMP_PDB_SUCCESS)
            {
              gimp_message (dialog->context->gimp, G_OBJECT (dialog),
                            GIMP_MESSAGE_ERROR,
                            _("Unable to run %s callback. "
                              "The corresponding plug-in may have "
                              "crashed."),
                            g_type_name (G_TYPE_FROM_INSTANCE (dialog)));
            }

          g_value_array_free (return_vals);
        }

      dialog->callback_busy = FALSE;
    }
}

GimpPdbDialog *
gimp_pdb_dialog_get_by_callback (GimpPdbDialogClass *klass,
                                 const gchar        *callback_name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PDB_DIALOG_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->dialogs; list; list = g_list_next (list))
    {
      GimpPdbDialog *dialog = list->data;

      if (dialog->callback_name &&
          ! strcmp (callback_name, dialog->callback_name))
        return dialog;
    }

  return NULL;
}


/*  private functions  */

static void
gimp_pdb_dialog_context_changed (GimpContext   *context,
                                 GimpObject    *object,
                                 GimpPdbDialog *dialog)
{
  if (object)
    gimp_pdb_dialog_run_callback (dialog, FALSE);
}

static void
gimp_pdb_dialog_plug_in_closed (GimpPlugInManager *manager,
                                GimpPlugIn        *plug_in,
                                GimpPdbDialog     *dialog)
{
  if (dialog->caller_context && dialog->callback_name)
    {
      if (! gimp_pdb_lookup_procedure (dialog->pdb, dialog->callback_name))
        {
          gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }
    }
}
