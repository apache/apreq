dnl modules enabled in this directory by default

dnl Authentication (authn), Access, and Authorization (authz)

dnl APACHE_MODULE(name, helptext[, objects[, structname[, default[, config]]]])

APACHE_MODPATH_INIT(apreq)

dnl Authentication modules; modules checking a username and password against a
dnl file, database, or other similar magic.
dnl
APACHE_MODULE(apreq, apreq POST-data filter, filter.lo handle.lo, , yes)

APACHE_MODPATH_FINISH
