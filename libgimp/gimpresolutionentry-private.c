/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpresolutionentry.c
 * Copyright (C) 2024 Jehan
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

#include "libgimpwidgets/gimpwidgets.h"

#include "gimp.h"
#include "gimpui.h"
#include "gimpresolutionentry-private.h"

#include "libgimp-intl.h"


enum
{
  PROP_0,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_PIXEL_DENSITY,
  PROP_UNIT,
  PROP_KEEP_RATIO,
  N_PROPS
};

struct _GimpResolutionEntry
{
  GtkGrid                   parent_instance;

  gint                      width;
  gint                      height;
  gdouble                   ppi;
  GimpUnit                 *unit;
  gdouble                   ratio;
  gboolean                  keep_ratio;

  GtkWidget                *phy_width_label;
  GtkWidget                *phy_height_label;

  GtkWidget                *chainbutton;
};

G_DEFINE_FINAL_TYPE (GimpResolutionEntry, gimp_resolution_entry, GTK_TYPE_GRID)

#define parent_class gimp_resolution_entry_parent_class


static void        gimp_resolution_entry_class_init      (GimpResolutionEntryClass *class);
static void        gimp_resolution_entry_init            (GimpResolutionEntry      *gre);
static void        gimp_resolution_entry_constructed     (GObject                  *object);
static void        gimp_resolution_entry_set_property    (GObject                  *object,
                                                          guint                     property_id,
                                                          const GValue             *value,
                                                          GParamSpec               *pspec);
static void        gimp_resolution_entry_get_property    (GObject                  *object,
                                                          guint                     property_id,
                                                          GValue                   *value,
                                                          GParamSpec               *pspec);

static GtkWidget * gimp_resolution_entry_attach_label    (GimpResolutionEntry      *entry,
                                                          const gchar              *text,
                                                          gint                      row,
                                                          gint                      column,
                                                          gfloat                    alignment);
static void        gimp_resolution_entry_format_label    (GimpResolutionEntry      *entry,
                                                          GtkWidget                *label,
                                                          gdouble                   size);
static void        gimp_resolution_entry_update_labels   (GimpResolutionEntry      *entry,
                                                          GParamSpec               *param_spec,
                                                          GtkWidget                *label);

static gboolean    gimp_resolution_entry_ppi_to_unit     (GBinding            *binding,
                                                          const GValue        *from_value,
                                                          GValue              *to_value,
                                                          GimpResolutionEntry *entry);
static gboolean    gimp_resolution_entry_unit_to_ppi     (GBinding            *binding,
                                                          const GValue        *from_value,
                                                          GValue              *to_value,
                                                          GimpResolutionEntry *entry);


static GParamSpec *props[N_PROPS] = { NULL, };

