# libnf2
New library for flow storage

## Use-cases

Simple use cases on how to use high-level API defined in libnf2.h.

### Reading
```C
FILE *f = fopen("exmaple.lnf", "r");
lnf_ctx_t *ctx = lnt_ctx_new(f, "/etc/lnf/elems", LNF_READ);

lnf_rec_t *rec = lnf_rec_init(ctx);

while(lnf_ctx_read(ctx, rec) != LNF_EOF) {
    uint8_t ipv4_src_addr[4];
    int result = lnf_rec_get(rec, 0, 8, ipv4_src_addr, sizeof(ipv4_src_addr));

    if (result != LNF_REC_OK) {
        WARNING(...);
        continue;
    }

    // ... do something with the address
}

lnf_rec_destroy(rec);
lnf_ctx_destroy(ctx);
```

### Writing
```C
FILE *f = fopen("exmaple.lnf", "w");
lnf_ctx_t *ctx = lnt_ctx_new(f, "/etc/lnf/elems", LNF_WRITE);

lnf_rec_t *rec = lnf_rec_init(ctx);
uint16_t dst_port = 42;
int result = 0;

result = lnf_rec_set(rec, 0, EXAMPLE_IPFIX_DST_PORT, (uint8_t *) dst_port, sizeof(dst_port));

// ...

result = lnf_ctx_write(ctx, rec);

// ...

lnf_rec_destroy(rec);
lnf_ctx_destroy(ctx);
```
