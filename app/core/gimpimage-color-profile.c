/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-color-profile.c
 * Copyright (C) 2015-2018 Michael Natterer <mitch@gimp.org>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpdialogconfig.h"

#include "gegl/gimp-babl.h"
#include "gegl/gimp-gegl-loops.h"

#include "gimp.h"
#include "gimpcontext.h"
#include "gimperror.h"
#include "gimplayer.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-private.h"
#include "gimpimage-undo.h"
#include "gimpimage-undo-push.h"
#include "gimpobjectqueue.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_image_convert_profile_layers   (GimpImage                *image,
                                                   GimpColorProfile         *src_profile,
                                                   GimpColorProfile         *dest_profile,
                                                   GimpColorRenderingIntent  intent,
                                                   gboolean                  bpc,
                                                   GimpProgress             *progress);
static void   gimp_image_convert_profile_colormap (GimpImage                *image,
                                                   GimpColorProfile         *src_profile,
                                                   GimpColorProfile         *dest_profile,
                                                   GimpColorRenderingIntent  intent,
                                                   gboolean                  bpc,
                                                   GimpProgress             *progress);

static void   gimp_image_fix_layer_format_spaces  (GimpImage                *image,
                                                   GimpProgress             *progress);

static void   gimp_image_create_color_transforms  (GimpImage                *image);


/*  public functions  */

gboolean
gimp_image_get_use_srgb_profile (GimpImage *image,
                                 gboolean  *hidden_profile)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (hidden_profile)
    *hidden_profile = (private->hidden_profile != NULL);

  return private->color_profile == NULL;
}

void
gimp_image_set_use_srgb_profile (GimpImage *image,
                                 gboolean   use_srgb)
{
  GimpImagePrivate *private;
  gboolean          old_use_srgb;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  old_use_srgb = (private->color_profile == NULL);

  use_srgb = use_srgb ? TRUE : FALSE;

  if (use_srgb == old_use_srgb)
    return;

  if (use_srgb)
    {
      GimpColorProfile *profile = gimp_image_get_color_profile (image);

      if (profile)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                                       _("Enable 'Use sRGB Profile'"));

          g_object_ref (profile);
          gimp_image_assign_color_profile (image, NULL, NULL, NULL);
          _gimp_image_set_hidden_profile (image, profile, TRUE);
          g_object_unref (profile);

          gimp_image_undo_group_end (image);
        }
    }
  else
    {
      GimpColorProfile *hidden = _gimp_image_get_hidden_profile (image);

      if (hidden)
        {
          gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                                       _("Disable 'Use sRGB Profile'"));

          g_object_ref (hidden);
          gimp_image_assign_color_profile (image, hidden, NULL, NULL);
          g_object_unref (hidden);

          gimp_image_undo_group_end (image);
        }
    }
}

GimpColorProfile *
_gimp_image_get_hidden_profile (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  return private->hidden_profile;
}

void
_gimp_image_set_hidden_profile (GimpImage        *image,
                                GimpColorProfile *profile,
                                gboolean          push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  if (profile != private->hidden_profile)
    {
      if (push_undo)
        gimp_image_undo_push_image_hidden_profile (image, NULL);

      g_set_object (&private->hidden_profile, profile);
    }
}

gboolean
gimp_image_validate_icc_parasite (GimpImage           *image,
                                  const GimpParasite  *icc_parasite,
                                  gboolean            *is_builtin,
                                  GError             **error)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (icc_parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (strcmp (gimp_parasite_name (icc_parasite),
              GIMP_ICC_PROFILE_PARASITE_NAME) != 0)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("ICC profile validation failed: "
                             "Parasite's name is not 'icc-profile'"));
      return FALSE;
    }

  if (gimp_parasite_flags (icc_parasite) != (GIMP_PARASITE_PERSISTENT |
                                             GIMP_PARASITE_UNDOABLE))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("ICC profile validation failed: "
                             "Parasite's flags are not (PERSISTENT | UNDOABLE)"));
      return FALSE;
    }

  return gimp_image_validate_icc_profile (image,
                                          gimp_parasite_data (icc_parasite),
                                          gimp_parasite_data_size (icc_parasite),
                                          is_builtin,
                                          error);
}

const GimpParasite *
gimp_image_get_icc_parasite (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return gimp_image_parasite_find (image, GIMP_ICC_PROFILE_PARASITE_NAME);
}

