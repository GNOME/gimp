/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-color-profile.c
 * Copyright (C) 2015-2018 Michael Natterer <mitch@ligma.org>
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

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmaconfig/ligmaconfig.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "config/ligmadialogconfig.h"

#include "gegl/ligma-babl.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligma.h"
#include "ligmacontext.h"
#include "ligmaerror.h"
#include "ligmalayer.h"
#include "ligmaimage.h"
#include "ligmaimage-color-profile.h"
#include "ligmaimage-colormap.h"
#include "ligmaimage-private.h"
#include "ligmaimage-undo.h"
#include "ligmaimage-undo-push.h"
#include "ligmaobjectqueue.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_image_convert_profile_layers   (LigmaImage                *image,
                                                   LigmaColorProfile         *src_profile,
                                                   LigmaColorProfile         *dest_profile,
                                                   LigmaColorRenderingIntent  intent,
                                                   gboolean                  bpc,
                                                   LigmaProgress             *progress);
static void   ligma_image_convert_profile_colormap (LigmaImage                *image,
                                                   LigmaColorProfile         *src_profile,
                                                   LigmaColorProfile         *dest_profile,
                                                   LigmaColorRenderingIntent  intent,
                                                   gboolean                  bpc,
                                                   LigmaProgress             *progress);

static void   ligma_image_fix_layer_format_spaces  (LigmaImage                *image,
                                                   LigmaProgress             *progress);

static void   ligma_image_create_color_transforms  (LigmaImage                *image);


/*  public functions  */

gboolean
ligma_image_get_use_srgb_profile (LigmaImage *image,
                                 gboolean  *hidden_profile)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (hidden_profile)
    *hidden_profile = (private->hidden_profile != NULL);

  return private->color_profile == NULL;
}

void
ligma_image_set_use_srgb_profile (LigmaImage *image,
                                 gboolean   use_srgb)
{
  LigmaImagePrivate *private;
  gboolean          old_use_srgb;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  old_use_srgb = (private->color_profile == NULL);

  use_srgb = use_srgb ? TRUE : FALSE;

  if (use_srgb == old_use_srgb)
    return;

  if (use_srgb)
    {
      LigmaColorProfile *profile = ligma_image_get_color_profile (image);

      if (profile)
        {
          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                                       _("Enable 'Use sRGB Profile'"));

          g_object_ref (profile);
          ligma_image_assign_color_profile (image, NULL, NULL, NULL);
          _ligma_image_set_hidden_profile (image, profile, TRUE);
          g_object_unref (profile);

          ligma_image_undo_group_end (image);
        }
    }
  else
    {
      LigmaColorProfile *hidden = _ligma_image_get_hidden_profile (image);

      if (hidden)
        {
          ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                                       _("Disable 'Use sRGB Profile'"));

          g_object_ref (hidden);
          ligma_image_assign_color_profile (image, hidden, NULL, NULL);
          g_object_unref (hidden);

          ligma_image_undo_group_end (image);
        }
    }
}

LigmaColorProfile *
_ligma_image_get_hidden_profile (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return private->hidden_profile;
}

void
_ligma_image_set_hidden_profile (LigmaImage        *image,
                                LigmaColorProfile *profile,
                                gboolean          push_undo)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (profile != private->hidden_profile)
    {
      if (push_undo)
        ligma_image_undo_push_image_hidden_profile (image, NULL);

      g_set_object (&private->hidden_profile, profile);
    }
}

gboolean
ligma_image_validate_icc_parasite (LigmaImage           *image,
                                  const LigmaParasite  *icc_parasite,
                                  const gchar         *profile_type,
                                  gboolean            *is_builtin,
                                  GError             **error)
{
  const guint8 *data;
  guint32       data_size;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (icc_parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (strcmp (ligma_parasite_get_name (icc_parasite),
              profile_type) != 0)
    {
      gchar *invalid_parasite_name;

      invalid_parasite_name =
        g_strdup_printf (_("ICC profile validation failed: "
                           "Parasite's name is not '%s'"),
                           profile_type);

      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           invalid_parasite_name);

      g_free (invalid_parasite_name);
      return FALSE;
    }

  if (ligma_parasite_get_flags (icc_parasite) != (LIGMA_PARASITE_PERSISTENT |
                                                 LIGMA_PARASITE_UNDOABLE))
    {
      g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                           _("ICC profile validation failed: "
                             "Parasite's flags are not (PERSISTENT | UNDOABLE)"));
      return FALSE;
    }

  data = ligma_parasite_get_data (icc_parasite, &data_size);

  return ligma_image_validate_icc_profile (image, data, data_size,
                                          profile_type, is_builtin, error);
}

