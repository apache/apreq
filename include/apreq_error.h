/*
**  Copyright 2003-2004  The Apache Software Foundation
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
*/

#ifndef APREQ_ERROR_H
#define APREQ_ERROR_H

#include "apr_errno.h"
#include "apreq.h"

#ifdef  __cplusplus
 extern "C" {
#endif 

APREQ_DECLARE(char *)
apreq_strerror(apr_status_t s, char *buf, apr_size_t bufsize);

/**
 * Beginning work on error-codes ...
 *
 *
 */
#ifndef APR_EBADARG
#define APR_EBADARG                APR_BADARG   /* XXX: don't use APR_BADARG */
#endif

/* 0's: generic error status codes */
#define APREQ_ERROR_GENERAL        APR_OS_START_USERERR
#define APREQ_ERROR_INTERRUPT      APREQ_ERROR_GENERAL + 1

/* 10's: malformed input */
#define APREQ_ERROR_NODATA         APREQ_ERROR_GENERAL + 10
#define APREQ_ERROR_BADSEQ         APREQ_ERROR_GENERAL + 11
#define APREQ_ERROR_BADCHAR        APREQ_ERROR_GENERAL + 12
#define APREQ_ERROR_BADTOKEN       APREQ_ERROR_GENERAL + 13
#define APREQ_ERROR_NOTOKEN        APREQ_ERROR_GENERAL + 14
#define APREQ_ERROR_BADATTR        APREQ_ERROR_GENERAL + 15
#define APREQ_ERROR_BADHEADER      APREQ_ERROR_GENERAL + 16
#define APREQ_ERROR_NOHEADER       APREQ_ERROR_GENERAL + 17

/* 20's: misconfiguration */
#define APREQ_ERROR_CONFLICT       APREQ_ERROR_GENERAL + 20 /*XXX this sucks*/
#define APREQ_ERROR_NOPARSER       APREQ_ERROR_GENERAL + 21
#define APREQ_ERROR_TAINTED        APREQ_ERROR_GENERAL + 22


/* 30's: limit violations */
#define APREQ_ERROR_OVERLIMIT      APREQ_ERROR_GENERAL + 30
#define APREQ_ERROR_UNDERLIMIT     APREQ_ERROR_GENERAL + 31

/* unsafe output */


static APR_INLINE
unsigned char apreq_status_is_error(apr_status_t s) {
    switch (s) {
    case APR_SUCCESS:
    case APR_INCOMPLETE:
    case APR_EINIT:
    case APREQ_ERROR_NODATA:

        return 0;
    default:
        return 1;
    }
}




#ifdef __cplusplus
 }
#endif

#endif /* APREQ_ERROR_H */
