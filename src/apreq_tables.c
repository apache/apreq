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

/*
 * Resource allocation code... the code here is responsible for making
 * sure that nothing leaks.
 *
 * rst --- 4/95 --- 6/95
 */

/*
#include "apr_private.h"
*/
#include "apr_general.h"
#include "apr_pools.h"
#include "apreq_tables.h"
#include "apr_strings.h"
#include "apr_lib.h"
#if APR_HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if APR_HAVE_STRING_H
#include <string.h>
#endif
#if APR_HAVE_STRINGS_H
#include <strings.h>
#endif
#include "apr_signal.h"



/********************* table_entry structure ********************/

/* private struct */

typedef struct apreq_table_entry_t {
    const char          *key;   /* = val->name : saves a ptr deref */
    const apreq_value_t *val;

    enum { RED, BLACK }  color;
    int                  tree[4];  /* LEFT RIGHT PARENT NEXT */
} apreq_table_entry_t;

/* LEFT & RIGHT must satisfy !LEFT==RIGHT , !RIGHT==LEFT */
#define LEFT   0
#define RIGHT  1
#define PARENT 2
#define NEXT   3



/********************* table structure ********************/


#define TABLE_HASH_SIZE 16
#define TABLE_INDEX_MASK 0x0f

/* TABLE_HASH must be compatible with table comparator t->cmp */
/* this one is strcasecmp compatible */
#define TABLE_HASH(key)  (TABLE_INDEX_MASK & *(unsigned char *)(key))


/* XXX: apreq_tables need to be signal( thread? )safe; currently
   these LOCK* macros indicate some unsafe locations in the code. */

/* coarse locking (maybe API worthy) */
#define LOCK_TABLE(t)
#define UNLOCK_TABLE(t)
#define TABLE_IS_LOCKED(t)

/* finer grained tree locks (internal) */
#define LOCK_TREE(t,n)
#define UNLOCK_TREE(t,n)
#define TREE_IS_LOCKED(t,n)


/* This may someday become public, but has hash+tree harmony embedded. */
typedef int (apreq_table_cmp_t)(const char *, const char *);

#define TF_MERGE     1
#define TF_BALANCE   2
#define TF_LOCK      4


struct apreq_table_t {
    apr_array_header_t   a;
    int                  ghosts;

    apreq_table_cmp_t   *cmp;

    apreq_value_copy_t  *copy;
    apreq_value_merge_t *merge;

    int                  root[TABLE_HASH_SIZE];
    unsigned long        locks; /* XXX ??? */

    unsigned char        flags;

#ifdef MAKE_TABLE_PROFILE
    int creator; /* XXX Is this the right type ??? */
#endif
};


/***************************** tree macros ****************************/


#define DEAD(idx)     ( (idx)[o].key == NULL )
#define KILL_ENTRY(t,idx) do { (idx)[o].key = NULL; \
                               (t)->ghosts++; } while (0)

/* NEVER KILL AN ENTRY THAT'S STILL WITHIN THE FOREST */
#define IN_FOREST(t,idx) ( (idx)[o].tree[PARENT] >= 0 || \
                           (idx) == t->root[TABLE_HASH((idx)[o].key)] )

/********************* (internal) tree operations ********************/


/* MUST ensure n's parent exists before using LR(n) */
#define LR(n) (  ( (n)[o].tree[PARENT][o].tree[LEFT] == (n) ) ? LEFT : RIGHT  )

#define PROMOTE(o,r,p)  do rotate(o,r,p,!LR(p)); while (o[p].tree[PARENT] >= 0)

static APR_INLINE void rotate(apreq_table_entry_t *o, 
                              int *root,
                              const int pivot, 
                              const int direction)
{
    const int child   = pivot[o].tree[!direction];
    const int parent  = pivot[o].tree[PARENT];

    pivot[o].tree[!direction] = child[o].tree[direction];

    if (pivot[o].tree[!direction] >= 0)
        pivot[o].tree[!direction][o].tree[PARENT] = pivot;

    (parent >= 0) ? parent[o].tree[LR(pivot)] : *root = child;

    child[o].tree[PARENT]      = parent;
    child[o].tree[direction]   = pivot;
    pivot[o].tree[PARENT]      = child;

}


static APR_INLINE int search(apreq_table_cmp_t *cmp,
                             apreq_table_entry_t *o, 
                             int *elt,
                             const char *key) 
{
    int idx = *elt;
    if ( idx < 0)
        return 0;
    while (1) {
        int direction = (*cmp)(key,idx[o].key);

        if (direction < 0 && idx[o].tree[LEFT] >= 0)
            idx = idx[o].tree[LEFT];
        else if (direction > 0 && idx[o].tree[RIGHT] >= 0)
            idx = idx[o].tree[RIGHT];
        else {
            *elt = idx;
            return direction;
        }
    }
    return 0;
}

/* XXX Are these macros really needed? */
#define TREE_PUSH      1
#define TREE_REPLACE   2

