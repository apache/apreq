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

#include <assert.h>
/********************* table_entry structure ********************/

/* private struct */

typedef struct apreq_table_entry_t {
    const apreq_value_t *val;
    const char          *key;   /* = val->name : saves a ptr deref */
    apr_uint32_t         checksum;
    enum { RED, BLACK }  color;
    int                  tree[4];  /* LEFT RIGHT UP NEXT */
} apreq_table_entry_t;

/* LEFT & RIGHT must satisfy !LEFT==RIGHT , !RIGHT==LEFT */
#define LEFT   0
#define RIGHT  1
#define UP     2
#define NEXT   3



/********************* table structure ********************/

#if APR_CHARSET_EBCDIC
#define CASE_MASK 0xbfbfbfbf
#else
#define CASE_MASK 0xdfdfdfdf
#endif

#define TABLE_HASH_SIZE 16
#define TABLE_INDEX_MASK 0x0f
#define TABLE_HASH(c)  (TABLE_INDEX_MASK & (unsigned char)(c))
#define COMPUTE_KEY_CHECKSUM(k, checksum)    \
{                                            \
    apr_uint32_t c = (apr_uint32_t)*(k);     \
    (checksum) = c;                          \
    (checksum) <<= 8;                        \
    if (c) {                                 \
        c = (apr_uint32_t)*++(k);            \
        checksum |= c;                       \
    }                                        \
    (checksum) <<= 8;                        \
    if (c) {                                 \
        c = (apr_uint32_t)*++(k);            \
        checksum |= c;                       \
    }                                        \
    (checksum) <<= 8;                        \
    if (c) {                                 \
        c = (apr_uint32_t)*++(k);            \
        checksum |= c;                       \
    }                                        \
    checksum &= CASE_MASK;                   \
}


/* XXX: apreq_tables need to be signal( thread? )safe; currently
   these LOCK* macros might indicate some unsafe locations in the code. */

#define LOCK_TABLE(t)
#define UNLOCK_TABLE(t)
#define TABLE_IS_LOCKED(t)

#define TF_BALANCE   1
#define TF_LOCK      2


struct apreq_table_t {
    apr_array_header_t   a;
    int                  ghosts;

    apreq_value_copy_t  *copy;  /* XXX: this may go away soon */
    apreq_value_merge_t *merge;

    int                  root[TABLE_HASH_SIZE];
    unsigned char        flags;
};


/***************************** tree macros ****************************/


#define DEAD(idx)     ( (idx)[o].key == NULL )
#define KILL(t,idx)    do { (idx)[o].key = NULL; \
                            (t)->ghosts++; } while (0)
#define RESURRECT(t,idx) do { (idx)[o].key = (idx)[o].val->name; \
           COMPUTE_KEY_CHECKSUM((idx)[o].key,(idx)[o].checksum); \
                                               (t)->ghosts--; } while (0)

/* NEVER KILL AN ENTRY THAT'S STILL WITHIN THE FOREST */
#define IN_FOREST(t,idx) ( !DEAD(idx) && ( (idx)[o].tree[UP] >= 0 || \
                   (idx) == t->root[TABLE_HASH((idx)[o].checksum>>24)] ) )

/* MUST ensure n's parent exists (>=0) before using LR(n) */
#define LR(n) (  ( (n)[o].tree[UP][o].tree[LEFT] == (n) ) ? LEFT : RIGHT  )

#define PROMOTE(r,p)  do rotate(o,r,o[p].tree[UP],!LR(p)); \
                         while (o[p].tree[UP] >= 0)

/********************* (internal) tree operations ********************
 *
 * good general references for binary tree algorithms:
 *
 * Ben Pfaff's book on libavl 2.0:
 *      http://www.msu.edu/user/pfaffben/avl/
 *
 */


/**
 *       pivot                            child
 *       /  \                             /  \
 *   child  direction   == rotate ===>   1  pivot
 *    / \                                   /  \
 *   1   2                                 2  direction
 *
 */

