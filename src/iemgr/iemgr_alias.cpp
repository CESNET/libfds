#include "iemgr_common.h"
#include "iemgr_element.h"
#include "iemgr_alias.h"

#include <string>

#include <cassert>
#include <cstdlib>

static void
alias_destroy(fds_iemgr_alias *alias);

using unique_alias = std::unique_ptr<fds_iemgr_alias, decltype(&::alias_destroy)>;

static bool
check_valid_alias_name(const char *s)
{
    std::array<const char *, 6> prefixes { "in ", "out ", "ingress " ,"egress ", "src " ,"dst " };
    for (auto p : prefixes) {
        if (strncmp(s, p, strlen(p)) == 0) {
            s += strlen(p);
            break;
        }
    }
    return check_valid_name(s);
}

/**
 * Create a new empty alias
 */
static fds_iemgr_alias *
alias_create()
{
    fds_iemgr_alias *alias = (fds_iemgr_alias *) calloc(sizeof(fds_iemgr_alias), 1);
    return alias;
}

/**
 * Destroy the alias
 */ 
static void
alias_destroy(fds_iemgr_alias *alias)
{
    free(alias->name);
    for (size_t i = 0; i < alias->aliased_names_cnt; i++) {
        free(alias->aliased_names[i]);
    }
    free(alias->aliased_names);
    free(alias->sources);
    free(alias);
}

/**
 * Add aliased name to the alias
 */
static bool
alias_add_aliased_name(fds_iemgr_alias *alias, const char *name)
{
    char *name_copy = strdup(name);
    if (name_copy == nullptr) {
        return false;
    }

    char **ref = array_push(&alias->aliased_names, &alias->aliased_names_cnt);
    if (ref == nullptr) {
        free(name_copy);
        return false;
    }

    *ref = name_copy;
    return true;
}

/**
 * Add source element to the alias
 */
static bool
alias_add_source(fds_iemgr_alias *alias, fds_iemgr_elem *elem)
{
    fds_iemgr_elem **ref = array_push(&alias->sources, &alias->sources_cnt);
    if (ref == nullptr) {
        return false;
    }  
    *ref = elem;
    return true;
}

/**
 * Create a deep copy of the alias
 */
static fds_iemgr_alias *
alias_copy(fds_iemgr_alias *alias)
{
    auto copy = unique_alias(alias_create(), &::alias_destroy);
    if (copy == nullptr) {
        return nullptr;
    }
    for (size_t i = 0; i < alias->aliased_names_cnt; i++) {
        if (!alias_add_aliased_name(copy.get(), alias->aliased_names[i])) {
            return nullptr;
        }
    }
    for (size_t i = 0; i < alias->sources_cnt; i++) {
        if (!alias_add_source(copy.get(), alias->sources[i])) {
            return nullptr;
        }
    }
    return copy.release();
}

/**
 * Migrate the alias to another iemgr
 */
static void
alias_migrate_sources(fds_iemgr *mgr, fds_iemgr_alias *alias)
{
    for (size_t i = 0; i < alias->sources_cnt; i++) {
        fds_iemgr_elem *e = alias->sources[i];
        e = (fds_iemgr_elem *) fds_iemgr_elem_find_id(mgr, e->scope->pen, e->id);
        assert(e != nullptr);
        alias->sources[i] = e;
    }
}

/**
 * Save the alias to the mgr
 */
