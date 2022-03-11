set(IPFIX_FILTER_SRC
    ipfix_filter.c
)

add_library(ipfix_filter_obj OBJECT ${IPFIX_FILTER_SRC})