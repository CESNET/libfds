#ifndef LIBFDS_TEST_FILTER_WRAPPER_H
#define LIBFDS_TEST_FILTER_WRAPPER_H

#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <initializer_list>
#include <libfds.h>
#include <algorithm>
#include <iterator>
#include <netinet/ether.h>
#include <arpa/inet.h>
#include <gtest/gtest.h>

struct Value {
    enum fds_filter_data_type type;
    enum fds_filter_data_type subtype;
    union fds_filter_value value;

    void clear() {
        type = FDS_FDT_NONE;
        subtype = FDS_FDT_NONE;
    }

    Value() {
        clear();
    }

    Value(const Value &other) {
        type = other.type;
        subtype = other.subtype;
        if (other.type == FDS_FDT_LIST) {
            value.list.items = new union fds_filter_value[other.value.list.length];
            for (int i = 0; i < other.value.list.length; i++) {
                if (other.subtype == FDS_FDT_STR) {
                    value.list.items[i].string.chars =
                        new char[other.value.list.items[i].string.length];
                    std::memcpy(value.list.items[i].string.chars,
                        other.value.list.items[i].string.chars,
                        other.value.list.items[i].string.length);
                    value.list.items[i].string.length = other.value.list.items[i].string.length;
                } else {
                    std::memcpy(&value.list.items[i], &other.value.list.items[i],
                        sizeof(union fds_filter_value));
                }
            }
            value.list.length = other.value.list.length;
        } else if (other.type == FDS_FDT_STR) {
            value.string.chars = new char[other.value.string.length];
            std::memcpy(value.string.chars, other.value.string.chars, other.value.string.length);
            value.string.length = other.value.string.length;
        } else {
            std::memcpy(&value, &other.value, sizeof(union fds_filter_value));
        }
    }

    Value(Value &&other) {
        type = other.type;
        subtype = other.subtype;
        std::memcpy(&value, &other.value, sizeof(union fds_filter_value));
        other.clear();
    }

    ~Value() {
        if (type == FDS_FDT_LIST) {
            if (subtype == FDS_FDT_STR) {
                for (int i = 0; i < value.list.length; i++) {
                    delete[] (value.list.items[i].string.chars);
                }
            }
            delete[] (value.list.items);
        } else if (type == FDS_FDT_STR) {
            delete[] (value.string.chars);
        }
    }

    static Value Int(int64_t i) {
        Value v;
        v.type = FDS_FDT_INT;
        v.value.int_ = i;
        return v;
    }

    static Value UInt(uint64_t u) {
        Value v;
        v.type = FDS_FDT_UINT;
        v.value.uint_ = u;
        return v;
    }

    static Value Float(double f) {
        Value v;
        v.type = FDS_FDT_FLOAT;
        v.value.float_ = f;
        return v;
    }

    static Value String(std::string s) {
        Value v;
        v.type = FDS_FDT_STR;
        v.value.string.length = std::strlen(s.c_str());
        v.value.string.chars = new char[v.value.string.length];
        std::memcpy(v.value.string.chars, s.c_str(), v.value.string.length);
        return v;
    }

    static Value Mac(std::string mac) {
        Value v;
        v.type = FDS_FDT_MAC_ADDRESS;
        struct ether_addr eaddr;
        ether_aton_r(mac.c_str(), &eaddr);
        std::memcpy(v.value.mac_address, eaddr.ether_addr_octet, 6);
        return v;
    }

    static Value Ip(std::string ip) {
        Value v;
        v.type = FDS_FDT_IP_ADDRESS;

        v.value.ip_address.version = 6;
        if (ip.find_first_of('.') != std::string::npos) {
            v.value.ip_address.version = 4;
        }

        v.value.ip_address.prefix_length = v.value.ip_address.version == 4 ? 32 : 128;
        auto slash_pos = ip.find_first_of('/');
        if (slash_pos != std::string::npos) {
            std::string prefix_length = std::string(ip.begin() + slash_pos + 1, ip.end());
            v.value.ip_address.prefix_length = std::stoi(prefix_length);
            ip = std::string(ip.begin(), ip.begin() + slash_pos);
        }

        int return_code = inet_pton(v.value.ip_address.version == 4 ? AF_INET : AF_INET6,
            ip.c_str(), &v.value.ip_address.bytes);
        if (return_code != 1) {
            throw std::runtime_error("invalid ip address");
        }

        return v;
    }

