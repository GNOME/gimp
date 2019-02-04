/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpiconsizescale.c
 * Copyright (C) 2016 Jehan <jehan@girinstud.io>
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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"

#include "gimpiconsizescale.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_GIMP
};

typedef struct _GimpIconSizeScalePrivate GimpIconSizeScalePrivate;

struct _GimpIconSizeScalePrivate
{
  Gimp      *gimp;

  GtkWidget *scale;
  GtkWidget *combo;
};

#define GET_PRIVATE(scale) \
        ((GimpIconSizeScalePrivate *) gimp_icon_size_scale_get_instance_private ((GimpIconSizeScale *) (scale)))


static void   gimp_icon_size_scale_constructed    (GObject           *object);
static void   gimp_icon_size_scale_finalize       (GObject           *object);
static void   gimp_icon_size_scale_set_property   (GObject           *object,
                                                   guint              property_id,
                                                   const GValue      *value,
                                                   GParamSpec        *pspec);
static void   gimp_icon_size_scale_get_property   (GObject           *object,
                                                   guint              property_id,
                                                   GValue            *value,
                                                   GParamSpec        *pspec);

/* Signals on GimpGuiConfig properties. */
static void   gimp_icon_size_scale_icon_theme_notify (GimpGuiConfig   *config,
                                                      GParamSpec      *pspec,
                                                      GtkRange        *scale);
static void   gimp_icon_size_scale_icon_size_notify  (GimpGuiConfig   *config,
                                                      GParamSpec      *pspec,
                                                      GtkWidget       *size_scale);

/* Signals on the combo. */
static void   gimp_icon_size_scale_combo_changed     (GtkComboBox     *combo,
                                                      GimpGuiConfig   *config);
/* Signals on the GtkScale. */
static void   gimp_icon_size_scale_value_changed     (GtkRange        *range,
                                                      GimpGuiConfig   *config);
static gboolean gimp_icon_size_scale_change_value    (GtkRange        *range,
                                                      GtkScrollType    scroll,
                                                      gdouble          value,
                                                      GimpGuiConfig   *config);


G_DEFINE_TYPE_WITH_PRIVATE (GimpIconSizeScale, gimp_icon_size_scale,
                            GIMP_TYPE_FRAME)

#define parent_class gimp_icon_size_scale_parent_class


