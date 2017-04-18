/**
  * \file   libfds.h
  * \brief  libfds C interface
  *
  * libfds.h is a public interface to interact with the whole libfds library.
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
  * \param[in] *file File descriptor
  * \param[in] *elems IPFIX elements definitions
  * \param flags Set of flags (TBD) (read/write/append) \n
  *         fds_READ - open file for reading \n
  *	        fds_APPEND - open file for reading in append mode \n
  *	        fds_WRITE - open file for for writing \n
  *         fds_COMP_X - compress context data using X \n
  *         fds_COMP_Y - compress context data using Y
  * \param[out] **ctx Newly created context
  *
  * \return 0 on success, otherwise an error number
  */
int
fds_ctx_new(FILE *file, const ipx_elems_t *elems, int flags, fds_ctx_t **ctx);

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
fds_ctx_destroy(fds_ctx_t *ctx);

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
fds_ctx_file_set(fds_ctx_t *ctx, FILE *file);

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
fds_ctx_file_get(fds_ctx_t *ctx);

/**
  * \ingroup context
  *
  * \brief Write new record into given context
  *
  * \param *ctx Context in which to write the record
  * \param *rec Record to write
  *
  * \return fds_OK, fds_CTX_ERR
  */
int
fds_ctx_write(fds_ctx_t *ctx, fds_rec_t *rec);

/**
  * \ingroup context
  *
  * \brief Read a record from given context
  *
  * \param *ctx Context in which to read a record
  * \param[in,out] rec
  *
  * \return fds_OK, fds_EOF, fds_CTX_ERR
  */
int
fds_ctx_read(fds_ctx_t *ctx, fds_rec_t *rec);

/**
  * \brief Callback function for fds_ctx_read_cond()
  */
typedef bool (*fds_cond_cb) (const fds_tmplt_t *, const fds_exp_t *, void *);

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
  * \return fds_OK, fds_EOF, LNC_CTX_ERR
  */
int
fds_ctx_read_cond(fds_ctx_t *ctx, fds_rec_t *rec, fds_cond_cb *cb, void *cb_data);

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
fds_exporter_t *
fds_exporter_add(fds_ctx_t *ctx, uint32_t odid, uint8_t addr[16],
	const char *description);

/**
  * \ingroup record-hl
  *
  * \brief Alias for internal record structure definition
  */
typedef struct fds_rec_s fds_rec_t;

/**
  * \ingroup record-hl
  *
  * \brief Create empty record
  *
  * \param *ctx Context in which to initialize record
  * \param[out] *rec Newly initialized record
  *
  * \return 0 on success, otherwise an error number
  */
int
fds_rec_init(fds_ctx_t *ctx, fds_rec_t *rec);

/**
  * \ingroup record-hl
  *
  * \brief Destroy given record
  */
void
fds_rec_destroy(fds_rec_t *rec);

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
fds_rec_clear(fds_rec_t *rec);

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
  * \return fds_REC_OK, fds_REC_ERR
  */
int
fds_rec_set(fds_rec_t *rec, uint32_t f_en, uint16_t f_id, const uint8_t *data,
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
  * \return fds_OK, fds_REC_ERR
  */
int
fds_rec_get(fds_rec_t *rec, uint32_t f_en, uint16_t f_id, uint8_t **data, uint16_t *size);

/**
  * \ingroup record-hl
  * \brief Get raw record data
  *
  * \param[in] Record which to retrieve
  */
const uint8_t *
fds_rec_raw_get(fds_rec_t *rec);

/**
  * \ingroup record-hl
  * \brief Set exporter to a record
  *
  * \param[in] rec Record in which to set exporter
  * \param[in] exp Exporter for given record
  *
  * \return fds_OK, fds_REC_RO_ERR (read-only error)
  */
int
fds_rec_exporter_set(fds_rec_t *rec, const fds_exporter_t *exp)

/**
  * \brief Retrieve exporter from record
  *
  * \param[in] rec Record from which to get exporter
  */
const fds_exporter_t *
fds_rec_exporter_get(fds_rec_t *rec)

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
fds_template_t *
fds_template_add(fds_ctx_t *ctx, uint16_t fld_cnt, const struct fds_tmplt_field *fields);

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
  * \return fds_OK, fds_TMPLT_ERR
  */
int
fds_rec_template_set(fds_rec_t *rec, fds_template_t *tmplt);

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
fds_raw_alloc(fds_ctx_t *ctx, fds_exporter_t *exp, fds_template_t *tmplt, uint16_t size);

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
  * \return fds_OK, fds_RAW_ERR
  */
int
fds_raw_finalize(fds_ctx_t *ctx);

