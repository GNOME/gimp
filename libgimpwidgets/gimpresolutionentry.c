/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2005 Peter Mattis and Spencer Kimball
 *
 * gimpresolutionentry.c
 * Copyright (C) 1999-2005 Sven Neumann <sven@gimp.org>
 *                         Michael Natterer <mitch@gimp.org>
 *                         Nathan Summers <rock@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gimpwidgets.h"

#include "gimpresolutionentry.h"

#include "libgimp/libgimp-intl.h"


#define SIZE_MAX_VALUE 500000.0

#define GIMP_RESOLUTION_ENTRY_DIGITS(unit) \
  (MIN (gimp_unit_get_digits (unit), 5) + 1)


enum
{
  WIDTH_CHANGED,
  HEIGHT_CHANGED,
  X_CHANGED,
  Y_CHANGED,
  UNIT_CHANGED,
  LAST_SIGNAL
};

static void   gimp_resolution_entry_class_init      (GimpResolutionEntryClass *class);
static void   gimp_resolution_entry_init            (GimpResolutionEntry      *gre);

static void   gimp_resolution_entry_finalize        (GObject            *object);

static void   gimp_resolution_entry_update_value    (GimpResolutionEntryField *gref,
                                                     gdouble              value);
static void   gimp_resolution_entry_value_callback  (GtkWidget           *widget,
                                                     gpointer             data);
static void   gimp_resolution_entry_update_unit     (GimpResolutionEntry *gre,
                                                     GimpUnit             unit);
static void   gimp_resolution_entry_unit_callback   (GtkWidget           *widget,
                                                     GimpResolutionEntry *gre);

static void   gimp_resolution_entry_field_init (GimpResolutionEntry      *gre,
                                                GimpResolutionEntryField *gref,
                                                GimpResolutionEntryField *corresponding,
                                                guint                     changed_signal,
                                                gdouble                   initial_val,
                                                GimpUnit                  initial_unit,
                                                gboolean                  size,
                                                gint                      spinbutton_width);


static void gimp_resolution_entry_field_set_boundaries
                                           (GimpResolutionEntryField *gref,
                                            gdouble                   lower,
                                            gdouble                   upper);

static void gimp_resolution_entry_field_set_value
                                           (GimpResolutionEntryField *gref,
                                            gdouble                   value);

static void  gimp_resolution_entry_format_label (GimpResolutionEntry *gre,
                                                 GtkWidget           *label,
                                                 gdouble              size);
static guint gimp_resolution_entry_signals[LAST_SIGNAL] = { 0 };

static GtkTableClass *parent_class = NULL;


GType
gimp_resolution_entry_get_type (void)
{
  static GType gre_type = 0;

  if (! gre_type)
    {
      const GTypeInfo gre_info =
      {
        sizeof (GimpResolutionEntryClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) gimp_resolution_entry_class_init,
        NULL,                /* class_finalize */
        NULL,                /* class_data     */
        sizeof (GimpResolutionEntry),
        0,              /* n_preallocs    */
        (GInstanceInitFunc) gimp_resolution_entry_init,
      };

      gre_type = g_type_register_static (GTK_TYPE_TABLE,
                                         "GimpResolutionEntry",
                                         &gre_info, 0);
    }

  return gre_type;
}

