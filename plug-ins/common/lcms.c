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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"


#define PLUG_IN_BINARY          "lcms"
#define PLUG_IN_ROLE            "gimp-lcms"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-profile-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-profile-apply-rgb"


enum
{
  PROC_APPLY,
  PROC_APPLY_RGB,
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

static GimpPDBStatusType  lcms_icc_apply     (GimpColorConfig   *config,
                                              GimpRunMode        run_mode,
                                              gint32             image,
                                              GFile             *file,
                                              GimpColorRenderingIntent intent,
                                              gboolean           bpc,
                                              gboolean          *dont_ask);

static gboolean     lcms_icc_apply_dialog    (gint32            image,
                                              GimpColorProfile *src_profile,
                                              GimpColorProfile *dest_profile,
                                              gboolean         *dont_ask);


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

static const Procedure procedures[] =
{
  { PLUG_IN_PROC_APPLY,     2 },
  { PLUG_IN_PROC_APPLY_RGB, 2 }
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

  config = gimp_get_color_configuration ();
  g_return_if_fail (config != NULL);

  intent = config->display_intent;
  bpc    = (intent == GIMP_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC);

  switch (proc)
    {
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

    case PROC_APPLY_RGB:
      run_mode = param[0].data.d_int32;
      image    = param[1].data.d_image;
      if (nparams > 2)
        intent = param[2].data.d_int32;
      if (nparams > 3)
        bpc    = param[3].data.d_int32 ? TRUE : FALSE;
      break;
    }

  switch (proc)
    {
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
    }

 done:
  if (config)
    g_object_unref (config);

  if (file)
    g_object_unref (file);

  values[0].data.d_status = status;
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
  GimpPDBStatusType  status       = GIMP_PDB_SUCCESS;
  GimpColorProfile  *src_profile  = NULL;
  GimpColorProfile  *dest_profile = NULL;

  g_return_val_if_fail (GIMP_IS_COLOR_CONFIG (config), GIMP_PDB_CALLING_ERROR);
  g_return_val_if_fail (image != -1, GIMP_PDB_CALLING_ERROR);

  if (file)
    g_object_ref (file);
  else if (config->rgb_profile)
    file = g_file_new_for_path (config->rgb_profile);

  if (file)
    {
      GError *error = NULL;

      dest_profile = gimp_color_profile_new_from_file (file, &error);

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

          g_object_unref (dest_profile);
          g_object_unref (file);
          return GIMP_PDB_EXECUTION_ERROR;
        }
    }

  src_profile = gimp_image_get_effective_color_profile (image);

  if (! dest_profile)
    dest_profile = gimp_color_profile_new_srgb ();

  if (gimp_color_profile_is_equal (src_profile, dest_profile))
    {
      g_printerr ("lcms: skipping conversion because profiles are equal:\n"
                  " %s\n"
                  " %s\n",
                  gimp_color_profile_get_label (src_profile),
                  gimp_color_profile_get_label (dest_profile));

      g_object_unref (src_profile);
      g_object_unref (dest_profile);

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
      ! gimp_image_convert_color_profile (image, dest_profile, intent, bpc))
    {
      status = GIMP_PDB_EXECUTION_ERROR;
    }

  g_object_unref (src_profile);
  g_object_unref (dest_profile);

  if (file)
    g_object_unref (file);

  return status;
}

static GtkWidget *
lcms_icc_profile_src_label_new (gint32            image,
                                GimpColorProfile *profile)
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

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",   gimp_color_profile_get_label (profile),
                        "wrap",    TRUE,
                        "justify", GTK_JUSTIFY_LEFT,
                        "xalign",  0.0,
                        "yalign",  0.0,
                        "xpad",    24,
                        NULL);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  return vbox;
}

static GtkWidget *
lcms_icc_profile_dest_label_new (GimpColorProfile *profile)
{
  GtkWidget *label;
  gchar     *text;

  text = g_strdup_printf (_("Convert the image to the RGB working space (%s)?"),
                          gimp_color_profile_get_label (profile));

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
lcms_icc_apply_dialog (gint32            image,
                       GimpColorProfile *src_profile,
                       GimpColorProfile *dest_profile,
                       gboolean         *dont_ask)
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
