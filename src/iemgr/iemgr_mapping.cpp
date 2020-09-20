#include "iemgr_mapping.h"
#include "iemgr_common.h"
#include "iemgr_alias.h"
#include "iemgr_element.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

/******************************************************************************
 * Mapping
 ******************************************************************************/

static void
mapping_destroy(fds_iemgr_mapping *m);

static void
mapping_item_destroy_fields(fds_iemgr_mapping_item *item);

using unique_mapping = std::unique_ptr<fds_iemgr_mapping, decltype(&::mapping_destroy)>;
using unique_mapping_item = std::unique_ptr<fds_iemgr_mapping_item, decltype(&::mapping_item_destroy_fields)>;

static fds_iemgr_mapping *
mapping_create()
{
    fds_iemgr_mapping *m = (fds_iemgr_mapping *) calloc(1, sizeof(fds_iemgr_mapping));
    return m;
}

static void
mapping_destroy(fds_iemgr_mapping *m)
{
    free(m->name);
    for (size_t i = 0; i < m->items_cnt; i++) {
        free(m->items[i].key);
    }
    free(m->items);
    free(m->elems);
}

static bool
mapping_add_elem(fds_iemgr_mapping *m, fds_iemgr_elem *elem)
{
    fds_iemgr_elem **elem_ptr = array_push(&m->elems, &m->elems_cnt);
    if (elem_ptr == nullptr) {
        return false;
    }
    *elem_ptr = elem;
    return true;
}

static bool
mapping_add_alias(fds_iemgr_mapping *m, fds_iemgr_alias *alias)
{
    for (size_t i = 0; i < alias->sources_cnt; i++) {
        if (!mapping_add_elem(m, alias->sources[i])) {
            return false;
        }
    }
    return true;
}

static bool
mapping_add_item(fds_iemgr_mapping *m, fds_iemgr_mapping_item item)
{
    fds_iemgr_mapping_item *item_ptr = array_push(&m->items, &m->items_cnt);
    if (item_ptr == nullptr) {
        return false;
    }
    *item_ptr = item;
    return true;
}

static bool
mapping_item_copy(fds_iemgr_mapping_item *src, fds_iemgr_mapping_item *dst)
{
    memcpy(dst, src, sizeof(fds_iemgr_mapping_item));
    dst->key = strdup(src->key);
    if (dst->key == nullptr) {
        return false;
    }
    return true;
}

static void
mapping_item_destroy_fields(fds_iemgr_mapping_item *item)
{
    free(item->key);
}

static fds_iemgr_mapping *
mapping_copy(fds_iemgr_mapping *m)
{
    auto holder = unique_mapping(mapping_create(), &::mapping_destroy);
    fds_iemgr_mapping *m_new = holder.get();
    
    m_new->name = strdup(m->name);
    if (m_new->name == nullptr) {
        return nullptr;
    }
    
    m_new->key_case_sensitive = m->key_case_sensitive;
    
    for (size_t i = 0; i < m->items_cnt; i++) {
        fds_iemgr_mapping_item item;
        if (!mapping_item_copy(&m->items[i], &item)) {
            return nullptr;
        }
        if (!mapping_add_item(m_new, item)) {
            mapping_item_destroy_fields(&item);
            return nullptr;
        }
    }

    m_new->elems = copy_flat_array(m->elems, m->elems_cnt);
    if (m_new->elems == nullptr) {
        return nullptr;
    }
    m_new->elems_cnt = m->elems_cnt;

    return holder.release();    
}

static void
mapping_migrate_elems(fds_iemgr *mgr, fds_iemgr_mapping *m)
{
    for (size_t i = 0; i < m->elems_cnt; i++) {
        m->elems[i] = (fds_iemgr_elem *)
            fds_iemgr_elem_find_id(mgr, m->elems[i]->scope->pen, m->elems[i]->id);
        assert(m->elems[i] != nullptr);
    }
}

