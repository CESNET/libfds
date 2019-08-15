# Filter

## Usage
```
int lookup_callback(struct fds_filter_lookup_args args) {
    ...
}

int data_callback(struct fds_filter_data_args args) {
    ...
}

fds_filter_t *filter;
if (!fds_filter_create(filter)) {
    ... error handling ...
}
fds_filter_set_lookup_callback(filter, lookup_callback);
fds_filter_set_data_callback(filter, data_callback);
struct my_context *context = malloc(sizeof (struct my_context));
if (!context) {
    ... error handling ...
}
fds_filter_set_context(filter, context);
if (!fds_filter_compile(filter, "ip 127.0.0.1 and port 80")) {
    ... error handling ...
}
...
if (fds_filter_evaluate(filter, data)) {
    printf("Pass\n");
} else {
    printf("Fail\n");
}
```

## TODO
- Correct support for ISO timestamps
- "Left side context" when looking up constant identifier?
- Consider mask when comparing ip addresses
- Use trie to efficiently check if IP address is in a list
- List concatenation (+ operator)?
- Correctly free allocated fds_filter_values in ast and eval tree
- Arithmetic operations POW and MOD
- Bitwise operations &, |, ^ (what about types?)
- Better regex for IPv6 addresses - might match too much now as identifiers can contain :
- ...