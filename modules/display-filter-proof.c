/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#ifdef HAVE_LCMS1
#include <lcms.h>
typedef DWORD cmsUInt32Number;
#else
#include <lcms2.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"

#define CDISPLAY_TYPE_PROOF            (cdisplay_proof_get_type ())
#define CDISPLAY_PROOF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_PROOF, CdisplayProof))
#define CDISPLAY_PROOF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_PROOF, CdisplayProofClass))
#define CDISPLAY_IS_PROOF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_PROOF))
#define CDISPLAY_IS_PROOF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_PROOF))


typedef struct _CdisplayProof      CdisplayProof;
typedef struct _CdisplayProofClass CdisplayProofClass;

struct _CdisplayProof
{
  GimpColorDisplay  parent_instance;

  gint              intent;
  gboolean          bpc;
  gchar            *profile;

  cmsHTRANSFORM     transform;
};

struct _CdisplayProofClass
{
  GimpColorDisplayClass parent_instance;
};


enum
{
  PROP_0,
  PROP_INTENT,
  PROP_BPC,
  PROP_PROFILE
};


GType              cdisplay_proof_get_type        (void);

static void        cdisplay_proof_finalize        (GObject          *object);
static void        cdisplay_proof_get_property    (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void        cdisplay_proof_set_property    (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);


static void        cdisplay_proof_convert_surface (GimpColorDisplay *display,
                                                   cairo_surface_t  *surface);
static GtkWidget * cdisplay_proof_configure       (GimpColorDisplay *display);
static void        cdisplay_proof_changed         (GimpColorDisplay *display);


static const GimpModuleInfo cdisplay_proof_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Color proof filter using ICC color profile"),
  "Banlu Kemiyatorn <id@project-ile.net>",
  "v0.1",
  "(c) 2002-2003, released under the GPL",
  "November 14, 2003"
};

G_DEFINE_DYNAMIC_TYPE (CdisplayProof, cdisplay_proof,
                       GIMP_TYPE_COLOR_DISPLAY)

G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_proof_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_proof_register_type (module);

  return TRUE;
}

static void
cdisplay_proof_class_init (CdisplayProofClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->finalize         = cdisplay_proof_finalize;
  object_class->get_property     = cdisplay_proof_get_property;
  object_class->set_property     = cdisplay_proof_set_property;

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_INTENT,
                                 "intent", NULL,
                                 GIMP_TYPE_COLOR_RENDERING_INTENT,
                                 GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                 0);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_BPC,
                                    "black-point-compensation", NULL,
                                    FALSE,
                                    0);
  GIMP_CONFIG_INSTALL_PROP_PATH (object_class, PROP_PROFILE,
                                 "profile", NULL,
                                 GIMP_CONFIG_PATH_FILE, NULL,
                                 0);

  display_class->name            = _("Color Proof");
  display_class->help_id         = "gimp-colordisplay-proof";
  display_class->stock_id        = GIMP_STOCK_DISPLAY_FILTER_PROOF;

  display_class->convert_surface = cdisplay_proof_convert_surface;
  display_class->configure       = cdisplay_proof_configure;
  display_class->changed         = cdisplay_proof_changed;

#ifdef HAVE_LCMS1
  cmsErrorAction (LCMS_ERROR_IGNORE);
#endif
}

static void
cdisplay_proof_class_finalize (CdisplayProofClass *klass)
{
}

static void
cdisplay_proof_init (CdisplayProof *proof)
{
  proof->transform = NULL;
  proof->profile   = NULL;
}

static void
cdisplay_proof_finalize (GObject *object)
{
  CdisplayProof *proof = CDISPLAY_PROOF (object);

  if (proof->profile)
    {
      g_free (proof->profile);
      proof->profile = NULL;
    }

  if (proof->transform)
    {
      cmsDeleteTransform (proof->transform);
      proof->transform = NULL;
    }

  G_OBJECT_CLASS (cdisplay_proof_parent_class)->finalize (object);
}

