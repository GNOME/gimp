/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Color management plug-in based on littleCMS
 * Copyright (C) 2006, 2007  Sven Neumann <sven@gimp.org>
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY          "lcms"
#define PLUG_IN_ROLE            "gimp-lcms"

#define PLUG_IN_PROC_SET        "plug-in-icc-profile-set"
#define PLUG_IN_PROC_SET_RGB    "plug-in-icc-profile-set-rgb"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-profile-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-profile-apply-rgb"

#define PLUG_IN_PROC_INFO       "plug-in-icc-profile-info"
#define PLUG_IN_PROC_FILE_INFO  "plug-in-icc-profile-file-info"


enum
{
  STATUS,
  PROFILE_NAME,
  PROFILE_DESC,
  PROFILE_INFO,
  NUM_RETURN_VALS
};

enum
{
  PROC_SET,
  PROC_SET_RGB,
  PROC_APPLY,
  PROC_APPLY_RGB,
  PROC_INFO,
  PROC_FILE_INFO,
  NONE
};

typedef struct
{
  const gchar *name;
  const gint   min_params;
} Procedure;

typedef struct
{
  GimpColorRenderingIntent intent;
  gboolean                 bpc;
} LcmsValues;


static void  query (void);
static void  run   (const gchar      *name,
                    gint              nparams,
                    const GimpParam  *param,
                    gint             *nreturn_vals,
                    GimpParam       **return_vals);

static GimpPDBStatusType  lcms_icc_set       (GimpColorConfig  *config,
                                              gint32            image,
                                              GFile            *file);
static GimpPDBStatusType  lcms_icc_apply     (GimpColorConfig  *config,
                                              GimpRunMode       run_mode,
                                              gint32            image,
                                              GFile            *file,
                                              GimpColorRenderingIntent intent,
                                              gboolean          bpc,
                                              gboolean         *dont_ask);
static GimpPDBStatusType  lcms_icc_info      (GimpColorConfig  *config,
                                              gint32            image,
                                              gchar           **name,
                                              gchar           **desc,
                                              gchar           **info);
static GimpPDBStatusType  lcms_icc_file_info (GFile            *file,
                                              gchar           **name,
                                              gchar           **desc,
                                              gchar           **info,
                                              GError          **error);

static cmsHPROFILE  lcms_image_get_profile       (GimpColorConfig *config,
                                                  gint32           image,
                                                  GError         **error);
static gboolean     lcms_image_set_profile       (gint32           image,
                                                  cmsHPROFILE      profile,
                                                  GFile           *file);
static gboolean     lcms_image_apply_profile     (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GFile           *file,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);
static void         lcms_image_transform_rgb     (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);
static void         lcms_layers_transform_rgb    (gint            *layers,
                                                  gint             num_layers,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);
static void         lcms_image_transform_indexed (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  GimpColorRenderingIntent intent,
                                                  gboolean          bpc);

static gboolean     lcms_icc_apply_dialog        (gint32           image,
                                                  cmsHPROFILE      src_profile,
                                                  cmsHPROFILE      dest_profile,
                                                  gboolean        *dont_ask);

static GimpPDBStatusType  lcms_dialog            (GimpColorConfig *config,
                                                  gint32           image,
                                                  gboolean         apply,
                                                  LcmsValues      *values);


