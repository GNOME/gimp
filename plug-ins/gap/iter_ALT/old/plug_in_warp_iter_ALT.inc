/* ----------------------------------------------------------------------
 * p_plug_in_warp_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_warp_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_warp_Vals 
    {
      gdouble   amount;
      gdouble   angle;
      gdouble   iter;
      gdouble   dither;
      gint      warp_map;
      gint      wrap_type;
      gint      mag_map;
      gint      mag_option;
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
    p_delta_gdouble(&buf.angle, buf_from->angle, buf_to->angle, total_steps, current_step);
    p_delta_gdouble(&buf.iter, buf_from->iter, buf_to->iter, total_steps, current_step);
    p_delta_gdouble(&buf.dither, buf_from->dither, buf_to->dither, total_steps, current_step);
    p_delta_gint(&buf.wrap_type, buf_from->wrap_type, buf_to->wrap_type, total_steps, current_step);
    p_delta_gint(&buf.mag_option, buf_from->mag_option, buf_to->mag_option, total_steps, current_step);

    gimp_set_data("plug_in_warp", &buf, sizeof(buf)); 

    return 0; /* OK */
}
