/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimploadprocedure.c
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

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimploadprocedure.h"
#include "gimppdb_pdb.h"
#include "gimpplugin-private.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


/**
 * GimpLoadProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write file load
 * procedures.
 *
 * It automatically adds the standard
 *
 * ( [enum@RunMode], [iface@Gio.File] )
 *
 * arguments and the standard
 *
 * ( [class@Image] )
 *
 * return value of a load procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * "args" [struct@ValueArray] of [callback@RunImageFunc] only contains
 * additionally added arguments.
 */


typedef struct _GimpLoadProcedurePrivate
{
  GimpRunLoadFunc  run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;

  gboolean         handles_raw;
  gchar           *thumbnail_proc;
} GimpLoadProcedurePrivate;


static void   gimp_load_procedure_constructed   (GObject              *object);
static void   gimp_load_procedure_finalize      (GObject              *object);

static void   gimp_load_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_load_procedure_run           (GimpProcedure        *procedure,
                                                 const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_load_procedure_create_config (GimpProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpLoadProcedure, gimp_load_procedure, GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_load_procedure_parent_class


static void
gimp_load_procedure_class_init (GimpLoadProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_load_procedure_constructed;
  object_class->finalize         = gimp_load_procedure_finalize;

  procedure_class->install       = gimp_load_procedure_install;
  procedure_class->run           = gimp_load_procedure_run;
  procedure_class->create_config = gimp_load_procedure_create_config;
}

static void
gimp_load_procedure_init (GimpLoadProcedure *procedure)
{
}

static void
gimp_load_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_file_argument (procedure, "file",
                                    "File",
                                    "The file to load",
                                    GIMP_FILE_CHOOSER_ACTION_OPEN,
                                    FALSE, NULL,
                                    GIMP_PARAM_READWRITE);

  gimp_procedure_add_image_return_value (procedure, "image",
                                         "Image",
                                         "Output image",
                                         TRUE,
                                         GIMP_PARAM_READWRITE);
}

static void
gimp_load_procedure_finalize (GObject *object)
{
  GimpLoadProcedure        *procedure = GIMP_LOAD_PROCEDURE (object);
  GimpLoadProcedurePrivate *priv;

  priv = gimp_load_procedure_get_instance_private (procedure);

  if (priv->run_data_destroy)
    priv->run_data_destroy (priv->run_data);

  g_clear_pointer (&priv->thumbnail_proc, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_load_procedure_install (GimpProcedure *procedure)
{
  GimpLoadProcedurePrivate *priv;
  GimpLoadProcedure        *load_proc = GIMP_LOAD_PROCEDURE (procedure);
  GimpFileProcedure        *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar              *mime_types;
  gint                      priority;

  priv = gimp_load_procedure_get_instance_private (load_proc);

  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  _gimp_pdb_set_file_proc_load_handler (gimp_procedure_get_name (procedure),
                                        gimp_file_procedure_get_extensions (file_proc),
                                        gimp_file_procedure_get_prefixes (file_proc),
                                        gimp_file_procedure_get_magics (file_proc));

  if (gimp_file_procedure_get_handles_remote (file_proc))
    _gimp_pdb_set_file_proc_handles_remote (gimp_procedure_get_name (procedure));

  mime_types = gimp_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    _gimp_pdb_set_file_proc_mime_types (gimp_procedure_get_name (procedure),
                                        mime_types);

  priority = gimp_file_procedure_get_priority (file_proc);
  if (priority != 0)
    _gimp_pdb_set_file_proc_priority (gimp_procedure_get_name (procedure),
                                      priority);

  if (priv->handles_raw)
    _gimp_pdb_set_file_proc_handles_raw (gimp_procedure_get_name (procedure));

  if (priv->thumbnail_proc)
    _gimp_pdb_set_file_proc_thumbnail_loader (gimp_procedure_get_name (procedure),
                                              priv->thumbnail_proc);
}

#define ARG_OFFSET 2

static GimpValueArray *
gimp_load_procedure_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  GimpLoadProcedurePrivate *priv;
  GimpPlugIn               *plug_in;
  GimpLoadProcedure        *load_proc = GIMP_LOAD_PROCEDURE (procedure);
  GimpValueArray           *remaining;
  GimpValueArray           *return_values;
  GimpProcedureConfig      *config;
  GimpImage                *image    = NULL;
  GimpMetadata             *metadata = NULL;
  gchar                    *mimetype = NULL;
  GimpMetadataLoadFlags     flags    = GIMP_METADATA_LOAD_ALL;
  GimpPDBStatusType         status   = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode               run_mode;
  GFile                    *file;
  gint                      i;

  priv = gimp_load_procedure_get_instance_private (load_proc);

  run_mode = GIMP_VALUES_GET_ENUM (args, 0);
  file     = GIMP_VALUES_GET_FILE (args, 1);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config   = _gimp_procedure_create_run_config (procedure);
  mimetype = (gchar *) gimp_file_procedure_get_mime_types (GIMP_FILE_PROCEDURE (procedure));

  if (mimetype != NULL)
    {
      char *delim;

      mimetype = g_strdup (mimetype);
      mimetype = g_strstrip (mimetype);
      delim = strstr (mimetype, ",");
      if (delim)
        *delim = '\0';
      /* Though docs only writes about the list being comma-separated, our
       * code apparently also split by spaces.
       */
      delim = strstr (mimetype, " ");
      if (delim)
        *delim = '\0';
      delim = strstr (mimetype, "\t");
      if (delim)
        *delim = '\0';

      metadata = gimp_metadata_load_from_file (file, NULL);
      g_free (mimetype);
    }
  else
    {
      flags = GIMP_METADATA_LOAD_NONE;
    }

  if (metadata == NULL)
    metadata = gimp_metadata_new ();

  _gimp_procedure_config_begin_run (config, image, run_mode, remaining, NULL);

  return_values = priv->run_func (procedure, run_mode,
                                  file, metadata, &flags,
                                  config, priv->run_data);

  if (return_values != NULL                       &&
      gimp_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (return_values, 0)))
    status = GIMP_VALUES_GET_ENUM (return_values, 0);

  _gimp_procedure_config_end_run (config, status);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_value_array_length (return_values) < 2 ||
          ! GIMP_VALUE_HOLDS_IMAGE (gimp_value_array_index (return_values, 1)))
        {
          GError *error = NULL;

          status = GIMP_PDB_EXECUTION_ERROR;
          g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                       _("This file loading plug-in returned SUCCESS as a status without an image. "
                         "This is a bug in the plug-in code. Contact the plug-in developer."));
          gimp_value_array_unref (return_values);
          return_values = gimp_procedure_new_return_values (procedure, status, error);
        }
      else
        {
          image = GIMP_VALUES_GET_IMAGE (return_values, 1);
        }
    }

  if (image != NULL && metadata != NULL && flags != GIMP_METADATA_LOAD_NONE)
    _gimp_image_metadata_load_finish (image, NULL, metadata, flags);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = gimp_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _gimp_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the GimpExportProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
  g_clear_object (&metadata);
  gimp_value_array_unref (remaining);

  return return_values;
}

