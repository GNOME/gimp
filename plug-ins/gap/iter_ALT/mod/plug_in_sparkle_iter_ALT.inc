/* ----------------------------------------------------------------------
 * p_plug_in_sparkle_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_sparkle_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_sparkle_Vals 
    {
      gdouble   lum_threshold;
      gdouble   flare_inten;
      gdouble   spike_len;
      gdouble   spike_pts;
      gdouble   spike_angle;
      gdouble   density;
      gdouble   opacity;
      gdouble   random_hue;
      gdouble   random_saturation;
      gint      preserve_luminosity;
      gint      invers;
      gint      border;
      gint      colortype;
    } t_plug_in_sparkle_Vals; 

    t_plug_in_sparkle_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_sparkle_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_sparkle_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_sparkle_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_sparkle_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_sparkle_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_sparkle_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_sparkle_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.lum_threshold, buf_from->lum_threshold, buf_to->lum_threshold, total_steps, current_step);
    p_delta_gdouble(&buf.flare_inten, buf_from->flare_inten, buf_to->flare_inten, total_steps, current_step);
    p_delta_gdouble(&buf.spike_len, buf_from->spike_len, buf_to->spike_len, total_steps, current_step);
    p_delta_gdouble(&buf.spike_pts, buf_from->spike_pts, buf_to->spike_pts, total_steps, current_step);
    p_delta_gdouble(&buf.spike_angle, buf_from->spike_angle, buf_to->spike_angle, total_steps, current_step);
    p_delta_gdouble(&buf.density, buf_from->density, buf_to->density, total_steps, current_step);
    p_delta_gdouble(&buf.opacity, buf_from->opacity, buf_to->opacity, total_steps, current_step);
    p_delta_gdouble(&buf.random_hue, buf_from->random_hue, buf_to->random_hue, total_steps, current_step);
    p_delta_gdouble(&buf.random_saturation, buf_from->random_saturation, buf_to->random_saturation, total_steps, current_step);
    p_delta_gint(&buf.preserve_luminosity, buf_from->preserve_luminosity, buf_to->preserve_luminosity, total_steps, current_step);
    p_delta_gint(&buf.invers, buf_from->invers, buf_to->invers, total_steps, current_step);
    p_delta_gint(&buf.border, buf_from->border, buf_to->border, total_steps, current_step);
    p_delta_gint(&buf.colortype, buf_from->colortype, buf_to->colortype, total_steps, current_step);

    gimp_set_data("plug_in_sparkle", &buf, sizeof(buf)); 

    return 0; /* OK */
}