const LigmaParasite *
ligma_image_get_icc_parasite (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return ligma_image_parasite_find (image, LIGMA_ICC_PROFILE_PARASITE_NAME);
}

void
ligma_image_set_icc_parasite (LigmaImage          *image,
                             const LigmaParasite *icc_parasite,
                             const gchar        *profile_type)
{
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  if (icc_parasite)
    {
      g_return_if_fail (ligma_image_validate_icc_parasite (image, icc_parasite,
                                                          profile_type,
                                                          NULL, NULL) == TRUE);

      ligma_image_parasite_attach (image, icc_parasite, TRUE);
    }
  else
    {
      ligma_image_parasite_detach (image, profile_type, TRUE);
    }
}

gboolean
ligma_image_validate_icc_profile (LigmaImage     *image,
                                 const guint8  *data,
                                 gsize          length,
                                 const gchar   *profile_type,
                                 gboolean      *is_builtin,
                                 GError       **error)
{
  LigmaColorProfile *profile;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (data != NULL || length == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  profile = ligma_color_profile_new_from_icc_profile (data, length, error);

  if (! profile)
    {
      if (g_strcmp0 (profile_type, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0)
        g_prefix_error (error, _("ICC profile validation failed: "));
      else if (g_strcmp0 (profile_type, LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME) == 0)
        g_prefix_error (error, _("Simulation ICC profile validation failed: "));

      return FALSE;
    }

  if (g_strcmp0 (profile_type, LIGMA_ICC_PROFILE_PARASITE_NAME) == 0)
    {
      if (! ligma_image_validate_color_profile (image, profile, is_builtin, error))
        {
          g_object_unref (profile);
          return FALSE;
        }
    }
  else
    {
      if (is_builtin)
        *is_builtin = FALSE;
    }

  g_object_unref (profile);

  return TRUE;
}

const guint8 *
ligma_image_get_icc_profile (LigmaImage *image,
                            gsize     *length)
{
  const LigmaParasite *parasite;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  parasite = ligma_image_parasite_find (image, LIGMA_ICC_PROFILE_PARASITE_NAME);

  if (parasite)
    {
      const guint8 *data;
      guint32       data_size;

      data = ligma_parasite_get_data (parasite, &data_size);

      if (length)
        *length = (gsize) data_size;

      return data;
    }

  if (length)
    *length = 0;

  return NULL;
}

gboolean
ligma_image_set_icc_profile (LigmaImage     *image,
                            const guint8  *data,
                            gsize          length,
                            const gchar   *profile_type,
                            GError       **error)
{
  LigmaParasite *parasite = NULL;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (data == NULL || length != 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (data)
    {
      gboolean is_builtin;

      parasite = ligma_parasite_new (profile_type,
                                    LIGMA_PARASITE_PERSISTENT |
                                    LIGMA_PARASITE_UNDOABLE,
                                    length, data);

      if (! ligma_image_validate_icc_parasite (image, parasite, profile_type,
                                              &is_builtin, error))
        {
          ligma_parasite_free (parasite);
          return FALSE;
        }

      /*  don't tag the image with the built-in profile  */
      if (is_builtin)
        {
          ligma_parasite_free (parasite);
          parasite = NULL;
        }
    }

  ligma_image_set_icc_parasite (image, parasite, profile_type);

  if (parasite)
    ligma_parasite_free (parasite);

  return TRUE;
}

gboolean
ligma_image_validate_color_profile (LigmaImage        *image,
                                   LigmaColorProfile *profile,
                                   gboolean         *is_builtin,
                                   GError          **error)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  format = ligma_image_get_layer_format (image, TRUE);

  return ligma_image_validate_color_profile_by_format (format,
                                                      profile, is_builtin,
                                                      error);
}

LigmaColorProfile *
ligma_image_get_color_profile (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->color_profile;
}

gboolean
ligma_image_set_color_profile (LigmaImage         *image,
                              LigmaColorProfile  *profile,
                              GError           **error)
{
  const guint8 *data   = NULL;
  gsize         length = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile),
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (profile)
    data = ligma_color_profile_get_icc_profile (profile, &length);

  return ligma_image_set_icc_profile (image, data, length,
                                     LIGMA_ICC_PROFILE_PARASITE_NAME,
                                     error);
}

LigmaColorProfile *
ligma_image_get_simulation_profile (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  return LIGMA_IMAGE_GET_PRIVATE (image)->simulation_profile;
}

gboolean
ligma_image_set_simulation_profile (LigmaImage         *image,
                                   LigmaColorProfile  *profile)
{
  LigmaImagePrivate *private;
  const guint8     *data   = NULL;
  gsize             length = 0;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (profile == NULL || LIGMA_IS_COLOR_PROFILE (profile),
                        FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (profile != private->simulation_profile)
    {
      g_set_object (&private->simulation_profile, profile);
      ligma_color_managed_simulation_profile_changed (LIGMA_COLOR_MANAGED (image));
    }

  if (profile)
    data = ligma_color_profile_get_icc_profile (profile, &length);

  return ligma_image_set_icc_profile (image, data, length,
                                     LIGMA_SIMULATION_ICC_PROFILE_PARASITE_NAME,
                                     NULL);
}

LigmaColorRenderingIntent
ligma_image_get_simulation_intent (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image),
                        LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  return LIGMA_IMAGE_GET_PRIVATE (image)->simulation_intent;
}

void
ligma_image_set_simulation_intent (LigmaImage               *image,
                                  LigmaColorRenderingIntent intent)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (intent != private->simulation_intent)
    {
      LigmaParasite *parasite;
      gchar         i;

      private->simulation_intent = intent;
      ligma_color_managed_simulation_intent_changed (LIGMA_COLOR_MANAGED (image));

      i = (gchar) intent;
      parasite = ligma_parasite_new ("image-simulation-intent",
                                    LIGMA_PARASITE_PERSISTENT,
                                    1, (gpointer) &i);

      ligma_image_parasite_attach (image, parasite, FALSE);
      ligma_parasite_free (parasite);
    }
}

