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

#define PLUG_IN_PROC_SET        "plug-in-icc-profile-set"
#define PLUG_IN_PROC_SET_RGB    "plug-in-icc-profile-set-rgb"

#define PLUG_IN_PROC_APPLY      "plug-in-icc-profile-apply"
#define PLUG_IN_PROC_APPLY_RGB  "plug-in-icc-profile-apply-rgb"


enum
{
  PROC_SET,
  PROC_SET_RGB,
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

static GimpPDBStatusType  lcms_icc_set       (GimpColorConfig   *config,
                                              gint32             image,
                                              GFile             *file);
static GimpPDBStatusType  lcms_icc_apply     (GimpColorConfig   *config,
                                              GimpRunMode        run_mode,
                                              gint32             image,
                                              GFile             *file,
                                              GimpColorRenderingIntent intent,
                                              gboolean           bpc,
                                              gboolean          *dont_ask);

static gboolean     lcms_image_set_profile   (gint32            image,
                                              GFile            *file);

static gboolean     lcms_icc_apply_dialog    (gint32            image,
                                              GimpColorProfile *src_profile,
                                              GimpColorProfile *dest_profile,
                                              gboolean         *dont_ask);

static GimpPDBStatusType  lcms_dialog        (GimpColorConfig  *config,
                                              gint32            image,
                                              gboolean          apply,
                                              LcmsValues       *values);


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
  { PLUG_IN_PROC_SET,       2 },
  { PLUG_IN_PROC_SET_RGB,   2 },
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

  gimp_plugin_menu_register (PLUG_IN_PROC_SET,
                             "<Image>/Image/Color Management");
  gimp_plugin_menu_register (PLUG_IN_PROC_APPLY,
                             "<Image>/Image/Color Management");
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

  success = lcms_image_set_profile (image, file);

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
      gchar *src_label  = gimp_color_profile_get_label (src_profile);
      gchar *dest_label = gimp_color_profile_get_label (dest_profile);

      g_object_unref (src_profile);
      g_object_unref (dest_profile);

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

static gboolean
lcms_image_set_profile (gint32  image,
                        GFile  *file)
{
  GimpColorProfile *profile = NULL;

  g_return_val_if_fail (image != -1, FALSE);

  if (file)
    {
      GError *error = NULL;

      profile = gimp_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          g_message ("%s", error->message);
          g_clear_error (&error);

          return FALSE;
        }
    }

  gimp_image_undo_group_start (image);

  gimp_image_set_color_profile (image, profile);
  gimp_image_detach_parasite (image, "icc-profile-name");

  gimp_image_undo_group_end (image);

  if (profile)
    g_object_unref (profile);

  return TRUE;
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
lcms_icc_profile_dest_label_new (GimpColorProfile *profile)
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

static GtkWidget *
lcms_icc_combo_box_new (GimpColorConfig *config,
                        const gchar     *filename)
{
  GtkWidget        *combo;
  GtkWidget        *dialog;
  gchar            *history;
  gchar            *label;
  gchar            *name;
  const gchar      *rgb_filename = NULL;
  GimpColorProfile *profile      = NULL;
  GError           *error        = NULL;

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

  g_object_unref (profile);

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
  GimpColorProfile         *src_profile;
  gchar                    *name;
  gboolean                  success = FALSE;
  gboolean                  run;

  src_profile = gimp_image_get_effective_color_profile (image);

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
      gchar            *filename = gimp_color_profile_combo_box_get_active (box);
      GFile            *file     = NULL;
      GimpColorProfile *dest_profile;

      gtk_widget_set_sensitive (dialog, FALSE);

      if (filename)
        {
          file = g_file_new_for_path (filename);
          g_free (filename);
        }

      if (file)
        {
          GError *error = NULL;

          dest_profile = gimp_color_profile_new_from_file (file, &error);

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
                success = gimp_image_convert_color_profile (image,
                                                            dest_profile,
                                                            values->intent,
                                                            values->bpc);
              else
                success = lcms_image_set_profile (image, file);
            }
          else
            {
              g_message (_("Destination profile is not for RGB color space."));
            }

          g_object_unref (dest_profile);
        }

      if (file)
        g_object_unref (file);

      if (success)
        break;
      else
        gtk_widget_set_sensitive (dialog, TRUE);
    }

  gtk_widget_destroy (dialog);

  g_object_unref (src_profile);

  return (run ?
          (success ? GIMP_PDB_SUCCESS : GIMP_PDB_EXECUTION_ERROR) :
          GIMP_PDB_CANCEL);
}
