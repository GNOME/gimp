/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimppropgui-newsprint.c
 * Copyright (C) 2019  Michael Natterer <mitch@gimp.org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "propgui-types.h"

#include "core/gimpcontext.h"

#include "gimppropgui.h"
#include "gimppropgui-generic.h"
#include "gimppropgui-newsprint.h"

#include "gimp-intl.h"


typedef enum
{
  COLOR_MODEL_WHITE_ON_BLACK,
  COLOR_MODEL_BLACK_ON_WHITE,
  COLOR_MODEL_RGB,
  COLOR_MODEL_CMYK,

  N_COLOR_MODELS
} ColorModel;

typedef enum
{
  PATTERN_LINE,
  PATTERN_CIRCLE,
  PATTERN_DIAMOND,
  PATTERN_PSCIRCLE,
  PATTERN_CROSS,

  N_PATTERNS
} Pattern;


typedef struct _Newsprint Newsprint;

struct _Newsprint
{
  GObject   *config;
  GtkWidget *notebook;
  GtkWidget *pattern_check;
  GtkWidget *period_check;
  GtkWidget *angle_check;
};


/*  local function prototypes  */

static void   newsprint_color_model_notify    (GObject          *config,
                                               const GParamSpec *pspec,
                                               GtkWidget        *label);
static void   newsprint_model_prop_notify     (GObject          *config,
                                               const GParamSpec *pspec,
                                               Newsprint        *np);
static void   newsprint_lock_patterns_toggled (GtkWidget        *widget,
                                               Newsprint        *np);
static void   newsprint_lock_periods_toggled  (GtkWidget        *widget,
                                               Newsprint        *np);
static void   newsprint_lock_angles_toggled   (GtkWidget        *widget,
                                               Newsprint        *np);


static const gchar *label_strings[N_COLOR_MODELS][4] =
{
  { NULL,       NULL,          NULL,         N_("White") },
  { NULL,       NULL,          NULL,         N_("Black") },
  { N_("Red"),  N_("Green"),   N_("Blue"),   NULL        },
  { N_("Cyan"), N_("Magenta"), N_("Yellow"), N_("Black") }
};

static const gchar *pattern_props[] =
{
  "pattern2",
  "pattern3",
  "pattern4",
  "pattern"
};

static const gchar *period_props[] =
{
  "period2",
  "period3",
  "period4",
  "period"
};

static const gchar *angle_props[] =
{
  "angle2",
  "angle3",
  "angle4",
  "angle"
};


/*  public functions  */