static void
gimp_resolution_entry_class_init (GimpResolutionEntryClass *klass)
{
  GObjectClass *object_class;

  object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gimp_resolution_entry_signals[HEIGHT_CHANGED] =
    g_signal_new ("height-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[WIDTH_CHANGED] =
    g_signal_new ("width-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[X_CHANGED] =
    g_signal_new ("x-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, value_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[Y_CHANGED] =
    g_signal_new ("y-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, refval_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  gimp_resolution_entry_signals[UNIT_CHANGED] =
    g_signal_new ("unit-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpResolutionEntryClass, unit_changed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize = gimp_resolution_entry_finalize;

  klass->value_changed   = NULL;
  klass->refval_changed  = NULL;
  klass->unit_changed    = NULL;
}

static void
gimp_resolution_entry_init (GimpResolutionEntry *gre)
{
  gre->unitmenu     = NULL;
  gre->unit         = GIMP_UNIT_INCH;
  gre->independent  = FALSE;

  gtk_table_set_col_spacings (GTK_TABLE (gre), 4);
  gtk_table_set_row_spacings (GTK_TABLE (gre), 2);
}

static void
gimp_resolution_entry_finalize (GObject *object)
{
  GimpResolutionEntry *gre;

  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (object));

  gre = GIMP_RESOLUTION_ENTRY (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_resolution_entry_field_init (GimpResolutionEntry      *gre,
                                  GimpResolutionEntryField *gref,
                                  GimpResolutionEntryField *corresponding,
                                  guint                     changed_signal,
                                  gdouble                   initial_val,
                                  GimpUnit                  initial_unit,
                                  gboolean                  size,
                                  gint                      spinbutton_width)
{
  gint digits;

  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gref->gre               = gre;
  gref->corresponding     = corresponding;
  gref->changed_signal    = gimp_resolution_entry_signals[changed_signal];

  if (size)
    {
      gref->value         = initial_val /
                            gimp_unit_get_factor (initial_unit) *
                            corresponding->value *
                            gimp_unit_get_factor (gre->unit);

      gref->phy_size      = initial_val /
                            gimp_unit_get_factor (initial_unit);
    }
  else
    gref->value           = initial_val;

  gref->min_value         = GIMP_MIN_RESOLUTION;
  gref->max_value         = GIMP_MAX_RESOLUTION;
  gref->adjustment        = NULL;

  gref->stop_recursion    = 0;

  gref->size              = size;

  if (size)
    {
      gref->label = g_object_new (GTK_TYPE_LABEL,
                                  "xalign", 0.0,
                                  "yalign", 0.5,
                                  NULL);
      gimp_label_set_attributes (GTK_LABEL (gref->label),
                                 PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                                 -1);

      gimp_resolution_entry_format_label (gre, gref->label, gref->phy_size);
    }

  digits = size ? 0 : GIMP_RESOLUTION_ENTRY_DIGITS (initial_unit);

  gref->spinbutton = gimp_spin_button_new (&gref->adjustment,
                                            gref->value,
                                            gref->min_value,
                                            gref->max_value,
                                            1.0, 10.0, 0.0,
                                            1.0,
                                            digits);


  if (spinbutton_width > 0)
    {
      if (spinbutton_width < 17)
        gtk_entry_set_width_chars (GTK_ENTRY (gref->spinbutton),
                                   spinbutton_width);
      else
        gtk_widget_set_size_request (gref->spinbutton,
                                     spinbutton_width, -1);
    }
}

/**
 * gimp_resolution_entry_new:
 * @width_label:       Optional label for the width control.
 * @width:             Width of the item, specified in terms of @size_unit.
 * @height_label:      Optional label for the height control.
 * @height:            Height of the item, specified in terms of @size_unit.
 * @size_unit:         Unit used to specify the width and height.
 * @x_label:           Optional label for the X resolution entry.
 * @initial_x:         The initial X resolution.
 * @y_label:           Optional label for the Y resolution entry.  Ignored if
 *                     @independent is %FALSE.
 * @initial_y:         The initial Y resolution.  Ignored if @independent is
 *                     %FALSE.
 * @initial_unit:      The initial unit.
 * @independent:       Whether the X and Y resolutions can be different values.
 * @spinbutton_width:  The minimal horizontal size of the #GtkSpinButton s.
 *
 * Creates a new #GimpResolutionEntry widget.
 *
 * The #GimpResolutionEntry is derived from #GtkTable and will have
 * an empty border of one cell width on each side plus an empty column left
 * of the #GimpUnitMenu to allow the caller to add labels or other widgets.
 *
 * A #GimpChainButton is displayed if independent is set to %TRUE.
 *
 * Returns: A pointer to the new #GimpResolutionEntry widget.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
gimp_resolution_entry_new (const gchar *width_label,
                           gdouble      width,
                           const gchar *height_label,
                           gdouble      height,
                           GimpUnit     size_unit,
                           const gchar *x_label,
                           gdouble      initial_x,
                           const gchar *y_label,
                           gdouble      initial_y,
                           GimpUnit     initial_unit,
                           gboolean     independent,
                           gint         spinbutton_width)
{
  GimpResolutionEntry *gre;

  gre = g_object_new (GIMP_TYPE_RESOLUTION_ENTRY, NULL);

  gre->unit        = initial_unit;
  gre->independent = independent;

  gtk_table_resize (GTK_TABLE (gre),
                    independent ? 5 : 4,
                    4);

  gimp_resolution_entry_field_init (gre, &gre->x,
                                    &gre->width,
                                    X_CHANGED,
                                    initial_x, initial_unit,
                                    FALSE,
                                    spinbutton_width);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->x.spinbutton,
                             1, 2,
                             3, 4);

  g_signal_connect (gre->x.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->x);

  gtk_widget_show (gre->x.spinbutton);

  if (independent)
    {
      gre->chainbutton = gimp_chain_button_new (GIMP_CHAIN_RIGHT);

      gtk_table_attach (GTK_TABLE (gre), gre->chainbutton,
                        2, 3,
                        3, 5,
                        GTK_SHRINK, GTK_SHRINK | GTK_FILL,
                        0, 0);

      gtk_widget_show (gre->chainbutton);

      gimp_resolution_entry_field_init (gre, &gre->y,
                                        &gre->height,
                                        Y_CHANGED,
                                        initial_y, initial_unit,
                                        FALSE,
                                        spinbutton_width);

      gtk_table_attach_defaults (GTK_TABLE (gre), gre->y.spinbutton,
                                 1, 2,
                                 4, 5);

      g_signal_connect (gre->y.adjustment, "value-changed",
                        G_CALLBACK (gimp_resolution_entry_value_callback),
                        &gre->y);

      gtk_widget_show (gre->y.spinbutton);
    }

  gre->unitmenu = gimp_unit_menu_new (_("pixels/%s"), initial_unit,
                                      FALSE, FALSE,
                                      TRUE);
  gtk_table_attach (GTK_TABLE (gre), gre->unitmenu,
                    3, 4,
                    independent ? 4 : 3, independent ? 5 : 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  g_signal_connect (gre->unitmenu, "unit-changed",
                    G_CALLBACK (gimp_resolution_entry_unit_callback),
                    gre);
  gtk_widget_show (gre->unitmenu);

  gimp_resolution_entry_field_init (gre, &gre->width,
                                    &gre->x,
                                    WIDTH_CHANGED,
                                    width, size_unit,
                                    TRUE,
                                    spinbutton_width);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->width.spinbutton,
                             1, 2,
                             1, 2);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->width.label,
                             3, 4,
                             1, 2);

  g_signal_connect (gre->width.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->width);

  gtk_widget_show (gre->width.spinbutton);
  gtk_widget_show (gre->width.label);

  gimp_resolution_entry_field_init (gre, &gre->height,
                                    independent ? &gre->y : &gre->x,
                                    HEIGHT_CHANGED,
                                    height, size_unit,
                                    TRUE,
                                    spinbutton_width);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->height.spinbutton,
                             1, 2,
                             2, 3);

  gtk_table_attach_defaults (GTK_TABLE (gre), gre->height.label,
                             3, 4,
                             2, 3);

  g_signal_connect (gre->height.adjustment, "value-changed",
                    G_CALLBACK (gimp_resolution_entry_value_callback),
                    &gre->height);

  gtk_widget_show (gre->height.spinbutton);
  gtk_widget_show (gre->height.label);

  if (width_label)
    gimp_resolution_entry_attach_label (gre, width_label,  1, 0, 0.0);

  if (height_label)
    gimp_resolution_entry_attach_label (gre, height_label, 2, 0, 0.0);

  if (x_label)
    gimp_resolution_entry_attach_label (gre, x_label,      3, 0, 0.0);

  if (independent && y_label)
    gimp_resolution_entry_attach_label (gre, y_label,      4, 0, 0.0);

  return GTK_WIDGET (gre);
}

/**
 * gimp_resolution_entry_attach_label:
 * @gre:       The #GimpResolutionEntry you want to add a label to.
 * @text:      The text of the label.
 * @row:       The row where the label will be attached.
 * @column:    The column where the label will be attached.
 * @alignment: The horizontal alignment of the label.
 *
 * Attaches a #GtkLabel to the #GimpResolutionEntry (which is a #GtkTable).
 *
 * Returns: A pointer to the new #GtkLabel widget.
 *
 * Since: GIMP 2.4
 **/
GtkWidget *
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
      GtkTableChild *child;
      GList         *list;

      for (list = GTK_TABLE (gre)->children; list; list = g_list_next (list))
        {
          child = list->data;

          if (child->left_attach == 1 && child->top_attach == row)
            {
              gtk_label_set_mnemonic_widget (GTK_LABEL (label),
                                             child->widget);
              break;
            }
        }
    }

  gtk_misc_set_alignment (GTK_MISC (label), alignment, 0.5);

  gtk_table_attach (GTK_TABLE (gre), label, column, column+1, row, row+1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  return label;
}

/**
 * gimp_resolution_entry_set_width_boundaries:
 * @gre:   The #GimpResolutionEntry you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the field in pixels.
 * @upper: The new upper boundary of the value of the field in pixels.
 *
 * Limits the range of possible values which can be entered in the width field
 * of the #GimpResolutionEntry.
 *
 * The current value of the field will be clamped to fit in its
 * new boundaries.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_width_boundaries (GimpResolutionEntry *gre,
                                            gdouble              lower,
                                            gdouble              upper)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_boundaries (&gre->width,
                                              lower,
                                              upper);
}

/**
 * gimp_resolution_entry_set_height_boundaries:
 * @gre:   The #GimpResolutionEntry you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the field in pixels.
 * @upper: The new upper boundary of the value of the field in pixels.
 *
 * Limits the range of possible values which can be entered in the height field
 * of the #GimpResolutionEntry.
 *
 * The current value of the field will be clamped to fit in its
 * new boundaries.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_height_boundaries (GimpResolutionEntry *gre,
                                             gdouble              lower,
                                             gdouble              upper)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_boundaries (&gre->height,
                                              lower,
                                              upper);
}

/**
 * gimp_resolution_entry_set_x_boundaries:
 * @gre:   The #GimpResolutionEntry you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the field, in the current unit.
 * @upper: The new upper boundary of the value of the field, in the current unit.
 *
 * Limits the range of possible values which can be entered in the x field
 * of the #GimpResolutionEntry.
 *
 * The current value of the field will be clamped to fit in its
 * new boundaries.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_x_boundaries (GimpResolutionEntry *gre,
                                        gdouble              lower,
                                        gdouble              upper)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_boundaries (&gre->x,
                                              lower,
                                              upper);
}

/**
 * gimp_resolution_entry_set_y_boundaries:
 * @gre:   The #GimpResolutionEntry you want to set value boundaries for.
 * @lower: The new lower boundary of the value of the field, in the current unit.
 * @upper: The new upper boundary of the value of the field, in the current unit.
 *
 * Limits the range of possible values which can be entered in the y field
 * of the #GimpResolutionEntry.
 *
 * The current value of the field will be clamped to fit in its
 * new boundaries.
 *
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_y_boundaries (GimpResolutionEntry *gre,
                                        gdouble              lower,
                                        gdouble              upper)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));
  g_return_if_fail (gre->independent);

  gimp_resolution_entry_field_set_boundaries (&gre->y,
                                              lower,
                                              upper);
}

static void
gimp_resolution_entry_field_set_boundaries (GimpResolutionEntryField *gref,
                                            gdouble                   lower,
                                            gdouble                   upper)
{
  g_return_if_fail (lower <= upper);

  gref->min_value        = lower;
  gref->max_value        = upper;

  GTK_ADJUSTMENT (gref->adjustment)->lower = gref->min_value;
  GTK_ADJUSTMENT (gref->adjustment)->upper = gref->max_value;

  if (gref->value > upper || gref->value < lower)
    gimp_resolution_entry_field_set_value (gref, gref->value);
}

/**
 * gimp_resolution_entry_get_width;
 * @gre:   The #GimpResolutionEntry you want to know the width of.
 *
 * Returns the width of the #GimpResolutionEntry in pixels.
 *
 * SeeAlso: gimp_resolution_get_x
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_width (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->width.value;
}

/**
 * gimp_resolution_entry_get_height;
 * @gre:   The #GimpResolutionEntry you want to know the height of.
 *
 * Returns the height of the #GimpResolutionEntry in pixels.
 *
 * SeeAlso: gimp_resolution_get_y
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_height (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->height.value;
}

/**
 * gimp_resolution_entry_get_x;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the X resolution of the #GimpResolutionEntry in the current unit.
 *
 * SeeAlso: gimp_resolution_get_x_in_dpi
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_x (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->x.value;
}

/**
 * gimp_resolution_entry_get_x_in_dpi;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the X resolution of the #GimpResolutionEntry in pixels per inch.
 *
 * SeeAlso: gimp_resolution_get_x
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_x_in_dpi (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->x.value / gimp_unit_get_factor (gre->unit);
}

/**
 * gimp_resolution_entry_get_y;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the Y resolution of the #GimpResolutionEntry in the current unit.
 *
 * SeeAlso: gimp_resolution_get_y_in_dpi
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_y (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->y.value;
}

/**
 * gimp_resolution_entry_get_y_in_dpi;
 * @gre:   The #GimpResolutionEntry you want to know the resolution of.
 *
 * Returns the Y resolution of the #GimpResolutionEntry in pixels per inch.
 *
 * SeeAlso: gimp_resolution_get_y
 *
 * Since: GIMP 2.4
 **/

gdouble
gimp_resolution_entry_get_y_in_dpi (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), 0);

  return gre->y.value / gimp_unit_get_factor (gre->unit);
}


static void
gimp_resolution_entry_update_value (GimpResolutionEntryField *gref,
                                    gdouble                   value)
{
  if (gref->stop_recursion > 0)
    return;

  gref->value = value;

  gref->stop_recursion++;

  if (gref->gre->independent &&
      !gref->size &&
      gimp_chain_button_get_active (GIMP_CHAIN_BUTTON (gref->gre->chainbutton)))
    {
      gimp_resolution_entry_update_value (&gref->gre->x, value);
      gimp_resolution_entry_update_value (&gref->gre->y, value);
    }

  if (gref->size)
    gimp_resolution_entry_update_value (gref->corresponding,
                                        gref->value /
                                          gref->phy_size /
                                          gimp_unit_get_factor (gref->gre->unit));
  else
    {
      gdouble factor = gimp_unit_get_factor (gref->gre->unit);

      if (gref->gre->independent)
        gimp_resolution_entry_update_value (gref->corresponding,
                                            gref->value *
                                              gref->corresponding->phy_size *
                                              factor);
      else
        {
          gimp_resolution_entry_update_value (&gref->gre->width,
                                              gref->value *
                                                gref->gre->width.phy_size *
                                                factor);

          gimp_resolution_entry_update_value (&gref->gre->height,
                                                gref->value *
                                                gref->gre->height.phy_size *
                                                factor);
        }
    }

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gref->adjustment), value);

  gref->stop_recursion--;

  g_signal_emit (gref->gre, gref->changed_signal, 0);
}

/**
 * gimp_resolution_entry_set_width:
 * @gre:   The #GimpResolutionEntry you want to set a value for.
 * @value: The new value for the width entry.
 *
 * Sets the value for the width control of the #GimpResolutionEntry.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_width (GimpResolutionEntry *gre,
                                 gdouble              value)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_value (&gre->width,
                                         value);
}

/**
 * gimp_resolution_entry_set_height:
 * @gre:   The #GimpResolutionEntry you want to set a value for.
 * @value: The new value for the height entry.
 *
 * Sets the value for the height control of the #GimpResolutionEntry.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_height_value (GimpResolutionEntry *gre,
                                        gdouble              value)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_value (&gre->height,
                                         value);
}

/**
 * gimp_resolution_entry_set_x:
 * @gre:   The #GimpResolutionEntry you want to set a value for.
 * @value: The new value for the X resolution in the current unit.
 *
 * Sets the value for the x resolution of the #GimpResolutionEntry.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_x (GimpResolutionEntry *gre,
                             gdouble              value)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_value (&gre->x,
                                         value);
}
/**
 * gimp_resolution_entry_set_y:
 * @gre:   The #GimpResolutionEntry you want to set a value for.
 * @value: The new value for the Y resolution in the current unit.
 *
 * Sets the value for the y resolution of the #GimpResolutionEntry.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_y (GimpResolutionEntry *gre,
                             gdouble              value)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gimp_resolution_entry_field_set_value (&gre->y,
                                         value);
}

static void
gimp_resolution_entry_field_set_value (GimpResolutionEntryField *gref,
                                       gdouble                   value)
{
  value = CLAMP (value, gref->min_value, gref->max_value);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gref->adjustment), value);
  gimp_resolution_entry_update_value (gref, value);
}

static void
gimp_resolution_entry_value_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpResolutionEntryField *gref;
  gdouble             new_value;

  gref = (GimpResolutionEntryField *) data;

  new_value = GTK_ADJUSTMENT (widget)->value;

  if (gref->value != new_value)
    gimp_resolution_entry_update_value (gref, new_value);
}

/**
 * gimp_resolution_entry_get_unit:
 * @gre: The #GimpResolutionEntry you want to know the unit of.
 *
 * Returns the #GimpUnit the user has selected in the #GimpResolutionEntry's
 * #GimpUnitMenu.
 *
 * Returns: The #GimpResolutionEntry's unit.
 *
 * Since: GIMP 2.4
 **/
GimpUnit
gimp_resolution_entry_get_unit (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), GIMP_UNIT_INCH);

  return gre->unit;
}

