/* Lighting Effects 0.2.2 -- image filter plug-in for GIMP
 *
 * Copyright (C) 1996-98 Tom Bech
 * Copyright (C) 1996-98 Federico Mena Quintero
 *
 * E-mail: tomb@gimp.org (Tom) or quartic@gimp.org (Federico)
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

#include <gtk/gtk.h>

#include <libgimp/gimp.h>

#include "lighting-apply.h"
#include "lighting-image.h"
#include "lighting-main.h"
#include "lighting-preview.h"
#include "lighting-shade.h"
#include "lighting-ui.h"

#include "libgimp/stdplugins-intl.h"


LightingValues mapvals;


typedef struct _Lighting      Lighting;
typedef struct _LightingClass LightingClass;

struct _Lighting
{
  GimpPlugIn parent_instance;
};

struct _LightingClass
{
  GimpPlugInClass parent_class;
};


#define LIGHTING_TYPE  (lighting_get_type ())
#define LIGHTING(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTING_TYPE, Lighting))

GType                   lighting_get_type         (void) G_GNUC_CONST;

static GList          * lighting_query_procedures (GimpPlugIn           *plug_in);
static GimpProcedure  * lighting_create_procedure (GimpPlugIn           *plug_in,
                                                   const gchar          *name);

static GimpValueArray * lighting_run              (GimpProcedure        *procedure,
                                                   GimpRunMode           run_mode,
                                                   GimpImage            *image,
                                                   GimpDrawable        **drawables,
                                                   GimpProcedureConfig  *config,
                                                   gpointer              run_data);

static void             set_default_settings      (void);
static void             check_drawables           (void);


G_DEFINE_TYPE (Lighting, lighting, GIMP_TYPE_PLUG_IN)

GIMP_MAIN (LIGHTING_TYPE)
DEFINE_STD_SET_I18N


static void
lighting_class_init (LightingClass *klass)
{
  GimpPlugInClass *plug_in_class = GIMP_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = lighting_query_procedures;
  plug_in_class->create_procedure = lighting_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
lighting_init (Lighting *lighting)
{
}

static GList *
lighting_query_procedures (GimpPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static GimpProcedure *
lighting_create_procedure (GimpPlugIn  *plug_in,
                       const gchar *name)
{
  GimpProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      GeglColor *default_color_1;
      GeglColor *default_color_2;
      GeglColor *default_color_3;
      GeglColor *default_color_4;
      GeglColor *default_color_5;
      GeglColor *default_color_6;

      gegl_init (NULL, NULL);

      default_color_1 = gegl_color_new ("white");
      default_color_2 = gegl_color_new ("white");
      default_color_3 = gegl_color_new ("white");
      default_color_4 = gegl_color_new ("white");
      default_color_5 = gegl_color_new ("white");
      default_color_6 = gegl_color_new ("white");

      procedure = gimp_image_procedure_new (plug_in, name,
                                            GIMP_PDB_PROC_TYPE_PLUGIN,
                                            lighting_run, NULL, NULL);

      gimp_procedure_set_image_types (procedure, "RGB*");
      gimp_procedure_set_sensitivity_mask (procedure,
                                           GIMP_PROCEDURE_SENSITIVE_DRAWABLE);

      gimp_procedure_set_menu_label (procedure, _("_Lighting Effects..."));
      gimp_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Light and Shadow/[Light]");

      gimp_procedure_set_documentation (procedure,
                                        _("Apply various lighting effects "
                                          "to an image"),
                                        "No help yet",
                                        name);
      gimp_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 0.2.0, March 15 1998");

      gimp_procedure_add_drawable_argument (procedure, "bump-drawable",
                                            _("B_ump map image"),
                                            _("Bumpmap drawable (set to NULL if disabled)"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_drawable_argument (procedure, "env-drawable",
                                            _("Enviro_nment map image"),
                                            _("Environmentmap drawable "
                                              "(set to NULL if disabled)"),
                                            TRUE,
                                            G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "do-bumpmap",
                                           _("Enable bump mappi_ng"),
                                           _("Enable bumpmapping"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "do-envmap",
                                           _("Enable en_vironment mapping"),
                                           _("Enable envmapping"),
                                           TRUE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "bumpmap-type",
                                          _("Cur_ve"),
                                          _("Type of mapping"),
                                          gimp_choice_new_with_values ("bumpmap-linear",     LINEAR_MAP,      _("Linear"),      NULL,
                                                                       "bumpmap-log",        LOGARITHMIC_MAP, _("Logarithmic"), NULL,
                                                                       "bumpmap-sinusoidal", SINUSOIDAL_MAP,  _("Sinusoidal"),  NULL,
                                                                       "bumpmap-spherical",  SPHERICAL_MAP,   _("Spherical"),   NULL,
                                                                       NULL),
                                          "bumpmap-linear",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "bumpmap-max-height",
                                          _("Ma_ximum height"),
                                          _("The maximum height of the bumpmap"),
                                          0.0, G_MAXFLOAT, 0.1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_choice_aux_argument (procedure, "which-light",
                                              _("Active"),
                                              _("Which light is active in the GUI"),
                                              gimp_choice_new_with_values ("light-1", 0, _("Light 1"), NULL,
                                                                           "light-2", 1, _("Light 2"), NULL,
                                                                           "light-3", 2, _("Light 3"), NULL,
                                                                           "light-4", 3, _("Light 4"), NULL,
                                                                           "light-5", 4, _("Light 5"), NULL,
                                                                           "light-6", 5, _("Light 6"), NULL,
                                                                           NULL),
                                              "light-1",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_boolean_aux_argument (procedure, "isolate",
                                               _("Isolate"),
                                               _("Only show the active lighting in "
                                                 "the preview"),
                                               FALSE,
                                               G_PARAM_READWRITE);

      gimp_procedure_add_choice_argument (procedure, "light-type-1",
                                          _("Type"),
                                          _("Type of light source"),
                                          gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                       "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                       "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                       "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                       NULL),
                                          "light-point",
                                          G_PARAM_READWRITE);

      gimp_procedure_add_color_argument (procedure, "light-color-1",
                                         _("Color"),
                                         _("Light source color"),
                                         TRUE, default_color_1,
                                         G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-intensity-1",
                                          _("Intensity"),
                                          _("Light source intensity"),
                                          0, 100.0, 1.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-position-x-1",
                                          _("Light position X"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-position-y-1",
                                          _("Light position Y"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-position-z-1",
                                          _("Light position Z"),
                                          _("Light source position (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-direction-x-1",
                                          _("Light direction X"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-direction-y-1",
                                          _("Light direction Y"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "light-direction-z-1",
                                          _("Light direction Z"),
                                          _("Light source direction (x,y,z)"),
                                          -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "ambient-intensity",
                                          _("Ambient intensity"),
                                          _("Material ambient intensity "
                                            "(Glowing)"),
                                          0, 1, 0.2,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "diffuse-intensity",
                                          _("Diffuse intensity"),
                                          _("Material diffuse intensity "
                                            "(Bright)"),
                                          0, 1, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "diffuse-reflectivity",
                                          _("Diffuse reflectivity"),
                                          _("Material diffuse reflectivity"),
                                          0, 1, 0.4,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "specular-reflectivity",
                                          _("Specular reflectivity"),
                                          _("Material specular reflectivity"),
                                          0, 1, 0.5,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "highlight",
                                          _("Highlight"),
                                          _("Material highlight (note, it's exponential) "
                                            "(Polished)"),
                                          0, G_MAXDOUBLE, 27.0,
                                          G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "metallic",
                                           _("Metallic"),
                                           _("Make surfaces look metallic"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "antialiasing",
                                           _("_Antialiasing"),
                                           _("Apply antialiasing"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "new-image",
                                           _("Create new ima_ge"),
                                           _("Create a new image"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_boolean_argument (procedure, "transparent-background",
                                           _("Trans_parent background"),
                                           _("Make background transparent"),
                                           FALSE,
                                           G_PARAM_READWRITE);

      gimp_procedure_add_double_argument (procedure, "distance",
                                          _("_Distance"),
                                          _("Distance of observer from surface"),
                                          0, 2.0, 0.25,
                                          G_PARAM_READWRITE);

      /* GUI-only arguments for additional light sources */
      gimp_procedure_add_choice_aux_argument (procedure, "light-type-2",
                                              _("Type"),
                                              _("Type of light source"),
                                              gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                           "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                           "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                           "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                           NULL),
                                              "light-none",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_color_aux_argument (procedure, "light-color-2",
                                             _("Color"),
                                             _("Light source color"),
                                             TRUE, default_color_2,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-intensity-2",
                                              _("Intensity"),
                                              _("Light source intensity"),
                                              0, 100.0, 1.0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-x-2",
                                              _("Light position X"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, -2,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-y-2",
                                              _("Light position Y"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-z-2",
                                              _("Light position Z"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-x-2",
                                              _("Light direction X"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-y-2",
                                              _("Light direction Y"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-z-2",
                                              _("Light direction Z"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_choice_aux_argument (procedure, "light-type-3",
                                              _("Type"),
                                              _("Type of light source"),
                                              gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                           "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                           "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                           "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                           NULL),
                                              "light-none",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_color_aux_argument (procedure, "light-color-3",
                                             _("Color"),
                                             _("Light source color"),
                                             TRUE, default_color_3,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-intensity-3",
                                              _("Intensity"),
                                              _("Light source intensity"),
                                              0, 100.0, 1.0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-x-3",
                                              _("Light position X"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-y-3",
                                              _("Light position Y"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 2,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-z-3",
                                              _("Light position Z"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-x-3",
                                              _("Light direction X"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-y-3",
                                              _("Light direction Y"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-z-3",
                                              _("Light direction Z"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_choice_aux_argument (procedure, "light-type-4",
                                              _("Type"),
                                              _("Type of light source"),
                                              gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                           "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                           "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                           "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                           NULL),
                                              "light-none",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_color_aux_argument (procedure, "light-color-4",
                                             _("Color"),
                                             _("Light source color"),
                                             TRUE, default_color_4,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-intensity-4",
                                              _("Intensity"),
                                              _("Light source intensity"),
                                              0, 100.0, 1.0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-x-4",
                                              _("Light position X"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-y-4",
                                              _("Light position Y"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-z-4",
                                              _("Light position Z"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-x-4",
                                              _("Light direction X"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-y-4",
                                              _("Light direction Y"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-z-4",
                                              _("Light direction Z"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_choice_aux_argument (procedure, "light-type-5",
                                              _("Type"),
                                              _("Type of light source"),
                                              gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                           "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                           "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                           "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                           NULL),
                                              "light-none",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_color_aux_argument (procedure, "light-color-5",
                                             _("Color"),
                                             _("Light source color"),
                                             TRUE, default_color_5,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-intensity-5",
                                              _("Intensity"),
                                              _("Light source intensity"),
                                              0, 100.0, 1.0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-x-5",
                                              _("Light position X"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-y-5",
                                              _("Light position Y"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-z-5",
                                              _("Light position Z"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-x-5",
                                              _("Light direction X"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-y-5",
                                              _("Light direction Y"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-z-5",
                                              _("Light direction Z"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_choice_aux_argument (procedure, "light-type-6",
                                              _("Type"),
                                              _("Type of light source"),
                                              gimp_choice_new_with_values ("light-none",        NO_LIGHT,          _("None"),        NULL,
                                                                           "light-directional", DIRECTIONAL_LIGHT, _("Directional"), NULL,
                                                                           "light-point",       POINT_LIGHT,       _("Point"),       NULL,
                                                                           "light-spot",        SPOT_LIGHT,        _("Spot"),        NULL,
                                                                           NULL),
                                              "light-none",
                                              G_PARAM_READWRITE);

      gimp_procedure_add_color_aux_argument (procedure, "light-color-6",
                                             _("Color"),
                                             _("Light source color"),
                                             TRUE, default_color_6,
                                             G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-intensity-6",
                                              _("Intensity"),
                                              _("Light source intensity"),
                                              0, 100.0, 1.0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-x-6",
                                              _("Light position X"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-y-6",
                                              _("Light position Y"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-position-z-6",
                                              _("Light position Z"),
                                              _("Light source position (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-x-6",
                                              _("Light direction X"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-y-6",
                                              _("Light direction Y"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                              G_PARAM_READWRITE);

      gimp_procedure_add_double_aux_argument (procedure, "light-direction-z-6",
                                              _("Light direction Z"),
                                              _("Light source direction (x,y,z)"),
                                              -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                                              G_PARAM_READWRITE);

      g_object_unref (default_color_1);
      g_object_unref (default_color_2);
      g_object_unref (default_color_3);
      g_object_unref (default_color_4);
      g_object_unref (default_color_5);
      g_object_unref (default_color_6);
    }

  return procedure;
}

static void
set_default_settings (void)
{
  gint k;

  mapvals.update_enabled = TRUE;
  mapvals.light_selected = 0;
  mapvals.light_isolated = FALSE;

  gimp_vector3_set (&mapvals.viewpoint,   0.5, 0.5, 0.25);
  gimp_vector3_set (&mapvals.planenormal, 0.0, 0.0, 1.0);

  gimp_vector3_set (&mapvals.lightsource[0].position,  -1.0, -1.0, 1.0);
  gimp_vector3_set (&mapvals.lightsource[0].direction, -1.0, -1.0, 1.0);

  for (gint i = 0; i < 4; i++)
    mapvals.lightsource[0].color[i] = 1.0;
  mapvals.lightsource[0].intensity = 1.0;
  mapvals.lightsource[0].type      = POINT_LIGHT;
  mapvals.lightsource[0].active    = TRUE;

  /* init lights 2 and 3 pos to upper left and below */
  gimp_vector3_set (&mapvals.lightsource[1].position,   2.0, -1.0, 1.0);
  gimp_vector3_set (&mapvals.lightsource[1].direction,  1.0, -1.0, 1.0);

  gimp_vector3_set (&mapvals.lightsource[2].position,   1.0,  2.0, 1.0);
  gimp_vector3_set (&mapvals.lightsource[2].direction,  0.0,  1.0, 1.0);

  /* init any remaining lights to directly overhead */
  for (k = 3; k < NUM_LIGHTS; k++)
    {
      gimp_vector3_set (&mapvals.lightsource[k].position,   0.0,  0.0, 1.0);
      gimp_vector3_set (&mapvals.lightsource[k].direction,  0.0,  0.0, 1.0);
    }

  for (k = 1; k < NUM_LIGHTS; k++)
    {
      for (gint i = 0; i < 4; i++)
        mapvals.lightsource[k].color[i] = 1.0;
      mapvals.lightsource[k].intensity = 1.0;
      mapvals.lightsource[k].type      = NO_LIGHT;
      mapvals.lightsource[k].active    = TRUE;
    }

  mapvals.material.ambient_int  =  0.2;
  mapvals.material.diffuse_int  =  0.5;
  mapvals.material.diffuse_ref  =  0.4;
  mapvals.material.specular_ref =  0.5;
  mapvals.material.highlight    = 27.0;
  mapvals.material.metallic     = FALSE;

  mapvals.pixel_threshold     = 0.25;
  mapvals.max_depth           =  3.0;
  mapvals.preview_zoom_factor =  1.0;

  mapvals.bumpmaptype = 0;
  mapvals.bumpmin     = 0.0;
  mapvals.bumpmax     = 0.1;

  mapvals.antialiasing           = FALSE;
  mapvals.create_new_image       = FALSE;
  mapvals.transparent_background = FALSE;
  mapvals.bump_mapped            = FALSE;
  mapvals.env_mapped             = FALSE;
  mapvals.ref_mapped             = FALSE;
  mapvals.previewquality         = FALSE;
  mapvals.interactive_preview    = TRUE;

  mapvals.bumpmap_id = -1;
  mapvals.envmap_id  = -1;
}

static void
check_drawables (void)
{
  GimpDrawable *drawable;
  GimpDrawable *map;

  if (mapvals.bump_mapped)
    {
      if (! gimp_item_id_is_drawable (mapvals.bumpmap_id))
        {
          mapvals.bump_mapped = FALSE;
          mapvals.bumpmap_id  = -1;
        }
      else
        {
          drawable = gimp_drawable_get_by_id (mapvals.drawable_id);
          map      = gimp_drawable_get_by_id (mapvals.bumpmap_id);

          if (gimp_drawable_is_indexed (map) ||
              (gimp_drawable_get_width  (drawable) != gimp_drawable_get_width  (map)) ||
              (gimp_drawable_get_height (drawable) != gimp_drawable_get_height (map)))
            {
              mapvals.bump_mapped = FALSE;
              mapvals.bumpmap_id  = -1;
            }
        }
    }

  if (mapvals.env_mapped)
    {
      if (! gimp_item_id_is_drawable (mapvals.envmap_id))
        {
          mapvals.env_mapped = FALSE;
          mapvals.envmap_id  = -1;
        }
      else
        {
          map = gimp_drawable_get_by_id (mapvals.envmap_id);

          if (gimp_drawable_is_gray   (map) ||
              gimp_drawable_has_alpha (map))
            {
              mapvals.env_mapped = FALSE;
              mapvals.envmap_id  = -1;
            }
        }
    }
}

static GimpValueArray *
lighting_run (GimpProcedure        *procedure,
              GimpRunMode           run_mode,
              GimpImage            *image,
              GimpDrawable        **drawables,
              GimpProcedureConfig  *config,
              gpointer              run_data)
{
  GimpDrawable *drawable;

  gegl_init (NULL, NULL);

  if (gimp_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   gimp_procedure_get_name (procedure));

      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  set_default_settings ();

  mapvals.drawable_id = gimp_item_get_id (GIMP_ITEM (drawable));

  check_drawables ();

  if (gimp_drawable_is_rgb (drawable))
    {
      mapvals.config = config;
      copy_from_config (config);

      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
          if (! main_dialog (procedure, config, drawable))
            {
              return gimp_procedure_new_return_values (procedure,
                                                       GIMP_PDB_CANCEL,
                                                       NULL);
            }

          compute_image ();

          gimp_displays_flush ();
          break;

        case GIMP_RUN_WITH_LAST_VALS:
          if (image_setup (drawable, FALSE))
            compute_image ();
          gimp_displays_flush ();
          break;

        case GIMP_RUN_NONINTERACTIVE:
          check_drawables ();
          if (image_setup (drawable, FALSE))
            compute_image ();
        default:
          break;
        }
    }
  else
    {
      return gimp_procedure_new_return_values (procedure,
                                               GIMP_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  g_free (xpostab);
  g_free (ypostab);

  return gimp_procedure_new_return_values (procedure, GIMP_PDB_SUCCESS, NULL);
}
