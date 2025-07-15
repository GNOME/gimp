/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbprogress.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "core/gimpcontext.h"

#include "pdb/gimppdb.h"

#include "gimp.h"
#include "gimpparamspecs.h"
#include "gimppdbprogress.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_PDB,
  PROP_CONTEXT,
  PROP_CALLBACK_NAME
};


static void      gimp_pdb_progress_class_init     (GimpPdbProgressClass *klass);
static void      gimp_pdb_progress_init           (GimpPdbProgress      *progress,
                                                   GimpPdbProgressClass *klass);
static void gimp_pdb_progress_progress_iface_init (GimpProgressInterface *iface);

static void      gimp_pdb_progress_constructed    (GObject            *object);
static void      gimp_pdb_progress_dispose        (GObject            *object);
static void      gimp_pdb_progress_finalize       (GObject            *object);
static void      gimp_pdb_progress_set_property   (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);

static GimpProgress * gimp_pdb_progress_progress_start   (GimpProgress *progress,
                                                          gboolean      cancellable,
                                                          const gchar  *message);
static void     gimp_pdb_progress_progress_end           (GimpProgress *progress);
static gboolean gimp_pdb_progress_progress_is_active     (GimpProgress *progress);
static void     gimp_pdb_progress_progress_set_text      (GimpProgress *progress,
                                                          const gchar  *message);
static void     gimp_pdb_progress_progress_set_value     (GimpProgress *progress,
                                                          gdouble       percentage);
static gdouble  gimp_pdb_progress_progress_get_value     (GimpProgress *progress);
static void     gimp_pdb_progress_progress_pulse         (GimpProgress *progress);
static GBytes * gimp_pdb_progress_progress_get_window_id (GimpProgress *progress);


static GObjectClass *parent_class = NULL;


GType
gimp_pdb_progress_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GimpPdbProgressClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_pdb_progress_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data     */
        sizeof (GimpPdbProgress),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_pdb_progress_init,
      };

      const GInterfaceInfo progress_iface_info =
      {
        (GInterfaceInitFunc) gimp_pdb_progress_progress_iface_init,
        NULL,           /* iface_finalize */
        NULL            /* iface_data     */
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "GimpPdbProgress",
                                     &info, 0);

      g_type_add_interface_static (type, GIMP_TYPE_PROGRESS,
                                   &progress_iface_info);
    }

  return type;
}

