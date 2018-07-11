/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpdevicestatus.c
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#undef GSEAL_ENABLE

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpbrush.h"
#include "core/gimpcontext.h"
#include "core/gimpdatafactory.h"
#include "core/gimpgradient.h"
#include "core/gimplist.h"
#include "core/gimppattern.h"
#include "core/gimptoolinfo.h"

#include "gimpdnd.h"
#include "gimpdeviceinfo.h"
#include "gimpdevicemanager.h"
#include "gimpdevices.h"
#include "gimpdevicestatus.h"
#include "gimpdialogfactory.h"
#include "gimppropwidgets.h"
#include "gimpview.h"
#include "gimpwidgets-utils.h"
#include "gimpwindowstrategy.h"

#include "gimp-intl.h"


#define CELL_SIZE 20 /* The size of the view cells */


enum
{
  PROP_0,
  PROP_GIMP
};


struct _GimpDeviceStatusEntry
{
  GimpDeviceInfo  *device_info;
  GimpContext     *context;
  GimpToolOptions *tool_options;

  GtkWidget       *ebox;
  GtkWidget       *options_hbox;
  GtkWidget       *tool;
  GtkWidget       *foreground;
  GtkWidget       *foreground_none;
  GtkWidget       *background;
  GtkWidget       *background_none;
  GtkWidget       *brush;
  GtkWidget       *brush_none;
  GtkWidget       *pattern;
  GtkWidget       *pattern_none;
  GtkWidget       *gradient;
  GtkWidget       *gradient_none;
};


static void gimp_device_status_constructed     (GObject               *object);
static void gimp_device_status_dispose         (GObject               *object);
static void gimp_device_status_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);

static void gimp_device_status_device_add      (GimpContainer         *devices,
                                                GimpDeviceInfo        *device_info,
                                                GimpDeviceStatus      *status);
static void gimp_device_status_device_remove   (GimpContainer         *devices,
                                                GimpDeviceInfo        *device_info,
                                                GimpDeviceStatus      *status);

static void gimp_device_status_notify_device   (GimpDeviceManager     *manager,
                                                const GParamSpec      *pspec,
                                                GimpDeviceStatus      *status);
static void gimp_device_status_config_notify   (GimpGuiConfig         *config,
                                                const GParamSpec      *pspec,
                                                GimpDeviceStatus      *status);
static void gimp_device_status_notify_info     (GimpDeviceInfo        *device_info,
                                                const GParamSpec      *pspec,
                                                GimpDeviceStatusEntry *entry);
static void gimp_device_status_save_clicked    (GtkWidget             *button,
                                                GimpDeviceStatus      *status);
static void gimp_device_status_view_clicked    (GtkWidget             *widget,
                                                GdkModifierType        state,
                                                const gchar           *identifier);


G_DEFINE_TYPE (GimpDeviceStatus, gimp_device_status, GIMP_TYPE_EDITOR)

#define parent_class gimp_device_status_parent_class


static void
gimp_device_status_class_init (GimpDeviceStatusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_device_status_constructed;
  object_class->dispose      = gimp_device_status_dispose;
  object_class->set_property = gimp_device_status_set_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp", NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_device_status_init (GimpDeviceStatus *status)
{
  status->gimp           = NULL;
  status->current_device = NULL;

  status->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (status->vbox), 2);
  gtk_box_pack_start (GTK_BOX (status), status->vbox, TRUE, TRUE, 0);
  gtk_widget_show (status->vbox);

  status->save_button =
    gimp_editor_add_button (GIMP_EDITOR (status), GIMP_ICON_DOCUMENT_SAVE,
                            _("Save device status"), NULL,
                            G_CALLBACK (gimp_device_status_save_clicked),
                            NULL,
                            status);
}

