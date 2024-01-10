/**
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$
 * $URL$
 *
 * The code for CCD700 control come from ccdserver r2:
 *
 *     Jaroslav Honsa <honsa@sunstel.asu.cas.cz> created:
 *
 *         https://stelweb.asu.cas.cz/svn/ccdserver/
 *
 *     from:
 *
 *         https://stelweb.asu.cas.cz/svn/bias/
 */

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/io.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <fitsio.h>

#include "modules.h"
#include "mod_ccd.h"
#include "mod_ccd_bilbo.h"
#include "header.h"
#include "thread.h"

PESO_T peso;

static BIL_BROCAM_INFO_T bil_brocam_info;
static int bil_pc_board_base;
static char ccd_readout_speeds[PESO_READOUT_SPEEDS_MAX + 1];
static char gan_ccd_gains[PESO_GAINS_MAX + 1];
static unsigned short int *imstart;
static unsigned short int *imbuf = NULL;

/* TODO: sdilet tuto funkci ve vsech modulech a i s daemonem exposed */
__attribute__((format(printf,1,2)))
static int save_sys_error(const char *p_fmt, ...)
{
    va_list ap;
    int len;

    va_start(ap, p_fmt);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);
    len = strlen(peso.msg);
    snprintf(peso.msg + len, CCD_MSG_MAX - len, " %i: %s", errno,
            strerror(errno));

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    va_end(ap);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, peso.msg);
    return 0;
}

__attribute__((format(printf,1,2)))
static int ccd_save_error(const char *p_fmt, ...)
{
    va_list ap;

    va_start(ap, p_fmt);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);
    vsnprintf(peso.msg, CCD_MSG_MAX, p_fmt, ap);

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

    va_end(ap);

    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_ERROR, peso.msg);
    return 0;
}

// Nastavi adresu pro cteni/zapis (rozdeli ji na dolni a horni cast)
static void bil_set_addr(int addr) {
    // write addr. bit 16 -> 31
    outw(addr >> 16, BIL_PCPTH);

    // write addr. bit 0 -> 15
    outw(addr & 0xffff, BIL_PCPTL);
}

static int bil_testram(void)
{
    int x;
    unsigned short Q;

    // disable transmitter, disable receiver
    outw(0, BIL_TXE);

    for (x = 0x8000000; x >= 0; x -= 0x100000)
    {
        bil_set_addr(x / 2);
        Q = x >> 16;

        // write to RAM
        outw(Q, BIL_RWD);
    }

    for (x = 0; x <= 0x8000000; x += 0x100000)
    {
        bil_set_addr(x / 2);

        // read from RAM
        Q = inw(BIL_RWD);

        if (Q != (x >> 16))
        {
            Q = x >> 10;
            goto out1;
        }
    }

    out1:

    // disable transmitter, enable receiver
    outw(0x2, BIL_TXE);

    return (Q * 1024);
}

static int bil_test_for_camera(void)
{
    time_t t1, t2;
    unsigned short int snabel_A = '@';

    // enable transmitter, enable receiver
    outw(0x3, BIL_TXE);
    usleep(10000); // 10ms

    // send data
    outw(snabel_A, BIL_TXDATA);
    time(&t1);

    // transmitter not empty
    while (inw(BIL_TXCOM) & 1)
    {
        time(&t2);
        if ((t2 - t1) > 5)
        {
            // disable transmitter, enable receiver
            outw(0x2, BIL_TXE);
            return -1;
        }

        usleep(10000); // 10ms
    }

    // disable transmitter, enable receiver
    outw(0x2, BIL_TXE);

    // serial input addr bit 16 -> 31
    outw(0, BIL_SERPT);

    // read and write increment disable
    outw(0, BIL_INCSET);

    return 0;
}

// waited max. 2s for empty transmitter
static int bil_wait(void)
{
    time_t t1;
    time_t t2;

    time(&t1);

    // transmitter not empty
    while (inw(BIL_TXCOM) & 1)
    {
        time(&t2);
        if ((t2 - t1) > 2)
        {
            return -1;
        }
    }

    return 0;
}

// Posila zavinacove prikazy kamere. Musi se to posilat po bytech (znacich).
static void bil_camera_send(char *p_command)
{
    int n = 0;

    // enable transmitter, enable receiver
    outw(0x3, BIL_TXE);
    usleep(10000); // 10ms

    while (p_command[n] != '\0')
    {
        // wait for tx port ready
        if (bil_wait() == 0)
        {
            return;
        }

        // send data
        outw(p_command[n++], BIL_TXDATA);
    }
    bil_wait();

    // send '\r'
    outw(0x0d, BIL_TXDATA);
    bil_wait();

    // disable transmitter, enable receiver
    outw(0x2, BIL_TXE);
}