static APR_INLINE void rotate(apreq_table_entry_t *o, 
                              int *root,
                              const int pivot, 
                              const int direction)
{
    const int child   = pivot[o].tree[!direction];
    const int parent  = pivot[o].tree[UP];

    pivot[o].tree[!direction] = child[o].tree[direction];

    if (pivot[o].tree[!direction] >= 0)
        pivot[o].tree[!direction][o].tree[UP] = pivot;

    if (parent >= 0)
        parent[o].tree[LR(pivot)] = child;
    else
        *root = child;

    child[o].tree[UP]        = parent;
    child[o].tree[direction] = pivot;
    pivot[o].tree[UP]        = child;

}


/*
 * puts location of "nearest" index in *elt
 * returns 0 if key is found, otherwise the return value's
 * sign represents the direction (LEFT <0, RIGHT >0) to move 
 * from elt.   This is where the key "should" be located
 * were it added to the tree.
 *
 */
static APR_INLINE int search(apreq_table_entry_t *o, 
                             int *elt,
                             const char *key) 
{
    int idx = *elt;
    apr_uint32_t checksum;
    if ( idx < 0)
        return 0;

    COMPUTE_KEY_CHECKSUM(key, checksum);

    while (1) {
        int direction = (checksum == idx[o].checksum) ? 
            strcasecmp(key,idx[o].key) : checksum - idx[o].checksum;

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


/* returns the index of the first similarly-named elt,
 * and -1 otherwise (the elt is new).
 */
static int insert(apreq_table_entry_t *o, int *root, int x,
                  apreq_table_entry_t *elt,
                  unsigned flags )
{
    int idx = elt - o;
    int s;

#define REBALANCE do {                                  \
    int parent = x[o].tree[UP];                         \
    x[o].color == RED;                                  \
    if (parent >= 0 && parent[o].color == RED) {        \
        int parent_direction = LR(parent);              \
        int grandparent = parent[o].tree[UP];           \
        assert(parent >=0 && grandparent >= 0);         \
        if (parent_direction != LR(x)) {                \
            rotate(o, root, parent, parent_direction);  \
            parent = x;                                 \
        }                                               \
        parent[o].color = BLACK;                        \
        grandparent[o].color = RED;                     \
        rotate(o, root, grandparent, !parent_direction);\
    } (*root)[o].color = BLACK; } while (0)

  
    if (flags & TF_BALANCE && *root != -1) {
        /*
         * M.A. Weiss' single-pass insertion algorithm for red-black trees
         * corrects potential red-red violations prior to insertion:
         *      http://www.cs.uiowa.edu/~hzhang/c44/lec14.PDF
         */

        while (1) {
            int left = x[o].tree[LEFT];
            int right = x[o].tree[RIGHT];

            s = (elt->checksum == x[o].checksum) ? 
                strcasecmp(elt->key,x[o].key) : elt->checksum - x[o].checksum;

            if (s < 0 && left >= 0) {
                if (left[o].color == RED && right >= 0 
                    && right[o].color == RED)
                {
                    left[o].color = right[o].color = BLACK;
                    REBALANCE;
                }
                x = left;
            }
            else if (s > 0 && right >= 0) {
                if (right[o].color == RED && left >= 0 
                    && left[o].color == RED)
                {
                    left[o].color = right[o].color = BLACK;
                    REBALANCE;
                }
                x = right;
            }
            else
                break;
        }
    }
    else
        s = search(o, &x, elt->val->name);

    if (s == 0) { /* found */
        int parent = x;
        if (parent < 0) { /* empty tree */
            *root = idx;
            elt->tree[LEFT]   = -1;
            elt->tree[RIGHT]  = -1;
            elt->tree[UP]     = -1;
            elt->color        = BLACK;
            return -1;
        }

        elt->color = parent[o].color;

        while (parent[o].tree[NEXT] >= 0)
            parent = parent[o].tree[NEXT];

        parent[o].tree[NEXT]  = idx;
        elt->tree[UP]         = -1;
        elt->tree[RIGHT]      = -1;
        elt->tree[LEFT]       = -1;
        return x;
    }


    /* The element wasn't in the tree, so add it */
    x[o].tree[s < 0 ? LEFT : RIGHT] = idx;

    elt->tree[UP]    =  x;
    elt->tree[RIGHT] = -1;
    elt->tree[LEFT]  = -1;

    if (flags & TF_BALANCE) {
        elt->color = RED;
        x = idx;
        REBALANCE;
    }
#undef REBALANCE
    return -1;
}

static void delete(apreq_table_entry_t *o, 
                   int *root,  const int idx, unsigned flags)
{
    int x;
    int parent = idx[o].tree[UP];

    if (idx[o].tree[LEFT] < 0 || idx[o].tree[RIGHT] < 0) {
        x = 1 + idx[o].tree[RIGHT] + idx[o].tree[LEFT];

        /* remove idx & promote x */
        if (parent >= 0)
            parent[o].tree[LR(idx)] = x;
        else
            *root = x;

        if (x >= 0)
            x[o].tree[UP] = parent;
        else
            x = parent;

        if (flags & TF_BALANCE == 0 || idx[o].color == RED)
            return;
    }
    else {
        /* find idx's in-order successor & replace idx with it */
        int y = idx[o].tree[RIGHT];
        while (y[o].tree[LEFT] >= 0)
            y = y[o].tree[LEFT];

        x = y[o].tree[RIGHT];

        if (y[o].tree[UP] != idx) {
            y[o].tree[RIGHT] = idx[o].tree[RIGHT];

            if (x >= 0) {
                /* replace y with x */
                x[o].tree[UP]                = y[o].tree[UP];
                y[o].tree[UP][o].tree[LR(y)] = x;
            }
            else
                x = y[o].tree[UP];
        }
        else
            if (x < 0)
                x = parent;

        /* copy idx's tree data into y ("RIGHT" is already done). */
        y[o].tree[LEFT] = idx[o].tree[LEFT];
        y[o].tree[UP]   = idx[o].tree[UP];

        /* replace idx with y */
        if (parent >= 0)
            parent[o].tree[LR(idx)] = y;
        else
            *root = y;

        if (flags & TF_BALANCE == 0 || y[o].color == RED) {
            y[o].color = idx[o].color;
            return;
        }
        else
            y[o].color = idx[o].color;
    }


    /* Rebalance tree to deal with violations of 
     * black node count via standard double-black promotion.
     * online ref:
     * Bruce Schneier's 1992 DDJ article:
     *      http://www.ddj.com/documents/s=1049/ddj9204c/9204c.htm
     *
     */

    while (x != *root && x[o].color == BLACK) {
        /* x has a parent & sibling */
        int parent = x[o].tree[UP];
        register const int direction = LR(x);
        int sibling = parent[o].tree[!direction];
        assert(parent >= 0);
        assert(sibling >= 0);
        if (sibling[o].color == RED) {
            sibling[o].color = BLACK;
            parent[o].color  = RED;
            rotate(o, root, parent, direction); /* promote sibling */
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
            rotate(o, root, parent, direction); /* promote sibling */
            x = *root;
        }
    }
    x[o].color = BLACK;
}

/*
 * Recursive function that combines two trees by root-inserting
 * the elements of the second (b) into the first (a).  Note that 
 * elements of the second tree must also be reindexed since their 
 * origin is n entries away from a's origin (o).
 *
 */
static int combine(apreq_table_entry_t *o, int a, int b, const int n)
{
    int left, right, next;

    if (b < 0)
        return a;

    b += n;

    left = b[o].tree[LEFT];
    right = b[o].tree[RIGHT];

    for(next = b; next[o].tree[NEXT] >= 0; next = next[o].tree[NEXT])
        next[o].tree[NEXT] += n;

    if (a >= 0) {
        int rv     = a;
        int parent = a[o].tree[UP];

        if (parent >= 0) {
            parent[o].tree[LR(a)] =  b;
            a[o].tree[UP]         = -1;
        }

        if (insert(o,&a,a,b+o,0) < 0) /* b is a new element for a's tree */
            rv = b;

        if (b[o].tree[UP] >= 0)
            PROMOTE(&a,b);

        b[o].tree[UP]    = parent;
        b[o].tree[LEFT]  = combine(o, b[o].tree[LEFT],  left,  n);
        b[o].tree[RIGHT] = combine(o, b[o].tree[RIGHT], right, n);

        return rv;
    }
    else {
        if (b[o].tree[UP] >= 0)
            b[o].tree[UP] += n;
        b[o].tree[LEFT]  = combine(o, -1,  left, n);
        b[o].tree[RIGHT] = combine(o, -1, right, n);

        return b;
    }

}


/*****************************************************************
 *
 * The "apreq_table" API.
 */

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

static void *apr_array_push_noclear(apr_array_header_t *arr)
{
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

#define table_push(t) ((apreq_table_entry_t *)apr_array_push_noclear(&(t)->a))


/********************* table ctors ********************/



APREQ_DECLARE(apreq_table_t *) apreq_table_make(apr_pool_t *p, int nelts)
{
    apreq_table_t *t = apr_palloc(p, sizeof *t);

    make_array_core(&t->a, p, nelts, sizeof(apreq_table_entry_t), 0);

    /* XXX: is memset(*,-1,*) portable ??? */
    memset(t->root, -1, TABLE_HASH_SIZE * sizeof(int));

    t->merge  = apreq_merge_values;
    t->copy   = apreq_copy_value;
    t->flags  = 0;
    t->ghosts = 0;
    return t;
}


APREQ_DECLARE(apreq_table_t *) apreq_table_copy(apr_pool_t *p, 
                                                const apreq_table_t *t)
{
    apreq_table_t *new = apr_palloc(p, sizeof *new);

    make_array_core(&new->a, p, t->a.nalloc, sizeof(apreq_table_entry_t), 0);
    memcpy(new->a.elts, t->a.elts, t->a.nelts * sizeof(apreq_table_entry_t));
    new->a.nelts = t->a.nelts;
    new->ghosts = t->ghosts;
    memcpy(new->root, t->root, TABLE_HASH_SIZE * sizeof(int));
    new->merge = t->merge;
    new->copy = t->copy;
    new->flags = t->flags;
    return new;
}


APREQ_DECLARE(apr_table_t *)apreq_table_export(apr_pool_t *p,
                                               const apreq_table_t *t)
{
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    apr_table_t *s = apr_table_make(p, t->a.nalloc);

    for (idx = 0; idx < t->a.nelts; ++idx) {
        if (! DEAD(idx)) {
            const apreq_value_t *v = idx[o].val; 
            apr_table_add(s, v->name, v->data);
        }
    }

    return s;
}

APREQ_DECLARE(apreq_table_t *)apreq_table_import(apr_pool_t *p, 
                                                 const apr_table_t *s, 
                                                 const unsigned f) 
{
    apreq_table_t *t = apreq_table_make(p,APREQ_NELTS);
    const apr_array_header_t *a = apr_table_elts(s);
    const apr_table_entry_t *e = (const apr_table_entry_t *)a->elts;
    const apr_table_entry_t *end = e + a->nelts;

    t->flags = f;

    for ( ; e < end; ++e) {
        apreq_value_t *v = apreq_make_value(p, e->key, strlen(e->key), 
                                            e->val, e->val ? 
                                                    strlen(e->val) :
                                                    0);
        apreq_table_add(t, v);
    }
    return t;
}



/********************* unary table ops ********************/


APR_INLINE
APREQ_DECLARE(void) apreq_table_clear(apreq_table_t *t)
{
    memset(t->root,-1,TABLE_HASH_SIZE * sizeof(int));
    t->a.nelts = 0;
    t->ghosts = 0;
}

APR_INLINE
APREQ_DECLARE(int) apreq_table_nelts(const apreq_table_t *t)
{
    return t->a.nelts - t->ghosts;
}

APR_INLINE
APREQ_DECLARE(int) apreq_table_ghosts(apreq_table_t *t)
{
    return t->ghosts;
}

APR_INLINE
APREQ_DECLARE(apreq_value_copy_t *)apreq_table_copier(apreq_table_t *t, 
                                                      apreq_value_copy_t *c)
{
    apreq_value_copy_t *original = t->copy;

    if (c != NULL)
        t->copy = c;
    return original;
}

APR_INLINE
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
            if (!DEAD(idx))
                insert(o, &t->root[TABLE_HASH(idx[o].checksum>>24)], 
                       t->root[TABLE_HASH(idx[o].checksum>>24)], idx+o, 
                       TF_BALANCE);

        t->flags |= TF_BALANCE;
    }
    else {
        t->flags &= ~TF_BALANCE;
    }
}


