/*********************************************************************************/
/* Lighting Effects 0.2.2 -- image filter plug-in for The Gimp program           */
/* Copyright (C) 1996-98 Tom Bech                                                */
/* Copyright (C) 1996-98 Federico Mena Quintero                                  */
/*===============================================================================*/
/* E-mail: tomb@gimp.org (Tom) or quartic@gimp.org (Federico)                    */
/* You can contact the original The Gimp authors at gimp@xcf.berkeley.edu        */
/*===============================================================================*/
/* This program is free software; you can redistribute it and/or modify it under */
/* the terms of the GNU General Public License as published by the Free Software */
/* Foundation; either version 2 of the License, or (at your option) any later    */
/* version.                                                                      */
/*===============================================================================*/
/* This program is distributed in the hope that it will be useful, but WITHOUT   */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS */
/* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.*/
/*===============================================================================*/
/* You should have received a copy of the GNU General Public License along with  */
/* this program (read the "COPYING" file); if not, write to the Free Software    */
/* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                     */
/*===============================================================================*/
/* In other words, you can't sue us for whatever happens while using this ;)     */
/*********************************************************************************/

#include "lighting_main.h"

/* Global variables */
/* ================ */

LightingValues mapvals;

/******************/
/* Implementation */
/******************/

void lighting_interactive    (GDrawable *drawable);
void lighting_noninteractive (GDrawable *drawable);

/*************************************/
/* Set parameters to standard values */
/*************************************/

void set_default_settings(void)
{
  gck_vector3_set(&mapvals.viewpoint,   0.5, 0.5, 0.25);
  gck_vector3_set(&mapvals.planenormal, 0.0, 0.0, 1.0);
  
  gck_vector3_set(&mapvals.lightsource.position,  1.0, 0.0, 1.0);
  gck_vector3_set(&mapvals.lightsource.direction, -1.0, -1.0, 1.0);

  gck_rgb_set(&mapvals.lightsource.color, 1.0, 1.0, 1.0);
  mapvals.lightsource.intensity = 1.0;
  mapvals.lightsource.type = POINT_LIGHT;

  mapvals.material.ambient_int = 0.3;
  mapvals.material.diffuse_int = 1.0;
  mapvals.material.diffuse_ref = 0.4;
  mapvals.material.specular_ref = 0.6;
  mapvals.material.highlight = 27.0;

  mapvals.pixel_treshold = 0.25;
  mapvals.max_depth = 3.0;
  mapvals.preview_zoom_factor = 1.0;
  
/*  mapvals.bumptype=0; */
  mapvals.bumpmaptype=0;
  mapvals.bumpmin=0.0;
  mapvals.bumpmax=0.1;

  mapvals.antialiasing = FALSE;
  mapvals.create_new_image = FALSE;
  mapvals.transparent_background = FALSE;
  mapvals.tooltips_enabled = FALSE;
  mapvals.bump_mapped=FALSE;
  mapvals.env_mapped=FALSE;
  mapvals.ref_mapped=FALSE;
  mapvals.previewquality=FALSE;
  
  mapvals.bumpmap_id=-1;
  mapvals.envmap_id=-1;
}