static void
gimp_icon_size_scale_class_init (GimpIconSizeScaleClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_icon_size_scale_constructed;
  object_class->finalize     = gimp_icon_size_scale_finalize;
  object_class->set_property = gimp_icon_size_scale_set_property;
  object_class->get_property = gimp_icon_size_scale_get_property;

  g_object_class_install_property (object_class, PROP_GIMP,
                                   g_param_spec_object ("gimp",
                                                        NULL, NULL,
                                                        GIMP_TYPE_GIMP,
                                                        GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
gimp_icon_size_scale_init (GimpIconSizeScale *object)
{
  GimpIconSizeScalePrivate *private = GET_PRIVATE (object);
  GtkWidget                *box;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_frame_set_label_widget (GTK_FRAME (object), box);
  private->combo = gimp_int_combo_box_new (_("Guess icon size from resolution"), 0,
                                           _("Use icon size from the theme"), 1,
                                           _("Custom icon size"), 2, NULL);
  gtk_box_pack_start (GTK_BOX (box), private->combo, FALSE, FALSE, 0);
  gtk_widget_show (box);

  private->scale = gtk_hscale_new_with_range (0.0, 3.0, 1.0);
  /* 'draw_value' updates round_digits. So set it first. */
  gtk_scale_set_draw_value (GTK_SCALE (private->scale), FALSE);
  gtk_range_set_round_digits (GTK_RANGE (private->scale), 0.0);
  gtk_widget_set_sensitive (GTK_WIDGET (private->scale), FALSE);
  gtk_container_add (GTK_CONTAINER (object), private->scale);
}

static void
gimp_icon_size_scale_constructed (GObject *object)
{
  GimpIconSizeScalePrivate *private = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (private->combo, "changed",
                    G_CALLBACK (gimp_icon_size_scale_combo_changed),
                    private->gimp->config);

  g_signal_connect (private->gimp->config, "notify::icon-theme",
                    G_CALLBACK (gimp_icon_size_scale_icon_theme_notify),
                    private->scale);
  gimp_icon_size_scale_icon_theme_notify (GIMP_GUI_CONFIG (private->gimp->config),
                                          NULL, GTK_RANGE (private->scale));

  g_signal_connect (private->scale, "change-value",
                    G_CALLBACK (gimp_icon_size_scale_change_value),
                    private->gimp->config);
  g_signal_connect (private->scale, "value-changed",
                    G_CALLBACK (gimp_icon_size_scale_value_changed),
                    private->gimp->config);

  g_signal_connect (private->gimp->config, "notify::icon-size",
                    G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                    private->scale);
  gimp_icon_size_scale_icon_size_notify (GIMP_GUI_CONFIG (private->gimp->config),
                                         NULL, private->scale);

  gtk_widget_show (private->combo);
  gtk_widget_show (private->scale);
}

static void
gimp_icon_size_scale_finalize (GObject *object)
{
  GimpIconSizeScalePrivate *private = GET_PRIVATE (object);

  g_signal_handlers_disconnect_by_func (private->gimp->config,
                                        G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                                        private->scale);
  g_signal_handlers_disconnect_by_func (private->gimp->config,
                                        G_CALLBACK (gimp_icon_size_scale_icon_theme_notify),
                                        private->scale);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_icon_size_scale_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpIconSizeScalePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      private->gimp = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_icon_size_scale_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpIconSizeScalePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_GIMP:
      g_value_set_object (value, private->gimp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_icon_size_scale_icon_theme_notify (GimpGuiConfig *config,
                                        GParamSpec    *pspec,
                                        GtkRange      *scale)
{
  GtkIconTheme *theme             = gtk_icon_theme_get_default();
  gint         *icon_sizes;
  gchar        *markup;
  gdouble       value             = gtk_range_get_value (scale);
  gboolean      update_value      = FALSE;
  gboolean      has_small_toolbar = FALSE;
  gboolean      has_large_toolbar = FALSE;
  gboolean      has_dnd           = FALSE;
  gboolean      has_dialog        = FALSE;
  gint          i;

  icon_sizes = gtk_icon_theme_get_icon_sizes (theme, "gimp-tool-move");
  for (i = 0; icon_sizes[i]; i++)
    {
      if (icon_sizes[i] == -1)
        {
          has_small_toolbar = TRUE;
          has_large_toolbar = TRUE;
          has_dnd           = TRUE;
          has_dialog        = TRUE;
          break;
        }
      else if (icon_sizes[i] > 13 && icon_sizes[i] < 19)
        {
          has_small_toolbar = TRUE;
        }
      else if (icon_sizes[i] > 21 && icon_sizes[i] < 27)
        {
          has_large_toolbar = TRUE;
        }
      else if (icon_sizes[i] > 29 && icon_sizes[i] < 35)
        {
          has_dnd = TRUE;
        }
      else if (icon_sizes[i] > 45 && icon_sizes[i] < 51)
        {
          has_dialog = TRUE;
        }
    }
  g_free (icon_sizes);

  gtk_scale_clear_marks (GTK_SCALE (scale));
  markup = (gchar *) C_("icon-size", "Small");
  if (! has_small_toolbar)
    {
      markup = g_strdup_printf ("<span strikethrough=\"true\">%s</span>",
                                markup);
      if (value == 0.0)
        update_value = TRUE;
    }
  gtk_scale_add_mark (GTK_SCALE (scale), 0.0, GTK_POS_BOTTOM,
                      markup);
  if (! has_small_toolbar)
    g_free (markup);

  markup = (gchar *) C_("icon-size", "Medium");
  if (! has_large_toolbar)
    {
      markup = g_strdup_printf ("<span strikethrough=\"true\">%s</span>",
                                markup);
      if (value == 1.0)
        update_value = TRUE;
    }
  gtk_scale_add_mark (GTK_SCALE (scale), 1.0, GTK_POS_BOTTOM,
                      markup);
  if (! has_large_toolbar)
    g_free (markup);

  markup = (gchar *) C_("icon-size", "Large");
  if (! has_dnd)
    {
      markup = g_strdup_printf ("<span strikethrough=\"true\">%s</span>",
                                markup);
      if (value == 2.0)
        update_value = TRUE;
    }
  gtk_scale_add_mark (GTK_SCALE (scale), 2.0, GTK_POS_BOTTOM,
                      markup);
  if (! has_dnd)
    g_free (markup);

  markup = (gchar *) C_("icon-size", "Huge");
  if (! has_dialog)
    {
      markup = g_strdup_printf ("<span strikethrough=\"true\">%s</span>",
                                markup);
      if (value == 3.0)
        update_value = TRUE;
    }
  gtk_scale_add_mark (GTK_SCALE (scale), 3.0, GTK_POS_BOTTOM,
                      markup);
  if (! has_dialog)
    g_free (markup);

  if (update_value)
    {
      GimpIconSize size;

      g_object_get (config, "icon-size", &size, NULL);

      if (size == GIMP_ICON_SIZE_THEME || size == GIMP_ICON_SIZE_AUTO)
        {
          g_signal_handlers_block_by_func (scale,
                                           G_CALLBACK (gimp_icon_size_scale_value_changed),
                                           config);
        }
      if (has_small_toolbar)
        gtk_range_set_value (scale, 0.0);
      else if (has_large_toolbar)
        gtk_range_set_value (scale, 1.0);
      else if (has_dnd)
        gtk_range_set_value (scale, 2.0);
      else
        gtk_range_set_value (scale, 3.0);
      if (size == GIMP_ICON_SIZE_THEME || size == GIMP_ICON_SIZE_AUTO)
        {
          g_signal_handlers_unblock_by_func (scale,
                                             G_CALLBACK (gimp_icon_size_scale_value_changed),
                                             config);
        }
    }
}

static void
gimp_icon_size_scale_icon_size_notify (GimpGuiConfig *config,
                                       GParamSpec    *pspec,
                                       GtkWidget     *size_scale)
{
  GimpIconSizeScalePrivate *private;
  GtkWidget                *frame = gtk_widget_get_parent (size_scale);
  GtkWidget                *combo;
  GimpIconSize              size;
  gdouble                   value = 1.0;

  private = GET_PRIVATE (frame);
  combo   = private->combo;

  g_object_get (config, "icon-size", &size, NULL);

  switch (size)
    {
    case GIMP_ICON_SIZE_SMALL:
      value = 0.0;
      break;
    case GIMP_ICON_SIZE_MEDIUM:
      value = 1.0;
      break;
    case GIMP_ICON_SIZE_LARGE:
      value = 2.0;
      break;
    case GIMP_ICON_SIZE_HUGE:
      value = 3.0;
      break;
    default: /* GIMP_ICON_SIZE_THEME */
      break;
    }
  g_signal_handlers_block_by_func (combo,
                                   G_CALLBACK (gimp_icon_size_scale_combo_changed),
                                   config);
  gimp_int_combo_box_set_active (GIMP_INT_COMBO_BOX (combo),
                                 (size == GIMP_ICON_SIZE_AUTO)? 0 :
                                 (size == GIMP_ICON_SIZE_THEME)? 1 : 2);
  g_signal_handlers_unblock_by_func (combo,
                                     G_CALLBACK (gimp_icon_size_scale_combo_changed),
                                     config);
  gtk_widget_set_sensitive (GTK_WIDGET (size_scale),
                            size != GIMP_ICON_SIZE_THEME &&
                            size != GIMP_ICON_SIZE_AUTO);


  if (size != GIMP_ICON_SIZE_THEME && size != GIMP_ICON_SIZE_AUTO)
    {
      g_signal_handlers_block_by_func (size_scale,
                                       G_CALLBACK (gimp_icon_size_scale_value_changed),
                                       config);
      gtk_range_set_value (GTK_RANGE (size_scale), value);
      g_signal_handlers_unblock_by_func (size_scale,
                                         G_CALLBACK (gimp_icon_size_scale_value_changed),
                                         config);
    }
}

static void
gimp_icon_size_scale_combo_changed (GtkComboBox   *combo,
                                    GimpGuiConfig *config)
{
  GtkWidget    *frame;
  GtkWidget    *scale;
  GimpIconSize  size;

  frame = gtk_widget_get_parent (gtk_widget_get_parent (GTK_WIDGET (combo)));
  scale = gtk_bin_get_child (GTK_BIN (frame));

  if (gtk_combo_box_get_active (combo) == 2)
    {
      gdouble value = gtk_range_get_value (GTK_RANGE (scale));

      if (value < 0.5)
        size = GIMP_ICON_SIZE_SMALL;
      else if (value < 1.5)
        size = GIMP_ICON_SIZE_MEDIUM;
      else if (value < 2.5)
        size = GIMP_ICON_SIZE_LARGE;
      else
        size = GIMP_ICON_SIZE_HUGE;
    }
  else
    {
      size = (gtk_combo_box_get_active (combo) == 0) ?
        GIMP_ICON_SIZE_AUTO : GIMP_ICON_SIZE_THEME;
    }
  gtk_widget_set_sensitive (GTK_WIDGET (scale),
                            gtk_combo_box_get_active (combo) == 2);

  g_signal_handlers_block_by_func (config,
                                   G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                                   scale);
  g_object_set (G_OBJECT (config),
                "icon-size", size,
                NULL);
  g_signal_handlers_unblock_by_func (config,
                                     G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                                     scale);
}

static void
gimp_icon_size_scale_value_changed (GtkRange      *range,
                                    GimpGuiConfig *config)
{
  GimpIconSize size;
  gdouble      value = gtk_range_get_value (range);

  if (value < 0.5)
    size = GIMP_ICON_SIZE_SMALL;
  else if (value < 1.5)
    size = GIMP_ICON_SIZE_MEDIUM;
  else if (value < 2.5)
    size = GIMP_ICON_SIZE_LARGE;
  else
    size = GIMP_ICON_SIZE_HUGE;

  g_signal_handlers_block_by_func (config,
                                   G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                                   range);
  g_object_set (G_OBJECT (config), "icon-size", size, NULL);
  g_signal_handlers_unblock_by_func (config,
                                     G_CALLBACK (gimp_icon_size_scale_icon_size_notify),
                                     range);
}

static gboolean
gimp_icon_size_scale_change_value (GtkRange      *range,
                                   GtkScrollType  scroll,
                                   gdouble        value,
                                   GimpGuiConfig *config)
{
  GtkIconTheme *theme             = gtk_icon_theme_get_default();
  gint         *icon_sizes;
  gboolean      has_small_toolbar = FALSE;
  gboolean      has_large_toolbar = FALSE;
  gboolean      has_dnd           = FALSE;
  gboolean      has_dialog        = FALSE;
  gint          i;

  /* We cannot check all icons. Use "gimp-tool-move" as template of
   * available sizes. */
  icon_sizes = gtk_icon_theme_get_icon_sizes (theme, "gimp-tool-move");
  for (i = 0; icon_sizes[i]; i++)
    {
      if (icon_sizes[i] == -1)
        {
          has_small_toolbar = TRUE;
          has_large_toolbar = TRUE;
          has_dnd           = TRUE;
          has_dialog        = TRUE;
          break;
        }
      else if (icon_sizes[i] > 13 && icon_sizes[i] < 19)
        has_small_toolbar = TRUE;
      else if (icon_sizes[i] > 21 && icon_sizes[i] < 27)
        has_large_toolbar = TRUE;
      else if (icon_sizes[i] > 29 && icon_sizes[i] < 35)
        has_dnd = TRUE;
      else if (icon_sizes[i] > 45 && icon_sizes[i] < 51)
        has_dialog = TRUE;
    }
  g_free (icon_sizes);

  if ((value < 0.5 && ! has_small_toolbar)                 ||
      (value >= 0.5 && value < 1.5 && ! has_large_toolbar) ||
      (value >= 1.5 && value < 2.5 && ! has_dnd)           ||
      (value >= 2.5 && ! has_dialog))
    /* Refuse the update. */
    return TRUE;
  else
    /* Accept the update. */
    return FALSE;
}

GtkWidget *
gimp_icon_size_scale_new (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return g_object_new (GIMP_TYPE_ICON_SIZE_SCALE,
                       "gimp", gimp,
                       NULL);
}