APREQ_DECLARE(apr_status_t) apreq_table_normalize(apreq_table_t *t)
{
    apr_status_t status = APR_SUCCESS;
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    apreq_value_t *a[APREQ_NELTS];
    apr_array_header_t arr = { t->a.pool, sizeof(apreq_value_t *), 0,
                               APREQ_NELTS, (char *)a };

    if (t->merge == NULL)
        return APR_ENOTIMPL;

    for (idx = 0; idx < t->a.nelts; ++idx) {
        int j = idx[o].tree[NEXT];
 
        if ( j >= 0 && IN_FOREST(t,idx) ) {
            apreq_value_t *v;

            arr.nelts = 0;

            *(const apreq_value_t **)apr_array_push(&arr) = idx[o].val;

            for ( ; j >= 0; j = j[o].tree[NEXT]) {
                KILL(t,j);
                *(const apreq_value_t **)apr_array_push(&arr) = j[o].val;
            }

            v = t->merge(t->a.pool, &arr);

            if (v == NULL) {
                status = APR_EINCOMPLETE;
                for (j = idx[o].tree[NEXT] ; j >= 0; j = j[o].tree[NEXT])
                    RESURRECT(t,j);
            }
            else {
                idx[o].val = v;
                idx[o].tree[NEXT] = -1;
            }
        }
    }

    while (DEAD(t->a.nelts-1)) {
        t->ghosts--;
        t->a.nelts--;
    }

    return status;
}

