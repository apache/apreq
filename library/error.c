/* Copyright 2000-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "apreq_error.h"
#include "apr_strings.h"

/*
 * stuffbuffer - like apr_cpystrn() but returns the address of the
 * dest buffer instead of the address of the terminating '\0'
 */
static char *stuffbuffer(char *buf, apr_size_t bufsize, const char *s)
{
    apr_cpystrn(buf,s,bufsize);
    return buf;
}

static const char *apreq_error_string(apr_status_t statcode)
{
    switch (statcode) {

    case APREQ_ERROR_GENERAL:
        return "Internal apreq error";

    case APREQ_ERROR_NODATA:
        return "Missing input data";

    case APREQ_ERROR_BADSEQ:
        return "Invalid byte sequence";

    case APREQ_ERROR_BADCHAR:
        return "Invalid character";

    case APREQ_ERROR_BADTOKEN:
        return "Invalid token";

    case APREQ_ERROR_NOTOKEN:
        return "Required token not found";

    case APREQ_ERROR_BADATTR:
        return "Unrecognized attribute";

    case APREQ_ERROR_BADHEADER:
        return "Malformed header string";

    case APREQ_ERROR_NOPARSER:
        return "Parser not found";

    case APREQ_ERROR_TAINTED:
        return "Attempt to perform unsafe action with tainted data";

    default:
        return "Error string not yet specified by apreq";
    }
}


APREQ_DECLARE(char *) apreq_strerror(apr_status_t statcode, char *buf,
                                 apr_size_t bufsize)
{
    if (statcode < APR_OS_START_USERERR || statcode >= APR_OS_START_EAIERR)
        return apr_strerror(statcode, buf, bufsize);
    return stuffbuffer(buf, bufsize, apreq_error_string(statcode));
}