gboolean
ligma_image_get_simulation_bpc (LigmaImage *image)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  return LIGMA_IMAGE_GET_PRIVATE (image)->simulation_bpc;
}

void
ligma_image_set_simulation_bpc (LigmaImage *image,
                               gboolean   bpc)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (bpc != private->simulation_bpc)
    {
      LigmaParasite *parasite;
      gchar         i;

      private->simulation_bpc = bpc;
      ligma_color_managed_simulation_bpc_changed (LIGMA_COLOR_MANAGED (image));

      i = (gchar) bpc;
      parasite = ligma_parasite_new ("image-simulation-bpc",
                                    LIGMA_PARASITE_PERSISTENT,
                                    1, (gpointer) &i);

      ligma_image_parasite_attach (image, parasite, FALSE);
      ligma_parasite_free (parasite);
    }
}

gboolean
ligma_image_validate_color_profile_by_format (const Babl         *format,
                                             LigmaColorProfile   *profile,
                                             gboolean           *is_builtin,
                                             GError            **error)
{
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (LIGMA_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (ligma_babl_format_get_base_type (format) == LIGMA_GRAY)
    {
      if (! ligma_color_profile_is_gray (profile))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("ICC profile validation failed: "
                                 "Color profile is not for grayscale color space"));
          return FALSE;
        }
    }
  else
    {
      if (! ligma_color_profile_is_rgb (profile))
        {
          g_set_error_literal (error, LIGMA_ERROR, LIGMA_FAILED,
                               _("ICC profile validation failed: "
                                 "Color profile is not for RGB color space"));
          return FALSE;
        }
    }

  if (is_builtin)
    {
      LigmaColorProfile *builtin =
        ligma_babl_get_builtin_color_profile (ligma_babl_format_get_base_type (format),
                                             ligma_babl_format_get_trc (format));

      *is_builtin = ligma_color_profile_is_equal (profile, builtin);
    }

  return TRUE;
}