static const GimpParamDef set_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" }
};
static const GimpParamDef set_rgb_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
};
static const GimpParamDef apply_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" },
  { GIMP_PDB_INT32,  "intent",       "Rendering intent (enum GimpColorRenderingIntent)" },
  { GIMP_PDB_INT32,  "bpc",          "Black point compensation"         }
};
static const GimpParamDef apply_rgb_args[] =
{
  { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"     },
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
  { GIMP_PDB_INT32,  "intent",       "Rendering intent (enum GimpColorRenderingIntent)" },
  { GIMP_PDB_INT32,  "bpc",          "Black point compensation"         }
};
static const GimpParamDef info_args[] =
{
  { GIMP_PDB_IMAGE,  "image",        "Input image"                      },
};
static const GimpParamDef file_info_args[] =
{
  { GIMP_PDB_STRING, "profile",      "Filename of an ICC color profile" }
};

static const Procedure procedures[] =
{
  { PLUG_IN_PROC_SET,       2 },
  { PLUG_IN_PROC_SET_RGB,   2 },
  { PLUG_IN_PROC_APPLY,     2 },
  { PLUG_IN_PROC_APPLY_RGB, 2 },
  { PLUG_IN_PROC_INFO,      1 },
  { PLUG_IN_PROC_FILE_INFO, 1 }
};

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
  static const GimpParamDef info_return_vals[] =
  {
    { GIMP_PDB_STRING, "profile-name", "Name"        },
    { GIMP_PDB_STRING, "profile-desc", "Description" },
    { GIMP_PDB_STRING, "profile-info", "Info"        }
  };

  gimp_install_procedure (PLUG_IN_PROC_SET,
                          N_("Set a color profile on the image"),
                          "This procedure sets an ICC color profile on an "
                          "image using the 'icc-profile' parasite. It does "
                          "not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("_Assign Color Profile..."),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (set_args), 0,
                          set_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_SET_RGB,
                          "Set the default RGB color profile on the image",
                          "This procedure sets the user-configured RGB "
                          "profile on an image using the 'icc-profile' "
                          "parasite. If no RGB profile is configured, sRGB "
                          "is assumed and the parasite is unset. This "
                          "procedure does not do any color conversion.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Assign default RGB Profile"),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (set_rgb_args), 0,
                          set_rgb_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_APPLY,
                          _("Apply a color profile on the image"),
                          "This procedure transform from the image's color "
                          "profile (or the default RGB profile if none is "
                          "set) to the given ICC color profile. Only RGB "
                          "color profiles are accepted. The profile "
                          "is then set on the image using the 'icc-profile' "
                          "parasite.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("_Convert to Color Profile..."),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (apply_args), 0,
                          apply_args, NULL);

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
                          "2006, 2007",
                          N_("Convert to default RGB Profile"),
                          "RGB*, INDEXED*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (apply_rgb_args), 0,
                          apply_rgb_args, NULL);

  gimp_install_procedure (PLUG_IN_PROC_INFO,
                          "Retrieve information about an image's color profile",
                          "This procedure returns information about the RGB "
                          "color profile attached to an image. If no RGB "
                          "color profile is attached, sRGB is assumed.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Image Color Profile Information"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (info_args),
                          G_N_ELEMENTS (info_return_vals),
                          info_args, info_return_vals);

  gimp_install_procedure (PLUG_IN_PROC_FILE_INFO,
                          "Retrieve information about a color profile",
                          "This procedure returns information about an ICC "
                          "color profile on disk.",
                          "Sven Neumann",
                          "Sven Neumann",
                          "2006, 2007",
                          N_("Color Profile Information"),
                          "*",
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (file_info_args),
                          G_N_ELEMENTS (info_return_vals),
                          file_info_args, info_return_vals);

  gimp_plugin_menu_register (PLUG_IN_PROC_SET,
                             "<Image>/Image/Mode/Color Profile");
  gimp_plugin_menu_register (PLUG_IN_PROC_APPLY,
                             "<Image>/Image/Mode/Color Profile");
}

