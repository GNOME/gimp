/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * cdisplay_colorblind.c
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
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

#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "libgimp/libgimp-intl.h"


typedef enum
{
  COLORBLIND_DEFICIENCY_NONE,
  COLORBLIND_DEFICIENCY_PROTONOPIA,
  COLORBLIND_DEFICIENCY_DEUTERANOPIA,
  COLORBLIND_DEFICIENCY_TRITANOPIA,
  COLORBLIND_DEFICIENCY_LAST = COLORBLIND_DEFICIENCY_TRITANOPIA
} ColorblindDeficiency;


#define DEFAULT_DEFICIENCY COLORBLIND_DEFICIENCY_DEUTERANOPIA


#define CDISPLAY_TYPE_COLORBLIND            (cdisplay_colorblind_type)
#define CDISPLAY_COLORBLIND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDISPLAY_TYPE_COLORBLIND, CdisplayColorblind))
#define CDISPLAY_COLORBLIND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CDISPLAY_TYPE_COLORBLIND, CdisplayColorblindClass))
#define CDISPLAY_IS_COLORBLIND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDISPLAY_TYPE_COLORBLIND))
#define CDISPLAY_IS_COLORBLIND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDISPLAY_TYPE_COLORBLIND))


typedef struct _CdisplayColorblind      CdisplayColorblind;
typedef struct _CdisplayColorblindClass CdisplayColorblindClass;

struct _CdisplayColorblind
{
  GimpColorDisplay      parent_instance;

  ColorblindDeficiency  deficiency;

  GtkWidget            *hbox;
  GtkWidget            *optionmenu;
};

struct _CdisplayColorblindClass
{
  GimpColorDisplayClass  parent_instance;
};


static GType   cdisplay_colorblind_get_type   (GTypeModule             *module);
static void    cdisplay_colorblind_class_init (CdisplayColorblindClass *klass);
static void    cdisplay_colorblind_init       (CdisplayColorblind      *colorblind);

static void    cdisplay_colorblind_finalize          (GObject            *object);

static GimpColorDisplay * cdisplay_colorblind_clone  (GimpColorDisplay   *display);
static void    cdisplay_colorblind_convert           (GimpColorDisplay   *display,
                                                      guchar             *buf,
                                                      gint                w,
                                                      gint                h,
                                                      gint                bpp,
                                                      gint                bpl);
static void    cdisplay_colorblind_load_state        (GimpColorDisplay   *display,
                                                      GimpParasite       *state);
static GimpParasite * cdisplay_colorblind_save_state (GimpColorDisplay   *display);
static GtkWidget    * cdisplay_colorblind_configure  (GimpColorDisplay   *display);
static void    cdisplay_colorblind_configure_reset   (GimpColorDisplay   *display);

static void    colorblind_deficiency_callback        (GtkWidget          *widget,
                                                      CdisplayColorblind *colorblind);


static const GimpModuleInfo cdisplay_colorblind_info = 
{
  N_("Colorblind display filter"),
  "Michael Natterer <mitch@gimp.org>",
  "v0.1",
  "(c) 2002, released under the GPL",
  "December 16, 2002"
};

static GType                  cdisplay_colorblind_type = 0;
static GimpColorDisplayClass *parent_class             = NULL;


G_MODULE_EXPORT const GimpModuleInfo *
gimp_module_query (GTypeModule *module)
{
  return &cdisplay_colorblind_info;
}

G_MODULE_EXPORT gboolean
gimp_module_register (GTypeModule *module)
{
  cdisplay_colorblind_get_type (module);

  return TRUE;
}

static GType
cdisplay_colorblind_get_type (GTypeModule *module)
{
  if (! cdisplay_colorblind_type)
    {
      static const GTypeInfo select_info =
      {
        sizeof (CdisplayColorblindClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) cdisplay_colorblind_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data     */
	sizeof (CdisplayColorblind),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) cdisplay_colorblind_init,
      };

      cdisplay_colorblind_type =
        g_type_module_register_type (module,
                                     GIMP_TYPE_COLOR_DISPLAY,
                                     "CdisplayColorblind",
                                     &select_info, 0);
    }

  return cdisplay_colorblind_type;
}

static void
cdisplay_colorblind_class_init (CdisplayColorblindClass *klass)
{
  GObjectClass          *object_class;
  GimpColorDisplayClass *display_class;

  object_class  = G_OBJECT_CLASS (klass);
  display_class = GIMP_COLOR_DISPLAY_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize         = cdisplay_colorblind_finalize;

  display_class->name            = _("Colorblind");
  display_class->help_page       = "modules/colorblind.html";
  display_class->clone           = cdisplay_colorblind_clone;
  display_class->convert         = cdisplay_colorblind_convert;
  display_class->load_state      = cdisplay_colorblind_load_state;
  display_class->save_state      = cdisplay_colorblind_save_state;
  display_class->configure       = cdisplay_colorblind_configure;
  display_class->configure_reset = cdisplay_colorblind_configure_reset;
}

static void
cdisplay_colorblind_init (CdisplayColorblind *colorblind)
{
  colorblind->deficiency = DEFAULT_DEFICIENCY;
}

