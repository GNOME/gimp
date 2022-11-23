/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapdbprogress.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "core/ligmacontext.h"

#include "pdb/ligmapdb.h"

#include "ligma.h"
#include "ligmaparamspecs.h"
#include "ligmapdbprogress.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_PDB,
  PROP_CONTEXT,
  PROP_CALLBACK_NAME
};


static void      ligma_pdb_progress_class_init     (LigmaPdbProgressClass *klass);
static void      ligma_pdb_progress_init           (LigmaPdbProgress      *progress,
                                                   LigmaPdbProgressClass *klass);
static void ligma_pdb_progress_progress_iface_init (LigmaProgressInterface *iface);

static void      ligma_pdb_progress_constructed    (GObject            *object);
static void      ligma_pdb_progress_dispose        (GObject            *object);
static void      ligma_pdb_progress_finalize       (GObject            *object);
static void      ligma_pdb_progress_set_property   (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);

static LigmaProgress * ligma_pdb_progress_progress_start   (LigmaProgress *progress,
                                                          gboolean      cancellable,
                                                          const gchar  *message);
static void     ligma_pdb_progress_progress_end           (LigmaProgress *progress);
static gboolean ligma_pdb_progress_progress_is_active     (LigmaProgress *progress);
static void     ligma_pdb_progress_progress_set_text      (LigmaProgress *progress,
                                                          const gchar  *message);
static void     ligma_pdb_progress_progress_set_value     (LigmaProgress *progress,
                                                          gdouble       percentage);
static gdouble  ligma_pdb_progress_progress_get_value     (LigmaProgress *progress);
static void     ligma_pdb_progress_progress_pulse         (LigmaProgress *progress);
static guint32  ligma_pdb_progress_progress_get_window_id (LigmaProgress *progress);


static GObjectClass *parent_class = NULL;


GType
ligma_pdb_progress_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (LigmaPdbProgressClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) ligma_pdb_progress_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (LigmaPdbProgress),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) ligma_pdb_progress_init,
      };

      const GInterfaceInfo progress_iface_info =
      {
        (GInterfaceInitFunc) ligma_pdb_progress_progress_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "LigmaPdbProgress",
                                     &info, 0);

      g_type_add_interface_static (type, LIGMA_TYPE_PROGRESS,
                                   &progress_iface_info);
    }

  return type;
}

static void
ligma_pdb_progress_class_init (LigmaPdbProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = ligma_pdb_progress_constructed;
  object_class->dispose      = ligma_pdb_progress_dispose;
  object_class->finalize     = ligma_pdb_progress_finalize;
  object_class->set_property = ligma_pdb_progress_set_property;

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        LIGMA_TYPE_PDB,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_pdb_progress_init (LigmaPdbProgress      *progress,
                        LigmaPdbProgressClass *klass)
{
  klass->progresses = g_list_prepend (klass->progresses, progress);
}

static void
ligma_pdb_progress_progress_iface_init (LigmaProgressInterface *iface)
{
  iface->start         = ligma_pdb_progress_progress_start;
  iface->end           = ligma_pdb_progress_progress_end;
  iface->is_active     = ligma_pdb_progress_progress_is_active;
  iface->set_text      = ligma_pdb_progress_progress_set_text;
  iface->set_value     = ligma_pdb_progress_progress_set_value;
  iface->get_value     = ligma_pdb_progress_progress_get_value;
  iface->pulse         = ligma_pdb_progress_progress_pulse;
  iface->get_window_id = ligma_pdb_progress_progress_get_window_id;
}

static void
ligma_pdb_progress_constructed (GObject *object)
{
  LigmaPdbProgress *progress = LIGMA_PDB_PROGRESS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_PDB (progress->pdb));
  ligma_assert (LIGMA_IS_CONTEXT (progress->context));
}

