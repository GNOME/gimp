/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpresolutionentry.h
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

#if !defined (__GIMP_UI_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimpui.h> can be included directly."
#endif

#ifndef __GIMP_RESOLUTION_ENTRY__
#define __GIMP_RESOLUTION_ENTRY__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */

/*
 * This widget was initially part of libgimpwidgets. Then it got removed and
 * moved as private code to file-pdf-load plug-in.
 *
 * I'm adding it back for GIMP 3 release as its own code but not making it
 * public (only to be used by GimpVectorLoadProcedure so far) so that we don't
 * have to make sure the API is really right.
 *
 * TODO: we want to eventually get this back to public API in libgimpwidgets,
 * with a much nicer API and also adding a property widget function.
 */
#define GIMP_TYPE_RESOLUTION_ENTRY (gimp_resolution_entry_get_type ())
G_DECLARE_FINAL_TYPE (GimpResolutionEntry, gimp_resolution_entry, GIMP, RESOLUTION_ENTRY, GtkGrid)

typedef struct _GimpResolutionEntry GimpResolutionEntry;

GtkWidget * gimp_prop_resolution_entry_new          (GObject                  *config,
                                                     const gchar              *width_prop,
                                                     const gchar              *height_prop,
                                                     const gchar              *ppi_prop,
                                                     const gchar              *unit_prop);
GtkWidget * gimp_resolution_entry_new               (const gchar              *width_label,
                                                     gint                      width,
                                                     const gchar              *height_label,
                                                     gint                      height,
                                                     const gchar              *res_label,
                                                     gdouble                   pixel_density,
                                                     GimpUnit                 *display_unit);

void        gimp_resolution_entry_set_width         (GimpResolutionEntry      *entry,
                                                     gint                      width);
void        gimp_resolution_entry_set_height        (GimpResolutionEntry      *entry,
                                                     gint                      height);
void        gimp_resolution_entry_set_pixel_density (GimpResolutionEntry      *entry,
                                                     gdouble                   ppi);
void        gimp_resolution_entry_set_unit          (GimpResolutionEntry      *entry,
                                                     GimpUnit                 *unit);
void        gimp_resolution_entry_set_keep_ratio    (GimpResolutionEntry      *entry,
                                                     gboolean                  keep_ratio);

gint        gimp_resolution_entry_get_width         (GimpResolutionEntry      *entry);
gint        gimp_resolution_entry_get_height        (GimpResolutionEntry      *entry);
gdouble     gimp_resolution_entry_get_density       (GimpResolutionEntry      *entry);
GimpUnit  * gimp_resolution_entry_get_unit          (GimpResolutionEntry      *entry);
gboolean    gimp_resolution_entry_get_keep_ratio    (GimpResolutionEntry      *entry);

G_END_DECLS

#endif /* __GIMP_RESOLUTION_ENTRY__ */