static void
gimp_resolution_entry_class_init (GimpResolutionEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_resolution_entry_constructed;
  object_class->set_property = gimp_resolution_entry_set_property;
  object_class->get_property = gimp_resolution_entry_get_property;

  props[PROP_WIDTH] = g_param_spec_int ("width",
                                        "Width in pixel",
                                        NULL,
                                        GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, GIMP_MIN_IMAGE_SIZE,
                                        GIMP_PARAM_READWRITE |
                                        G_PARAM_EXPLICIT_NOTIFY |
                                        G_PARAM_CONSTRUCT);
  props[PROP_HEIGHT] = g_param_spec_int ("height",
                                         "Height in pixel",
                                         NULL,
                                         GIMP_MIN_IMAGE_SIZE, GIMP_MAX_IMAGE_SIZE, GIMP_MIN_IMAGE_SIZE,
                                         GIMP_PARAM_READWRITE |
                                         G_PARAM_EXPLICIT_NOTIFY |
                                         G_PARAM_CONSTRUCT);
  props[PROP_PIXEL_DENSITY] = g_param_spec_double ("pixel-density",
                                                   "Pixel density in pixel per inch",
                                                   NULL,
                                                   GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION, 300.0,
                                                   GIMP_PARAM_READWRITE |
                                                   G_PARAM_EXPLICIT_NOTIFY |
                                                   G_PARAM_CONSTRUCT);
  props[PROP_UNIT] = gimp_param_spec_unit ("unit",
                                           "Physical unit for the pixel density",
                                           _("This unit is used to select the pixel density "
                                           "and show dimensions in physical unit"),
                                           FALSE, FALSE,
                                           gimp_unit_inch (),
                                           GIMP_PARAM_READWRITE |
                                           G_PARAM_EXPLICIT_NOTIFY |
                                           G_PARAM_CONSTRUCT);
  props[PROP_KEEP_RATIO] = g_param_spec_boolean ("keep-ratio",
                                                 _("_Keep aspect ratio"),
                                                 _("Force dimensions with aspect ratio"),
                                                 TRUE,
                                                 GIMP_PARAM_READWRITE |
                                                 G_PARAM_EXPLICIT_NOTIFY |
                                                 G_PARAM_CONSTRUCT);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_resolution_entry_init (GimpResolutionEntry *entry)
{
  entry->width      = 0;
  entry->height     = 0;
  entry->ppi        = 300.0;
  entry->unit       = gimp_unit_inch ();
  entry->keep_ratio = TRUE;

  gtk_grid_set_row_spacing (GTK_GRID (entry), 2);
  gtk_grid_set_column_spacing (GTK_GRID (entry), 4);
}

static void
gimp_resolution_entry_constructed (GObject *object)
{
  GimpResolutionEntry *entry = GIMP_RESOLUTION_ENTRY (object);
  GtkTreeModel        *model;
  GtkWidget           *label;
  GtkWidget           *widget;
  GBinding            *binding;
  GtkAdjustment       *adj;

  g_return_if_fail (entry->height != 0);

  /* Initial ratio is from the initial values. */
  entry->ratio = (gdouble) entry->width / (gdouble) entry->height;

  widget = gimp_prop_spin_button_new (object, "pixel-density", 1.0, 10.0, 2);
  adj = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget));
  binding = g_object_get_data (G_OBJECT (adj), "gimp-prop-adjustment-binding");
  g_binding_unbind (binding);
  binding = g_object_bind_property_full (object, "pixel-density",
                                         widget, "value",
                                         G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                                         (GBindingTransformFunc) gimp_resolution_entry_ppi_to_unit,
                                         (GBindingTransformFunc) gimp_resolution_entry_unit_to_ppi,
                                         entry, NULL);
  g_object_set_data (G_OBJECT (adj), "gimp-prop-adjustment-binding", binding);
  gtk_grid_attach (GTK_GRID (entry), widget, 1, 3, 1, 1);
  gtk_widget_show (widget);

  widget = gimp_prop_unit_combo_box_new (object, "unit");
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  g_object_set (model,
                "short-format", _("pixels/%a"),
                "long-format",  _("pixels/%a"),
                NULL);
  gtk_grid_attach (GTK_GRID (entry), widget, 3, 3, 1, 1);
  gtk_widget_show (widget);

  widget = gimp_prop_spin_button_new (object, "width", 1.0, 10.0, 0);
  gtk_grid_attach (GTK_GRID (entry), widget, 1, 1, 1, 1);
  gtk_widget_show (widget);

  label = g_object_new (GTK_TYPE_LABEL,
                        "xalign", 0.0,
                        "yalign", 0.5,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_resolution_entry_format_label (entry, label,
                                      entry->width / entry->ppi);
  entry->phy_width_label = label;
  gtk_grid_attach (GTK_GRID (entry), entry->phy_width_label, 3, 1, 1, 1);
  gtk_widget_show (entry->phy_width_label);

  widget = gimp_prop_spin_button_new (object, "height", 1.0, 10.0, 0);
  gtk_grid_attach (GTK_GRID (entry), widget, 1, 2, 1, 1);
  gtk_widget_show (widget);

  label = g_object_new (GTK_TYPE_LABEL,
                        "xalign", 0.0,
                        "yalign", 0.5,
                        NULL);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gimp_resolution_entry_format_label (entry, label,
                                      entry->height / entry->ppi);
  entry->phy_height_label = label;
  gtk_grid_attach (GTK_GRID (entry), entry->phy_height_label, 3, 2, 1, 1);
  gtk_widget_show (entry->phy_height_label);

  g_signal_connect (object, "notify::width",
                    G_CALLBACK (gimp_resolution_entry_update_labels),
                    entry->phy_width_label);
  g_signal_connect (object, "notify::height",
                    G_CALLBACK (gimp_resolution_entry_update_labels),
                    entry->phy_height_label);
  g_signal_connect (object, "notify::pixel-density",
                    G_CALLBACK (gimp_resolution_entry_update_labels),
                    NULL);
}

