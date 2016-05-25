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

#include <glib.h>

#include <string.h>

#ifdef G_OS_WIN32
#define STRICT
#include <windows.h>
#define LCMS_WIN_TYPES_ALREADY_DEFINED
#endif

#include <lcms2.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_QUARTZ
#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#endif

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


#define CDISPLAY_TYPE_LCMS            (cdisplay_lcms_get_type ())
#define CDISPLAY_LCMS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_LCMS, CdisplayLcms))
#define CDISPLAY_LCMS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_LCMS, CdisplayLcmsClass))
#define CDISPLAY_IS_LCMS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_LCMS))
#define CDISPLAY_IS_LCMS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_LCMS))


typedef struct _CdisplayLcms      CdisplayLcms;
typedef struct _CdisplayLcmsClass CdisplayLcmsClass;

struct _CdisplayLcms
{
  GimpColorDisplay    parent_instance;

  GimpColorTransform *transform;
};

struct _CdisplayLcmsClass
{
  GimpColorDisplayClass parent_instance;
};


GType                     cdisplay_lcms_get_type             (void);

static void               cdisplay_lcms_finalize             (GObject          *object);

static GtkWidget        * cdisplay_lcms_configure            (GimpColorDisplay *display);
static void               cdisplay_lcms_convert_buffer       (GimpColorDisplay *display,
                                                              GeglBuffer       *buffer,
                                                              GeglRectangle    *area);
static void               cdisplay_lcms_changed              (GimpColorDisplay *display);

static GimpColorProfile * cdisplay_lcms_get_display_profile  (CdisplayLcms     *lcms);

static void               cdisplay_lcms_attach_labelled      (GtkTable         *table,
                                                              gint              row,
                                                              const gchar      *text,
                                                              GtkWidget        *widget);
static void               cdisplay_lcms_update_profile_label (CdisplayLcms     *lcms,
                                                              const gchar      *name);
static void               cdisplay_lcms_notify_profile       (GObject          *config,
                                                              GParamSpec       *pspec,
                                                              CdisplayLcms     *lcms);


static const GimpModuleInfo cdisplay_lcms_info =
{
  GIMP_MODULE_ABI_VERSION,
  N_("Color management display filter using ICC color profiles"),
  "Sven Neumann",
  "v0.3",
  "(c) 2005 - 2007, released under the GPL",
  "2005 - 2007"
};

G_DEFINE_DYNAMIC_TYPE (CdisplayLcms, cdisplay_lcms,
                       GIMP_TYPE_COLOR_DISPLAY)

G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_lcms_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_lcms_register_type (module);

  return TRUE;
}

static void
cdisplay_lcms_class_init (CdisplayLcmsClass *klass)
{
  GObjectClass          *object_class  = G_OBJECT_CLASS (klass);
  GimpColorDisplayClass *display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  object_class->finalize         = cdisplay_lcms_finalize;

  display_class->name            = _("Color Management");
  display_class->help_id         = "gimp-colordisplay-lcms";
  display_class->icon_name       = GIMP_STOCK_DISPLAY_FILTER_LCMS;

  display_class->configure       = cdisplay_lcms_configure;
  display_class->convert_buffer  = cdisplay_lcms_convert_buffer;
  display_class->changed         = cdisplay_lcms_changed;
}

static void
cdisplay_lcms_class_finalize (CdisplayLcmsClass *klass)
{
}

static void
cdisplay_lcms_init (CdisplayLcms *lcms)
{
}

static void
cdisplay_lcms_finalize (GObject *object)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (object);

  if (lcms->transform)
    {
      g_object_unref (lcms->transform);
      lcms->transform = NULL;
    }

  G_OBJECT_CLASS (cdisplay_lcms_parent_class)->finalize (object);
}

static GtkWidget *
cdisplay_lcms_configure (GimpColorDisplay *display)
{
  CdisplayLcms *lcms   = CDISPLAY_LCMS (display);
  GObject      *config = G_OBJECT (gimp_color_display_get_config (display));
  GtkWidget    *vbox;
  GtkWidget    *hint;
  GtkWidget    *table;
  GtkWidget    *label;
  gint          row = 0;

  if (! config)
    return NULL;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);

  hint = gimp_hint_box_new (_("This filter takes its configuration "
                              "from the Color Management section "
                              "in the Preferences dialog."));
  gtk_box_pack_start (GTK_BOX (vbox), hint, FALSE, FALSE, 0);
  gtk_widget_show (hint);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  cdisplay_lcms_attach_labelled (GTK_TABLE (table), row++,
                                 _("Mode of operation:"),
                                 gimp_prop_enum_label_new (config, "mode"));

  label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  g_object_set_data (G_OBJECT (lcms), "rgb-profile", label);
  cdisplay_lcms_attach_labelled (GTK_TABLE (table), row++,
                                 _("Image profile:"),
                                 label);
  cdisplay_lcms_update_profile_label (lcms, "rgb-profile");

  label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  g_object_set_data (G_OBJECT (lcms), "display-profile", label);
  cdisplay_lcms_attach_labelled (GTK_TABLE (table), row++,
                                 _("Monitor profile:"),
                                 label);
  cdisplay_lcms_update_profile_label (lcms, "display-profile");

  label = gtk_label_new (NULL);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  g_object_set_data (G_OBJECT (lcms), "printer-profile", label);
  cdisplay_lcms_attach_labelled (GTK_TABLE (table), row++,
                                 _("Print simulation profile:"),
                                 label);
  cdisplay_lcms_update_profile_label (lcms, "printer-profile");

  g_signal_connect_object (config, "notify",
                           G_CALLBACK (cdisplay_lcms_notify_profile),
                           lcms, 0);

  return vbox;
}