LigmaColorProfile *
ligma_image_get_builtin_color_profile (LigmaImage *image)
{
  const Babl *format;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  format = ligma_image_get_layer_format (image, FALSE);

  return ligma_babl_get_builtin_color_profile (ligma_babl_format_get_base_type (format),
                                              ligma_babl_format_get_trc (format));
}

gboolean
ligma_image_assign_color_profile (LigmaImage         *image,
                                 LigmaColorProfile  *dest_profile,
                                 LigmaProgress      *progress,
                                 GError           **error)
{
  LigmaColorProfile *src_profile;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (dest_profile == NULL ||
                        LIGMA_IS_COLOR_PROFILE (dest_profile), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (dest_profile &&
      ! ligma_image_validate_color_profile (image, dest_profile, NULL, error))
    return FALSE;

  src_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

  if (src_profile == dest_profile ||
      (src_profile && dest_profile &&
       ligma_color_profile_is_equal (src_profile, dest_profile)))
    return TRUE;

  if (progress)
    ligma_progress_start (progress, FALSE,
                         dest_profile ?
                         _("Assigning color profile") :
                         _("Discarding color profile"));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                               dest_profile ?
                               _("Assign color profile") :
                               _("Discard color profile"));

  _ligma_image_set_hidden_profile (image, NULL, TRUE);

  ligma_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  ligma_image_parasite_detach (image, "icc-profile-name", TRUE);

  if (ligma_image_get_base_type (image) == LIGMA_INDEXED)
    ligma_image_colormap_update_formats (image);

  ligma_image_fix_layer_format_spaces (image, progress);

  ligma_image_undo_group_end (image);

  return TRUE;
}

gboolean
ligma_image_convert_color_profile (LigmaImage                *image,
                                  LigmaColorProfile         *dest_profile,
                                  LigmaColorRenderingIntent  intent,
                                  gboolean                  bpc,
                                  LigmaProgress             *progress,
                                  GError                  **error)
{
  LigmaColorProfile *src_profile;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (LIGMA_IS_COLOR_PROFILE (dest_profile), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! ligma_image_validate_color_profile (image, dest_profile, NULL, error))
    return FALSE;

  src_profile = ligma_color_managed_get_color_profile (LIGMA_COLOR_MANAGED (image));

  if (ligma_color_profile_is_equal (src_profile, dest_profile))
    return TRUE;

  if (progress)
    ligma_progress_start (progress, FALSE,
                         _("Converting from '%s' to '%s'"),
                         ligma_color_profile_get_label (src_profile),
                         ligma_color_profile_get_label (dest_profile));

  ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_IMAGE_CONVERT,
                               _("Color profile conversion"));

  /* retain src_profile across ligma_image_set_color_profile() */
  g_object_ref (src_profile);

  _ligma_image_set_hidden_profile (image, NULL, TRUE);

  ligma_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  ligma_image_parasite_detach (image, "icc-profile-name", TRUE);

  switch (ligma_image_get_base_type (image))
    {
    case LIGMA_RGB:
    case LIGMA_GRAY:
      ligma_image_convert_profile_layers (image,
                                         src_profile, dest_profile,
                                         intent, bpc,
                                         progress);
      break;

    case LIGMA_INDEXED:
      ligma_image_convert_profile_colormap (image,
                                           src_profile, dest_profile,
                                           intent, bpc,
                                           progress);
      ligma_image_fix_layer_format_spaces (image, progress);
      break;
    }

  g_object_unref (src_profile);

  ligma_image_undo_group_end (image);

  if (progress)
    ligma_progress_end (progress);

  return TRUE;
}