GtkWidget *
_gimp_prop_gui_new_newsprint (GObject                  *config,
                              GParamSpec              **param_specs,
                              guint                     n_param_specs,
                              GeglRectangle            *area,
                              GimpContext              *context,
                              GimpCreatePickerFunc      create_picker_func,
                              GimpCreateControllerFunc  create_controller_func,
                              gpointer                  creator)
{
  Newsprint *np;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *combo;
  GtkWidget *labels[4];
  GtkWidget *pages[4];
  GtkWidget *check;
  gint       i;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);

  np = g_new0 (Newsprint, 1);

  np->config = config;

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  g_object_set_data_full (G_OBJECT (main_vbox), "newsprint", np,
                          (GDestroyNotify) g_free);

  frame = gimp_frame_new (_("Channels"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  combo = _gimp_prop_gui_new_generic (config,
                                      param_specs + 0, 1,
                                      area,
                                      context,
                                      create_picker_func,
                                      create_controller_func,
                                      creator);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  np->notebook = gtk_notebook_new ();
  gtk_box_pack_start (GTK_BOX (vbox), np->notebook, FALSE, FALSE, 0);
  gtk_widget_show (np->notebook);

  for (i = 0; i < 4; i++)
    {
      GtkWidget   *widget;
      const gchar *unused;
      gint         remaining;

      labels[i] = gtk_label_new (NULL);
      pages[i]  = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

      g_object_set_data (G_OBJECT (labels[i]), "channel", GINT_TO_POINTER (i));

      g_signal_connect_object (config, "notify::color-model",
                               G_CALLBACK (newsprint_color_model_notify),
                               G_OBJECT (labels[i]), 0);

      newsprint_color_model_notify (config, NULL, labels[i]);

      gtk_container_set_border_width (GTK_CONTAINER (pages[i]), 6);
      gtk_notebook_append_page (GTK_NOTEBOOK (np->notebook),
                                pages[i], labels[i]);

      widget = gimp_prop_widget_new_from_pspec (config,
                                                param_specs[1 + 3 * i],
                                                area, context,
                                                create_picker_func,
                                                create_controller_func,
                                                creator, &unused);
      gtk_box_pack_start (GTK_BOX (pages[i]), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);

      g_object_bind_property (G_OBJECT (widget),   "visible",
                              G_OBJECT (pages[i]), "visible",
                              G_BINDING_SYNC_CREATE);

      if (i == 3)
        remaining = 3;
      else
        remaining = 2;

      widget = _gimp_prop_gui_new_generic (config,
                                           param_specs + 2 + 3 * i, remaining,
                                           area, context,
                                           create_picker_func,
                                           create_controller_func,
                                           creator);
      gtk_box_pack_start (GTK_BOX (pages[i]), widget, FALSE, FALSE, 0);
      gtk_widget_show (widget);
    }

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  np->pattern_check = check =
    gtk_check_button_new_with_mnemonic (_("_Lock patterns"));
  gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (newsprint_lock_patterns_toggled),
                    np);

  np->period_check = check =
    gtk_check_button_new_with_mnemonic (_("Loc_k periods"));
  gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (newsprint_lock_periods_toggled),
                    np);

  np->angle_check = check =
    gtk_check_button_new_with_mnemonic (_("Lock a_ngles"));
  gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);
  gtk_widget_show (check);

  g_signal_connect (check, "toggled",
                    G_CALLBACK (newsprint_lock_angles_toggled),
                    np);

  frame = gimp_frame_new (_("Quality"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 14, 1,
                                     area,
                                     context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  frame = gimp_frame_new (_("Effects"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = _gimp_prop_gui_new_generic (config,
                                     param_specs + 15, 3,
                                     area,
                                     context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  g_signal_connect (config, "notify",
                    G_CALLBACK (newsprint_model_prop_notify),
                    np);

  return main_vbox;
}


/*  private functions  */

static void
newsprint_color_model_notify (GObject          *config,
                              const GParamSpec *pspec,
                              GtkWidget        *label)
{
  ColorModel  model;
  gint        channel;

  g_object_get (config,
                "color-model", &model,
                NULL);

  channel =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "channel"));

  if (label_strings[model][channel])
    gtk_label_set_text (GTK_LABEL (label),
                        gettext (label_strings[model][channel]));
}

static gboolean
newsprint_sync_model_props (Newsprint         *np,
                            const GParamSpec  *pspec,
                            const gchar      **props,
                            gint               n_props)
{
  gint i;

  for (i = 0; i < n_props; i++)
    {
      if (! strcmp (pspec->name, props[i]))
        {
          GValue value = G_VALUE_INIT;
          gint   j;

          g_value_init (&value, pspec->value_type);

          g_object_get_property (np->config, pspec->name, &value);

          for (j = 0; j < n_props; j++)
            {
              if (i != j)
                {
                  g_signal_handlers_block_by_func (np->config,
                                                   newsprint_model_prop_notify,
                                                   np);

                  g_object_set_property (np->config, props[j], &value);

                  g_signal_handlers_unblock_by_func (np->config,
                                                     newsprint_model_prop_notify,
                                                     np);
                }
            }

          g_value_unset (&value);

          return TRUE;
        }
    }

  return FALSE;
}

static void
newsprint_model_prop_notify (GObject          *config,
                             const GParamSpec *pspec,
                             Newsprint        *np)
{

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (np->pattern_check)) &&
      newsprint_sync_model_props (np, pspec,
                                  pattern_props, G_N_ELEMENTS (pattern_props)))
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (np->period_check)) &&
      newsprint_sync_model_props (np, pspec,
                                  period_props, G_N_ELEMENTS (period_props)))
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (np->angle_check)) &&
      newsprint_sync_model_props (np, pspec,
                                  angle_props, G_N_ELEMENTS (angle_props)))
    return;
}

static void
newsprint_lock_patterns_toggled (GtkWidget *widget,
                                 Newsprint *np)

{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GParamSpec *pspec;
      gint        channel;

      channel = gtk_notebook_get_current_page (GTK_NOTEBOOK (np->notebook));

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (np->config),
                                            pattern_props[channel]);

      newsprint_sync_model_props (np, pspec,
                                  pattern_props, G_N_ELEMENTS (pattern_props));
    }
}

static void
newsprint_lock_periods_toggled (GtkWidget *widget,
                                Newsprint *np)

{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GParamSpec *pspec;
      gint        channel;

      channel = gtk_notebook_get_current_page (GTK_NOTEBOOK (np->notebook));

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (np->config),
                                            period_props[channel]);

      newsprint_sync_model_props (np, pspec,
                                  period_props, G_N_ELEMENTS (period_props));
    }
}

static void
newsprint_lock_angles_toggled (GtkWidget *widget,
                               Newsprint *np)

{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GParamSpec *pspec;
      gint        channel;

      channel = gtk_notebook_get_current_page (GTK_NOTEBOOK (np->notebook));

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (np->config),
                                            angle_props[channel]);

      newsprint_sync_model_props (np, pspec,
                                  angle_props, G_N_ELEMENTS (angle_props));
    }
}