static int bury_table_entries(apreq_table_t *t, apr_array_header_t *q)
{
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    int j, m, rv=0, *p = (int *)q->elts;
    register int n;

    /* add sentinel */
    *(int *)apr_array_push(q) = t->a.nelts; 
    q->nelts--;

    /* remove entries O(t->nelts)! */
    for ( j=1, n=0; n < q->nelts; ++j, ++n )
        for (m = p[n]+1; m < p[n+1]; ++m)
            m[o-j] = m[o];

    t->a.nelts -= q->nelts;

    /* reindex trees */
    /* EXPENSIVE: ~O(4 * t->a.nelts * q->nelts) */

#define REINDEX(P) for (n=0; P > p[n]; ++n) ; P -= n; rv += n

    for (j=0; j < TABLE_HASH_SIZE; ++j) {
        REINDEX( t->root[j] );
    }

    for ( j=0; j < t->a.nelts; ++j ) {
        REINDEX( j[o].tree[LEFT]  );
        REINDEX( j[o].tree[RIGHT] );
        REINDEX( j[o].tree[UP]    );
        REINDEX( j[o].tree[NEXT]  );
    }

#undef REINDEX

    return rv;
}


APREQ_DECLARE(int) apreq_table_exorcise(apreq_table_t *t)
{
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    int idx;
    int a[APREQ_NELTS];
    apr_array_header_t arr = { t->a.pool, sizeof(int), 0,
                               APREQ_NELTS, (char *)a };


    if (t->ghosts == 0)
        return 0;

    for (idx = 0; idx < t->a.nelts; ++idx)
        if (DEAD(idx)) {
            *(int *)apr_array_push(&arr) = idx;
        }

    t->ghosts = 0;
    return bury_table_entries(t,&arr);
}