void check_drawables(void)
{
  /* Check that envmap_id and bumpmap_id references legal images (are valid drawables) */
  /* ================================================================================= */

  if (mapvals.bumpmap_id!=-1 && gimp_drawable_image_id(mapvals.bumpmap_id)==-1)
    {
      mapvals.bump_mapped=FALSE;
      mapvals.bumpmap_id=-1;
    }

  if (mapvals.envmap_id!=-1 && gimp_drawable_image_id(mapvals.envmap_id)==-1)
    {
      mapvals.env_mapped=FALSE;
      mapvals.envmap_id=-1;
    }

  if (mapvals.bump_mapped)
    {
      /* Check if bump-map is grayscale and of the same size as the input drawable */
      /* ========================================================================= */
      
      if (!gimp_drawable_gray(mapvals.bumpmap_id) || 
           gimp_drawable_width(mapvals.drawable_id)!=gimp_drawable_width(mapvals.bumpmap_id) ||
           gimp_drawable_height(mapvals.drawable_id)!=gimp_drawable_height(mapvals.bumpmap_id))
        {
          /* If not then we silently disable bump mapping */
          /* ============================================ */

          mapvals.bump_mapped=FALSE;
          mapvals.bumpmap_id=-1;
        }
    }

  if (mapvals.env_mapped)
    {
      /* Check if env-map is grayscale or has alpha */
      /* ========================================== */
      
      if (gimp_drawable_gray(mapvals.envmap_id) || 
          gimp_drawable_has_alpha(mapvals.envmap_id))
        {
          /* If it has then we silently disable env mapping */
          /* ============================================== */

          mapvals.bump_mapped=FALSE;
          mapvals.bumpmap_id=-1;
        }
    }
}

MAIN();

