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
  gchar            *filename;

  cmsHTRANSFORM     transform;

  GtkWidget        *table;
  GtkWidget        *optionmenu;
  GtkWidget        *toggle;
};

struct _CdisplayProofClass
{
  GimpColorDisplayClass parent_instance;
};


static GType              cdisplay_proof_get_type   (GTypeModule     *module);
static void               cdisplay_proof_class_init (CdisplayProofClass *klass);
static void               cdisplay_proof_init       (CdisplayProof   *proof);

static void               cdisplay_proof_finalize   (GObject         *object);

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
  GObjectClass          *object_class;
  GimpColorDisplayClass *display_class;

  object_class  = G_OBJECT_CLASS (klass);
  display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = cdisplay_proof_finalize;

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
  proof->intent    = INTENT_PERCEPTUAL;
  proof->bpc       = FALSE;
  proof->transform = NULL;
  proof->filename  = NULL;
  proof->table     = NULL;

  cdisplay_proof_changed (GIMP_COLOR_DISPLAY (proof));
}

static void
cdisplay_proof_finalize (GObject *object)
{
  CdisplayProof *proof = CDISPLAY_PROOF (object);

  if (proof->table)
    gtk_widget_destroy (proof->table);

  if (proof->filename)
    {
      g_free (proof->filename);
      proof->filename = NULL;
    }
  if (proof->transform)
    {
      cmsDeleteTransform (proof->transform);
      proof->transform = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpColorDisplay *
cdisplay_proof_clone (GimpColorDisplay *display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);
  CdisplayProof *copy;

  copy = CDISPLAY_PROOF (gimp_color_display_new (G_TYPE_FROM_INSTANCE (proof)));
  copy->intent    = proof->intent;
  copy->bpc       = proof->bpc;
  copy->filename  = g_strdup (proof->filename);

  cdisplay_proof_changed (GIMP_COLOR_DISPLAY (copy));

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
          g_free (proof->filename);

          proof->intent   = atoi (tokens[0]);
          proof->bpc      = atoi (tokens[1]) ? TRUE : FALSE;
          proof->filename = tokens[2];
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
                         proof->filename ? proof->filename : "");

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

  g_signal_connect (proof->table, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &proof->table);

  gtk_table_set_col_spacings (GTK_TABLE (proof->table), 4);
  gtk_table_set_row_spacings (GTK_TABLE (proof->table), 2);

  proof->optionmenu =
    gimp_int_option_menu_new (FALSE,
			      G_CALLBACK (proof_intent_callback),
			      proof, proof->intent,

			      _("Perceptual"),
			      INTENT_PERCEPTUAL, NULL,
			      _("Relative Colorimetric"),
			      INTENT_RELATIVE_COLORIMETRIC, NULL,
			      _("Saturation"),
			      INTENT_SATURATION, NULL,
			      _("Absolute Colorimetric"),
			      INTENT_ABSOLUTE_COLORIMETRIC, NULL,

                              NULL);

  gimp_table_attach_aligned (GTK_TABLE (proof->table), 0, 0,
                             _("_Intent:"), 1.0, 0.5,
                             proof->optionmenu, 1, TRUE);

  entry = gimp_file_entry_new (_("Choose an ICC Color Profile"),
                               proof->filename, FALSE, FALSE);
  gimp_table_attach_aligned (GTK_TABLE (proof->table), 0, 1,
                             _("_Profile:"), 1.0, 0.5,
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
cdisplay_proof_configure_reset (GimpColorDisplay * display)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);

  proof->intent = INTENT_PERCEPTUAL;
  proof->bpc    = FALSE;

  if (proof->table)
    {
      gimp_int_option_menu_set_history (GTK_OPTION_MENU (proof->optionmenu),
                                        INTENT_PERCEPTUAL);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (proof->toggle),
                                    proof->bpc);
    }

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (proof));
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

  if (! proof->filename)
    return;

  /*  This should be read from the global parasite pool.
   *  For now, just use the built-in sRGB profile.
   */
  rgbProfile = cmsCreate_sRGBProfile ();

  proofProfile = cmsOpenProfileFromFile (proof->filename, "r");

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
  gimp_menu_item_update (widget, &proof->intent);

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (proof));
}

static void
proof_bpc_callback (GtkWidget     *widget,
                    CdisplayProof *proof)
{
  gimp_toggle_button_update (widget, &proof->bpc);

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (proof));
}

static void
proof_file_callback (GtkWidget     *widget,
                     CdisplayProof *proof)
{
  g_free (proof->filename);
  proof->filename = gimp_file_entry_get_filename (GIMP_FILE_ENTRY (widget));

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (proof));
}
