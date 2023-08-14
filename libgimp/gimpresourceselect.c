/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "gimp.h"

/* Data bounced back and forth to/from core and libgimp,
 * and here from the temp PDB procedure to the idle func.
 * But it is opaque to core, passed and returned unaltered.
 *
 * Not all fields are meaningful in each direction or transfer.
 * !!! We don't pass resource to core in this struct,
 * only from the temp callback to the idle func.
 *
 * Lifetime is as long as the remote dialog is open.
 * Closing the chooser dialog frees the adaption struct.
 */
typedef struct
{
  /* This portion is passed to and from idle. */
  guint                idle_id;
  gint                 resource_id;
  GType                resource_type;
  gboolean             closing;

  /* This portion is passed to and from core, and to idle. */
  gchar                       *temp_PDB_callback_name;
  GimpResourceChoosedCallback  callback;
  gpointer                     owner_data;
  GDestroyNotify               data_destroy;
} GimpResourceAdaption;


/*  local */

static void             gimp_resource_data_free (GimpResourceAdaption *adaption);

static GimpValueArray * gimp_temp_resource_run  (GimpProcedure        *procedure,
                                                 GimpProcedureConfig  *config,
                                                 gpointer              run_data);
static gboolean         gimp_temp_resource_idle (GimpResourceAdaption *adaption);


/*  public */

/* Annotation, which appears in the libgimp API doc.
 * Not a class, only functions.
 * The functions appear as class methods of GimpResource class.
 * Formerly, API had separate functions for each resource subclass.
 */

/**
 * SECTION: gimpresourceselect
 * @title: GimpResourceSelect
 * @short_description: A resource selection dialog.
 *
 * A resource selection dialog.
 *
 * An adapter and proxy between libgimp and core.
 * (see Adapter and Proxy patterns in programming literature.)
 *
 * Proxy: to a remote dialog in core.
 * Is a dialog, but the dialog is remote (another process.)
 * Remote dialog is a chooser dialog of subclass of GimpResource,
 * e.g. GimpBrush, GimpFont, etc.
 *
 * Adapter: gets a callback via PDB procedure from remote dialog
 * and shuffles parameters to call a owner's callback on libgimp side.
 *
 * Generic on type of GimpResource subclass.
 * That is, the type of GimpResource subclass is passed.
 *
 * Responsibilities:
 *
 *    - implement a proxy  to a chooser widget in core
 *
 * Collaborations:
 *
 *     - called by GimpResourceSelectButton to popup as a sibling widget
 *     - PDB procedures to/from core, which implements the remote dialog
 *       (from via PDB temp callback, to via PDB procs such as gimp_fonts_popup)
 *     - plugins implementing their own GUI
 **/

/* This was extracted from gimpbrushselect.c, gimpfontselect.c, etc.
 * and those were deleted.
 */

/* Functions that dispatch on resource type.
 *
 * For now, the design is that core callbacks pass
 * attributes of the resource (not just the resource.)
 * FUTURE: core will use the same signature for all remote dialogs,
 * regardless of resource type.
 * Then all this dispatching code can be deleted.
 */


/* Create args for temp PDB callback.
 * Must match signature that core hardcodes in a subclass of gimp_pdb_dialog.
 *
 * For example, see app/widgets/paletteselect.c
 *
 * When testing, the error "Unable to run callback... temp_proc... wrong type"
 * means a signature mismatch.
 *
 * Note the signature from core might be from an old design where
 * the core sent all data needed to draw the resource.
 * In the new design, libgimp gets the attributes of the resource
 * from core in a separate PDB proc call back to core.
 * While core uses the old design and libgimp uses the new design,
 * libgimp simply ignores the extra args (adapts the signature.)
 */
