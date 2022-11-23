/* Lighting Effects 0.2.2 -- image filter plug-in for LIGMA
 *
 * Copyright (C) 1996-98 Tom Bech
 * Copyright (C) 1996-98 Federico Mena Quintero
 *
 * E-mail: tomb@ligma.org (Tom) or quartic@ligma.org (Federico)
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

#include <libligma/ligma.h>

#include "lighting-apply.h"
#include "lighting-image.h"
#include "lighting-main.h"
#include "lighting-preview.h"
#include "lighting-shade.h"
#include "lighting-ui.h"

#include "libligma/stdplugins-intl.h"


LightingValues mapvals;


typedef struct _Lighting      Lighting;
typedef struct _LightingClass LightingClass;

struct _Lighting
{
  LigmaPlugIn parent_instance;
};

struct _LightingClass
{
  LigmaPlugInClass parent_class;
};


#define LIGHTING_TYPE  (lighting_get_type ())
#define LIGHTING (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGHTING_TYPE, Lighting))

GType                   lighting_get_type         (void) G_GNUC_CONST;

static GList          * lighting_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * lighting_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * lighting_run              (LigmaProcedure        *procedure,
                                                   LigmaRunMode           run_mode,
                                                   LigmaImage            *image,
                                                   gint                  n_drawables,
                                                   LigmaDrawable        **drawables,
                                                   const LigmaValueArray *args,
                                                   gpointer              run_data);

static void             set_default_settings      (void);
static void             check_drawables           (void);


G_DEFINE_TYPE (Lighting, lighting, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (LIGHTING_TYPE)
DEFINE_STD_SET_I18N


static void
lighting_class_init (LightingClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = lighting_query_procedures;
  plug_in_class->create_procedure = lighting_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
lighting_init (Lighting *lighting)
{
}

static GList *
lighting_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
lighting_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      LigmaRGB white = { 1.0, 1.0, 1.0, 1.0 };

      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            lighting_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Lighting Effects..."));
      ligma_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Light and Shadow/Light");

      ligma_procedure_set_documentation (procedure,
                                        _("Apply various lighting effects "
                                          "to an image"),
                                        "No help yet",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Tom Bech & Federico Mena Quintero",
                                      "Tom Bech & Federico Mena Quintero",
                                      "Version 0.2.0, March 15 1998");

      LIGMA_PROC_ARG_DRAWABLE (procedure, "bump-drawable",
                              "Bump drawable",
                              "Bumpmap drawable (set to NULL if disabled)",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DRAWABLE (procedure, "env-drawable",
                              "Env drawable",
                              "Environmentmap drawable (set to NULL if disabled",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "do-bumpmap",
                             "Do bumpmap",
                             "Enable bumpmapping",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "do-envmap",
                             "Do envmap",
                             "Enable envmapping",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "bumpmap-type",
                         "Bumpmap type",
                         "Type of mapping (0=linear, 1=log, 2=sinusoidal, "
                         "3=spherical)",
                         0, 2, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "light-type",
                         "Light type",
                         "Type of lightsource (0=point, 1=directional, "
                         "3=spot, 4=none)",
                         0 ,4, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_RGB (procedure, "light-color",
                         "Light color",
                         "Light source color",
                         TRUE, &white,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-x",
                            "Light position X",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-y",
                            "Light position Y",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-position-z",
                            "Light position Z",
                            "Light source position (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-x",
                            "Light direction X",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-y",
                            "Light direction Y",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, -1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "light-direction-z",
                            "Light direction Z",
                            "Light source direction (x,y,z)",
                            -G_MAXDOUBLE, G_MAXDOUBLE, 1,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "ambient-intensity",
                            "Ambient intensity",
                            "Material ambient intensity",
                            0, 1, 0.2,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "diffuse-intensity",
                            "Diffuse intensity",
                            "Material diffuse intensity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "diffuse-reflectivity",
                            "Diffuse reflectivity",
                            "Material diffuse reflectivity",
                            0, 1, 0.4,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "specular-reflectivity",
                            "Specular reflectivity",
                            "Material specular reflectivity",
                            0, 1, 0.5,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "highlight",
                            "Highlight",
                            "Material highlight (note, it's exponential)",
                            0, G_MAXDOUBLE, 27.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "antialiasing",
                             "Antialiasing",
                             "Apply antialiasing",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "new-image",
                             "New image",
                             "Create a new image",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "transparent-background",
                             "Transparent background",
                             "Make background transparent",
                             FALSE,
                             G_PARAM_READWRITE);
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

  ligma_vector3_set (&mapvals.viewpoint,   0.5, 0.5, 0.25);
  ligma_vector3_set (&mapvals.planenormal, 0.0, 0.0, 1.0);

  ligma_vector3_set (&mapvals.lightsource[0].position,  -1.0, -1.0, 1.0);
  ligma_vector3_set (&mapvals.lightsource[0].direction, -1.0, -1.0, 1.0);

  ligma_rgba_set (&mapvals.lightsource[0].color, 1.0, 1.0, 1.0, 1.0);
  mapvals.lightsource[0].intensity = 1.0;
  mapvals.lightsource[0].type      = POINT_LIGHT;
  mapvals.lightsource[0].active    = TRUE;

  /* init lights 2 and 3 pos to upper left and below */
  ligma_vector3_set (&mapvals.lightsource[1].position,   2.0, -1.0, 1.0);
  ligma_vector3_set (&mapvals.lightsource[1].direction,  1.0, -1.0, 1.0);

  ligma_vector3_set (&mapvals.lightsource[2].position,   1.0,  2.0, 1.0);
  ligma_vector3_set (&mapvals.lightsource[2].direction,  0.0,  1.0, 1.0);

  /* init any remaining lights to directly overhead */
  for (k = 3; k < NUM_LIGHTS; k++)
    {
      ligma_vector3_set (&mapvals.lightsource[k].position,   0.0,  0.0, 1.0);
      ligma_vector3_set (&mapvals.lightsource[k].direction,  0.0,  0.0, 1.0);
    }

  for (k = 1; k < NUM_LIGHTS; k++)
    {
      ligma_rgba_set (&mapvals.lightsource[k].color, 1.0, 1.0, 1.0, 1.0);
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
  LigmaDrawable *drawable;
  LigmaDrawable *map;

  if (mapvals.bump_mapped)
    {
      if (! ligma_item_id_is_drawable (mapvals.bumpmap_id))
        {
          mapvals.bump_mapped = FALSE;
          mapvals.bumpmap_id  = -1;
        }
      else
        {
          drawable = ligma_drawable_get_by_id (mapvals.drawable_id);
          map      = ligma_drawable_get_by_id (mapvals.bumpmap_id);

          if (ligma_drawable_is_indexed (map) ||
              (ligma_drawable_get_width  (drawable) != ligma_drawable_get_width  (map)) ||
              (ligma_drawable_get_height (drawable) != ligma_drawable_get_height (map)))
            {
              mapvals.bump_mapped = FALSE;
              mapvals.bumpmap_id  = -1;
            }
        }
    }

  if (mapvals.env_mapped)
    {
      if (! ligma_item_id_is_drawable (mapvals.envmap_id))
        {
          mapvals.env_mapped = FALSE;
          mapvals.envmap_id  = -1;
        }
      else
        {
          map = ligma_drawable_get_by_id (mapvals.envmap_id);

          if (ligma_drawable_is_gray   (map) ||
              ligma_drawable_has_alpha (map))
            {
              mapvals.env_mapped = FALSE;
              mapvals.envmap_id  = -1;
            }
        }
    }
}