/********************* accessors ********************/



APREQ_DECLARE(const char *) apreq_table_get(const apreq_table_t *t,
                                             const char *key)
{
    int idx = t->root[TABLE_HASH(*key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if ( idx < 0 || search(o,&idx,key) )
	return NULL;

    return v2c(idx[o].val);
}

APREQ_DECLARE(const char *) apreq_table_cache(apreq_table_t *t,
                                              const char *key)
{
    int *root = &t->root[TABLE_HASH(*key)];
    int idx = *root;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if ( idx < 0 || search(o,&idx,key) )
	return NULL;

    if (idx != *root) {
        t->flags &= ~TF_BALANCE;
        PROMOTE(root,idx);
    }
    return v2c(idx[o].val);
}


APREQ_DECLARE(apr_status_t) apreq_table_keys(const apreq_table_t *t,
                                             apr_array_header_t *keys)
{
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    if (t->a.nelts == t->ghosts)
        return APR_NOTFOUND;

    for (idx = 0; idx < t->a.nelts; ++idx)
        if (IN_FOREST(t,idx))
            *(const char **)apr_array_push(keys) = idx[o].val->name;

    return APR_SUCCESS;

}


APREQ_DECLARE(apr_status_t) apreq_table_values(const apreq_table_t *t,
                                               const char *key,
                                               apr_array_header_t *values)
{
    int idx;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    if (t->a.nelts == t->ghosts)
        return APR_NOTFOUND;

    if (key == NULL) {  /* fetch all values */
        for (idx = 0; idx < t->a.nelts; ++idx)
            if (IN_FOREST(t,idx))
                *(const apreq_value_t **)apr_array_push(values) = idx[o].val;
    }
    else {
        idx = t->root[TABLE_HASH(*key)];
        if ( idx>=0 && search(o,&idx,key) == 0 )
           for ( ; idx>=0; idx = idx[o].tree[NEXT] )
                *(const apreq_value_t **)apr_array_push(values) = idx[o].val;
        else
            return APR_NOTFOUND;
    }

    return APR_SUCCESS;
}


APREQ_DECLARE(void) apreq_table_set(apreq_table_t *t, 
                                    const apreq_value_t *val)
{
    const char *key = val->name;
    int idx;
    apreq_table_entry_t *e = table_push(t);
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    e->key = key;
    e->val = val;
    e->tree[NEXT] = -1;
    COMPUTE_KEY_CHECKSUM(e->key,e->checksum);
    idx = insert(o,&t->root[TABLE_HASH(*key)], 
                 t->root[TABLE_HASH(*key)] ,e,t->flags);

    if (idx >= 0) {
        int n;
        idx[o].val = val;

        for ( n=idx[o].tree[NEXT]; n>=0; n=n[o].tree[NEXT] )
            KILL(t,n);

        while (DEAD(t->a.nelts-1)) {
            t->ghosts--;
            t->a.nelts--;
        }
        idx[o].tree[NEXT] = -1;
    }
}


APREQ_DECLARE(void) apreq_table_unset(apreq_table_t *t, const char *key)
{
    int idx = t->root[TABLE_HASH(*key)];
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    if (idx >= 0 && search(o,&idx,key) == 0) {
        int n;
        delete(o,&t->root[TABLE_HASH(*key)],idx, t->flags);
        for ( n=idx; n>=0; n=n[o].tree[NEXT] )
            KILL(t,n);
    }
}


APREQ_DECLARE(apr_status_t) apreq_table_merge(apreq_table_t *t,
                                      const apreq_value_t *val)
{
    const char *key = val->name;
    int idx = t->root[TABLE_HASH(*key)];
    apreq_table_entry_t *e = table_push(t);
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

    e->key = key;
    e->val = val;
    e->tree[NEXT] = -1;
    COMPUTE_KEY_CHECKSUM(key,e->checksum);
    idx = insert(o,&t->root[TABLE_HASH(*key)],idx,e,t->flags);

    if (idx >= 0) {
        int n;
        apreq_value_t *a[APREQ_NELTS];
        apr_array_header_t arr = { t->a.pool, sizeof(apreq_value_t *), 0,
                                         APREQ_NELTS, (char *)a };

        *(const apreq_value_t **)apr_array_push(&arr) = idx[o].val;

        for (n = idx[o].tree[NEXT]; n >= 0; n = n[o].tree[NEXT]) {
            *(const apreq_value_t **)apr_array_push(&arr) = n[o].val;
            KILL(t,n);
        }

        *(const apreq_value_t **)apr_array_push(&arr) = val;

        val = t->merge(t->a.pool, &arr);

        if (val == NULL) {
            for (n = idx[o].tree[NEXT]; n >= 0; n = n[o].tree[NEXT])
                RESURRECT(t,n);

            return APR_ENOTIMPL;
        }

        idx[o].val = val;

        while (DEAD(t->a.nelts-1)) {
            t->ghosts--;
            t->a.nelts--;
        }

        idx[o].tree[NEXT] = -1;
    }

    return val->status;
}


APREQ_DECLARE(apr_status_t) apreq_table_add(apreq_table_t *t, 
                                            const apreq_value_t *val)
{
    if (val == NULL || val->name == NULL)
        return APR_EGENERAL;

    switch (val->status) {
    case APR_SUCCESS:
    case APR_EINPROGRESS:
    case APR_INCOMPLETE:
    {
        const char *key = val->name;
        apreq_table_entry_t *elt = table_push(t);
        int *root = &t->root[TABLE_HASH(*key)];
        apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;

        elt->key = key;
        elt->val = val;
        elt->tree[NEXT] = -1;
        COMPUTE_KEY_CHECKSUM(elt->key,elt->checksum);
        insert(o, root, *root, elt, t->flags);
        return APR_SUCCESS;
    }

    default:
        return val->status;
    }

    /* NOT REACHED */
    return APR_SUCCESS;
}



/********************* binary operations ********************/


APREQ_DECLARE(void) apreq_table_cat(apreq_table_t *t, 
                                    const apreq_table_t *s)
{
    const int n = t->a.nelts;
    int idx;
    apreq_table_entry_t *o;
    if (t->ghosts == n) {
        t->a.nelts == 0;
        t->ghosts = s->ghosts;
        apr_array_cat(&t->a,&s->a);
        return;
    }

    apr_array_cat(&t->a, &s->a);
    o = (apreq_table_entry_t *)t->a.elts;
    t->ghosts += s->ghosts;

    if (t->flags & TF_BALANCE) {
        for (idx = n; idx < t->a.nelts; ++idx) {
            const unsigned char hash = TABLE_HASH(idx[o].checksum>>24);

            if (DEAD(idx))
                continue;

            if (idx[o].tree[NEXT] >= 0)
                idx[o].tree[NEXT] += n;

            if (t->root[hash] < 0) {
                if (idx[o].tree[LEFT] >= 0)
                    idx[o].tree[LEFT] += n;
                if (idx[o].tree[RIGHT] >= 0)
                    idx[o].tree[RIGHT] += n;
                if (idx[o].tree[UP] >= 0)
                    idx[o].tree[UP] += n;

                t->root[hash] = idx;
            }
            else if ( idx[o].tree[UP] >= 0 || s->root[hash] == idx-n ) 
                insert(o, &t->root[hash], t->root[hash], idx+o, TF_BALANCE);
        }
    }
    else {
        for (idx = 0; idx < TABLE_HASH_SIZE; ++idx)
            t->root[idx] = combine(o,t->root[idx],s->root[idx],n);
    }
}

APREQ_DECLARE(apreq_table_t *) apreq_table_overlay(apr_pool_t *p,
                                             const apreq_table_t *overlay,
                                             const apreq_table_t *base)
{
    apreq_table_t *t = apr_palloc(p, sizeof *t);

    t->a.pool = p;
    t->a.elt_size = sizeof(apreq_table_entry_t);
    t->copy = overlay->copy;
    t->merge = overlay->merge;
    t->flags = overlay->flags;
    t->ghosts = overlay->ghosts;

    memcpy(t->root, overlay->root, TABLE_HASH_SIZE * sizeof(int));

    t->a.elts = apr_palloc(p, t->a.elt_size *
                           (overlay->a.nalloc + base->a.nalloc));

    memcpy(t->a.elts, overlay->a.elts, overlay->a.elt_size * 
                                       overlay->a.nelts);

    t->a.nelts = overlay->a.nelts;
    t->a.nalloc = overlay->a.nalloc + base->a.nalloc;

    if (base->a.nelts) {
        if (t->a.nelts)
            apreq_table_cat(t, base);
        else
            memcpy(t->root, base->root, TABLE_HASH_SIZE * sizeof(int));
    }

    return t;
}


static apreq_value_t *collapse(apr_pool_t *p, 
                               const apr_array_header_t *arr)
{
    apreq_value_t **a = (apreq_value_t **)arr->elts;
    return (arr->nelts == 0) ? NULL : a[arr->nelts - 1];
}


APREQ_DECLARE(apr_status_t) apreq_table_overlap(apreq_table_t *a, 
                                                const apreq_table_t *b,
                                                const unsigned flags)
{

    const int m = a->a.nelts;
    const int n = b->a.nelts;
    apr_pool_t *p = b->a.pool;
    apr_status_t s;

    if (m + n == 0)
        return APR_SUCCESS;

    /* copy (extend) a using b's pool */
    if (a->a.pool != p) {
        make_array_core(&a->a, p, m+n, sizeof(apreq_table_entry_t), 0);
    }

    apreq_table_cat(a, b);

    if (flags == APR_OVERLAP_TABLES_SET) {
        apreq_value_merge_t *f = a->merge;
        a->merge = collapse;
        s = apreq_table_normalize(a);
        a->merge = f;
    }
    else
        s = apreq_table_normalize(a);

    return s;
}


/********************* iterators ********************/

APR_INLINE
APREQ_DECLARE(const apr_array_header_t *)apreq_table_elts(apreq_table_t *t)
{
    if (t->ghosts)
        apreq_table_exorcise(t);
    return (const apr_array_header_t *)t;
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
APREQ_DECLARE_NONSTD(int) apreq_table_do(apreq_table_do_callback_fn_t *comp,
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
                                   void *rec, const apreq_table_t *t, 
                                   va_list vp)
{
    char *argp;
    apreq_table_entry_t *o = (apreq_table_entry_t *)t->a.elts;
    int vdorv = 1;

    argp = va_arg(vp, char *);
    do {
        int rv = 1, idx;
        if (argp) {     /* Scan for entries that match the next key */
            idx = t->root[TABLE_HASH(*argp)];
            if ( search(o,&idx,argp) == 0 )
                while (idx >= 0) {
                    rv = (*comp) (rec, idx[o].val->name, v2c(idx[o].val));
                    idx = idx[o].tree[NEXT];
                }
        }
        else {          /* Scan the entire table */
            for (idx = 0; rv && (idx < t->a.nelts); ++idx)
                /* if (idx[o].key) */
                if (! DEAD(idx) )
                    rv = (*comp) (rec, idx[o].val->name, v2c(idx[o].val));
        }
        if (rv == 0) {
            vdorv = 0;
        }
    } while (argp && ((argp = va_arg(vp, char *)) != NULL));

    return vdorv;
}
