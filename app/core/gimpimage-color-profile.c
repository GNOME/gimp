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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>
#include <lcms2.h>

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gegl/gimp-babl.h"

#include "gimp.h"
#include "gimpdrawable.h"
#include "gimperror.h"
#include "gimpimage.h"
#include "gimpimage-color-profile.h"
#include "gimpimage-colormap.h"
#include "gimpimage-private.h"
#include "gimpimage-undo.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_image_convert_profile_rgb     (GimpImage                *image,
                                                  GimpColorProfile         *src_profile,
                                                  GimpColorProfile         *dest_profile,
                                                  GimpColorRenderingIntent  intent,
                                                  gboolean                  bpc,
                                                  GimpProgress             *progress);
static void   gimp_image_convert_profile_indexed (GimpImage                *image,
                                                  GimpColorProfile         *src_profile,
                                                  GimpColorProfile         *dest_profile,
                                                  GimpColorRenderingIntent  intent,
                                                  gboolean                  bpc,
                                                  GimpProgress             *progress);


/*  public functions  */

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

      gimp_image_parasite_attach (image, icc_parasite);
    }
  else
    {
      gimp_image_parasite_detach (image, GIMP_ICC_PROFILE_PARASITE_NAME);
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
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (length != 0, FALSE);
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
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (GIMP_IS_COLOR_PROFILE (profile), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (gimp_image_get_base_type (image) == GIMP_GRAY)
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("ICC profile validation failed: "
                             "Cannot attach a color profile to a GRAY image"));
      return FALSE;
    }

  if (! gimp_color_profile_is_rgb (profile))
    {
      g_set_error_literal (error, GIMP_ERROR, GIMP_FAILED,
                           _("ICC profile validation failed: "
                             "Color profile is not for RGB color space"));
      return FALSE;
    }

  if (is_builtin)
    {
      GimpColorProfile *builtin;

      builtin = gimp_image_get_builtin_color_profile (image);

      *is_builtin = gimp_color_profile_is_equal (profile, builtin);
    }

  return TRUE;
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

GimpColorProfile *
gimp_image_get_builtin_color_profile (GimpImage *image)
{
  static GimpColorProfile *srgb_profile       = NULL;
  static GimpColorProfile *linear_rgb_profile = NULL;
  const  Babl             *format;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  if (! srgb_profile)
    {
      srgb_profile       = gimp_color_profile_new_srgb ();
      linear_rgb_profile = gimp_color_profile_new_linear_rgb ();
    }

  format = gimp_image_get_layer_format (image, FALSE);

  if (gimp_babl_format_get_linear (format))
    {
      if (! srgb_profile)
        {
          srgb_profile = gimp_color_profile_new_srgb ();
          g_object_add_weak_pointer (G_OBJECT (srgb_profile),
                                     (gpointer) &srgb_profile);
        }

      return linear_rgb_profile;
    }
  else
    {
      if (! linear_rgb_profile)
        {
          linear_rgb_profile = gimp_color_profile_new_linear_rgb ();
          g_object_add_weak_pointer (G_OBJECT (linear_rgb_profile),
                                     (gpointer) &linear_rgb_profile);
        }

      return srgb_profile;
    }
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
  g_return_val_if_fail (dest_profile != NULL, FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (! gimp_image_validate_color_profile (image, dest_profile, NULL, error))
    return FALSE;

  src_profile = gimp_color_managed_get_color_profile (GIMP_COLOR_MANAGED (image));

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    {
      g_object_unref (src_profile);
      return TRUE;
    }

  if (progress)
    gimp_progress_start (progress, FALSE,
                         _("Converting from '%s' to '%s'"),
                         gimp_color_profile_get_label (src_profile),
                         gimp_color_profile_get_label (dest_profile));

  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_IMAGE_CONVERT,
                               _("Color profile conversion"));

  gimp_image_set_color_profile (image, dest_profile, NULL);
  /*  omg...  */
  gimp_image_parasite_detach (image, "icc-profile-name");

  switch (gimp_image_get_base_type (image))
    {
    case GIMP_RGB:
      gimp_image_convert_profile_rgb (image,
                                      src_profile, dest_profile,
                                      intent, bpc,
                                      progress);
      break;

    case GIMP_GRAY:
      break;

    case GIMP_INDEXED:
      gimp_image_convert_profile_indexed (image,
                                          src_profile, dest_profile,
                                          intent, bpc,
                                          progress);
      break;
    }

  gimp_image_undo_group_end (image);

  if (progress)
    gimp_progress_end (progress);

  g_object_unref (src_profile);

  return TRUE;
}