static void
run (const gchar      *name,
     gint              nparams,
     const GimpParam  *param,
     gint             *nreturn_vals,
     GimpParam       **return_vals)
{
  GimpPDBStatusType         status   = GIMP_PDB_CALLING_ERROR;
  gint                      proc     = NONE;
  GimpRunMode               run_mode = GIMP_RUN_NONINTERACTIVE;
  gint32                    image    = -1;
  GFile                    *file     = NULL;
  GimpColorConfig          *config   = NULL;
  gboolean                  dont_ask = FALSE;
  GimpColorRenderingIntent  intent;
  gboolean                  bpc;
  static GimpParam          values[6];

  INIT_I18N ();
  gegl_init (NULL, NULL);

  values[0].type = GIMP_PDB_STATUS;

  *nreturn_vals = 1;
  *return_vals  = values;

  for (proc = 0; proc < G_N_ELEMENTS (procedures); proc++)
    {
      if (strcmp (name, procedures[proc].name) == 0)
        break;
    }

  if (proc == NONE)
    goto done;

  if (nparams < procedures[proc].min_params)
    goto done;

  if (proc != PROC_FILE_INFO)
    {
      config = gimp_get_color_configuration ();
      /* Later code relies on config != NULL if proc != PROC_FILE_INFO */
      g_return_if_fail (config != NULL);
      intent = config->display_intent;
    }
  else
    intent = GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL;

  bpc = (intent == GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  switch (proc)
    {
    case PROC_SET:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        file = g_file_new_for_path (param[2].data.d_string);
      break;

    case PROC_APPLY:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        file = g_file_new_for_path (param[2].data.d_string);
      if (nparams > 3)
        intent = param[3].data.d_int32;
      if (nparams > 4)
        bpc    = param[4].data.d_int32 ? TRUE : FALSE;
      break;

    case PROC_SET_RGB:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      break;

    case PROC_APPLY_RGB:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        intent = param[2].data.d_int32;
      if (nparams > 3)
        bpc    = param[3].data.d_int32 ? TRUE : FALSE;
      break;

    case PROC_INFO:
      image    = param[0].data.d_image;
      break;

    case PROC_FILE_INFO:
      file = g_file_new_for_path (param[0].data.d_string);
      break;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      LcmsValues values = { intent, bpc };

      switch (proc)
        {
        case PROC_SET:
          status = lcms_dialog (config, image, FALSE, &values);
          goto done;

        case PROC_APPLY:
          gimp_get_data (name, &values);

          status = lcms_dialog (config, image, TRUE, &values);

          if (status == GIMP_PDB_SUCCESS)
            gimp_set_data (name, &values, sizeof (LcmsValues));
          goto done;

        default:
          break;
        }
    }

  switch (proc)
    {
    case PROC_SET:
    case PROC_SET_RGB:
      status = lcms_icc_set (config, image, file);
      break;

    case PROC_APPLY:
    case PROC_APPLY_RGB:
      status = lcms_icc_apply (config, run_mode,
                               image, file, intent, bpc,
                               &dont_ask);

      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          *nreturn_vals = 2;

          values[1].type         = GIMP_PDB_INT32;
          values[1].data.d_int32 = dont_ask;
        }
      break;

    case PROC_INFO:
    case PROC_FILE_INFO:
      {
        gchar  *name  = NULL;
        gchar  *desc  = NULL;
        gchar  *info  = NULL;
        GError *error = NULL;

        if (proc == PROC_INFO)
          status = lcms_icc_info (config, image, &name, &desc, &info);
        else
          status = lcms_icc_file_info (file, &name, &desc, &info, &error);

        if (status == GIMP_PDB_SUCCESS)
          {
            *nreturn_vals = NUM_RETURN_VALS;

            values[PROFILE_NAME].type          = GIMP_PDB_STRING;
            values[PROFILE_NAME].data.d_string = name;

            values[PROFILE_DESC].type          = GIMP_PDB_STRING;
            values[PROFILE_DESC].data.d_string = desc;

            values[PROFILE_INFO].type          = GIMP_PDB_STRING;
            values[PROFILE_INFO].data.d_string = info;
          }
        else if (error)
          {
            *nreturn_vals = 2;

            values[1].type          = GIMP_PDB_STRING;
            values[1].data.d_string = error->message;
          }
      }
      break;
    }

 done:
  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  if (config)
    g_object_unref (config);

  if (file)
    g_object_unref (file);

  values[0].data.d_status = status;
}

static GimpPDBStatusType
lcms_icc_set (GimpColorConfig *config,
              gint32           image,
              GFile           *file)
{
  gboolean success;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  if (file)
    g_object_ref (file);
  else if (config->rgb_profile)
    file = g_file_new_for_path (config->rgb_profile);

  success = lcms_image_set_profile (image, NULL, file);

  if (file)
    g_object_unref (file);

  return success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR;
}

