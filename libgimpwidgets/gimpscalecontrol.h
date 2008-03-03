/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimpscalecontrol.h
 * Copyright (C) 2000 Michael Natterer <mitch@gimp.org>,
 *               2008 Bill Skaggs <weskaggs@primate.ucdavis.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_SCALE_CONTROL_H__
#define __GIMP_SCALE_CONTROL_H__

G_BEGIN_DECLS

#define GIMP_TYPE_SCALE_CONTROL            (gimp_scale_control_get_type ())
#define GIMP_SCALE_CONTROL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_SCALE_CONTROL, GimpScaleControl))
#define GIMP_SCALE_CONTROL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_SCALE_CONTROL, GimpScaleControlClass))
#define GIMP_IS_SCALE_CONTROL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_SCALE_CONTROL))
#define GIMP_IS_SCALE_CONTROL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_SCALE_CONTROL))
#define GIMP_SCALE_CONTROL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_SCALE_CONTROL, GimpScaleControlClass))

/* FIXME: shouldn't need these defines */
#define GIMP_SCALE_CONTROL_LABEL(adj)   (GIMP_SCALE_CONTROL (adj)->label)

#define GIMP_SCALE_CONTROL_SCALE(adj)  (GIMP_SCALE_CONTROL(adj)->scale)
#define GIMP_SCALE_CONTROL_SCALE_ADJ(adj) \
        gtk_range_get_adjustment \
        (GTK_RANGE (GIMP_SCALE_CONTROL(adj)->scale))

#define GIMP_SCALE_CONTROL_SPINBUTTON(adj) (GIMP_SCALE_CONTROL(adj)->spinbutton)
#define GIMP_SCALE_CONTROL_SPINBUTTON_ADJ(adj) \
        gtk_spin_button_get_adjustment \
        (GTK_SPIN_BUTTON (GIMP_SCALE_CONTROL(adj)->spinbutton))

typedef struct _GimpScaleControlClass  GimpScaleControlClass;

struct _GimpScaleControl
{
  GtkAdjustment  parent_instance;

  gboolean       color_scale;
  gint           scale_width;
  gint           spinbutton_width;
  guint          digits;
  gboolean       constrain;
  gdouble        unconstrained_lower;
  gdouble        unconstrained_upper;
  const gchar   *tooltip;
  const gchar   *help_id;
  gboolean       logarithmic;
  GtkUpdateType  policy;
  const gchar   *text;

  GtkObject     *scale_adj;
  GtkObject     *spinbutton_adj;

  GtkWidget     *label;
  GtkWidget     *scale;
  GtkWidget     *spinbutton;
  GtkWidget     *button;
  GtkWidget     *button_label;
  GtkWidget     *entry;
  GtkWidget     *popup_button;
  GtkWidget     *popup_window;
};

struct _GimpScaleControlClass
{
  GtkAdjustmentClass  parent_class;

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


/* For information look into the C source or the html documentation */

GType       gimp_scale_control_get_type (void) G_GNUC_CONST;

GtkObject * gimp_scale_control_new             (GtkTable    *table,
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

GtkObject * gimp_color_scale_control_new       (GtkTable    *table,
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

GtkObject * gimp_scale_control_compact_new     (GtkTable    *table,
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

void        gimp_scale_control_set_sensitive   (GtkObject   *adjustment,
                                              gboolean     sensitive);

void        gimp_scale_control_set_logarithmic (GtkObject   *adjustment,
                                              gboolean     logarithmic);
gboolean    gimp_scale_control_get_logarithmic (GtkObject   *adjustment);

void        gimp_scale_control_set_update_policy (GtkObject     *adjustment,
                                                GtkUpdateType  policy);

GtkWidget * gimp_scale_control_get_widget     (GtkObject     *adjustment);

G_END_DECLS

#endif /* __GIMP_SCALE_CONTROL_H__ */
