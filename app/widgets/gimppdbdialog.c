/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbdialog.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpresource.h"

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


static void     gimp_pdb_dialog_constructed       (GObject            *object);
static void     gimp_pdb_dialog_dispose           (GObject            *object);
static void     gimp_pdb_dialog_set_property      (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);

static void     gimp_pdb_dialog_response          (GtkDialog          *dialog,
                                                   gint                response_id);

static void     gimp_pdb_dialog_context_changed   (GimpContext        *context,
                                                   GimpObject         *object,
                                                   GimpPdbDialog      *dialog);
static void     gimp_pdb_dialog_plug_in_closed    (GimpPlugInManager  *manager,
                                                   GimpPlugIn         *plug_in,
                                                   GimpPdbDialog      *dialog);

static gboolean gimp_pdb_dialog_run_callback_idle (GimpPdbDialog      *dialog);


G_DEFINE_ABSTRACT_TYPE (GimpPdbDialog, gimp_pdb_dialog, GIMP_TYPE_DIALOG)

#define parent_class gimp_pdb_dialog_parent_class


static void
gimp_pdb_dialog_class_init (GimpPdbDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = gimp_pdb_dialog_constructed;
  object_class->dispose      = gimp_pdb_dialog_dispose;
  object_class->set_property = gimp_pdb_dialog_set_property;

  dialog_class->response     = gimp_pdb_dialog_response;

  klass->run_callback        = NULL;
  klass->get_object          = NULL;
  klass->set_object          = NULL;

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
gimp_pdb_dialog_init (GimpPdbDialog *dialog)
{
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_OK"), GTK_RESPONSE_OK);
  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         _("_Cancel"), GTK_RESPONSE_CANCEL);
}

static void
gimp_pdb_dialog_constructed (GObject *object)
{
  GimpPdbDialog      *dialog = GIMP_PDB_DIALOG (object);
  GimpPdbDialogClass *klass  = GIMP_PDB_DIALOG_GET_CLASS (object);
  const gchar        *signal_name;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  klass->dialogs = g_list_prepend (klass->dialogs, dialog);

  gimp_assert (GIMP_IS_PDB (dialog->pdb));
  gimp_assert (GIMP_IS_CONTEXT (dialog->caller_context));
  gimp_assert (g_type_is_a (dialog->select_type, GIMP_TYPE_OBJECT));

  dialog->context = gimp_context_new (dialog->caller_context->gimp,
                                      G_OBJECT_TYPE_NAME (object),
                                      NULL);

  if (g_type_is_a (dialog->select_type, GIMP_TYPE_RESOURCE))
    {
      gimp_context_set_by_type (dialog->context, dialog->select_type,
                                dialog->initial_object);

      signal_name = gimp_context_type_to_signal_name (dialog->select_type);

      g_signal_connect_object (dialog->context, signal_name,
                               G_CALLBACK (gimp_pdb_dialog_context_changed),
                               dialog, 0);
    }

  g_signal_connect_object (dialog->caller_context->gimp->plug_in_manager,
                           "plug-in-closed",
                           G_CALLBACK (gimp_pdb_dialog_plug_in_closed),
                           dialog, 0);
}