static void
gimp_resolution_entry_update_unit (GimpResolutionEntry *gre,
                                   GimpUnit             unit)
{
  GimpUnit  old_unit;
  gint      digits;
  gdouble   factor;

  old_unit  = gre->unit;
  gre->unit = unit;

  digits = (gimp_unit_get_digits (GIMP_UNIT_INCH) -
            gimp_unit_get_digits (unit));

  gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gre->x.spinbutton),
                              MAX (3 + digits, 3));


  factor = gimp_unit_get_factor (old_unit) / gimp_unit_get_factor (unit);

  gre->x.min_value *= factor;
  gre->x.max_value *= factor;
  gre->x.value     *= factor;

  gtk_adjustment_set_value (GTK_ADJUSTMENT (gre->x.adjustment),
                            gre->x.value);


  if (gre->independent)
    {
      gtk_spin_button_set_digits (GTK_SPIN_BUTTON (gre->y.spinbutton),
                                  MAX (3 + digits, 3));

      gre->y.min_value *= factor;
      gre->y.max_value *= factor;
      gre->y.value     *= factor;

      gtk_adjustment_set_value (GTK_ADJUSTMENT (gre->y.adjustment),
                                gre->y.value);
    }


  gimp_resolution_entry_format_label (gre,
                                      gre->width.label, gre->width.phy_size);
  gimp_resolution_entry_format_label (gre,
                                      gre->height.label, gre->height.phy_size);

  g_signal_emit (gre, gimp_resolution_entry_signals[UNIT_CHANGED], 0);
}

