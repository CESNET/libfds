/**
  * \file   libnf2.h
  * \brief  libnf2 C interface
  *
  * libnf2.h is a public interface to interact with the whole libnf2 library.
  * API is divided into high-level, mid-level and low-level API each best suited
  * for different use-cases and ease of development.
  *
  * The API and library is work in progress. Any comments are welcome and
  * aprreciated.
  *
  * \author Petr Stehlík <stehlik@cesnet.cz>
  * \author Lukáš Huták <xhutak01@stud.fit.vutbr.cz>
  * \author Petr Velan <petr.velan@cesnet.cz>
  *
  * \date 04/2017
  */

/**
  * \defgroup context   API for context manipulation, creation and deletion
  * \defgroup exporter  Exporter operations
  * \defgroup record-hl High-level API for record manipulation
  * \defgroup template  Template operations (Mid-level API)
  * \defgroup record-ll Low-level API for record manipulation
  *                     Ultimate low-level API for memory manipulation
  */

/**
  * \ingroup context
  *
  * \warning Any file operation must be handled by user (i.e. opening/closing
  * file)!
  */

/**
  * \ingroup context
  *
  * \brief Create new context with given file descriptor and set of flags
  *
  * \note TODO: loading a list of available items
  *
  * \param *file File descriptor
  * \param flags Set of flags (TBD) (read/write/append) \n
  *         LNF_READ - open file for reading \n
  *	        LNF_APPEND - open file for reading in append mode \n
  *	        LNF_WRITE - open file for for writing \n
  *         LNF_COMP_X - compress context data using X \n
  *         LNF_COMP_Y - compress context data using Y
  *
  * \return pointer to newly created context
  */
lnf_ctx_t *
lnf_ctx_new(FILE *file, const char *elem_dir, int flags);

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
  * \param *rec Record to write
  *
  * \return LNF_OK, LNF_CTX_ERR
  */
int
lnf_ctx_write(lnf_ctx_t *ctx, lnf_rec_t *rec);

/**
  * \ingroup context
  *
  * \brief Read a record from given context
  *
  * \param *ctx Context in which to read a record
  * \param[in,out] rec
  *
  * \return LNF_OK, LNF_EOF, LNF_CTX_ERR
  */
int
lnf_ctx_read(lnf_ctx_t *ctx, lnf_rec_t *rec);

/**
  * \brief Callback function for lnf_ctx_read_cond()
  */
typedef bool (*lnf_cond_cb) (const lnf_tmplt_t *, const lnf_exp_t *, void *);

/**
  * \ingroup context
  *
  * \brief Read record from context under a condition evaluated by the callback
  * function
  *
  * If condition is not met the flow block is skipped.
  *
  * \param[in] ctx Context to read from
  * \param[in,out] rec Read record
  * \param[in] cb Callback function for conditional read
  * \param[in] cb_data Callback function data
  *
  * \return LNF_OK, LNF_EOF, LNC_CTX_ERR
  */
int
lnf_ctx_read_cond(lnf_ctx_t *ctx, lnf_rec_t *rec, lnf_cond_cb *cb, void *cb_data);

/**
  * TODO: How to read only the significant blocks from a record
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
  * \param *ctx Context in which to initialize record
  *
  * \return Pointer to newly initialized record
  */
lnf_rec_t *
lnf_rec_init(lnf_ctx_t *ctx);

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
  * Record data are not copied, only pointer to it is returned with its size.
  *
  * \param[in]  *rec    Record from which to retrieve the value
  * \param[in]  f_en    Enterprise number
  * \param[in]  f_id    Field ID
  * \param[out] **data  Retrieved data
  * \param[out] *size   Size of retrieved data
  *
  * \return LNF_OK, LNF_REC_ERR
  */
int
lnf_rec_get(lnf_rec_t *rec, uint32_t f_en, uint16_t f_id, uint8_t **data, uint16_t *size);

/**
  * \ingroup record-hl
  * \brief Get raw record data
  *
  * \param[in] Record which to retrieve
  */
const uint8_t *
lnf_rec_raw_get(lnf_rec_t *rec);

/**
  * \ingroup record-hl
  * \brief Set exporter to a record
  *
  * \param[in] rec Record in which to set exporter
  * \param[in] exp Exporter for given record
  *
  * \return LNF_OK, LNF_REC_RO_ERR (read-only error)
  */
int
lnf_rec_exporter_set(lnf_rec_t *rec, const lnf_exporter_t *exp)

/**
  * \brief Retrieve exporter from record
  *
  * \param[in] rec Record from which to get exporter
  */
const lnf_exporter_t *
lnf_rec_exporter_get(lnf_rec_t *rec)

/**
  * Middle level - TEMPLATES
  */
/**
  * \ingroup template
  *
  * \brief Add new template to context
  *
  * \param *ctx Context to which to add the new template
  * \param flp_cnt number of fields
  * \param *fields fields of the template
  *
  * \return pointer to newly created template structure
  */
lnf_template_t *
lnf_template_add(lnf_ctx_t *ctx, uint16_t fld_cnt, const struct lnf_tmplt_field *fields);

/**
  * \ingroup template
  *
  * \brief Set template to a record
  *
  * \note If the template is NULL it will set the record's template dynamically.
  * If the record was previously created dynamically it will not use
  * the previous template.
  *
  * \note Record must be empty otherwise no action is taken.
  *
  * \param *rec     Record for which to set a template
  * \param *tmplt   Pointer to template
  *
  * \return LNF_OK, LNF_TMPLT_ERR
  */
int
lnf_rec_template_set(lnf_rec_t *rec, lnf_template_t *tmplt);

/**
  * Low level
  */
/**
  * \ingroup record-ll
  *
  * \brief Allocate memory for new record
  *
  * \param *ctx     Working context
  * \param *exp     Exporter of the record
  * \param *tmplt   Template of the record
  * \param size     Size of memory to allocate
  *
  * \return Pointer to buffer
  */
uint8_t *
lnf_raw_alloc(lnf_ctx_t *ctx, lnf_exporter_t *exp, lnf_template_t *tmplt, uint16_t size);

/**
  * \ingroup record-ll
  *
  * \brief Finalizes writing of a record into a context
  *
  * \note Record size is the first two bytes of the record.
  *
  * \param *ctx Context in which the record was written
  * \param size Final size of the record which was written
  *
  * \return LNF_OK, LNF_RAW_ERR
  */
int
lnf_raw_finalize(lnf_ctx_t *ctx);