static void
cdisplay_proof_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  CdisplayProof *proof = CDISPLAY_PROOF (object);

  switch (property_id)
    {
    case PROP_INTENT:
      g_value_set_enum (value, proof->intent);
      break;
    case PROP_BPC:
      g_value_set_boolean (value, proof->bpc);
      break;
    case PROP_PROFILE:
      g_value_set_string (value, proof->profile);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_proof_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  CdisplayProof *proof = CDISPLAY_PROOF (object);

  switch (property_id)
    {
    case PROP_INTENT:
      proof->intent = g_value_get_enum (value);
      break;
    case PROP_BPC:
      proof->bpc = g_value_get_boolean (value);
      break;
    case PROP_PROFILE:
      g_free (proof->profile);
      proof->profile = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (proof));
}

static void
cdisplay_proof_convert_surface (GimpColorDisplay *display,
                                cairo_surface_t  *surface)
{
  CdisplayProof  *proof  = CDISPLAY_PROOF (display);
  gint            width  = cairo_image_surface_get_width (surface);
  gint            height = cairo_image_surface_get_height (surface);
  gint            stride = cairo_image_surface_get_stride (surface);
  guchar         *buf    = cairo_image_surface_get_data (surface);
  cairo_format_t  fmt    = cairo_image_surface_get_format (surface);
  guchar         *rowbuf;
  gint            x, y;
  guchar          r, g, b, a;

  if (fmt != CAIRO_FORMAT_ARGB32)
    return;

  if (! proof->transform)
    return;

  rowbuf = g_malloc (stride);

  for (y = 0; y < height; y++, buf += stride)
    {
      /* Switch buf from ARGB premul to ARGB non-premul, since lcms ignores the
       * alpha channel.  The macro takes care of byte order.
       */
      for (x = 0; x < width; x++)
        {
          GIMP_CAIRO_ARGB32_GET_PIXEL (buf + 4*x, r, g, b, a);
          rowbuf[4*x+0] = a;
          rowbuf[4*x+1] = r;
          rowbuf[4*x+2] = g;
          rowbuf[4*x+3] = b;
        }

      cmsDoTransform (proof->transform, rowbuf, rowbuf, width);

      /* And back to ARGB premul */
      for (x = 0; x < width; x++)
        {
          a = rowbuf[4*x+0];
          r = rowbuf[4*x+1];
          g = rowbuf[4*x+2];
          b = rowbuf[4*x+3];
          GIMP_CAIRO_ARGB32_SET_PIXEL (buf + 4*x, r, g, b, a);
        }
    }

  g_free (rowbuf);
}

static void
cdisplay_proof_combo_box_set_active (GimpColorProfileComboBox *combo,
                                     const gchar              *filename)
{
  cmsHPROFILE  profile = NULL;
  gchar       *label   = NULL;

  if (filename)
    profile = cmsOpenProfileFromFile (filename, "r");

  if (profile)
    {
#ifdef HAVE_LCMS1
      label = gimp_any_to_utf8 (cmsTakeProductDesc (profile), -1, NULL);
#else
      cmsUInt32Number  descSize;
      gchar           *descData;

      descSize = cmsGetProfileInfoASCII(profile, cmsInfoDescription,
                                        "en", "US", NULL, 0);
      if (descSize > 0)
        {
          descData = g_new (gchar, descSize + 1);
          descSize = cmsGetProfileInfoASCII (profile, cmsInfoDescription,
                                             "en", "US", descData, descSize);
          if (descSize > 0)
            label = gimp_any_to_utf8 (descData, -1, NULL);

          g_free (descData);
        }
#endif

      if (! label)
#ifdef HAVE_LCMS1
        label = gimp_any_to_utf8 (cmsTakeProductName (profile), -1, NULL);
#else
        {
          descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                             "en", "US", NULL, 0);
          if (descSize > 0)
            {
              descData = g_new (gchar, descSize + 1);
              descSize = cmsGetProfileInfoASCII (profile, cmsInfoModel,
                                                 "en", "US", descData, descSize);
              if (descSize > 0)
                label = gimp_any_to_utf8 (descData, -1, NULL);

              g_free (descData);
            }
        }
#endif

      cmsCloseProfile (profile);
    }

  gimp_color_profile_combo_box_set_active (combo, filename, label);
  g_free (label);
}