static GimpPDBStatusType
lcms_icc_apply (GimpColorConfig          *config,
                GimpRunMode               run_mode,
                gint32                    image,
                GFile                    *file,
                GimpColorRenderingIntent  intent,
                gboolean                  bpc,
                gboolean                 *dont_ask)
{
  GimpPDBStatusType status       = GIMP_PDB_SUCCESS;
  cmsHPROFILE       src_profile  = NULL;
  cmsHPROFILE       dest_profile = NULL;
  GError           *error        = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  if (file)
    g_object_ref (file);
  else if (config->rgb_profile)
    file = g_file_new_for_path (config->rgb_profile);

  if (file)
    {
      GError *error = NULL;

      dest_profile = gimp_color_profile_open_from_file (file, &error);

      if (! dest_profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);

          return GIMP_PDB_EXECUTION_ERROR;
        }

      if (! gimp_color_profile_is_rgb (dest_profile))
        {
          g_message (_("Color profile '%s' is not for RGB color space."),
                     gimp_file_get_utf8_name (file));

          gimp_color_profile_close (dest_profile);
          g_object_unref (file);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  src_profile = lcms_image_get_profile (config, image, &error);

  if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }

  if (! src_profile && ! dest_profile)
    return GIMP_PDB_SUCCESS;

  if (! src_profile)
    src_profile = gimp_color_profile_new_srgb ();

  if (! dest_profile)
    dest_profile = gimp_color_profile_new_srgb ();

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    {
      gchar *src_label  = gimp_color_profile_get_label (src_profile);
      gchar *dest_label = gimp_color_profile_get_label (dest_profile);

      gimp_color_profile_close (src_profile);
      gimp_color_profile_close (dest_profile);

      g_printerr ("lcms: skipping conversion because profiles seem to be equal:\n");
      g_printerr (" %s\n", src_label);
      g_printerr (" %s\n", dest_label);

      g_free (src_label);
      g_free (dest_label);

      if (file)
        g_object_unref (file);

      return GIMP_PDB_SUCCESS;
    }

  if (run_mode == GIMP_RUN_INTERACTIVE &&
      ! lcms_icc_apply_dialog (image, src_profile, dest_profile, dont_ask))
    {
      status = GIMP_PDB_CANCEL;
    }

  if (status == GIMP_PDB_SUCCESS &&
      ! lcms_image_apply_profile (image,
                                  src_profile, dest_profile, file,
                                  intent, bpc))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  gimp_color_profile_close (src_profile);
  gimp_color_profile_close (dest_profile);

  if (file)
    g_object_unref (file);

  return status;
}

static GimpPDBStatusType
lcms_icc_info (GimpColorConfig *config,
               gint32           image,
               gchar          **name,
               gchar          **desc,
               gchar          **info)
{
  cmsHPROFILE  profile;
  GError      *error = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  profile = lcms_image_get_profile (config, image, &error);

  if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }

  if (! profile)
    profile = gimp_color_profile_new_srgb ();

  if (name) *name = gimp_color_profile_get_model (profile);
  if (desc) *desc = gimp_color_profile_get_description (profile);
  if (info) *info = gimp_color_profile_get_summary (profile);

  gimp_color_profile_close (profile);

  return GIMP_PDB_SUCCESS;
}

static GimpPDBStatusType
lcms_icc_file_info (GFile   *file,
                    gchar  **name,
                    gchar  **desc,
                    gchar  **info,
                    GError **error)
{
  cmsHPROFILE profile;

  profile = gimp_color_profile_open_from_file (file, error);

  if (! profile)
    return GIMP_PDB_EXECUTION_ERROR;

  *name = gimp_color_profile_get_model (profile);
  *desc = gimp_color_profile_get_description (profile);
  *info = gimp_color_profile_get_summary (profile);

  gimp_color_profile_close (profile);

  return GIMP_PDB_SUCCESS;
}

