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


#define CDISPLAY_TYPE_LCMS            (cdisplay_lcms_type)
#define CDISPLAY_LCMS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_LCMS, CdisplayLcms))
#define CDISPLAY_LCMS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_LCMS, CdisplayLcmsClass))
#define CDISPLAY_IS_LCMS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_LCMS))
#define CDISPLAY_IS_LCMS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_LCMS))


typedef struct _CdisplayLcms CdisplayLcms;
typedef struct _CdisplayLcmsClass CdisplayLcmsClass;

struct _CdisplayLcms
{
  GimpColorDisplay  parent_instance;

  GimpColorConfig  *config;
  cmsHTRANSFORM     transform;
};

struct _CdisplayLcmsClass
{
  GimpColorDisplayClass parent_instance;
};


enum
{
  PROP_0,
  PROP_CONFIG
};


static GType     cdisplay_lcms_get_type     (GTypeModule       *module);
static void      cdisplay_lcms_class_init   (CdisplayLcmsClass *klass);
static void      cdisplay_lcms_init         (CdisplayLcms      *lcms);
static void      cdisplay_lcms_dispose      (GObject           *object);
static void      cdisplay_lcms_get_property (GObject           *object,
                                             guint              property_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);
static void      cdisplay_lcms_set_property (GObject           *object,
                                             guint              property_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);

static GtkWidget * cdisplay_lcms_configure  (GimpColorDisplay  *display);
static void        cdisplay_lcms_convert    (GimpColorDisplay  *display,
                                             guchar            *buf,
                                             gint               width,
                                             gint               height,
                                             gint               bpp,
                                             gint               bpl);
static void        cdisplay_lcms_changed    (GimpColorDisplay  *display);
static void        cdisplay_lcms_set_config (CdisplayLcms      *lcms,
                                             GimpColorConfig   *config);

static cmsHPROFILE  cdisplay_lcms_get_display_profile (CdisplayLcms    *lcms,
                                                       GimpColorConfig *config);


static const GimpModuleInfo cdisplay_lcms_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Color management display filter using ICC color profiles"),
  "Sven Neumann",
  "v0.1",
  "(c) 2005, released under the GPL",
  "2005"
};

static GType                  cdisplay_lcms_type = 0;
static GimpColorDisplayClass *parent_class       = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_lcms_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_lcms_get_type (module);

  return TRUE;
}

static GType
cdisplay_lcms_get_type (GTypeModule *module)
{
  if (! cdisplay_lcms_type)
    {
      static const GTypeInfo display_info =
      {
        sizeof (CdisplayLcmsClass),
        (GBaseInitFunc)     NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cdisplay_lcms_class_init,
        NULL,			/* class_finalize */
        NULL,			/* class_data     */
        sizeof (CdisplayLcms),
        0,			/* n_preallocs    */
        (GInstanceInitFunc) cdisplay_lcms_init,
      };

       cdisplay_lcms_type =
	g_type_module_register_type (module, GIMP_TYPE_COLOR_DISPLAY,
				     "CdisplayLcms", &display_info, 0);
    }

  return cdisplay_lcms_type;
}

static void
cdisplay_lcms_class_init (CdisplayLcmsClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->dispose      = cdisplay_lcms_dispose;
  object_class->get_property = cdisplay_lcms_get_property;
  object_class->set_property = cdisplay_lcms_set_property;

  GIMP_CONFIG_INSTALL_PROP_OBJECT (object_class, PROP_CONFIG,
                                   "config", NULL,
                                   GIMP_TYPE_COLOR_CONFIG,
                                   0);

  display_class->name        = _("Color Management");
  display_class->help_id     = "gimp-colordisplay-lcms";
  display_class->configure   = cdisplay_lcms_configure;
  display_class->convert     = cdisplay_lcms_convert;
  display_class->changed     = cdisplay_lcms_changed;

  cmsErrorAction (LCMS_ERROR_IGNORE);
}

static void
cdisplay_lcms_init (CdisplayLcms *lcms)
{
  lcms->config    = NULL;
  lcms->transform = NULL;
}

