static apr_pool_t *apreq_xs_cgi_global_pool;

MODULE = APR::Request::CGI   PACKAGE = APR::Request::CGI

BOOT:
    apr_pool_create(&apreq_xs_cgi_global_pool, NULL);
    apreq_initialize(apreq_xs_cgi_global_pool);