static LigmaValueArray *
lighting_run (LigmaProcedure        *procedure,
              LigmaRunMode           run_mode,
              LigmaImage            *image,
              gint                  n_drawables,
              LigmaDrawable        **drawables,
              const LigmaValueArray *args,
              gpointer              run_data)
{
  LigmaDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   ligma_procedure_get_name (procedure));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  set_default_settings ();

  ligma_get_data (PLUG_IN_PROC, &mapvals);

  mapvals.drawable_id = ligma_item_get_id (LIGMA_ITEM (drawable));

  check_drawables ();

  if (ligma_drawable_is_rgb (drawable))
    {
      switch (run_mode)
        {
        case LIGMA_RUN_INTERACTIVE:
          if (! main_dialog (drawable))
            {
              return ligma_procedure_new_return_values (procedure,
                                                       LIGMA_PDB_CANCEL,
                                                       NULL);
            }

          compute_image ();

          ligma_set_data (PLUG_IN_PROC, &mapvals, sizeof (LightingValues));
          ligma_displays_flush ();
          break;

        case LIGMA_RUN_WITH_LAST_VALS:
          if (image_setup (drawable, FALSE))
            compute_image ();
          ligma_displays_flush ();
          break;

        case LIGMA_RUN_NONINTERACTIVE:
          mapvals.bumpmap_id             = LIGMA_VALUES_GET_DRAWABLE_ID (args, 0);
          mapvals.envmap_id              = LIGMA_VALUES_GET_DRAWABLE_ID (args, 1);
          mapvals.bump_mapped                = LIGMA_VALUES_GET_BOOLEAN (args, 2);
          mapvals.env_mapped                 = LIGMA_VALUES_GET_BOOLEAN (args, 3);
          mapvals.bumpmaptype                = LIGMA_VALUES_GET_INT     (args, 4);
          mapvals.lightsource[0].type        = LIGMA_VALUES_GET_INT     (args, 5);

          LIGMA_VALUES_GET_RGB (args, 6, &mapvals.lightsource[0].color);

          mapvals.lightsource[0].position.x  = LIGMA_VALUES_GET_DOUBLE  (args, 7);
          mapvals.lightsource[0].position.y  = LIGMA_VALUES_GET_DOUBLE  (args, 8);
          mapvals.lightsource[0].position.z  = LIGMA_VALUES_GET_DOUBLE  (args, 9);
          mapvals.lightsource[0].direction.x = LIGMA_VALUES_GET_DOUBLE  (args, 10);
          mapvals.lightsource[0].direction.y = LIGMA_VALUES_GET_DOUBLE  (args, 11);
          mapvals.lightsource[0].direction.z = LIGMA_VALUES_GET_DOUBLE  (args, 12);
          mapvals.material.ambient_int       = LIGMA_VALUES_GET_DOUBLE  (args, 13);
          mapvals.material.diffuse_int       = LIGMA_VALUES_GET_DOUBLE  (args, 14);
          mapvals.material.diffuse_ref       = LIGMA_VALUES_GET_DOUBLE  (args, 15);
          mapvals.material.specular_ref      = LIGMA_VALUES_GET_DOUBLE  (args, 16);
          mapvals.material.highlight         = LIGMA_VALUES_GET_DOUBLE  (args, 17);
          mapvals.antialiasing               = LIGMA_VALUES_GET_BOOLEAN (args, 18);
          mapvals.create_new_image           = LIGMA_VALUES_GET_BOOLEAN (args, 19);
          mapvals.transparent_background     = LIGMA_VALUES_GET_BOOLEAN (args, 20);

          check_drawables ();
          if (image_setup (drawable, FALSE))
            compute_image ();
        default:
          break;
        }
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  g_free (xpostab);
  g_free (ypostab);

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}
