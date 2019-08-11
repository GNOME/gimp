/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsaveprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "gimp.h"
#include "gimpsaveprocedure.h"


struct _GimpSaveProcedurePrivate
{
  GimpSaveFunc    run_func;
  gpointer        run_data;
  GDestroyNotify  run_data_destroy;
};


static void   gimp_save_procedure_constructed (GObject              *object);
static void   gimp_save_procedure_finalize    (GObject              *object);

static void   gimp_save_procedure_install     (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_save_procedure_run         (GimpProcedure        *procedure,
                                               const GimpValueArray *args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSaveProcedure, gimp_save_procedure,
                            GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_save_procedure_parent_class


static void
gimp_save_procedure_class_init (GimpSaveProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed  = gimp_save_procedure_constructed;
  object_class->finalize     = gimp_save_procedure_finalize;

  procedure_class->install   = gimp_save_procedure_install;
  procedure_class->run       = gimp_save_procedure_run;
}

static void
gimp_save_procedure_init (GimpSaveProcedure *procedure)
{
  procedure->priv = gimp_save_procedure_get_instance_private (procedure);
}

static void
gimp_save_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_image_id ("image",
                                                         "Image",
                                                         "The image to save",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_drawable_id ("drawable",
                                                            "Drawable",
                                                            "The drawable "
                                                            "to save",
                                                            FALSE,
                                                            G_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("uri",
                                                       "URI",
                                                       "The URI of the file "
                                                       "to save to",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
  gimp_procedure_add_argument (procedure,
                               gimp_param_spec_string ("raw-uri",
                                                       "Raw URI",
                                                       "The URI of the file "
                                                       "to save to",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       GIMP_PARAM_READWRITE));
}

static void
gimp_save_procedure_finalize (GObject *object)
{
  GimpSaveProcedure *procedure = GIMP_SAVE_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_save_procedure_install (GimpProcedure *procedure)
{
  GimpFileProcedure *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  gimp_register_save_handler (gimp_procedure_get_name (procedure),
                              gimp_file_procedure_get_extensions (file_proc),
                              gimp_file_procedure_get_prefixes (file_proc));

  if (gimp_file_procedure_get_handles_uri (file_proc))
    gimp_register_file_handler_uri (gimp_procedure_get_name (procedure));

  mime_types = gimp_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    gimp_register_file_handler_mime (gimp_procedure_get_name (procedure),
                                     mime_types);

  priority = gimp_file_procedure_get_priority (file_proc);
  if (priority != 0)
    gimp_register_file_handler_priority (gimp_procedure_get_name (procedure),
                                         priority);
}

static GimpValueArray *
gimp_save_procedure_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  GimpSaveProcedure *save_proc = GIMP_SAVE_PROCEDURE (procedure);
  GimpValueArray    *remaining;
  GimpValueArray    *return_values;
  GimpRunMode        run_mode;
  gint32             image_id;
  gint32             drawable_id;
  const gchar       *uri;
  GFile             *file;
  gint               i;

  run_mode    = g_value_get_enum           (gimp_value_array_index (args, 0));
  image_id    = gimp_value_get_image_id    (gimp_value_array_index (args, 1));
  drawable_id = gimp_value_get_drawable_id (gimp_value_array_index (args, 2));
  uri         = g_value_get_string         (gimp_value_array_index (args, 3));
  /* raw_uri  = g_value_get_string         (gimp_value_array_index (args, 4)); */

  file = g_file_new_for_uri (uri);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - 5);

  for (i = 5; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  return_values = save_proc->priv->run_func (procedure,
                                             run_mode,
                                             image_id, drawable_id,
                                             file,
                                             remaining,
                                             save_proc->priv->run_data);

  gimp_value_array_unref (remaining);
  g_object_unref (file);

  return return_values;
}


/*  public functions  */

GimpProcedure  *
gimp_save_procedure_new (GimpPlugIn      *plug_in,
                         const gchar     *name,
                         GimpPDBProcType  proc_type,
                         GimpSaveFunc     run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  GimpSaveProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_INTERNAL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_SAVE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}
