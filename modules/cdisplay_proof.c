/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#ifdef HAVE_LCMS_LCMS_H
#include <lcms/lcms.h>
#else
#include <lcms.h>
#endif

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpmath/gimpmath.h"

#include "libgimp/libgimp-intl.h"


#define CDISPLAY_TYPE_PROOF_INTENT (cdisplay_proof_intent_type)
static GType  cdisplay_proof_intent_get_type (GTypeModule *module);

static const GEnumValue cdisplay_proof_intent_enum_values[] =
{
  { INTENT_PERCEPTUAL,
    N_("Perceptual"),            "perceptual"            },
  { INTENT_RELATIVE_COLORIMETRIC,
    N_("Relative Colorimetric"), "relative-colorimetric" },
  { INTENT_SATURATION,
    N_("Saturation"),            "saturation"            },
  { INTENT_ABSOLUTE_COLORIMETRIC,
    N_("Absolute Colorimetric"), "absolute-colorimetric" },
  { 0, NULL, NULL }
};


#define CDISPLAY_TYPE_PROOF            (cdisplay_proof_type)
#define CDISPLAY_PROOF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_PROOF, CdisplayProof))
#define CDISPLAY_PROOF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_PROOF, CdisplayProofClass))
#define CDISPLAY_IS_PROOF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_PROOF))
#define CDISPLAY_IS_PROOF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_PROOF))


typedef struct _CdisplayProof CdisplayProof;
typedef struct _CdisplayProofClass CdisplayProofClass;

struct _CdisplayProof
{
  GimpColorDisplay  parent_instance;

  gint              intent;
  gboolean          bpc;
  gchar            *profile;

  cmsHTRANSFORM     transform;

  GtkWidget        *table;
  GtkWidget        *combo;
  GtkWidget        *toggle;
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


static GType              cdisplay_proof_get_type   (GTypeModule     *module);
static void               cdisplay_proof_class_init (CdisplayProofClass *klass);
static void               cdisplay_proof_init       (CdisplayProof   *proof);

static void               cdisplay_proof_dispose      (GObject       *object);
static void               cdisplay_proof_finalize     (GObject       *object);
static void               cdisplay_proof_get_property (GObject       *object,
                                                       guint          property_id,
                                                       GValue        *value,
                                                       GParamSpec    *pspec);
static void               cdisplay_proof_set_property (GObject       *object,
                                                       guint          property_id,
                                                       const GValue  *value,
                                                       GParamSpec    *pspec);


static GimpColorDisplay * cdisplay_proof_clone      (GimpColorDisplay *display);
static void               cdisplay_proof_convert    (GimpColorDisplay *display,
                                                     guchar           *buf,
                                                     gint              width,
                                                     gint              height,
                                                     gint              bpp,
                                                     gint              bpl);
static void               cdisplay_proof_load_state (GimpColorDisplay *display,
                                                     GimpParasite     *state);
static GimpParasite     * cdisplay_proof_save_state (GimpColorDisplay *display);
static GtkWidget        * cdisplay_proof_configure  (GimpColorDisplay *display);
static void               cdisplay_proof_configure_reset (GimpColorDisplay *display);

static void               cdisplay_proof_changed    (GimpColorDisplay *display);

static void               proof_intent_callback     (GtkWidget        *widget,
                                                     CdisplayProof    *proof);
static void               proof_bpc_callback        (GtkWidget        *widget,
                                                     CdisplayProof    *proof);
static void               proof_file_callback       (GtkWidget        *widget,
                                                     CdisplayProof    *proof);


static const GimpModuleInfo cdisplay_proof_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Color proof filter using ICC color profile"),
  "Banlu Kemiyatorn <id@project-ile.net>",
  "v0.1",
  "(c) 2002-2003, released under the GPL",
  "November 14, 2003"
};

static GType                  cdisplay_proof_type        = 0;
static GType                  cdisplay_proof_intent_type = 0;
static GimpColorDisplayClass *parent_class               = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_proof_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_proof_get_type (module);
  cdisplay_proof_intent_get_type (module);

  return TRUE;
}

