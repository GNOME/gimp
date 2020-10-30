/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscaleentry.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>,
 *               2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_WIDGETS_H_INSIDE__) && !defined (GIMP_WIDGETS_COMPILATION)
#error "Only <libgimpwidgets/gimpwidgets.h> can be included directly."
#endif

#ifndef __GIMP_SCALE_ENTRY_H__
#define __GIMP_SCALE_ENTRY_H__

G_BEGIN_DECLS

#define GIMP_TYPE_SCALE_ENTRY            (gimp_scale_entry_get_type ())
#define GIMP_SCALE_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SCALE_ENTRY, GimpScaleEntry))
#define GIMP_SCALE_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SCALE_ENTRY, GimpScaleEntryClass))
#define GIMP_IS_SCALE_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_SCALE_ENTRY))
#define GIMP_IS_SCALE_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SCALE_ENTRY))
#define GIMP_SCALE_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SCALE_ENTRY, GimpScaleEntryClass))


typedef struct _GimpScaleEntryPrivate GimpScaleEntryPrivate;
typedef struct _GimpScaleEntryClass   GimpScaleEntryClass;

struct _GimpScaleEntry
{
  GtkGrid                parent_instance;

  GimpScaleEntryPrivate *priv;
};

struct _GimpScaleEntryClass
{
  GtkGridClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};

GType           gimp_scale_entry_get_type        (void) G_GNUC_CONST;

GtkWidget     * gimp_scale_entry_new2            (const gchar *text,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  gdouble      step_increment,
                                                  gdouble      page_increment,
                                                  guint        digits);

gdouble         gimp_scale_entry_get_value       (GimpScaleEntry *entry);

GtkWidget     * gimp_scale_entry_get_label       (GimpScaleEntry *entry);
GtkWidget     * gimp_scale_entry_get_spin_button (GimpScaleEntry *entry);
GtkWidget     * gimp_scale_entry_get_scale       (GimpScaleEntry *entry);

void            gimp_scale_entry_set_range       (GimpScaleEntry *entry,
                                                  gdouble         lower,
                                                  gdouble         upper,
                                                  gboolean        limit_scale);

void            gimp_scale_entry_set_logarithmic (GimpScaleEntry *entry,
                                                  gboolean        logarithmic);
gboolean        gimp_scale_entry_get_logarithmic (GimpScaleEntry *entry);

/**
 * GIMP_SCALE_ENTRY_LABEL:
 * @adj: The #GtkAdjustment returned by gimp_scale_entry_new().
 *
 * Returns: the scale_entry's #GtkLabel.
 **/
#define GIMP_SCALE_ENTRY_LABEL(adj) \
        (g_object_get_data (G_OBJECT (adj), "label"))

/**
 * GIMP_SCALE_ENTRY_SCALE:
 * @adj: The #GtkAdjustment returned by gimp_scale_entry_new().
 *
 * Returns: the scale_entry's #GtkHScale.
 **/
#define GIMP_SCALE_ENTRY_SCALE(adj) \
        (g_object_get_data (G_OBJECT (adj), "scale"))

/**
 * GIMP_SCALE_ENTRY_SCALE_ADJ:
 * @adj: The #GtkAdjustment returned by gimp_scale_entry_new().
 *
 * Returns: the #GtkAdjustment of the scale_entry's #GtkHScale.
 **/
#define GIMP_SCALE_ENTRY_SCALE_ADJ(adj)     \
        gtk_range_get_adjustment \
        (GTK_RANGE (g_object_get_data (G_OBJECT (adj), "scale")))

/**
 * GIMP_SCALE_ENTRY_SPINBUTTON:
 * @adj: The #GtkAdjustment returned by gimp_scale_entry_new().
 *
 * Returns: the scale_entry's #GtkSpinButton.
 **/
#define GIMP_SCALE_ENTRY_SPINBUTTON(adj) \
        (g_object_get_data (G_OBJECT (adj), "spinbutton"))

/**
 * GIMP_SCALE_ENTRY_SPINBUTTON_ADJ:
 * @adj: The #GtkAdjustment returned by gimp_scale_entry_new().
 *
 * Returns: the #GtkAdjustment of the scale_entry's #GtkSpinButton.
 **/
#define GIMP_SCALE_ENTRY_SPINBUTTON_ADJ(adj) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (g_object_get_data (G_OBJECT (adj), "spinbutton")))


GtkAdjustment * gimp_scale_entry_new             (GtkGrid     *grid,
                                                  gint         column,
                                                  gint         row,
                                                  const gchar *text,
                                                  gint         scale_width,
                                                  gint         spinbutton_width,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  gdouble      step_increment,
                                                  gdouble      page_increment,
                                                  guint        digits,
                                                  gboolean     constrain,
                                                  gdouble      unconstrained_lower,
                                                  gdouble      unconstrained_upper,
                                                  const gchar *tooltip,
                                                  const gchar *help_id);

GtkAdjustment * gimp_color_scale_entry_new       (GtkGrid     *grid,
                                                  gint         column,
                                                  gint         row,
                                                  const gchar *text,
                                                  gint         scale_width,
                                                  gint         spinbutton_width,
                                                  gdouble      value,
                                                  gdouble      lower,
                                                  gdouble      upper,
                                                  gdouble      step_increment,
                                                  gdouble      page_increment,
                                                  guint        digits,
                                                  const gchar *tooltip,
                                                  const gchar *help_id);

void            gimp_scale_entry_set_sensitive   (GtkAdjustment *adjustment,
                                                  gboolean       sensitive);




G_END_DECLS

#endif /* __GIMP_SCALE_ENTRY_H__ */
