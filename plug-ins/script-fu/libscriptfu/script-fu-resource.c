/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <libgimp/gimp.h>

#include "script-fu-types.h"
#include "script-fu-resource.h"

/* This encapsulates the implementation of the SFResourceType.
 * It knows how to manipulate a SFArg of that type.
 *
 * Separate because it is likely to change:
 *    when the old-style GUI of script-fu-interface is obsoleted.
 */


static gint32 sf_resource_arg_get_ID_from_context (SFArg *arg);

/* Called at registration time.
 * The name may be empty i.e. NULL, the special value "from context",
 * a name that matches a resource, or an invalid name of a resource.
 * We can't check for validity now because Gimp is not done initializing resources.
 */
void
sf_resource_arg_set_name_default (SFArg *arg, GType resource_type, gchar *name_of_default)
{
  /* Store the name and later put to ParamSpecResource.name_of_default.*/
  arg->default_value.sfa_resource.declared_name_of_default = g_strdup (name_of_default);
  arg->default_value.sfa_resource.resource_type = resource_type;

  /* Init current value to unknown. */
  sf_resource_arg_set (arg, -1);
}

/* Return the name stored at registration time. */
gchar *
sf_resource_arg_get_name_default (SFArg *arg)
{
  return arg->default_value.sfa_resource.declared_name_of_default;
}


/* Return a default value from the declared name of default.
 * Ensure the value is acceptable by a ResourceChooser widget.
 * Returns NULL when the name_of_default is not the name of a resource.
 * A NULL will make a ResourceChooser widget get from context.
 *
 * This does not increase the reference count.
 * ScriptFu does not keep a reference,
 * only passes the reference when declaring args to PDB procedure.
 */
GimpResource*
sf_resource_arg_get_default (SFArg *arg)
{
  GimpResource *result;

  result = gimp_resource_get_by_name (
    arg->default_value.sfa_resource.resource_type,
    sf_resource_arg_get_name_default (arg));

  g_debug ("%s name %s result %p", G_STRFUNC, sf_resource_arg_get_name_default (arg), result);
  return result;
}

/* Return the current value. */
GimpResource *sf_resource_arg_get_value          (SFArg *arg)
{
  return gimp_resource_get_by_id (arg->value.sfa_resource.history);
}

/* Sets the arg's internal value.
 *
 * FUTURE: Deprecated when script-fu-interface is deleted.
 */
void
sf_resource_arg_set (SFArg *arg, gint32 ID)
{
  g_debug ("%s ID: %d", G_STRFUNC, ID);
  arg->value.sfa_resource.history = ID;
}


/* Free allocated memory of an SFArg. */
void
sf_resource_arg_free (SFArg *arg)
{
  g_free (arg->default_value.sfa_resource.declared_name_of_default);
}

/* Reset the current value to the default value.
 * FUTURE: obsolete with script-fu-interface.
 */
void
sf_resource_arg_reset (SFArg *arg)
{
  /* Copy the whole struct.
   * This copies a gchar * but we don't need to free it.
   */
  arg->value.sfa_resource = arg->default_value.sfa_resource;
}

/* Return representation of the current value.
 * Representation in Scheme language: a literal numeric for the ID.
 * Transfer full, caller must free.
 */
gchar*
sf_resource_arg_get_repr (SFArg *arg)
{
  return g_strdup_printf ("%d", arg->value.sfa_resource.history);
}

/* Init the current value of an SFArg that is a resource
 * when it is not already set, i.e. -1.
 * Init to default value, either the declared named resource, or from context.
 *
 * !!! The current value is traditionally called "history"
 * because it persists across runs of the plugin.
 *
 * For old-style interface, where the data flow is different,
 * from the arg to the widget then back,
 * but not automatic via a bound property.
 * Must initialize inc case user does not touch a widget.
 *
 * Cannot be called at registration time.
 *
 * FUTURE: obsolete with script-fu-interface.
 */
void
sf_resource_arg_init_current_value (SFArg *arg)
{
  if (arg->value.sfa_resource.history < 1)
    {
      /* The ID has not flowed from a widget in a prior run of plugin. */
      GimpResource *default_resource = sf_resource_arg_get_default (arg);
      if (default_resource != NULL)
        sf_resource_arg_set (arg, gimp_resource_get_id (default_resource));
      else
        /* The author declared default is not valid. */
        sf_resource_arg_set (arg, sf_resource_arg_get_ID_from_context (arg));

    }
  /* Else the current value is positive int, a resource ID from prior run. */

  g_debug ("%s %i", G_STRFUNC, arg->value.sfa_resource.history);
}

/* FUTURE: obsolete with script-fu-interface. */
static gint32
sf_resource_arg_get_ID_from_context (SFArg *arg)
{
  GimpResource *resource = NULL;
  gint32        result_ID;
  GType         resource_type = arg->default_value.sfa_resource.resource_type;

  if (resource_type == GIMP_TYPE_BRUSH)
    resource = GIMP_RESOURCE (gimp_context_get_brush ());
  else if (resource_type == GIMP_TYPE_FONT)
    resource = GIMP_RESOURCE (gimp_context_get_font ());
  else if (resource_type == GIMP_TYPE_GRADIENT)
    resource = GIMP_RESOURCE (gimp_context_get_gradient ());
  else if (resource_type == GIMP_TYPE_PALETTE)
    resource = GIMP_RESOURCE (gimp_context_get_palette ());
  else if (resource_type == GIMP_TYPE_PATTERN)
    resource = GIMP_RESOURCE (gimp_context_get_pattern ());

  if (resource == NULL)
    {
      g_warning ("%s: Failed get resource from context", G_STRFUNC);
      result_ID = -1;
    }
  else
    {
      result_ID = gimp_resource_get_id (resource);
    }
  return result_ID;
}
