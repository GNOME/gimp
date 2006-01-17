/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2005 Peter Mattis and Spencer Kimball
 *
 * gimpresolutionentry.h
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
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GIMP_RESOLUTION_ENTRY_H__
#define __GIMP_RESOLUTION_ENTRY_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_RESOLUTION_ENTRY            (gimp_resolution_entry_get_type ())
#define GIMP_RESOLUTION_ENTRY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntry))
#define GIMP_RESOLUTION_ENTRY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntryClass))
#define GIMP_IS_RESOLUTION_ENTRY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, GIMP_TYPE_RESOLUTION_ENTRY))
#define GIMP_IS_RESOLUTION_ENTRY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_RESOLUTION_ENTRY))
#define GIMP_RESOLUTION_ENTRY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_RESOLUTION_ENTRY, GimpResolutionEntryClass))


typedef struct _GimpResolutionEntryClass  GimpResolutionEntryClass;

typedef struct _GimpResolutionEntryField  GimpResolutionEntryField;

struct _GimpResolutionEntryField
{
  GimpResolutionEntry      *gre;
  GimpResolutionEntryField *corresponding;

  gboolean       size;

  GtkWidget     *label;

  guint          changed_signal;

  GtkObject     *adjustment;
  GtkWidget     *spinbutton;

  gdouble        phy_size;

  gdouble        value;
  gdouble        min_value;
  gdouble        max_value;

  gint           stop_recursion;
};


struct _GimpResolutionEntry
{
  GtkTable   parent_instance;


  GimpUnit                  size_unit;
  GimpUnit                  unit;
  gboolean                  independent;

  GtkWidget                *unitmenu;
  GtkWidget                *chainbutton;

  GimpResolutionEntryField  width;
  GimpResolutionEntryField  height;
  GimpResolutionEntryField  x;
  GimpResolutionEntryField  y;

};

struct _GimpResolutionEntryClass
{
  GtkTableClass  parent_class;

  void (* value_changed)  (GimpResolutionEntry *gse);
  void (* refval_changed) (GimpResolutionEntry *gse);
  void (* unit_changed)   (GimpResolutionEntry *gse);

  /* Padding for future expansion */
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
};


/* For information look into the C source or the html documentation */

GType       gimp_resolution_entry_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_resolution_entry_new          (const gchar   *width_label,
		                                gdouble        width,
					        const gchar   *height_label,
                                                gdouble        height,
					        GimpUnit       size_unit,
					        const gchar   *x_label,
                                                gdouble        initial_x,
					        const gchar   *y_label,
                                                gdouble        initial_y,
                                                GimpUnit       intial_unit,
                                                gboolean       independent,
                                                gint           spinbutton_width);

GtkWidget * gimp_resolution_entry_attach_label (GimpResolutionEntry *gre,
                                                const gchar         *text,
                                                gint                 row,
                                                gint                 column,
                                                gfloat               alignment);

void gimp_resolution_entry_set_width_boundaries
                                               (GimpResolutionEntry *gre,
                                                gdouble              lower,
                                                gdouble              upper);
void gimp_resolution_entry_set_height_boundaries
                                               (GimpResolutionEntry *gre,
                                                gdouble              lower,
                                                gdouble              upper);
void gimp_resolution_entry_set_x_boundaries    (GimpResolutionEntry *gre,
                                                gdouble              lower,
                                                gdouble               upper);
void gimp_resolution_entry_set_y_boundaries    (GimpResolutionEntry *gre,
                                                gdouble              lower,
                                                gdouble              upper);

gdouble gimp_resolution_entry_get_width        (GimpResolutionEntry *gre);
gdouble gimp_resolution_entry_get_height       (GimpResolutionEntry *gre);

gdouble gimp_resolution_entry_get_x            (GimpResolutionEntry *gre);
gdouble gimp_resolution_entry_get_x_in_dpi     (GimpResolutionEntry *gre);

gdouble gimp_resolution_entry_get_y            (GimpResolutionEntry *gre);
gdouble gimp_resolution_entry_get_y_in_dpi     (GimpResolutionEntry *gre);


void gimp_resolution_entry_set_width           (GimpResolutionEntry *gre,
                                                gdouble              value);
void gimp_resolution_entry_set_height          (GimpResolutionEntry *gre,
                                                gdouble              value);

void gimp_resolution_entry_set_x               (GimpResolutionEntry *gre,
                                                gdouble              value);
void gimp_resolution_entry_set_y               (GimpResolutionEntry *gre,
                                                gdouble              value);

GimpUnit    gimp_resolution_entry_get_unit     (GimpResolutionEntry *gre);
void        gimp_resolution_entry_set_unit     (GimpResolutionEntry *gre,
                                                GimpUnit             unit);

void        gimp_resolution_entry_show_unit_menu
                                               (GimpResolutionEntry *gre,
                                                gboolean             show);

void        gimp_resolution_entry_set_pixel_digits
                                               (GimpResolutionEntry *gre,
                                                gint                 digits);

void        gimp_resolution_entry_grab_focus  (GimpResolutionEntry *gre);

void        gimp_resolution_entry_set_activates_default
                                              (GimpResolutionEntry *gre,
                                               gboolean             setting);

GtkWidget * gimp_resolution_entry_get_width_help_widget
                                              (GimpResolutionEntry *gre);
GtkWidget * gimp_resolution_entry_get_height_help_widget
                                              (GimpResolutionEntry *gre);
GtkWidget * gimp_resolution_entry_get_x_help_widget
                                              (GimpResolutionEntry *gre);
GtkWidget * gimp_resolution_entry_get_y_help_widget
                                              (GimpResolutionEntry *gre);

/* signal callback convenience functions */
void        gimp_resolution_entry_update_width    (GimpResolutionEntry *gre,
                                                   gpointer             data);
void        gimp_resolution_entry_update_height   (GimpResolutionEntry *gre,
                                                   gpointer             data);

void        gimp_resolution_entry_update_x        (GimpResolutionEntry *gre,
                                                   gpointer             data);
void        gimp_resolution_entry_update_x_in_dpi (GimpResolutionEntry *gre,
                                                   gpointer             data);

void        gimp_resolution_entry_update_y        (GimpResolutionEntry *gre,
                                                   gpointer             data);
void        gimp_resolution_entry_update_y_in_dpi (GimpResolutionEntry *gre,
                                                   gpointer             data);
G_END_DECLS

#endif /* __GIMP_RESOLUTION_ENTRY_H__ */
