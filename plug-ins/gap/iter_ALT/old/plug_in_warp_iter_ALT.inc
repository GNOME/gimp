/* ----------------------------------------------------------------------
 * p_plug_in_warp_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_warp_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_warp_Vals 
    {
      gdouble amount;
      gint    warp_map;
      gint    iter;
      gdouble dither;
      gdouble angle;
      gint    wrap_type;
      gint    mag_map;
      gint    mag_use;
      gint    substeps;
      gint    grad_map;
      gdouble grad_scale;
      gint    vector_map;
      gdouble vector_scale;
      gdouble vector_angle;
      
      
    } t_plug_in_warp_Vals; 

    t_plug_in_warp_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_warp_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_warp_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_warp_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_warp_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_warp_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_warp_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_warp_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.amount, buf_from->amount, buf_to->amount, total_steps, current_step);
    p_delta_gintdrawable(&buf.warp_map, buf_from->warp_map, buf_to->warp_map, total_steps, current_step);
    p_delta_gint(&buf.iter, buf_from->iter, buf_to->iter, total_steps, current_step);
    p_delta_gdouble(&buf.dither, buf_from->dither, buf_to->dither, total_steps, current_step);
    p_delta_gdouble(&buf.angle, buf_from->angle, buf_to->angle, total_steps, current_step);
    p_delta_gint(&buf.wrap_type, buf_from->wrap_type, buf_to->wrap_type, total_steps, current_step);
    p_delta_gintdrawable(&buf.mag_map, buf_from->mag_map, buf_to->mag_map, total_steps, current_step);
    p_delta_gint(&buf.mag_use, buf_from->mag_use, buf_to->mag_use, total_steps, current_step);
    p_delta_gint(&buf.substeps, buf_from->substeps, buf_to->substeps, total_steps, current_step);
    p_delta_gintdrawable(&buf.grad_map, buf_from->grad_map, buf_to->grad_map, total_steps, current_step);

    p_delta_gdouble(&buf.grad_scale, buf_from->grad_scale, buf_to->grad_scale, total_steps, current_step);
    p_delta_gintdrawable(&buf.vector_map, buf_from->vector_map, buf_to->vector_map, total_steps, current_step);

    p_delta_gdouble(&buf.vector_scale, buf_from->vector_scale, buf_to->vector_scale, total_steps, current_step);
    p_delta_gdouble(&buf.vector_angle, buf_from->vector_angle, buf_to->vector_angle, total_steps, current_step);
    
    gimp_set_data("plug_in_warp", &buf, sizeof(buf)); 

    return 0; /* OK */
}