#define TREE_POP       1
#define TREE_DROP      2

static int insert(apreq_table_cmp_t *cmp,
                  apreq_table_entry_t *o, int *root, int x,
                  apreq_table_entry_t *elt,
                  unsigned flags )
{
    int idx = elt - o;
    int s = search(cmp, o, &x, elt->key);

    if (s == 0) { /* found */
        if (x < 0) { /* empty tree */
            *root = idx;
            elt->tree[LEFT]   = -1;
            elt->tree[RIGHT]  = -1;
            elt->tree[PARENT] = -1;
            return -1;
        }

        switch (flags) {
            int parent;

        case TREE_PUSH:
            parent = x;

            while (parent[o].tree[NEXT] >= 0)
                parent = parent[o].tree[NEXT];

            parent[o].tree[NEXT]  = idx;
            elt->tree[PARENT]      = -1;
            elt->tree[RIGHT]       = -1;
            elt->tree[LEFT]        = -1;
            return x;

        case TREE_REPLACE:
            parent = x[o].tree[PARENT];

            if (x[o].tree[LEFT] >= 0)
                x[o].tree[LEFT][o].tree[PARENT] = idx;

            if (x[o].tree[RIGHT] >= 0)
                x[o].tree[RIGHT][o].tree[PARENT] = idx;

            (parent >= 0) ? parent[o].tree[LR(x)] : *root = idx;

            elt->tree[PARENT] = parent;
            elt->tree[RIGHT]  = x[o].tree[RIGHT];
            elt->tree[LEFT]   = x[o].tree[LEFT];
            elt->color        = x[o].color;

            return x;

        default:
            return -1;
        }
    }


    /* The element wasn't in the tree, so add it */
    x[o].tree[s < 0 ? LEFT : RIGHT] = idx;
    elt->tree[PARENT] =  x;
    elt->tree[RIGHT]  = -1;
    elt->tree[LEFT]   = -1;

    elt->color = RED;


    if (flags & TF_BALANCE) {
        while (idx[o].tree[PARENT] >= 0 && idx[o].tree[PARENT][o].color == RED)
        {
        /* parent , grandparent, & parent_sibling nodes all exist */

            int parent = idx[o].tree[PARENT];
            int grandparent = parent[o].tree[PARENT];
            register const int parent_direction = LR(parent);
            int parent_sibling = grandparent[o].tree[!parent_direction];

            if (parent_sibling[o].color == RED) {
                parent[o].color            = BLACK;
                parent_sibling[o].color    = BLACK;
                grandparent[o].color       = RED;
                idx                        = grandparent;
                parent                     = idx[o].tree[PARENT];
            }
            else {  /* parent_sibling->color == BLACK */

                if ( LR(idx) != parent_direction ) { /* opposite direction */
                    rotate(o, root, parent, parent_direction); /* demotes idx */
                    idx           = parent;
                    parent        = idx[o].tree[PARENT]; /* idx's old sibling */
                    /* grandparent is unchanged */
                }

                parent[o].color      = BLACK;
                grandparent[o].color = RED;
                rotate(o, root, grandparent, !parent_direction);
            }
        }
    }
    (*root)[o].color = BLACK;
    return -1;
}


