/* ----------------------------------------------------------------------
 * p_plug_in_lighting_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_lighting_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_lighting_Vals 
    {
      gint32    drawable_id;
      gint32    bumpdrawable;
      gint32    envdrawable;

      /* Render variables */
      /* ================ */
  
      t_GckVector3       viewpoint;
      t_GckVector3       planenormal;
      t_LightSettings    lightsource;
      t_MaterialSettings material;
      t_MaterialSettings refmaterial;

      gdouble pixel_treshold;
      gdouble bumpmax;
      gdouble bumpmin;

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
      
    } t_plug_in_lighting_Vals; 

    t_plug_in_lighting_Vals  buf, buf_from, buf_to; 

    if(len_struct != sizeof(t_plug_in_lighting_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_lighting_iter  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_lighting_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_lighting_ITER_FROM", &buf_from); 
    gimp_get_data("plug_in_lighting_ITER_TO",   &buf_to); 
    memcpy(&buf, &buf_from, sizeof(buf));


    p_delta_GckVector3(&buf.viewpoint, &buf_from.viewpoint, &buf_to.viewpoint, total_steps, current_step);
    p_delta_GckVector3(&buf.planenormal, &buf_from.planenormal, &buf_to.planenormal, total_steps, current_step);
    p_delta_LightSettings(&buf.lightsource, &buf_from.lightsource, &buf_to.lightsource, total_steps, current_step);
    p_delta_MaterialSettings(&buf.material, &buf_from.material, &buf_to.material, total_steps, current_step);
    p_delta_MaterialSettings(&buf.refmaterial, &buf_from.refmaterial, &buf_to.refmaterial, total_steps, current_step);

    
    p_delta_gdouble(&buf.pixel_treshold, buf_from.pixel_treshold, buf_to.pixel_treshold, total_steps, current_step);
    p_delta_gdouble(&buf.bumpmax, buf_from.bumpmax, buf_to.bumpmax, total_steps, current_step);
    p_delta_gdouble(&buf.bumpmin, buf_from.bumpmin, buf_to.bumpmin, total_steps, current_step);
/*
    p_delta_gdouble(&buf.wave_cx, buf_from.wave_cx, buf_to.wave_cx, total_steps, current_step);
    p_delta_gdouble(&buf.wave_cy, buf_from.wave_cy, buf_to.wave_cy, total_steps, current_step);
    p_delta_gdouble(&buf.wave_lx, buf_from.wave_lx, buf_to.wave_lx, total_steps, current_step);
    p_delta_gdouble(&buf.wave_ly, buf_from.wave_ly, buf_to.wave_ly, total_steps, current_step);
 */
    p_delta_gint(&buf.max_depth, buf_from.max_depth, buf_to.max_depth, total_steps, current_step);
    p_delta_gint(&buf.bumpmaptype, buf_from.bumpmaptype, buf_to.bumpmaptype, total_steps, current_step);

/*    p_delta_gint(&buf.bumptype, buf_from.bumptype, buf_to.bumptype, total_steps, current_step);
 */
    
    p_delta_gint(&buf.antialiasing, buf_from.antialiasing, buf_to.antialiasing, total_steps, current_step);
    p_delta_gint(&buf.transparent_background, buf_from.transparent_background, buf_to.transparent_background, total_steps, current_step);
    p_delta_gint(&buf.bump_mapped, buf_from.bump_mapped, buf_to.bump_mapped, total_steps, current_step);
    p_delta_gint(&buf.ref_mapped, buf_from.ref_mapped, buf_to.ref_mapped, total_steps, current_step);
    p_delta_gint(&buf.bumpstretch, buf_from.bumpstretch, buf_to.bumpstretch, total_steps, current_step);
    p_delta_gint(&buf.previewquality, buf_from.previewquality, buf_to.previewquality, total_steps, current_step);

    gimp_set_data("plug_in_lighting", &buf, sizeof(buf)); 

    return 0; /* OK */
}
