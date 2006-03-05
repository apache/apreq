dnl modules enabled in this directory by default
dnl APACHE_MODULE(name, helptext[, objects[, structname[, default[, config]]]])

APACHE_MODPATH_INIT(apreq)

APACHE_MODULE(apreq2, apreq POST-data filter, filter.lo handle.lo, apreq, yes)

APACHE_MODPATH_FINISH