static GimpProcedureConfig *
gimp_load_procedure_create_config (GimpProcedure  *procedure,
                                   GParamSpec    **args,
                                   gint            n_args)
{
  if (n_args > ARG_OFFSET)
    {
      args   += ARG_OFFSET;
      n_args -= ARG_OFFSET;
    }
  else
    {
      args   = NULL;
      n_args = 0;
    }

  return GIMP_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}


/*  public functions  */

/**
 * gimp_load_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new load procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * Returns: (transfer full): a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_load_procedure_new (GimpPlugIn      *plug_in,
                         const gchar     *name,
                         GimpPDBProcType  proc_type,
                         GimpRunLoadFunc  run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  GimpLoadProcedure        *procedure;
  GimpLoadProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_LOAD_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  priv = gimp_load_procedure_get_instance_private (procedure);

  priv->run_func         = run_func;
  priv->run_data         = run_data;
  priv->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}

/**
 * gimp_load_procedure_set_handles_raw:
 * @procedure:   A load procedure object.
 * @handles_raw: The procedure's handles raw flag.
 *
 * Registers a load procedure as capable of handling raw digital camera loads.
 *
 * Note that you cannot call this function on [class@VectorLoadProcedure]
 * subclass objects.
 *
 * Since: 3.0
 **/
void
gimp_load_procedure_set_handles_raw (GimpLoadProcedure *procedure,
                                     gint               handles_raw)
{
  GimpLoadProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure));

  priv = gimp_load_procedure_get_instance_private (procedure);

  priv->handles_raw = handles_raw;
}

/**
 * gimp_load_procedure_get_handles_raw:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's 'handles raw' flag as set with
 * [method@GimpLoadProcedure.set_handles_raw].
 *
 * Returns: The procedure's 'handles raw' flag.
 *
 * Since: 3.0
 **/
gint
gimp_load_procedure_get_handles_raw (GimpLoadProcedure *procedure)
{
  GimpLoadProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure), 0);

  priv = gimp_load_procedure_get_instance_private (procedure);

  return priv->handles_raw;
}

/**
 * gimp_load_procedure_set_thumbnail_loader:
 * @procedure:      A load procedure object.
 * @thumbnail_proc: The name of the thumbnail load procedure.
 *
 * Associates a thumbnail loader with a file load procedure.
 *
 * Some file formats allow for embedded thumbnails, other file formats
 * contain a scalable image or provide the image data in different
 * resolutions. A file plug-in for such a format may register a
 * special procedure that allows GIMP to load a thumbnail preview of
 * the image. This procedure is then associated with the standard
 * load procedure using this function.
 *
 * Since: 3.0
 **/
void
gimp_load_procedure_set_thumbnail_loader (GimpLoadProcedure *procedure,
                                          const gchar       *thumbnail_proc)
{
  GimpLoadProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure));

  priv = gimp_load_procedure_get_instance_private (procedure);
  g_free (priv->thumbnail_proc);
  priv->thumbnail_proc = g_strdup (thumbnail_proc);
}

/**
 * gimp_load_procedure_get_thumbnail_loader:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's thumbnail loader procedure as set with
 * [method@GimpLoadProcedure.set_thumbnail_loader].
 *
 * Returns: The procedure's thumbnail loader procedure
 *
 * Since: 3.0
 **/
const gchar *
gimp_load_procedure_get_thumbnail_loader (GimpLoadProcedure *procedure)
{
  GimpLoadProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure), NULL);

  priv = gimp_load_procedure_get_instance_private (procedure);

  return priv->thumbnail_proc;
}