void
gimp_image_set_icc_parasite (GimpImage          *image,
                             const GimpParasite *icc_parasite)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));

  if (icc_parasite)
    {
      g_return_if_fail (gimp_image_validate_icc_parasite (image, icc_parasite,
                                                          NULL, NULL) == TRUE);

      gimp_image_parasite_attach (image, icc_parasite, TRUE);
    }
  else
    {
      gimp_image_parasite_detach (image, GIMP_ICC_PROFILE_PARASITE_NAME, TRUE);
    }
}

gboolean
gimp_image_validate_icc_profile (GimpImage     *image,
                                 const guint8  *data,
                                 gsize          length,
                                 gboolean      *is_builtin,
                                 GError       **error)
{
  GimpColorProfile *profile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (data != NULL || length == 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  profile = gimp_color_profile_new_from_icc_profile (data, length, error);

  if (! profile)
    {
      g_prefix_error (error, _("ICC profile validation failed: "));
      return FALSE;
    }

  if (! gimp_image_validate_color_profile (image, profile, is_builtin, error))
    {
      g_object_unref (profile);
      return FALSE;
    }

  g_object_unref (profile);

  return TRUE;
}

const guint8 *
gimp_image_get_icc_profile (GimpImage *image,
                            gsize     *length)
{
  const GimpParasite *parasite;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  parasite = gimp_image_parasite_find (image, GIMP_ICC_PROFILE_PARASITE_NAME);

  if (parasite)
    {
      if (length)
        *length = gimp_parasite_data_size (parasite);

      return gimp_parasite_data (parasite);
    }

  if (length)
    *length = 0;

  return NULL;
}

gboolean
gimp_image_set_icc_profile (GimpImage     *image,
                            const guint8  *data,
                            gsize          length,
                            GError       **error)
{
  GimpParasite *parasite = NULL;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (data == NULL || length != 0, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (data)
    {
      gboolean is_builtin;

      parasite = gimp_parasite_new (GIMP_ICC_PROFILE_PARASITE_NAME,
                                    GIMP_PARASITE_PERSISTENT |
                                    GIMP_PARASITE_UNDOABLE,
                                    length, data);

      if (! gimp_image_validate_icc_parasite (image, parasite, &is_builtin,
                                              error))
        {
          gimp_parasite_free (parasite);
          return FALSE;
        }

      /*  don't tag the image with the built-in profile  */
      if (is_builtin)
        {
          gimp_parasite_free (parasite);
          parasite = NULL;
        }
    }

  gimp_image_set_icc_parasite (image, parasite);

  if (parasite)
    gimp_parasite_free (parasite);

  return TRUE;
}

gboolean
gimp_image_validate_color_profile (GimpImage        *image,
                                   GimpColorProfile *profile,
                                   gboolean         *is_builtin,
                                   GError          **error)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  format = gimp_image_get_layer_format (image, TRUE);

  return gimp_image_validate_color_profile_by_format (format,
                                                      profile, is_builtin,
                                                      error);
}

GimpColorProfile *
gimp_image_get_color_profile (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  return GIMP_IMAGE_GET_PRIVATE (image)->color_profile;
}

gboolean
gimp_image_set_color_profile (GimpImage         *image,
                              GimpColorProfile  *profile,
                              GError           **error)
{
  const guint8 *data   = NULL;
  gsize         length = 0;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (profile == NULL || GIMP_IS_COLOR_PROFILE (profile),
                        FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (profile)
    data = gimp_color_profile_get_icc_profile (profile, &length);

  return gimp_image_set_icc_profile (image, data, length, error);
}

gboolean
gimp_image_validate_color_profile_by_format (const Babl         *format,
                                             GimpColorProfile   *profile,
                                             gboolean           *is_builtin,
                                             GError            **error)
{
  g_return_val_if_fail (format != NULL, FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_babl_format_get_base_type (format) == GIMP_GRAY)
    {
      if (! gimp_color_profile_is_gray (profile))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("ICC profile validation failed: "
                                 "Color profile is not for grayscale color space"));
          return FALSE;
        }
    }
  else
    {
      if (! gimp_color_profile_is_rgb (profile))
        {
          g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                               _("ICC profile validation failed: "
                                 "Color profile is not for RGB color space"));
          return FALSE;
        }
    }

  if (is_builtin)
    {
      GimpColorProfile *builtin =
        gimp_babl_get_builtin_color_profile (gimp_babl_format_get_base_type (format),
                                             gimp_babl_format_get_trc (format));

      *is_builtin = gimp_color_profile_is_equal (profile, builtin);
    }

  return TRUE;
}