/*  private functions  */

static void
gimp_image_convert_profile_rgb (GimpImage                *image,
                                GimpColorProfile         *src_profile,
                                GimpColorProfile         *dest_profile,
                                GimpColorRenderingIntent  intent,
                                gboolean                  bpc,
                                GimpProgress             *progress)
{
  GList *layers;
  GList *list;
  gint   n_drawables;
  gint   nth_drawable;

  layers = gimp_image_get_layer_list (image);

  n_drawables = g_list_length (layers);

  for (list = layers, nth_drawable = 0;
       list;
       list = g_list_next (list), nth_drawable++)
    {
      GimpDrawable    *drawable = list->data;
      cmsHPROFILE      src_lcms;
      cmsHPROFILE      dest_lcms;
      const Babl      *iter_format;
      cmsUInt32Number  lcms_format;
      cmsUInt32Number  flags;
      cmsHTRANSFORM    transform;

      if (gimp_viewable_get_children (GIMP_VIEWABLE (drawable)))
        continue;

      src_lcms  = gimp_color_profile_get_lcms_profile (src_profile);
      dest_lcms = gimp_color_profile_get_lcms_profile (dest_profile);

      iter_format =
        gimp_color_profile_get_format (gimp_drawable_get_format (drawable),
                                       &lcms_format);

      flags = cmsFLAGS_NOOPTIMIZE;

      if (bpc)
        flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;

      transform = cmsCreateTransform (src_lcms,  lcms_format,
                                      dest_lcms, lcms_format,
                                      intent, flags);

      if (transform)
        {
          GeglBuffer         *buffer;
          GeglBufferIterator *iter;

          buffer = gimp_drawable_get_buffer (drawable);

          gimp_drawable_push_undo (drawable, NULL, NULL,
                                   0, 0,
                                   gegl_buffer_get_width  (buffer),
                                   gegl_buffer_get_height (buffer));

          iter = gegl_buffer_iterator_new (buffer, NULL, 0,
                                           iter_format,
                                           GEGL_ACCESS_READWRITE,
                                           GEGL_ABYSS_NONE);

          while (gegl_buffer_iterator_next (iter))
            {
              cmsDoTransform (transform,
                              iter->data[0], iter->data[0], iter->length);
            }

          gimp_drawable_update (drawable, 0, 0,
                                gegl_buffer_get_width  (buffer),
                                gegl_buffer_get_height (buffer));

          cmsDeleteTransform (transform);
        }

      if (progress)
        gimp_progress_set_value (progress,
                                 (gdouble) nth_drawable / (gdouble) n_drawables);
    }

  g_list_free (layers);
}

static void
gimp_image_convert_profile_indexed (GimpImage                *image,
                                    GimpColorProfile         *src_profile,
                                    GimpColorProfile         *dest_profile,
                                    GimpColorRenderingIntent  intent,
                                    gboolean                  bpc,
                                    GimpProgress             *progress)
{
  cmsHPROFILE         src_lcms;
  cmsHPROFILE         dest_lcms;
  guchar             *cmap;
  gint                n_colors;
  GimpColorTransform  transform;

  src_lcms  = gimp_color_profile_get_lcms_profile (src_profile);
  dest_lcms = gimp_color_profile_get_lcms_profile (dest_profile);

  n_colors = gimp_image_get_colormap_size (image);
  cmap     = g_memdup (gimp_image_get_colormap (image), n_colors * 3);

  transform = cmsCreateTransform (src_lcms,  TYPE_RGB_8,
                                  dest_lcms, TYPE_RGB_8,
                                  intent,
                                  cmsFLAGS_NOOPTIMIZE |
                                  (bpc ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0));

  if (transform)
    {
      cmsDoTransform (transform, cmap, cmap, n_colors);
      cmsDeleteTransform (transform);

      gimp_image_set_colormap (image, cmap, n_colors, TRUE);
    }
  else
    {
      g_warning ("cmsCreateTransform() failed!");
    }

  g_free (cmap);
}
