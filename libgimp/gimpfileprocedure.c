/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfileprocedure.c
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
#include "gimpfileprocedure.h"


struct _GimpFileProcedurePrivate
{
  gchar    *mime_types;
  gchar    *extensions;
  gchar    *prefixes;
  gchar    *magics;
  gint      priority;
  gboolean  handles_remote;
};


static void   gimp_file_procedure_constructed (GObject *object);
static void   gimp_file_procedure_finalize    (GObject *object);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpFileProcedure, gimp_file_procedure,
                                     GIMP_TYPE_PROCEDURE)

#define parent_class gimp_file_procedure_parent_class


static void
gimp_file_procedure_class_init (GimpFileProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_file_procedure_constructed;
  object_class->finalize     = gimp_file_procedure_finalize;
}

static void
gimp_file_procedure_init (GimpFileProcedure *procedure)
{
  procedure->priv = gimp_file_procedure_get_instance_private (procedure);
}

static void
gimp_file_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                      "Run mode",
                      "The run mode",
                      GIMP_TYPE_RUN_MODE,
                      GIMP_RUN_NONINTERACTIVE,
                      G_PARAM_READWRITE);
}

static void
gimp_file_procedure_finalize (GObject *object)
{
  GimpFileProcedure *procedure = GIMP_FILE_PROCEDURE (object);

  g_clear_pointer (&procedure->priv->mime_types, g_free);
  g_clear_pointer (&procedure->priv->extensions, g_free);
  g_clear_pointer (&procedure->priv->prefixes,   g_free);
  g_clear_pointer (&procedure->priv->magics,     g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/*  public functions  */

/**
 * gimp_file_procedure_set_mime_types:
 * @procedure: A #GimpFileProcedure.
 * @mime_types: A comma-separated list of MIME types, such as "image/jpeg".
 *
 * Associates MIME types with a file handler procedure.
 *
 * Registers MIME types for a file handler procedure. This allows GIMP
 * to determine the MIME type of the file opened or saved using this
 * procedure. It is recommended that only one MIME type is registered
 * per file procedure; when registering more than one MIME type, GIMP
 * will associate the first one with files opened or saved with this
 * procedure.
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_mime_types (GimpFileProcedure *procedure,
                                    const gchar       *mime_types)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  g_free (procedure->priv->mime_types);
  procedure->priv->mime_types = g_strdup (mime_types);
}

/**
 * gimp_file_procedure_get_mime_types:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's mime-type as set with
 *          gimp_file_procedure_set_mime_types().
 *
 * Since: 3.0
 **/
const gchar *
gimp_file_procedure_get_mime_types (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), NULL);

  return procedure->priv->mime_types;
}

/**
 * gimp_file_procedure_set_extensions:
 * @procedure:  A #GimpFileProcedure.
 * @extensions: A comma separated list of extensions this procedure can
 *              handle (i.e. "jpg,jpeg").
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_extensions (GimpFileProcedure *procedure,
                                    const gchar       *extensions)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  g_free (procedure->priv->extensions);
  procedure->priv->extensions = g_strdup (extensions);
}

/**
 * gimp_file_procedure_get_extensions:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's extensions as set with
 *          gimp_file_procedure_set_extensions().
 *
 * Since: 3.0
 **/
const gchar *
gimp_file_procedure_get_extensions (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), NULL);

  return procedure->priv->extensions;
}

/**
 * gimp_file_procedure_set_prefixes:
 * @procedure: A #GimpFileProcedure.
 * @prefixes:  A comma separated list of prefixes this procedure can
 *             handle (i.e. "http:,ftp:").
 *
 * It should almost never be necessary to register prefixes with file
 * procedures, because most sorts of URIs should be handled by GIO.
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_prefixes (GimpFileProcedure *procedure,
                                  const gchar       *prefixes)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  g_free (procedure->priv->prefixes);
  procedure->priv->prefixes = g_strdup (prefixes);
}

/**
 * gimp_file_procedure_get_prefixes:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's prefixes as set with
 *          gimp_file_procedure_set_prefixes().
 *
 * Since: 3.0
 **/
const gchar *
gimp_file_procedure_get_prefixes (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), NULL);

  return procedure->priv->prefixes;
}

/**
 * gimp_file_procedure_set_magics:
 * @procedure: A #GimpFileProcedure.
 * @magics:    A comma separated list of magic file information this procedure
 *             can handle (i.e. "0,string,GIF").
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_magics (GimpFileProcedure *procedure,
                                const gchar       *magics)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  g_free (procedure->priv->magics);
  procedure->priv->magics = g_strdup (magics);
}

/**
 * gimp_file_procedure_get_magics:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's magics as set with
 *          gimp_file_procedure_set_magics().
 *
 * Since: 3.0
 **/
const gchar *
gimp_file_procedure_get_magics (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), NULL);

  return procedure->priv->magics;
}

/**
 * gimp_file_procedure_set_priority:
 * @procedure: A #GimpFileProcedure.
 * @priority: The procedure's priority.
 *
 * Sets the priority of a file handler procedure. When more than one
 * procedure matches a given file, the procedure with the lowest
 * priority is used; if more than one procedure has the lowest
 * priority, it is unspecified which one of them is used. The default
 * priority for file handler procedures is 0.
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_priority (GimpFileProcedure *procedure,
                                  gint               priority)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  procedure->priv->priority = priority;
}

/**
 * gimp_file_procedure_get_priority:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's priority as set with
 *          gimp_file_procedure_set_priority().
 *
 * Since: 3.0
 **/
gint
gimp_file_procedure_get_priority (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), 0);

  return procedure->priv->priority;
}

/**
 * gimp_file_procedure_set_handles_remote:
 * @procedure:      A #GimpFileProcedure.
 * @handles_remote: The procedure's 'handles remote' flag.
 *
 * Registers a file procedure as capable of handling arbitrary remote
 * URIs via GIO.
 *
 * When @handles_remote is set to %TRUE, the procedure will get a
 * #GFile passed that can point to a remote file.
 *
 * When @handles_remote is set to %FALSE, the procedure will get a
 * local #GFile passed and can use g_file_get_path() to get to a
 * filename that can be used with whatever non-GIO means of dealing
 * with the file.
 *
 * Since: 3.0
 **/
void
gimp_file_procedure_set_handles_remote (GimpFileProcedure *procedure,
                                        gint               handles_remote)
{
  g_return_if_fail (GIMP_IS_FILE_PROCEDURE (procedure));

  procedure->priv->handles_remote = handles_remote;
}

/**
 * gimp_file_procedure_get_handles_remote:
 * @procedure: A #GimpFileProcedure.
 *
 * Returns: The procedure's 'handles remote' flag as set with
 *          gimp_file_procedure_set_handles_remote().
 *
 * Since: 3.0
 **/
gint
gimp_file_procedure_get_handles_remote (GimpFileProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_FILE_PROCEDURE (procedure), 0);

  return procedure->priv->handles_remote;
}