static void
gimp_resolution_entry_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpResolutionEntry *entry = GIMP_RESOLUTION_ENTRY (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      gimp_resolution_entry_set_width (entry, g_value_get_int (value));
      break;
    case PROP_HEIGHT:
      gimp_resolution_entry_set_height (entry, g_value_get_int (value));
      break;
    case PROP_PIXEL_DENSITY:
      gimp_resolution_entry_set_pixel_density (entry, g_value_get_double (value));
      break;
    case PROP_UNIT:
      gimp_resolution_entry_set_unit (entry, g_value_get_object (value));
      break;
    case PROP_KEEP_RATIO:
      gimp_resolution_entry_set_keep_ratio (entry, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_resolution_entry_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpResolutionEntry *entry = GIMP_RESOLUTION_ENTRY (object);

  switch (property_id)
    {
    case PROP_WIDTH:
      g_value_set_int (value, entry->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, entry->height);
      break;
    case PROP_PIXEL_DENSITY:
      g_value_set_double (value, entry->ppi);
      break;
    case PROP_UNIT:
      g_value_set_object (value, entry->unit);
      break;
    case PROP_KEEP_RATIO:
      g_value_set_boolean (value, entry->keep_ratio);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_prop_resolution_entry_new (GObject     *config,
                                const gchar *width_prop,
                                const gchar *height_prop,
                                const gchar *ppi_prop,
                                const gchar *unit_prop)
{
  GtkWidget  *widget;
  GParamSpec *w_pspec;
  GParamSpec *h_pspec;
  GParamSpec *d_pspec;
  GParamSpec *u_pspec;
  gint        width  = 0;
  gint        height = 0;
  gdouble     ppi    = 300.0;
  GimpUnit   *unit   = gimp_unit_inch ();

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (width_prop != NULL, NULL);
  g_return_val_if_fail (height_prop != NULL, NULL);
  g_return_val_if_fail (ppi_prop != NULL, NULL);
  g_return_val_if_fail (unit_prop != NULL, NULL);

  w_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), width_prop);
  h_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), height_prop);
  d_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), ppi_prop);
  u_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), unit_prop);

  g_return_val_if_fail (w_pspec != NULL, NULL);
  g_return_val_if_fail (h_pspec != NULL, NULL);
  g_return_val_if_fail (d_pspec != NULL, NULL);
  g_return_val_if_fail (u_pspec != NULL, NULL);

  g_object_get (config,
                width_prop,  &width,
                height_prop, &height,
                ppi_prop,    &ppi,
                unit_prop,   &unit,
                NULL);

  widget = gimp_resolution_entry_new (g_param_spec_get_nick (w_pspec), width,
                                      g_param_spec_get_nick (h_pspec), height,
                                      g_param_spec_get_nick (d_pspec), ppi,
                                      unit);

  g_object_bind_property (config, width_prop,
                          widget, "width",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property (config, height_prop,
                          widget, "height",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property (config, ppi_prop,
                          widget, "pixel-density",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
  g_object_bind_property (config, unit_prop,
                          widget, "unit",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  return widget;
}

/**
 * gimp_resolution_entry_new:
 * @width_label:       Optional label for the width control.
 * @width:             Width of the item, specified in pixels.
 * @height_label:      Optional label for the height control.
 * @height:            Height of the item, specified in pixels.
 * @res_label:         Optional label for the resolution entry.
 * @pixel_density:     The initial resolution in pixel per inch.
 * @display_unit:      The display unit.
 *
 * Creates a new #GimpResolutionEntry widget.
 *
 * The #GimpResolutionEntry is derived from #GtkGrid and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #GimpUnitMenu to allow the caller to add labels or other widgets.
 *
 * A #GimpChainButton is displayed if independent is set to %TRUE.
 *
 * Returns: A pointer to the new #GimpResolutionEntry widget.
 **/
GtkWidget *
gimp_resolution_entry_new (const gchar *width_label,
                           gint         width,
                           const gchar *height_label,
                           gint         height,
                           const gchar *res_label,
                           gdouble      pixel_density,
                           GimpUnit    *display_unit)
{
  GimpResolutionEntry *entry;

  entry = g_object_new (GIMP_TYPE_RESOLUTION_ENTRY,
                        "width",         width,
                        "height",        height,
                        "pixel-density", pixel_density,
                        "unit",          display_unit,
                        NULL);

  if (width_label)
    gimp_resolution_entry_attach_label (entry, width_label,  1, 0, 0.0);

  if (height_label)
    gimp_resolution_entry_attach_label (entry, height_label, 2, 0, 0.0);

  if (res_label)
    gimp_resolution_entry_attach_label (entry, res_label,    3, 0, 0.0);

  return GTK_WIDGET (entry);
}

static gboolean
gimp_resolution_entry_idle_notify (GimpResolutionEntry *entry)
{
  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_WIDTH]);
  g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_HEIGHT]);

  return G_SOURCE_REMOVE;
}

