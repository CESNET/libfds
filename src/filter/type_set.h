#ifndef FDS_FILTER_TYPE_SET_H
#define FDS_FILTER_TYPE_SET_H

#include "array.h"
#include "operations.h"

#define TYPE_SET_CAPACITY 32 // assume 32 casts for a data type is more than enough

struct type_set_s {
    int types[TYPE_SET_CAPACITY];
    int n_types;
};

static inline bool
has_type(struct type_set_s *ts, int type)
{
    for (int i = 0; i < ts->n_types; i++) {
        if (ts->types[i] == type) {
            return true;
        }
    }
    return false;
}

static inline void
add_type(struct type_set_s *ts, int type)
{
    if (!has_type(ts, type)) {
        assert(ts->n_types < TYPE_SET_CAPACITY);
        ts->types[ts->n_types++] = type;
    }
}

static inline void
set_intersect(struct type_set_s *ts1, struct type_set_s *ts2, struct type_set_s *out_ts)
{
    for (int i = 0; i < ts1->n_types; i++) {
        if (has_type(ts2, ts1->types[i])) {
            add_type(out_ts, ts1->types[i]);
        }
    }
}

static inline void
add_type_and_all_casts(struct array_s *operations, struct type_set_s *ts, int data_type)
{
    add_type(ts, data_type);

    operation_s *cast = NULL;
    while ((cast = find_next_cast(operations, cast, data_type)) != NULL) {
        add_type(ts, cast->out_data_type);
    }
}

static inline int
choose_best_type(struct array_s *operations, struct type_set_s *ts)
{
    // TODO: do this a better way?
    int best_type = ts->types[0];
    operation_s *best_type_cast = find_cast(operations, DT_ANY, ts->types[0]);
    for (int i = 1; i < ts->n_types; i++) {
        operation_s *cast = find_cast(operations, DT_ANY, ts->types[i]);
        if (cast < best_type_cast) {
            best_type_cast = cast;
            best_type = ts->types[i];
        }
    }
    return best_type;
}


#endif