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

    static int lookup_callback(struct fds_filter_lookup_args args) {
        Filter *filter = reinterpret_cast<Filter *>(args.context);
        if (filter->identifiers.find(args.name) == filter->identifiers.end()) {
            return 0;
        }
        identifier_data &data = filter->identifiers[args.name];
        *args.id = data.id;
        *args.type = data.type;
        *args.is_constant = data.is_constant ? 1 : 0;
        if (data.is_constant) {
            assert(data.values.size() == 1);
            *args.output_value = data.values[0];
        }
        return 1;
    }

    static int data_callback(struct fds_filter_data_args args) {
        Filter *filter = reinterpret_cast<Filter *>(args.context);
        identifier_data data;
        int found = 0;
        for (auto &p : filter->identifiers) {
            if (p.second.id == args.id) {
                data = p.second;
                found = 1;
                break;
            }
        }
        assert(found);
        if (args.reset) {
            filter->counter = 0;
        }
        int n_values = data.values.size();
        if (filter->counter >= n_values) {
            return 0;
        }
        *args.output_value = data.values[filter->counter];
        filter->counter++;
        *args.has_more = filter->counter < n_values;
        return 1;
    }

    Filter() {
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
        filter = fds_filter_create();
        if (filter == NULL) {
            return 0;
        }
        fds_filter_set_lookup_callback(filter, lookup_callback);
        fds_filter_set_data_callback(filter, data_callback);
        fds_filter_set_context(filter, this);
        int rc = fds_filter_compile(filter, filter_expr);
        fds_filter_print_errors(filter, stderr);
        return rc;
    }

    int evaluate() {
        int rc = fds_filter_evaluate(filter, NULL);
        fds_filter_print_errors(filter, stderr);
        return rc;
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

TEST(Filter, arithmetic)
{
    Filter filter;
    filter.set_identifier("a", FDS_FILTER_TYPE_UINT, true, // Const
                          { (fds_filter_value) { .uint_ = 10 } });
    filter.set_identifier("b", FDS_FILTER_TYPE_UINT, true, // Const
                          { (fds_filter_value) { .uint_ = 20 } });
    filter.set_identifier("c", FDS_FILTER_TYPE_UINT, false, // Not const
                          { (fds_filter_value) { .uint_ = 30 } });

    filter.set_expression("10 + 20 == 30");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("(10 * 20) + 30 > 100");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("a + b == c");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("(a * b) + c > 100");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("60 * (a * b) + c > 100");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("60 * ((a * b) + c) > 100");
    EXPECT_TRUE(filter.compile_and_evaluate());
}

TEST(Filter, list)
{
    Filter filter;
    filter.set_identifier("a", FDS_FILTER_TYPE_UINT, true, // Const
                          { (fds_filter_value) { .uint_ = 10 } });
    filter.set_identifier("b", FDS_FILTER_TYPE_UINT, true, // Const
                          { (fds_filter_value) { .uint_ = 20 } });
    filter.set_identifier("c", FDS_FILTER_TYPE_UINT, false, // Not const
                          { (fds_filter_value) { .uint_ = 30 } });

    filter.set_expression("10 in [10, 20, 30]");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("10 in [20, 10, 30]");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("10 in [20, 30, 10]");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("10 in [a, b, c]");
    EXPECT_FALSE(filter.compile());

    filter.set_expression("10 in [a, b]");
    EXPECT_TRUE(filter.compile_and_evaluate());

    filter.set_expression("10 in []");
    EXPECT_TRUE(filter.compile());
    EXPECT_FALSE(filter.evaluate());

    filter.set_expression("127.0.0.1 in [192.168.0.1, 127.0.0.1]");
    EXPECT_TRUE(filter.compile_and_evaluate());
}

TEST(Filter_scanner, identifiers_with_space)
{
    Filter filter;
    filter.set_identifier("src ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 32, .bytes = { 127, 0, 0, 1 } } },
    });
    filter.set_expression("src ip 127.0.0.1");
    EXPECT_TRUE(filter.compile());
    EXPECT_TRUE(filter.evaluate());

    filter.set_expression("not src ip 127.0.0.2");
    EXPECT_TRUE(filter.compile());
    EXPECT_TRUE(filter.evaluate());
}

TEST(Filter, ipv4_address_with_mask)
{
    Filter filter;
    filter.set_identifier("ip", FDS_FILTER_TYPE_IP_ADDRESS, false, {
        (fds_filter_value) { .ip_address = { .version = 4, .mask = 24, .bytes = { 192, 168, 0, 1 } } },
    });
    filter.set_expression("ip 192.168.0.0/24");
    EXPECT_TRUE(filter.compile());
    EXPECT_TRUE(filter.evaluate());
}