static void
gimp_device_status_constructed (GObject *object)
{
  GimpDeviceStatus *status = GIMP_DEVICE_STATUS (object);
  GimpContainer    *devices;
  GList            *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_assert (GIMP_IS_GIMP (status->gimp));

  devices = GIMP_CONTAINER (gimp_devices_get_manager (status->gimp));

  for (list = GIMP_LIST (devices)->queue->head; list; list = list->next)
    gimp_device_status_device_add (devices, list->data, status);

  g_signal_connect_object (devices, "add",
                           G_CALLBACK (gimp_device_status_device_add),
                           status, 0);
  g_signal_connect_object (devices, "remove",
                           G_CALLBACK (gimp_device_status_device_remove),
                           status, 0);

  g_signal_connect (devices, "notify::current-device",
                    G_CALLBACK (gimp_device_status_notify_device),
                    status);

  gimp_device_status_notify_device (GIMP_DEVICE_MANAGER (devices), NULL, status);

  g_signal_connect_object (status->gimp->config, "notify::devices-share-tool",
                           G_CALLBACK (gimp_device_status_config_notify),
                           status, 0);

  gimp_device_status_config_notify (GIMP_GUI_CONFIG (status->gimp->config),
                                    NULL, status);
}

static void
gimp_device_status_dispose (GObject *object)
{
  GimpDeviceStatus *status = GIMP_DEVICE_STATUS (object);

  if (status->devices)
    {
      GList *list;

      for (list = status->devices; list; list = list->next)
        {
          GimpDeviceStatusEntry *entry = list->data;

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                gimp_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (GimpDeviceStatusEntry, entry);
        }

      g_list_free (status->devices);
      status->devices = NULL;

      g_signal_handlers_disconnect_by_func (gimp_devices_get_manager (status->gimp),
                                            gimp_device_status_notify_device,
                                            status);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_device_status_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpDeviceStatus *status = GIMP_DEVICE_STATUS (object);

  switch (property_id)
    {
    case PROP_GIMP:
      status->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
pack_prop_widget (GtkBox     *hbox,
                  GtkWidget  *widget,
                  GtkWidget **none_widget)
{
  GtkSizeGroup *size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, widget);
  gtk_widget_show (widget);

  *none_widget = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), *none_widget, FALSE, FALSE, 0);
  gtk_size_group_add_widget (size_group, *none_widget);

  g_object_unref (size_group);
}

