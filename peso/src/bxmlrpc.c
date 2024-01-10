/**                                     
 * Author: Jan Fuchs <fuky@sunstel.asu.cas.cz>
 * $Date$
 * $Rev$                                
 * $URL$
 */

#include <stdio.h>
#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

static xmlrpc_env bxr_env;
static int bxr_is_client_init = 0;

int bxr_client_init(void)
{
    if (!bxr_is_client_init)
    {
        xmlrpc_env_init(&bxr_env);
        xmlrpc_client_init2(&bxr_env, 0, "peso", SVN_REV, NULL, 0);
        if (bxr_env.fault_occurred)
        {
            fprintf(stderr, "\nXML-RPC Fault: %s (%d)\n", bxr_env.fault_string,
                    bxr_env.fault_code);
            xmlrpc_env_clean(&bxr_env);
            return -1;
        }

        bxr_is_client_init = 1;
    }

    return 0;
}

int bxr_client_cleanup(void)
{
    if (bxr_is_client_init)
    {
        xmlrpc_env_clean(&bxr_env);
        xmlrpc_client_cleanup();
        bxr_is_client_init = 0;
    }

    return 0;
}
