/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimage-color-profile.c
 * Copyright (C) 2015 Michael Natterer <mitch@gimp.org>
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
#include "gimpdrawable.h"
#include "gimperror.h"
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

static void   gimp_image_create_color_transforms  (GimpImage                *image);


/*  public functions  */

gboolean
gimp_image_get_is_color_managed (GimpImage *image)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);

  return GIMP_IMAGE_GET_PRIVATE (image)->is_color_managed;
}

void
gimp_image_set_is_color_managed (GimpImage *image,
                                 gboolean   is_color_managed,
                                 gboolean   push_undo)
{
  GimpImagePrivate *private;

  g_return_if_fail (GIMP_IS_IMAGE (image));

  private = GIMP_IMAGE_GET_PRIVATE (image);

  is_color_managed = is_color_managed ? TRUE : FALSE;

  if (is_color_managed != private->is_color_managed)
    {
      if (push_undo)
        gimp_image_undo_push_image_color_managed (image, NULL);

      private->is_color_managed = is_color_managed;

      gimp_color_managed_profile_changed (GIMP_COLOR_MANAGED (image));
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
      GimpColorProfile *builtin;

      builtin = gimp_babl_format_get_color_profile (format);

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

  return gimp_babl_format_get_color_profile (format);
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

  if (! src_profile || gimp_color_profile_is_equal (src_profile, dest_profile))
    return TRUE;

  if (progress)
    gimp_progress_start (progress, FALSE,
                         _("Converting from '%s' to '%s'"),
                         gimp_color_profile_get_label (src_profile),
                         gimp_color_profile_get_label (dest_profile));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               _("Color profile conversion"));

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
      break;
    }

  gimp_image_set_is_color_managed (image, TRUE, TRUE);
  gimp_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  gimp_image_parasite_detach (image, "icc-profile-name", TRUE);

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

  if (gimp_image_get_is_color_managed (image) &&
      gimp_image_get_color_profile (image))
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

  if (private->is_color_managed)
    return private->transform_to_srgb_u8;

  return NULL;
}

GimpColorTransform *
gimp_image_get_color_transform_from_srgb_u8 (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  if (private->is_color_managed)
    return private->transform_from_srgb_u8;

  return NULL;
}

GimpColorTransform *
gimp_image_get_color_transform_to_srgb_double (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  if (private->is_color_managed)
    return private->transform_to_srgb_double;

  return NULL;
}

GimpColorTransform *
gimp_image_get_color_transform_from_srgb_double (GimpImage *image)
{
  GimpImagePrivate *private;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  private = GIMP_IMAGE_GET_PRIVATE (image);

  gimp_image_create_color_transforms (image);

  if (private->is_color_managed)
    return private->transform_from_srgb_double;

  return NULL;
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
      private->color_profile =
        gimp_color_profile_new_from_icc_profile (gimp_parasite_data (icc_parasite),
                                                 gimp_parasite_data_size (icc_parasite),
                                                 NULL);
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
      gimp_drawable_push_undo (drawable, NULL, NULL,
                               0, 0,
                               gimp_item_get_width  (GIMP_ITEM (drawable)),
                               gimp_item_get_height (GIMP_ITEM (drawable)));

      gimp_gegl_convert_color_profile (gimp_drawable_get_buffer (drawable),
                                       NULL,
                                       src_profile,
                                       gimp_drawable_get_buffer (drawable),
                                       NULL,
                                       dest_profile,
                                       intent, bpc,
                                       progress);

      gimp_drawable_update (drawable, 0, 0, -1, -1);
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
  GimpColorTransformFlags  flags = 0;
  guchar                  *cmap;
  gint                     n_colors;

  n_colors = gimp_image_get_colormap_size (image);
  cmap     = g_memdup (gimp_image_get_colormap (image), n_colors * 3);

  if (bpc)
    flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

  transform = gimp_color_transform_new (src_profile,
                                        babl_format ("R'G'B' u8"),
                                        dest_profile,
                                        babl_format ("R'G'B' u8"),
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
      g_warning ("cmsCreateTransform() failed!");
    }

  g_free (cmap);
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