static void
gimp_pdb_progress_class_init (GimpPdbProgressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->constructed  = gimp_pdb_progress_constructed;
  object_class->dispose      = gimp_pdb_progress_dispose;
  object_class->finalize     = gimp_pdb_progress_finalize;
  object_class->set_property = gimp_pdb_progress_set_property;

  g_object_class_install_property (object_class, PROP_PDB,
                                   g_param_spec_object ("pdb", NULL, NULL,
                                                        GIMP_TYPE_PDB,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_pdb_progress_init (GimpPdbProgress      *progress,
                        GimpPdbProgressClass *klass)
{
  klass->progresses = g_list_prepend (klass->progresses, progress);
}

static void
gimp_pdb_progress_progress_iface_init (GimpProgressInterface *iface)
{
  iface->start         = gimp_pdb_progress_progress_start;
  iface->end           = gimp_pdb_progress_progress_end;
  iface->is_active     = gimp_pdb_progress_progress_is_active;
  iface->set_text      = gimp_pdb_progress_progress_set_text;
  iface->set_value     = gimp_pdb_progress_progress_set_value;
  iface->get_value     = gimp_pdb_progress_progress_get_value;
  iface->pulse         = gimp_pdb_progress_progress_pulse;
  iface->get_window_id = gimp_pdb_progress_progress_get_window_id;
}

static void
gimp_pdb_progress_constructed (GObject *object)
{
  GimpPdbProgress *progress = GIMP_PDB_PROGRESS (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_PDB (progress->pdb));
  gimp_assert (GIMP_IS_CONTEXT (progress->context));
}

static void
gimp_pdb_progress_dispose (GObject *object)
{
  GimpPdbProgressClass *klass = GIMP_PDB_PROGRESS_GET_CLASS (object);

  klass->progresses = g_list_remove (klass->progresses, object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_pdb_progress_finalize (GObject *object)
{
  GimpPdbProgress *progress = GIMP_PDB_PROGRESS (object);

  g_clear_object (&progress->pdb);
  g_clear_object (&progress->context);
  g_clear_pointer (&progress->callback_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_pdb_progress_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GimpPdbProgress *progress = GIMP_PDB_PROGRESS (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_set_object (&progress->pdb, g_value_get_object (value));
      break;
    case PROP_CONTEXT:
      g_set_object (&progress->context, g_value_get_object (value));
      break;
    case PROP_CALLBACK_NAME:
      g_set_str (&progress->callback_name, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_pdb_progress_run_callback (GimpPdbProgress      *progress,
                                GimpProgressCommand   command,
                                const gchar          *text,
                                gdouble               value,
                                GBytes              **handle)
{
  if (progress->callback_name && ! progress->callback_busy)
    {
      GimpValueArray *return_vals;

      progress->callback_busy = TRUE;

      return_vals =
        gimp_pdb_execute_procedure_by_name (progress->pdb,
                                            progress->context,
                                            NULL, NULL,
                                            progress->callback_name,
                                            GIMP_TYPE_PROGRESS_COMMAND, command,
                                            G_TYPE_STRING,              text,
                                            G_TYPE_DOUBLE,              value,
                                            G_TYPE_NONE);

      if (g_value_get_enum (gimp_value_array_index (return_vals, 0)) !=
          GIMP_PDB_SUCCESS)
        {
          gimp_message (progress->context->gimp, NULL, GIMP_MESSAGE_ERROR,
                        _("Unable to run %s callback. "
                          "The corresponding plug-in may have crashed."),
                        g_type_name (G_TYPE_FROM_INSTANCE (progress)));
        }
      else if (gimp_value_array_length (return_vals) >= 2 &&
               handle != NULL                             &&
               G_VALUE_HOLDS_BOXED (gimp_value_array_index (return_vals, 1)))
        {
          *handle = g_value_dup_boxed (gimp_value_array_index (return_vals, 1));
        }

      gimp_value_array_unref (return_vals);

      progress->callback_busy = FALSE;
    }
}

static GimpProgress *
gimp_pdb_progress_progress_start (GimpProgress *progress,
                                  gboolean      cancellable,
                                  const gchar  *message)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (! pdb_progress->active)
    {
      gimp_pdb_progress_run_callback (pdb_progress,
                                      GIMP_PROGRESS_COMMAND_START,
                                      message, 0.0, NULL);

      pdb_progress->active = TRUE;
      pdb_progress->value  = 0.0;

      return progress;
    }

  return NULL;
}

static void
gimp_pdb_progress_progress_end (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      gimp_pdb_progress_run_callback (pdb_progress,
                                      GIMP_PROGRESS_COMMAND_END,
                                      NULL, 0.0, NULL);

      pdb_progress->active = FALSE;
      pdb_progress->value  = 0.0;
    }
}

static gboolean
gimp_pdb_progress_progress_is_active (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  return pdb_progress->active;
}

static void
gimp_pdb_progress_progress_set_text (GimpProgress *progress,
                                     const gchar  *message)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    gimp_pdb_progress_run_callback (pdb_progress,
                                    GIMP_PROGRESS_COMMAND_SET_TEXT,
                                    message, 0.0, NULL);
}

static void
gimp_pdb_progress_progress_set_value (GimpProgress *progress,
                                      gdouble       percentage)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    {
      gimp_pdb_progress_run_callback (pdb_progress,
                                      GIMP_PROGRESS_COMMAND_SET_VALUE,
                                      NULL, percentage, NULL);
      pdb_progress->value = percentage;
    }
}

static gdouble
gimp_pdb_progress_progress_get_value (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  return pdb_progress->value;

}

static void
gimp_pdb_progress_progress_pulse (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (pdb_progress->active)
    gimp_pdb_progress_run_callback (pdb_progress,
                                    GIMP_PROGRESS_COMMAND_PULSE,
                                    NULL, 0.0, NULL);
}

static GBytes *
gimp_pdb_progress_progress_get_window_id (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);
  GBytes          *handle       = NULL;

  gimp_pdb_progress_run_callback (pdb_progress,
                                  GIMP_PROGRESS_COMMAND_GET_WINDOW,
                                  NULL, 0.0, &handle);
  return handle;
}

GimpPdbProgress *
gimp_pdb_progress_get_by_callback (GimpPdbProgressClass *klass,
                                   const gchar          *callback_name)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_PDB_PROGRESS_CLASS (klass), NULL);
  g_return_val_if_fail (callback_name != NULL, NULL);

  for (list = klass->progresses; list; list = g_list_next (list))
    {
      GimpPdbProgress *progress = list->data;

      if (! g_strcmp0 (callback_name, progress->callback_name))
        return progress;
    }

  return NULL;
}
