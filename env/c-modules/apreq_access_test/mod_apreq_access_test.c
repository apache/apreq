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

#if CONFIG_FOR_HTTPD_TEST

<Location /apreq_access_test>
   TestAccess test
   SetHandler apreq_request_test
</Location>

#endif

#define APACHE_HTTPD_TEST_ACCESS_CHECKER apreq_access_checker
#define APACHE_HTTPD_TEST_COMMANDS       access_cmds
#define APACHE_HTTPD_TEST_PER_DIR_CREATE create_access_config 

#include "apache_httpd_test.h"
#include "apreq_params.h"
#include "apreq_env.h"
#include "httpd.h"
#include "apr_strings.h"

module AP_MODULE_DECLARE_DATA apreq_access_test_module;

struct access_test_cfg {
    apr_pool_t *pool;
    const char *param;
};

static const char *access_config(cmd_parms *cmd, void *dv, const char *arg)
{
    struct access_test_cfg *cfg = (struct access_test_cfg *)dv;
    cfg->param = apr_pstrdup(cfg->pool, arg);
    return NULL;
}

static const command_rec access_cmds[] =
{
    AP_INIT_TAKE1("TestAccess", access_config, NULL, OR_LIMIT, "'param'"),
    {NULL}
};

static void *create_access_config(apr_pool_t *p, char *dummy)
{
    struct access_test_cfg *cfg = apr_palloc(p, sizeof *cfg);
    cfg->pool = p;
    cfg->param = dummy;
    return cfg;
}

static int apreq_access_checker(request_rec *r)
{
    apreq_request_t *req = apreq_request(r, NULL);
    apreq_param_t *param;
    struct access_test_cfg *cfg = (struct access_test_cfg *)
        ap_get_module_config(r->per_dir_config, &apreq_access_test_module);

    if (!cfg || !cfg->param)
        return DECLINED;

    param = apreq_param(req, cfg->param);
    if (param) {
        apreq_log(APREQ_DEBUG 0, r, "%s => %s", cfg->param, param->v.data);
        return OK;
    }
    else {
        if (req->body)
            apreq_log(APREQ_DEBUG HTTP_FORBIDDEN, r, "%s not found in %d elts",
                      cfg->param, apr_table_elts(req->body)->nelts);
        return HTTP_FORBIDDEN;
    }
}

APACHE_HTTPD_TEST_MODULE(apreq_access_test);
