/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppdbprogress.c
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

#include <glib-object.h>

#include "core-types.h"

#include "core/gimpcontext.h"

#include "pdb/procedural_db.h"

#include "gimppdbprogress.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_CONTEXT,
  PROP_CALLBACK_NAME
};


static void      gimp_pdb_progress_class_init     (GimpPdbProgressClass *klass);
static void      gimp_pdb_progress_init           (GimpPdbProgress      *progress,
                                                   GimpPdbProgressClass *klass);
static void      gimp_pdb_progress_progress_iface_init (GimpProgressInterface *progress_iface);

static GObject * gimp_pdb_progress_constructor    (GType               type,
                                                   guint               n_params,
                                                   GObjectConstructParam *params);
static void      gimp_pdb_progress_dispose        (GObject            *object);
static void      gimp_pdb_progress_finalize       (GObject            *object);
static void      gimp_pdb_progress_set_property   (GObject            *object,
                                                   guint               property_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);

static GimpProgress *
                gimp_pdb_progress_progress_start     (GimpProgress *progress,
                                                      const gchar  *message,
                                                      gboolean      cancelable);
static void     gimp_pdb_progress_progress_end       (GimpProgress *progress);
static gboolean gimp_pdb_progress_progress_is_active (GimpProgress *progress);
static void     gimp_pdb_progress_progress_set_text  (GimpProgress *progress,
                                                      const gchar  *message);
static void     gimp_pdb_progress_progress_set_value (GimpProgress *progress,
                                                      gdouble       percentage);
static gdouble  gimp_pdb_progress_progress_get_value (GimpProgress *progress);


static GObjectClass *parent_class = NULL;


GType
gimp_pdb_progress_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      static const GTypeInfo info =
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

      static const GInterfaceInfo progress_iface_info =
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

  object_class->constructor  = gimp_pdb_progress_constructor;
  object_class->dispose      = gimp_pdb_progress_dispose;
  object_class->finalize     = gimp_pdb_progress_finalize;
  object_class->set_property = gimp_pdb_progress_set_property;

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        GIMP_TYPE_CONTEXT,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_CALLBACK_NAME,
                                   g_param_spec_string ("callback-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        G_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_pdb_progress_init (GimpPdbProgress      *progress,
                        GimpPdbProgressClass *klass)
{
  klass->progresses = g_list_prepend (klass->progresses, progress);
}

static void
gimp_pdb_progress_progress_iface_init (GimpProgressInterface *progress_iface)
{
  progress_iface->start     = gimp_pdb_progress_progress_start;
  progress_iface->end       = gimp_pdb_progress_progress_end;
  progress_iface->is_active = gimp_pdb_progress_progress_is_active;
  progress_iface->set_text  = gimp_pdb_progress_progress_set_text;
  progress_iface->set_value = gimp_pdb_progress_progress_set_value;
  progress_iface->get_value = gimp_pdb_progress_progress_get_value;
}

static GObject *
gimp_pdb_progress_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
  GObject         *object;
  GimpPdbProgress *progress;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

  progress = GIMP_PDB_PROGRESS (object);

  g_assert (GIMP_IS_CONTEXT (progress->context));

  return object;
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

  if (progress->context)
    {
      g_object_unref (progress->context);
      progress->context = NULL;
    }

  if (progress->callback_name)
    {
      g_free (progress->callback_name);
      progress->callback_name = NULL;
    }

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
    case PROP_CONTEXT:
      if (progress->context)
        g_object_unref (progress->context);
      progress->context = (GimpContext *) g_value_dup_object (value);
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

static void
gimp_pdb_progress_run_callback (GimpPdbProgress     *progress,
                                GimpProgressCommand  command,
                                const gchar         *text,
                                gdouble              value)
{
  if (! progress->callback_busy)
    {
      Argument *return_vals;
      gint      n_return_vals;

      progress->callback_busy = TRUE;

      return_vals = procedural_db_run_proc (progress->context->gimp,
                                            progress->context,
                                            NULL,
                                            progress->callback_name,
                                            &n_return_vals,
                                            GIMP_PDB_INT32,  command,
                                            GIMP_PDB_STRING, text,
                                            GIMP_PDB_FLOAT,  value,
                                            GIMP_PDB_END);

      if (! return_vals ||
          return_vals[0].value.pdb_int != GIMP_PDB_SUCCESS)
        {
          g_message (_("Unable to run %s callback. "
                       "The corresponding plug-in may have crashed."),
                     g_type_name (G_TYPE_FROM_INSTANCE (progress)));
        }

      if (return_vals)
        procedural_db_destroy_args (return_vals, n_return_vals);

      progress->callback_busy = FALSE;
    }
}

static GimpProgress *
gimp_pdb_progress_progress_start (GimpProgress *progress,
                                  const gchar  *message,
                                  gboolean      cancelable)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  if (! pdb_progress->active)
    {
      gimp_pdb_progress_run_callback (pdb_progress,
                                      GIMP_PROGRESS_COMMAND_START,
                                      message, 0.0);

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
                                      NULL, 0.0);

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
                                    message, 0.0);
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
                                      NULL, percentage);
      pdb_progress->value = percentage;
    }
}

static gdouble
gimp_pdb_progress_progress_get_value (GimpProgress *progress)
{
  GimpPdbProgress *pdb_progress = GIMP_PDB_PROGRESS (progress);

  return pdb_progress->value;

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

      if (progress->callback_name &&
          ! strcmp (callback_name, progress->callback_name))
	return progress;
    }

  return NULL;
}

void
gimp_pdb_progresss_check_callback (GimpPdbProgressClass *klass)
{
  GList *list;

  g_return_if_fail (GIMP_IS_PDB_PROGRESS_CLASS (klass));

  list = klass->progresses;

  while (list)
    {
      GimpPdbProgress *progress = list->data;

      list = g_list_next (list);

      if (progress->callback_name)
        {
          if (! procedural_db_lookup (progress->context->gimp,
                                      progress->callback_name))
            {
            }
        }
    }
}