void
ligma_image_import_color_profile (LigmaImage    *image,
                                 LigmaContext  *context,
                                 LigmaProgress *progress,
                                 gboolean      interactive)
{
  LigmaColorProfile *profile = NULL;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  if ((profile = ligma_image_get_color_profile (image)))
    {
      LigmaColorProfilePolicy     policy;
      LigmaColorProfile          *dest_profile = NULL;
      LigmaColorProfile          *pref_profile = NULL;
      LigmaColorRenderingIntent   intent;
      gboolean                   bpc;

      policy = LIGMA_DIALOG_CONFIG (image->ligma->config)->color_profile_policy;
      intent = LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
      bpc    = TRUE;

      if (ligma_image_get_base_type (image) == LIGMA_GRAY)
        pref_profile = ligma_color_config_get_gray_color_profile (image->ligma->config->color_management, NULL);
      else
        pref_profile = ligma_color_config_get_rgb_color_profile (image->ligma->config->color_management, NULL);

      if (policy == LIGMA_COLOR_PROFILE_POLICY_ASK)
        {
          if (ligma_color_profile_is_equal (profile, ligma_image_get_builtin_color_profile (image)) ||
              (pref_profile && ligma_color_profile_is_equal (pref_profile, profile)))
            {
              /* If already using the default profile or the preferred
               * profile for the image type, no need to ask. Just keep
               * the profile.
               */
              policy = LIGMA_COLOR_PROFILE_POLICY_KEEP;
            }
          else if (interactive)
            {
              gboolean dont_ask = FALSE;

              policy = ligma_query_profile_policy (image->ligma, image, context,
                                                  &dest_profile,
                                                  &intent, &bpc,
                                                  &dont_ask);

              if (dont_ask)
                {
                  g_object_set (G_OBJECT (image->ligma->config),
                                "color-profile-policy", policy,
                                NULL);
                }
            }
          else
            {
              policy = LIGMA_COLOR_PROFILE_POLICY_KEEP;
            }
        }

      if (policy == LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED ||
          policy == LIGMA_COLOR_PROFILE_POLICY_CONVERT_BUILTIN)
        {
          if (! dest_profile)
            {
              if (policy == LIGMA_COLOR_PROFILE_POLICY_CONVERT_PREFERRED)
                {
                  if (ligma_image_get_base_type (image) == LIGMA_GRAY)
                    dest_profile = ligma_color_config_get_gray_color_profile (image->ligma->config->color_management, NULL);
                  else
                    dest_profile = ligma_color_config_get_rgb_color_profile (image->ligma->config->color_management, NULL);
                }

              if (! dest_profile)
                {
                  /* Built-in policy or no preferred profile set. */
                  dest_profile = ligma_image_get_builtin_color_profile (image);
                  g_object_ref (dest_profile);
                }
            }

          ligma_image_convert_color_profile (image, dest_profile,
                                            intent, bpc,
                                            progress, NULL);

          g_object_unref (dest_profile);
        }

      g_clear_object (&pref_profile);
    }
}

LigmaColorTransform *
ligma_image_get_color_transform_to_srgb_u8 (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_create_color_transforms (image);

  return private->transform_to_srgb_u8;
}

LigmaColorTransform *
ligma_image_get_color_transform_from_srgb_u8 (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_create_color_transforms (image);

  return private->transform_from_srgb_u8;
}

LigmaColorTransform *
ligma_image_get_color_transform_to_srgb_double (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_create_color_transforms (image);

  return private->transform_to_srgb_double;
}

LigmaColorTransform *
ligma_image_get_color_transform_from_srgb_double (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  ligma_image_create_color_transforms (image);

  return private->transform_from_srgb_double;
}

void
ligma_image_color_profile_pixel_to_srgb (LigmaImage  *image,
                                        const Babl *pixel_format,
                                        gpointer    pixel,
                                        LigmaRGB    *color)
{
  LigmaColorTransform *transform;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  transform = ligma_image_get_color_transform_to_srgb_double (image);

  if (transform)
    {
      ligma_color_transform_process_pixels (transform,
                                           pixel_format,
                                           pixel,
                                           babl_format ("R'G'B'A double"),
                                           color,
                                           1);
    }
  else
    {
      ligma_rgba_set_pixel (color, pixel_format, pixel);
    }
}

