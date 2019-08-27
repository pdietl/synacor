/**
 * cmc_string.h
 *
 * Creation Date: 17/07/2019
 *
 * Authors:
 * Leonardo Vencovsky (https://github.com/LeoVen)
 *
 */

/* A very simple fixed size string used by all collections' method to_string */

#ifndef CMC_STRING_H
#define CMC_STRING_H

#include <stddef.h>
#include <inttypes.h>

const size_t cmc_string_len = 200;

typedef struct cmc_string_s
{
    char s[200];
} cmc_string;

/* to_string formats */
const char *cmc_string_fmt_deque = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", front:%" PRIuMAX ", back:%" PRIuMAX " }";
const char *cmc_string_fmt_hashmap = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", load:%lf, cmp:%p, hash:%p }";
const char *cmc_string_fmt_hashset = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", load:%lf, cmp:%p, hash:%p }";
const char *cmc_string_fmt_heap = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", type:%s, cmp:%p }";
const char *cmc_string_fmt_linkedlist = "%s at %p { count:%" PRIuMAX ", head:%p, tail:%p }";
const char *cmc_string_fmt_list = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX " }";
const char *cmc_string_fmt_queue = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", front:%" PRIuMAX ", back:%" PRIuMAX " }";
const char *cmc_string_fmt_stack = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX " }";
const char *cmc_string_fmt_treemap = "%s at %p { root:%p, count:%" PRIuMAX ", cmp:%p }";
const char *cmc_string_fmt_treeset = "%s at %p { root:%p, count:%" PRIuMAX ", cmp:%p }";

const char *cmc_string_fmt_intervalheap = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", size:%" PRIuMAX ", count:%" PRIuMAX ", cmp:%p }";
const char *cmc_string_fmt_multimap = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", load:%lf, cmp:%p, hash:%p }";
const char *cmc_string_fmt_multiset = "%s at %p { buffer:%p, capacity:%" PRIuMAX ", count:%" PRIuMAX ", cardinality:%" PRIuMAX ", load:%lf, cmp:%p, hash:%p }";;

#endif /* CMC_STRING_H */