// Zapise do read_to_buf length bytu pocinajic adresou start_addr data
// z pameti na PC-karte. Pri cteni probiha kontrola spravnosti, pri chybe
// se cteni opakuje.
int bil_pc_read(unsigned short *p_read_to_buf, int start_addr, int length)
{
    unsigned short int rbuf1[BIL_BUFZ];
    unsigned short int *p_rbuf2;
    int i, n, m, addr, ecnt;

    n = length / BIL_BUFZ;
    m = length % BIL_BUFZ;
    addr = start_addr;
    p_rbuf2 = p_read_to_buf;

    // read increment enable, write increment disable
    outw(1, BIL_INCSET);

    if (n > 0)
    {
        for (i = 0; i < n; i++)
        {
            ecnt = 0;

            bil_set_addr(addr);
            insw(BIL_RWD, p_rbuf2, BIL_BUFZ); // read from RAM

            bil_set_addr(addr);
            insw(BIL_RWD, rbuf1, BIL_BUFZ);

            while (memcmp((char *) rbuf1, (char *) p_rbuf2, BIL_BUFZ * sizeof(short int)) != 0)
            {
                bil_set_addr(addr);
                insw(BIL_RWD, p_rbuf2, BIL_BUFZ);

                bil_set_addr(addr);
                insw(BIL_RWD, rbuf1, BIL_BUFZ);

                if (++ecnt >= 1)
                {
                    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN,
                            "Read error addr:%08X len:%d n=%d m=%d %dx", addr,
                            BIL_BUFZ, n, m, ecnt);
                }

                if (ecnt >= 5)
                {
                    ccd_save_error("PC-board reading error BIL_BUFZ!!!");
                    return -1;
                }
            }

            p_rbuf2 += BIL_BUFZ;
            addr += BIL_BUFZ;
        }
    }

    if (m > 0)
    {
        ecnt = 0;
        bil_set_addr(addr);
        insw(BIL_RWD, p_rbuf2, m);

        bil_set_addr(addr);
        insw(BIL_RWD, rbuf1, m);

        while (memcmp((char *) rbuf1, (char *) p_rbuf2, m * sizeof(short int)) != 0)
        {
            bil_set_addr(addr);
            insw(BIL_RWD, p_rbuf2, m);

            bil_set_addr(addr);
            insw(BIL_RWD, rbuf1, m);

            // TODO: odstranit potencionalni nekonecny cyklus
            if ((length != 102) && (length > 2))
            {/*Copak to asi znamena?*/
                if (++ecnt >= 1)
                {
                    log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN,
                            "Read error addr:%08X len:%d n=%d m=%d %dx", addr,
                            BIL_BUFZ, n, m, ecnt);
                }
            }

            if (ecnt >= 5)
            {
                ccd_save_error("PC-board reading error m!!!");
                return -1;
            }
        }
    }

    // read and write increment enable
    outw(0, BIL_INCSET);

    return 0;
}

// Vycte 2*102 byte z kamery. V ni jsou dulezita nastaveni kamery. Naplni
// tim strukturu brocam_info. Vicebajtova cisla je treba obratit, protoze
// jsou otocene.
int bil_get_camera_info()
{
    int ecnt = 0;
    char *s;

    s = (char *) &bil_brocam_info;

    unsigned short int *is = (unsigned short int *) s;
    unsigned short int dummy;
    unsigned short int check_buf[102];

    bil_pc_read((unsigned short int *) &bil_brocam_info, 0, 102);
    bil_pc_read(check_buf, 0, 102);

    memcpy(bil_brocam_info.cam_id, bil_brocam_info._cam_id, 4);

    while ((memcmp((char *) check_buf, (char *) &bil_brocam_info, 2 * 102) != 0)
            || (strcmp(bil_brocam_info.cam_id, "BROR") != 0))
    {
        /* 10x zkusi precist cam_id kamery, kdyz se to nezdari, skonci */
        /* Jeste zkontroluje, jestli je to spravny typ kamery */
        ecnt++;
        bil_pc_read((unsigned short int *) &bil_brocam_info, 0, 102);
        bil_pc_read(check_buf, 0, 102);

//      if ( ecnt > 0) {
//        fprintf(stderr, "Necte to spravne hlavicku!\n");
//      }

        if (ecnt > 10)
        {
            ccd_save_error("Camera header FATAL error! Lost connection.");
            return -1;
        }

        memcpy(bil_brocam_info.cam_id, bil_brocam_info._cam_id, 4);
        usleep(10000);
    }

    memcpy(bil_brocam_info.ccd_type, bil_brocam_info._ccd_type, 16);

    swab((void *) &s[22], (void *) &s[22], 64);
    swab((void *) &s[128], (void *) &s[128], 2);
    swab((void *) &s[88], (void *) &s[88], 16);

    dummy = is[34];
    is[34] = is[35];
    is[35] = dummy;

    dummy = is[36];
    is[36] = is[37];
    is[37] = dummy;

    dummy = is[38];
    is[38] = is[39];
    is[39] = dummy;

    dummy = is[40];
    is[40] = is[41];
    is[41] = dummy;

    dummy = is[44];
    is[44] = is[45];
    is[45] = dummy;

    dummy = is[46];
    is[46] = is[47];
    is[47] = dummy;

    dummy = is[48];
    is[48] = is[49];
    is[49] = dummy;

    dummy = is[50];
    is[50] = is[51];
    is[51] = dummy;

    return 0;
}

