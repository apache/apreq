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

#include "apreq.h"
#include "apreq_env.h"
#include "apr_strings.h"
#include "apr_lib.h"
#include "apr_env.h"
#include "apr_file_io.h"


extern const apreq_env_t cgi_module;
static const apreq_env_t *apreq_env = &cgi_module;

extern void apreq_parser_initialize(void);


APREQ_DECLARE(const apreq_env_t *) apreq_env_module(const apreq_env_t *mod)
{
    apreq_parser_initialize();
    if (mod != NULL) {
        const apreq_env_t *old_mod = apreq_env;
        apreq_env = mod;
        return old_mod;
    }
    return apreq_env;
}


APREQ_DECLARE_NONSTD(void) apreq_log(const char *file, int line,
                                     int level, apr_status_t status,
                                     void *env, const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    apreq_env->log(file,line,level,status,env,fmt,vp);
    va_end(vp);
}

APREQ_DECLARE(apr_pool_t *) apreq_env_pool(void *env)
{
    return apreq_env->pool(env);
}

APREQ_DECLARE(apr_bucket_alloc_t *) apreq_env_bucket_alloc(void *env)
{
    return apreq_env->bucket_alloc(env);
}

APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(void *env, apreq_jar_t *jar)
{
    return apreq_env->jar(env,jar);
}

APREQ_DECLARE(apreq_request_t *) apreq_env_request(void *env,
                                                   apreq_request_t *req)
{
    return apreq_env->request(env,req);
}

APREQ_DECLARE(const char *) apreq_env_query_string(void *env)
{
    return apreq_env->query_string(env);
}

APREQ_DECLARE(const char *) apreq_env_header_in(void *env, const char *name)
{
    return apreq_env->header_in(env, name);
}

APREQ_DECLARE(apr_status_t)apreq_env_header_out(void *env, 
                                                const char *name,
                                                char *val)
{
    return apreq_env->header_out(env,name,val);
}

APREQ_DECLARE(apr_status_t) apreq_env_read(void *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes)
{
    return apreq_env->read(env,block,bytes);
}

APREQ_DECLARE(const char *) apreq_env_temp_dir(void *env, const char *path)
{
    if (path != NULL)
        /* ensure path is a valid pointer during the entire request */
        path = apr_pstrdup(apreq_env_pool(env),path);

    return apreq_env->temp_dir(env,path);
}

APREQ_DECLARE(apr_off_t) apreq_env_max_body(void *env, apr_off_t bytes)
{
    return apreq_env->max_body(env,bytes);
}

APREQ_DECLARE(apr_ssize_t) apreq_env_max_brigade(void *env, apr_ssize_t bytes)
{
    return apreq_env->max_brigade(env,bytes);
}

/** @} */