static void delete(apreq_table_entry_t *o, 
                   int *root,  const int idx, unsigned flags)
{
    int x,y;

    if ((flags & TREE_POP) && idx[o].tree[NEXT] >= 0) {
        /* pop elt off the stack */
        x = idx[o].tree[PARENT];
        y = idx[o].tree[NEXT];
        idx[o].tree[NEXT] = 0;

        y[o].tree[PARENT] = x;
        y[o].tree[LEFT]   = idx[o].tree[LEFT];
        y[o].tree[RIGHT]  = idx[o].tree[RIGHT];
        y[o].color        = idx[o].color;

        (x >= 0) ? x[o].tree[LR(idx)] : *root = y;

        idx[o].tree[PARENT] = -1;
        idx[o].tree[LEFT]   = -1;
        idx[o].tree[RIGHT]  = -1;

        return;
    }

    if (idx[o].tree[LEFT] < 0 || idx[o].tree[RIGHT] < 0)
        y = idx;
    else {
        y = idx[o].tree[RIGHT];
        while (y[o].tree[LEFT] >= 0)
            y = y[o].tree[LEFT];
    }

    /* x is y's only child */
    x = (y[o].tree[LEFT] >= 0) ? y[o].tree[LEFT] : y[o].tree[RIGHT];

    /* remove y from the parent chain */
    x[o].tree[PARENT] = y[o].tree[PARENT];

    (y[o].tree[PARENT] >= 0) ?
        y[o].tree[PARENT][o].tree[LR(y)] : *root = x;

    if (y != idx) {     /* swap y[o] with idx[o] */
        y[o].tree[LEFT] = idx[o].tree[LEFT];
        y[o].tree[RIGHT] = idx[o].tree[RIGHT];
        y[o].tree[PARENT] = idx[o].tree[PARENT];
        
        (y[o].tree[PARENT] >= 0) ?
            y[o].tree[PARENT][o].tree[LR(y)] : *root = y;

        idx[o].tree[LEFT] = -1;
        idx[o].tree[RIGHT] = -1;
        idx[o].tree[PARENT] = -1;
    }

    if (y[o].color == RED) {
        y[o].color = idx[o].color;
        return;
    }

    /* rebalance tree (standard double-black promotion) */

    x[o].color = idx[o].color; /* should this be y[o].color ??? */

    if (flags & TF_BALANCE) {
        while (x != *root && x[o].color == BLACK) 
        {
            /* x has a grandparent, parent & sibling */
            int parent = x[o].tree[PARENT];
            int grandparent = parent[o].tree[PARENT];
            register const int direction = LR(x);
            int sibling = parent[o].tree[!direction];

            if (sibling[o].color == RED) {
                sibling[o].color = BLACK;
                parent[o].color  = RED;
                rotate(o, root, parent, !direction); /* demote x */
                grandparent = sibling;
                sibling = parent[o].tree[!direction];
            }

            if (sibling[o].tree[direction][o].color == BLACK && 
                sibling[o].tree[!direction][o].color == BLACK) 
            {
                sibling[o].color = RED;
                x = parent;
            }
            else {
                if (sibling[o].tree[!direction][o].color == BLACK) {
                    sibling[o].tree[direction][o].color = BLACK;
                    sibling[o].color = RED;
                    rotate(o, root, sibling, !direction); /* demote sibling */
                    sibling = parent[o].tree[!direction];
                }
                sibling[o].color = parent[o].color;
                parent[o].color = BLACK;
                sibling[o].tree[!direction][o].color = BLACK;
                rotate(o, root, parent, direction); /* promote x */
                *root = x;
            }

        }
    }
    x[o].color = BLACK;
}

static int combine(apreq_table_cmp_t *cmp,
                   apreq_table_entry_t *o, int a, 
                   int b, const int n)
{
    if (a < 0)
        return b;
    else if (b < 0)
        return a;
    else {
        int parent = a[o].tree[PARENT];
        int left = b[o].tree[LEFT] + n;
        int right = b[o].tree[RIGHT] + n;
        parent[o].tree[LR(a)] = b;
        a[o].tree[PARENT] = -1;
        b = insert(cmp,o,&a,b,b+o,TREE_PUSH);
        if (a != b)
            PROMOTE(o,&a,b);
        b[o].tree[PARENT] = parent;
        b[o].tree[LEFT]  = combine(cmp, o, b[o].tree[LEFT],  left,  n);
        b[o].tree[RIGHT] = combine(cmp, o, b[o].tree[RIGHT], right, n);
        return b;
    }
}


/*****************************************************************
 *
 * The "apreq_table" API.
 */


/* static void make_array_core(apr_array_header_t *res, apr_pool_t *p,
			    int nelts, int elt_size, int clear) */

#define make_array_core(a,p,n,s,c) do {        \
    if (c)                                     \
        (a)->elts = apr_pcalloc(p, (n) * (s)); \
    else                                       \
        (a)->elts = apr_palloc( p, (n) * (s)); \
    (a)->pool = p;                             \
    (a)->elt_size = s;                         \
    (a)->nelts = 0;                            \
    (a)->nalloc = n;                           \
  } while (0)

#define v2c(ptr) ( (ptr) ? (ptr)->data : NULL )

/*
 * XXX: if you tweak this you should look at is_empty_table() and table_elts()
 * in alloc.h
 */

static void *apr_array_push_noclear(apr_array_header_t *arr)
{
    /* XXX: this function is not reentrant */

    if (arr->nelts == arr->nalloc) {
        int new_size = (arr->nalloc <= 0) ? 1 : arr->nalloc * 2;
        char *new_data;

        new_data = apr_palloc(arr->pool, arr->elt_size * new_size);

        memcpy(new_data, arr->elts, arr->nalloc * arr->elt_size);
        arr->elts = new_data;
        arr->nalloc = new_size;
    }

    ++arr->nelts;
    return arr->elts + (arr->elt_size * (arr->nelts - 1));
}

#ifdef MAKE_TABLE_PROFILE
static apreq_table_entry_t *table_push(apreq_table_t *t)
{
    if (t->a.nelts == t->a.nalloc) {
        return NULL;
    }
    return (apreq_table_entry_t *) apr_array_push_noclear(&t->a);
}
#else /* MAKE_TABLE_PROFILE */
#define table_push(t) ((apreq_table_entry_t *)apr_array_push_noclear(&(t)->a))
#endif /* MAKE_TABLE_PROFILE */



/********************* table ctors ********************/


static apreq_value_t *collapse(apr_pool_t *p, 
                               const apr_array_header_t *arr)
{
    apreq_value_t **a = (apreq_value_t **)arr->elts;
    return (arr->nelts == 0) ? NULL : a[arr->nelts - 1];
}

