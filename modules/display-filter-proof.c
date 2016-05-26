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

#include <gegl.h>
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
  GimpColorDisplay    parent_instance;

  gint                intent;
  gboolean            bpc;
  gchar              *profile;

  GimpColorTransform *transform;
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


static GType       cdisplay_proof_get_type        (void);

static void        cdisplay_proof_finalize        (GObject          *object);
static void        cdisplay_proof_get_property    (GObject          *object,
                                                   guint             property_id,
                                                   GValue           *value,
                                                   GParamSpec       *pspec);
static void        cdisplay_proof_set_property    (GObject          *object,
                                                   guint             property_id,
                                                   const GValue     *value,
                                                   GParamSpec       *pspec);


static void        cdisplay_proof_convert_buffer  (GimpColorDisplay *display,
                                                   GeglBuffer       *buffer,
                                                   GeglRectangle    *area);
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

  GIMP_CONFIG_PROP_ENUM (object_class, PROP_INTENT,
                         "intent",
                         _("Intent"),
                         NULL,
                         GIMP_TYPE_COLOR_RENDERING_INTENT,
                         GIMP_COLOR_RENDERING_INTENT_PERCEPTUAL,
                         0);

  GIMP_CONFIG_PROP_BOOLEAN (object_class, PROP_BPC,
                            "black-point-compensation",
                            _("Black point compensation"),
                            NULL,
                            FALSE,
                            0);

  GIMP_CONFIG_PROP_PATH (object_class, PROP_PROFILE,
                         "profile",
                         _("Profile"),
                         NULL,
                         GIMP_CONFIG_PATH_FILE, NULL,
                         0);

  display_class->name            = _("Color Proof");
  display_class->help_id         = "gimp-colordisplay-proof";
  display_class->icon_name       = GIMP_STOCK_DISPLAY_FILTER_PROOF;

  display_class->convert_buffer  = cdisplay_proof_convert_buffer;
  display_class->configure       = cdisplay_proof_configure;
  display_class->changed         = cdisplay_proof_changed;
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
      g_object_unref (proof->transform);
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
cdisplay_proof_convert_buffer (GimpColorDisplay *display,
                               GeglBuffer       *buffer,
                               GeglRectangle    *area)
{
  CdisplayProof *proof = CDISPLAY_PROOF (display);

  if (proof->transform)
    gimp_color_transform_process_buffer (proof->transform,
                                         buffer, area,
                                         buffer, area);
}

static void
cdisplay_proof_profile_changed (GtkWidget     *combo,
                                CdisplayProof *proof)
{
  GFile *file;
  gchar *path = NULL;

  file = gimp_color_profile_combo_box_get_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo));

  if (file)
    {
      path = g_file_get_path (file);
      g_object_unref (file);
    }

  g_object_set (proof,
                "profile", path,
                NULL);

  g_free (path);
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

  dialog = gimp_color_profile_chooser_dialog_new (_("Choose an ICC Color Profile"),
                                                  NULL,
                                                  GTK_FILE_CHOOSER_ACTION_OPEN);

  history = gimp_personal_rc_file ("profilerc");
  combo = gimp_color_profile_combo_box_new (dialog, history);
  g_free (history);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (cdisplay_proof_profile_changed),
                    proof);

  if (proof->profile)
    {
      GFile *file = g_file_new_for_path (proof->profile);

      gimp_color_profile_combo_box_set_active_file (GIMP_COLOR_PROFILE_COMBO_BOX (combo),
                                                    file, NULL);
      g_object_unref (file);
    }

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
  CdisplayProof    *proof = CDISPLAY_PROOF (display);
  GimpColorProfile *rgb_profile;
  GimpColorProfile *proof_profile;
  GFile            *file;

  if (proof->transform)
    {
      g_object_unref (proof->transform);
      proof->transform = NULL;
    }

  if (! proof->profile)
    return;

  rgb_profile = gimp_color_profile_new_rgb_srgb ();

  file = g_file_new_for_path (proof->profile);
  proof_profile = gimp_color_profile_new_from_file (file, NULL);
  g_object_unref (file);

  if (proof_profile)
    {
      GimpColorTransformFlags flags = 0;

      if (proof->bpc)
        flags |= GIMP_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      proof->transform =
        gimp_color_transform_new_proofing (rgb_profile,
                                           babl_format ("R'G'B'A float"),
                                           rgb_profile,
                                           babl_format ("R'G'B'A float"),
                                           proof_profile,
                                           proof->intent,
                                           proof->intent,
                                           flags);

      g_object_unref (proof_profile);
    }

  g_object_unref (rgb_profile);
}
