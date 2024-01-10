/*!
 *  \file    th_connect.h
 *  \author  Jan Fuchs <fuky@sunstel.asu.cas.cz>
 *  $Date$
 *  $Rev$
 *
 *  \brief
 */

#ifndef __TH_CONNECT_H
#define __TH_CONNECT_H

#include "peso.h"

#define TH_CONNECT_MAX 64
#define TH_CONNECT_CMD_MAX 8
#define TH_CONNECT_MSG_MAX 1024

int th_connect_id_cmd;
int th_connect_id_msg;

char th_connect_cmd[TH_CONNECT_MAX][TH_CONNECT_CMD_MAX][PESO_CMD_MAX];
char th_connect_msg[TH_CONNECT_MAX][TH_CONNECT_MSG_MAX][PESO_MSG_MAX];

void *th_connect(void *arg);

#endif