static void bil_pc_write(unsigned short *write_from_buf, int start_addr, int length)
{
    // write increment enable
    outw(2, BIL_INCSET);

    bil_set_addr(start_addr);
    outsw(BIL_RWD, write_from_buf, length);

    // read and write increment disable
    outw(0, BIL_INCSET);
}

//static void bil_clear_pcboard(void)
//{
//    unsigned char buf[65536];
//    int cnt;
//
//    fprintf(stderr, "Clearing PC-board RAM\n");
//    memset(buf, 0, 65536);
//
//    for (cnt = 0; cnt < BIL_MEMZ / 65536; cnt++)
//    {
//        bil_pc_write((unsigned short int *) buf, cnt * 65536, 32768);
//    }
//}

// TODO: nebezpecna fce, zapisuje na nesmyslne misto v pameti, proverit v Biasu
//void bil_clear_info_pcboard(void)
//{
//    unsigned short xsb[152];
//    int x;
//
//    for (x = 0; x < 152; x++)
//    {
//        xsb[x] = 0;
//    }
//
//    bil_pc_write(xsb, 0, 152);
//}

void bil_set_mppmode(int yesno)
{
    if ((yesno) && (bil_brocam_info.mpp_mode != 0))
    {
        bil_camera_send("@TMPP");
    }
    else if ((!yesno) && (bil_brocam_info.mpp_mode == 0))
    {
        bil_camera_send("@TMPP");
    }
}

void bil_set_autoshutter(int yesno)
{
    if ((yesno) && (bil_brocam_info.auto_shutter == 0))
    {
        bil_camera_send("@TASH");
    }
    else if ((!yesno) && (bil_brocam_info.auto_shutter != 0))
    {
        bil_camera_send("@TASH");
    }
}

void bil_set_autoclear(int yesno)
{
    if ((yesno) && (bil_brocam_info.auto_clear == 0))
    {
        bil_camera_send("@TACL");
    }
    else if ((!yesno) && (bil_brocam_info.auto_clear != 0))
    {
        bil_camera_send("@TACL");
    }
}

void bil_set_autoreadout(int yesno)
{
    if ((yesno) && (bil_brocam_info.auto_readout == 0))
    {
        bil_camera_send("@TARD");
    }
    else if ((!yesno) && (bil_brocam_info.auto_readout != 0))
    {
        bil_camera_send("@TARD");
    }
}

static void bil_subtract_32768(unsigned short int * sbuf, size_t count) {
    int n;
    unsigned short int *sp;

    sp = sbuf;
    for ( n = 0; n < count; n++ ) {
        *sp++ -= 32768;
    }
}

