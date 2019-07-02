#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

struct fds_filter_ast_node *
ast_node_create()
{
    struct fds_filter_ast_node *node = malloc(sizeof(node));
    if (node == NULL) {
        return NULL;
    }
    node->op = FDS_FILTER_AST_NONE;
    node->left = NULL;
    node->right = NULL;
    node->identifier_id = 0;
    node->identifier_name = NULL;
    node->type = FDS_FILTER_TYPE_NONE;
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
	fprintf(outstream, "(%s, ", fds_filter_op_str[node->op]);
    if (node->op == FDS_FILTER_AST_IDENTIFIER) {
        fprintf(outstream, "name: %s, id: %d, ", node->identifier.name, node->identifier.id, fds_filter_datatype_str[node->data.type]);
	}
	fprintf(outstream, "type: %s, ", fds_filter_datatype_str[node->data.type]);
	fprintf(outstream, "value: ", fds_filter_datatype_str[node->data.type]);
	switch (node->data.type) {
	case FDS_FILTER_TYPE_BOOL:
		fprintf(outstream, "%s", node->data.value.int_ != 0 ? "true" : "false");
		break;
	case FDS_FILTER_TYPE_STR:
		fprintf(outstream, "%*s", node->data.value.string.length, node->data.value.string.chars);
		break;
	case FDS_FILTER_TYPE_INT:
		fprintf(outstream, "%d", node->data.value.int_);
		break;
	case FDS_FILTER_TYPE_UINT:
		fprintf(outstream, "%u", node->data.value.uint_);
		break;
	case FDS_FILTER_TYPE_IP_ADDRESS:
		if (node->data.value.ip_address.version == 4) {
			fprintf(outstream, "%d.%d.%d.%d", 
					node->data.value.ip_address.bytes[0], 
					node->data.value.ip_address.bytes[1], 
					node->data.value.ip_address.bytes[2], 
					node->data.value.ip_address.bytes[3]);
		} else {
			fprintf(outstream, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
					node->data.value.ip_address.bytes[0], 
					node->data.value.ip_address.bytes[1], 
					node->data.value.ip_address.bytes[2], 
					node->data.value.ip_address.bytes[3], 
					node->data.value.ip_address.bytes[4], 
					node->data.value.ip_address.bytes[5], 
					node->data.value.ip_address.bytes[6], 
					node->data.value.ip_address.bytes[7], 
					node->data.value.ip_address.bytes[8], 
					node->data.value.ip_address.bytes[9], 
					node->data.value.ip_address.bytes[10], 
					node->data.value.ip_address.bytes[11], 
					node->data.value.ip_address.bytes[12], 
					node->data.value.ip_address.bytes[13], 
					node->data.value.ip_address.bytes[14], 
					node->data.value.ip_address.bytes[15]);
		}
		break;
	case FDS_FILTER_TYPE_MAC_ADDRESS:
		fprintf(outstream, "%02x:%02x:%02x:%02x:%02x:%02x", 
				node->data.value.mac_address[0], 
				node->data.value.mac_address[1], 
				node->data.value.mac_address[2], 
				node->data.value.mac_address[3], 
				node->data.value.mac_address[4], 
				node->data.value.mac_address[5]);
		break;
	}
	fprintf(outstream, ")\n");
    level++;
    ast_print(outstream, node->left);
    ast_print(outstream, node->right);

    level--;
}