static void
cdisplay_lcms_convert_buffer (GimpColorDisplay *display,
                              GeglBuffer       *buffer,
                              GeglRectangle    *area)
{
  CdisplayLcms *lcms = CDISPLAY_LCMS (display);

  if (lcms->transform)
    gimp_color_transform_process_buffer (lcms->transform,
                                         buffer, area,
                                         buffer, area);
}

static void
cdisplay_lcms_changed (GimpColorDisplay *display)
{
  CdisplayLcms     *lcms   = CDISPLAY_LCMS (display);
  GtkWidget        *widget = NULL;
  GimpColorConfig  *config;
  GimpColorManaged *managed;
  GimpColorProfile *src_profile;

  if (lcms->transform)
    {
      g_object_unref (lcms->transform);
      lcms->transform = NULL;
    }

  config  = gimp_color_display_get_config (display);
  managed = gimp_color_display_get_managed (display);

  if (! config || ! managed)
    return;

  if (GTK_IS_WIDGET (managed))
    widget = gtk_widget_get_toplevel (GTK_WIDGET (managed));

  src_profile = gimp_color_managed_get_color_profile (managed);

  lcms->transform =
    gimp_widget_get_color_transform (widget,
                                     config, src_profile,
                                     babl_format ("R'G'B'A float"),
                                     babl_format ("R'G'B'A float"));
}

static GimpColorProfile *
cdisplay_lcms_get_display_profile (CdisplayLcms *lcms)
{
  GimpColorConfig  *config;
  GimpColorManaged *managed;
  GtkWidget        *widget  = NULL;
  GimpColorProfile *profile = NULL;

  config  = gimp_color_display_get_config (GIMP_COLOR_DISPLAY (lcms));
  managed = gimp_color_display_get_managed (GIMP_COLOR_DISPLAY (lcms));

  if (GTK_IS_WIDGET (managed))
    widget = gtk_widget_get_toplevel (GTK_WIDGET (managed));

  if (config->display_profile_from_gdk)
    profile = gimp_widget_get_color_profile (widget);

  if (! profile)
    profile = gimp_color_config_get_display_color_profile (config, NULL);

  return profile;
}

static void
cdisplay_lcms_attach_labelled (GtkTable    *table,
                               gint         row,
                               const gchar *text,
                               GtkWidget   *widget)
{
  GtkWidget *label;

  label = g_object_new (GTK_TYPE_LABEL,
                        "label",  text,
                        "xalign", 1.0,
                        "yalign", 0.5,
                        NULL);

  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_table_attach (table, label, 0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  if (GTK_IS_LABEL (widget))
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);

  gtk_table_attach (table, widget, 1, 2, row, row + 1,
                    GTK_FILL | GTK_EXPAND, GTK_FILL, 0, 0);
  gtk_widget_show (widget);
}

static void
cdisplay_lcms_update_profile_label (CdisplayLcms *lcms,
                                    const gchar  *name)
{
  GimpColorConfig  *config;
  GimpColorManaged *managed;
  GtkWidget        *label;
  GimpColorProfile *profile = NULL;
  const gchar      *text;
  const gchar      *tooltip;

  config  = gimp_color_display_get_config (GIMP_COLOR_DISPLAY (lcms));
  managed = gimp_color_display_get_managed (GIMP_COLOR_DISPLAY (lcms));

  label = g_object_get_data (G_OBJECT (lcms), name);

  if (! label)
    return;

  if (strcmp (name, "rgb-profile") == 0)
    {
      profile = gimp_color_managed_get_color_profile (managed);

      if (profile)
        g_object_ref (profile);
    }
  else if (g_str_has_prefix (name, "display-profile"))
    {
      profile = cdisplay_lcms_get_display_profile (lcms);
    }
  else if (strcmp (name, "printer-profile") == 0)
    {
      profile = gimp_color_config_get_printer_color_profile (config, NULL);
    }
  else
    {
      g_return_if_reached ();
    }

  if (profile)
    {
      text    = gimp_color_profile_get_label (profile);
      tooltip = gimp_color_profile_get_summary (profile);
    }
  else
    {
      text    = _("None");
      tooltip = NULL;
    }

  gtk_label_set_text (GTK_LABEL (label), text);
  gimp_help_set_help_data (label, tooltip, NULL);

  if (profile)
    g_object_unref (profile);
}

static void
cdisplay_lcms_notify_profile (GObject      *config,
                              GParamSpec   *pspec,
                              CdisplayLcms *lcms)
{
  cdisplay_lcms_update_profile_label (lcms, pspec->name);
}