static cmsHPROFILE
lcms_image_get_profile (GimpColorConfig  *config,
                        gint32            image,
                        GError          **error)
{
  GimpParasite *parasite;
  cmsHPROFILE   profile = NULL;

  g_return_val_if_fail (image != -1, NULL);

  parasite = gimp_image_get_parasite (image, "icc-profile");

  if (parasite)
    {
      profile = gimp_color_profile_open_from_data (gimp_parasite_data (parasite),
                                                   gimp_parasite_data_size (parasite),
                                                   error);
      if (! profile)
        g_prefix_error (error, _("Error parsing 'icc-profile': "));

      gimp_parasite_free (parasite);
    }
  else
    {
      profile = gimp_color_config_get_rgb_color_profile (config, error);
    }

  return profile;
}

static gboolean
lcms_image_set_profile (gint32       image,
                        cmsHPROFILE  profile,
                        GFile       *file)
{
  g_return_val_if_fail (image != -1, FALSE);

  if (file)
    {
      cmsHPROFILE   file_profile;
      GimpParasite *parasite;
      guint8       *profile_data;
      gsize         profile_length;
      GError       *error = NULL;

      /* check that this file is actually an ICC profile */
      file_profile = gimp_color_profile_open_from_file (file, &error);

      if (! file_profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);

          return FALSE;
        }

      profile_data = gimp_color_profile_save_to_data (file_profile,
                                                      &profile_length,
                                                      &error);
      gimp_color_profile_close (file_profile);

      if (! profile_data)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);

          return FALSE;
        }

      gimp_image_undo_group_start (image);

      parasite = gimp_parasite_new ("icc-profile",
                                    GIMP_PARASITE_PERSISTENT |
                                    GIMP_PARASITE_UNDOABLE,
                                    profile_length, profile_data);

      g_free (profile_data);

      gimp_image_attach_parasite (image, parasite);
      gimp_parasite_free (parasite);
    }
  else
    {
      gimp_image_undo_group_start (image);

      gimp_image_detach_parasite (image, "icc-profile");
    }

  gimp_image_detach_parasite (image, "icc-profile-name");

  gimp_image_undo_group_end (image);

  return TRUE;
}

static gboolean
lcms_image_apply_profile (gint32                    image,
                          cmsHPROFILE               src_profile,
                          cmsHPROFILE               dest_profile,
                          GFile                    *file,
                          GimpColorRenderingIntent  intent,
                          gboolean                  bpc)
{
  gchar  *src_label;
  gchar  *dest_label;
  gint32  saved_selection = -1;

  gimp_image_undo_group_start (image);

  if (! lcms_image_set_profile (image, dest_profile, file))
    {
      gimp_image_undo_group_end (image);

      return FALSE;
    }

  src_label  = gimp_color_profile_get_label (src_profile);
  dest_label = gimp_color_profile_get_label (dest_profile);

  gimp_progress_init_printf (_("Converting from '%s' to '%s'"),
                             src_label, dest_label);

  g_printerr ("lcms: converting from '%s' to '%s'\n", src_label, dest_label);

  g_free (dest_label);
  g_free (src_label);

  if (! gimp_selection_is_empty (image))
    {
      saved_selection = gimp_selection_save (image);
      gimp_selection_none (image);
    }

  switch (gimp_image_base_type (image))
    {
    case GIMP_RGB:
      lcms_image_transform_rgb (image,
                                src_profile, dest_profile, intent, bpc);
      break;

    case GIMP_GRAY:
      g_warning ("colorspace conversion not implemented for "
                 "grayscale images");
      break;

    case GIMP_INDEXED:
      lcms_image_transform_indexed (image,
                                    src_profile, dest_profile, intent, bpc);
      break;
    }

  if (saved_selection != -1)
    {
      gimp_image_select_item (image, GIMP_CHANNEL_OP_REPLACE, saved_selection);
      gimp_image_remove_channel (image, saved_selection);
    }

  gimp_progress_update (1.0);

  gimp_image_undo_group_end (image);

  return TRUE;
}