/**
 * gimp_resolution_entry_set_unit:
 * @gre:  The #GimpResolutionEntry you want to change the unit for.
 * @unit: The new unit.
 *
 * Sets the #GimpResolutionEntry's unit. The resolution will
 * stay the same but the value in pixels per unit will change
 * accordingly.
 *
 * Since: Gimp 2.4
 **/
void
gimp_resolution_entry_set_unit (GimpResolutionEntry *gre,
                                GimpUnit             unit)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));
  g_return_if_fail (unit != GIMP_UNIT_PIXEL);
  g_return_if_fail (unit != GIMP_UNIT_PERCENT);

  gimp_unit_menu_set_unit (GIMP_UNIT_MENU (gre->unitmenu), unit);
  gimp_resolution_entry_update_unit (gre, unit);
}

static void
gimp_resolution_entry_unit_callback (GtkWidget           *widget,
                                     GimpResolutionEntry *gre)
{
  GimpUnit new_unit;

  new_unit = gimp_unit_menu_get_unit (GIMP_UNIT_MENU (widget));

  if (gre->unit != new_unit)
    gimp_resolution_entry_update_unit (gre, new_unit);
}

/**
 * gimp_resolution_entry_show_unit_menu:
 * @gre: a #GimpResolutionEntry
 * @show: Boolean
 *
 * Controls whether a unit menu is shown in the size entry.  If
 * @show is #TRUE, the menu is shown; otherwise it is hidden.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_show_unit_menu (GimpResolutionEntry *gre,
                                      gboolean             show)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  if (show)
    gtk_widget_show (gre->unitmenu);
  else
    gtk_widget_hide (gre->unitmenu);
}


/**
 * gimp_resolution_entry_set_pixel_digits:
 * @gre: a #GimpResolutionEntry
 * @digits: the number of digits to display for a pixel size
 *
 * Similar to gimp_unit_menu_set_pixel_digits(), this function allows
 * you set up a #GimpResolutionEntry so that sub-pixel sizes can be entered.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_pixel_digits (GimpResolutionEntry *gre,
                                        gint                 digits)
{
  GimpUnitMenu *menu;

  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  menu = GIMP_UNIT_MENU (gre->unitmenu);

  gimp_unit_menu_set_pixel_digits (menu, digits);
  gimp_resolution_entry_update_unit (gre, gimp_unit_menu_get_unit (menu));
}


/**
 * gimp_resolution_entry_grab_focus:
 * @gre: The #GimpResolutionEntry you want to grab the keyboard focus.
 *
 * This function is rather ugly and just a workaround for the fact that
 * it's impossible to implement gtk_widget_grab_focus() for a #GtkTable.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_grab_focus (GimpResolutionEntry *gre)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gtk_widget_grab_focus (gre->x.spinbutton);
}

/**
 * gimp_resolution_entry_set_activates_default:
 * @gre: A #GimpResolutionEntry
 * @setting: %TRUE to activate window's default widget on Enter keypress
 *
 * Iterates over all entries in the #GimpResolutionEntry and calls
 * gtk_entry_set_activates_default() on them.
 *
 * Since: GIMP 2.4
 **/
