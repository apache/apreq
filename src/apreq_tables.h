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

/**
 * @file apreq_tables.h
 * @brief APREQ Table library
 */
/**
 * @defgroup APREQ_Table Table routines
 * @ingroup APREQ
 *
 * Basic ADT for managing collections in APREQ.  Tables
 * remember the order in which entries are added, and respect 
 * this order whenever iterator APIs are used.
 *
 * APREQ Tables have a hash-like lookup API as well, and support 
 * multivalued keys.  Table keys are C strings, but the return values 
 * are guaranteed to point at the "data" attribute of an apreq_value_t.  
 * (The table key for an apreq_value_t is its "name".) Thus tables can 
 * be used to do lookups on binary/opaque data stored within an 
 * apreq_value_t.
 *
 * Tables also provide an abstract API for merging and copying the 
 * values it contains.
 * @{
 */

/** the table abstract data type */
typedef struct apreq_table_t apreq_table_t;

/**
 * Make a new table
 * @param p The pool to allocate the pool out of
 * @param nelts The number of elements in the initial table.
 * @return The new table.
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_make(apr_pool_t *p, int nelts);
#define apreq_make_table(p,n) apreq_table_make(p,n)

/**
 * Create a new table and copy another table into it
 * @param p The pool to allocate the new table out of
 * @param t The table to copy
 * @return A copy of the table passed in
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_copy(apr_pool_t *p,
                                            const apreq_table_t *t);


/**
 * Create an APR Table from an APREQ Table.
 * @param p The pool to allocate the APR table from
 * @param t The APREQ table to copy
 */
APREQ_DECLARE(apr_table_t *) apreq_table_export(apr_pool_t *p,
                                                const apreq_table_t *t);

/**
 * Create an APREQ Table from an APR Table.
 * @param p The pool to allocate the APREQ table from
 * @param t The APR table to copy
 */
APREQ_DECLARE(apreq_table_t *)apreq_table_import(apr_pool_t *p, 
                                                 apr_table_t *t);

/**
 * Delete all of the elements from a table
 * @param t The table to clear
 */
APR_INLINE
APREQ_DECLARE(void) apreq_table_clear(apreq_table_t *t);

/**
 * Return the number of elements within the table.
 * @param t The table to clear
 */
APR_INLINE
APREQ_DECLARE(int) apreq_table_nelts(const apreq_table_t *t);
#define apreq_table_is_empty(t) ( apreq_table_nelts(t) == 0 )

/**
 * Returns the pool associated to the table's underlying array.
 * @param t The table.
 */

APR_INLINE
APREQ_DECLARE(apr_pool_t *) apreq_table_pool(apreq_table_t *t);


/**
 * Get/set method for the table's value copier.
 * @param t Table.
 * @param c The new t->copy callback.  c = NULL is ignored;
 *          a non-NULL value replaces the table's internal copier.
 * @return The original t->copy callback (prior to any assignment).
 */
APR_INLINE
APREQ_DECLARE(apreq_value_copy_t *) apreq_table_copier(apreq_table_t *t, 
                                           apreq_value_copy_t *c);

/**
 * Get/set method for the table's value merger.
 * @param t Table.
 * @param m The new t->merge callback.  m = NULL is ignored;
 *          a non-NULL value replaces the table's internal merger.
 * @return The original t->merge callback (prior to any assignment).
 */
APR_INLINE
APREQ_DECLARE(apreq_value_merge_t *) apreq_table_merger(apreq_table_t *t,
                                           apreq_value_merge_t *m);

/**
 * Attempt to merge multivalued entries together, eliminating
 * redunandant entries with t->merge.  See apreq_table_merger 
 * for additional details.
 *
 * @param t Table.
 */
APREQ_DECLARE(apr_status_t) apreq_table_normalize(apreq_table_t *t);

/**
 * Count the number of dead entries within the table.
 * Mainly used for diagnostic purposes, since ghost entries are 
 * ignored by the table's accessor APIs.
 *
 * @param t Table.
 */
APR_INLINE
APREQ_DECLARE(int) apreq_table_ghosts(apreq_table_t *t);

/**
 * Remove dead entries from the table.  This can
 * be a very expensive and somewhat useless function
 * as far as the table API goes.  It's voodoo, so 
 * try not to use it.
 *
 * @param t Table.
 */
APREQ_DECLARE(int) apreq_table_exorcise(apreq_table_t *t);


/**
 * Get the value associated with a given key from the table.
 * @param t The table to search for the key
 * @param key The key to search for
 * @return The data associated with the key, guaranteed to
 * point at the "data" attribute of an apreq_value_t struct.
 *
 */