static void
gimp_device_status_device_add (GimpContainer    *devices,
                               GimpDeviceInfo   *device_info,
                               GimpDeviceStatus *status)
{
  GimpDeviceStatusEntry *entry;
  GClosure              *closure;
  GParamSpec            *pspec;
  GtkWidget             *vbox;
  GtkWidget             *hbox;
  GtkWidget             *label;
  gchar                 *name;

  entry = g_slice_new0 (GimpDeviceStatusEntry);

  status->devices = g_list_prepend (status->devices, entry);

  entry->device_info = device_info;
  entry->context     = gimp_context_new (GIMP_TOOL_PRESET (device_info)->gimp,
                                         gimp_object_get_name (device_info),
                                         NULL);

  gimp_context_define_properties (entry->context,
                                  GIMP_CONTEXT_PROP_MASK_TOOL       |
                                  GIMP_CONTEXT_PROP_MASK_FOREGROUND |
                                  GIMP_CONTEXT_PROP_MASK_BACKGROUND |
                                  GIMP_CONTEXT_PROP_MASK_BRUSH      |
                                  GIMP_CONTEXT_PROP_MASK_PATTERN    |
                                  GIMP_CONTEXT_PROP_MASK_GRADIENT,
                                  FALSE);

  closure = g_cclosure_new (G_CALLBACK (gimp_device_status_notify_info),
                            entry, NULL);
  g_object_watch_closure (G_OBJECT (status), closure);
  g_signal_connect_closure (device_info, "notify", closure,
                            FALSE);

  entry->ebox = gtk_event_box_new ();
  gtk_box_pack_start (GTK_BOX (status->vbox), entry->ebox,
                      FALSE, FALSE, 0);
  gtk_widget_show (entry->ebox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_container_add (GTK_CONTAINER (entry->ebox), vbox);
  gtk_widget_show (vbox);

  /*  the device name  */

  if (device_info->display == NULL ||
      device_info->display == gdk_display_get_default ())
    name = g_strdup (gimp_object_get_name (device_info));
  else
    name = g_strdup_printf ("%s (%s)",
                            gimp_object_get_name (device_info),
                            gdk_display_get_name (device_info->display));

  label = gtk_label_new (name);
  g_free (name);

  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  the row of properties  */

  hbox = entry->options_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  the tool  */

  entry->tool = gimp_prop_view_new (G_OBJECT (entry->context), "tool",
                                    entry->context, CELL_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry->tool, FALSE, FALSE, 0);
  gtk_widget_show (entry->tool);

  /*  the foreground color  */

  entry->foreground = gimp_prop_color_area_new (G_OBJECT (entry->context),
                                                "foreground",
                                                CELL_SIZE, CELL_SIZE,
                                                GIMP_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->foreground,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->foreground, &entry->foreground_none);

  /*  the background color  */

  entry->background = gimp_prop_color_area_new (G_OBJECT (entry->context),
                                                "background",
                                                CELL_SIZE, CELL_SIZE,
                                                GIMP_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->background,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->background, &entry->background_none);

  /*  the brush  */

  entry->brush = gimp_prop_view_new (G_OBJECT (entry->context), "brush",
                                     entry->context, CELL_SIZE);
  GIMP_VIEW (entry->brush)->clickable  = TRUE;
  GIMP_VIEW (entry->brush)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->brush, &entry->brush_none);

  g_signal_connect (entry->brush, "clicked",
                    G_CALLBACK (gimp_device_status_view_clicked),
                    "gimp-brush-grid|gimp-brush-list");

  /*  the pattern  */

  entry->pattern = gimp_prop_view_new (G_OBJECT (entry->context), "pattern",
                                       entry->context, CELL_SIZE);
  GIMP_VIEW (entry->pattern)->clickable  = TRUE;
  GIMP_VIEW (entry->pattern)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->pattern, &entry->pattern_none);

  g_signal_connect (entry->pattern, "clicked",
                    G_CALLBACK (gimp_device_status_view_clicked),
                    "gimp-pattern-grid|gimp-pattern-list");

  /*  the gradient  */

  entry->gradient = gimp_prop_view_new (G_OBJECT (entry->context), "gradient",
                                        entry->context, 2 * CELL_SIZE);
  GIMP_VIEW (entry->gradient)->clickable  = TRUE;
  GIMP_VIEW (entry->gradient)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->gradient, &entry->gradient_none);

  g_signal_connect (entry->gradient, "clicked",
                    G_CALLBACK (gimp_device_status_view_clicked),
                    "gimp-gradient-list|gimp-gradient-grid");

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (device_info),
                                        "tool-options");
  gimp_device_status_notify_info (device_info, pspec, entry);
}

static void
gimp_device_status_device_remove (GimpContainer    *devices,
                                  GimpDeviceInfo   *device_info,
                                  GimpDeviceStatus *status)
{
  GList *list;

  for (list = status->devices; list; list = list->next)
    {
      GimpDeviceStatusEntry *entry = list->data;

      if (entry->device_info == device_info)
        {
          status->devices = g_list_remove (status->devices, entry);

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                gimp_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (GimpDeviceStatusEntry, entry);

          return;
        }
    }
}

GtkWidget *
gimp_device_status_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_DEVICE_STATUS,
                       "gimp", gimp,
                       NULL);
}


/*  private functions  */

static void
gimp_device_status_notify_device (GimpDeviceManager *manager,
                                  const GParamSpec  *pspec,
                                  GimpDeviceStatus  *status)
{
  GList *list;

  status->current_device = gimp_device_manager_get_current_device (manager);

  for (list = status->devices; list; list = list->next)
    {
      GimpDeviceStatusEntry *entry = list->data;

      gtk_widget_set_state (entry->ebox,
                            entry->device_info == status->current_device ?
                            GTK_STATE_SELECTED : GTK_STATE_NORMAL);
    }
}