void
gimp_resolution_entry_set_width (GimpResolutionEntry *entry,
                                 gint                 width)
{
  if (width == 0)
    {
      /* Do nothing, i.e. revert back to value it was. Yet notify after an idle
       * source, otherwise the entry is not updated (the binding likely blocks
       * notifications for this property to avoid infinite loops.
       */
      g_idle_add ((GSourceFunc) gimp_resolution_entry_idle_notify, entry);
    }
  else if (entry->width != width)
    {
      g_object_freeze_notify (G_OBJECT (entry));

      if (entry->keep_ratio && entry->width != 0)
        {
          gint height;

          height = (gint) ((gdouble) width / entry->ratio);

          if (height != entry->height)
            {
              entry->height = height;
              g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_HEIGHT]);
            }
        }

      entry->width = width;
      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_WIDTH]);

      g_object_thaw_notify (G_OBJECT (entry));
    }
}

void
gimp_resolution_entry_set_height (GimpResolutionEntry *entry,
                                  gint                 height)
{
  if (height == 0)
    {
      /* Do nothing, i.e. revert back to value it was. Yet notify after an idle
       * source, otherwise the entry is not updated (the binding likely blocks
       * notifications for this property to avoid infinite loops.
       */
      g_idle_add ((GSourceFunc) gimp_resolution_entry_idle_notify, entry);
    }
  else if (entry->height != height)
    {
      g_object_freeze_notify (G_OBJECT (entry));

      if (entry->keep_ratio && entry->height != 0)
        {
          gint width;

          width = (gint) ((gdouble) height * entry->ratio);

          if (width != entry->width)
            {
              entry->width = width;
              g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_WIDTH]);
            }
        }

      entry->height = height;
      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_HEIGHT]);

      g_object_thaw_notify (G_OBJECT (entry));
    }
}

void
gimp_resolution_entry_set_pixel_density (GimpResolutionEntry *entry,
                                         gdouble              ppi)
{
  if (entry->ppi != ppi)
    {
      entry->ppi = ppi;
      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_PIXEL_DENSITY]);
    }
}

void
gimp_resolution_entry_set_unit (GimpResolutionEntry *entry,
                                GimpUnit            *unit)
{
  g_return_if_fail (unit != gimp_unit_pixel ());
  g_return_if_fail (unit != gimp_unit_percent ());

  if (entry->unit != unit)
    {
      entry->unit = unit;
      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_UNIT]);
      /* Even though the pixel density (in pixel per inch) has not in fact
       * changed, we send a notification to force a refresh of the displayed
       * pixel per other-unit value in the entry field.
       */
      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_PIXEL_DENSITY]);

      if (entry->phy_width_label)
        gimp_resolution_entry_format_label (entry, entry->phy_width_label,
                                            gimp_unit_get_factor (entry->unit) *
                                            entry->width / entry->ppi);
      if (entry->phy_height_label)
        gimp_resolution_entry_format_label (entry, entry->phy_height_label,
                                            gimp_unit_get_factor (entry->unit) *
                                            entry->height / entry->ppi);
    }
}