static GType
cdisplay_proof_get_type (GTypeModule *module)
{
  if (! cdisplay_proof_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (CdisplayProofClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cdisplay_proof_class_init,
        NULL,			/* class_finalize */
        NULL,			/* class_data     */
        sizeof (CdisplayProof),
        0,			/* n_preallocs    */
        (GInstanceInitFunc) cdisplay_proof_init,
      };

      cdisplay_proof_type =
	g_type_module_register_type (module, GIMP_TYPE_COLOR_DISPLAY,
				     "CdisplayProof", &display_info, 0);
    }

  return cdisplay_proof_type;
}

static GType
cdisplay_proof_intent_get_type (GTypeModule *module)
{
  if (! cdisplay_proof_intent_type)
    cdisplay_proof_intent_type =
      gimp_module_register_enum (module,
                                 "CDisplayProofIntent",
                                 cdisplay_proof_intent_enum_values);

  return cdisplay_proof_intent_type;
}

static void
cdisplay_proof_class_init (CdisplayProofClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose          = cdisplay_proof_dispose;
  object_class->finalize         = cdisplay_proof_finalize;
  object_class->get_property     = cdisplay_proof_get_property;
  object_class->set_property     = cdisplay_proof_set_property;

  g_object_class_install_property (object_class, PROP_INTENT,
                                   g_param_spec_enum ("intent", NULL, NULL,
                                                      CDISPLAY_TYPE_PROOF_INTENT,
                                                      INTENT_PERCEPTUAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      GIMP_MODULE_PARAM_SERIALIZE));

  g_object_class_install_property (object_class, PROP_BPC,
                                   g_param_spec_boolean ("black-point-compensation", NULL, NULL,
                                                         FALSE,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT |
                                                         GIMP_MODULE_PARAM_SERIALIZE));

  g_object_class_install_property (object_class, PROP_PROFILE,
                                   g_param_spec_string ("profile", NULL, NULL,
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        GIMP_MODULE_PARAM_SERIALIZE));

  display_class->name            = _("Color Proof");
  display_class->help_id         = "gimp-colordisplay-proof";
  display_class->clone           = cdisplay_proof_clone;
  display_class->convert         = cdisplay_proof_convert;
  display_class->load_state      = cdisplay_proof_load_state;
  display_class->save_state      = cdisplay_proof_save_state;
  display_class->configure       = cdisplay_proof_configure;
  display_class->configure_reset = cdisplay_proof_configure_reset;
  display_class->changed         = cdisplay_proof_changed;

  cmsErrorAction (LCMS_ERROR_IGNORE);
}

static void
cdisplay_proof_init (CdisplayProof *proof)
{
  proof->transform = NULL;
  proof->profile   = NULL;
  proof->table     = NULL;
}

static void
cdisplay_proof_dispose (GObject *object)
{
  CdisplayProof *proof = CDISPLAY_PROOF (object);

  if (proof->table)
    gtk_widget_destroy (proof->table);

  G_OBJECT_CLASS (parent_class)->dispose (object);
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

  G_OBJECT_CLASS (parent_class)->finalize (object);
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

static GimpColorDisplay *
cdisplay_proof_clone (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  CdisplayProof *copy;

  copy = CDISPLAY_PROOF (gimp_color_display_new (G_TYPE_FROM_INSTANCE (proof)));
  copy->intent  = proof->intent;
  copy->bpc     = proof->bpc;
  copy->profile = g_strdup (proof->profile);

  return GIMP_COLOR_DISPLAY (copy);
}

static void
cdisplay_proof_convert (GimpColorDisplay *display,
			guchar           *buf,
			gint              width,
                        gint              height,
                        gint              bpp,
                        gint              bpl)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  gint           y;

  if (bpp != 3)
    return;

  if (! proof->transform)
    return;

  for (y = 0; y < height; y++, buf += bpl)
    cmsDoTransform (proof->transform, buf, buf, width);
}

static void
cdisplay_proof_load_state (GimpColorDisplay *display,
                           GimpParasite     *state)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  const gchar   *str;

  str = gimp_parasite_data (state);

  if (str[gimp_parasite_data_size (state) - 1] == '\0')
    {
      gchar **tokens = g_strsplit (str, ",", 3);

      if (tokens[0] && tokens[1] && tokens[2])
        {
          g_object_set (proof,
                        "intent",                   atoi (tokens[0]),
                        "black-point-compensation", atoi (tokens[1]),
                        "profile",                  tokens[2],
                        NULL);
        }

      g_strfreev (tokens);
    }
}

