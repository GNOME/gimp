#define FLAME_NXFORMS 6
#define FLAME_NVARS   7

typedef struct {
   double var[FLAME_NVARS];   /* normalized interp coefs between variations */
   double c[3][2];      /* the coefs to the affine part of the function */
   double density;      /* prob is this function is chosen. 0 - 1 */
   double color;        /* color coord for this function. 0 - 1 */
} t_flame_xform;

typedef double t_flame_clrmap[256][3];

typedef struct {
   t_flame_xform xform[FLAME_NXFORMS];
   t_flame_clrmap cmap;
   double time;
   int  cmap_index;
   double brightness;           /* 1.0 = normal */
   double contrast;             /* 1.0 = normal */
   double gamma;
   int  width, height;          /* of the final image */
   int  spatial_oversample;
   double center[2];             /* camera center */
   double zoom;                  /* effects ppu and sample density */
   double pixels_per_unit;       /* and scale */
   double spatial_filter_radius; /* variance of gaussian */
   double sample_density;        /* samples per pixel (not bucket) */
   /* in order to motion blur more accurately we compute the logs of the 
      sample density many times and average the results.  we interplate
      only this many times. */
   int nbatches;
   /* this much color resolution.  but making it too high induces clipping */
   int white_level;
   int cmap_inter; /* if this is true, then color map interpolates one entry
		      at a time with a bright edge */
   double pulse[2][2]; /* [i][0]=magnitute [i][1]=frequency */
   double wiggle[2][2]; /* frequency is /minute, assuming 30 frames/s */
} t_flame_control_point;

static void p_delta_flame_xform(t_flame_xform *val, t_flame_xform *val_from, t_flame_xform *val_to, gint32 total_steps, gdouble current_step)
{
    int l_x, l_y;

    for(l_x=0; l_x < FLAME_NVARS; l_x++)
    {
      p_delta_gdouble(&val->var[l_x], val_from->var[l_x], val_to->var[l_x], total_steps, current_step);
    }

    for(l_x=0; l_x < 3; l_x++)
    {
      for(l_y=0; l_y < 2; l_y++)
      {
         p_delta_gdouble(&val->c[l_x][l_y], val_from->c[l_x][l_y], val_to->c[l_x][l_y], total_steps, current_step);
      }
    }
    p_delta_gdouble(&val->density, val_from->density, val_to->density, total_steps, current_step);
    p_delta_gdouble(&val->color, val_from->color, val_to->color, total_steps, current_step);
}

static void p_delta_flame_control_point(t_flame_control_point *val, t_flame_control_point *val_from, t_flame_control_point *val_to, gint32 total_steps, gdouble current_step)
{
    int l_x, l_y;

    for(l_x=0; l_x < FLAME_NXFORMS; l_x++)
    {
      p_delta_flame_xform(&val->xform[l_x], &val_from->xform[l_x], &val_to->xform[l_x], total_steps, current_step);
    }
    
    /* p_delta_flame_clrmap(&val->cmap, &val_from->cmap, &val_to->cmap, total_steps, current_step); */
    for(l_x=0; l_x < 256; l_x++)
    {
      for(l_y=0; l_y < 3; l_y++)
      {
         p_delta_gdouble(&val->cmap[l_x][l_y], val_from->cmap[l_x][l_y], val_to->cmap[l_x][l_y], total_steps, current_step);
      }
    }

    p_delta_gdouble(&val->time, val_from->time, val_to->time, total_steps, current_step);
    p_delta_int(&val->cmap_index, val_from->cmap_index, val_to->cmap_index, total_steps, current_step);
    p_delta_gdouble(&val->brightness, val_from->brightness, val_to->brightness, total_steps, current_step);
    p_delta_gdouble(&val->contrast, val_from->contrast, val_to->contrast, total_steps, current_step);
    p_delta_gdouble(&val->gamma, val_from->gamma, val_to->gamma, total_steps, current_step);
    p_delta_int(&val->width, val_from->width, val_to->width, total_steps, current_step);
    p_delta_int(&val->height, val_from->height, val_to->height, total_steps, current_step);
    p_delta_int(&val->spatial_oversample, val_from->spatial_oversample, val_to->spatial_oversample, total_steps, current_step);

    for(l_x=0; l_x < 2; l_x++)
    {
      p_delta_gdouble(&val->center[l_x], val_from->center[l_x], val_to->center[l_x], total_steps, current_step);
    }

    p_delta_gdouble(&val->zoom, val_from->zoom, val_to->zoom, total_steps, current_step);
    p_delta_gdouble(&val->pixels_per_unit, val_from->pixels_per_unit, val_to->pixels_per_unit, total_steps, current_step);
    p_delta_gdouble(&val->spatial_filter_radius, val_from->spatial_filter_radius, val_to->spatial_filter_radius, total_steps, current_step);
    p_delta_gdouble(&val->sample_density, val_from->sample_density, val_to->sample_density, total_steps, current_step);
    p_delta_int(&val->nbatches, val_from->nbatches, val_to->nbatches, total_steps, current_step);
    p_delta_int(&val->white_level, val_from->white_level, val_to->white_level, total_steps, current_step);
    p_delta_int(&val->cmap_inter, val_from->cmap_inter, val_to->cmap_inter, total_steps, current_step);
    
    for(l_x=0; l_x < 2; l_x++)
    {
      for(l_y=0; l_y < 2; l_y++)
      {
         p_delta_gdouble(&val->pulse[l_x][l_y], val_from->pulse[l_x][l_y], val_to->pulse[l_x][l_y], total_steps, current_step);
         p_delta_gdouble(&val->wiggle[l_x][l_y], val_from->wiggle[l_x][l_y], val_to->wiggle[l_x][l_y], total_steps, current_step);        
      }
    }

}


/* ----------------------------------------------------------------------
 * p_plug_in_flame_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_flame_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_flame_Vals 
    {
      int randomize;  /* superseded */
      int variation;
      gint32 cmap_drawable;
      t_flame_control_point cp;
    } t_plug_in_flame_Vals; 

    t_plug_in_flame_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_flame_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_flame_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_flame_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_flame_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_flame_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_flame_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_flame_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_int(&buf.randomize, buf_from->randomize, buf_to->randomize, total_steps, current_step);
    p_delta_int(&buf.variation, buf_from->variation, buf_to->variation, total_steps, current_step);

    p_delta_flame_control_point(&buf.cp, &buf_from->cp, &buf_to->cp, total_steps, current_step);

    gimp_set_data("plug_in_flame", &buf, sizeof(buf)); 

    return 0; /* OK */
}
