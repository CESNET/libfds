#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "error.h"

struct fds_filter_ast_node *
ast_node_create()
{
    struct fds_filter_ast_node *node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->op = FDS_FILTER_AST_NONE;
    node->left = NULL;
    node->right = NULL;
    node->identifier_id = 0;
    node->identifier_name = NULL;
    node->type = FDS_FILTER_TYPE_NONE;
    node->subtype = FDS_FILTER_TYPE_NONE;
	node->identifier_is_constant = 0;
    memset(&node->value, 0, sizeof(node->value));
    node->location.first_line = 0;
    node->location.last_line = 0;
    node->location.first_column = 0;
    node->location.last_column = 0;
    return node;
}

void 
ast_destroy(struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return;
    }
    // TODO: check based on node type
    // TODO: also destroy value etc
    free(node->left);
    free(node->right);
    free(node);
}

void 
ast_print(FILE *outstream, struct fds_filter_ast_node *node)
{
    if (node == NULL) {
        return; 
    }
    static int level = 0;
    for (int i = 0; i < level; i++) { 
        fprintf(outstream, "    "); 
    }
	fprintf(outstream, "(%s, ", ast_op_to_str(node->op));
    if (node->op == FDS_FILTER_AST_IDENTIFIER) {
        fprintf(outstream, "name: %s, id: %d, ", node->identifier_name, node->identifier_id, type_to_str(node->type));
	}
	fprintf(outstream, "type: %s, ", type_to_str(node->type));
	fprintf(outstream, "value: ", type_to_str(node->type));
	switch (node->type) {
	case FDS_FILTER_TYPE_BOOL:
		fprintf(outstream, "%s", node->value.int_ != 0 ? "true" : "false");
		break;
	case FDS_FILTER_TYPE_STR:
		fprintf(outstream, "%*s", node->value.string.length, node->value.string.chars);
		break;
	case FDS_FILTER_TYPE_INT:
		fprintf(outstream, "%d", node->value.int_);
		break;
	case FDS_FILTER_TYPE_UINT:
		fprintf(outstream, "%u", node->value.uint_);
		break;
	case FDS_FILTER_TYPE_IP_ADDRESS:
		if (node->value.ip_address.version == 4) {
			fprintf(outstream, "%d.%d.%d.%d", 
					node->value.ip_address.bytes[0], 
					node->value.ip_address.bytes[1], 
					node->value.ip_address.bytes[2], 
					node->value.ip_address.bytes[3]);
		} else {
			fprintf(outstream, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
					node->value.ip_address.bytes[0], 
					node->value.ip_address.bytes[1], 
					node->value.ip_address.bytes[2], 
					node->value.ip_address.bytes[3], 
					node->value.ip_address.bytes[4], 
					node->value.ip_address.bytes[5], 
					node->value.ip_address.bytes[6], 
					node->value.ip_address.bytes[7], 
					node->value.ip_address.bytes[8], 
					node->value.ip_address.bytes[9], 
					node->value.ip_address.bytes[10], 
					node->value.ip_address.bytes[11], 
					node->value.ip_address.bytes[12], 
					node->value.ip_address.bytes[13], 
					node->value.ip_address.bytes[14], 
					node->value.ip_address.bytes[15]);
		}
		break;
	case FDS_FILTER_TYPE_MAC_ADDRESS:
		fprintf(outstream, "%02x:%02x:%02x:%02x:%02x:%02x", 
				node->value.mac_address[0], 
				node->value.mac_address[1], 
				node->value.mac_address[2], 
				node->value.mac_address[3], 
				node->value.mac_address[4], 
				node->value.mac_address[5]);
		break;
	}
	fprintf(outstream, ")\n");
    level++;
    ast_print(outstream, node->left);
    ast_print(outstream, node->right);

    level--;
}
