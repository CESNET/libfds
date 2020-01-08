add_library(filter_obj OBJECT
    array.h
    common.h
    values.h

    operations.h
    operations/float.c operations/float.h
    operations/int.c operations/int.h
    operations/ip.c operations/ip.h
    operations/mac.c operations/mac.h
    operations/str.c operations/str.h
    operations/uint.c operations/uint.h
    operations/other.c operations/other.h

    error.h error.c

    ast_common.h 
    ast_semantic.c

    eval_common.h
    eval_generator.c
    eval_evaluator.c

    filter.h filter.c
    scanner.c scanner.h
    parser.c parser.h
    opts.c opts.h
)