static void
gimp_device_status_config_notify (GimpGuiConfig    *config,
                                  const GParamSpec *pspec,
                                  GimpDeviceStatus *status)
{
  gboolean  show_options;
  GList    *list;

  show_options = ! GIMP_GUI_CONFIG (status->gimp->config)->devices_share_tool;

  for (list = status->devices; list; list = list->next)
    {
      GimpDeviceStatusEntry *entry = list->data;

      gtk_widget_set_visible (entry->options_hbox, show_options);
    }
}

static void
toggle_prop_visible (GtkWidget *widget,
                     GtkWidget *widget_none,
                     gboolean   available)
{
  gtk_widget_set_visible (widget, available);
  gtk_widget_set_visible (widget_none, ! available);
}

static void
gimp_device_status_notify_info (GimpDeviceInfo        *device_info,
                                const GParamSpec      *pspec,
                                GimpDeviceStatusEntry *entry)
{
  GimpToolOptions *tool_options = GIMP_TOOL_PRESET (device_info)->tool_options;

  if (tool_options != entry->tool_options)
    {
      GimpContextPropMask serialize_props;

      entry->tool_options = tool_options;
      gimp_context_set_parent (entry->context, GIMP_CONTEXT (tool_options));

      serialize_props =
        gimp_context_get_serialize_properties (GIMP_CONTEXT (tool_options));

      toggle_prop_visible (entry->foreground,
                           entry->foreground_none,
                           serialize_props & GIMP_CONTEXT_PROP_MASK_FOREGROUND);

      toggle_prop_visible (entry->background,
                           entry->background_none,
                           serialize_props & GIMP_CONTEXT_PROP_MASK_BACKGROUND);

      toggle_prop_visible (entry->brush,
                           entry->brush_none,
                           serialize_props & GIMP_CONTEXT_PROP_MASK_BRUSH);

      toggle_prop_visible (entry->pattern,
                           entry->pattern_none,
                           serialize_props & GIMP_CONTEXT_PROP_MASK_PATTERN);

      toggle_prop_visible (entry->gradient,
                           entry->gradient_none,
                           serialize_props & GIMP_CONTEXT_PROP_MASK_GRADIENT);
    }

  if (! gimp_device_info_get_device (device_info, NULL) ||
      gimp_device_info_get_mode (device_info) == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (entry->ebox);
    }
  else
    {
      gtk_widget_show (entry->ebox);
    }

  if (! strcmp (pspec->name, "tool-options"))
    {
      GimpRGB color;
      guchar  r, g, b;
      gchar   buf[64];

      gimp_context_get_foreground (entry->context, &color);
      gimp_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Foreground: %d, %d, %d"), r, g, b);
      gimp_help_set_help_data (entry->foreground, buf, NULL);

      gimp_context_get_background (entry->context, &color);
      gimp_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Background: %d, %d, %d"), r, g, b);
      gimp_help_set_help_data (entry->background, buf, NULL);
    }
}

static void
gimp_device_status_save_clicked (GtkWidget        *button,
                                 GimpDeviceStatus *status)
{
  gimp_devices_save (status->gimp, TRUE);
}

static void
gimp_device_status_view_clicked (GtkWidget       *widget,
                                 GdkModifierType  state,
                                 const gchar     *identifier)
{
  GimpDeviceStatus  *status;
  GimpDialogFactory *dialog_factory;

  status = GIMP_DEVICE_STATUS (gtk_widget_get_ancestor (widget,
                                                        GIMP_TYPE_DEVICE_STATUS));
  dialog_factory = gimp_dialog_factory_get_singleton ();

  gimp_window_strategy_show_dockable_dialog (GIMP_WINDOW_STRATEGY (gimp_get_window_strategy (status->gimp)),
                                             status->gimp,
                                             dialog_factory,
                                             gtk_widget_get_screen (widget),
                                             gimp_widget_get_monitor (widget),
                                             identifier);
}
