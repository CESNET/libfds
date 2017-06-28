#include <iostream>
#include "../../include/libfds/xml_parser.h"

int main() {
    fds_xml_t *parser;
    
    fds_xml_create(&parser);
    
    static const struct fds_xml_args args[] = {
            OPTS_ROOT("root"),
                //OPTS_ELEM(1, "name", OPTS_T_UINT, OPTS_P_OPT),
            OPTS_END
    };
    fds_xml_set_args(args, parser);

    const char *mem =
            "<root>"
                    "<name>300</name>"
            "</root>";
    fds_xml_parse(parser, mem, true);   
    
    fds_xml_destroy(parser);
}
