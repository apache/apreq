/* ====================================================================
 * The Apache Software License, Version 1.1
 *
 * Copyright (c) 2000-2002 The Apache Software Foundation.  All rights
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

#ifndef APREQ_TABLES_H
#define APREQ_TABLES_H

#include "apr_tables.h"
#include "apreq.h"
#include <stddef.h>
#if APR_HAVE_STDARG_H
#include <stdarg.h>     /* for va_list */
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Define the structures used by the APR general-purpose library.
 */

/**
 * @file apreq_tables.h
 * @brief APR Table library
 */
/**
 * @defgroup APREQ_Table Table routines
 * @ingroup APREQ
 *
 * Memory allocation stuff, like pools, arrays, and tables.  Pools
 * and tables are opaque structures to applications, but arrays are
 * published.
 * @{
 */
/** the table abstract data type */
typedef struct apreq_table_t apreq_table_t;

/**
 * Make a new table
 * @param p The pool to allocate the pool out of
 * @param nelts The number of elements in the initial table.
 * @return The new table.
 * @warning This table can only store text data
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_make(apr_pool_t *p, int nelts);

/**
 * Create a new table and copy another table into it
 * @param p The pool to allocate the new table out of
 * @param t The table to copy
 * @return A copy of the table passed in
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_copy(apr_pool_t *p,
                                            const apreq_table_t *t);


APREQ_DECLARE(apr_table_t *)apreq_table_export(apreq_table_t *t, 
                                             apr_pool_t *p);
APREQ_DECLARE(apreq_table_t *)apreq_table_import(apr_pool_t *p, 
                                               apr_table_t *s, 
                                               unsigned f);


/**
 * Delete all of the elements from a table
 * @param t The table to clear
 */
APREQ_DECLARE(void) apreq_table_clear(apreq_table_t *t);

APREQ_DECLARE(int) apreq_table_nelts(const apreq_table_t *t);
APREQ_DECLARE(int) apreq_is_empty_table(const apreq_table_t *t);

APREQ_DECLARE(apreq_value_copy_t *) apreq_table_copier(apreq_table_t *t, 
                                           apreq_value_copy_t *c);
APREQ_DECLARE(apreq_value_merge_t *) apreq_table_merger(apreq_table_t *t, 
                                           apreq_value_merge_t *m);

APREQ_DECLARE(void) apreq_table_cache_lookups(apreq_table_t *t, 
                                              const int on);

APREQ_DECLARE(void) apreq_table_balance(apreq_table_t *t, const int on);
APREQ_DECLARE(void) apreq_table_normalize(apreq_table_t *t);

APREQ_DECLARE(int) apreq_table_ghosts(apreq_table_t *t);
APREQ_DECLARE(int) apreq_table_exorcise(apreq_table_t *t);


/**
 * Get the value associated with a given key from the table.  After this call,
 * The data is still in the table
 * @param t The table to search for the key
 * @param key The key to search for
 * @return The value associated with the key
 */
APREQ_DECLARE(const char*) apreq_table_get(apreq_table_t *t, 
                                           const char *key);


APREQ_DECLARE(apr_array_header_t *) apreq_table_keys(const 
                                                     apreq_table_t *t,
                                                     apr_pool_t *p);

APREQ_DECLARE(apr_array_header_t *) apreq_table_values(apreq_table_t *t,
                                                       const char *key,
                                                       apr_pool_t *p);


/**
 * Add a key/value pair to a table, if another element already exists with the
 * same key, this will over-write the old data.
 * @param t The table to add the data to.
 * @param key The key fo use
 * @param val The value to add
 * @remark When adding data, this function makes a copy of both the key and the
 *         value.
 */
APREQ_DECLARE(void) apreq_table_set(apreq_table_t *t, const char *key, 
                                  const char *val);

APREQ_DECLARE(void) apreq_table_setb(apreq_table_t *t, const char *key, 
                                     int klen, const char *val, int vlen);

