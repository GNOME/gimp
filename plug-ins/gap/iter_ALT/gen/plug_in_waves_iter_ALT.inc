/* ----------------------------------------------------------------------
 * p_plug_in_waves_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_waves_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_waves_Vals 
    {
      gdouble   amplitude;
      gdouble   phase;
      gdouble   wavelength;
      long      type;
      long      reflective;
    } t_plug_in_waves_Vals; 

    t_plug_in_waves_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_waves_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_waves_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_waves_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_waves_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_waves_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_waves_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_waves_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.amplitude, buf_from->amplitude, buf_to->amplitude, total_steps, current_step);
    p_delta_gdouble(&buf.phase, buf_from->phase, buf_to->phase, total_steps, current_step);
    p_delta_gdouble(&buf.wavelength, buf_from->wavelength, buf_to->wavelength, total_steps, current_step);
    p_delta_long(&buf.type, buf_from->type, buf_to->type, total_steps, current_step);
    p_delta_long(&buf.reflective, buf_from->reflective, buf_to->reflective, total_steps, current_step);

    gimp_set_data("plug_in_waves", &buf, sizeof(buf)); 

    return 0; /* OK */
}