void
ligma_image_color_profile_srgb_to_pixel (LigmaImage     *image,
                                        const LigmaRGB *color,
                                        const Babl    *pixel_format,
                                        gpointer       pixel)
{
  LigmaColorTransform *transform;

  g_return_if_fail (LIGMA_IS_IMAGE (image));

  transform = ligma_image_get_color_transform_from_srgb_double (image);

  if (transform)
    {
      ligma_color_transform_process_pixels (transform,
                                           babl_format ("R'G'B'A double"),
                                           color,
                                           pixel_format,
                                           pixel,
                                           1);
    }
  else
    {
      ligma_rgba_get_pixel (color, pixel_format, pixel);
    }
}


/*  internal API  */

void
_ligma_image_free_color_profile (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->color_profile);
  private->layer_space = NULL;

  g_clear_object (&private->hidden_profile);

  _ligma_image_free_color_transforms (image);
}

void
_ligma_image_free_color_transforms (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->transform_to_srgb_u8);
  g_clear_object (&private->transform_from_srgb_u8);
  g_clear_object (&private->transform_to_srgb_double);
  g_clear_object (&private->transform_from_srgb_double);

  private->color_transforms_created = FALSE;
}

void
_ligma_image_update_color_profile (LigmaImage          *image,
                                  const LigmaParasite *icc_parasite)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  _ligma_image_free_color_profile (image);

  if (icc_parasite)
    {
      GError       *error = NULL;
      const guint8 *data;
      guint32       data_size;

      data = ligma_parasite_get_data (icc_parasite, &data_size);
      private->color_profile =
        ligma_color_profile_new_from_icc_profile (data, data_size, NULL);

      private->layer_space =
        ligma_color_profile_get_space (private->color_profile,
                                      LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                      &error);
      if (! private->layer_space)
        {
          g_printerr ("%s: failed to create Babl space from profile: %s\n",
                      G_STRFUNC, error->message);
          g_clear_error (&error);
        }
    }

  ligma_color_managed_profile_changed (LIGMA_COLOR_MANAGED (image));
}

void
_ligma_image_update_simulation_profile (LigmaImage          *image,
                                       const LigmaParasite *icc_parasite)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->simulation_profile);

  if (icc_parasite)
    {
      const guint8 *data;
      guint32       data_size;

      data = ligma_parasite_get_data (icc_parasite, &data_size);
      private->simulation_profile =
        ligma_color_profile_new_from_icc_profile (data, data_size, NULL);
    }

  ligma_color_managed_simulation_profile_changed (LIGMA_COLOR_MANAGED (image));
}


/*  private functions  */

static void
ligma_image_convert_profile_layers (LigmaImage                *image,
                                   LigmaColorProfile         *src_profile,
                                   LigmaColorProfile         *dest_profile,
                                   LigmaColorRenderingIntent  intent,
                                   gboolean                  bpc,
                                   LigmaProgress             *progress)
{
  LigmaObjectQueue *queue;
  GList           *layers;
  GList           *list;
  LigmaDrawable    *drawable;

  queue    = ligma_object_queue_new (progress);
  progress = LIGMA_PROGRESS (queue);

  layers = ligma_image_get_layer_list (image);

  for (list = layers; list; list = g_list_next (list))
    {
      if (! ligma_viewable_get_children (list->data))
        ligma_object_queue_push (queue, list->data);
    }

  g_list_free (layers);

  while ((drawable = ligma_object_queue_pop (queue)))
    {
      LigmaItem   *item = LIGMA_ITEM (drawable);
      GeglBuffer *buffer;
      gboolean    alpha;

      alpha = ligma_drawable_has_alpha (drawable);

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                ligma_item_get_width  (item),
                                                ligma_item_get_height (item)),
                                ligma_image_get_layer_format (image, alpha));

      ligma_gegl_convert_color_profile (ligma_drawable_get_buffer (drawable),
                                       NULL,
                                       src_profile,
                                       buffer,
                                       NULL,
                                       dest_profile,
                                       intent, bpc,
                                       progress);

      ligma_drawable_set_buffer (drawable, TRUE, NULL, buffer);
      g_object_unref (buffer);
    }

  g_object_unref (queue);
}