/**
 * Add a key/value pair to a table, if another element already exists with the
 * same key, this will over-write the old data.
 * @param t The table to add the data to.
 * @param key The key to use
 * @param val The value to add
 * @warning When adding data, this function does not make a copy of the key or 
 *          the value, so care should be taken to ensure that the values will 
 *          not change after they have been added..
 */
APREQ_DECLARE(void) apreq_table_setn(apreq_table_t *t, const char *key, 
                                     const char *val);

APREQ_DECLARE(void) apreq_table_setv(apreq_table_t *t, const char *key, 
                                     apreq_value_t *val);


/**
 * Remove data from the table
 * @param t The table to remove data from
 * @param key The key of the data being removed
 */
APREQ_DECLARE(void) apreq_table_unset(apreq_table_t *t, const char *key);

/**
 * Add data to a table by merging the value with data that has already been 
 * stored
 * @param t The table to search for the data
 * @param key The key to merge data for
 * @param val The data to add
 * @remark If the key is not found, then this function acts like apr_table_add
 */

APREQ_DECLARE(void) apreq_table_merge(apreq_table_t *t, const char *key, 
                                      const char *val);

APREQ_DECLARE(void) apreq_table_mergeb(apreq_table_t *t,
                                       const char *key, int klen,
                                       const char *val, int vlen);

/**
 * Add data to a table by merging the value with data that has already been 
 * stored
 * @param t The table to search for the data
 * @param key The key to merge data for
 * @param val The data to add
 * @remark If the key is not found, then this function acts like apr_table_addn
 */
APREQ_DECLARE(void) apreq_table_mergen(apreq_table_t *t, const char *key,
                                     const char *val);

APREQ_DECLARE(void) apreq_table_mergev(apreq_table_t *t,
                                       const char *key,
                                       apreq_value_t *val);

/**
 * Add data to a table, regardless of whether there is another element with the
 * same key.
 * @param t The table to add to
 * @param key The key to use
 * @param val The value to add.
 * @remark When adding data, this function makes a copy of both the key and the
 *         value.
 */
APREQ_DECLARE(void) apreq_table_add(apreq_table_t *t, 
                                    const char *key, const char *val);
APREQ_DECLARE(void) apreq_table_addb(apreq_table_t *t, 
                                     const char *key, int klen, 
                                     const char *val, int vlen);

/**
 * Add data to a table, regardless of whether there is another element with the
 * same key.
 * @param t The table to add to
 * @param key The key to use
 * @param val The value to add.
 * @remark When adding data, this function does not make a copy of the key or the
 *         value, so care should be taken to ensure that the values will not 
 *         change after they have been added..
 */
APREQ_DECLARE(void) apreq_table_addn(apreq_table_t *t, const char *key, 
                                     const char *val);
APREQ_DECLARE(void) apreq_table_addv(apreq_table_t *t, 
                                     const char *key, 
                                     apreq_value_t *val);

APREQ_DECLARE(void) apreq_table_cat(apreq_table_t *t, const apreq_table_t *s);
/**
 * Merge two tables into one new table
 * @param p The pool to use for the new table
 * @param over The first table to put in the new table
 * @param under The table to add at the end of the new table
 * @return A new table containing all of the data from the two passed in
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_overlay(apr_pool_t *p,
                                                   const apreq_table_t *over,
                                                   const apreq_table_t *under);

/**
 * For each element in table b, either use setn or mergen to add the data
 * to table a.  Which method is used is determined by the flags passed in.
 * @param a The table to add the data to.
 * @param b The table to iterate over, adding its data to table a
 * @param flags How to add the table to table a.  One of:
 *          APR_OVERLAP_TABLES_SET        Use apr_table_setn
 *          APR_OVERLAP_TABLES_MERGE      Use apr_table_mergen
 * @remark  This function is highly optimized, and uses less memory and CPU cycles
 *          than a function that just loops through table b calling other functions.
 */
