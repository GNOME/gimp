/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-icc-profile.c
 * Copyright (C) 2006  Sven Neumann <sven@gimp.org>
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

#include <glib-object.h>

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpparamspecs.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"

#include "plug-in-icc-profile.h"

#include "gimp-intl.h"


#define ICC_PROFILE_APPLY_RGB_PROC  "plug-in-icc-profile-apply-rgb"
#define ICC_PROFILE_INFO_PROC       "plug-in-icc-profile-info"


gboolean
plug_in_icc_profile_apply_rgb (GimpImage     *image,
                               GimpContext   *context,
                               GimpProgress  *progress,
                               GimpRunMode    run_mode,
                               GError       **error)
{
  Gimp          *gimp = image->gimp;
  GimpProcedure *procedure;

  if (gimp_image_base_type (image) == GIMP_GRAY)
    return FALSE;

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, ICC_PROFILE_APPLY_RGB_PROC);

  if (procedure &&
      procedure->num_args >= 2 &&
      GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0]) &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID (procedure->args[1]))
    {
      GValueArray       *return_vals;
      GimpPDBStatusType  status;

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb, context, progress,
                                            ICC_PROFILE_APPLY_RGB_PROC,
                                            GIMP_TYPE_INT32, run_mode,
                                            GIMP_TYPE_IMAGE_ID,
                                            gimp_image_get_ID (image),
                                            G_TYPE_NONE);

      status = g_value_get_enum (return_vals->values);
      g_value_array_free (return_vals);

      switch (status)
        {
        case GIMP_PDB_SUCCESS:
          return TRUE;

        default:
          g_set_error (error, 0, 0,
                       _("Error running '%s'"), ICC_PROFILE_APPLY_RGB_PROC);
          return FALSE;
        }
    }

  g_set_error (error, 0, 0,
               _("Plug-In missing (%s)"), ICC_PROFILE_APPLY_RGB_PROC);

  return FALSE;
}

gboolean
plug_in_icc_profile_info (GimpImage     *image,
                          GimpContext   *context,
                          GimpProgress  *progress,
                          gchar        **name,
                          gchar        **desc,
                          gchar        **info,
                          GError       **error)
{
  Gimp          *gimp = image->gimp;
  GimpProcedure *procedure;

  procedure = gimp_pdb_lookup_procedure (gimp->pdb, ICC_PROFILE_INFO_PROC);

  if (procedure &&
      procedure->num_args >= 2 &&
      GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0]) &&
      GIMP_IS_PARAM_SPEC_IMAGE_ID (procedure->args[1]))
    {
      GValueArray       *return_vals;
      GimpPDBStatusType  status;

      return_vals =
        gimp_pdb_execute_procedure_by_name (gimp->pdb, context, progress,
                                            ICC_PROFILE_INFO_PROC,
                                            GIMP_TYPE_INT32,
                                            GIMP_RUN_NONINTERACTIVE,
                                            GIMP_TYPE_IMAGE_ID,
                                            gimp_image_get_ID (image),
                                            G_TYPE_NONE);

      status = g_value_get_enum (return_vals->values);

      switch (status)
        {
        case GIMP_PDB_SUCCESS:
          if (name)
            {
              GValue *value = g_value_array_get_nth (return_vals, 1);

              *name = (G_VALUE_HOLDS_STRING (value) ?
                       g_value_dup_string (value) : NULL);
            }

          if (desc)
            {
              GValue *value = g_value_array_get_nth (return_vals, 2);

              *desc = (G_VALUE_HOLDS_STRING (value) ?
                       g_value_dup_string (value) : NULL);
            }

          if (info)
            {
              GValue *value = g_value_array_get_nth (return_vals, 3);

              *info = (G_VALUE_HOLDS_STRING (value) ?
                       g_value_dup_string (value) : NULL);
            }
          break;

        default:
          g_set_error (error, 0, 0,
                       _("Error running '%s'"), ICC_PROFILE_INFO_PROC);
          break;
        }

      g_value_array_free (return_vals);

      return (status == GIMP_PDB_SUCCESS);
    }

  g_set_error (error, 0, 0,
               _("Plug-In missing (%s)"), ICC_PROFILE_INFO_PROC);

  return FALSE;
}
