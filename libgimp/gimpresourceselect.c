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
#include "gimpresourceselect-private.h"

typedef struct
{
  GType                        resource_type;
  GimpResourceChoosedCallback  callback;
  gpointer                     owner_data;
  GDestroyNotify               data_destroy;
} GimpResourceAdaption;


/*  local */

static void             gimp_resource_data_free               (GimpResourceAdaption *adaption);

static GimpValueArray * gimp_temp_resource_run                (GimpProcedure        *procedure,
                                                               GimpProcedureConfig  *config,
                                                               gpointer              run_data);
static gboolean         gimp_resource_select_remove_after_run (const gchar         *procedure_name);


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
  gimp_procedure_add_resource_argument (procedure, "resource",
                                        "Resource", "The resource",
                                        TRUE,
                                        NULL, /* no default*/
                                        G_PARAM_READWRITE);

  /* Create args for the extra, superfluous args that core is passing.*/
  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      /* No other args. */
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      gimp_procedure_add_double_array_argument (procedure, "gradient-data",
                                                "Gradient data",
                                                "The gradient data",
                                                G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      gimp_procedure_add_int_argument (procedure, "mask-width",
                                       "Brush width",
                                       NULL,
                                       0, 10000, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "mask-height",
                                       "Brush height",
                                       NULL,
                                       0, 10000, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_bytes_argument (procedure, "mask-data",
                                         "Mask data",
                                         "The brush mask data",
                                         G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      gimp_procedure_add_int_argument (procedure, "num-colors",
                                       "Num colors",
                                       "Number of colors",
                                       0, G_MAXINT, 0,
                                       G_PARAM_READWRITE);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      gimp_procedure_add_int_argument (procedure, "mask-width",
                                       "Mask width",
                                       "Pattern width",
                                       0, 10000, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "mask-height",
                                       "Mask height",
                                       "Pattern height",
                                       0, 10000, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_int_argument (procedure, "mask-bpp",
                                       "Mask bpp",
                                       "Pattern bytes per pixel",
                                       0, 10000, 0,
                                       G_PARAM_READWRITE);

      gimp_procedure_add_bytes_argument (procedure, "mask-data",
                                         "Mask data",
                                         "The pattern mask data",
                                         G_PARAM_READWRITE);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }

  gimp_procedure_add_boolean_argument (procedure, "closing",
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
                      const gchar   *temp_PDB_callback_name,
                      GType          resource_type)
{
  gboolean result = FALSE;

  if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    {
      result = gimp_brushes_popup (temp_PDB_callback_name, title, GIMP_BRUSH (resource), parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    {
      result = gimp_fonts_popup (temp_PDB_callback_name, title, GIMP_FONT (resource), parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    {
      result = gimp_gradients_popup (temp_PDB_callback_name, title, GIMP_GRADIENT (resource), parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    {
      result = gimp_palettes_popup (temp_PDB_callback_name, title, GIMP_PALETTE (resource), parent_handle);
    }
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    {
      result = gimp_patterns_popup (temp_PDB_callback_name, title, GIMP_PATTERN (resource), parent_handle);
    }
  else
    {
      g_warning ("%s: unhandled resource type", G_STRFUNC);
    }

  return result;
}

/**
 * gimp_resource_select_new:
 * @title:                          Title of the resource selection dialog.
 * @resource:                       The resource to set as the initial choice.
 * @resource_type:                  The type of the subclass of [class@Resource].
 * @callback: (scope notified):     The callback function to call when the user chooses a resource.
 * @owner_data: (closure callback): The run_data given to @callback.
 * @data_destroy: (destroy owner_data): The destroy function for @owner_data.
 *
 * Invoke a resource chooser dialog which may call @callback with the chosen
 * @resource and @owner_data.
 *
 * A proxy to a remote dialog in core, which knows the installed resources.
 *
 * Note: this function is private and must not be used in plug-ins or
 * considered part of the API.
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

  g_return_val_if_fail (resource != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);
  g_return_val_if_fail (g_type_is_a (resource_type, GIMP_TYPE_RESOURCE), NULL);

  temp_PDB_callback_name = gimp_pdb_temp_procedure_name (gimp_get_pdb ());

  adaption = g_slice_new0 (GimpResourceAdaption);

  adaption->callback      = callback;
  adaption->owner_data    = owner_data;
  adaption->data_destroy  = data_destroy;
  adaption->resource_type = resource_type;

  procedure = gimp_procedure_new (plug_in,
                                  temp_PDB_callback_name,
                                  GIMP_PDB_PROC_TYPE_TEMPORARY,
                                  gimp_temp_resource_run,
                                  adaption,
                                  (GDestroyNotify) gimp_resource_data_free);

  create_callback_PDB_procedure_params (procedure, resource_type);

  gimp_plug_in_add_temp_procedure (plug_in, procedure);
  g_object_unref (procedure);
  g_free (temp_PDB_callback_name);

  if (popup_remote_chooser (title, parent_handle, resource, gimp_procedure_get_name (procedure), resource_type))
    {
      /* Allow callbacks to be watched */
      gimp_plug_in_persistent_enable (plug_in);

      return gimp_procedure_get_name (procedure);
    }
  else
    {
      g_warning ("Failed to open remote resource select dialog.");
      gimp_plug_in_remove_temp_procedure (plug_in, temp_PDB_callback_name);
      return NULL;
    }
}


/* gimp_resource_select_set:
 * @callback_name: a callback name as returned by [func@resource_select_new].
 * @resource:      a [class@Resource] of type @resource_type.
 *
 * Set currently selected resource in remote chooser.
 *
 * Note: this function is private and must not be used in plug-ins or
 * considered part of the API.
 */
void
gimp_resource_select_set (const gchar  *callback_name,
                          GimpResource *resource)
{
  GType resource_type;

  g_return_if_fail (resource != NULL);

  resource_type = G_TYPE_FROM_INSTANCE (resource);
  g_return_if_fail (g_type_is_a (resource_type, GIMP_TYPE_RESOURCE));

  if (g_type_is_a (resource_type, GIMP_TYPE_FONT))
    gimp_fonts_set_popup (callback_name, GIMP_FONT (resource));
  else if (g_type_is_a (resource_type, GIMP_TYPE_GRADIENT))
    gimp_gradients_set_popup (callback_name, GIMP_GRADIENT (resource));
  else if (g_type_is_a (resource_type, GIMP_TYPE_BRUSH))
    gimp_brushes_set_popup (callback_name, GIMP_BRUSH (resource));
  else if (g_type_is_a (resource_type, GIMP_TYPE_PALETTE))
    gimp_palettes_set_popup (callback_name, GIMP_PALETTE (resource));
  else if (g_type_is_a (resource_type, GIMP_TYPE_PATTERN))
    gimp_patterns_set_popup (callback_name, GIMP_PATTERN (resource));
  else
    g_return_if_reached ();
}

/*  private functions  */

static void
gimp_resource_data_free (GimpResourceAdaption *adaption)
{
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
  GimpResource         *resource;
  gboolean              closing;

  g_object_get (config,
                "resource", &resource,
                "closing",  &closing,
                NULL);

  if (adaption->callback)
    adaption->callback (resource, closing,
                        adaption->owner_data);

  if (closing)
    g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                     (GSourceFunc) gimp_resource_select_remove_after_run,
                     g_strdup (gimp_procedure_get_name (procedure)),
                     g_free);

  g_object_unref (resource);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}

static gboolean
gimp_resource_select_remove_after_run (const gchar *procedure_name)
{
  gimp_plug_in_remove_temp_procedure (gimp_get_plug_in (), procedure_name);

  return G_SOURCE_REMOVE;
}