static void
cdisplay_colorblind_finalize (GObject *object)
{
  CdisplayColorblind *colorblind;

  colorblind = CDISPLAY_COLORBLIND (object);

  if (colorblind->hbox)
    gtk_widget_destroy (colorblind->hbox);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static GimpColorDisplay *
cdisplay_colorblind_clone (GimpColorDisplay *display)
{
  CdisplayColorblind *colorblind;
  CdisplayColorblind *copy;

  colorblind = CDISPLAY_COLORBLIND (display);

  copy = CDISPLAY_COLORBLIND (gimp_color_display_new (G_TYPE_FROM_INSTANCE (colorblind)));

  copy->deficiency = colorblind->deficiency;

  return GIMP_COLOR_DISPLAY (copy);
}

static void
cdisplay_colorblind_convert (GimpColorDisplay *display,
                             guchar           *buf,
                             gint              width,
                             gint              height,
                             gint              bpp,
                             gint              bpl)
{
  CdisplayColorblind *colorblind;
  gint                i, j;
  guchar             *b;

  colorblind = CDISPLAY_COLORBLIND (display);

  if (bpp != 3)
    return;

  j = height;

  while (j--)
    {
      i = width;
      b = buf;

      switch (colorblind->deficiency)
        {
        case COLORBLIND_DEFICIENCY_PROTONOPIA:
        case COLORBLIND_DEFICIENCY_DEUTERANOPIA:
          /*  FIXME: need proper formulas  */
          while (i--)
            {
              guchar red   = b[0] >> 1;
              guchar green = b[1] >> 1;

              b[0] = b[1] = red + green;

              b += bpp;
            }
          break;

        case COLORBLIND_DEFICIENCY_TRITANOPIA:
          /*  FIXME: need proper formula  */
          while (i--)
            {
              b[2] = 0;

              b += bpp;
            }
          break;

        default:
          break;
        }

      buf += bpl;
    }
}

static void
cdisplay_colorblind_load_state (GimpColorDisplay *display,
                                GimpParasite     *state)
{
  CdisplayColorblind *colorblind;
  gchar              *str;

  colorblind = CDISPLAY_COLORBLIND (display);

  str = gimp_parasite_data (state);

  if (str[gimp_parasite_data_size (state) - 1] == '\0')
    {
      gint deficiency;

      if (sscanf (str, "%d", &deficiency) == 1)
        {
          if (deficiency > COLORBLIND_DEFICIENCY_NONE &&
              deficiency <= COLORBLIND_DEFICIENCY_LAST)
            {
              colorblind->deficiency = deficiency;

              gimp_color_display_changed (GIMP_COLOR_DISPLAY (colorblind));
            }
        }
    }
}

static GimpParasite *
cdisplay_colorblind_save_state (GimpColorDisplay *display)
{
  CdisplayColorblind *colorblind;
  gchar               buf[32];

  colorblind = CDISPLAY_COLORBLIND (display);

  g_snprintf (buf, sizeof (buf), "%d", colorblind->deficiency);

  return gimp_parasite_new ("Display/Colorblind", GIMP_PARASITE_PERSISTENT,
			    strlen (buf) + 1, buf);
}

static GtkWidget *
cdisplay_colorblind_configure (GimpColorDisplay *display)
{
  CdisplayColorblind *colorblind;
  GtkWidget          *label;

  colorblind = CDISPLAY_COLORBLIND (display);

  if (colorblind->hbox)
    gtk_widget_destroy (colorblind->hbox);

  colorblind->hbox = gtk_hbox_new (FALSE, 2);
  g_object_add_weak_pointer (G_OBJECT (colorblind->hbox),
                             (gpointer) &colorblind->hbox);

  label = gtk_label_new ( _("Color Deficiency Type:"));
  gtk_box_pack_start (GTK_BOX (colorblind->hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  colorblind->optionmenu =
    gimp_option_menu_new2 (FALSE,
                           G_CALLBACK (colorblind_deficiency_callback),
                           colorblind,
                           GINT_TO_POINTER (colorblind->deficiency),

                           _("Protonopia (insensitivity to red)"),
                           GINT_TO_POINTER (COLORBLIND_DEFICIENCY_PROTONOPIA),
                           NULL,

                           _("Deuteranopia (insensitivity to green)"),
                           GINT_TO_POINTER (COLORBLIND_DEFICIENCY_DEUTERANOPIA),
                           NULL,

                           _("Tritanopia (insensitivity to blue)"),
                           GINT_TO_POINTER (COLORBLIND_DEFICIENCY_TRITANOPIA),
                           NULL,

                           NULL);

  gtk_box_pack_start (GTK_BOX (colorblind->hbox), colorblind->optionmenu,
                      FALSE, FALSE, 0);
  gtk_widget_show (colorblind->optionmenu);

  return colorblind->hbox;
}

static void
cdisplay_colorblind_configure_reset (GimpColorDisplay *display)
{
  CdisplayColorblind *colorblind;
 
  colorblind = CDISPLAY_COLORBLIND (display);

  if (colorblind->optionmenu)
    {
      gimp_option_menu_set_history (GTK_OPTION_MENU (colorblind->optionmenu),
                                    GINT_TO_POINTER (DEFAULT_DEFICIENCY));
      colorblind->deficiency = DEFAULT_DEFICIENCY;

      gimp_color_display_changed (GIMP_COLOR_DISPLAY (colorblind));
    }
}

static void
colorblind_deficiency_callback (GtkWidget          *widget,
                                CdisplayColorblind *colorblind)
{
  gimp_menu_item_update (widget, &colorblind->deficiency);

  gimp_color_display_changed (GIMP_COLOR_DISPLAY (colorblind));
}
