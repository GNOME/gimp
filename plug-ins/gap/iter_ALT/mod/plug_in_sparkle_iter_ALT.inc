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

    gimp_set_data("plug_in_sparkle", &buf, sizeof(buf)); 

    return 0; /* OK */
}
