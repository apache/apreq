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


APREQ_DECLARE_NONSTD(void) apreq_log(const char *file, int line,
                                     int level, apr_status_t status,
                                     apreq_env_handle_t *env,
                                     const char *fmt, ...)
{
    va_list vp;
    va_start(vp, fmt);
    env->module->log(file,line,level,status,env,fmt,vp);
    va_end(vp);
}

APREQ_DECLARE(apr_pool_t *) apreq_env_pool(apreq_env_handle_t *env)
{
    return env->module->pool(env);
}

APREQ_DECLARE(apr_bucket_alloc_t *) apreq_env_bucket_alloc(apreq_env_handle_t *env)
{
    return env->module->bucket_alloc(env);
}

APREQ_DECLARE(apreq_jar_t *) apreq_env_jar(apreq_env_handle_t *env,
                                           apreq_jar_t *jar)
{
    return env->module->jar(env,jar);
}

APREQ_DECLARE(apreq_request_t *) apreq_env_request(apreq_env_handle_t *env,
                                                   apreq_request_t *req)
{
    return env->module->request(env,req);
}

APREQ_DECLARE(const char *) apreq_env_query_string(apreq_env_handle_t *env)
{
    return env->module->query_string(env);
}

APREQ_DECLARE(const char *) apreq_env_header_in(apreq_env_handle_t *env,
                                                const char *name)
{
    return env->module->header_in(env, name);
}

APREQ_DECLARE(apr_status_t)apreq_env_header_out(apreq_env_handle_t *env,
                                                const char *name,
                                                char *val)
{
    return env->module->header_out(env,name,val);
}

APREQ_DECLARE(apr_status_t) apreq_env_read(apreq_env_handle_t *env,
                                           apr_read_type_e block,
                                           apr_off_t bytes)
{
    return env->module->read(env,block,bytes);
}

APREQ_DECLARE(const char *) apreq_env_temp_dir(apreq_env_handle_t *env,
                                               const char *path)
{
    if (path != NULL)
        /* ensure path is a valid pointer during the entire request */
        path = apr_pstrdup(apreq_env_pool(env),path);

    return env->module->temp_dir(env,path);
}

APREQ_DECLARE(apr_off_t) apreq_env_max_body(apreq_env_handle_t *env,
                                            apr_off_t bytes)
{
    return env->module->max_body(env,bytes);
}

APREQ_DECLARE(apr_ssize_t) apreq_env_max_brigade(apreq_env_handle_t *env,
                                                 apr_ssize_t bytes)
{
    return env->module->max_brigade(env,bytes);
}

/** @} */