static void
gimp_pdb_dialog_dispose (GObject *object)
{
  GimpPdbDialog      *dialog = GIMP_PDB_DIALOG (object);
  GimpPdbDialogClass *klass  = GIMP_PDB_DIALOG_GET_CLASS (object);

  klass->dialogs = g_list_remove (klass->dialogs, object);

  g_clear_object (&dialog->pdb);
  g_clear_object (&dialog->caller_context);
  g_clear_object (&dialog->context);
  g_clear_object (&dialog->initial_object);

  g_clear_pointer (&dialog->callback_name, g_free);

  g_clear_object (&dialog->menu_factory);

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
      dialog->pdb = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      dialog->caller_context = g_value_dup_object (value);
      break;

    case PROP_SELECT_TYPE:
      dialog->select_type = g_value_get_gtype (value);
      break;

    case PROP_INITIAL_OBJECT:
      dialog->initial_object = g_value_dup_object (value);
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
gimp_pdb_dialog_response (GtkDialog *gtk_dialog,
                          gint       response_id)
{
  GimpPdbDialog *dialog = GIMP_PDB_DIALOG (gtk_dialog);

  if (response_id != GTK_RESPONSE_OK)
    {
      if (g_type_is_a (dialog->select_type, GIMP_TYPE_RESOURCE))
        {
          gimp_context_set_by_type (dialog->context, dialog->select_type,
                                    dialog->initial_object);
        }
      else
        {
          GimpPdbDialogClass *klass = GIMP_PDB_DIALOG_GET_CLASS (dialog);

          g_return_if_fail (klass->set_object != NULL);
          klass->set_object (dialog, dialog->initial_object);
        }
    }

  gimp_pdb_dialog_run_callback (&dialog, TRUE);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
gimp_pdb_dialog_run_callback (GimpPdbDialog **dialog,
                              gboolean        closing)
{
  GimpPdbDialogClass *klass = GIMP_PDB_DIALOG_GET_CLASS (*dialog);
  GimpObject         *object;

  g_object_add_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);

  if (g_type_is_a ((*dialog)->select_type, GIMP_TYPE_RESOURCE))
    {
      object = gimp_context_get_by_type ((*dialog)->context, (*dialog)->select_type);
    }
  else
    {
      g_return_if_fail (klass->get_object != NULL);
      object = klass->get_object (*dialog);
    }

  if (*dialog                  &&
      klass->run_callback      &&
      (*dialog)->callback_name &&
      ! (*dialog)->callback_busy)
    {
      (*dialog)->callback_busy = TRUE;

      if (gimp_pdb_lookup_procedure ((*dialog)->pdb, (*dialog)->callback_name) &&
          (object == NULL || g_type_is_a (G_TYPE_FROM_INSTANCE (object), (*dialog)->select_type)))
        {
          GimpValueArray *return_vals;
          GError         *error = NULL;

          return_vals = klass->run_callback (*dialog, object, closing, &error);

          if (*dialog &&
              g_value_get_enum (gimp_value_array_index (return_vals, 0)) !=
              GIMP_PDB_SUCCESS)
            {
              const gchar *message;

              if (error && error->message)
                message = error->message;
              else
                message = _("The corresponding plug-in may have crashed.");

              gimp_message ((*dialog)->caller_context->gimp, G_OBJECT (*dialog),
                            GIMP_MESSAGE_ERROR,
                            _("Unable to run %s callback.\n%s"),
                            g_type_name (G_TYPE_FROM_INSTANCE (*dialog)),
                            message);
            }
          else if (*dialog && error)
            {
              gimp_message_literal ((*dialog)->caller_context->gimp, G_OBJECT (*dialog),
                                    GIMP_MESSAGE_ERROR,
                                    error->message);
              g_error_free (error);
            }

          gimp_value_array_unref (return_vals);
        }

      if (*dialog)
        (*dialog)->callback_busy = FALSE;
    }

  if (*dialog)
    g_object_remove_weak_pointer (G_OBJECT (*dialog), (gpointer) dialog);
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
  /* XXX Long explanation below because this was annoying to debug!
   *
   * The context might change for 2 reasons: either from code coming from the
   * plug-in or because of direct GUI interaction with the dialog. This second
   * case may happen while the plug-in is doing something else completely and
   * making PDB calls too. If the callback itself triggers the plug-in to run
   * other PDB calls, we may end up in a case where PDB requests made by the
   * plug-in receive the wrong result. Here is an actual case I encountered:
   *
   * 1. The plug-in calls gimp-gradient-get-uniform-samples to draw a button.
   * 2. Because you click inside the dialog (using any of the Gimp*Select
   *    subclasses, e.g. GimpBrushSelect), the core dialog run its callback
   *    indicating that a resource changed. This is unrelated to step 1. It just
   *    happens nearly in the same time by a race condition.
   * 3. The plug-in receives the callback as a temp procedure request, processes
   *    it first. In particular, as part of the code in libgimp/plug-in, it
   *    verifies the resource is valid calling gimp-resource-id-is-valid.
   * 4. Meanwhile, the core returns gimp-gradient-get-uniform-samples result.
   * 5. The plug-in takes it as a result for gimp-resource-id-is-valid with
   *    invalid return values (size and types).
   * 6. Then again, it receives result of gimp-resource-id-is-valid as being the
   *    result of gimp-gradient-get-uniform-samples, again invalid.
   *
   * This scenario mostly happens because the callback is not the consequence of
   * the plug-in's requests (if the callback from core were the consequence of a
   * call from the plug-in, the PDB protocol would ensure that every message in
   * both direction get its return values in the proper order). The problem here
   * is really that the callback interrupts unexpectedly PDB requests coming
   * from the plug-in while the plug-in process can't really know that this
   * callback is not a consequence of its own PDB call.
   *
   * Running as idle will make the callback run when the core is not already
   * processing anything, which should also mean that the plug-in is not waiting
   * for an unrelated procedure result.
   * To be fair, I am not 100% sure if a complex race condition may not still
   * happen, but extensive testing were successful so far.
   */
  if (object)
    g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                     (GSourceFunc) gimp_pdb_dialog_run_callback_idle,
                     g_object_ref (dialog), g_object_unref);
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

static gboolean
gimp_pdb_dialog_run_callback_idle (GimpPdbDialog *dialog)
{
  gimp_pdb_dialog_run_callback (&dialog, FALSE);

  return G_SOURCE_REMOVE;
}