APREQ_DECLARE(apreq_table_t *) apreq_table_make(apr_pool_t *p, int nelts)
{
    apreq_table_t *t = apr_palloc(p, sizeof *t);

    make_array_core(&t->a, p, nelts, sizeof(apreq_table_entry_t), 0);
#ifdef MAKE_TABLE_PROFILE
    t->creator = __builtin_return_address(0);
#endif

    /* XXX: is memset(*,-1,*) portable ??? */
    memset(t->root, -1, TABLE_HASH_SIZE * sizeof(int));

    t->cmp    = strcasecmp;
    t->merge  = apreq_value_merge;
    t->copy   = apreq_value_copy;
    t->flags  = 0;
    t->ghosts = 0;
    return t;
}

APREQ_DECLARE(apreq_table_t *) apreq_table_copy(apr_pool_t *p, 
                                              const apreq_table_t *t)
{
    apreq_table_t *new = apr_palloc(p, sizeof *new);

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that t->a.pool
     * have a life span at least as long as p
     */
    if (!apr_pool_is_ancestor(t->a.pool, p)) {
	fprintf(stderr, "copy_table: t's pool is not an ancestor of p\n");
	abort();
    }
#endif

    make_array_core(&new->a, p, t->a.nalloc, sizeof(apreq_table_entry_t), 0);
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(apreq_table_entry_t));
    new->a.nelts = t->a.nelts;
    memcpy(new->root, t->root, TABLE_HASH_SIZE * sizeof(int));
    new->merge = t->merge;
    new->copy = t->copy;
    new->cmp = t->cmp;
    new->flags=t->flags;
    return new;
}

APREQ_DECLARE(apr_table_t *)apreq_table_export(const apreq_table_t *t, 
                                               apr_pool_t *p)
{
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    apr_table_t *s = apr_table_make(p, t->a.nelts);

    for (idx = 0; idx < t->a.nelts; ++idx) {
        if (! DEAD(idx)) {
            const apreq_value_t *v = idx[o].val; 
            apr_table_addn(s, v->name, v->data);
        }
    }

    return s;
}

APREQ_DECLARE(apreq_table_t *)apreq_table_import(apr_pool_t *p, 
                                                 const apr_table_t *s, 
                                                 const unsigned f) 
{
    apreq_table_t *t = apreq_table_make(p,APREQ_DEFAULT_NELTS);
    const apr_array_header_t *a = apr_table_elts(s);
    const apr_table_entry_t *e = (const apr_table_entry_t *)a->elts;
    const apr_table_entry_t *end = e + a->nelts;

    t->flags = f;

    for ( ; e < end; e++) {
        apreq_value_t *v = e->val ? 
            apreq_value_make(p,e->key,e->val,strlen(e->val)) : 
            NULL;
        apreq_table_add(t, v);
    }
    return t;
}



/********************* unary table ops ********************/



APREQ_DECLARE(void) apreq_table_clear(apreq_table_t *t)
{
    memset(t->root,-1,TABLE_HASH_SIZE * sizeof(int));
    t->a.nelts = 0;
    t->ghosts = 0;
}

APREQ_DECLARE(int) apreq_table_nelts(const apreq_table_t *t)
{
    return t->a.nelts - t->ghosts;
}

APREQ_DECLARE(apreq_value_copy_t *)apreq_table_copier(apreq_table_t *t, 
                                                      apreq_value_copy_t *c)
{
    apreq_value_copy_t *original = t->copy;

    if (c != NULL)
        t->copy = c;
    return original;
}

APREQ_DECLARE(apreq_value_merge_t *)apreq_table_merger(apreq_table_t *t, 
                                                       apreq_value_merge_t *m)
{
    apreq_value_merge_t *original = t->merge;

    if (m != NULL)
        t->merge = m;
    return original;
}


APREQ_DECLARE(void) apreq_table_balance(apreq_table_t *t, const int on)
{
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    if (on) {
        int idx;
        if (t->flags & TF_BALANCE)
            return;

        memset(t->root,-1,TABLE_HASH_SIZE * sizeof(int));
        for (idx = 0; idx < t->a.nelts; ++idx)
            insert(t->cmp, o, &t->root[TABLE_HASH(idx[o].key)], -1, &idx[o], 
                   TF_BALANCE | TREE_PUSH);

        t->flags |= TF_BALANCE;
    }
    else {
        t->flags &= ~TF_BALANCE;
    }
}

APREQ_DECLARE(apr_status_t) apreq_table_normalize(apreq_table_t *t)
{
    if (t->flags & TF_MERGE) {
        int idx;
        apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
        apr_array_header_t *a = apr_array_make(t->a.pool, APREQ_DEFAULT_NELTS,
                                               sizeof (apreq_value_t *));
        
        for (idx = 0; idx < t->a.nelts; ++idx) {
            int j = idx[o].tree[NEXT];

            if ( j >= 0 && IN_FOREST(t,idx) ) {

                LOCK_TABLE(t);

                idx[o].tree[NEXT] = -1;
                a->nelts = 0;
                *(const apreq_value_t **)apr_array_push(a) = idx[o].val;

                for ( ; j >= 0; j = j[o].tree[NEXT]) {
                    *(const apreq_value_t **)apr_array_push(a) = j[o].val;
                    KILL_ENTRY(t,j);
                }

                idx[o].val = t->merge(t->a.pool, a);

                UNLOCK_TABLE(t);
            }
        }
        return APR_SUCCESS;
    }
    else
        return APR_EGENERAL;
}

