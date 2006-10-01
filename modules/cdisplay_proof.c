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

#include <glib.h>  /* lcms.h uses the "inline" keyword */

#ifdef HAVE_LCMS_LCMS_H
#include <lcms/lcms.h>
#else
#include <lcms.h>
#endif

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"

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


static GType       cdisplay_proof_get_type     (GTypeModule        *module);
static void        cdisplay_proof_class_init   (CdisplayProofClass *klass);
static void        cdisplay_proof_init         (CdisplayProof      *proof);

static void        cdisplay_proof_finalize     (GObject          *object);
static void        cdisplay_proof_get_property (GObject          *object,
                                                guint             property_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);
static void        cdisplay_proof_set_property (GObject          *object,
                                                guint             property_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);


static void        cdisplay_proof_convert      (GimpColorDisplay *display,
                                                guchar           *buf,
                                                gint              width,
                                                gint              height,
                                                gint              bpp,
                                                gint              bpl);
static GtkWidget * cdisplay_proof_configure    (GimpColorDisplay *display);
static void        cdisplay_proof_changed      (GimpColorDisplay *display);


static const GimpModuleInfo cdisplay_proof_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Color proof filter using ICC color profile"),
  "Banlu Kemiyatorn <id@project-ile.net>",
  "v0.1",
  "(c) 2002-2003, released under the GPL",
  "November 14, 2003"
};

static GType                  cdisplay_proof_type = 0;
static GimpColorDisplayClass *parent_class        = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_proof_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_proof_get_type (module);

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

static void
cdisplay_proof_class_init (CdisplayProofClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize     = cdisplay_proof_finalize;
  object_class->get_property = cdisplay_proof_get_property;
  object_class->set_property = cdisplay_proof_set_property;

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

  display_class->name        = _("Color Proof");
  display_class->help_id     = "gimp-colordisplay-proof";
  display_class->stock_id    = GIMP_STOCK_DISPLAY_FILTER_PROOF;

  display_class->convert     = cdisplay_proof_convert;
  display_class->configure   = cdisplay_proof_configure;
  display_class->changed     = cdisplay_proof_changed;

  cmsErrorAction (LCMS_ERROR_IGNORE);
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

static GtkWidget *
cdisplay_proof_configure (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  GtkWidget     *table;
  GtkWidget     *combo;
  GtkWidget     *entry;
  GtkWidget     *toggle;

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);

  combo = gimp_prop_enum_combo_box_new (G_OBJECT (proof), "intent", 0, 0);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("_Intent:"), 0.0, 0.5,
                             combo, 1, FALSE);

  entry = gimp_prop_file_chooser_button_new (G_OBJECT (proof), "profile",
                                             _("Choose an ICC Color Profile"),
                                             GTK_FILE_CHOOSER_ACTION_OPEN);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 1,
                             _("_Profile:"), 0.0, 0.5,
                             entry, 1, FALSE);

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