static void
lcms_image_transform_rgb (gint32                    image,
                          cmsHPROFILE               src_profile,
                          cmsHPROFILE               dest_profile,
                          GimpColorRenderingIntent  intent,
                          gboolean                  bpc)
{
  gint *layers;
  gint  num_layers;

  layers = gimp_image_get_layers (image, &num_layers);

  lcms_layers_transform_rgb (layers, num_layers,
                             src_profile, dest_profile,
                             intent, bpc);

  g_free (layers);
}

static void
lcms_layers_transform_rgb (gint                     *layers,
                           gint                      num_layers,
                           cmsHPROFILE               src_profile,
                           cmsHPROFILE               dest_profile,
                           GimpColorRenderingIntent  intent,
                           gboolean                  bpc)
{
  gint i;

  for (i = 0; i < num_layers; i++)
    {
      gint32           layer_id = layers[i];
      const Babl      *iter_format = NULL;
      cmsUInt32Number  lcms_format = 0;
      cmsHTRANSFORM    transform;
      gint            *children;
      gint             num_children;

      children = gimp_item_get_children (layer_id, &num_children);

      if (children)
        {
          lcms_layers_transform_rgb (children, num_children,
                                     src_profile, dest_profile,
                                     intent, bpc);

          g_free (children);

          continue;
        }

      iter_format = gimp_color_profile_get_format (gimp_drawable_get_format (layer_id),
                                                   &lcms_format);

      transform = cmsCreateTransform (src_profile,  lcms_format,
                                      dest_profile, lcms_format,
                                      intent,
                                      cmsFLAGS_NOOPTIMIZE |
                                      (bpc ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0));

      if (transform)
        {
          GeglBuffer         *src_buffer;
          GeglBuffer         *dest_buffer;
          GeglBufferIterator *iter;
          gint                layer_width;
          gint                layer_height;
          gint                layer_bpp;
          gboolean            layer_alpha;
          gdouble             progress_start = (gdouble) i / num_layers;
          gdouble             progress_end   = (gdouble) (i + 1) / num_layers;
          gdouble             range          = progress_end - progress_start;
          gint                count          = 0;
          gint                done           = 0;

          src_buffer   = gimp_drawable_get_buffer (layer_id);
          dest_buffer  = gimp_drawable_get_shadow_buffer (layer_id);
          layer_width  = gegl_buffer_get_width (src_buffer);
          layer_height = gegl_buffer_get_height (src_buffer);
          layer_bpp    = babl_format_get_bytes_per_pixel (iter_format);
          layer_alpha  = babl_format_has_alpha (iter_format);

          iter = gegl_buffer_iterator_new (src_buffer, NULL, 0,
                                           iter_format,
                                           GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

          gegl_buffer_iterator_add (iter, dest_buffer, NULL, 0,
                                    iter_format,
                                    GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

          while (gegl_buffer_iterator_next (iter))
            {
              /*  lcms doesn't touch the alpha channel, simply
               *  copy everything to dest before the transform
               */
              if (layer_alpha)
                memcpy (iter->data[1], iter->data[0],
                        iter->length * layer_bpp);

              cmsDoTransform (transform,
                              iter->data[0], iter->data[1], iter->length);
            }

          g_object_unref (src_buffer);
          g_object_unref (dest_buffer);

          gimp_drawable_merge_shadow (layer_id, TRUE);
          gimp_drawable_update (layer_id, 0, 0, layer_width, layer_height);

          if (count++ % 32 == 0)
            {
              gimp_progress_update (progress_start +
                                    (gdouble) done /
                                    (layer_width * layer_height) * range);
            }

          cmsDeleteTransform (transform);
        }
    }
}

static void
lcms_image_transform_indexed (gint32                    image,
                              cmsHPROFILE               src_profile,
                              cmsHPROFILE               dest_profile,
                              GimpColorRenderingIntent  intent,
                              gboolean                  bpc)
{
  cmsHTRANSFORM    transform;
  guchar          *cmap;
  gint             n_cmap_bytes;
  cmsUInt32Number  format = TYPE_RGB_8;

  cmap = gimp_image_get_colormap (image, &n_cmap_bytes);

  transform = cmsCreateTransform (src_profile,  format,
                                  dest_profile, format,
                                  intent,
                                  cmsFLAGS_NOOPTIMIZE |
                                  (bpc ? cmsFLAGS_BLACKPOINTCOMPENSATION : 0));

  if (transform)
    {
      cmsDoTransform (transform, cmap, cmap, n_cmap_bytes / 3);
      cmsDeleteTransform (transform);
    }
  else
    {
      g_warning ("cmsCreateTransform() failed!");
    }

  gimp_image_set_colormap (image, cmap, n_cmap_bytes);
}

static GtkWidget *
lcms_icc_profile_src_label_new (gint32       image,
                                cmsHPROFILE  profile)
{
  GtkWidget *vbox;
  GtkWidget *label;
  gchar     *name;
  gchar     *text;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  name = gimp_image_get_name (image);
  text = g_strdup_printf (_("The image '%s' has an embedded color profile:"),
                          name);
  g_free (name);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   text,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  g_free (text);

  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  name = gimp_color_profile_get_label (profile);
  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   name,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        "xpad",    24,
                        NULL);
  g_free (name);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return vbox;
}

static GtkWidget *
lcms_icc_profile_dest_label_new (cmsHPROFILE  profile)
{
  GtkWidget *label;
  gchar     *name;
  gchar     *text;

  name = gimp_color_profile_get_label (profile);
  text = g_strdup_printf (_("Convert the image to the RGB working space (%s)?"),
                          name);
  g_free (name);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   text,
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        NULL);
  g_free (text);

  return label;
}