static int
mapping_save_to_mgr(fds_iemgr *mgr, fds_iemgr_mapping *m)
{
    for (size_t i = 0; i < m->elems_cnt; i++) {
        if (!element_add_mapping_ref(m->elems[i], m)) {
            mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
    }
    return FDS_OK;
}

static int
mapping_add_match(fds_iemgr *mgr, fds_iemgr_mapping *m, const char *match_name)
{
    fds_iemgr_alias *alias = find_alias_by_name(mgr, match_name);
    if (alias != nullptr) {
        if (!mapping_add_alias(m, alias)) {
            mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
        return FDS_OK;
    }

    fds_iemgr_elem *elem = (fds_iemgr_elem *) fds_iemgr_elem_find_name(mgr, match_name);
    if (elem != nullptr) {
        if (!mapping_add_elem(m, elem)) {
            mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
        return FDS_OK;
    }

    mgr->err_msg = "No matching alias or element found with name " + std::string(match_name);
    return FDS_ERR_NOTFOUND;
}

int
mappings_copy(const fds_iemgr *old_mgr, fds_iemgr *new_mgr)
{
    for (fds_iemgr_mapping *m : old_mgr->mappings) {
        unique_mapping copy = unique_mapping(mapping_copy(m), &::mapping_destroy);
        if (copy.get() == nullptr) {
            new_mgr->err_msg = ERRMSG_NOMEM;
            return FDS_ERR_NOMEM;
        }
        mapping_migrate_elems(new_mgr, copy.get());
        int rc = mapping_save_to_mgr(new_mgr, copy.get());
        if (rc != FDS_OK) {
            return rc;
        }
        copy.release();
    }
    return FDS_OK;
}

/**
 * Destroy mappings in iemgr
 */
void
mappings_destroy(fds_iemgr *mgr)
{
    for (fds_iemgr_mapping *m : mgr->mappings) {
        mapping_destroy(m);
    }
}

const struct fds_iemgr_mapping_item *
find_mapping_in_elem(const fds_iemgr_elem *elem, const char *key)
{
    for (size_t i = 0; i < elem->mappings_cnt; i++) {
        fds_iemgr_mapping *mapping = elem->mappings[i]; 
        for (size_t j = 0; j < mapping->items_cnt; j++) {
            if (mapping->key_case_sensitive) {
                if (strcmp(mapping->items[j].key, key) == 0) {
                    return &mapping->items[j];
                }                
            } else {
                if (strcasecmp(mapping->items[j].key, key) == 0) {
                    return &mapping->items[j];
                }                
            }
        }
    }
    return nullptr;
}


/******************************************************************************
 * Parsing
 ******************************************************************************/

/**
 * Create a parser to process the mapping XML file
 */
static fds_xml_t *
create_parser(fds_iemgr_t *mgr);

/**
 * Process the <group> element
 */
static int
read_mapping(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx);

/**
 * Process the <item-list> element
 */
static int
read_item_list(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_mapping *mapping);

/**
 * Process the <item> element of <item-list>
 */
static int
read_item(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_mapping *mapping);

/**
 * Read the mappings file, process it and store it to the iemgr
 */
int
fds_iemgr_mapping_read_file(fds_iemgr_t *mgr, const char *file_path)
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
    auto parser = unique_parser(create_parser(mgr), &::fds_xml_destroy);
    if (parser == nullptr) {
        return FDS_ERR_DENIED;
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
        if (cont->id != GROUP) {
            continue;
        }

        int rc = read_mapping(mgr, cont->ptr_ctx);
        if (rc != FDS_OK) {
            return rc;
        }
    }

    return FDS_OK;
}

static fds_xml_t *
create_parser(fds_iemgr_t *mgr)
{
    fds_xml_t *parser = fds_xml_create();
    if (parser == nullptr) {
        mgr->err_msg = ERRMSG_NOMEM;
        return nullptr;
    }

    static const struct fds_xml_args args_item[] = {
        FDS_OPTS_ELEM(ITEM_KEY, "key", FDS_OPTS_T_STRING, 0),
        FDS_OPTS_ELEM(ITEM_VALUE, "value", FDS_OPTS_T_INT, 0),
        FDS_OPTS_END
    };

    static const struct fds_xml_args args_item_list[] = {
        FDS_OPTS_ATTR(ITEM_LIST_MODE, "mode", FDS_OPTS_T_STRING, FDS_OPTS_P_OPT),
        FDS_OPTS_NESTED(ITEM_LIST_ITEM, "item", args_item, FDS_OPTS_P_MULTI),
        FDS_OPTS_END
    };

    static const struct fds_xml_args args_group[] = {
        FDS_OPTS_ELEM(GROUP_NAME, "name", FDS_OPTS_T_STRING, 0),
        FDS_OPTS_ELEM(GROUP_MATCH, "match", FDS_OPTS_T_STRING, FDS_OPTS_P_MULTI),
        FDS_OPTS_NESTED(GROUP_ITEM_LIST, "item-list", args_item_list, 0),
        FDS_OPTS_END
    };

    static const struct fds_xml_args args_main[] = {
        FDS_OPTS_ROOT("ipfix-mapping"),
        FDS_OPTS_NESTED(GROUP, "group", args_group, FDS_OPTS_P_OPT | FDS_OPTS_P_MULTI),
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
read_mapping(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx)
{
    auto mapping = unique_mapping(mapping_create(), &::mapping_destroy);
    mapping->key_case_sensitive = true;
    int rc;
    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case GROUP_NAME:
            if (*cont->ptr_string == '\0') {
                mgr->err_msg = "Group name cannot be empty.";
                return FDS_ERR_FORMAT;
            }
            mapping->name = strdup(cont->ptr_string);
            if (mapping->name == nullptr) {
                mgr->err_msg = ERRMSG_NOMEM;
                return FDS_ERR_NOMEM;
            }
            break;
        case GROUP_MATCH:
            rc = mapping_add_match(mgr, mapping.get(), cont->ptr_string);
            if (rc != FDS_OK) {
                return rc;
            }
            break;
        case GROUP_ITEM_LIST:
            rc = read_item_list(mgr, cont->ptr_ctx, mapping.get());
            if (rc != FDS_OK) {
                return rc;
            }  
            break;
        }
    }
    rc = mapping_save_to_mgr(mgr, mapping.get());
    if (rc != FDS_OK) {
        return rc;
    }
    mapping.release();
    return FDS_OK;
}

static int
read_item_list(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_mapping *mapping)
{
    int rc;
    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case ITEM_LIST_ITEM:
            rc = read_item(mgr, cont->ptr_ctx, mapping);
            if (rc != FDS_OK) {
                return rc;
            }
            break;
        case ITEM_LIST_MODE:
            if (strcasecmp(cont->ptr_string, "caseSensitive") == 0) {
                mapping->key_case_sensitive = true;
            } else if (strcasecmp(cont->ptr_string, "caseInsensitive") == 0) {
                mapping->key_case_sensitive = false;
            } else {
                return FDS_ERR_FORMAT;
            }
            break;
        }
    }
    return FDS_OK;
}

