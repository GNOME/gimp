/* ----------------------------------------------------------------------
 * p_plug_in_video_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_video_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_video_Vals 
    {
      long      pattern_number;
      long      additive;
      long      rotated;
    } t_plug_in_video_Vals; 

    t_plug_in_video_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_video_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_video_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_video_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_video_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_video_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_video_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_video_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.pattern_number, buf_from->pattern_number, buf_to->pattern_number, total_steps, current_step);
    p_delta_long(&buf.additive, buf_from->additive, buf_to->additive, total_steps, current_step);
    p_delta_long(&buf.rotated, buf_from->rotated, buf_to->rotated, total_steps, current_step);

    gimp_set_data("plug_in_video", &buf, sizeof(buf)); 

    return 0; /* OK */
}
