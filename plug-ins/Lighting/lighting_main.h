#ifndef LIGHTINGMAINH
#define LIGHTINGMAINH

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gck/gck.h>
#include <libgimp/gimp.h>

#include "lighting_ui.h"
#include "lighting_image.h"
#include "lighting_apply.h"
#include "lighting_preview.h"

/* Defines and stuff */
/* ================= */

#define TILE_CACHE_SIZE 16

/* Typedefs */
/* ======== */

typedef enum {
  POINT_LIGHT,
  DIRECTIONAL_LIGHT,
  SPOT_LIGHT, 
  NO_LIGHT
} LightType;

enum {
  IMAGE_BUMP,
  WAVES_BUMP
};

typedef struct
{
  gdouble ambient_int;
  gdouble diffuse_int;
  gdouble diffuse_ref;
  gdouble specular_ref;
  gdouble highlight;
  GckRGB  color;
} MaterialSettings;

typedef struct
{
  LightType  type;
  GckVector3 position;
  GckVector3 direction;
  GckRGB     color;
  gdouble    intensity;
} LightSettings;

typedef struct {

  gint32 drawable_id;
  gint32 bumpmap_id;
  gint32 envmap_id;

  /* Render variables */
  /* ================ */

  GckVector3       viewpoint;
  GckVector3       planenormal;
  LightSettings    lightsource;
  MaterialSettings material;
  MaterialSettings ref_material;

  gdouble pixel_treshold;
  gdouble bumpmax,bumpmin;
/*  gdouble wave_cx,wave_cy;
  gdouble wave_lx,wave_ly;
  gdouble wave_amp,wave_ph; */
  gint    max_depth;
  gint    bumpmaptype;
/*  gint    bumptype; */

  /* Flags */
  /* ===== */
  
  gint antialiasing;
  gint create_new_image;
  gint transparent_background;
  gint tooltips_enabled;
  gint bump_mapped;
  gint env_mapped;
  gint ref_mapped;
  gint bumpstretch;
  gint previewquality;

  /* Misc */
  /* ==== */
  
  gdouble preview_zoom_factor;

} LightingValues;

/* Externally visible variables */
/* ============================ */

extern LightingValues mapvals;
extern GckRGB         background;

#endif
