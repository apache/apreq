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

#ifndef APREQ_APACHE2_H
#define APREQ_APACHE2_H

#include "apreq.h"
#include <httpd.h>

#ifdef  __cplusplus
 extern "C" {
#endif

/**
 * Create an apreq handle which communicates with an Apache 2
 * request_rec.
 */
APREQ_DECLARE(apreq_env_handle_t*) apreq_handle_apache2(request_rec *r);

#ifdef __cplusplus
 }
#endif

#endif
