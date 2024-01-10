/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 */

#ifndef __FCE_H
#define __FCE_H

#define PESO_MAX_NUMBER_OF_EXPOSE 999

int fce_make_fits_prefix(char instrument, char *p_prefix, int prefix_len);
int fce_make_filename(char *p_path, char *p_prefix, char *p_filename,
        int filename_len);
int fce_isfile_exist(char *p_filename);
int fce_isdir_rwxu(char *p_path);
int fce_isallow_fitshdr_c(int c);
char *fce_strstrip(char *p_str);

#endif
