/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2003 The Apache Software Foundation.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution,
 *    if any, must include the following acknowledgment:
 *       "This product includes software developed by the
 *        Apache Software Foundation (http://www.apache.org/)."
 *    Alternately, this acknowledgment may appear in the software itself,
 *    if and wherever such third-party acknowledgments normally appear.
 *
 * 4. The names "Apache" and "Apache Software Foundation" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache",
 *    nor may "Apache" appear in their name, without prior written
 *    permission of the Apache Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE APACHE SOFTWARE FOUNDATION OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Software Foundation.  For more
 * information on the Apache Software Foundation, please see
 * <http://www.apache.org/>.
 */

#include "apreq.h"
#include "apreq_env.h"
#include "apreq_params.h"
#include "apreq_parsers.h"
#include "apreq_cookie.h"

#include "apr_lib.h"
#include "apr_strings.h"
#include <stdlib.h>
#include "test_apreq.h"

/* rigged environent for unit tests */

static const char env_name[] = "CGI";
#define CRLF "\015\012"

apr_bucket_brigade *bb;
apr_table_t *table;

APREQ_DECLARE(apr_pool_t *)apreq_env_pool(void *env)
{
    return p;
}

APREQ_DECLARE(const char *)apreq_env_header_in(void *env, const char *name)
{
    return env;
}

APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name, 
                                                char *value)
{    
    return printf("%s: %s" CRLF, name, value) > 0 ? APR_SUCCESS : APR_EGENERAL;
}

APREQ_DECLARE(const char *)apreq_env_query_string(void *env)
{
    return env;
}

APREQ_DECLARE(apreq_jar_t *)apreq_env_jar(void *env, apreq_jar_t *jar)
{
    return NULL;
}

APREQ_DECLARE(apreq_request_t *)apreq_env_request(void *env, 
                                                  apreq_request_t *req)
{
    return NULL;
}

static int loglevel = 10;
APREQ_DECLARE_LOG(apreq_log)
{
    va_list vp;
    if (level < loglevel)
        return;

    va_start(vp, fmt);
    fprintf(stderr, "[%s(%d)] %s\n", file, line, apr_pvsprintf(p,fmt,vp));
    va_end(vp);
}

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env, apr_read_type_e block,
                                           apr_off_t bytes)
{
    return APR_ENOTIMPL;
}
