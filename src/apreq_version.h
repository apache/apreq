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

#ifndef APREQ_VERSION_H
#define APREQ_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apreq.h"
#include "apr_version.h"

/**
 * @file apreq_version.h
 * @brief Versioning API for libapreq
 *
 * There are several different mechanisms for accessing the version. There
 * is a string form, and a set of numbers; in addition, there are constants
 * which can be compiled into your application, and you can query the library
 * being used for its actual version.
 *
 * Note that it is possible for an application to detect that it has been
 * compiled against a different version of libapreq by use of the compile-time
 * constants and the use of the run-time query function.
 *
 * libapreq version numbering follows the guidelines specified in:
 *
 *     http://apr.apache.org/versioning.html
 */

/* The numeric compile-time version constants. These constants are the
 * authoritative version numbers for libapreq. 
 */

/** major version 
 * Major API changes that could cause compatibility problems for older
 * programs such as structure size changes.  No binary compatibility is
 * possible across a change in the major version.
 */
#define APREQ_MAJOR_VERSION       2

/** 
 * Minor API changes that do not cause binary compatibility problems.
 * Should be reset to 0 when upgrading APREQ_MAJOR_VERSION
 */
#define APREQ_MINOR_VERSION       0

/** patch level */
#define APREQ_PATCH_VERSION       1

/** 
 *  This symbol is defined for internal, "development" copies of libapreq.
 *  This symbol will be #undef'd for releases. 
 */
#define APREQ_IS_DEV_VERSION


/** The formatted string of libapreq's version */
#define APREQ_VERSION_STRING \
     APR_STRINGIFY(APREQ_MAJOR_VERSION) "." \
     APR_STRINGIFY(APREQ_MINOR_VERSION) "." \
     APR_STRINGIFY(APREQ_PATCH_VERSION) \
     APREQ_IS_DEV_STRING

/**
 * Return libapreq's version information information in a numeric form.
 *
 *  @param pvsn Pointer to a version structure for returning the version
 *              information.
 */
APREQ_DECLARE(void) apreq_version(apr_version_t *pvsn);

/** Return libapreq's version information as a string. */
APREQ_DECLARE(const char *) apreq_version_string(void);


/** Internal: string form of the "is dev" flag */
#ifdef APREQ_IS_DEV_VERSION
#define APREQ_IS_DEV_STRING "-dev"
#else
#define APREQ_IS_DEV_STRING ""
#endif

#ifdef __cplusplus
}
#endif

#endif /* APREQ_VERSION_H */
