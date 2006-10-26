/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#ifdef HAVE_LCMS_LCMS_H
#include <lcms/lcms.h>
#else
#include <lcms.h>
#endif

#include <libgimp/gimp.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_PROC_SET        "plug-in-icc-profile-set"
#define PLUG_IN_PROC_SET_RGB    "plug-in-icc-profile-set-rgb"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-profile-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-profile-apply-rgb"

#define PLUG_IN_PROC_INFO       "plug-in-icc-profile-info"


enum
{
  STATUS,
  PRODUCT_NAME,
  PRODUCT_DESC,
  PRODUCT_INFO,
  NUM_RETURN_VALS
};

static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);

static GimpPDBStatusType  lcms_icc_set   (gint32        image,
                                          const gchar  *filename);
static GimpPDBStatusType  lcms_icc_apply (gint32        image,
                                          const gchar  *filename);
static GimpPDBStatusType  lcms_icc_info  (gint32        image,
                                          gchar       **name,
                                          gchar       **desc,
                                          gchar       **info);

static cmsHPROFILE  lcms_image_get_profile      (gint32          image);
static gboolean     lcms_image_set_profile      (gint32          image,
                                                 const gchar    *filename);
static void         lcms_image_transform_pixels (gint32          image,
                                                 cmsHPROFILE     src_profile,
                                                 cmsHPROFILE     dest_profile);
static void         lcms_image_transform_cmap   (gint32          image,
                                                 cmsHPROFILE     src_profile,
                                                 cmsHPROFILE     dest_profile);

static void         lcms_drawable_transform     (GimpDrawable   *drawable,
                                                 cmsHTRANSFORM   transform,
                                                 gdouble         progress_start,
                                                 gdouble         progress_end);

static cmsHPROFILE  lcms_config_get_profile     (void);