static void
cdisplay_lcms_dispose (GObject *object)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (object);

  cdisplay_lcms_set_config (lcms, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
cdisplay_lcms_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      g_value_set_object (value, lcms->config);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
cdisplay_lcms_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      cdisplay_lcms_set_config (lcms, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkWidget *
cdisplay_lcms_configure (GimpColorDisplay *display)
{
  return g_object_new (GTK_TYPE_LABEL,
                       "label",      _("This module takes its configuration "
                                       "from the <i>Color Management</i> "
                                       "section in the Preferences dialog."),
                       "use-markup", TRUE,
                       "justify",    GTK_JUSTIFY_LEFT,
                       "wrap",       TRUE,
                       "xalign",     0.5,
                       "yalign",     0.5,
                       NULL);
}

static void
cdisplay_lcms_convert (GimpColorDisplay *display,
                       guchar           *buf,
                       gint              width,
                       gint              height,
                       gint              bpp,
                       gint              bpl)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (display);
  gint          y;

  if (bpp != 3)
    return;

  if (! lcms->transform)
    return;

  for (y = 0; y < height; y++, buf += bpl)
    cmsDoTransform (lcms->transform, buf, buf, width);
}

static void
cdisplay_lcms_changed (GimpColorDisplay *display)
{
  CdisplayLcms    *lcms   = CDISPLAY_LCMS (display);
  GimpColorConfig *config = lcms->config;

  cmsHPROFILE      src_profile   = NULL;
  cmsHPROFILE      dest_profile  = NULL;
  cmsHPROFILE      proof_profile = NULL;

  if (lcms->transform)
    {
      cmsDeleteTransform (lcms->transform);
      lcms->transform = NULL;
    }

  if (! config)
    return;

  switch (config->mode)
    {
    case GIMP_COLOR_MANAGEMENT_OFF:
      return;

    case GIMP_COLOR_MANAGEMENT_SOFTPROOF:
      if (config->printer_profile)
        proof_profile = cmsOpenProfileFromFile (config->printer_profile, "r");

      /*  fallthru  */

    case GIMP_COLOR_MANAGEMENT_DISPLAY:
      /*  this should be taken from the image  */
      src_profile = cmsCreate_sRGBProfile ();

      dest_profile = cdisplay_lcms_get_display_profile (lcms, config);
      break;
    }

  if (proof_profile)
    {
      lcms->transform = cmsCreateProofingTransform (src_profile, TYPE_RGB_8,
                                                    (dest_profile ?
                                                     dest_profile :
                                                     src_profile), TYPE_RGB_8,
                                                    proof_profile,
                                                    config->display_intent,
                                                    config->simulation_intent,
                                                    cmsFLAGS_SOFTPROOFING);
      cmsCloseProfile (proof_profile);
    }
  else if (dest_profile)
    {
      lcms->transform = cmsCreateTransform (src_profile,  TYPE_RGB_8,
                                            dest_profile, TYPE_RGB_8,
                                            config->display_intent,
                                            0);
    }

  if (dest_profile)
    cmsCloseProfile (dest_profile);

  cmsCloseProfile (src_profile);
}

static void
cdisplay_lcms_set_config (CdisplayLcms    *lcms,
                          GimpColorConfig *config)
{
  if (config == lcms->config)
    return;

  if (lcms->config)
    {
      g_signal_handlers_disconnect_by_func (lcms->config,
                                            G_CALLBACK (gimp_color_display_changed),
                                            lcms);
      g_object_unref (lcms->config);
    }

  lcms->config = config;

  if (lcms->config)
    {
      g_object_ref (lcms->config);
      g_signal_connect_swapped (lcms->config, "notify",
                                G_CALLBACK (gimp_color_display_changed),
                                lcms);
    }

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (lcms));
}


static cmsHPROFILE
cdisplay_lcms_get_display_profile (CdisplayLcms    *lcms,
                                   GimpColorConfig *config)
{
#if defined (GDK_WINDOWING_X11)
  if (config->display_profile_from_gdk)
    {
      /*  FIXME: need to access the display's screen here  */
      GdkScreen *screen = gdk_screen_get_default ();
      GdkAtom    type   = GDK_NONE;
      gint       format = 0;
      gint       nitems = 0;
      guchar    *data   = NULL;

      g_return_val_if_fail (GDK_IS_SCREEN (screen), NULL);

      if (gdk_property_get (gdk_screen_get_root_window (screen),
                        gdk_atom_intern ("_ICC_PROFILE", FALSE),
                        GDK_NONE,
                        0, 64 * 1024 * 1024, FALSE,
                        &type, &format, &nitems, &data) && nitems > 0)
        {
          cmsHPROFILE  profile = cmsOpenProfileFromMem (data, nitems);

          g_free (data);

          if (profile)
            {
              const gchar *name = cmsTakeProductName (profile);

              g_printerr ("obtained ICC profile from X server: %s\n",
                          name ? name : "<untitled>");
            }

          return profile;
        }
    }
#endif

  if (config->display_profile)
    return cmsOpenProfileFromFile (config->display_profile, "r");

  return NULL;
}
