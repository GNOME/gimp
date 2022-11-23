/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmadevicestatus.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmabrush.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmagradient.h"
#include "core/ligmalist.h"
#include "core/ligmapattern.h"
#include "core/ligmatoolinfo.h"

#include "ligmadnd.h"
#include "ligmadeviceinfo.h"
#include "ligmadevicemanager.h"
#include "ligmadevices.h"
#include "ligmadevicestatus.h"
#include "ligmadialogfactory.h"
#include "ligmapropwidgets.h"
#include "ligmaview.h"
#include "ligmawidgets-utils.h"
#include "ligmawindowstrategy.h"

#include "ligma-intl.h"


#define CELL_SIZE 20 /* The size of the view cells */


enum
{
  PROP_0,
  PROP_LIGMA
};


struct _LigmaDeviceStatusEntry
{
  LigmaDeviceInfo  *device_info;
  LigmaContext     *context;
  LigmaToolOptions *tool_options;

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


static void ligma_device_status_constructed     (GObject               *object);
static void ligma_device_status_dispose         (GObject               *object);
static void ligma_device_status_set_property    (GObject               *object,
                                                guint                  property_id,
                                                const GValue          *value,
                                                GParamSpec            *pspec);

static void ligma_device_status_device_add      (LigmaContainer         *devices,
                                                LigmaDeviceInfo        *device_info,
                                                LigmaDeviceStatus      *status);
static void ligma_device_status_device_remove   (LigmaContainer         *devices,
                                                LigmaDeviceInfo        *device_info,
                                                LigmaDeviceStatus      *status);

static void ligma_device_status_notify_device   (LigmaDeviceManager     *manager,
                                                const GParamSpec      *pspec,
                                                LigmaDeviceStatus      *status);
static void ligma_device_status_config_notify   (LigmaGuiConfig         *config,
                                                const GParamSpec      *pspec,
                                                LigmaDeviceStatus      *status);
static void ligma_device_status_notify_info     (LigmaDeviceInfo        *device_info,
                                                const GParamSpec      *pspec,
                                                LigmaDeviceStatusEntry *entry);
static void ligma_device_status_save_clicked    (GtkWidget             *button,
                                                LigmaDeviceStatus      *status);
static void ligma_device_status_view_clicked    (GtkWidget             *widget,
                                                GdkModifierType        state,
                                                const gchar           *identifier);


G_DEFINE_TYPE (LigmaDeviceStatus, ligma_device_status, LIGMA_TYPE_EDITOR)

#define parent_class ligma_device_status_parent_class


static void
ligma_device_status_class_init (LigmaDeviceStatusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_device_status_constructed;
  object_class->dispose      = ligma_device_status_dispose;
  object_class->set_property = ligma_device_status_set_property;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_device_status_init (LigmaDeviceStatus *status)
{
  status->ligma           = NULL;
  status->current_device = NULL;

  status->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_set_border_width (GTK_CONTAINER (status->vbox), 2);
  gtk_box_pack_start (GTK_BOX (status), status->vbox, TRUE, TRUE, 0);
  gtk_widget_show (status->vbox);

  status->save_button =
    ligma_editor_add_button (LIGMA_EDITOR (status), LIGMA_ICON_DOCUMENT_SAVE,
                            _("Save device status"), NULL,
                            G_CALLBACK (ligma_device_status_save_clicked),
                            NULL,
                            status);
}

static void
ligma_device_status_constructed (GObject *object)
{
  LigmaDeviceStatus *status = LIGMA_DEVICE_STATUS (object);
  LigmaContainer    *devices;
  GList            *list;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_LIGMA (status->ligma));

  devices = LIGMA_CONTAINER (ligma_devices_get_manager (status->ligma));

  for (list = LIGMA_LIST (devices)->queue->head; list; list = list->next)
    ligma_device_status_device_add (devices, list->data, status);

  g_signal_connect_object (devices, "add",
                           G_CALLBACK (ligma_device_status_device_add),
                           status, 0);
  g_signal_connect_object (devices, "remove",
                           G_CALLBACK (ligma_device_status_device_remove),
                           status, 0);

  g_signal_connect (devices, "notify::current-device",
                    G_CALLBACK (ligma_device_status_notify_device),
                    status);

  ligma_device_status_notify_device (LIGMA_DEVICE_MANAGER (devices), NULL, status);

  g_signal_connect_object (status->ligma->config, "notify::devices-share-tool",
                           G_CALLBACK (ligma_device_status_config_notify),
                           status, 0);

  ligma_device_status_config_notify (LIGMA_GUI_CONFIG (status->ligma->config),
                                    NULL, status);
}

static void
ligma_device_status_dispose (GObject *object)
{
  LigmaDeviceStatus *status = LIGMA_DEVICE_STATUS (object);

  if (status->devices)
    {
      GList *list;

      for (list = status->devices; list; list = list->next)
        {
          LigmaDeviceStatusEntry *entry = list->data;

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                ligma_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (LigmaDeviceStatusEntry, entry);
        }

      g_list_free (status->devices);
      status->devices = NULL;

      g_signal_handlers_disconnect_by_func (ligma_devices_get_manager (status->ligma),
                                            ligma_device_status_notify_device,
                                            status);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_device_status_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaDeviceStatus *status = LIGMA_DEVICE_STATUS (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      status->ligma = g_value_get_object (value);
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
ligma_device_status_device_add (LigmaContainer    *devices,
                               LigmaDeviceInfo   *device_info,
                               LigmaDeviceStatus *status)
{
  LigmaDeviceStatusEntry *entry;
  GdkDisplay            *display;

  GClosure              *closure;
  GParamSpec            *pspec;
  GtkWidget             *vbox;
  GtkWidget             *hbox;
  GtkWidget             *label;
  gchar                 *name;

  entry = g_slice_new0 (LigmaDeviceStatusEntry);

  status->devices = g_list_prepend (status->devices, entry);

  entry->device_info = device_info;
  entry->context     = ligma_context_new (LIGMA_TOOL_PRESET (device_info)->ligma,
                                         ligma_object_get_name (device_info),
                                         NULL);

  ligma_context_define_properties (entry->context,
                                  LIGMA_CONTEXT_PROP_MASK_TOOL       |
                                  LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                                  LIGMA_CONTEXT_PROP_MASK_BACKGROUND |
                                  LIGMA_CONTEXT_PROP_MASK_BRUSH      |
                                  LIGMA_CONTEXT_PROP_MASK_PATTERN    |
                                  LIGMA_CONTEXT_PROP_MASK_GRADIENT,
                                  FALSE);

  closure = g_cclosure_new (G_CALLBACK (ligma_device_status_notify_info),
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

  ligma_device_info_get_device (device_info, &display);
  if (display == NULL || display == gdk_display_get_default ())
    name = g_strdup (ligma_object_get_name (device_info));
  else
    name = g_strdup_printf ("%s (%s)",
                            ligma_object_get_name (device_info),
                            gdk_display_get_name (display));

  label = gtk_label_new (name);
  g_free (name);

  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  ligma_label_set_attributes (GTK_LABEL (label),
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

  entry->tool = ligma_prop_view_new (G_OBJECT (entry->context), "tool",
                                    entry->context, CELL_SIZE);
  gtk_box_pack_start (GTK_BOX (hbox), entry->tool, FALSE, FALSE, 0);

  /*  the foreground color  */

  entry->foreground = ligma_prop_color_area_new (G_OBJECT (entry->context),
                                                "foreground",
                                                CELL_SIZE, CELL_SIZE,
                                                LIGMA_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->foreground,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->foreground, &entry->foreground_none);

  /*  the background color  */

  entry->background = ligma_prop_color_area_new (G_OBJECT (entry->context),
                                                "background",
                                                CELL_SIZE, CELL_SIZE,
                                                LIGMA_COLOR_AREA_FLAT);
  gtk_widget_add_events (entry->background,
                         GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
  pack_prop_widget (GTK_BOX (hbox), entry->background, &entry->background_none);

  /*  the brush  */

  entry->brush = ligma_prop_view_new (G_OBJECT (entry->context), "brush",
                                     entry->context, CELL_SIZE);
  LIGMA_VIEW (entry->brush)->clickable  = TRUE;
  LIGMA_VIEW (entry->brush)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->brush, &entry->brush_none);

  g_signal_connect (entry->brush, "clicked",
                    G_CALLBACK (ligma_device_status_view_clicked),
                    "ligma-brush-grid|ligma-brush-list");

  /*  the pattern  */

  entry->pattern = ligma_prop_view_new (G_OBJECT (entry->context), "pattern",
                                       entry->context, CELL_SIZE);
  LIGMA_VIEW (entry->pattern)->clickable  = TRUE;
  LIGMA_VIEW (entry->pattern)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->pattern, &entry->pattern_none);

  g_signal_connect (entry->pattern, "clicked",
                    G_CALLBACK (ligma_device_status_view_clicked),
                    "ligma-pattern-grid|ligma-pattern-list");

  /*  the gradient  */

  entry->gradient = ligma_prop_view_new (G_OBJECT (entry->context), "gradient",
                                        entry->context, 2 * CELL_SIZE);
  LIGMA_VIEW (entry->gradient)->clickable  = TRUE;
  LIGMA_VIEW (entry->gradient)->show_popup = TRUE;
  pack_prop_widget (GTK_BOX (hbox), entry->gradient, &entry->gradient_none);

  g_signal_connect (entry->gradient, "clicked",
                    G_CALLBACK (ligma_device_status_view_clicked),
                    "ligma-gradient-list|ligma-gradient-grid");

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (device_info),
                                        "tool-options");
  ligma_device_status_notify_info (device_info, pspec, entry);
}

static void
ligma_device_status_device_remove (LigmaContainer    *devices,
                                  LigmaDeviceInfo   *device_info,
                                  LigmaDeviceStatus *status)
{
  GList *list;

  for (list = status->devices; list; list = list->next)
    {
      LigmaDeviceStatusEntry *entry = list->data;

      if (entry->device_info == device_info)
        {
          status->devices = g_list_remove (status->devices, entry);

          g_signal_handlers_disconnect_by_func (entry->device_info,
                                                ligma_device_status_notify_info,
                                                entry);

          g_object_unref (entry->context);
          g_slice_free (LigmaDeviceStatusEntry, entry);

          return;
        }
    }
}

GtkWidget *
ligma_device_status_new (Ligma *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  return g_object_new (LIGMA_TYPE_DEVICE_STATUS,
                       "ligma", ligma,
                       NULL);
}


/*  private functions  */

static void
ligma_device_status_notify_device (LigmaDeviceManager *manager,
                                  const GParamSpec  *pspec,
                                  LigmaDeviceStatus  *status)
{
  GList *list;

  status->current_device = ligma_device_manager_get_current_device (manager);

  for (list = status->devices; list; list = list->next)
    {
      LigmaDeviceStatusEntry *entry  = list->data;
      GtkWidget             *widget = entry->ebox;
      GtkStyleContext       *style  = gtk_widget_get_style_context (widget);

      if (entry->device_info == status->current_device)
        {
          gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_SELECTED, TRUE);
          gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
        }
      else
        {
          gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_NORMAL, TRUE);
          gtk_style_context_remove_class (style, GTK_STYLE_CLASS_VIEW);
        }
    }
}

static void
ligma_device_status_config_notify (LigmaGuiConfig    *config,
                                  const GParamSpec *pspec,
                                  LigmaDeviceStatus *status)
{
  gboolean  show_options;
  GList    *list;

  show_options = ! LIGMA_GUI_CONFIG (status->ligma->config)->devices_share_tool;

  for (list = status->devices; list; list = list->next)
    {
      LigmaDeviceStatusEntry *entry = list->data;

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
ligma_device_status_notify_info (LigmaDeviceInfo        *device_info,
                                const GParamSpec      *pspec,
                                LigmaDeviceStatusEntry *entry)
{
  LigmaToolOptions *tool_options = LIGMA_TOOL_PRESET (device_info)->tool_options;

  if (tool_options != entry->tool_options)
    {
      LigmaContextPropMask serialize_props;

      entry->tool_options = tool_options;
      ligma_context_set_parent (entry->context, LIGMA_CONTEXT (tool_options));

      serialize_props =
        ligma_context_get_serialize_properties (LIGMA_CONTEXT (tool_options));

      toggle_prop_visible (entry->foreground,
                           entry->foreground_none,
                           serialize_props & LIGMA_CONTEXT_PROP_MASK_FOREGROUND);

      toggle_prop_visible (entry->background,
                           entry->background_none,
                           serialize_props & LIGMA_CONTEXT_PROP_MASK_BACKGROUND);

      toggle_prop_visible (entry->brush,
                           entry->brush_none,
                           serialize_props & LIGMA_CONTEXT_PROP_MASK_BRUSH);

      toggle_prop_visible (entry->pattern,
                           entry->pattern_none,
                           serialize_props & LIGMA_CONTEXT_PROP_MASK_PATTERN);

      toggle_prop_visible (entry->gradient,
                           entry->gradient_none,
                           serialize_props & LIGMA_CONTEXT_PROP_MASK_GRADIENT);
    }

  if (! ligma_device_info_get_device (device_info, NULL) ||
      ligma_device_info_get_mode (device_info) == GDK_MODE_DISABLED)
    {
      gtk_widget_hide (entry->ebox);
    }
  else
    {
      gtk_widget_show (entry->ebox);
    }

  if (! strcmp (pspec->name, "tool-options"))
    {
      LigmaRGB color;
      guchar  r, g, b;
      gchar   buf[64];

      ligma_context_get_foreground (entry->context, &color);
      ligma_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Foreground: %d, %d, %d"), r, g, b);
      ligma_help_set_help_data (entry->foreground, buf, NULL);

      ligma_context_get_background (entry->context, &color);
      ligma_rgb_get_uchar (&color, &r, &g, &b);
      g_snprintf (buf, sizeof (buf), _("Background: %d, %d, %d"), r, g, b);
      ligma_help_set_help_data (entry->background, buf, NULL);
    }
}

static void
ligma_device_status_save_clicked (GtkWidget        *button,
                                 LigmaDeviceStatus *status)
{
  ligma_devices_save (status->ligma, TRUE);
}

static void
ligma_device_status_view_clicked (GtkWidget       *widget,
                                 GdkModifierType  state,
                                 const gchar     *identifier)
{
  LigmaDeviceStatus  *status;
  LigmaDialogFactory *dialog_factory;

  status = LIGMA_DEVICE_STATUS (gtk_widget_get_ancestor (widget,
                                                        LIGMA_TYPE_DEVICE_STATUS));
  dialog_factory = ligma_dialog_factory_get_singleton ();

  ligma_window_strategy_show_dockable_dialog (LIGMA_WINDOW_STRATEGY (ligma_get_window_strategy (status->ligma)),
                                             status->ligma,
                                             dialog_factory,
                                             ligma_widget_get_monitor (widget),
                                             identifier);
}
