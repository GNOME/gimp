/* ----------------------------------------------------------------------
 * p_plug_in_randomize_hurl_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_randomize_hurl_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_randomize_hurl_Vals 
    {
      gdouble   rndm_pct;
      gdouble   rndm_rcount;
      long      seed_type;
      long      rndm_seed;
    } t_plug_in_randomize_hurl_Vals; 

    t_plug_in_randomize_hurl_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_randomize_hurl_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_randomize_hurl_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_randomize_hurl_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_randomize_hurl_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_randomize_hurl_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_randomize_hurl_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_randomize_hurl_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_gdouble(&buf.rndm_pct, buf_from->rndm_pct, buf_to->rndm_pct, total_steps, current_step);
    p_delta_gdouble(&buf.rndm_rcount, buf_from->rndm_rcount, buf_to->rndm_rcount, total_steps, current_step);
    p_delta_long(&buf.seed_type, buf_from->seed_type, buf_to->seed_type, total_steps, current_step);
    p_delta_long(&buf.rndm_seed, buf_from->rndm_seed, buf_to->rndm_seed, total_steps, current_step);

    gimp_set_data("plug_in_randomize_hurl", &buf, sizeof(buf)); 

    return 0; /* OK */
}
