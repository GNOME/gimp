/* ----------------------------------------------------------------------
 * p_plug_in_map_object_iter 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_map_object_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct 
    {
      t_GckVector3    viewpoint;
      t_GckVector3    firstaxis;
      t_GckVector3    secondaxis;
      t_GckVector3    normal;
      t_GckVector3    position;
      t_GckVector3    scale;
      t_LightSettings lightsource;

      t_MaterialSettings material;
      t_MaterialSettings refmaterial;

      t_MapType maptype;

      gint antialiasing;
      gint create_new_image;
      gint transparent_background;
      gint tiled;
      gint showgrid;
      gint tooltips_enabled;
      gint showcaps;

      glong preview_zoom_factor;

      gdouble alpha,beta,gamma;
      gdouble maxdepth;
      gdouble pixeltreshold;
      gdouble radius;
      
      gdouble cylinder_radius;
      gdouble cylinder_length;

      gint32 boxmap_id[6];
      gint32 cylindermap_id[2];

    } t_plug_in_map_object_Vals; 

    t_plug_in_map_object_Vals  buf, buf_from, buf_to;
    int                        l_idx;

    if(len_struct != sizeof(t_plug_in_map_object_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_map_object_iter  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_map_object_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_map_object_ITER_FROM", &buf_from); 
    gimp_get_data("plug_in_map_object_ITER_TO",   &buf_to); 
    memcpy(&buf, &buf_from, sizeof(buf));

    p_delta_GckVector3(&buf.viewpoint, &buf_from.viewpoint, &buf_to.viewpoint, total_steps, current_step);
    p_delta_GckVector3(&buf.firstaxis, &buf_from.firstaxis, &buf_to.firstaxis, total_steps, current_step);
    p_delta_GckVector3(&buf.secondaxis, &buf_from.secondaxis, &buf_to.secondaxis, total_steps, current_step);
    p_delta_GckVector3(&buf.normal, &buf_from.normal, &buf_to.normal, total_steps, current_step);
    p_delta_GckVector3(&buf.position, &buf_from.position, &buf_to.position, total_steps, current_step);
    p_delta_GckVector3(&buf.scale, &buf_from.scale, &buf_to.scale, total_steps, current_step);

    p_delta_LightSettings(&buf.lightsource, &buf_from.lightsource, &buf_to.lightsource, total_steps, current_step);
    p_delta_MaterialSettings(&buf.material, &buf_from.material, &buf_to.material, total_steps, current_step);
    p_delta_MaterialSettings(&buf.refmaterial, &buf_from.refmaterial, &buf_to.refmaterial, total_steps, current_step);

    /* MapType is not iterated */

    p_delta_gdouble(&buf.alpha, buf_from.alpha, buf_to.alpha, total_steps, current_step);
    p_delta_gdouble(&buf.beta, buf_from.beta, buf_to.beta, total_steps, current_step);
    p_delta_gdouble(&buf.gamma, buf_from.gamma, buf_to.gamma, total_steps, current_step);
    p_delta_gdouble(&buf.maxdepth, buf_from.maxdepth, buf_to.maxdepth, total_steps, current_step);
    p_delta_gdouble(&buf.pixeltreshold, buf_from.pixeltreshold, buf_to.pixeltreshold, total_steps, current_step);
    p_delta_gdouble(&buf.radius, buf_from.radius, buf_to.radius, total_steps, current_step);

    /* switches that are not iterated: tlied, showgrid, tooltips_enabled, showcaps */

    p_delta_gdouble(&buf.cylinder_radius, buf_from.cylinder_radius, buf_to.cylinder_radius, total_steps, current_step);
    p_delta_gdouble(&buf.cylinder_length, buf_from.cylinder_length, buf_to.cylinder_length, total_steps, current_step);


    /* animate the used maps (drawing_ids) */
    
    for(l_idx = 0; l_idx < 6; l_idx++)
    {
      p_delta_drawable(&buf.boxmap_id[l_idx], buf_from.boxmap_id[l_idx], buf_to.boxmap_id[l_idx], total_steps, current_step);
    }

    for(l_idx = 0; l_idx < 2; l_idx++)
    {
      p_delta_drawable(&buf.cylindermap_id[l_idx], buf_from.cylindermap_id[l_idx], buf_to.cylindermap_id[l_idx], total_steps, current_step);
    }
    
    buf.create_new_image           = FALSE;
    buf.antialiasing               =    buf_from.antialiasing;
    buf.transparent_background     =    buf_from.transparent_background;
    buf.tiled                      =    buf_from.tiled;

    gimp_set_data("plug_in_map_object", &buf, sizeof(buf)); 

    return 0; /* OK */
}