/**
 *<PRE>
 * Conceptually, apr_table_overlap does this:
 *
 *  apr_array_header_t *barr = apr_table_elts(b);
 *  apr_table_entry_t *belt = (apr_table_entry_t *)barr->elts;
 *  int i;
 *
 *  for (i = 0; i < barr->nelts; ++i) {
 *      if (flags & APR_OVERLAP_TABLES_MERGE) {
 *          apr_table_mergen(a, belt[i].key, belt[i].val);
 *      }
 *      else {
 *          apr_table_setn(a, belt[i].key, belt[i].val);
 *      }
 *  }
 *
 *  Except that it is more efficient (less space and cpu-time) especially
 *  when b has many elements.
 *
 *  Notice the assumptions on the keys and values in b -- they must be
 *  in an ancestor of a's pool.  In practice b and a are usually from
 *  the same pool.
 * </PRE>
 */

APREQ_DECLARE(void) apreq_table_overlap(apreq_table_t *a, 
                                        const apreq_table_t *b,
                                        const unsigned flags);



/* iterator API */

APREQ_DECLARE(apr_status_t) apreq_table_fetch(const apreq_table_t *t,
                                              const char **key,
                                              const apreq_value_t **val,
                                              int *off);


APREQ_DECLARE(apr_status_t) apreq_table_first(const apreq_table_t *t,
                                              const char **key,
                                              const apreq_value_t **val,
                                              int *off);
APREQ_DECLARE(apr_status_t) apreq_table_next(const apreq_table_t *t,
                                             const char **key, 
                                             const apreq_value_t **val,
                                             int *off);
APREQ_DECLARE(apr_status_t) apreq_table_last(const apreq_table_t *t, 
                                             const char **key,
                                             const apreq_value_t **val, 
                                             int *off);
APREQ_DECLARE(apr_status_t) apreq_table_prev(const apreq_table_t *t, 
                                             const char **key, 
                                             const apreq_value_t **val,
                                             int *off);
/**
 * Declaration prototype for the iterator callback function of apr_table_do()
 * and apr_table_vdo().
 * @param rec The data passed as the first argument to apr_table_[v]do()
 * @param key The key from this iteration of the table
 * @param key The value from this iteration of the table
 * @remark Iteration continues while this callback function returns non-zero.
 * To export the callback function for apr_table_[v]do() it must be declared 
 * in the _NONSTD convention.
 */
typedef int (apreq_table_do_callback_fn_t)(void *rec, const char *key, 
                                           const char *val);

/** 
 * Iterate over a table running the provided function once for every
 * element in the table.  If there is data passed in as a vararg, then the 
 * function is only run on those elements whose key matches something in 
 * the vararg.  If the vararg is NULL, then every element is run through the
 * function.  Iteration continues while the function returns non-zero.
 * @param comp The function to run
 * @param rec The data to pass as the first argument to the function
 * @param t The table to iterate over
 * @param ... The vararg.  If this is NULL, then all elements in the table are
 *            run through the function, otherwise only those whose key matches
 *            are run.
 * @return FALSE if one of the comp() iterations returned zero; TRUE if all
 *            iterations returned non-zero
 * @see apr_table_do_callback_fn_t
 */
APREQ_DECLARE(int) apreq_table_do(apreq_table_do_callback_fn_t *comp,
                                  void *rec, 
                                  const apreq_table_t *t, ...);

/** 
 * Iterate over a table running the provided function once for every
 * element in the table.  If there is data passed in as a vararg, then the 
 * function is only run on those element's whose key matches something in 
 * the vararg.  If the vararg is NULL, then every element is run through the
 * function.  Iteration continues while the function returns non-zero.
 * @param comp The function to run
 * @param rec The data to pass as the first argument to the function
 * @param t The table to iterate over
 * @param vp The vararg table.  If this is NULL, then all elements in the 
 *                table are run through the function, otherwise only those 
 *                whose key matches are run.
 * @return FALSE if one of the comp() iterations returned zero; TRUE if all
 *            iterations returned non-zero
 * @see apr_table_do_callback_fn_t
 */
APREQ_DECLARE(int) apreq_table_vdo(apreq_table_do_callback_fn_t *comp,
                                 void *rec, 
                                 const apreq_table_t *t, va_list);


/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* ! APR_TABLES_H */