// Nastavi ruzne parametry kamery dulezite pro vycitani dat
static void bil_set_common()
{
    char sb[BIL_SB_MAX];
    char amplmode = 0;
    int n;

    for (n = 0; n < 8; n++)
    {
        snprintf(sb, BIL_SB_MAX, "@BLLO%02d+%05d", n, BiasLevelLow[n]);
        bil_camera_send(sb);

        snprintf(sb, BIL_SB_MAX, "@BLHI%02d+%05d", n, BiasLevelHigh[n]);
        bil_camera_send(sb);
    }

    usleep(40000); // 40ms

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    // bil_brocam_info.totx + xover
    snprintf(sb, BIL_SB_MAX, "@XTOT%04d", bil_brocam_info.totx);
    bil_camera_send(sb);

    // bil_brocam_info.toty + yover
    snprintf(sb, BIL_SB_MAX, "@YTOT%04d", bil_brocam_info.toty);
    bil_camera_send(sb);

    usleep(40000);

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    snprintf(sb, BIL_SB_MAX, "@READ%02d", amplmode);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@XBEG%04d", 0);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@YBEG%04d", 0);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@XNUM%04d", 2030);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@YNUM%04d", 800);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@XBIN%04d", 1);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@YBIN%04d", 1);
    bil_camera_send(sb);

    snprintf(sb, BIL_SB_MAX, "@MODE%d", 0);
    bil_camera_send(sb);

    usleep(60000);

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    bil_camera_send("@FRES");

    /* Nevim jestli je tohle potreba:
     * Asi ano, jinak je brocam_info.imx nula. Nevim proc.
     */
    //bil_clear_info_pcboard();
    usleep(60000);

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    /* Tohle nastaveni probehne jenom kdyz zname spravne hodnoty z vystupu
     * bil_get_camera_info() */
    bil_set_mppmode(TRUE);
    bil_set_autoshutter(TRUE);
    bil_set_autoclear(TRUE);
    bil_set_autoreadout(TRUE);
}

int ccd_init(void)
{
    int ramsize;

    memset(&ccd_readout_speeds, '\0', PESO_READOUT_SPEEDS_MAX + 1);
    memset(gan_ccd_gains, '\0', PESO_GAINS_MAX + 1);
    memset(&bil_brocam_info, 0, sizeof(BIL_BROCAM_INFO_T));

    peso.state = CCD_STATE_UNKNOWN_E;
    peso.imgtype = CCD_IMGTYPE_UNKNOWN_E;
    peso.archive = 0;
    peso.require_temp = peso.p_exposed_cfg->ccd.temp;
    peso.x1 = peso.p_exposed_cfg->ccd.x1;
    peso.x2 = peso.p_exposed_cfg->ccd.x2;
    peso.xb = peso.p_exposed_cfg->ccd.xb;
    peso.y1 = peso.p_exposed_cfg->ccd.y1;
    peso.y2 = peso.p_exposed_cfg->ccd.y2;
    peso.yb = peso.p_exposed_cfg->ccd.yb;
    bil_pc_board_base = peso.p_exposed_cfg->ccd_bilbo.pc_board_base;

    // changes the I/O privilege level of the calling process
    if (iopl(3) == -1) {
        save_sys_error("iopl(3) failed:");
        return -1;
    }

    // disable transmitter, enable receiver
    outw(0x2, BIL_TXE);

    // serial input addr bit 16 -> 31
    outw(0, BIL_SERPT);

    // read increment disable, write increment disable
    outw(0, BIL_INCSET);

    ramsize = bil_testram() / 1024;
    if ( ramsize == 0 ) {
        ccd_save_error("Error: PC-card has no memory");
        return -1;
    }

    if (bil_test_for_camera() == -1) {
        ccd_save_error("Error: No camera present");
        return -1;
    }

    bil_set_common();

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    if (bil_brocam_info.imx == 0)
    {
        ccd_save_error("bil_brocam_info.imx == 0");
        return -1;
    }

    peso.actual_temp = bil_brocam_info.ccd_temp / 100.0;

    return 0;
}

// TODO: implementovat
int ccd_uninit(void)
{
    return 0;
}

int ccd_expose_init(void)
{
    peso_set_int(&peso.readout_time, peso.p_exposed_cfg->ccd.readout_time);

    /* LOCK */
    //pthread_mutex_lock(peso.p_global_mutex);

    //temp = peso.require_temp * 100;
    //shutter = (peso.shutter) ? OPEN_PRE_TRIGGER : OPEN_NEVER;

    //pthread_mutex_unlock(peso.p_global_mutex);
    /* UNLOCK */

//    if (p_raw_data != NULL)
//    {
//        free(p_raw_data);
//        p_raw_data = NULL;
//    }
//
//    if ((p_raw_data = (unsigned short *) malloc(size)) == NULL)
//    {
//        save_sys_error("Error: malloc():");
//        return -1;
//    }

//    bil_pc_prepare(((BIL_HEADER / 2) - 2) + peso.p_exposed_cfg->ccd.x2,
//        peso.p_exposed_cfg->ccd.x2, peso.p_exposed_cfg->ccd.y2, 0x06800021);

    return 0;
}