static GimpParasite *
cdisplay_proof_save_state (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  GimpParasite  *state;
  gchar         *str;

  str = g_strdup_printf ("%d,%d,%s",
                         proof->intent,
                         proof->bpc,
                         proof->profile ? proof->profile : "");

  state = gimp_parasite_new ("Display/Proof", GIMP_PARASITE_PERSISTENT,
                             strlen (str) + 1, str);

  g_free (str);

  return state;
}

static GtkWidget *
cdisplay_proof_configure (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  GtkWidget     *entry;

  if (proof->table)
    gtk_widget_destroy (proof->table);

  proof->table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (proof->table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (proof->table), 6);

  g_signal_connect (proof->table, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &proof->table);

  proof->combo = gimp_int_combo_box_new (_("Perceptual"),
                                         INTENT_PERCEPTUAL,
                                         _("Relative Colorimetric"),
                                         INTENT_RELATIVE_COLORIMETRIC,
                                         _("Saturation"),
                                         INTENT_SATURATION,
                                         _("Absolute Colorimetric"),
                                         INTENT_ABSOLUTE_COLORIMETRIC,
                                         NULL);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (proof->combo),
                                 proof->intent);

  g_signal_connect (proof->combo, "changed",
                    G_CALLBACK (proof_intent_callback),
                    proof);

  gimp_table_attach_aligned (GTK_TABLE (proof->table), 0, 0,
                             _("_Intent:"), 0.0, 0.5,
                             proof->combo, 1, FALSE);

  entry = gimp_file_entry_new (_("Choose an ICC Color Profile"),
                               proof->profile, FALSE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (proof->table), 0, 1,
                             _("_Profile:"), 0.0, 0.5,
                             entry, 1, FALSE);

  g_signal_connect (entry, "filename-changed",
                    G_CALLBACK (proof_file_callback),
                    proof);

  proof->toggle =
    gtk_check_button_new_with_mnemonic (_("_Black Point Compensation"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proof->toggle), proof->bpc);
  gtk_table_attach_defaults (GTK_TABLE (proof->table),
                             proof->toggle, 1, 2, 2, 3);
  gtk_widget_show (proof->toggle);

  g_signal_connect (proof->toggle, "clicked",
		    G_CALLBACK (proof_bpc_callback),
                    proof);

  return proof->table;
}

static void
cdisplay_proof_configure_reset (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);

  g_object_set (proof,
                "intent",                   INTENT_PERCEPTUAL,
                "black-point-compensation", FALSE,
                NULL);

  if (proof->table)
    {
      gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (proof->combo),
                                     proof->intent);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proof->toggle),
                                    proof->bpc);
    }
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

  /*  This should be read from the global parasite pool.
   *  For now, just use the built-in sRGB profile.
   */
  rgbProfile = cmsCreate_sRGBProfile ();

  proofProfile = cmsOpenProfileFromFile (proof->profile, "r");

  if (proofProfile)
    {
      proof->transform = cmsCreateProofingTransform (rgbProfile,
                                                     TYPE_RGB_8,
                                                     rgbProfile, TYPE_RGB_8,
                                                     proofProfile,
                                                     proof->intent,
                                                     proof->intent,
                                                     cmsFLAGS_SOFTPROOFING |
                                                     (proof->bpc ?
                                                      cmsFLAGS_WHITEBLACKCOMPENSATION : 0));

      cmsCloseProfile (proofProfile);
    }

  cmsCloseProfile (rgbProfile);
}

static void
proof_intent_callback (GtkWidget     *widget,
                       CdisplayProof *proof)
{
  gint  value;

  gimp_int_combo_box_get_active (GIMP_INT_COMBO_BOX (widget), &value);

  g_object_set (proof,
                "intent", value,
                NULL);
}

static void
proof_bpc_callback (GtkWidget     *widget,
                    CdisplayProof *proof)
{
  gboolean  value;

  gimp_toggle_button_update (widget, &value);

  g_object_set (proof,
                "black-point-compensation", value,
                NULL);
}

static void
proof_file_callback (GtkWidget     *widget,
                     CdisplayProof *proof)
{
  gchar *filename = gimp_file_entry_get_filename (GIMP_FILE_ENTRY (widget));

  g_object_set (proof,
                "profile", filename,
                NULL);

  g_free (filename);
}