static void
ligma_pdb_progress_dispose (GObject *object)
{
  LigmaPdbProgressClass *klass = LIGMA_PDB_PROGRESS_GET_CLASS (object);

  klass->progresses = g_list_remove (klass->progresses, object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_pdb_progress_finalize (GObject *object)
{
  LigmaPdbProgress *progress = LIGMA_PDB_PROGRESS (object);

  g_clear_object (&progress->pdb);
  g_clear_object (&progress->context);
  g_clear_pointer (&progress->callback_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_pdb_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaPdbProgress *progress = LIGMA_PDB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PDB:
      if (progress->pdb)
        g_object_unref (progress->pdb);
      progress->pdb = g_value_dup_object (value);
      break;

    case PROP_CONTEXT:
      if (progress->context)
        g_object_unref (progress->context);
      progress->context = g_value_dup_object (value);
      break;

    case PROP_CALLBACK_NAME:
      if (progress->callback_name)
        g_free (progress->callback_name);
      progress->callback_name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gdouble
ligma_pdb_progress_run_callback (LigmaPdbProgress     *progress,
                                LigmaProgressCommand  command,
                                const gchar         *text,
                                gdouble              value)
{
  gdouble retval = 0;

  if (progress->callback_name && ! progress->callback_busy)
    {
      LigmaValueArray *return_vals;

      progress->callback_busy = TRUE;

      return_vals =
        ligma_pdb_execute_procedure_by_name (progress->pdb,
                                            progress->context,
                                            NULL, NULL,
                                            progress->callback_name,
                                            LIGMA_TYPE_PROGRESS_COMMAND, command,
                                            G_TYPE_STRING,              text,
                                            G_TYPE_DOUBLE,              value,
                                            G_TYPE_NONE);

      if (g_value_get_enum (ligma_value_array_index (return_vals, 0)) !=
          LIGMA_PDB_SUCCESS)
        {
          ligma_message (progress->context->ligma, NULL, LIGMA_MESSAGE_ERROR,
                        _("Unable to run %s callback. "
                          "The corresponding plug-in may have crashed."),
                        g_type_name (G_TYPE_FROM_INSTANCE (progress)));
        }
      else if (ligma_value_array_length (return_vals) >= 2 &&
               G_VALUE_HOLDS_DOUBLE (ligma_value_array_index (return_vals, 1)))
        {
          retval = g_value_get_double (ligma_value_array_index (return_vals, 1));
        }

      ligma_value_array_unref (return_vals);

      progress->callback_busy = FALSE;
    }

  return retval;
}

static LigmaProgress *
ligma_pdb_progress_progress_start (LigmaProgress *progress,
                                  gboolean      cancellable,
                                  const gchar  *message)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  if (! pdb_progress->active)
    {
      ligma_pdb_progress_run_callback (pdb_progress,
                                      LIGMA_PROGRESS_COMMAND_START,
                                      message, 0.0);

      pdb_progress->active = TRUE;
      pdb_progress->value  = 0.0;

      return progress;
    }

  return NULL;
}

static void
ligma_pdb_progress_progress_end (LigmaProgress *progress)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      ligma_pdb_progress_run_callback (pdb_progress,
                                      LIGMA_PROGRESS_COMMAND_END,
                                      NULL, 0.0);

      pdb_progress->active = FALSE;
      pdb_progress->value  = 0.0;
    }
}

static gboolean
ligma_pdb_progress_progress_is_active (LigmaProgress *progress)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  return pdb_progress->active;
}

static void
ligma_pdb_progress_progress_set_text (LigmaProgress *progress,
                                     const gchar  *message)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    ligma_pdb_progress_run_callback (pdb_progress,
                                    LIGMA_PROGRESS_COMMAND_SET_TEXT,
                                    message, 0.0);
}

static void
ligma_pdb_progress_progress_set_value (LigmaProgress *progress,
                                      gdouble       percentage)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      ligma_pdb_progress_run_callback (pdb_progress,
                                      LIGMA_PROGRESS_COMMAND_SET_VALUE,
                                      NULL, percentage);
      pdb_progress->value = percentage;
    }
}

static gdouble
ligma_pdb_progress_progress_get_value (LigmaProgress *progress)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  return pdb_progress->value;

}

static void
ligma_pdb_progress_progress_pulse (LigmaProgress *progress)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    ligma_pdb_progress_run_callback (pdb_progress,
                                    LIGMA_PROGRESS_COMMAND_PULSE,
                                    NULL, 0.0);
}

static guint32
ligma_pdb_progress_progress_get_window_id (LigmaProgress *progress)
{
  LigmaPdbProgress *pdb_progress = LIGMA_PDB_PROGRESS (progress);

  return (guint32)
    ligma_pdb_progress_run_callback (pdb_progress,
                                    LIGMA_PROGRESS_COMMAND_GET_WINDOW,
                                    NULL, 0.0);
}

LigmaPdbProgress *
ligma_pdb_progress_get_by_callback (LigmaPdbProgressClass *klass,
                                   const gchar          *callback_name)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_PDB_PROGRESS_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->progresses; list; list = g_list_next (list))
    {
      LigmaPdbProgress *progress = list->data;

      if (! g_strcmp0 (callback_name, progress->callback_name))
        return progress;
    }

  return NULL;
}