static gboolean
lcms_icc_apply_dialog (gint32       image,
                       cmsHPROFILE  src_profile,
                       cmsHPROFILE  dest_profile,
                       gboolean    *dont_ask)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *toggle = NULL;
  gboolean   run;

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (_("Convert to RGB working space?"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func, PLUG_IN_PROC_APPLY,

                            _("_Keep"),    GTK_RESPONSE_CANCEL,

                            NULL);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("_Convert"), GTK_RESPONSE_OK);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_icon_name ("gtk-convert",
                                                      GTK_ICON_SIZE_BUTTON));

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = lcms_icc_profile_src_label_new (image, src_profile);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  label = lcms_icc_profile_dest_label_new (dest_profile);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  if (dont_ask)
    {
      toggle = gtk_check_button_new_with_mnemonic (_("_Don't ask me again"));
      gtk_box_pack_end (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
      gtk_widget_show (toggle);
    }

  run = (gimp_dialog_run (GIMP_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (dont_ask)
    {
      *dont_ask = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (toggle));
    }

  gtk_widget_destroy (dialog);

  return run;
}

static GtkWidget *
lcms_icc_combo_box_new (GimpColorConfig *config,
                        const gchar     *filename)
{
  GtkWidget   *combo;
  GtkWidget   *dialog;
  gchar       *history;
  gchar       *label;
  gchar       *name;
  const gchar *rgb_filename = NULL;
  cmsHPROFILE  profile      = NULL;
  GError      *error        = NULL;

  dialog = gimp_color_profile_chooser_dialog_new (_("Select destination profile"));

  history = gimp_personal_rc_file ("profilerc");
  combo = gimp_color_profile_combo_box_new (dialog, history);
  g_free (history);

  profile = gimp_color_config_get_rgb_color_profile (config, &error);

  if (profile)
    {
      rgb_filename = config->rgb_profile;
    }
  else if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }

  if (! profile)
    profile = gimp_color_profile_new_srgb ();

  name = gimp_color_profile_get_label (profile);
  label = g_strdup_printf (_("RGB workspace (%s)"), name);
  g_free (name);

  gimp_color_profile_close (profile);

  gimp_color_profile_combo_box_add (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                    rgb_filename, label);
  g_free (label);

  gimp_color_profile_combo_box_set_active (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                           filename, NULL);

  return combo;
}

