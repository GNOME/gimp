#ifndef __LIGHTING_MAIN_H__
#define __LIGHTING_MAIN_H__

/* Defines and stuff */
/* ================= */

#define TILE_CACHE_SIZE 16

/* Typedefs */
/* ======== */

typedef enum
{
  POINT_LIGHT,
  DIRECTIONAL_LIGHT,
  SPOT_LIGHT, 
  NO_LIGHT
} LightType;

enum
{
  LINEAR_MAP,
  LOGARITHMIC_MAP,
  SINUSOIDAL_MAP,
  SPHERICAL_MAP
};

enum
{
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
  LightType   type;
  GimpVector3 position;
  GimpVector3 direction;
  GckRGB      color;
  gdouble     intensity;
} LightSettings;

typedef struct
{
  gint32 drawable_id;
  gint32 bumpmap_id;
  gint32 envmap_id;

  /* Render variables */
  /* ================ */

  GimpVector3      viewpoint;
  GimpVector3      planenormal;
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

#endif  /* __LIGHTING_MAIN_H__ */
