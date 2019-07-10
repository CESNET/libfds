#include <libfds.h>
#include <gtest/gtest.h>
#include <map>
#include <string>
#include <algorithm>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

struct Filter {
    struct identifier_data {
        int id = -1;
        fds_filter_type type = FDS_FILTER_TYPE_NONE;
        bool is_constant;
        std::vector<fds_filter_value> values;
    };
    std::map<std::string, identifier_data> identifiers;
    int identifier_count = 0;
    int counter = 0;
    const char *filter_expr;
    fds_filter_t *filter;

    static int lookup_callback(const char *name, void *data_context, int *id, fds_filter_type *type, int *is_constant, fds_filter_value *value) {
        Filter *filter = reinterpret_cast<Filter *>(data_context);
        if (filter->identifiers.find(name) == filter->identifiers.end()) {
            return 0;
        }

        identifier_data &data = filter->identifiers[name];
        *id = data.id;
        *type = data.type;
        *is_constant = data.is_constant ? 1 : 0;
        if (data.is_constant) {
            assert(data.values.size() == 1);
            *value = data.values[0];
        }
    }

    static int data_callback(int id, void *data_context, int reset_context, void *, union fds_filter_value *value) {
        Filter *filter = reinterpret_cast<Filter *>(data_context);
        // auto data = std::find_if(identifiers.begin(), identifiers.end(), [id](auto &p) -> bool {
        //     return p.second.id == id;
        // });
        identifier_data data;
        for (auto &p : filter->identifiers) {
            if (p.second.id == id) {
                data = p.second;
                break;
            }
        }
        assert(data.id != -1);
        // assert(data != identifiers.end());
        if (reset_context) {
            filter->counter = 0;
        }
        int n_values = data.values.size();
        if (filter->counter < n_values - 1) {
            *value = data.values[filter->counter];
            filter->counter++;
            return FDS_FILTER_MORE;
        } else if (filter->counter == n_values - 1) {
            *value = data.values[filter->counter];
            filter->counter++;
            return FDS_FILTER_END;
        } else {
            return FDS_FILTER_NOT_FOUND;
        }
    }

    Filter() {

    }

    Filter(const char *filter_expr) {
        this->filter_expr = filter_expr;
    }

    void set_expression(const char *filter_expr) {
        this->filter_expr = filter_expr;
    }

    void set_identifier(const char *name, fds_filter_type type, bool is_constant, std::vector<fds_filter_value> values) {
        identifier_data data;
        identifier_count++;
        data.id = identifier_count;
        data.type = type;
        data.values = values;
        data.is_constant = is_constant;
        identifiers[name] = data;
    }

    int compile() {
        int rc = fds_filter_create(filter_expr, lookup_callback, data_callback, this, &filter);
        if (!rc) {
            return 0;
        }
        return 1;
    }

    int evaluate() {
        return fds_filter_evaluate(filter, NULL);
    }

    int error_count() {
        return fds_filter_get_error_count(filter);
    }

    int compile_and_evaluate() {
        return compile() && evaluate();
    }
};

TEST(Filter, ip_multiple_values_1)
{
    Filter filter;
    filter.set_expression("ip 127.0.0.1");
    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } },
    });
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } },
    });
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
    });
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } }
    });
    EXPECT_FALSE(filter.compile_and_evaluate());
}

TEST(Filter, ip_multiple_values_2)
{
    Filter filter;
    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } },
    });
    filter.set_expression("not ip 127.0.0.1");
    EXPECT_TRUE(filter.compile());
    EXPECT_FALSE(filter.evaluate());
}

// ip 127.0.0.1 and port 80
// ip 127.0.0.1 or port 80
// not ip 127.0.0.1 and not port 80
// not ip 127.0.0.1 or not port 80
// not (ip 127.0.0.1 or port 80)

TEST(Filter, ip_port_multiple_values)
{
    Filter filter;
    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 192, 168, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 85, 123, 45, 6 } } },
    });
    filter.set_identifier("port", FDS_FILTER_TYPE_UINT, false, {
        (fds_filter_value) { .uint_ = 80 },
        (fds_filter_value) { .uint_ = 443 },
        (fds_filter_value) { .uint_ = 22 }
    });

    // Contains ip 127.0.0.1 and contains port 80
    filter.set_expression("ip 127.0.0.1 and port 80");
    EXPECT_TRUE(filter.compile_and_evaluate());

    // Contains ip 127.0.0.1 and does not contain port 80
    filter.set_expression("ip 127.0.0.1 and not port 80");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Contains ip 127.0.0.1 and does not contain port 60
    filter.set_expression("ip 127.0.0.1 and not port 60");
    EXPECT_TRUE(filter.compile_and_evaluate());

    // Contains ip 127.0.1.1 and does not contain port 60
    filter.set_expression("ip 127.0.1.1 and not port 60");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Does not contain ip 192.168.0.1 or does not contain port 443
    filter.set_expression("not ip 192.168.0.1 or not port 443");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Does not contain ip 192.168.0.1 or does not contain port 55
    filter.set_expression("not ip 192.168.0.1 or not port 55");
    EXPECT_TRUE(filter.compile_and_evaluate());
}

TEST(Filter, ip_port_undefined_field)
{
    Filter filter;
    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, { });
    filter.set_identifier("port", FDS_FILTER_TYPE_UINT, false, {
        (fds_filter_value) { .uint_ = 80 },
        (fds_filter_value) { .uint_ = 443 },
        (fds_filter_value) { .uint_ = 22 }
    });

    // Contains ip 127.0.0.1
    filter.set_expression("ip 127.0.0.1");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Contains ip 127.0.0.1 and contains port 80
    filter.set_expression("ip 127.0.0.1 and port 80");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Does not contain ip 127.0.0.1 and contains port 80
    filter.set_expression("not ip 127.0.0.1 and port 80");
    EXPECT_TRUE(filter.compile_and_evaluate());

    // Contains ip 127.0.0.1 and does not contain port 80
    filter.set_expression("ip 127.0.0.1 and not port 80");
    EXPECT_FALSE(filter.compile_and_evaluate());

    // Does not contain ip 192.168.0.1 or does not contain port 443
    filter.set_expression("not ip 192.168.0.1 or not port 443");
    EXPECT_TRUE(filter.compile_and_evaluate());
}