int ccd_expose_start(void)
{
    char sb[BIL_SB_MAX];

    snprintf(sb, BIL_SB_MAX, "@TIME%08d", (peso.exptime * 1000 + 1));

    bil_camera_send(sb);
    bil_camera_send("@SINT");
    usleep(60000); // 60ms

    return 0;
}

void bil_pc_abort(void)
{
    // enable transmitter, enable receiver
    outw(0x3, BIL_TXE);
    usleep(10000); // 10ms

    // send data
    outw(0x0d, BIL_TXDATA);

    bil_wait();

    // disable transmitter, enable receiver
    outw(0x2, BIL_TXE);
}

void bil_pc_prepare(int start_addr, int lcnt, int bcnt, int value)
{
    int x, y;
    int check, ecnt;

    y = start_addr + (lcnt * bcnt);

    for (x = start_addr; x <= y; x += lcnt)
    {
        for (ecnt = 0; ecnt <= 5; ++ecnt)
        {
            bil_pc_write((unsigned short *) &value, x, 2);

            usleep(10000); // 10ms

            bil_pc_read((unsigned short *) &check, x, 2);
            if (check == value)
            {
                break;
            }

            if (ecnt >= 5)
            {
                // TODO: zalogovat
                //jprintf("Prepare error %08X:  %08X %08X\n", x, value, check);
                return;
            }
        }
    }
}

// Nasledujici fce nedava smysl, protoze fce Bias/com2.c/add_tint(),
// resi to same pouze odeslanim "@TIME%08d"
//static void ccd_readout(void)
//{
//    bil_set_autoreadout(TRUE);
//    bil_set_autoclear(FALSE);
//    bil_set_autoshutter(FALSE);
//
//    bil_camera_send("@TIME00000001");
//
//    bil_pc_prepare(((BIL_HEADER / 2) - 2) + peso.p_exposed_cfg->ccd.x2,
//            peso.p_exposed_cfg->ccd.x2, peso.p_exposed_cfg->ccd.y2, 0x06800021);
//
//    bil_camera_send("@TIME00000001");
//    bil_camera_send("@SINT");
//}

static void ccd_addtime(int addtime)
{
    char sb[BIL_SB_MAX];
    int t = addtime * 1000;

    // Postrada smysl, to same zaridi "@TIME%08d"
//    if ((t + bil_brocam_info.expleft) <= 0)
//    {
//        ccd_readout();
//        return;
//    }

    t += bil_brocam_info.exptime;
    if (t <= 0) {
        t = 1;
    }

    snprintf(sb, BIL_SB_MAX, "@TIME%08d", t);
    bil_camera_send(sb);
}

int ccd_expose(void)
{
    time_t actual_time;
    int addtime;

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }
    //printf("expleft:        %8d\n", bil_brocam_info.expleft);

    if (peso.exptime_update != 0)
    {
        char sb[BIL_SB_MAX];
        int exptime_update;

        /* LOCK */
        pthread_mutex_lock(peso.p_global_mutex);

        if (peso.exptime_update == -1)
        {
            peso.exptime = CCD_EXPTIME_MAX;
        }
        else if (peso.exptime == 0)
        {
            peso.readout = 1;
        }
        else
        {
            peso.exptime = peso.exptime_update;
        }

        exptime_update = peso.exptime;
        peso.exptime_update = 0;

        pthread_mutex_unlock(peso.p_global_mutex);
        /* UNLOCK */

        if (!peso.readout) {
            snprintf(sb, BIL_SB_MAX, "@TIME%08d", exptime_update * 1000);
            bil_camera_send(sb);
        }
    }

    if (peso.abort != 0)
    {
        bil_pc_abort();
        bil_set_autoreadout(FALSE);
        bil_camera_send("@TIME00000001");

        peso_set_int(&peso.abort, 2);
        return 0;
    }
    else if (peso.readout)
    {
        //ccd_readout();
        bil_camera_send("@TIME00000001");
        return 0;
    }

    (void) time(&actual_time);

    peso_set_int(&peso.elapsed_time, actual_time - peso.start_exposure_time);

    if (peso.elapsed_time >= peso.exptime)
    {
        return 0;
    }

    /* exposing */
    return 1;
}

int ccd_readout(void)
{
    time_t actual_time;
    //short status = 0;
    //unsigned long bytes;

    (void) time(&actual_time);
    peso_set_int(&peso.elapsed_time, actual_time - peso.stop_exposure_time);

    if (peso.elapsed_time >= 35)
    {
        return 0;
    }

    return 1;
}

