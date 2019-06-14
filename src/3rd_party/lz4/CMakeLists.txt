# Build a static library with LZ4 algorithm
set(LZ4_SRC
    lz4.h
    lz4.c
)

add_library(fds_lz4 STATIC ${LZ4_SRC})
