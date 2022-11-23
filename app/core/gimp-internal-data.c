/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2002 Spencer Kimball, Peter Mattis, and others
 *
 * ligma-internal-data.c
 * Copyright (C) 2017 Ell
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-gradients.h"
#include "ligma-internal-data.h"
#include "ligmadata.h"
#include "ligmadataloaderfactory.h"
#include "ligmaerror.h"
#include "ligmagradient-load.h"

#include "ligma-intl.h"


#define LIGMA_INTERNAL_DATA_DIRECTORY "internal-data"


typedef LigmaData * (* LigmaDataGetFunc) (Ligma *ligma);


typedef struct _LigmaInternalDataFile LigmaInternalDataFile;

struct _LigmaInternalDataFile
{
  const gchar      *name;
  LigmaDataGetFunc   get_func;
  LigmaDataLoadFunc  load_func;
};


/*  local function prototypes  */

static gboolean   ligma_internal_data_create_directory (GError                     **error);

static GFile    * ligma_internal_data_get_file         (const LigmaInternalDataFile  *data_file);

static gboolean   ligma_internal_data_load_data_file   (Ligma                        *ligma,
                                                       const LigmaInternalDataFile  *data_file,
                                                       GError                     **error);
static gboolean   ligma_internal_data_save_data_file   (Ligma                        *ligma,
                                                       const LigmaInternalDataFile  *data_file,
                                                       GError                     **error);
static gboolean   ligma_internal_data_delete_data_file (Ligma                        *ligma,
                                                       const LigmaInternalDataFile  *data_file,
                                                       GError                     **error);

/*  static variables  */

static const LigmaInternalDataFile internal_data_files[] =
{
  /* Custom gradient */
  {
    .name      = "custom" LIGMA_GRADIENT_FILE_EXTENSION,
    .get_func  = (LigmaDataGetFunc) ligma_gradients_get_custom,
    .load_func = ligma_gradient_load
  }
};


/*  public functions  */

gboolean
ligma_internal_data_load (Ligma    *ligma,
                         GError **error)
{
  gint i;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const LigmaInternalDataFile *data_file = &internal_data_files[i];

      if (! ligma_internal_data_load_data_file (ligma, data_file, error))
        return FALSE;
    }

  return TRUE;
}

gboolean
ligma_internal_data_save (Ligma    *ligma,
                         GError **error)
{
  gint i;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_internal_data_create_directory (error))
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const LigmaInternalDataFile *data_file = &internal_data_files[i];

      if (! ligma_internal_data_save_data_file (ligma, data_file, error))
        return FALSE;
    }

  return TRUE;
}

gboolean
ligma_internal_data_clear (Ligma    *ligma,
                          GError **error)
{
  gint i;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  for (i = 0; i < G_N_ELEMENTS (internal_data_files); i++)
    {
      const LigmaInternalDataFile *data_file = &internal_data_files[i];

      if (! ligma_internal_data_delete_data_file (ligma, data_file, error))
        return FALSE;
    }

  return TRUE;
}


/*  private functions  */

static gboolean
ligma_internal_data_create_directory (GError **error)
{
  GFile    *directory;
  GError   *my_error = NULL;
  gboolean  success;

  directory = ligma_directory_file (LIGMA_INTERNAL_DATA_DIRECTORY, NULL);

  success = g_file_make_directory (directory, NULL, &my_error);

  g_object_unref (directory);

  if (! success)
    {
      if (my_error->code == G_IO_ERROR_EXISTS)
        {
          g_clear_error (&my_error);
          success = TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
        }
    }

  return success;
}

static GFile *
ligma_internal_data_get_file (const LigmaInternalDataFile *data_file)
{
  return ligma_directory_file (LIGMA_INTERNAL_DATA_DIRECTORY,
                              data_file->name,
                              NULL);
}

static gboolean
ligma_internal_data_load_data_file (Ligma                        *ligma,
                                   const LigmaInternalDataFile  *data_file,
                                   GError                     **error)
{
  GFile        *file;
  GInputStream *input;
  LigmaData     *data;
  GList        *list;
  GError       *my_error = NULL;

  file = ligma_internal_data_get_file (data_file);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  input = G_INPUT_STREAM (g_file_read (file, NULL, &my_error));

  if (! input)
    {
      g_object_unref (file);

      if (my_error->code == G_IO_ERROR_NOT_FOUND)
        {
          g_clear_error (&my_error);
          return TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
          return FALSE;
        }
    }

  list = data_file->load_func (ligma->user_context, file, input, error);

  g_object_unref (input);
  g_object_unref (file);

  if (! list)
    return FALSE;

  data = data_file->get_func (ligma);

  ligma_data_copy (data, LIGMA_DATA (list->data));

  g_list_free_full (list, g_object_unref);

  return TRUE;
}

static gboolean
ligma_internal_data_save_data_file (Ligma                        *ligma,
                                   const LigmaInternalDataFile  *data_file,
                                   GError                     **error)
{
  GFile         *file;
  GOutputStream *output;
  LigmaData      *data;
  gboolean       success;

  file = ligma_internal_data_get_file (data_file);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  output = G_OUTPUT_STREAM (g_file_replace (file, NULL, FALSE,
                                            G_FILE_CREATE_NONE,
                                            NULL, error));

  if (! output)
    {
      g_object_unref (file);

      return FALSE;
    }

  data = data_file->get_func (ligma);

  /* we bypass ligma_data_save() and call the data's save() virtual function
   * directly, since ligma_data_save() is a nop for internal data.
   *
   * FIXME:  we save the data whether it's dirty or not, since it might not
   * exist on disk.  currently, we only use this for cheap data, such as
   * gradients, so this is not a big concern, but if we save more expensive
   * data in the future, we should fix this.
   */
  ligma_assert (LIGMA_DATA_GET_CLASS (data)->save);
  success = LIGMA_DATA_GET_CLASS (data)->save (data, output, error);

  if (success)
    {
      if (! g_output_stream_close (output, NULL, error))
        {
          g_prefix_error (error,
                          _("Error saving '%s': "),
                          ligma_file_get_utf8_name (file));
          success = FALSE;
        }
    }
  else
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_cancellable_cancel (cancellable);
      if (error && *error)
        {
          g_prefix_error (error,
                          _("Error saving '%s': "),
                          ligma_file_get_utf8_name (file));
        }
      else
        {
          g_set_error (error, LIGMA_DATA_ERROR, LIGMA_DATA_ERROR_WRITE,
                       _("Error saving '%s'"),
                       ligma_file_get_utf8_name (file));
        }
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }

  g_object_unref (output);
  g_object_unref (file);

  return success;
}

static gboolean
ligma_internal_data_delete_data_file (Ligma                        *ligma,
                                     const LigmaInternalDataFile  *data_file,
                                     GError                     **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  file = ligma_internal_data_get_file (data_file);

  if (ligma->be_verbose)
    g_print ("Deleting '%s'\n", ligma_file_get_utf8_name (file));

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}
