#ifndef FDS_FILTER_ARRAY_H
#define FDS_FILTER_ARRAY_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct array_s {
    int item_size;
    int capacity;
    int num_items;
    void *items;
};

static inline struct array_s
array_make(int item_size)
{
    return (struct array_s) {
        .item_size = item_size,
        .num_items = 0,
        .capacity = 0,
        .items = NULL
    };
}

static inline bool
array_reserve(struct array_s *array, int capacity)
{
    if (array->capacity >= capacity) {
        return true;
    }

    void *tmp = realloc(array->items, capacity * array->item_size);
    if (!tmp) {
        return false;
    }
    array->capacity = capacity;
    array->items = tmp;
    return true;
}

static inline void *
array_item_at(struct array_s *array, int index)
{
    return (char *)array->items + (index * array->item_size);
}

static inline void
array_set_items_at(struct array_s *array, int index, void *item, int num_items)
{
    memcpy(array_item_at(array, index), item, array->item_size * num_items);
}

static inline void
array_set_item_at(struct array_s *array, int index, void *item)
{
    array_set_items_at(array, index, item, 1);
}

static inline void
array_move_items(struct array_s *array, int from_index, int to_index, int num_items)
{
    memmove(array_item_at(array, to_index), array_item_at(array, from_index), array->item_size * num_items);
}

static inline bool
array_extend_front(struct array_s *array, void *item, int num_items)
{
    if (!array_reserve(array, array->num_items + num_items)) {
        return false;
    }
    array_move_items(array, 0, num_items, array->num_items);
    array_set_items_at(array, 0, item, num_items);
    array->num_items += num_items;
    return true;
}

static inline bool
array_push_front(struct array_s *array, void *item)
{
    return array_extend_front(array, item, 1);
}


static inline bool
array_extend_back(struct array_s *array, void *item, int num_items)
{
    if (!array_reserve(array, array->num_items + num_items)) {
        return false;
    }
    array_set_items_at(array, array->num_items, item, num_items);
    array->num_items += num_items;
    return true;
}

static inline bool
array_push_back(struct array_s *array, void *item)
{
    return array_extend_back(array, item, 1);
}

static inline void
array_destroy(struct array_s *array)
{
    free(array->items);
}

#define ARRAY_FOR_EACH(ARRAY, TYPE_NAME, LOOP_VAR) \
    for (TYPE_NAME *LOOP_VAR = array_item_at((ARRAY), 0); \
         LOOP_VAR != array_item_at((ARRAY), (ARRAY)->num_items); \
         LOOP_VAR++)

#define ARRAY_FOR_CONTINUE(ARRAY, PREV, TYPE_NAME, LOOP_VAR) \
    for (TYPE_NAME LOOP_VAR = (PREV) ? (PREV) + 1 : array_item_at((ARRAY), 0); \
         LOOP_VAR != array_item_at((ARRAY), (ARRAY)->num_items); \
         LOOP_VAR++)

#define ARRAY_FIND(ARRAY, VAR, COND) \
    { \
        int found = 0; \
        for ((VAR) = array_item_at((ARRAY), 0); \
            (VAR) != array_item_at((ARRAY), (ARRAY)->num_items); \
            (VAR)++) { \
                if ((COND)) { found = 1; break; } \
            } \
            if (!found) { (VAR) = NULL; } \
    }

#endif