static void
create_callback_PDB_procedure_params (GimpProcedure *procedure,
                                      GType          resource_type)
{
  GIMP_PROC_ARG_STRING (procedure, "resource-name",
                        "Resource name",
                        "The resource name",
                        NULL,
                        G_PARAM_READWRITE);

  /* Create args for the extra, superfluous args that core is passing.*/
  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      /* No other args. */
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      GIMP_PROC_ARG_INT (procedure, "gradient-width",
                         "Gradient width",
                         "The gradient width",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_FLOAT_ARRAY (procedure, "gradient-data",
                                 "Gradient data",
                                 "The gradient data",
                                 G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      GIMP_PROC_ARG_DOUBLE (procedure, "opacity",
                            "Opacity",
                            NULL,
                            0.0, 100.0, 100.0,
                            G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "spacing",
                         "Spacing",
                         NULL,
                         -1, 1000, 20,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_ENUM (procedure, "paint-mode",
                          "Paint mode",
                          NULL,
                          GIMP_TYPE_LAYER_MODE,
                          GIMP_LAYER_MODE_NORMAL,
                          G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mask-width",
                         "Brush width",
                         NULL,
                         0, 10000, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mask-height",
                         "Brush height",
                         NULL,
                         0, 10000, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BYTES (procedure, "mask-data",
                           "Mask data",
                           "The brush mask data",
                           G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      GIMP_PROC_ARG_INT (procedure, "num-colors",
                         "Num colors",
                         "Number of colors",
                         0, G_MAXINT, 0,
                         G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      GIMP_PROC_ARG_INT (procedure, "mask-width",
                         "Mask width",
                         "Pattern width",
                         0, 10000, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mask-height",
                         "Mask height",
                         "Pattern height",
                         0, 10000, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_INT (procedure, "mask-bpp",
                         "Mask bpp",
                         "Pattern bytes per pixel",
                         0, 10000, 0,
                         G_PARAM_READWRITE);

      GIMP_PROC_ARG_BYTES (procedure, "mask-data",
                           "Mask data",
                           "The pattern mask data",
                           G_PARAM_READWRITE);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }

  GIMP_PROC_ARG_BOOLEAN (procedure, "closing",
                         "Closing",
                         "If the dialog was closing",
                         FALSE,
                         G_PARAM_READWRITE);
}

/* Open (create) a remote chooser dialog of resource.
 *
 * Dispatch on subclass of GimpResource.
 * Call a PDB procedure that communicates with core to create remote dialog.
 */
static gboolean
popup_remote_chooser (const gchar   *title,
                      GBytes        *parent_handle,
                      GimpResource  *resource,
                      gchar         *temp_PDB_callback_name,
                      GType          resource_type)
{
  gboolean result = FALSE;
  gchar   *resource_name;

  /* The PDB procedure still takes a name  */
  resource_name = gimp_resource_get_name (resource);

  if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      result = gimp_brushes_popup (temp_PDB_callback_name, title, resource_name, parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      result = gimp_fonts_popup (temp_PDB_callback_name, title, resource_name, parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      result = gimp_gradients_popup (temp_PDB_callback_name, title, resource_name, parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      result = gimp_palettes_popup (temp_PDB_callback_name, title, resource_name, parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      result = gimp_patterns_popup (temp_PDB_callback_name, title, resource_name, parent_handle);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }

  return result;
}

/*Does nothing, quietly, when the remote dialog is not open. */
static void
close_remote_chooser (gchar *temp_PDB_callback_name,
                      GType  resource_type)
{
  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      gimp_fonts_close_popup (temp_PDB_callback_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      gimp_gradients_close_popup (temp_PDB_callback_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      gimp_brushes_close_popup (temp_PDB_callback_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      gimp_palettes_close_popup (temp_PDB_callback_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      gimp_patterns_close_popup (temp_PDB_callback_name);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }
}

/**
 * gimp_resource_select_new:
 * @title:      Title of the resource selection dialog.
 * @resource:   The resource to set as the initial choice.
 * @resource_type: The type of the subclass of resource.
 * @callback: (scope notified): The callback function to call when a user chooses a resource.
 * @owner_data: (closure callback): The run_data given to @callback.
 * @data_destroy: (destroy owner_data): The destroy function for @owner_data.
 *
 * Invoke a resource chooser dialog which may call @callback with the chosen
 * resource and owner's @data.
 *
 * A proxy to a remote dialog in core, which knows the model i.e. installed resources.
 *
 * Generic on type of #GimpResource subclass passed in @resource_type.
 *
 * Returns: (transfer none): the name of a temporary PDB procedure. The
 *          string belongs to the resource selection dialog and will be
 *          freed automatically when the dialog is closed.
 **/
const gchar *
gimp_resource_select_new (const gchar                 *title,
                          GBytes                      *parent_handle,
                          GimpResource                *resource,
                          GType                        resource_type,
                          GimpResourceChoosedCallback  callback,
                          gpointer                     owner_data,
                          GDestroyNotify               data_destroy)
{
  GimpPlugIn    *plug_in = gimp_get_plug_in ();
  GimpProcedure *procedure;
  gchar         *temp_PDB_callback_name;
  GimpResourceAdaption  *adaption;

  g_debug ("%s", G_STRFUNC);

  g_return_val_if_fail (resource != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);
  g_return_val_if_fail (resource_type != 0, NULL);

  temp_PDB_callback_name = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  adaption = g_slice_new0 (GimpResourceAdaption);

  adaption->temp_PDB_callback_name  = temp_PDB_callback_name;
  adaption->callback                = callback;
  adaption->owner_data              = owner_data;
  adaption->data_destroy            = data_destroy;
  adaption->resource_type           = resource_type;

  /* !!! Only part of the adaption has been initialized. */

  procedure = gimp_procedure_new (plug_in,
                                  temp_PDB_callback_name,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_resource_run,
                                  adaption,
                                  (GDestroyNotify) gimp_resource_data_free);

  create_callback_PDB_procedure_params (procedure, resource_type);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);

  if (popup_remote_chooser (title, parent_handle, resource, temp_PDB_callback_name, resource_type))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_extension_enable (plug_in);

      return temp_PDB_callback_name;
    }
  else
    {
      g_warning ("Failed to open remote resource select dialog.");
      gimp_plug_in_remove_temp_procedure (plug_in, temp_PDB_callback_name);
      return NULL;
    }
}


void
gimp_resource_select_destroy (const gchar *temp_PDB_callback_name)
{
  GimpPlugIn *plug_in = gimp_get_plug_in ();

  g_return_if_fail (temp_PDB_callback_name != NULL);

  gimp_plug_in_remove_temp_procedure (plug_in, temp_PDB_callback_name);
}


/* Set currently selected resource in remote chooser.
 *
 * Calls a PDB procedure.
 *
 * Note core is still using string name of resource,
 * so pdb/groups/<foo>_select.pdb, <foo>_set_popup must have type string.
 */
void
gimp_resource_select_set (const gchar  *temp_pdb_callback,
                          GimpResource *resource,
                          GType         resource_type)
{
  gchar *resource_name;

  /* The remote setter is e.g. gimp_fonts_set_popup, a PDB procedure.
   * It still takes a name aka ID instead of a resource object.
   */
  resource_name = gimp_resource_get_name (resource);

  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      gimp_fonts_set_popup (temp_pdb_callback, resource_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      gimp_gradients_set_popup (temp_pdb_callback, resource_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      gimp_brushes_set_popup (temp_pdb_callback, resource_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      gimp_palettes_set_popup (temp_pdb_callback, resource_name);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      gimp_patterns_set_popup (temp_pdb_callback, resource_name);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }
}

/*  private functions  */

/* Free a GimpResourceAdaption struct.
 * A GimpResourceAdaption and this func are passed to a GimpProcedure
 * and this func is called back when the procedure is removed.
 *
 * This can be called for the exception: failed to open remote dialog.
 *
 * Each allocated field must be safely freed (not assuming it is valid pointer.)
 */
static void
gimp_resource_data_free (GimpResourceAdaption *adaption)
{
  if (adaption->idle_id)
    g_source_remove (adaption->idle_id);

  if (adaption->temp_PDB_callback_name)
    {
      close_remote_chooser (adaption->temp_PDB_callback_name,
                            adaption->resource_type);
      g_free (adaption->temp_PDB_callback_name);
    }

  if (adaption->data_destroy)
    adaption->data_destroy (adaption->owner_data);

  g_slice_free (GimpResourceAdaption, adaption);
}

/* Run func for the temporary PDB procedure.
 * Called when user chooses a resource in remote dialog.
 */
static GimpValueArray *
gimp_temp_resource_run (GimpProcedure       *procedure,
                        GimpProcedureConfig *config,
                        gpointer             run_data)
{
  GimpResourceAdaption *adaption = run_data;
  gchar                *resource_name;
  GimpResource         *resource;

  g_object_get (config,
                "resource-name", &resource_name,
                "closing",       &adaption->closing,
                NULL);

  resource = gimp_resource_get_by_name (adaption->resource_type,
                                        resource_name);

  adaption->resource_id = gimp_resource_get_id (resource);

  if (! adaption->idle_id)
    adaption->idle_id = g_idle_add ((GSourceFunc) gimp_temp_resource_idle,
                                    adaption);

  g_free (resource_name);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_temp_resource_idle (GimpResourceAdaption *adaption)
{
  adaption->idle_id = 0;

  if (adaption->callback)
    adaption->callback (gimp_resource_get_by_id (adaption->resource_id),
                        adaption->closing,
                        adaption->owner_data);

  adaption->resource_id = 0;

  if (adaption->closing)
    {
      gchar *temp_PDB_callback_name = adaption->temp_PDB_callback_name;

      adaption->temp_PDB_callback_name = NULL;
      gimp_resource_select_destroy (temp_PDB_callback_name);
      g_free (temp_PDB_callback_name);
    }

  return G_SOURCE_REMOVE;
}