GimpColorProfile *
gimp_image_get_builtin_color_profile (GimpImage *image)
{
  const Babl *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  format = gimp_image_get_layer_format (image, FALSE);

  return gimp_babl_get_builtin_color_profile (gimp_babl_format_get_base_type (format),
                                              gimp_babl_format_get_trc (format));
}

gboolean
gimp_image_assign_color_profile (GimpImage         *image,
                                 GimpColorProfile  *dest_profile,
                                 GimpProgress      *progress,
                                 GError           **error)
{
  GimpColorProfile *src_profile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (dest_profile == NULL ||
                        GIMP_IS_COLOR_PROFILE (dest_profile), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (dest_profile &&
      ! gimp_image_validate_color_profile (image, dest_profile, NULL, error))
    return FALSE;

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  if (src_profile == dest_profile ||
      (src_profile && dest_profile &&
       gimp_color_profile_is_equal (src_profile, dest_profile)))
    return TRUE;

  if (progress)
    gimp_progress_start (progress, FALSE,
                         dest_profile ?
                         _("Assigning color profile") :
                         _("Discarding color profile"));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               dest_profile ?
                               _("Assign color profile") :
                               _("Discard color profile"));

  _gimp_image_set_hidden_profile (image, NULL, TRUE);

  gimp_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  gimp_image_parasite_detach (image, "icc-profile-name", TRUE);

  if (gimp_image_get_base_type (image) == GIMP_INDEXED)
    gimp_image_colormap_update_formats (image);

  gimp_image_fix_layer_format_spaces (image, progress);

  gimp_image_undo_group_end (image);

  return TRUE;
}

gboolean
gimp_image_convert_color_profile (GimpImage                *image,
                                  GimpColorProfile         *dest_profile,
                                  GimpColorRenderingIntent  intent,
                                  gboolean                  bpc,
                                  GimpProgress             *progress,
                                  GError                  **error)
{
  GimpColorProfile *src_profile;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (dest_profile), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_image_validate_color_profile (image, dest_profile, NULL, error))
    return FALSE;

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    return TRUE;

  if (progress)
    gimp_progress_start (progress, FALSE,
                         _("Converting from '%s' to '%s'"),
                         gimp_color_profile_get_label (src_profile),
                         gimp_color_profile_get_label (dest_profile));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               _("Color profile conversion"));

  /* retain src_profile across gimp_image_set_color_profile() */
  g_object_ref (src_profile);

  _gimp_image_set_hidden_profile (image, NULL, TRUE);

  gimp_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  gimp_image_parasite_detach (image, "icc-profile-name", TRUE);

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
    case GIMP_GRAY:
      gimp_image_convert_profile_layers (image,
                                         src_profile, dest_profile,
                                         intent, bpc,
                                         progress);
      break;

    case GIMP_INDEXED:
      gimp_image_convert_profile_colormap (image,
                                           src_profile, dest_profile,
                                           intent, bpc,
                                           progress);
      gimp_image_fix_layer_format_spaces (image, progress);
      break;
    }

  g_object_unref (src_profile);

  gimp_image_undo_group_end (image);

  if (progress)
    gimp_progress_end (progress);

  return TRUE;
}

void
gimp_image_import_color_profile (GimpImage    *image,
                                 GimpContext  *context,
                                 GimpProgress *progress,
                                 gboolean      interactive)
{
  g_return_if_fail (GIMP_IS_IMAGE (image));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (gimp_image_get_color_profile (image))
    {
      GimpColorProfilePolicy     policy;
      GimpColorProfile          *dest_profile = NULL;
      GimpColorRenderingIntent   intent;
      gboolean                   bpc;

      policy = GIMP_DIALOG_CONFIG (image->gimp->config)->color_profile_policy;
      intent = GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC;
      bpc    = TRUE;

      if (policy == GIMP_COLOR_PROFILE_POLICY_ASK)
        {
          if (interactive)
            {
              gboolean dont_ask = FALSE;

              policy = gimp_query_profile_policy (image->gimp, image, context,
                                                  &dest_profile,
                                                  &intent, &bpc,
                                                  &dont_ask);

              if (dont_ask)
                {
                  g_object_set (G_OBJECT (image->gimp->config),
                                "color-profile-policy", policy,
                                NULL);
                }
            }
          else
            {
              policy = GIMP_COLOR_PROFILE_POLICY_KEEP;
            }
        }

      if (policy == GIMP_COLOR_PROFILE_POLICY_CONVERT)
        {
          if (! dest_profile)
            {
              dest_profile = gimp_image_get_builtin_color_profile (image);
              g_object_ref (dest_profile);
            }

          gimp_image_convert_color_profile (image, dest_profile,
                                            intent, bpc,
                                            progress, NULL);

          g_object_unref (dest_profile);
        }
    }
}

