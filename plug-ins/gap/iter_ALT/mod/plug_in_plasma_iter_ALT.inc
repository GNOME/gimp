/* ----------------------------------------------------------------------
 * p_plug_in_plasma_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_plasma_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_plasma_Vals 
    {
      long      seed;
      gdouble   turbulence;
      gboolean timeseed;
    } t_plug_in_plasma_Vals; 

    t_plug_in_plasma_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_plasma_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_plasma_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_plasma_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_plasma_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_plasma_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_plasma_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_plasma_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.seed, buf_from->seed, buf_to->seed, total_steps, current_step);
    p_delta_gdouble(&buf.turbulence, buf_from->turbulence, buf_to->turbulence, total_steps, current_step);

    gimp_set_data("plug_in_plasma", &buf, sizeof(buf)); 

    return 0; /* OK */
}