void
gimp_resolution_entry_set_keep_ratio (GimpResolutionEntry *entry,
                                      gboolean             keep_ratio)
{
  if (keep_ratio != entry->keep_ratio)
    {
      entry->keep_ratio = keep_ratio;

      if (entry->keep_ratio)
        entry->ratio = (gdouble) entry->width / (gdouble) entry->height;

      g_object_notify_by_pspec (G_OBJECT (entry), props[PROP_KEEP_RATIO]);
    }
}

gint
gimp_resolution_entry_get_width (GimpResolutionEntry *entry)
{
  return entry->width;
}

gint
gimp_resolution_entry_get_height (GimpResolutionEntry *entry)
{
  return entry->height;
}

gdouble
gimp_resolution_entry_get_density (GimpResolutionEntry *entry)
{
  return entry->ppi;
}

GimpUnit *
gimp_resolution_entry_get_unit (GimpResolutionEntry *entry)
{
  return entry->unit;
}

gboolean
gimp_resolution_entry_get_keep_ratio (GimpResolutionEntry *entry)
{
  return entry->keep_ratio;
}


/* Private functions. */

/**
 * gimp_resolution_entry_attach_label:
 * @gre:       The #GimpResolutionEntry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #GimpResolutionEntry (which is a #GtkGrid).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 **/
static GtkWidget *
gimp_resolution_entry_attach_label (GimpResolutionEntry *gre,
                                    const gchar         *text,
                                    gint                 row,
                                    gint                 column,
                                    gfloat               alignment)
{
  GtkWidget *label;

  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);
  g_return_val_if_fail (text != NULL, NULL);

  label = gtk_label_new_with_mnemonic (text);

  if (column == 0)
    {
      GList *children;
      GList *list;

      children = gtk_container_get_children (GTK_CONTAINER (gre));

      for (list = children; list; list = g_list_next (list))
        {
          GtkWidget *child = list->data;
          gint       left_attach;
          gint       top_attach;

          gtk_container_child_get (GTK_CONTAINER (gre), child,
                                   "left-attach", &left_attach,
                                   "top-attach",  &top_attach,
                                   NULL);

          if (left_attach == 1 && top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label), child);
              break;
            }
        }

      g_list_free (children);
    }

  gtk_label_set_xalign (GTK_LABEL (label), alignment);

  gtk_grid_attach (GTK_GRID (gre), label, column, row, 1, 1);
  gtk_widget_show (label);

  return label;
}

static void
gimp_resolution_entry_format_label (GimpResolutionEntry *entry,
                                    GtkWidget           *label,
                                    gdouble              size_inch)
{
  gchar *format = g_strdup_printf ("%%.%df %%s",
                                   gimp_unit_get_digits (entry->unit));
  gchar *text = g_strdup_printf (format,
                                 size_inch * gimp_unit_get_factor (entry->unit),
                                 gimp_unit_get_name (entry->unit));
  g_free (format);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
}

static void
gimp_resolution_entry_update_labels (GimpResolutionEntry *entry,
                                     GParamSpec          *param_spec,
                                     GtkWidget           *label)
{
  if ((label == NULL && entry->phy_width_label != NULL) ||
      (label != NULL && label == entry->phy_width_label))
    gimp_resolution_entry_format_label (entry, entry->phy_width_label,
                                        entry->width / entry->ppi);

  if ((label == NULL && entry->phy_height_label != NULL) ||
      (label != NULL && label == entry->phy_height_label))
    gimp_resolution_entry_format_label (entry, entry->phy_height_label,
                                        entry->height / entry->ppi);
}

static gboolean
gimp_resolution_entry_ppi_to_unit (GBinding            *binding,
                                   const GValue        *from_value,
                                   GValue              *to_value,
                                   GimpResolutionEntry *entry)
{
  gdouble ppi = g_value_get_double (from_value);

  g_value_set_double (to_value, ppi / gimp_unit_get_factor (entry->unit));

  return TRUE;
}

static gboolean
gimp_resolution_entry_unit_to_ppi (GBinding            *binding,
                                   const GValue        *from_value,
                                   GValue              *to_value,
                                   GimpResolutionEntry *entry)
{
  gdouble ppu = g_value_get_double (from_value);

  g_value_set_double (to_value, ppu * gimp_unit_get_factor (entry->unit));

  return TRUE;
}
