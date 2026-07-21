/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimpsavable.h
 * Copyright (C) 2026 Jehan
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

#pragma once


/* The GimpSavable interface if for various objects which will be saved
 * as part of a GIMP project. Each object type handles its own format
 * to be printed as XML into the output stream passed into to_xml().
 *
 * Additionally to printing its XML, an object may also do further
 * saving operations, such as, for drawables, making sure their buffer
 * is up-to-date in the on-disk cache.
 *
 * A few utils are also provided for storing some external data.
 */

#define WLBR_VERSION 1

typedef struct _GimpSaveState GimpSaveState;

struct _GimpSaveState
{
  GOutputStream *output;
  GFile         *xcf_file;
  GHashTable    *icc_references;
  GQueue        *elements;
};

#define GIMP_TYPE_SAVABLE (gimp_savable_get_type ())
G_DECLARE_INTERFACE (GimpSavable, gimp_savable, GIMP, SAVABLE, GObject)


struct _GimpSavableInterface
{
  GTypeInterface base_iface;

  /*  virtual functions  */
  void   (* save) (GimpSavable   *savable,
                   GimpSaveState *state);
};


void         gimp_savable_save                 (GimpSavable            *savable,
                                                GimpSaveState          *state);


/* Shared Utils */

void         gimp_savable_print_element_start  (GimpSaveState          *state,
                                                const gchar            *element_name,
                                                ...) G_GNUC_NULL_TERMINATED;
void         gimp_savable_print_element_end    (GimpSaveState          *state,
                                                const gchar            *element_name);
void         gimp_savable_print_element        (GimpSaveState          *state,
                                                const gchar            *element_name,
                                                const gchar            *format_value,
                                                ...) G_GNUC_NULL_TERMINATED;

void         gimp_savable_config_save          (GimpConfig             *config,
                                                const gchar            *element_name,
                                                GimpSaveState          *state);

void         gimp_savable_format_save          (const Babl             *format,
                                                GimpSaveState          *state);
void         gimp_savable_space_save           (const Babl             *space,
                                                GimpSaveState          *state,
                                                gint                    space_id);
void         gimp_savable_color_save           (GeglColor              *color,
                                                const gchar            *name,
                                                const Babl             *restricted_format,
                                                GimpSaveState          *state);

void         gimp_savable_save_all_spaces      (GimpImage              *image,
                                                GimpSaveState          *state);

void         gimp_savable_unit_save            (GimpUnit               *unit,
                                                GimpSaveState          *state);
void         gimp_savable_metadata_save        (GimpMetadata           *metadata,
                                                GimpSaveState          *state);
void         gimp_savable_parasite_save        (GimpParasite           *parasite,
                                                GimpSaveState          *state);
void         gimp_savable_matrix2_save         (GimpMatrix2            *matrix,
                                                GimpSaveState          *state);
void         gimp_savable_value_save           (GValue                 *value,
                                                GParamSpec             *pspec,
                                                GimpSaveState          *state);
void         gimp_savable_value_array_save     (GimpValueArray         *values,
                                                GimpSaveState          *state);

void         gimp_savable_composite_mode_save  (GimpLayerCompositeMode  composite_mode,
                                                GimpLayerMode           mode,
                                                GimpSaveState          *state);
void         gimp_savable_composite_space_save (GimpLayerColorSpace     composite_space,
                                                GimpLayerMode           mode,
                                                GimpSaveState          *state);
void         gimp_savable_blend_space_save     (GimpLayerColorSpace     blend_space,
                                                GimpLayerMode           mode,
                                                GimpSaveState          *state);