void
gimp_resolution_entry_set_activates_default (GimpResolutionEntry *gre,
                                             gboolean             setting)
{
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  gtk_entry_set_activates_default (GTK_ENTRY (gre->width.spinbutton),
                                   setting);
  gtk_entry_set_activates_default (GTK_ENTRY (gre->height.spinbutton),
                                   setting);
  gtk_entry_set_activates_default (GTK_ENTRY (gre->x.spinbutton),
                                   setting);

  if (gre->independent)
    gtk_entry_set_activates_default (GTK_ENTRY (gre->y.spinbutton),
                                     setting);

}

/**
 * gimp_resolution_entry_get_width_help_widget:
 * @gre: a #GimpResolutionEntry
 *
 * You shouldn't fiddle with the internals of a #GimpResolutionEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
gimp_resolution_entry_get_width_help_widget (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);

  return gre->width.spinbutton;
}

/**
 * gimp_resolution_entry_get_height_help_widget:
 * @gre: a #GimpResolutionEntry
 *
 * You shouldn't fiddle with the internals of a #GimpResolutionEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
gimp_resolution_entry_get_height_help_widget (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);

  return gre->height.spinbutton;
}

/**
 * gimp_resolution_entry_get_x_help_widget:
 * @gre: a #GimpResolutionEntry
 *
 * You shouldn't fiddle with the internals of a #GimpResolutionEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
gimp_resolution_entry_get_x_help_widget (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);

  return gre->x.spinbutton;
}

/**
 * gimp_resolution_entry_get_x_help_widget:
 * @gre: a #GimpResolutionEntry
 *
 * You shouldn't fiddle with the internals of a #GimpResolutionEntry but
 * if you want to set tooltips using gimp_help_set_help_data() you
 * can use this function to get a pointer to the spinbuttons.
 *
 * Return value: a #GtkWidget pointer that you can attach a tooltip to.
 **/