static int
alias_save_to_mgr(fds_iemgr *mgr, fds_iemgr_alias *alias)
{
    for (size_t i = 0; i < alias->aliased_names_cnt; i++) {
        if (fds_iemgr_alias_find(mgr, alias->aliased_names[i])) {
            mgr->err_msg = "Duplicate aliased name '" + std::string(alias->aliased_names[i]) + "'";
            return FDS_ERR_FORMAT;
        }
    }

    mgr->aliases.push_back(alias);
    for (size_t i = 0; i < alias->aliased_names_cnt; i++) {
        mgr->aliased_names.emplace_back(std::string(alias->aliased_names[i]), alias);
    }
    sort_vec(mgr->aliased_names);

    for (size_t i = 0; i < alias->sources_cnt; i++) {
        if (!element_add_alias_ref(alias->sources[i], alias)) {
            mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
    }

    return FDS_OK;
}

/**
 * Destroy aliases from iemgr
 */
void
aliases_destroy(fds_iemgr *mgr)
{
    for (fds_iemgr_alias *alias : mgr->aliases) {
        alias_destroy(alias);
    }
}

/**
 * Copy aliases from old iemgr to new iemgr
 */
int
aliases_copy(const fds_iemgr *old_mgr, fds_iemgr *new_mgr)
{
    for (fds_iemgr_alias *alias : old_mgr->aliases) {
        fds_iemgr_alias *copy = alias_copy(alias);
        if (copy == nullptr) {
            new_mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
        alias_migrate_sources(new_mgr, alias);
        int rc = alias_save_to_mgr(new_mgr, alias);
        if (rc != FDS_OK) {
            return rc;
        }
    }
    return FDS_OK;
}

fds_iemgr_alias *
find_alias_by_name(fds_iemgr *mgr, const char *name)
{
    for (fds_iemgr_alias *alias : mgr->aliases) {
        if (strcmp(alias->name, name) == 0) {
            return alias;
        }
    }
    return nullptr;
}

/*****************************************************************************
 * Parsing
 ****************************************************************************/

/**
 * Create parser for the alias XML config file 
 */
static fds_xml_t *
parser_create(fds_iemgr_t *mgr);

/**
 * Process the <element>, the child of the root element <ipfix-aliases>
 */
static int
read_element(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx);

/**
 * Process the <source> of an alias definition, the child of <element>
 */
static int
read_source(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_alias *alias);

int
fds_iemgr_alias_read_file(fds_iemgr_t *mgr, const char *file_path)
{
    // Open file
    auto file = unique_file(fopen(file_path, "r"), &::fclose);
    if (file == nullptr) {
        mgr->err_msg = "Cannot open file " + std::string(file_path) + ": " + std::strerror(errno);
        return FDS_ERR_NOTFOUND;
    }

    // Save modification time to the manager
    if (!mtime_save(mgr, file_path)) {
        return FDS_ERR_DENIED;
    }

    // Create parser
    auto parser = unique_parser(parser_create(mgr), &::fds_xml_destroy);
    if (parser == nullptr) {
        return FDS_ERR_NOMEM;
    }

    // Parse file using parser
    fds_xml_ctx_t *xml_ctx = fds_xml_parse_file(parser.get(), file.get(), false);
    if (xml_ctx == nullptr) {
        mgr->err_msg = fds_xml_last_err(parser.get());
        return FDS_ERR_DENIED;
    }

    // Read the alias file elements
    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        if (cont->id != ELEM) {
            continue;
        }

        int rc = read_element(mgr, cont->ptr_ctx);
        if (rc != FDS_OK) {
            return rc;
        }
    }

    return FDS_OK;
}

static fds_xml_t *
parser_create(fds_iemgr_t *mgr)
{
    fds_xml_t *parser = fds_xml_create();
    if (parser == NULL) {
        mgr->err_msg = ERRMSG_NOMEM;
        return nullptr;
    }

    static const struct fds_xml_args args_source[] = {
        FDS_OPTS_ATTR(SOURCE_MODE, "mode", FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_ELEM(SOURCE_ID, "id", FDS_OPTS_T_STRING, FDS_OPTS_P_MULTI),
        FDS_OPTS_END
    };

    static const struct fds_xml_args args_elem[] = {
        FDS_OPTS_ELEM(ELEM_NAME, "name", FDS_OPTS_T_STRING, 0),
        FDS_OPTS_ELEM(ELEM_ALIAS, "alias", FDS_OPTS_T_STRING, FDS_OPTS_P_MULTI),
        FDS_OPTS_NESTED(ELEM_SOURCE, "source", args_source, 0),
        FDS_OPTS_END
    };

    static const struct fds_xml_args args_main[] = {
        FDS_OPTS_ROOT("ipfix-aliases"),
        FDS_OPTS_NESTED(ELEM, "element", args_elem, FDS_OPTS_P_OPT | FDS_OPTS_P_MULTI),
        FDS_OPTS_END
    };

    if (fds_xml_set_args(parser, args_main) != FDS_OK) {
        mgr->err_msg = fds_xml_last_err(parser);
        fds_xml_destroy(parser);
        return nullptr;
    }
    
    return parser;
}

static int
read_element(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx)
{
    auto alias = unique_alias(alias_create(), &::alias_destroy);
    alias->mode = FDS_ALIAS_ANY_OF; // Default value
    int rc;
    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case ELEM_NAME:
            if (*cont->ptr_string == '\0') {
                mgr->err_msg = "Alias name cannot be empty.";
                return FDS_ERR_FORMAT;
            }
            alias->name = strdup(cont->ptr_string);
            if (alias->name == nullptr) {
                mgr->err_msg = ERRMSG_NOMEM;
                return FDS_ERR_NOMEM;
            }
            break;
        case ELEM_ALIAS:
            if (*cont->ptr_string == '\0') {
                mgr->err_msg = "Alias cannot be empty.";
                return FDS_ERR_FORMAT;
            }
            if (!check_valid_alias_name(cont->ptr_string)) {
                mgr->err_msg = 
                    "Invalid characters in alias '" + std::string(cont->ptr_string) + "'. "
                    "Aliases must only consist of alphanumeric characters and underscores and "
                    "must not begin with a number. Special prefixes 'src ', 'dst ', 'in ', 'out ', "
                    "'ingress ', 'egress ' are permitted.";
                return FDS_ERR_FORMAT;
            }
            if (!alias_add_aliased_name(alias.get(), cont->ptr_string)) {
                mgr->err_msg = ERRMSG_NOMEM;
                return FDS_ERR_NOMEM;
            }
            break;
        case ELEM_SOURCE:
            rc = read_source(mgr, cont->ptr_ctx, alias.get());
            if (rc != FDS_OK) {
                return rc;
            }
            break;
        }
    }
    rc = alias_save_to_mgr(mgr, alias.get());
    if (rc != FDS_OK) {
        return rc;
    }
    alias.release();
    return FDS_OK;
}


static int
read_source(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_alias *alias)
{
    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case SOURCE_MODE:
            if (strcasecmp(cont->ptr_string, "firstOf") == 0) {
                alias->mode = FDS_ALIAS_FIRST_OF;
            } else if (strcasecmp(cont->ptr_string, "anyOf") == 0) {
                alias->mode = FDS_ALIAS_ANY_OF;
            } else {
                mgr->err_msg = "Invalid value for source mode";
                return FDS_ERR_FORMAT;
            }
            break;
        case SOURCE_ID:
            fds_iemgr_elem *elem = (fds_iemgr_elem *) 
                fds_iemgr_elem_find_name(mgr, cont->ptr_string);
            if (elem == nullptr) {
                mgr->err_msg = "No element with name " + std::string(cont->ptr_string);
                return FDS_ERR_NOTFOUND;
            }
            if (!alias_add_source(alias, elem)) {
                mgr->err_msg = ERRMSG_NOMEM;
                return FDS_ERR_NOMEM;
            }
            break;
        }
    }
    return FDS_OK;
}
