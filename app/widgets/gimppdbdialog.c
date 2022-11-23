/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@ligma.org>
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

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "pdb/ligmapdb.h"

#include "ligmamenufactory.h"
#include "ligmapdbdialog.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


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


static void   ligma_pdb_dialog_constructed     (GObject            *object);
static void   ligma_pdb_dialog_dispose         (GObject            *object);
static void   ligma_pdb_dialog_set_property    (GObject            *object,
                                               guint               property_id,
                                               const GValue       *value,
                                               GParamSpec         *pspec);

static void   ligma_pdb_dialog_response        (GtkDialog          *dialog,
                                               gint                response_id);

static void   ligma_pdb_dialog_context_changed (LigmaContext        *context,
                                               LigmaObject         *object,
                                               LigmaPdbDialog      *dialog);
static void   ligma_pdb_dialog_plug_in_closed  (LigmaPlugInManager  *manager,
                                               LigmaPlugIn         *plug_in,
                                               LigmaPdbDialog      *dialog);


G_DEFINE_ABSTRACT_TYPE (LigmaPdbDialog, ligma_pdb_dialog, LIGMA_TYPE_DIALOG)

#define parent_class ligma_pdb_dialog_parent_class


static void
ligma_pdb_dialog_class_init (LigmaPdbDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = ligma_pdb_dialog_constructed;
  object_class->dispose      = ligma_pdb_dialog_dispose;
  object_class->set_property = ligma_pdb_dialog_set_property;

  dialog_class->response     = ligma_pdb_dialog_response;

  klass->run_callback        = NULL;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        LIGMA_TYPE_PDB,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_SELECT_TYPE,
                                   g_param_spec_gtype ("select-type",
                                                       NULL, NULL,
                                                       LIGMA_TYPE_OBJECT,
                                                       LIGMA_PARAM_WRITABLE |
                                                       G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_INITIAL_OBJECT,
                                   g_param_spec_object ("initial-object",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_OBJECT,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_MENU_FACTORY,
                                   g_param_spec_object ("menu-factory",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_MENU_FACTORY,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_pdb_dialog_init (LigmaPdbDialog *dialog)
{
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_Close"), GTK_RESPONSE_CLOSE);
}

static void
ligma_pdb_dialog_constructed (GObject *object)
{
  LigmaPdbDialog      *dialog = LIGMA_PDB_DIALOG (object);
  LigmaPdbDialogClass *klass  = LIGMA_PDB_DIALOG_GET_CLASS (object);
  const gchar        *signal_name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  klass->dialogs = g_list_prepend (klass->dialogs, dialog);

  ligma_assert (LIGMA_IS_PDB (dialog->pdb));
  ligma_assert (LIGMA_IS_CONTEXT (dialog->caller_context));
  ligma_assert (g_type_is_a (dialog->select_type, LIGMA_TYPE_OBJECT));

  dialog->context = ligma_context_new (dialog->caller_context->ligma,
                                      G_OBJECT_TYPE_NAME (object),
                                      NULL);

  ligma_context_set_by_type (dialog->context, dialog->select_type,
                            dialog->initial_object);

  dialog->initial_object = NULL;

  signal_name = ligma_context_type_to_signal_name (dialog->select_type);

  g_signal_connect_object (dialog->context, signal_name,
                           G_CALLBACK (ligma_pdb_dialog_context_changed),
                           dialog, 0);
  g_signal_connect_object (dialog->context->ligma->plug_in_manager,
                           "plug-in-closed",
                           G_CALLBACK (ligma_pdb_dialog_plug_in_closed),
                           dialog, 0);
}

static void
ligma_pdb_dialog_dispose (GObject *object)
{
  LigmaPdbDialog      *dialog = LIGMA_PDB_DIALOG (object);
  LigmaPdbDialogClass *klass  = LIGMA_PDB_DIALOG_GET_CLASS (object);

  klass->dialogs = g_list_remove (klass->dialogs, object);

  g_clear_object (&dialog->pdb);
  g_clear_object (&dialog->caller_context);
  g_clear_object (&dialog->context);

  g_clear_pointer (&dialog->callback_name, g_free);

  g_clear_object (&dialog->menu_factory);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_pdb_dialog_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaPdbDialog *dialog = LIGMA_PDB_DIALOG (object);

  switch (property_id)
    {
    case PROP_PDB:
      dialog->pdb = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      dialog->caller_context = g_value_dup_object (value);
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
      dialog->menu_factory = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_pdb_dialog_response (GtkDialog *gtk_dialog,
                          gint       response_id)
{
  LigmaPdbDialog *dialog = LIGMA_PDB_DIALOG (gtk_dialog);

  ligma_pdb_dialog_run_callback (&dialog, TRUE);
  if (dialog)
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
ligma_pdb_dialog_run_callback (LigmaPdbDialog **dialog,
                              gboolean        closing)
{
  LigmaPdbDialogClass *klass = LIGMA_PDB_DIALOG_GET_CLASS (*dialog);
  LigmaObject         *object;

  g_object_add_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);
  object = ligma_context_get_by_type ((*dialog)->context, (*dialog)->select_type);

  if (*dialog && object        &&
      klass->run_callback      &&
      (*dialog)->callback_name &&
      ! (*dialog)->callback_busy)
    {
      (*dialog)->callback_busy = TRUE;

      if (ligma_pdb_lookup_procedure ((*dialog)->pdb, (*dialog)->callback_name))
        {
          LigmaValueArray *return_vals;
          GError         *error = NULL;

          return_vals = klass->run_callback (*dialog, object, closing, &error);

          if (*dialog &&
              g_value_get_enum (ligma_value_array_index (return_vals, 0)) !=
              LIGMA_PDB_SUCCESS)
            {
              const gchar *message;

              if (error && error->message)
                message = error->message;
              else
                message = _("The corresponding plug-in may have crashed.");

              ligma_message ((*dialog)->context->ligma, G_OBJECT (*dialog),
                            LIGMA_MESSAGE_ERROR,
                            _("Unable to run %s callback.\n%s"),
                            g_type_name (G_TYPE_FROM_INSTANCE (*dialog)),
                            message);
            }
          else if (*dialog && error)
            {
              ligma_message_literal ((*dialog)->context->ligma, G_OBJECT (*dialog),
                                    LIGMA_MESSAGE_ERROR,
                                    error->message);
              g_error_free (error);
            }

          ligma_value_array_unref (return_vals);
        }

      if (*dialog)
        (*dialog)->callback_busy = FALSE;
    }

  if (*dialog)
    g_object_remove_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);
}

LigmaPdbDialog *
ligma_pdb_dialog_get_by_callback (LigmaPdbDialogClass *klass,
                                 const gchar        *callback_name)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_PDB_DIALOG_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->dialogs; list; list = g_list_next (list))
    {
      LigmaPdbDialog *dialog = list->data;

      if (dialog->callback_name &&
          ! strcmp (callback_name, dialog->callback_name))
        return dialog;
    }

  return NULL;
}


/*  private functions  */

static void
ligma_pdb_dialog_context_changed (LigmaContext   *context,
                                 LigmaObject    *object,
                                 LigmaPdbDialog *dialog)
{
  if (object)
    ligma_pdb_dialog_run_callback (&dialog, FALSE);
}

static void
ligma_pdb_dialog_plug_in_closed (LigmaPlugInManager *manager,
                                LigmaPlugIn        *plug_in,
                                LigmaPdbDialog     *dialog)
{
  if (dialog->caller_context && dialog->callback_name)
    {
      if (! ligma_pdb_lookup_procedure (dialog->pdb, dialog->callback_name))
        {
          gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
        }
    }
}