int ccd_save_raw_image()
{
    int x, y, offset, items, imlen, wlen;
    int x1, x2;
    unsigned short int *xp, *xpend, dummy;
    int Xover, Yover, xover = 0, yover = 0;
    FILE *fw;
    int startline = 0;
    int endline = 800;

    if ((imbuf = (unsigned short int *) malloc(
            (size_t) (bil_brocam_info.totx * bil_brocam_info.toty * 2))) == NULL)
    {
        ccd_save_error("malloc failed");
        return (-1);
    }

    offset = startline * bil_brocam_info.imx; /* Pocet bytu od zacatku buferu */
    items = bil_brocam_info.imx * (endline - startline); /* Kolik pixlu se precte */
    imlen = 2 * items;
    imstart = imbuf + offset;

    bil_pc_read(imstart, BIL_HEADER / 2 + offset, items);

    Xover = xover - (bil_brocam_info.beginx - 1);
    if (Xover < 0)
    {
        Xover = 0;
    }
    else
    {
        Xover /= bil_brocam_info.binx;
    }
    Yover = (bil_brocam_info.beginy - 1 + bil_brocam_info.imy)
            - (bil_brocam_info.toty - yover);
    if (Yover < 0)
    {
        Yover = 0;
    }
    else
    {
        Yover /= bil_brocam_info.biny;
    }

    x1 = bil_brocam_info.imx - (xover / bil_brocam_info.binx);
    x2 = x1 / 2;

    for (y = startline; y < endline; y++)
    {
        xp = imbuf + (y * bil_brocam_info.imx);
        xpend = xp + x1 - 1;
        for (x = 0; x < x2; x++)
        {
            dummy = *(xp);
            *(xp++) = *(xpend);
            *(xpend--) = dummy;
        }
    }

    //bil_subtract_32768(imstart, items);
    //swab((void *) imstart, (void *) imstart, imlen);

    /* peso.raw_image not lock */
    if ((fw = fopen(peso.raw_image, "w")) == NULL)
    {
        return -1;
    }

    if ((wlen = fwrite(imstart, 1, imlen, fw)) != imlen)
    {
        ccd_save_error("Error writing data to %s", peso.raw_image);
    }

    if (fclose(fw) == EOF)
    {
        return -1;
    }

    return 0;
}

int ccd_save_fits_file(fitsfile *p_fits, int *p_fits_status)
{
    //int index = 0;
    long fpixel = 1;
    long nelements = peso.x2 * peso.y2;

    if (fits_write_img(p_fits, TUSHORT, fpixel, nelements, imstart,
            p_fits_status))
    {
        return -1;
    }

    return 0;
}

int ccd_expose_end(void)
{
    return 0;
}

int ccd_expose_uninit(void)
{
    //free(p_raw_data);
    //p_raw_data = NULL;

    return 0;
}

// TODO: otestovat
int ccd_get_temp(double *p_temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    if (bil_get_camera_info() < 0)
    {
        log4c_category_log(peso.p_logcat, LOG4C_PRIORITY_WARN, "Bad reading camera info!");
    }

    *p_temp = bil_brocam_info.ccd_temp / 100.0;

    //strncpy(peso.msg, "ccd_get_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

int ccd_set_temp(double temp)
{
    if (mod_ccd_check_state(peso.state) == -1)
    {
        return -1;
    }

    //strncpy(peso.msg, "ccd_set_temp() failure", CCD_MSG_MAX);
    //return -1;

    return 0;
}

// Nezamykat, vola se z klienta
int ccd_set_readout_speed(char *p_speed)
{
    return 0;
}

const char *ccd_get_readout_speed(void)
{
    return bilbo_speed_str2enum[peso.readout_speed].str;
}

const char *ccd_get_readout_speeds(void)
{
    int i;
    int size = 0;

    if (ccd_readout_speeds[0] == '\0')
    {
        for (i = 0; i < BILBO_SPEED_MAX_E; ++i)
        {
            strncat(ccd_readout_speeds, bilbo_speed_str2enum[i].str,
                    PESO_READOUT_SPEEDS_MAX - size - 1);
            strcat(ccd_readout_speeds, ";");
            size += strlen(bilbo_speed_str2enum[i].str);
        }
    }

    return ccd_readout_speeds;
}

// Nezamykat, vola se z klienta
int ccd_set_gain(char *p_gain)
{
    return 0;
}

const char *ccd_get_gain(void)
{
    return "default";
}

const char *ccd_get_gains(void)
{
    return "default";
}