GtkWidget *
gimp_resolution_entry_get_y_help_widget (GimpResolutionEntry *gre)
{
  g_return_val_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre), NULL);
  g_return_val_if_fail (gre->independent, NULL);

  return gre->x.spinbutton;
}

/**
 * gimp_resolution_entry_update_width:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the width, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_width (GimpResolutionEntry *gre,
                                    gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_width (gre);
}


/**
 * gimp_resolution_entry_update_height:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the height, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_height (GimpResolutionEntry *gre,
                                     gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_height (gre);
}


/**
 * gimp_resolution_entry_update_x:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the X resolution, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_x (GimpResolutionEntry *gre,
                                gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_x (gre);
}

/**
 * gimp_resolution_entry_update_x_in_dpi:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the X resolution, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_x_in_dpi (GimpResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_x_in_dpi (gre);
}

/**
 * gimp_resolution_entry_update_y:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the Y resolution, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_y (GimpResolutionEntry *gre,
                                gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_y (gre);
}

/**
 * gimp_resolution_entry_update_y_in_dpi:
 * @gre: the #GimpResolutionEntry
 * @data: a pointer to a gdouble
 *
 * Convenience function to set a double to the Y resolution, suitable
 * for use as a signal callback.
 *
 * Since: GIMP 2.4
 */

void
gimp_resolution_entry_update_y_in_dpi (GimpResolutionEntry *gre,
                                       gpointer             data)
{
  gdouble *val;

  g_return_if_fail (gre  != NULL);
  g_return_if_fail (data != NULL);
  g_return_if_fail (GIMP_IS_RESOLUTION_ENTRY (gre));

  val = (gdouble *) data;

  *val = gimp_resolution_entry_get_y_in_dpi (gre);
}

static void
gimp_resolution_entry_format_label (GimpResolutionEntry *gre,
                                    GtkWidget           *label,
                                    gdouble              size)
{
  gchar *format = g_strdup_printf ("%%.%df %%s",
                                   gimp_unit_get_digits (gre->unit));
  gchar *text = g_strdup_printf (format,
                                 size * gimp_unit_get_factor (gre->unit),
                                 gimp_unit_get_plural (gre->unit));
  g_free (format);

  gtk_label_set_text (GTK_LABEL (label), text);
  g_free (text);
}
