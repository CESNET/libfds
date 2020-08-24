# Filter

General filtering component used for filtration of flow data records.


Supported operations
--------------------

 - Comparison operators `==`, `<`, `>`, `<=`, `>=`, `!=`. If the comparison operator is ommited, the default comparison is `==`.

 - The `contains` operator for substring comparison, e.g. `DNSName contains "example"`.
 
 - Arithmetic operations `+`, `-`, `*`, `/`, `%`.
 
 - Bitwise operations not `~`, or `|`, and `&`, xor `^`.
 
 - The `in` operator for list comparison, e.g. `port in [80, 443]`.

 - The logical `and`, `or`, `not` operators. 


Value types
-----------

 - Numbers can be integer or floating point. Integer numbers can also be written in their hexadecimal or binary form using the `0x` or `0b` prefix. 
   Floating point numbers also support the exponential notation such as `1.2345e+2`. A number can be explicitly unsigned using the `u` suffix.
   Numbers also support size suffixes `B`, `k`, `M`, `G`, `T`, and time suffixes `ns`, `us`, `ms`, `s`, `m`, `d`.

 - Strings are values enclosed in a pair of double quotes `"`. Supported escape sequences are `\n`, `\r`, `\t` and `\"`. 
   The escape sequences to write characters using their octal or hexadecimal value are also supported, e.g. `\ux22` or `\042`.
   
 - IP addresses are written in their usual format, e.g. `127.0.0.1` or `1234:5678:9abc:def1:2345:6789:abcd:ef12`. The shortened IPv6 version is also supported, e.g. `::ff`.
   IP addresses can also contain a suffix specifying their prefix length, e.g. `10.0.0.0/16`. 
 
 - MAC addresses are written in their usual format, e.g. `12:34:56:78:9a:bc`.

 - Timestamps use the ISO timestamp format, e.g. `2020-04-05T24:00Z`.