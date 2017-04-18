# libfds
Library for flow data storage

## Use-cases

Simple use cases on how to use high-level API defined in libfds.h.

### Reading
```C
FILE *f = fopen("example.fds", "r");

fds_ctx_t *ctx = NULL;
int err = 0;

err = lnt_ctx_new(f, fds_READ, &ctx);

if (err != 0) {
    ERROR(...);
}

fds_rec_t *rec = NULL;
err = fds_rec_init(ctx, &rec);

if (err != 0) {
    ERROR(...);
}

while(fds_ctx_read(ctx, rec) != fds_EOF) {
    uint8_t ipv4_src_addr[4];
    int result = fds_rec_get(rec, 0, 8, ipv4_src_addr, sizeof(ipv4_src_addr));

    if (result != fds_REC_OK) {
        WARNING(...);
        continue;
    }

    // ... do something with the address
}

fds_rec_destroy(rec);
fds_ctx_destroy(ctx);

free(rec);
free(ctx);
```

### Writing
```C
FILE *f = fopen("example.fds", "w");

fds_ctx_t *ctx = NULL;
int err = 0;

err = lnt_ctx_new(f, fds_WRITE, &ctx);

if (err != 0) {
    ERROR(...);
}

fds_rec_t *rec = NULL;
err = fds_rec_init(ctx, &rec);

if (err != 0) {
    ERROR(...);
}

uint16_t dst_port = 42;
int result = 0;

result = fds_rec_set(rec, 0, EXAMPLE_IPFIX_DST_PORT, (uint8_t *) dst_port, sizeof(dst_port));

// ...

result = fds_ctx_write(ctx, rec);

// ...

fds_rec_destroy(rec);
fds_ctx_destroy(ctx);

free(rec);
free(ctx);
```