static void query(void)
{
  static GParamDef args[] =
    {
      { PARAM_INT32,    "run_mode",              "Interactive (0), non-interactive (1)" },
      { PARAM_IMAGE,    "image",                 "Input image" },
      { PARAM_DRAWABLE, "drawable",              "Input drawable" },
      { PARAM_DRAWABLE, "bumpdrawable",          "Bumpmap drawable (set to 0 if disabled)" },
      { PARAM_DRAWABLE, "envdrawable",           "Environmentmap drawable (set to 0 if disabled)" },
      { PARAM_INT32,    "dobumpmap",             "Enable bumpmapping (TRUE/FALSE)" },
      { PARAM_INT32,    "doenvmap",              "Enable envmapping (TRUE/FALSE)" },
      { PARAM_INT32,    "bumpmaptype",           "Type of mapping (0=linear,1=log, 2=sinusoidal, 3=spherical)" },
      { PARAM_INT32,    "lighttype",             "Type of lightsource (0=point,1=directional,3=spot,4=none)" },
      { PARAM_COLOR,    "lightcolor",            "Lightsource color (r,g,b)" },
      { PARAM_FLOAT,    "lightposition_x",       "Lightsource position (x,y,z)" },
      { PARAM_FLOAT,    "lightposition_y",       "Lightsource position (x,y,z)" },
      { PARAM_FLOAT,    "lightposition_z",       "Lightsource position (x,y,z)" },
      { PARAM_FLOAT,    "lightdirection_x",      "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT,    "lightdirection_y",      "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT,    "lightdirection_z",      "Lightsource direction [x,y,z]" },
      { PARAM_FLOAT,    "ambient_intensity",     "Material ambient intensity (0..1)" },
      { PARAM_FLOAT,    "diffuse_intensity",     "Material diffuse intensity (0..1)" },
      { PARAM_FLOAT,    "diffuse_reflectivity",  "Material diffuse reflectivity (0..1)" },
      { PARAM_FLOAT,    "specular_reflectivity", "Material specular reflectivity (0..1)" },
      { PARAM_FLOAT,    "highlight",             "Material highlight (0..->), note: it's expotential" },
      { PARAM_INT32,    "antialiasing",          "Apply antialiasing (TRUE/FALSE)" },
      { PARAM_INT32,    "newimage",              "Create a new image (TRUE/FALSE)" },
      { PARAM_INT32,    "transparentbackground", "Make background transparent (TRUE/FALSE)" }
    };


  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_lighting",
			  "Apply various lighting effects to an image",
			  "No help yet",
			  "Tom Bech & Federico Mena Quintero",
			  "Tom Bech & Federico Mena Quintero",
			  "Version 0.2.0, March 15 1998",
			  "<Image>/Filters/Light Effects/Lighting Effects",
			  "RGB*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void run(gchar   *name,
                gint     nparams,
                GParam  *param,
                gint    *nreturn_vals,
                GParam **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /* Set default values */
  /* ================== */

  set_default_settings();

  /* Possibly retrieve data */
  /* ====================== */

  gimp_get_data ("plug_in_lighting", &mapvals);

  /* Get the specified drawable */
  /* ========================== */
  
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  mapvals.drawable_id=drawable->id;

  check_drawables();

  if (status == STATUS_SUCCESS)
    {
      /* Make sure that the drawable is RGBA or RGB color */
      /* ================================================ */

      if (gimp_drawable_color(drawable->id))
	{
	  /* Set the tile cache size */
          /* ======================= */

	  gimp_tile_cache_ntiles(TILE_CACHE_SIZE);
          
          switch (run_mode)
            {
              case RUN_INTERACTIVE:
                lighting_interactive(drawable);
                gimp_set_data("plug_in_lighting", &mapvals, sizeof(LightingValues));
              break;
              case RUN_WITH_LAST_VALS:
                image_setup(drawable,FALSE);
                compute_image();
                break;
              case RUN_NONINTERACTIVE:
                if (nparams != 24)
                  status = STATUS_CALLING_ERROR;
                else if (status == STATUS_SUCCESS)
                  {
                    mapvals.bumpmap_id              = param[3].data.d_drawable;
                    mapvals.envmap_id               = param[4].data.d_drawable;
                    mapvals.bump_mapped             = (gint)param[5].data.d_int32;
                    mapvals.env_mapped              = (gint)param[6].data.d_int32;
                    mapvals.bumpmaptype             = (gint)param[7].data.d_int32;
                    mapvals.lightsource.type        = (LightType)param[8].data.d_int32;
                    mapvals.lightsource.color.r     = param[9].data.d_color.red;
                    mapvals.lightsource.color.g     = param[9].data.d_color.green;
                    mapvals.lightsource.color.b     = param[9].data.d_color.blue;
                    mapvals.lightsource.position.x  = param[10].data.d_float;
                    mapvals.lightsource.position.y  = param[11].data.d_float;
                    mapvals.lightsource.position.z  = param[12].data.d_float;
                    mapvals.lightsource.direction.x = param[13].data.d_float;
                    mapvals.lightsource.direction.y = param[14].data.d_float;
                    mapvals.lightsource.direction.z = param[15].data.d_float;
                    mapvals.material.ambient_int    = param[16].data.d_float;
                    mapvals.material.diffuse_int    = param[17].data.d_float;
                    mapvals.material.diffuse_ref    = param[18].data.d_float;
                    mapvals.material.specular_ref   = param[19].data.d_float;
                    mapvals.material.highlight      = param[20].data.d_float;
                    mapvals.antialiasing            = (gint)param[21].data.d_int32;
                    mapvals.create_new_image        = (gint)param[22].data.d_int32;
                    mapvals.transparent_background  = (gint)param[23].data.d_int32;

                    check_drawables();
                    image_setup(drawable, FALSE);
                    compute_image();
                  }
              default:
                break;
            }
        }
      else
        status = STATUS_EXECUTION_ERROR;
    }

  if (run_mode != RUN_NONINTERACTIVE)
     gimp_displays_flush();                                              

  values[0].data.d_status = status;
  gimp_drawable_detach (drawable);

  if (xpostab)
      g_free(xpostab);
  if (ypostab)
      g_free(ypostab);
}

GPlugInInfo PLUG_IN_INFO =
{
  NULL,    /* init_proc */
  NULL,    /* quit_proc */
  query,   /* query_proc */
  run,     /* run_proc */
};

void lighting_interactive(GDrawable *drawable)
{
  gchar **argv;
  gint argc;

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("lighting");

  gdk_set_show_events(0);
  gdk_set_use_xshm(gimp_use_xshm());

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc());

  /* Create application window */
  /* ========================= */

  create_main_dialog();
  
  /* Prepare images */
  /* ============== */

  image_setup(drawable,TRUE);
  
  /* Gtk main event loop */
  /* =================== */
  
  gtk_main();
  gdk_flush();
}

void lighting_noninteractive(GDrawable *drawable)
{
  printf("Noninteractive not yet implemented! Sorry.\n");
}

