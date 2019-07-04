#include "evaluate.h"
#include <assert.h>

void
evaluate_context_init(struct evaluate_context *ec)
{
    ec->program_counter = 0;
    ec->stack_top = -1;
    ec->stack_capacity = 0;
    ec->instructions_count = 0;
    ec->instructions = NULL;
}

void
evaluate_context_destroy(struct evaluate_context *ec)
{
    free(ec->instructions);
}

int
instruction_add(struct evaluate_context *ec, int opcode, union fds_filter_value arg) 
{
    struct instruction *new_instructions = realloc(ec->instructions, sizeof(ec->instructions) * (ec->instructions_count + 1));
    if (new_instructions == NULL) {
        return 0;
    }
    ec->instructions = new_instructions;
    ec->instructions[ec->instructions_count] = (struct instruction) { opcode, arg };
    ec->instructions_count++;
    return 1;
}

static inline void
stack_push(struct evaluate_context *ec, union fds_filter_value value)
{
    assert(ec->stack_top < ec->stack_capacity);
    ec->stack_top++;
    ec->stack[ec->stack_top] = value;
}

static inline union fds_filter_value
stack_pop(struct evaluate_context *ec)
{
    assert(ec->stack_top > -1);
    ec->stack_top--;
    return ec->stack[ec->stack_top + 1];
}

static inline union fds_filter_value *
stack_top(struct evaluate_context *ec)
{
    assert(ec->stack_top > -1);
    return &ec->stack[ec->stack_top];
}

int
evaluate(struct fds_filter *filter, struct evaluate_context *ec, void *data)
{
    int program_counter = 0;
    while (program_counter < ec->instructions_count) {
        struct instruction instruction = ec->instructions[program_counter];
        program_counter++;
        switch (instruction.opcode) {
        
        case LOAD_CONST:
        {
            stack_push(ec, instruction.arg);
            break;
        }
        
        case LOAD_IDENTIFIER:
        {
            // TODO
            break;
        }

        case JUMP_IF_MORE:
        {
            // TODO
            break;
        }

        case JUMP_IF_TRUE:
        {
            // TODO
            break;
        }

        case JUMP_IF_FALSE:
        {
            // TODO
            break;
        }

        case ADD_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ += b.uint_;
            stack_push(ec, a);
            break;
        }

        case SUB_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ -= b.uint_;
            stack_push(ec, a);
            break;
        }

        case MUL_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ *= b.uint_;
            stack_push(ec, a);
            break;
        }

        case DIV_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ /= b.uint_;
            stack_push(ec, a);
            break;
        }

        case CAST_UINT_TO_INT:
        {
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.uint_;
            stack_push(ec, a);
            break;
        }

        case CAST_UINT_TO_FLOAT:
        {
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ == b.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_NE_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ != b.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_GT_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ > b.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_LT_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ < b.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_GE_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ >= b.uint_;
            stack_push(ec, a);
            break;
        }

        case CMP_LE_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.uint_ <= b.uint_;
            stack_push(ec, a);
            break;
        }

        case ADD_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ + b.int_;
            stack_push(ec, a);
            break;
        }

        case SUB_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ - b.int_;
            stack_push(ec, a);
            break;
        };

        case MUL_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ * b.int_;
            stack_push(ec, a);
            break;
        }

        case DIV_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ / b.int_;
            stack_push(ec, a);
            break;
        };

        case NEGATE_INT:
        {
            union fds_filter_value a = stack_pop(ec);
            a.int_ = -a.int_;
            stack_push(ec, a);
            break;
        }

        case CAST_INT_TO_FLOAT:
        {
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ == b.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_NE_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ != b.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_GT_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ > b.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_LT_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ < b.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_GE_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ >= b.int_;
            stack_push(ec, a);
            break;
        }

        case CMP_LE_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.int_ = a.int_ <= b.int_;
            stack_push(ec, a);
            break;
        }

        case ADD_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.float_ + b.float_;
            stack_push(ec, a);
            break;
        }

        case SUB_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.float_ - b.float_;
            stack_push(ec, a);
            break;
        }

        case MUL_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.float_ * b.float_;
            stack_push(ec, a);
            break;
        }

        case DIV_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.float_ = a.float_ / b.float_;
            stack_push(ec, a);
            break;
        }

        case NEGATE_FLOAT:
        {
            union fds_filter_value a = stack_pop(ec);
            a.float_ = -a.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ == b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_NE_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ != b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_GT_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ > b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_LT_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ < b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_GE_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ >= b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_LE_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.float_ <= b.float_;
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_IP_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.ip_address.version == b.ip_address.version && (a.ip_address.version == 4 ?
						memcmp(a.ip_address.bytes, b.ip_address.bytes, 4) == 0 : memcmp(a.ip_address.bytes, b.ip_address.bytes, 16) == 0);
            stack_push(ec, a);
            break;
        }

        case CMP_NE_IP_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = a.ip_address.version == b.ip_address.version && (a.ip_address.version == 4 ?
						memcmp(a.ip_address.bytes, b.ip_address.bytes, 4) != 0 : memcmp(a.ip_address.bytes, b.ip_address.bytes, 16) != 0);
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_MAC_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = memcmp(a.mac_address, b.mac_address, 6) == 0;
            stack_push(ec, a);
            break;
        }

        case CMP_NE_MAC_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
            a.uint_ = memcmp(a.mac_address, b.mac_address, 6) != 0;
            stack_push(ec, a);
            break;
        }

        case STR_CONTAINS_STR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case CONCAT_STR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case CMP_EQ_STR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case CMP_NE_STR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_STR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_INT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_UINT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_FLOAT:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_IP_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        case LIST_CONTAINS_MAC_ADDR:
        {
            union fds_filter_value b = stack_pop(ec);
            union fds_filter_value a = stack_pop(ec);
			// TODO
            stack_push(ec, a);
            break;
        }

        }
    }
}