GimpColorTransform *
gimp_image_get_color_transform_to_srgb_u8 (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  return private->transform_to_srgb_u8;
}

GimpColorTransform *
gimp_image_get_color_transform_from_srgb_u8 (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  return private->transform_from_srgb_u8;
}

GimpColorTransform *
gimp_image_get_color_transform_to_srgb_double (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  return private->transform_to_srgb_double;
}

GimpColorTransform *
gimp_image_get_color_transform_from_srgb_double (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  return private->transform_from_srgb_double;
}

void
gimp_image_color_profile_pixel_to_srgb (GimpImage  *image,
                                        const Babl *pixel_format,
                                        gpointer    pixel,
                                        GimpRGB    *color)
{
  GimpColorTransform *transform;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  transform = gimp_image_get_color_transform_to_srgb_double (image);

  if (transform)
    {
      gimp_color_transform_process_pixels (transform,
                                           pixel_format,
                                           pixel,
                                           babl_format ("R'G'B'A double"),
                                           color,
                                           1);
    }
  else
    {
      gimp_rgba_set_pixel (color, pixel_format, pixel);
    }
}

void
gimp_image_color_profile_srgb_to_pixel (GimpImage     *image,
                                        const GimpRGB *color,
                                        const Babl    *pixel_format,
                                        gpointer       pixel)
{
  GimpColorTransform *transform;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  transform = gimp_image_get_color_transform_from_srgb_double (image);

  if (transform)
    {
      gimp_color_transform_process_pixels (transform,
                                           babl_format ("R'G'B'A double"),
                                           color,
                                           pixel_format,
                                           pixel,
                                           1);
    }
  else
    {
      gimp_rgba_get_pixel (color, pixel_format, pixel);
    }
}


/*  internal API  */

void
_gimp_image_free_color_profile (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->color_profile);
  private->layer_space = NULL;

  g_clear_object (&private->hidden_profile);

  _gimp_image_free_color_transforms (image);
}

void
_gimp_image_free_color_transforms (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  g_clear_object (&private->transform_to_srgb_u8);
  g_clear_object (&private->transform_from_srgb_u8);
  g_clear_object (&private->transform_to_srgb_double);
  g_clear_object (&private->transform_from_srgb_double);

  private->color_transforms_created = FALSE;
}

void
_gimp_image_update_color_profile (GimpImage          *image,
                                  const GimpParasite *icc_parasite)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  _gimp_image_free_color_profile (image);

  if (icc_parasite)
    {
      GError *error = NULL;

      private->color_profile =
        gimp_color_profile_new_from_icc_profile (gimp_parasite_data (icc_parasite),
                                                 gimp_parasite_data_size (icc_parasite),
                                                 NULL);

      private->layer_space =
        gimp_color_profile_get_space (private->color_profile,
                                      GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                      &error);
      if (! private->layer_space)
        {
          g_printerr ("%s: failed to create Babl space from profile: %s\n",
                      G_STRFUNC, error->message);
          g_clear_error (&error);
        }
    }

  gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
}


/*  private functions  */

static void
gimp_image_convert_profile_layers (GimpImage                *image,
                                   GimpColorProfile         *src_profile,
                                   GimpColorProfile         *dest_profile,
                                   GimpColorRenderingIntent  intent,
                                   gboolean                  bpc,
                                   GimpProgress             *progress)
{
  GimpObjectQueue *queue;
  GList           *layers;
  GList           *list;
  GimpDrawable    *drawable;

  queue    = gimp_object_queue_new (progress);
  progress = GIMP_PROGRESS (queue);

  layers = gimp_image_get_layer_list (image);

  for (list = layers; list; list = g_list_next (list))
    {
      if (! gimp_viewable_get_children (list->data))
        gimp_object_queue_push (queue, list->data);
    }

  g_list_free (layers);

  while ((drawable = gimp_object_queue_pop (queue)))
    {
      GimpItem   *item = GIMP_ITEM (drawable);
      GeglBuffer *buffer;
      gboolean    alpha;

      alpha = gimp_drawable_has_alpha (drawable);

      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                gimp_item_get_width  (item),
                                                gimp_item_get_height (item)),
                                gimp_image_get_layer_format (image, alpha));

      gimp_gegl_convert_color_profile (gimp_drawable_get_buffer (drawable),
                                       NULL,
                                       src_profile,
                                       buffer,
                                       NULL,
                                       dest_profile,
                                       intent, bpc,
                                       progress);

      gimp_drawable_set_buffer (drawable, TRUE, NULL, buffer);
      g_object_unref (buffer);
    }

  g_object_unref (queue);
}