static int bury_table_entries(apreq_table_t *t, apr_array_header_t *q)
{
    /* EXPENSIVE: ~O(4 * t->a.nelts * q->nelts) */

    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    int j, n, m, *p = (int *)q->elts;
    int reindex = 0;

    /* add sentinel */
    *(int *)apr_array_push(q) = t->a.nelts; 
    q->nelts--;

    LOCK_TABLE(t);

    /* remove entries O(t->nelts) */
    for ( j=1, n=0; n < q->nelts; ++j, ++n )
        for (m = p[n]+1; m < p[n+1]; ++m) {
            m[o-j] = m[o];
        }

    t->a.nelts -= q->nelts;

    /* reindex trees */

#define REINDEX(P) for (n=0; P > p[n]; ++n) P--; reindex += n

    for (j=0; j < TABLE_HASH_SIZE; ++j) {
        REINDEX( t->root[j] );
    }

    for ( j=0; j < t->a.nelts; ++j ) {
        REINDEX( j[o].tree[LEFT]   );
        REINDEX( j[o].tree[RIGHT]  );
        REINDEX( j[o].tree[PARENT] );
        REINDEX( j[o].tree[NEXT]   );
    }

#undef REINDEX

    UNLOCK_TABLE(t);
    return reindex;
}

APREQ_DECLARE(int) apreq_table_ghosts(apreq_table_t *t)
{
    return t->ghosts;
}
APREQ_DECLARE(int) apreq_table_exorcise(apreq_table_t *t)
{
    int rv = 0;

    if (t->ghosts == 0)
        return 0;

    if (t->ghosts < t->a.nelts) {
        apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

        /* first eliminate trailing ghosts */
        while (DEAD(t->a.nelts - 1)) {
            t->a.nelts--;
            t->ghosts--;
        }

        /* bury any remaining ghosts */
        if (t->ghosts) {
            int idx;
            apr_array_header_t *a = apr_array_make(t->a.pool, 
                                                   t->ghosts + 1, 
                                                   sizeof(int));
            for (idx = 0; idx < t->a.nelts; ++idx)
                if (DEAD(idx))
                    *(int *)apr_array_push(a) = idx;

            rv = bury_table_entries(t,a);
        }
    }
    else { /* nothing but ghosts */
        t->a.nelts = 0;
    }

    t->ghosts = 0;
    return rv;
}



/********************* accessors ********************/