static int
read_item(fds_iemgr_t *mgr, fds_xml_ctx_t *xml_ctx, fds_iemgr_mapping *mapping)
{
    fds_iemgr_mapping_item item;
    auto holder = unique_mapping_item(&item, &::mapping_item_destroy_fields);

    const struct fds_xml_cont *cont;
    while (fds_xml_next(xml_ctx, &cont) != FDS_EOC) {
        switch (cont->id) {
        case ITEM_KEY:
            if (*cont->ptr_string == '\0') {
                mgr->err_msg = "Item key cannot be empty.";
                return FDS_ERR_FORMAT;
            }
            if (!check_valid_name(cont->ptr_string)) {
                mgr->err_msg = 
                    "Invalid characters in item key '" + std::string(cont->ptr_string) + "'. "
                    "Key names must only consist of alphanumeric characters and underscores "
                    "and must not begin with a number.";
                return FDS_ERR_FORMAT;
            }
            item.key = strdup(cont->ptr_string);
            if (item.key == nullptr) {
                mgr->err_msg = ERRMSG_NOMEM;
                return FDS_ERR_NOMEM;
            }
            break;
        case ITEM_VALUE:
            item.value.i = cont->val_int;
            break;
        }
    }

    if (!mapping_add_item(mapping, item)) {
        mgr->err_msg = ERRMSG_NOMEM;
        return FDS_ERR_NOMEM;
    }

    holder.release();
    return FDS_OK;
}



