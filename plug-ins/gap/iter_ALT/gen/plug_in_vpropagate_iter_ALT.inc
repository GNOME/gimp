/* ----------------------------------------------------------------------
 * p_plug_in_vpropagate_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_vpropagate_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_vpropagate_Vals 
    {
      long      propagate_mode;
      long      propagating_channel;
      gdouble   propagating_rate;
      long      direction_mask;
      long      lower_limit;
      long      upper_limit;
    } t_plug_in_vpropagate_Vals; 

    t_plug_in_vpropagate_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_vpropagate_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_vpropagate_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_vpropagate_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_vpropagate_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_vpropagate_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_vpropagate_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_vpropagate_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.propagate_mode, buf_from->propagate_mode, buf_to->propagate_mode, total_steps, current_step);
    p_delta_long(&buf.propagating_channel, buf_from->propagating_channel, buf_to->propagating_channel, total_steps, current_step);
    p_delta_gdouble(&buf.propagating_rate, buf_from->propagating_rate, buf_to->propagating_rate, total_steps, current_step);
    p_delta_long(&buf.direction_mask, buf_from->direction_mask, buf_to->direction_mask, total_steps, current_step);
    p_delta_long(&buf.lower_limit, buf_from->lower_limit, buf_to->lower_limit, total_steps, current_step);
    p_delta_long(&buf.upper_limit, buf_from->upper_limit, buf_to->upper_limit, total_steps, current_step);

    gimp_set_data("plug_in_vpropagate", &buf, sizeof(buf)); 

    return 0; /* OK */
}