const GimpPlugInInfo PLUG_IN_INFO =
{
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef base_args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,  "image",    "Input image"                      },
  };
  static const GimpParamDef args[] =
  {
    { GIMP_PDB_INT32,  "run-mode", "Interactive, non-interactive"     },
    { GIMP_PDB_IMAGE,  "image",    "Input image"                      },
    { GIMP_PDB_STRING, "profile",  "Filename of an ICC color profile" }
  };
  static const GimpParamDef info_return_vals[] =
  {
    { GIMP_PDB_STRING, "product-name", "Name"         },
    { GIMP_PDB_STRING, "product-desc", "Description"  },
    { GIMP_PDB_STRING, "product-info", "Info"         }
  };

  gimp_install_procedure (PLUG_IN_PROC_SET,
                          "Set ICC color profile on the image",
                          "This procedure sets an ICC color profile on an "
                          "image using the 'icc-profile' parasite.  It does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Set color profile"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_SET_RGB,
                          "Set the default RGB color profile on the image",
                          "This procedure sets the user-configured RGB "
                          "profile on an image using the 'icc-profile' "
                          "parasite.  If no RGB profile is configured, sRGB "
                          "is assumed and the parasite is unset.  This "
                          "procedure does not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Set default RGB profile"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args), 0,
                          base_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY,
                          "Apply a color profile on the image",
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the given ICC color profile.  The profile "
                          "is then set on the image using the 'icc-profile' "
                          "parasite.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Apply color profile"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (args), 0,
                          args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY_RGB,
                          "Apply default RGB color profile on the image",
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the configured default RGB color profile.  "
                          "The profile is then set on the image using the "
                          "'icc-profile' parasite.  If no RGB color profile "
                          "is configured, sRGB is assumed and the parasite "
                          "is unset.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Apply default RGB profile"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args), 0,
                          base_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_INFO,
                          "Retrieve information about an image's color profile",
                          "This procedure returns information about the "
                          "color profile attached to an image.  If no profile "
                          "is attached, sRGB is assumed.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006",
                          N_("Color Profile Information"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (base_args),
                          G_N_ELEMENTS (info_return_vals),
                          base_args, info_return_vals);
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpRunMode        run_mode;
  GimpPDBStatusType  status = GIMP_PDB_SUCCESS;
  static GimpParam   values[6];

  INIT_I18N ();

  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = status;

  *nreturn_vals = 1;
  *return_vals  = values;

  if (nparams < 1)
    {
      values[0].data.d_status = GIMP_PDB_CALLING_ERROR;
      return;
    }

  run_mode = param[0].data.d_int32;

  if (strcmp (name, PLUG_IN_PROC_SET) == 0)
    {
      if (nparams < 3)
        status = GIMP_PDB_CALLING_ERROR;
      else
        status = lcms_icc_set (param[1].data.d_image, param[2].data.d_string);
    }
  else if (strcmp (name, PLUG_IN_PROC_SET_RGB) == 0)
    {
      if (nparams < 2)
        status = GIMP_PDB_CALLING_ERROR;
      else
        status = lcms_icc_set (param[1].data.d_image, NULL);
    }
  else if (strcmp (name, PLUG_IN_PROC_APPLY) == 0)
    {
      if (nparams < 3)
        status = GIMP_PDB_CALLING_ERROR;
      else
        status = lcms_icc_apply (param[1].data.d_image, param[2].data.d_string);
    }
  else if (strcmp (name, PLUG_IN_PROC_APPLY_RGB) == 0)
    {
      if (nparams < 2)
        status = GIMP_PDB_CALLING_ERROR;
      else
        status = lcms_icc_apply (param[1].data.d_image, NULL);
    }
  else if (strcmp (name, PLUG_IN_PROC_INFO) == 0)
    {
      gchar *name;
      gchar *desc;
      gchar *info;

      if (nparams < 2)
        status = GIMP_PDB_CALLING_ERROR;
      else
        status = lcms_icc_info (param[1].data.d_image, &name, &desc, &info);

      if (status == GIMP_PDB_SUCCESS)
        {
          *nreturn_vals = NUM_RETURN_VALS;

          values[PRODUCT_NAME].type          = GIMP_PDB_STRING;
          values[PRODUCT_NAME].data.d_string = name;

          values[PRODUCT_DESC].type          = GIMP_PDB_STRING;
          values[PRODUCT_DESC].data.d_string = desc;

          values[PRODUCT_INFO].type          = GIMP_PDB_STRING;
          values[PRODUCT_INFO].data.d_string = info;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  values[0].data.d_status = status;
}

static GimpPDBStatusType
lcms_icc_set (gint32       image,
              const gchar *filename)
{
  gboolean success;

  if (filename)
    {
      success = lcms_image_set_profile (image, filename);
    }
  else
    {
      GimpColorConfig *config = gimp_get_color_configuration ();

      success = lcms_image_set_profile (image, config->rgb_profile);

      g_object_unref (config);
    }

  return success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
}

static gchar *
lcms_icc_profile_get_name (cmsHPROFILE profile)
{
  const gchar *str = cmsTakeProductName (profile);

  if (! g_utf8_validate (str, -1, NULL))
    return g_strdup (_("(invalid UTF-8 string)"));

  return g_strdup (str);
}

static gchar *
lcms_icc_profile_get_desc (cmsHPROFILE profile)
{
  const gchar *str = cmsTakeProductDesc (profile);

  if (! g_utf8_validate (str, -1, NULL))
    return g_strdup (_("(invalid UTF-8 string)"));

  return g_strdup (str);
}

static gchar *
lcms_icc_profile_get_info (cmsHPROFILE profile)
{
  const gchar *str = cmsTakeProductInfo (profile);

  if (! g_utf8_validate (str, -1, NULL))
    return g_strdup (_("(invalid UTF-8 string)"));

  return g_strdup (str);
}

static gboolean
lcms_icc_profile_is_rgb (cmsHPROFILE profile)
{
  return (cmsGetColorSpace (profile) == icSigRgbData);
}

static GimpPDBStatusType
lcms_icc_apply (gint32       image,
                const gchar *filename)
{
  GimpColorConfig *config       = gimp_get_color_configuration ();
  cmsHPROFILE      src_profile  = NULL;
  cmsHPROFILE      dest_profile = NULL;

  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config),
                        GIMP_PDB_EXECUTION_ERROR);

  if (! filename)
    filename = config->rgb_profile;

  if (filename)
    {
      dest_profile = cmsOpenProfileFromFile (filename, "r");

      if (! dest_profile)
        {
          g_message (_("Could not open color profile from '%s'"),
                     gimp_filename_to_utf8 (filename));

          g_object_unref (config);
          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (! lcms_icc_profile_is_rgb (dest_profile))
        {
          g_message (_("Color profile '%s' is not for RGB color space."),
                     gimp_filename_to_utf8 (filename));

          cmsCloseProfile (dest_profile);
          g_object_unref (config);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  src_profile = lcms_image_get_profile (image);

  if (! src_profile && ! dest_profile)
    return GIMP_PDB_SUCCESS;

  if (! lcms_icc_profile_is_rgb (src_profile))
    {
      g_warning ("Attached color profile is not for RGB color space.");

      cmsCloseProfile (src_profile);
      src_profile = NULL;
    }

  if (! src_profile)
    src_profile = cmsCreate_sRGBProfile ();

  if (! dest_profile)
    dest_profile = cmsCreate_sRGBProfile ();

  if (lcms_image_set_profile (image, filename))
    {
      gchar *src  = lcms_icc_profile_get_desc (src_profile);
      gchar *dest = lcms_icc_profile_get_desc (dest_profile);

      /* ICC color profile conversion */
      gimp_progress_init_printf (_("Converting from '%s' to '%s'"), src, dest);

      g_printerr ("lcms: converting from '%s' to '%s'\n", src, dest);

      g_free (dest);
      g_free (src);

      switch (gimp_image_base_type (image))
        {
        case GIMP_RGB:
        case GIMP_GRAY:
          lcms_image_transform_pixels (image, src_profile, dest_profile);
          break;

        case GIMP_INDEXED:
          lcms_image_transform_cmap (image, src_profile, dest_profile);
          break;
        }

      gimp_progress_update (1.0);
   }

  cmsCloseProfile (src_profile);
  cmsCloseProfile (dest_profile);

  g_object_unref (config);

  return GIMP_PDB_SUCCESS;
}

static GimpPDBStatusType
lcms_icc_info (gint32   image,
               gchar  **name,
               gchar  **desc,
               gchar  **info)
{
  cmsHPROFILE  profile = lcms_image_get_profile (image);

  if (profile)
    {
      *name = lcms_icc_profile_get_name (profile);
      *desc = lcms_icc_profile_get_desc (profile);
      *info = lcms_icc_profile_get_info (profile);

      cmsCloseProfile (profile);
    }
  else
    {
      *name = g_strdup ("sRGB");
      *desc = g_strdup ("sRGB");
      *info = g_strdup (_("Default RGB working space"));
    }

  return GIMP_PDB_SUCCESS;
}

static cmsHPROFILE
lcms_image_get_profile (gint32 image)
{
  GimpParasite *parasite;
  cmsHPROFILE   profile = NULL;

  g_return_val_if_fail (image != -1, NULL);

  parasite = gimp_image_parasite_find (image, "icc-profile");

  if (parasite)
    {
      profile = cmsOpenProfileFromMem ((gpointer) gimp_parasite_data (parasite),
                                       gimp_parasite_data_size (parasite));

      /* FIXME: we leak the parasite, the data is used by the profile */

      if (! profile)
        g_message (_("Data attached as 'icc-profile' does not appear to "
                     "be an ICC color profile"));
    }
  else
    {
      profile = lcms_config_get_profile ();
    }

  return profile;
}

static gboolean
lcms_image_set_profile (gint32       image,
                        const gchar *filename)
{
  gboolean success = FALSE;

  g_return_val_if_fail (image != -1, FALSE);

  if (filename)
    {
      GMappedFile  *file;
      cmsHPROFILE   profile;
      GError       *error = NULL;

      file = g_mapped_file_new (filename, FALSE, &error);

      if (! file)
        {
          g_message (_("Could not open '%s' for reading: %s"),
                     gimp_filename_to_utf8 (filename), error->message);
          g_error_free (error);

          return FALSE;
        }

      /* check that this file is actually an ICC profile */
      profile = cmsOpenProfileFromMem (g_mapped_file_get_contents (file),
                                       g_mapped_file_get_length (file));

      if (profile)
        {
          gboolean rgb = lcms_icc_profile_is_rgb (profile);

          cmsCloseProfile (profile);

          if (rgb)
            {
              GimpParasite *parasite;

              parasite =
                gimp_parasite_new ("icc-profile", GIMP_PARASITE_PERSISTENT,
                                   g_mapped_file_get_length (file),
                                   g_mapped_file_get_contents (file));
              gimp_image_parasite_attach (image, parasite);
              gimp_parasite_free (parasite);

              success = TRUE;
            }
          else
            {
              g_message (_("Color profile '%s' is not for RGB color space."),
                         gimp_filename_to_utf8 (filename));
            }
        }
      else
        {
          g_message (_("'%s' does not appear to be an ICC color profile"),
                     gimp_filename_to_utf8 (filename));
        }

      g_mapped_file_free (file);
    }
  else
    {
      gimp_image_parasite_detach (image, "icc-profile");

      success = TRUE;
    }

  return success;
}

static void
lcms_image_transform_pixels (gint32       image,
                             cmsHPROFILE  src_profile,
                             cmsHPROFILE  dest_profile)
{
  cmsHTRANSFORM  transform   = NULL;
  DWORD          last_format = 0;
  gint          *layers;
  gint           num_layers;
  gint           i;

  layers = gimp_image_get_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    {
      GimpDrawable *drawable = gimp_drawable_get (layers[i]);
      DWORD         format;

      switch (drawable->bpp)
        {
        case 1:
          format = TYPE_GRAY_8;
          break;
        case 2:
          format = TYPE_GRAYA_8;
          break;
        case 3:
          format = TYPE_RGB_8;
          break;
        case 4:
          format = TYPE_RGBA_8;
          break;

        default:
          g_warning ("%s: unexpected bpp", G_GNUC_FUNCTION);
          continue;
        }

      if (! transform || format != last_format)
        {
          if (transform)
            cmsDeleteTransform (transform);

          transform = cmsCreateTransform (src_profile,  format,
                                          dest_profile, format,
                                          INTENT_PERCEPTUAL, 0);
          last_format = format;
        }

      lcms_drawable_transform (drawable, transform,
                               (gdouble) i / num_layers,
                               (gdouble) (i + 1) / num_layers);

      gimp_drawable_detach (drawable);
    }

  if (transform)
    cmsDeleteTransform(transform);

  g_free (layers);
}

static void
lcms_image_transform_cmap (gint32       image,
                           cmsHPROFILE  src_profile,
                           cmsHPROFILE  dest_profile)
{
  cmsHTRANSFORM   transform;
  guchar         *cmap;
  gint            num_colors;

  cmap = gimp_image_get_colormap (image, &num_colors);

  transform = cmsCreateTransform (src_profile,  TYPE_RGB_8,
                                  dest_profile, TYPE_RGB_8,
                                  INTENT_PERCEPTUAL, 0);

  cmsDoTransform (transform, cmap, cmap, num_colors * 3);

  cmsDeleteTransform(transform);

  gimp_image_set_colormap (image, cmap, num_colors);
}

static void
lcms_drawable_transform (GimpDrawable  *drawable,
                         cmsHTRANSFORM  transform,
                         gdouble        progress_start,
                         gdouble        progress_end)
{
  GimpPixelRgn	rgn;
  gpointer      pr;
  gdouble       range = progress_end - progress_start;
  guint         count = 0;
  guint         done  = 0;

  gimp_pixel_rgn_init (&rgn, drawable,
                       0, 0, drawable->width, drawable->height, TRUE, FALSE);

  for (pr = gimp_pixel_rgns_register (1, &rgn);
       pr != NULL;
       pr = gimp_pixel_rgns_process (pr))
    {
      guchar *data = rgn.data;
      gint    y;

      for (y = 0; y < rgn.h; y++)
        {
          cmsDoTransform (transform, data, data, rgn.w);

          data += rgn.rowstride;
        }

      done += rgn.h * rgn.w;

      if (count++ % 16 == 0)
        gimp_progress_update (progress_start +
                              (gdouble) done /
                              (drawable->width * drawable->height) * range);
    }

  gimp_progress_update (progress_end);

  gimp_drawable_flush (drawable);
  gimp_drawable_update (drawable->drawable_id,
                        0, 0, drawable->width, drawable->height);
}

static cmsHPROFILE
lcms_config_get_profile (void)
{
  GimpColorConfig *config  = gimp_get_color_configuration ();
  cmsHPROFILE      profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), NULL);

  if (config->rgb_profile)
    {
      profile = cmsOpenProfileFromFile (config->rgb_profile, "r");

      if (! profile)
        g_message (_("Could not open ICC profile from '%s'"),
                   gimp_filename_to_utf8 (config->rgb_profile));
    }

  g_object_unref (config);

  return profile;
}
