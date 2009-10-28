#include "isds_priv.h"
#include "utils.h"


/* Get ISDS dmstatus info from ISDS @response xml node set.
 * Be ware that different request familes return differently encoded status
 * (e.g. dmStatus, dbStatus)
 * @response is SOAP body response 
 * @code is status code of the response
 * @message is automatically allocated status message*/
_hidden isds_error isds_response_dmstatus(xmlNodePtr response,
        unsigned int *code, xmlChar **message) {
    isds_error err = IE_SUCCESS;

    if (!response) {
        err = IE_INVAL;
        goto leave;
    }


leave:

    return err;
}