static void
cdisplay_proof_file_chooser_dialog_response (GtkFileChooser           *dialog,
                                             gint                      response,
                                             GimpColorProfileComboBox *combo)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      gchar *filename = gtk_file_chooser_get_filename (dialog);

      if (filename)
        {
          cdisplay_proof_combo_box_set_active (combo, filename);

          g_free (filename);
        }
    }

  gtk_widget_hide (GTK_WIDGET (dialog));
}

static GtkWidget *
cdisplay_proof_file_chooser_dialog_new (void)
{
  GtkWidget     *dialog;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Choose an ICC Color Profile"),
                                        NULL,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,

                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,

                                        NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

#ifndef G_OS_WIN32
  {
    const gchar folder[] = "/usr/share/color/icc";

    if (g_file_test (folder, G_FILE_TEST_IS_DIR))
      gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                            folder, NULL);
  }
#endif

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("ICC color profile (*.icc, *.icm)"));
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Cc]");
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Mm]");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  return dialog;
}

static void
cdisplay_proof_profile_changed (GtkWidget     *combo,
                                CdisplayProof *proof)
{
  gchar *profile;

  profile = gimp_color_profile_combo_box_get_active (GIMP_COLOR_PROFILE_COMBO_BOX (combo));

  g_object_set (proof,
                "profile", profile,
                NULL);

  g_free (profile);
}

static GtkWidget *
cdisplay_proof_configure (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  GtkWidget     *table;
  GtkWidget     *combo;
  GtkWidget     *toggle;
  GtkWidget     *dialog;
  gchar         *history;

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);

  dialog = cdisplay_proof_file_chooser_dialog_new ();

  history = gimp_personal_rc_file ("profilerc");
  combo = gimp_color_profile_combo_box_new (dialog, history);
  g_free (history);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (cdisplay_proof_file_chooser_dialog_response),
                    combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (cdisplay_proof_profile_changed),
                    proof);

  if (proof->profile)
    cdisplay_proof_combo_box_set_active (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                         proof->profile);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Profile:"), 0.0, 0.5,
                             combo, 1, FALSE);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (proof), "intent", 0, 0);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Intent:"), 0.0, 0.5,
                             combo, 1, FALSE);

  toggle = gimp_prop_check_button_new (G_OBJECT (proof),
                                       "black-point-compensation",
                                       _("_Black Point Compensation"));
  gtk_table_attach_defaults (GTK_TABLE (table), toggle, 1, 2, 2, 3);
  gtk_widget_show (toggle);

  return table;
}

static void
cdisplay_proof_changed (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  cmsHPROFILE    rgbProfile;
  cmsHPROFILE    proofProfile;

  if (proof->transform)
    {
      cmsDeleteTransform (proof->transform);
      proof->transform = NULL;
    }

  if (! proof->profile)
    return;

  rgbProfile = cmsCreate_sRGBProfile ();

  proofProfile = cmsOpenProfileFromFile (proof->profile, "r");

  if (proofProfile)
    {
      cmsUInt32Number flags = cmsFLAGS_SOFTPROOFING;

      if (proof->bpc)
        flags |= cmsFLAGS_BLACKPOINTCOMPENSATION;

      proof->transform = cmsCreateProofingTransform (rgbProfile, TYPE_ARGB_8,
                                                     rgbProfile, TYPE_ARGB_8,
                                                     proofProfile,
                                                     proof->intent,
                                                     proof->intent,
                                                     flags);

      cmsCloseProfile (proofProfile);
    }

  cmsCloseProfile (rgbProfile);
}