static void
ligma_image_convert_profile_colormap (LigmaImage                *image,
                                     LigmaColorProfile         *src_profile,
                                     LigmaColorProfile         *dest_profile,
                                     LigmaColorRenderingIntent  intent,
                                     gboolean                  bpc,
                                     LigmaProgress             *progress)
{
  LigmaColorTransform      *transform;
  const Babl              *src_format;
  const Babl              *dest_format;
  LigmaColorTransformFlags  flags = 0;
  guchar                  *cmap;
  gint                     n_colors;

  n_colors = ligma_image_get_colormap_size (image);
  cmap     = ligma_image_get_colormap (image);

  if (bpc)
    flags |= LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

  src_format  = ligma_color_profile_get_format (src_profile,
                                               babl_format ("R'G'B' u8"),
                                               LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                               NULL);
  dest_format = ligma_color_profile_get_format (dest_profile,
                                               babl_format ("R'G'B' u8"),
                                               LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                               NULL);

  transform = ligma_color_transform_new (src_profile,  src_format,
                                        dest_profile, dest_format,
                                        intent, flags);

  if (transform)
    {
      ligma_color_transform_process_pixels (transform,
                                           babl_format ("R'G'B' u8"), cmap,
                                           babl_format ("R'G'B' u8"), cmap,
                                           n_colors);
      g_object_unref (transform);

      ligma_image_set_colormap (image, cmap, n_colors, TRUE);
    }
  else
    {
      g_warning ("ligma_color_transform_new() failed!");
    }

  g_free (cmap);
}

static void
ligma_image_fix_layer_format_spaces (LigmaImage    *image,
                                    LigmaProgress *progress)
{
  LigmaObjectQueue *queue;
  GList           *layers;
  GList           *list;
  LigmaLayer       *layer;

  queue = ligma_object_queue_new (progress);

  layers = ligma_image_get_layer_list (image);

  for (list = layers; list; list = g_list_next (list))
    {
      if (! ligma_viewable_get_children (list->data))
        ligma_object_queue_push (queue, list->data);
    }

  g_list_free (layers);

  while ((layer = ligma_object_queue_pop (queue)))
    {
      ligma_layer_fix_format_space (layer, TRUE, TRUE);
    }

  g_object_unref (queue);
}

static void
ligma_image_create_color_transforms (LigmaImage *image)
{
  LigmaImagePrivate *private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->color_profile &&
      ! private->color_transforms_created)
    {
      LigmaColorProfile        *srgb_profile;
      LigmaColorTransformFlags  flags = 0;

      srgb_profile = ligma_color_profile_new_rgb_srgb ();

      flags |= LIGMA_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;
      flags |= LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      private->transform_to_srgb_u8 =
        ligma_color_transform_new (private->color_profile,
                                  ligma_image_get_layer_format (image, TRUE),
                                  srgb_profile,
                                  babl_format ("R'G'B'A u8"),
                                  LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_from_srgb_u8 =
        ligma_color_transform_new (srgb_profile,
                                  babl_format ("R'G'B'A u8"),
                                  private->color_profile,
                                  ligma_image_get_layer_format (image, TRUE),
                                  LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_to_srgb_double =
        ligma_color_transform_new (private->color_profile,
                                  ligma_image_get_layer_format (image, TRUE),
                                  srgb_profile,
                                  babl_format ("R'G'B'A double"),
                                  LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_from_srgb_double =
        ligma_color_transform_new (srgb_profile,
                                  babl_format ("R'G'B'A double"),
                                  private->color_profile,
                                  ligma_image_get_layer_format (image, TRUE),
                                  LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      g_object_unref (srgb_profile);

      private->color_transforms_created = TRUE;
    }
}