    static Value List(std::initializer_list<Value> list) {
        Value v;
        v.type = FDS_FDT_LIST;
        auto list_it = list.begin();
        v.subtype = list_it->type;
        v.value.list.length = list.size();
        v.value.list.items = new union fds_filter_value[list.size()];
        for (int i = 0; i < list.size(); i++) {
            std::memcpy(&v.value.list.items[i], &list_it->value, sizeof(union fds_filter_value));
            ((Value *)list_it)->clear();
            list_it++;
        }
        return v;
    }

};



class Filter : public ::testing::Test {
protected:
    fds_filter_t *filter = NULL;

    int last_id = 0;

    int last_field_n = 0;

    struct constant_info {
        std::string name;
        int id;
        Value value;
    };

    struct field_info {
        std::string name;
        int id;
        std::vector<Value> values;
    };

    std::vector<constant_info> constants;

    std::vector<field_info> fields;

    std::string expr;

    void reset() {
        TearDown();
        SetUp();
    }

    void SetUp() override {
        filter = fds_filter_create();
        if (filter == NULL) {
            throw std::runtime_error("error creating filter");
        }
        fds_filter_set_lookup_callback(filter, lookup_callback);
        fds_filter_set_const_callback(filter, const_callback);
        fds_filter_set_field_callback(filter, field_callback);
        fds_filter_set_user_context(filter, this);
    }

    void TearDown() override {
        fds_filter_destroy(filter);
        filter = nullptr;
    }

    void expression(std::string expr) {
        this->expr = expr;
    }

    void constant(std::string name, Value value) {
        assert(
            std::find_if(constants.begin(), constants.end(),
            [name](constant_info &x) { return x.name == name; }) == constants.end()
        );
        constants.push_back({ name, ++last_id, value });
    }

    void field(std::string name, Value value) {
        auto field = std::find_if(fields.begin(), fields.end(),
            [name](field_info &x) { return x.name == name; });
        if (field != fields.end()) {
            field->values.push_back(value);
        } else {
            fields.push_back({ name, ++last_id, { value } });
        }
    }

    int compile() {
        reset();
        int return_code = fds_filter_compile(filter, expr.c_str());
        return return_code;
    }

    int compile(std::string expr) {
        expression(expr);
        return compile();
    }

    int evaluate() {
        int return_code = fds_filter_evaluate(filter, NULL);
        return return_code;
    }

    int evaluate(std::string expr) {
        expression(expr);
        int return_code;
        return_code = compile();
        if (return_code != FDS_FILTER_OK) {
            return return_code;
        }
        return_code = evaluate();
        return return_code;
    }

    static int
    lookup_callback(const char *name, void *user_context, struct fds_filter_identifier_attributes *attributes)
    {
        Filter *filter = reinterpret_cast<Filter *>(user_context);

        for (auto &v : filter->constants) {
            if (std::strcmp(v.name.c_str(), name) == 0) {
                attributes->id = v.id;
                attributes->identifier_type = FDS_FIT_CONST;
                attributes->data_type = v.value.type;
                attributes->data_subtype = v.value.subtype;
                return FDS_FILTER_OK;
            }
        }

        for (auto &v : filter->fields) {
            if (std::strcmp(v.name.c_str(), name) == 0) {
                attributes->id = v.id;
                attributes->identifier_type = FDS_FIT_FIELD;
                attributes->data_type = v.values[0].type;
                attributes->data_subtype = v.values[0].subtype;
                return FDS_FILTER_OK;
            }
        }

        return FDS_FILTER_FAIL;
    }

    static void
    const_callback(int id, void *user_context, union fds_filter_value *value)
    {
        Filter *filter = reinterpret_cast<Filter *>(user_context);
        for (auto &v : filter->constants) {
            if (id == v.id) {
                std::memcpy(value, &v.value.value, sizeof(union fds_filter_value));
                return;
            }
        }
    }

    static int
    field_callback(int id, void *user_context, int reset_flag, void *input_data, union fds_filter_value *value)
    {
        Filter *filter = reinterpret_cast<Filter *>(user_context);
        if (reset_flag) {
            filter->last_field_n = 0;
        }

        auto field = std::find_if(filter->fields.begin(), filter->fields.end(),
            [id](field_info &x) { return x.id == id; });
        if (field == filter->fields.end() || field->values.size() == filter->last_field_n) {
            return FDS_FILTER_FAIL;
        }

        std::memcpy(value, &field->values[filter->last_field_n].value, sizeof(union fds_filter_value));
        filter->last_field_n++;

        return field->values.size() == filter->last_field_n ? FDS_FILTER_OK : FDS_FILTER_OK_MORE;
    }
};

#endif