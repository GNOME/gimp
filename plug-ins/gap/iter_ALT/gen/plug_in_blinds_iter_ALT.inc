/* ----------------------------------------------------------------------
 * p_plug_in_blinds_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_blinds_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_blinds_Vals 
    {
      long      angle_dsp;
      long      number_of_segments;
      long      orientation;
      long      backgndg_trans;
    } t_plug_in_blinds_Vals; 

    t_plug_in_blinds_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_blinds_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_blinds_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_blinds_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_blinds_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_blinds_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_blinds_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_blinds_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.angle_dsp, buf_from->angle_dsp, buf_to->angle_dsp, total_steps, current_step);
    p_delta_long(&buf.number_of_segments, buf_from->number_of_segments, buf_to->number_of_segments, total_steps, current_step);
    p_delta_long(&buf.orientation, buf_from->orientation, buf_to->orientation, total_steps, current_step);
    p_delta_long(&buf.backgndg_trans, buf_from->backgndg_trans, buf_to->backgndg_trans, total_steps, current_step);

    gimp_set_data("plug_in_blinds", &buf, sizeof(buf)); 

    return 0; /* OK */
}