APREQ_DECLARE(const char*) apreq_table_get(const apreq_table_t *t, 
                                           const char *key);

/**
 * Return the keys (i.e. value names) in an (char *) array,
 * preserving their original order.
 * @param t Table.
 * @param keys array used to store the result.
 */

APREQ_DECLARE(apr_status_t) apreq_table_keys(const apreq_table_t *t,
                                             apr_array_header_t *keys);

/**
 * Add an apreq_value_t to the table. If another value already exists
 * with the same name, this will replace the old value.
 * @param t The table to add the data to.
 * @param val The value to add
 * @remark When adding data, this function uses the value's name as key.
 * Nothing is copied.
 */
APREQ_DECLARE(void) apreq_table_set(apreq_table_t *t, const apreq_value_t *v);

/**
 * Remove a key from the table.
 * @param t Table.
 * @param key The name of the apreq_values to remove.
 * @remark The table will drops ALL values associated with the key.
 */
APREQ_DECLARE(void) apreq_table_unset(apreq_table_t *t, const char *key);

/**
 * Add data to a table by merging the value with previous ones.
 * @param t The table to search for the data
 * @param key The key for locating previous values to merge with this one.
 * @param val The data to add
 * @remark If the key is not found, then this function acts like apr_table_add.
 * If multiple entries are found, they are all merged into a single value
 * via t->merge.
 */

APREQ_DECLARE(apr_status_t) apreq_table_merge(apreq_table_t *t, 
                                              const apreq_value_t *v);

/**
 * Add data to a table, regardless of whether there is another element with the
 * same key.
 * @param t The table to add to
 * @param val The value to add.
 * @remark This function does not make copies.
 */
APREQ_DECLARE(apr_status_t) apreq_table_addv(apreq_table_t *t,
                                             const apreq_value_t *v);

/**
 * Merge two tables into one new table
 * @param p The pool to use for the new table
 * @param over The first table to put in the new table
 * @param under The table to add at the end of the new table
 * @return A new table containing all of the data from the two passed in.
 */
APREQ_DECLARE(apreq_table_t *) apreq_table_overlay(apr_pool_t *p,
                                                   const apreq_table_t *over,
                                                   const apreq_table_t *under);
/**
 * Append one table to the end of another.
 * @param t The table to be modified.
 * @param s The values from this table are added to "t".
 * @remark This function splices the internal binary trees from "s"
 * into "t", so it will be faster than iterating s with apreq_table_add.
 * From a user's perspective, the result should be identical.
 */
APREQ_DECLARE(void) apreq_table_cat(apreq_table_t *t, const apreq_table_t *s);

/**
 * XXX: This doc needs to be modified for APREQ.  Volunteers?

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

APREQ_DECLARE(apr_status_t) apreq_table_overlap(apreq_table_t *a, 
                                                const apreq_table_t *b,
                                                const unsigned flags);

/** Iterator API */

/**
 *  NOTE: elt_size > sizeof(apreq_value_t *) !!!
 *  So iterating over the values in the array must go something like this:
 *
 *  apr_array_header_t *arr = apreq_table_elts(t);
 *  char *current, *end = arr->elts + arr->elt_size * arr->nelts;
 *
 *  for (current = arr->elts; current < end; current += arr->elt_size) {
 *      apreq_value_t *v = *(apreq_value_t **)current;
 *
 *       ... do something with "v" ..
 *  }
 */

APR_INLINE
APREQ_DECLARE(const apr_array_header_t *)apreq_table_elts(apreq_table_t *t);

/**
 * Declaration prototype for the iterator callback function of apr_table_do()
 * and apr_table_vdo().
 * @param rec The data passed as the first argument to apr_table_[v]do()
 * @param key The key from this iteration of the table
 * @param val The v->data from this iteration of the table
 * @remark Iteration continues while this callback function returns non-zero.
 * To export the callback function for apr_table_[v]do() it must be declared 
 * in the _NONSTD convention.
 */
typedef int (apreq_table_do_callback_fn_t)(void *ctx, const char *key, 
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
 * @see apreq_table_do_callback_fn_t
 */
APREQ_DECLARE_NONSTD(int) apreq_table_do(apreq_table_do_callback_fn_t *comp,
                                         void *ctx,
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
 * @see apreq_table_do_callback_fn_t
 */
APREQ_DECLARE(int) apreq_table_vdo(apreq_table_do_callback_fn_t *comp,
                                   void *ctx, const apreq_table_t *t, 
                                   va_list vp);



/** @} */
#ifdef __cplusplus
}
#endif

#endif	/* ! APR_TABLES_H */
