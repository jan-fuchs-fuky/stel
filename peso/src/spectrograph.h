/*                                       
 *   Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *   $Date$
 *   $Rev$                                          
 *   $URL$                                          
 */

#ifndef __SPECTROGRAPH_H
#define __SPECTROGRAPH_H

#define SGH_SERVER_URL_MAX 1023
#define SGH_ERR_MSG_MAX    1023
#define SGH_ANSWER_MAX     1023
#define SGH_COMMAND_MAX    1023

typedef struct
{
    int flat;
    int comp;
    int dichroic_mirror;
    int spectral_filter;
    int collimator;
    int oes_collimator;
    int star_calib;
    int coude_oes;
    int exp_freq;
    int exp_sum;
    int grating_position;
    int focus_1400;
    int focus_700;
    int focus_oes;
    int coude_temp;
    int oes_temp;
    int oes_iodine_cell;
    int correction_plate_700;
    int correction_plate_400;
} SGH_INFO_T;

const char *sgh_get_err_msg(void);
int sgh_init(void);
int sgh_uninit(void);
int sgh_spectrograph_execute(char *p_command, char *p_answer);
int sgh_spectrograph_info(SGH_INFO_T *p_sgh_info);
int sgh_gratang2gratpos(double gratang);
double sgh_gratpos2gratang(int gratpos);
void sgh_gratpos2gratang_str(int gratpos, char *p_gratang, int gratang_len);
float sgh_temp2human(int temp_raw);

#endif