static GimpPDBStatusType
lcms_dialog (GimpColorConfig *config,
             gint32           image,
             gboolean         apply,
             LcmsValues      *values)
{
  GimpColorProfileComboBox *box;
  GtkWidget                *dialog;
  GtkWidget                *main_vbox;
  GtkWidget                *frame;
  GtkWidget                *label;
  GtkWidget                *combo;
  cmsHPROFILE               src_profile;
  gchar                    *name;
  gboolean                  success = FALSE;
  gboolean                  run;
  GError                   *error   = NULL;

  src_profile = lcms_image_get_profile (config, image, &error);

  if (error)
    {
      g_message ("%s", error->message);
      g_clear_error (&error);
    }

  if (! src_profile)
    src_profile = gimp_color_profile_new_srgb ();

  gimp_ui_init (PLUG_IN_BINARY, FALSE);

  dialog = gimp_dialog_new (apply ?
                            _("Convert to ICC Color Profile") :
                            _("Assign ICC Color Profile"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            gimp_standard_help_func,
                            apply ? PLUG_IN_PROC_APPLY : PLUG_IN_PROC_SET,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,

                            apply ? GTK_STOCK_CONVERT : _("_Assign"),
                            GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gimp_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  frame = gimp_frame_new (_("Current Color Profile"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  name = gimp_color_profile_get_label (src_profile);

  label = gtk_label_new (name);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_container_add (GTK_CONTAINER (frame), label);
  gtk_widget_show (label);

  g_free (name);

  frame = gimp_frame_new (apply ? _("Convert to") : _("Assign"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  combo = lcms_icc_combo_box_new (config, NULL);
  gtk_container_add (GTK_CONTAINER (frame), combo);
  gtk_widget_show (combo);

  box = GIMP_COLOR_PROFILE_COMBO_BOX (combo);

  if (apply)
    {
      GtkWidget *vbox;
      GtkWidget *hbox;
      GtkWidget *toggle;

      vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
      gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);
      gtk_widget_show (vbox);

      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
      gtk_widget_show (hbox);

      label = gtk_label_new_with_mnemonic (_("_Rendering Intent:"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      combo = gimp_enum_combo_box_new (GIMP_TYPE_COLOR_RENDERING_INTENT);
      gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
      gtk_widget_show (combo);

      gimp_int_combo_box_connect (GIMP_INT_COMBO_BOX (combo),
                                  values->intent,
                                  G_CALLBACK (gimp_int_combo_box_get_active),
                                  &values->intent);

      gtk_label_set_mnemonic_widget (GTK_LABEL (label), combo);

      toggle =
        gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), values->bpc);
      gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
      gtk_widget_show (toggle);

      g_signal_connect (toggle, "toggled",
                        G_CALLBACK (gimp_toggle_button_update),
                        &values->bpc);
    }

  while ((run = gimp_dialog_run (GIMP_DIALOG (dialog))) == GTK_RESPONSE_OK)
    {
      gchar       *filename = gimp_color_profile_combo_box_get_active (box);
      GFile       *file     = NULL;
      cmsHPROFILE  dest_profile;

      gtk_widget_set_sensitive (dialog, FALSE);

      if (filename)
        file = g_file_new_for_path (filename);

      if (file)
        {
          GError *error = NULL;

          dest_profile = gimp_color_profile_open_from_file (file, &error);

          if (! dest_profile)
            {
              g_message ("%s", error->message);
              g_clear_error (&error);
            }
        }
      else
        {
          dest_profile = gimp_color_profile_new_srgb ();
        }

      if (dest_profile)
        {
          if (gimp_color_profile_is_rgb (dest_profile))
            {
              if (apply)
                success = lcms_image_apply_profile (image,
                                                    src_profile, dest_profile,
                                                    file,
                                                    values->intent,
                                                    values->bpc);
              else
                success = lcms_image_set_profile (image,
                                                  dest_profile, file);
            }
          else
            {
              g_message (_("Destination profile is not for RGB color space."));
            }

          gimp_color_profile_close (dest_profile);
        }

      if (file)
        g_object_unref (file);

      if (success)
        break;
      else
        gtk_widget_set_sensitive (dialog, TRUE);
    }

  gtk_widget_destroy (dialog);

  gimp_color_profile_close (src_profile);

  return (run ?
          (success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR) :
          GIMP_PDB_CANCEL);
}
