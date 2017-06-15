
Coding Style for Contributors
=============================

TL;DR;
------

Use ``clang-format`` tool (RPM package clang-analyzer, DEB package clang-format)
and the attached configuration file, ``.clang-format``, to format your source
code.

.. code-block:: bash

    clang-format -style=file src/my_file.cpp


Indentation and line length
---------------------------

Tabs should be used for line continuation and indentation - assume 4 columns
per tab stop. Spaces should be used to indent single line tailing comment after
a statement on the same line, but spaces at line ends must be avoided.
Line length is 80 (or less) characters.

**Do not use more than 3 (in neccessary case 4) indentation levels.**
You are doing probably something wrong if you need more levels of indentation.

The preferred way to ease multiple indentation levels in a ``switch``
statement is to align the switch and its subordinate case labels in the same
column:

.. code-block:: c

    switch (action) {
    case ACTION_A:
        return do_action_A();
    case ACTION_B:
        return do_action_B();
    case ACTION_C:
        return do_action_C();
    default:
        return 0;
    }


Naming
------

Variable and function names should be short and descriptive, written in
lower-case only. Camel case is therefore not allowed.

However, names of macros defining constants and labels in enums are capitalized.
Macros resembling functions may be named in lower case.

.. code-block:: c

    #define PI 3.14

    enum colors {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE
    };


All functions, typedefs, structures, unions and enums that represent public 
interface of a C module withing the same context must start with the same
prefix. For example, simple bit set functions:

.. code-block:: c

    bitset_t *
    bitset_create(size_t size);

    void
    bitset_destroy(bitset_t *set);

    void
    bitset_set(bitset_t *set, size_t idx, bool val);

    bool
    bitset_get(const bitset_t *set, size_t idx);


Braces and spaces
-----------------

Put the opening brace last on the line, and put the closing brace first.

.. code-block:: cpp

    if (var_a == var_b) {
        do_something_A();
    } else {
    	do_something_B();
    }

    struct my_struct {
        int x;
        int y;
    };

    union my_union {
        ...
    };

    class my_class {
    public:
        my_class();
        ~my_class();
    private:
        int i;
    };

However, all functions have opening brace at the beginning of the next line:

.. code-block:: c

    int
    my_function(int value)
    {
        ...
    }

Operands and operators must be separated by a single space:

.. code-block:: c

    msg->length = htons(len + 4);

Break multiline conditions before logical operators:

.. code-block:: c

    if (condition_A && condition_B
        && condition_C) {
        some_action();
        some_other_action();
    }

Similarly, break multiline expression before operators i.e. put the operators
at the beginning of the next line:

.. code-block:: c

    net_profit = gross_profit - overhead
        - cost_of_goods - payroll;



Function definitions
--------------------

Return type must be placed on a separated line before the function name. There
are no spaces between the function name and the parameter list.

.. code-block:: c

    struct ipfix_message *
    message_create_from_mem(void *msg, int len, struct input_info *input_info,
        int source_status)
    {
        some_action();
        some_other_action();
    }

malloc vs. calloc
-----------------

To ensure that all variables are initialized, the use of malloc should
be avoided, if possible, calloc is generally preferred in a C code. In case one
decides for using malloc, a comment must be added to explain the case.

Documentation and comments
--------------------------

Every file should have a copyright statement at the top, followed by import
statements and a code itself.

For documentation of functions, use Doxygen style comments with backshashes (\\).
Each description of a parameter should have an attritube specifying the
direction. Please note that names of parameters and their descriptions are
aligned in the same column.

.. code-block:: c

    /**
     * \brief Get a value of a float/double
     *
     * The \p value is read from a data \p field and converted from
     * "network byte order" to "host byte order".
     * \param[in]  field  Pointer to the data field (in "network byte order")
     * \param[in]  size   Size of the data field (min: 1 byte, max: 8 bytes)
     * \param[out] value  Pointer to a variable for the result
     * \return On success returns #IPX_CONVERT_OK and fills the \p value. Otherwise
     *   (usually the incorrect \p size of the field) returns #IPX_CONVERT_ERR_ARG
     *   and the \p value is not filled.
     */
    static int
    ipx_get_float(const void *field, size_t size, double *value);


In-body comments should tell *what* your code does, not how. The preferred 
styles for single and long (multi-line) comments are:

.. code-block:: c

    // Single-line comments should be used just for single lines, to comment code.

    /*
     * Multi-line comments should be used as soon as a comment
     * spans multiple lines.
     */

This is wrong:

.. code-block:: c

    /* Use single-line comment style instead this one. */


Other
-----

In case of any doubts or issues other than described in this document, the
`Linux kernel coding style
<https://www.kernel.org/doc/Documentation/process/coding-style.rst>`_ is leading.

