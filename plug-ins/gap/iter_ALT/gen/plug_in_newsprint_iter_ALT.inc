/* ----------------------------------------------------------------------
 * p_plug_in_newsprint_iter_ALT 
 * ----------------------------------------------------------------------
 */
gint p_plug_in_newsprint_iter_ALT(GRunModeType run_mode, gint32 total_steps, gdouble current_step, gint32 len_struct) 
{
    typedef struct t_plug_in_newsprint_Vals 
    {
      long      cell_width;
      long      colourspace;
      long      k_pullout;
      gdouble   gry_ang;
      long      gry_spotfn;
      gdouble   red_ang;
      long      red_spotfn;
      gdouble   grn_ang;
      long      grn_spotfn;
      gdouble   blu_ang;
      long      blu_spotfn;
      long      oversample;
    } t_plug_in_newsprint_Vals; 

    t_plug_in_newsprint_Vals  buf, *buf_from, *buf_to; 

    if(len_struct != sizeof(t_plug_in_newsprint_Vals)) 
    {
      fprintf(stderr, "ERROR: p_plug_in_newsprint_iter_ALT  stored Data missmatch in size %d != %d\n",   
                       (int)len_struct, sizeof(t_plug_in_newsprint_Vals) ); 
      return -1;  /* ERROR */ 
    }

    gimp_get_data("plug_in_newsprint_ITER_FROM", g_plugin_data_from); 
    gimp_get_data("plug_in_newsprint_ITER_TO",   g_plugin_data_to); 

    buf_from = (t_plug_in_newsprint_Vals *)&g_plugin_data_from[0]; 
    buf_to   = (t_plug_in_newsprint_Vals *)&g_plugin_data_to[0]; 
    memcpy(&buf, buf_from, sizeof(buf));

    p_delta_long(&buf.cell_width, buf_from->cell_width, buf_to->cell_width, total_steps, current_step);
    p_delta_long(&buf.colourspace, buf_from->colourspace, buf_to->colourspace, total_steps, current_step);
    p_delta_long(&buf.k_pullout, buf_from->k_pullout, buf_to->k_pullout, total_steps, current_step);
    p_delta_gdouble(&buf.gry_ang, buf_from->gry_ang, buf_to->gry_ang, total_steps, current_step);
    p_delta_long(&buf.gry_spotfn, buf_from->gry_spotfn, buf_to->gry_spotfn, total_steps, current_step);
    p_delta_gdouble(&buf.red_ang, buf_from->red_ang, buf_to->red_ang, total_steps, current_step);
    p_delta_long(&buf.red_spotfn, buf_from->red_spotfn, buf_to->red_spotfn, total_steps, current_step);
    p_delta_gdouble(&buf.grn_ang, buf_from->grn_ang, buf_to->grn_ang, total_steps, current_step);
    p_delta_long(&buf.grn_spotfn, buf_from->grn_spotfn, buf_to->grn_spotfn, total_steps, current_step);
    p_delta_gdouble(&buf.blu_ang, buf_from->blu_ang, buf_to->blu_ang, total_steps, current_step);
    p_delta_long(&buf.blu_spotfn, buf_from->blu_spotfn, buf_to->blu_spotfn, total_steps, current_step);
    p_delta_long(&buf.oversample, buf_from->oversample, buf_to->oversample, total_steps, current_step);

    gimp_set_data("plug_in_newsprint", &buf, sizeof(buf)); 

    return 0; /* OK */
}