APREQ_DECLARE(const char *) apreq_table_get(const apreq_table_t *t,
                                             const char *key)
{
    int idx = t->root[TABLE_HASH(key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if ( idx < 0 || search(t->cmp,o,&idx,key) )
	return NULL;

    return v2c(idx[o].val);
}

APREQ_DECLARE(const char *) apreq_table_get_cached(apreq_table_t *t,
                                                   const char *key)
{
    int *root = &t->root[TABLE_HASH(key)];
    int idx = *root;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if ( idx < 0 || search(t->cmp,o,&idx,key) )
	return NULL;

    if (idx != *root) {
        t->flags &= ~TF_BALANCE;
        PROMOTE(o,root,idx);
    }
    return v2c(idx[o].val);
}

APREQ_DECLARE(apr_array_header_t *) apreq_table_keys(const apreq_table_t *t,
                                                     apr_pool_t *p)
{
    int i;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    apr_array_header_t *a = apr_array_make(p, t->a.nelts, 
                                           sizeof(char *));

    for (i = 0; i < t->a.nelts; ++i)
        if (IN_FOREST(t,i))
            *(const char **)apr_array_push(a) = i[o].key;

    return a;
}

APREQ_DECLARE(apr_array_header_t *) apreq_table_values(const apreq_table_t *t,
                                                       apr_pool_t *p,
                                                       const char *key)
{
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    apr_array_header_t *a = apr_array_make(p, key ? APREQ_DEFAULT_NELTS : 
                                                    t->a.nelts - t->ghosts,
                                                    sizeof(apreq_value_t *));

    if (key == NULL) {  /* fetch all values */
        for (idx = 0; idx < t->a.nelts; ++idx)
            if (IN_FOREST(t,idx))
                *(const apreq_value_t **)apr_array_push(a) = idx[o].val;
    }
    else {
        idx = t->root[TABLE_HASH(key)];
        if ( idx>=0 && search(t->cmp,o,&idx,key) == 0 )
           for ( ; idx>=0; idx = idx[o].tree[NEXT] )
                *(const apreq_value_t **)apr_array_push(a) = idx[o].val;
    }

    return a;
}



/********************* table_set* & table_unset ********************/


APREQ_DECLARE(void) apreq_table_set(apreq_table_t *t, const apreq_value_t *val)
{
    const char *key = val->name;
    int idx = t->root[TABLE_HASH(key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_pool_find(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_pool_find(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    if (idx >= 0 && search(t->cmp,o,&idx,key) == 0) {
        int n;
        idx[o].val = val;

        for ( n=idx[o].tree[NEXT]; n>=0; n=n[o].tree[NEXT] )
            KILL_ENTRY(t,n);

        idx[o].tree[NEXT] = -1;        
    }
    else {
        apreq_table_entry_t *e = table_push(t);
        e->key = key;
        e->val = val;
        e->tree[NEXT] = -1;
        insert(t->cmp, o,&t->root[TABLE_HASH(key)],idx,e,TREE_PUSH);
    }
}

APREQ_DECLARE(void) apreq_table_unset(apreq_table_t *t, const char *key)
{
    int idx = t->root[TABLE_HASH(key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if (idx >= 0 && search(t->cmp,o,&idx,key) == 0) {
        int n;

        LOCK_TABLE(t);

        delete(o,&t->root[TABLE_HASH(key)],idx,TREE_DROP);

        for ( n=idx; n>=0; n=n[o].tree[NEXT] )
            KILL_ENTRY(t,n);

        UNLOCK_TABLE(t);
    }
}



/********************* table_merge* *********************/

APREQ_DECLARE(void) apreq_table_merge(apreq_table_t *t,
                                      const apreq_value_t *val)
{
    const char *key = val->name;
    int idx = t->root[TABLE_HASH(key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_pool_find(val), t->a.pool)) {
            /* XXX */
	    fprintf(stderr, "apreq_table_merge: val not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    if (t->merge && idx >= 0 && search(t->cmp,o,&idx,val->name) == 0) {
        int n;
        apr_array_header_t *arr = apr_array_make(t->a.pool, 
                                                 APREQ_DEFAULT_NELTS, 
                                                 sizeof(apreq_value_t *));

        if (idx[o].val)
            *(const apreq_value_t **)apr_array_push(arr) = idx[o].val;

        for (n = idx[o].tree[NEXT]; n >= 0; n = n[o].tree[NEXT]) {
            KILL_ENTRY(t,n);

            if (n[o].val)
                *(const apreq_value_t **)apr_array_push(arr) = n[o].val;
        }

        if (val)
            *(const apreq_value_t **)apr_array_push(arr) = val;

        if (arr->nelts > 1)
            idx[o].val = t->merge(t->a.pool, arr);
    }

    else { /* not found */
        apreq_table_entry_t *e = table_push(t);
        e->key = val->name;
        e->val = val;
        e->tree[NEXT] = -1;
        insert(t->cmp,o,&t->root[TABLE_HASH(key)],idx,e,TREE_PUSH);
    }
}


/********************* table_add* *********************/

APREQ_DECLARE(void) apreq_table_add(apreq_table_t *t, 
                                    const apreq_value_t *val)
{
    const char *key = val->name;
    apreq_table_entry_t *elt = table_push(t);
    int *root = &t->root[TABLE_HASH(key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

#ifdef POOL_DEBUG
    {
	if (!apr_pool_is_ancestor(apr_pool_find(key), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
	if (!apr_pool_is_ancestor(apr_pool_find(val), t->a.pool)) {
	    fprintf(stderr, "table_set: key not in ancestor pool of t\n");
	    abort();
	}
    }
#endif

    elt->key = val->name;
    elt->val = val;
    elt->tree[NEXT] = -1;
    insert(t->cmp, o, root, *root, elt,TREE_PUSH);
}



/********************* binary operations ********************/



APREQ_DECLARE(void) apreq_table_cat(apreq_table_t *t, const apreq_table_t *s)
{
    const int n = t->a.nelts;
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    apr_array_cat(&t->a, &s->a);

    LOCK_TABLE(t);

    if (t->flags & TF_BALANCE) {
        for (idx = n; idx < t->a.nelts; ++idx) {
            const unsigned char hash = TABLE_HASH(idx[o].key);

            if (idx[o].tree[PARENT] == idx - n) { /* ghost from s */
                KILL_ENTRY(t,idx);
                continue;
            }

            if (idx[o].tree[NEXT] >= 0)
                idx[o].tree[NEXT] += n;
            
            if (t->root[hash] < 0) {
                if (idx[o].tree[LEFT] >= 0)
                    idx[o].tree[LEFT] += n;
                if (idx[o].tree[RIGHT] >= 0)
                    idx[o].tree[RIGHT] += n;
                if (idx[o].tree[PARENT] >= 0)
                    idx[o].tree[PARENT] += n;

                t->root[hash] = idx;
            }
            else if ( idx[o].tree[PARENT] >= 0 || 
                      s->root[hash] == idx-n ) 
            {
                insert(t->cmp, o, &t->root[hash], t->root[hash], idx+o, 
                       TREE_PUSH | TF_BALANCE);
            }
        }
    }
    else {
        for (idx = 0; idx < TABLE_HASH_SIZE; ++idx)
            t->root[idx] = combine(t->cmp, o,t->root[idx],s->root[idx],n);
    }

    UNLOCK_TABLE(t);
}

APREQ_DECLARE(apreq_table_t *) apreq_table_overlay(apr_pool_t *p,
                                                   const apreq_table_t *overlay,
                                                   const apreq_table_t *base)
{
    apreq_table_t *t = apr_palloc(p, sizeof *t);

#ifdef POOL_DEBUG
    /* we don't copy keys and values, so it's necessary that
     * overlay->a.pool and base->a.pool have a life span at least
     * as long as p
     */
    if (!apr_pool_is_ancestor(overlay->a.pool, p)) {
	fprintf(stderr,
		"overlay_tables: overlay's pool is not an ancestor of p\n");
	abort();
    }
    if (!apr_pool_is_ancestor(base->a.pool, p)) {
	fprintf(stderr,
		"overlay_tables: base's pool is not an ancestor of p\n");
	abort();
    }
#endif

    if (overlay->cmp != base->cmp)
        return NULL;

    t->a.pool = p;
    t->a.elt_size = sizeof(apreq_table_entry_t);
    t->copy = overlay->copy;
    t->cmp = overlay->cmp;
    t->merge = overlay->merge;
    t->flags = overlay->flags & base->flags;
    t->ghosts = overlay->ghosts;

    memcpy(t->root, overlay->root, TABLE_HASH_SIZE * sizeof(int));

    /* copy overlay array */
    t->a.elts = overlay->a.elts;
    t->a.nelts = overlay->a.nelts;
    t->a.nalloc = overlay->a.nelts;
    (void)apr_array_push(&t->a); /* induce copying */
    t->a.nelts--;


    if (base->a.nelts) {
        if (t->a.nelts)
            apreq_table_cat(t, base);
        else
            memcpy(t->root, base->root, TABLE_HASH_SIZE * sizeof(int));
    }

    return t;
}

APREQ_DECLARE(void) apreq_table_overlap(apreq_table_t *a, 
                                      const apreq_table_t *b,
                                      const unsigned flags)
{

    const int m = a->a.nelts;
    const int n = b->a.nelts;
    apr_pool_t *p = b->a.pool;

    if (m + n == 0 || a->cmp != b->cmp)
        return; /* ERROR !?!? */

    /* copy (extend) a using b's pool */
    if (a->a.pool != p) {
        make_array_core(&a->a, p, m+n, sizeof(apreq_table_entry_t), 0);
    }

    apreq_table_cat(a, b);

    if (flags == APR_OVERLAP_TABLES_SET) {
        apreq_value_merge_t *f = a->merge;
        a->merge = collapse;
        apreq_table_normalize(a);
        a->merge = f;
    }
    else
        apreq_table_normalize(a);
}


/********************* iterators ********************/

APREQ_DECLARE(apr_status_t) apreq_table_fetch(const apreq_table_t *t,
                                              const apreq_value_t **val,
                                              int *off)
{
    int idx = *off;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if (idx < 0 || idx >= t->a.nelts - t->ghosts) {
        *val = NULL;
        return APR_EGENERAL;
    }

    if (t->ghosts) {   /* count ghosts: O(N) */
        int n;

        for (n=0; n <= idx; ++n)
            if ( DEAD(n) )
                ++idx;

        *off = idx;
    }

    *val = idx[o].val;

    return APR_SUCCESS;
}

APREQ_DECLARE(apr_status_t) apreq_table_first(const apreq_table_t *t,
                                              const apreq_value_t **val,
                                              int *off)
{
    *off = -1;
    return apreq_table_next(t,val,off);
}

APREQ_DECLARE(apr_status_t) apreq_table_next(const apreq_table_t *t,
                                             const apreq_value_t **val,
                                             int *off)
{
    int idx = *off;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if (idx < -1 || idx >= t->a.nelts) {
        *val = NULL;
        return APR_EGENERAL;
    }

    do { 
        if (++idx == t->a.nelts) {
            *off = idx;
            *val = NULL;
            return APR_EGENERAL;
        }
    } while ( DEAD(idx) );

    *off = idx;
    *val = idx[o].val;
    return APR_SUCCESS;
}

APREQ_DECLARE(apr_status_t) apreq_table_last(const apreq_table_t *t, 
                                             const apreq_value_t **val, 
                                             int *off)
{
    *off = t->a.nelts;
    return apreq_table_prev(t,val,off);
}

APREQ_DECLARE(apr_status_t) apreq_table_prev(const apreq_table_t *t, 
                                             const apreq_value_t **val,
                                             int *off)
{
    int idx = *off;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if (idx <= 0 || idx > t->a.nelts) {
        *val = NULL;
        return APR_EGENERAL;
    }

    do { 
        if (--idx == -1) {
            *off = idx;
            *val = NULL;
            return APR_EGENERAL;
        }
    } while ( DEAD(idx) );

    *off = idx;
    *val = idx[o].val;
    return APR_SUCCESS;
}

/* And now for something completely abstract ...

 * For each key value given as a vararg:
 *   run the function pointed to as
 *     int comp(void *r, char *key, char *value);
 *   on each valid key-value pair in the apr_table_t t that matches the vararg key,
 *   or once for every valid key-value pair if the vararg list is empty,
 *   until the function returns false (0) or we finish the table.
 *
 * Note that we restart the traversal for each vararg, which means that
 * duplicate varargs will result in multiple executions of the function
 * for each matching key.  Note also that if the vararg list is empty,
 * only one traversal will be made and will cut short if comp returns 0.
 *
 * Note that the table_get and table_merge functions assume that each key in
 * the apr_table_t is unique (i.e., no multiple entries with the same key).  This
 * function does not make that assumption, since it (unfortunately) isn't
 * true for some of Apache's tables.
 *
 * Note that rec is simply passed-on to the comp function, so that the
 * caller can pass additional info for the task.
 *
 * ADDENDUM for apr_table_vdo():
 * 
 * The caching api will allow a user to walk the header values:
 *
 * apr_status_t apr_cache_el_header_walk(apr_cache_el *el, 
 *    int (*comp)(void *, const char *, const char *), void *rec, ...);
 *
 * So it can be ..., however from there I use a  callback that use a va_list:
 *
 * apr_status_t (*cache_el_header_walk)(apr_cache_el *el, 
 *    int (*comp)(void *, const char *, const char *), void *rec, va_list);
 *
 * To pass those ...'s on down to the actual module that will handle walking
 * their headers, in the file case this is actually just an apr_table - and
 * rather than reimplementing apr_table_do (which IMHO would be bad) I just
 * called it with the va_list. For mod_shmem_cache I don't need it since I
 * can't use apr_table's, but mod_file_cache should (though a good hash would
 * be better, but that's a different issue :). 
 *
 * So to make mod_file_cache easier to maintain, it's a good thing
 */
APREQ_DECLARE(int) apreq_table_do(apreq_table_do_callback_fn_t *comp,
                                  void *rec, const apreq_table_t *t, ...)
{
    int rv;

    va_list vp;
    va_start(vp, t);
    rv = apreq_table_vdo(comp, rec, t, vp);
    va_end(vp);

    return rv;
} 

/* XXX: do the semantics of this routine make any sense?  Right now,
 * if the caller passed in a non-empty va_list of keys to search for,
 * the "early termination" facility only terminates on *that* key; other
 * keys will continue to process.  Note that this only has any effect
 * at all if there are multiple entries in the table with the same key,
 * otherwise the called function can never effectively early-terminate
 * this function, as the zero return value is effectively ignored.
 *
 * Note also that this behavior is at odds with the behavior seen if an
 * empty va_list is passed in -- in that case, a zero return value terminates
 * the entire apr_table_vdo (which is what I think should happen in
 * both cases).
 *
 * If nobody objects soon, I'm going to change the order of the nested
 * loops in this function so that any zero return value from the (*comp)
 * function will cause a full termination of apr_table_vdo.  I'm hesitant
 * at the moment because these (funky) semantics have been around for a
 * very long time, and although Apache doesn't seem to use them at all,
 * some third-party vendor might.  I can only think of one possible reason
 * the existing semantics would make any sense, and it's very Apache-centric,
 * which is this: if (*comp) is looking for matches of a particular
 * substring in request headers (let's say it's looking for a particular
 * cookie name in the Set-Cookie headers), then maybe it wants to be
 * able to stop searching early as soon as it finds that one and move
 * on to the next key.  That's only an optimization of course, but changing
 * the behavior of this function would mean that any code that tried
 * to do that would stop working right.
 *
 * Sigh.  --JCW, 06/28/02
 */
APREQ_DECLARE(int) apreq_table_vdo(apr_table_do_callback_fn_t *comp,
                                 void *rec, const apreq_table_t *t, va_list vp)
{
    char *argp;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    int vdorv = 1;

    argp = va_arg(vp, char *);
    do {
        int rv = 1, idx;
        if (argp) {     /* Scan for entries that match the next key */
            idx = t->root[TABLE_HASH(argp)];
            if ( search(t->cmp,o,&idx,argp) == 0 )
                while (idx >= 0) {
                    rv = (*comp) (rec, idx[o].key, v2c(idx[o].val));
                    idx = idx[o].tree[NEXT];
                }
        }
        else {          /* Scan the entire table */
            for (idx = 0; rv && (idx < t->a.nelts); ++idx)
                /* if (idx[o].key) */
                if (! DEAD(idx) )
                    rv = (*comp) (rec, idx[o].key, v2c(idx[o].val));
        }
        if (rv == 0) {
            vdorv = 0;
        }
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));

    return vdorv;
}