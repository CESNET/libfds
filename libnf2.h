/**
  * \file   libnf2.h
  * \brief  libnf2 C interface
  */

/**
  * \defgroup context   Context operations
  * \defgroup exporter  Exporter operations
  * \defgroup record-hl High-level API for record manipulation
  * \defgroup template  Template operations (Mid-level API)
  * \defgroup record-ll Low-level API for record manipulation
  *                     Ultimate low-level API for memory manipulation
  */

/**
  * \ingroup context
  *
  * API for context manipulation, creation and deletion
  *
  * \note Any file operation must be handled by user (i.e. opening/closing file)
  */

/**
  * \ingroup context
  *
  * \brief Create new context with given file descriptor and set of flags
  *
  * \param *file File descriptor
  * \param flags Set of flags (TBD)
  *
  * \return pointer to newly created context
  */
lnf_ctx_t *
lnf_ctx_new(FILE *file, int flags);

/**
  * \ingroup context
  *
  * \brief Destroy given context
  *
  * When destroying context the finalization procedure is called beforehand
  * appending metainfo to given context.
  *
  * \param *ctx Context to destroy
  * \return void
  */
void
lnf_ctx_destroy(lnf_ctx_t *ctx);

/**
  * \ingroup context
  *
  * \brief Set new file descriptor for given context
  *
  * The diffence from destroy is that setting new file descriptor only stores
  * the file but keeps metainfo (i.e. templates).
  *
  * \param *ctx Context in which to set new file descriptor
  * \param *file File descriptor
  */
void
lnf_ctx_file_set(lnf_ctx_t *ctx, FILE *file);

/**
  * \ingroup context
  *
  * \brief Retrieve file descriptor from given context
  *
  * Get file descriptor in order to close it in near future (should be called
  * only before _destroy or _set).
  *
  * \param *ctx Context in which to set new file descriptor
  *
  * \return File descriptor
  */
FILE *
lnf_ctx_file_get(lnf_ctx_t *ctx);



/**
  * \ingroup context
  *
  * \brief Write new record into given context
  *
  * \param *ctx Context in which to write the record
  * \param *exp Exporter info
  * \param *rec Record to write
  */
void
lnf_ctx_write(lnf_ctx_t *ctx, lnf_exporter_t *exp, lnf_rec_t *rec);

/**
  * \ingroup context
  *
  * \brief Read a record from given context
  *
  * \param *ctx Context in which to read a record
  * \param TBD
  */
void
lnf_ctx_read(lnf_ctx_t *ctx, ...);

/**
  * TODO: How to read only the significant blocks from a record
  */

/**
  * \ingroup exporter
  *
  * \brief Operations with exporters
  */

/**
  * \ingroup exporter
  *
  * \brief Add exporter to given context
  *
  * \param *ctx Context
  * \param odid ID of the exporter
  * \param addr[]
  * \param *description Description of the exporter
  *
  * \return Initialized exporter
  */
lnf_exporter_t *
lnf_exporter_add(lnf_ctx_t *ctx, uint32_t odid, uint8_t addr[16],
	const char *description);

/**
  * \ingroup record-hl
  *
  * \brief Alias for internal record structure definition
  */
typedef struct lnf_rec_s lnf_rec_t;

/**
  * \ingroup record-hl
  *
  * \brief Create empty record
  *
  * \return Pointer to newly allocated record
  */
lnf_rec_t *
lnf_rec_init();

/**
  * \ingroup record-hl
  *
  * \brief Destroy given record
  */
void
lnf_rec_destroy(lnf_rec_t *rec);

/**
  * \ingroup record-hl
  *
  * \brief Removes all data from given record but keeps its template
  *
  * Clears the whole record apart from its template. When inserting new data
  * into such record it will try to match the data with its current template.
  * If it can't find a match it will destroy its template and start building
  * a new one.
  *
  * \param *rec Record to clear
  */
lnf_rec_clear(lnf_rec_t *rec);

/**
  * \ingroup record-hl
  *
  * \brief Set a value in a record
  *
  * \note Dynamic item is recognized by its internal definition
  *
  * \param *rec Record to manipulate
  * \param f_en Enterprise number
  * \param f_id field ID
  * \param *data Data to store
  * \param len length of data
  *
  * \return LNF_REC_OK, LNF_REC_ERR
  */
int
lnf_rec_set(lnf_rec_t *rec, uint32_t f_en, uint16_t f_id, const uint8_t *data,
	uint16_t len);

/**
  * \ingroup record-hl
  *
  * \brief Get a value from a record
  *
  * \param *rec Record from which to retrieve the value
  * \param f_en Enterprise number
  * \param f_id Field ID
  * \param *data Retrieved data
  *
  * \note TODO: how do we know how much data we will retrieve?
  *
  * \return LNF_REC_OK, LNF_REC_ERR
  */
int
lnf_rec_get(lnf_rec_t *rec, uint32_t f_en, uint16_t f_id, uint8_t *data);

/**
  * \ingroup record-hl
  *
  * \brief Get a next value from a record
  *
  * \param *rec Record from which to retrieve the value
  * \param *f_en Enterprise number
  * \param *f_id Field ID
  * \param *data Retrieved data
  *
  * \return LNF_REC_OK, LNF_REC_ERR
  */
/*int
lnf_rec_next(lnf_rec_t *rec, uint32_t *f_en, uint16_t *f_id, uint8_t *data);
*/


// Middle level
lnf_template_t *
lnf_template_add(lnf_ctx_t *ctx, uint16_t fld_cnt, const struct lnf_ctx_tmplt_field *fields);

// V pripade, že sablona je NULL bude skladat zaznam dynamicky. Pokud jiz 
// drive tvorit zaznam dynamicky, nebude předpokládat použití stejné šablony.
void
lnf_rec_set_template(lnf_rec_t *rec, lnf_template_t *tmplt);


// Low level

// Vrati ukazatel do bufferu
uint8_t *
lnf_raw_alloc(lnf_ctx_t *ctx, lnf_exporter_t *exp, lnf_template_t *tmplt, uint16_t size);

// Oznami ze zaznam byl v datech vyplnen (pozita delka jsou prvni 2B pouzite pameti)
lnf_raw_finalize(lnf_ctx_t *ctx);

