static void
gimp_image_convert_profile_colormap (GimpImage                *image,
                                     GimpColorProfile         *src_profile,
                                     GimpColorProfile         *dest_profile,
                                     GimpColorRenderingIntent  intent,
                                     gboolean                  bpc,
                                     GimpProgress             *progress)
{
  GimpColorTransform      *transform;
  const Babl              *src_format;
  const Babl              *dest_format;
  GimpColorTransformFlags  flags = 0;
  guchar                  *cmap;
  gint                     n_colors;

  n_colors = gimp_image_get_colormap_size (image);
  cmap     = g_memdup (gimp_image_get_colormap (image), n_colors * 3);

  if (bpc)
    flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

  src_format  = gimp_color_profile_get_format (src_profile,
                                               babl_format ("R'G'B' u8"),
                                               GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                               NULL);
  dest_format = gimp_color_profile_get_format (dest_profile,
                                               babl_format ("R'G'B' u8"),
                                               GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                               NULL);

  transform = gimp_color_transform_new (src_profile,  src_format,
                                        dest_profile, dest_format,
                                        intent, flags);

  if (transform)
    {
      gimp_color_transform_process_pixels (transform,
                                           babl_format ("R'G'B' u8"), cmap,
                                           babl_format ("R'G'B' u8"), cmap,
                                           n_colors);
      g_object_unref (transform);

      gimp_image_set_colormap (image, cmap, n_colors, TRUE);
    }
  else
    {
      g_warning ("gimp_color_transform_new() failed!");
    }

  g_free (cmap);
}

static void
gimp_image_fix_layer_format_spaces (GimpImage    *image,
                                    GimpProgress *progress)
{
  GimpObjectQueue *queue;
  GList           *layers;
  GList           *list;
  GimpLayer       *layer;

  queue = gimp_object_queue_new (progress);

  layers = gimp_image_get_layer_list (image);

  for (list = layers; list; list = g_list_next (list))
    {
      if (! gimp_viewable_get_children (list->data))
        gimp_object_queue_push (queue, list->data);
    }

  g_list_free (layers);

  while ((layer = gimp_object_queue_pop (queue)))
    {
      gimp_layer_fix_format_space (layer, TRUE, TRUE);
    }

  g_object_unref (queue);
}

static void
gimp_image_create_color_transforms (GimpImage *image)
{
  GimpImagePrivate *private = GIMP_IMAGE_GET_PRIVATE (image);

  if (private->color_profile &&
      ! private->color_transforms_created)
    {
      GimpColorProfile        *srgb_profile;
      GimpColorTransformFlags  flags = 0;

      srgb_profile = gimp_color_profile_new_rgb_srgb ();

      flags |= GIMP_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;
      flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      private->transform_to_srgb_u8 =
        gimp_color_transform_new (private->color_profile,
                                  gimp_image_get_layer_format (image, TRUE),
                                  srgb_profile,
                                  babl_format ("R'G'B'A u8"),
                                  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_from_srgb_u8 =
        gimp_color_transform_new (srgb_profile,
                                  babl_format ("R'G'B'A u8"),
                                  private->color_profile,
                                  gimp_image_get_layer_format (image, TRUE),
                                  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_to_srgb_double =
        gimp_color_transform_new (private->color_profile,
                                  gimp_image_get_layer_format (image, TRUE),
                                  srgb_profile,
                                  babl_format ("R'G'B'A double"),
                                  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      private->transform_from_srgb_double =
        gimp_color_transform_new (srgb_profile,
                                  babl_format ("R'G'B'A double"),
                                  private->color_profile,
                                  gimp_image_get_layer_format (image, TRUE),
                                  GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                  flags);

      g_object_unref (srgb_profile);

      private->color_transforms_created = TRUE;
    }
}
