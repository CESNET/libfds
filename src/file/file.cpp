/**
 * @file   src/file/file.cpp
 * @author Lukas Hutak <lukas.hutak@cesnet.cz>
 * @brief  C API wrapper (source file)
 * @date   May 2018
 *
 * Copyright(c) 2019 CESNET z.s.p.o.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <bitset>
#include <new> // nothrow

#include <libfds.h>
#include <libfds/file.h>


#include "File_base.hpp"
#include "File_exception.hpp"
#include "File_reader.hpp"
#include "File_writer.hpp"

using namespace fds_file;

/// Max size of an error message
static constexpr size_t ERR_BUFFER_SIZE = 512U;
/// Flag mask of all file operation modes (read/write/append)
static constexpr uint32_t FMASK_MODE = FDS_FILE_READ | FDS_FILE_WRITE | FDS_FILE_APPEND;
/// Flag mask of all compression algorithms
static constexpr uint32_t FMASK_COMP = FDS_FILE_LZ4 | FDS_FILE_ZSTD;

/// Parsed file mode
enum class file_mode {
    READER,   ///< File reader
    WRITER,   ///< File writer
    APPENDER, ///< File appender
};

/// Auxiliary file instance
struct fds_file_s {
    /// File handler (reader/writer)
    File_base *m_handler;

    struct {
        /// File mode
        file_mode mode;
        /// Compression algorithm
        enum fds_file_alg alg;
        /// Reference to a manager of Information Elements
        const fds_iemgr_t *iemgr;
    } m_params; ///< Parsed parameters

    struct {
        /// A fatal internal error has occurred
        bool is_fatal;
        /// Last error message
        char buffer[ERR_BUFFER_SIZE];
    } m_error; ///< Error buffer
};

/**
 * @brief Copy an error message into the internal buffer
 * @note If the size of the message exceeds the buffer size, the message is truncated.
 * @param[in] inst File handler
 * @param[in] msg  Error message
 */
static inline void
error_set(struct fds_file_s *inst, const char *msg) noexcept {
    size_t len = strnlen(msg, ERR_BUFFER_SIZE - 1);
    char *buffer = inst->m_error.buffer;
    strncpy(buffer, msg, len);
    buffer[ERR_BUFFER_SIZE - 1] = '\0';
}

/**
 * @brief Copy an error message into the internal buffer
 * @note If the size of the message exceeds the buffer size, the message is truncated.
 * @param[in] inst File handler
 * @param[in] msg  Error message
 */
static inline void
error_set(struct fds_file_s *ins, const std::string &msg)  noexcept {
    error_set(ins, msg.c_str());
}

/**
 * @brief Reset error message and clear fatal error flag
 * @param[in] inst File handler
 */
static inline void
error_reset(struct fds_file_s *inst)
{
    inst->m_error.is_fatal = false;
    strcpy(inst->m_error.buffer, "No error");
}

/**
 * @brief Parse and check flags for file opening
 * @param[in]  file  File handler (only for log)
 * @param[in]  flags Flags of fds_file_open()
 * @param[out] mode  Extracted operation mode
 * @param[out] alg   Extracted compression algorithm
 * @param[out] io    Extracted I/O method
 * @return #FDS_OK on success
 * @return #FDS_ERR_ARG on error and the error buffer is filled
 */
static int
flags_parse(struct fds_file_s *file, uint32_t flags, file_mode &mode, enum fds_file_alg &alg,
    Io_factory::Type &io)
{
    // Check operation mode flags
    std::bitset<32> bset_mode(flags & FMASK_MODE);
    if (bset_mode.count() != 1) {
        error_set(file, "Invalid argument (operation mode not selected)");
        return FDS_ERR_ARG;
    }

    switch (flags & FMASK_MODE) {
    case FDS_FILE_READ:
        mode = file_mode::READER;
        flags &= ~FMASK_COMP; // Ignore compression flags
        break;
    case FDS_FILE_WRITE:
        mode = file_mode::WRITER;
        break;
    case FDS_FILE_APPEND:
        mode = file_mode::APPENDER;
        break;
    default:
        error_set(file, "Operation mode not selected");
        return FDS_ERR_ARG;
    }

    // Check compression mode flags
    std::bitset<32> bset_comp(flags & FMASK_COMP);
    if (bset_comp.count() > 1) {
        error_set(file, "Invalid argument (multiple compression algorithms)");
        return FDS_ERR_ARG;
    }

    alg = FDS_FILE_CALG_NONE;
    if ((flags & FDS_FILE_LZ4) != 0) {
        alg = FDS_FILE_CALG_LZ4;
    } else if ((flags &  FDS_FILE_ZSTD) != 0) {
        alg = FDS_FILE_CALG_ZSTD;
    }

    // Determine I/O type
    io = ((flags & FDS_FILE_NOASYNC) != 0)
        ? Io_factory::Type::IO_SYNC
        : Io_factory::Type::IO_DEFAULT;

    return FDS_OK;
}

/**
 * @brief Fatal error check
 *
 * If a fatal error has occurred (memory allocation error, malformed file, etc), return
 * #FDS_ERR_INTERNAL code.
 * @param[in] file File handler
 */
#define FATAL_TEST(file)                                \
    if ((file)->m_error.is_fatal) {                     \
        return FDS_ERR_INTERNAL;                        \
    }

/**
 * @brief C++ API wrapped
 *
 * The main purpose is to catch all C++ exceptions and store error message into the internal
 * error buffer. If an exception has occurred, particular error code is returned!
 * @param[in] file File handler
 * @param[in] func Function or block that must be protected against the exceptions
 */
#define API_WRAPPER(file, func)                         \
    try {                                               \
        func;                                           \
    } catch (File_exception &ex) {                      \
        if (ex.code() == FDS_ERR_INTERNAL) {            \
            (file)->m_error.is_fatal = true;            \
        }                                               \
        error_set((file), ex.what());                   \
        return ex.code();                               \
    } catch (std::exception &ex) {                      \
        (file)->m_error.is_fatal = true;                \
        error_set((file), ex.what());                   \
        return FDS_ERR_INTERNAL;                        \
    } catch (...) {                                     \
        (file)->m_error.is_fatal = true;                \
        error_set((file), "Unknown error");             \
        return FDS_ERR_INTERNAL;                        \
    }

fds_file_t *
fds_file_init()
{
    // Create the instance
    std::unique_ptr<struct fds_file_s> inst(new(std::nothrow) struct fds_file_s);
    if (!inst) {
        return nullptr;
    }

    // Set default parameters
    error_set(inst.get(), "No opened file");
    inst->m_error.is_fatal = true;
    inst->m_handler = nullptr;
    inst->m_params.iemgr = nullptr;

    return inst.release();
}

void
fds_file_close(fds_file_t *file)
{
    delete file->m_handler;
    delete file;
}

const char *
fds_file_error(const fds_file_t *file)
{
    return file->m_error.buffer;
}

int
fds_file_open(fds_file_t *file, const char *path, uint32_t flags)
{
    // Replace the previous file
    delete file->m_handler;
    file->m_handler = nullptr;
    file->m_error.is_fatal = true; // Just in case of an error

    // Parse flags
    file_mode new_mode;
    enum fds_file_alg new_alg;
    Io_factory::Type new_io_type;

    int rc = flags_parse(file, flags, new_mode, new_alg, new_io_type);
    if (rc != FDS_OK) {
        return rc;
    }

    File_base *new_file = nullptr;
    API_WRAPPER(file, {
        if (new_mode == file_mode::READER) {
            // File reader
            new_file = new File_reader(path, new_io_type);
        } else {
            // File writer/appender
            bool append = (new_mode == file_mode::APPENDER) ? true : false;
            new_file = new File_writer(path, new_alg, append, new_io_type);
        }
    })

    // Wrap the pointer so it is automatically freed in case of an error during IE manager setup
    std::unique_ptr<File_base> new_file_wrap(new_file);

    if (file->m_params.iemgr != nullptr) {
        // Manager of Information Elements is already configured
        API_WRAPPER(file, new_file_wrap->iemgr_set(file->m_params.iemgr));
    }

    file->m_handler = new_file_wrap.release();
    file->m_params.mode = new_mode;
    file->m_params.alg = new_alg;
    error_reset(file); // Resets the fatal error flag
    return FDS_OK;
}

const struct fds_file_stats *
fds_file_stats_get(fds_file_t *file)
{
    if (!file->m_handler) {
        return nullptr;
    }

    return file->m_handler->stats_get();
}

int
fds_file_set_iemgr(fds_file_t *file, const fds_iemgr_t *iemgr)
{
    if (!file->m_handler) {
        // Safe the IE manager reference for the later use
        file->m_params.iemgr = iemgr;
        return FDS_OK;
    }

    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->iemgr_set(iemgr));
    file->m_params.iemgr = iemgr;
    return FDS_OK;
}

int
fds_file_session_add(fds_file_t *file, const struct fds_file_session *info, fds_file_sid_t *sid)
{
    FATAL_TEST(file);

    if (!info || !sid) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    API_WRAPPER(file, *sid = file->m_handler->session_add(info));
    return FDS_OK;
}

int
fds_file_session_get(fds_file_t *file, fds_file_sid_t sid, const struct fds_file_session **info)
{
    FATAL_TEST(file);

    if (!info) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    const struct fds_file_session *ptr;
    API_WRAPPER(file, ptr = file->m_handler->session_get(sid));
    if (!ptr) {
        error_set(file, "Transport Session not found");
        return FDS_ERR_NOTFOUND;
    }

    *info = ptr;
    return FDS_OK;
}

int
fds_file_session_list(fds_file_t *file, fds_file_sid_t **sid_arr, size_t *sid_size)
{
    FATAL_TEST(file);

    if (!sid_arr || !sid_size) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    API_WRAPPER(file, file->m_handler->session_list(sid_arr, sid_size));
    return FDS_OK;
}

int
fds_file_session_odids(fds_file_t *file, fds_file_sid_t sid, uint32_t **odid_arr, size_t *odid_size)
{
    FATAL_TEST(file);

    if (!odid_arr || !odid_size) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    API_WRAPPER(file, {
        const fds_file_session *ptr = file->m_handler->session_get(sid);
        if (!ptr) {
            error_set(file, "Transport Session not found");
            return FDS_ERR_NOTFOUND;
        }

        file->m_handler->session_odids(sid, odid_arr, odid_size);
    });

    return FDS_OK;
}

int
fds_file_read_sfilter(fds_file_t *file, const fds_file_sid_t *sid, const uint32_t *odid)
{
    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->read_sfilter_conf(sid, odid));
    return FDS_OK;
}

int
fds_file_read_rewind(fds_file_t *file)
{
    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->read_rewind());
    return FDS_OK;
}

int
fds_file_read_rec(fds_file_t *file, struct fds_drec *rec, struct fds_file_read_ctx *ctx)
{
    FATAL_TEST(file);
    API_WRAPPER(file, {
        return file->m_handler->read_rec(rec, ctx);
    });
    return FDS_OK;
}

int
fds_file_write_ctx(fds_file_t *file, fds_file_sid_t sid, uint32_t odid, uint32_t exp_time)
{
    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->select_ctx(sid, odid, exp_time));
    return FDS_OK;
}

int
fds_file_write_tmplt_add(fds_file_t *file, enum fds_template_type t_type, const uint8_t *t_data,
    uint16_t t_size)
{
    FATAL_TEST(file);

    if (!t_data || t_size == 0) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    API_WRAPPER(file, file->m_handler->tmplt_add(t_type, t_data, t_size));
    return FDS_OK;
}

int
fds_file_write_tmplt_remove(fds_file_t *file, uint16_t tid)
{
    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->tmplt_remove(tid));
    return FDS_OK;
}

int
fds_file_write_tmplt_get(fds_file_t *file, uint16_t tid, enum fds_template_type *t_type,
    const uint8_t **t_data, uint16_t *t_size)
{
    FATAL_TEST(file);
    API_WRAPPER(file, file->m_handler->tmplt_get(tid, t_type, t_data, t_size));
    return FDS_OK;
}

int
fds_file_write_rec(fds_file_t *file, uint16_t tid, const uint8_t *rec_data, uint16_t rec_size)
{
    FATAL_TEST(file);

    if (!rec_data || rec_size == 0) {
        error_set(file, "Invalid argument");
        return FDS_ERR_ARG;
    }

    API_WRAPPER(file, file->m_handler->write_rec(tid, rec_data, rec_size));
    return FDS_OK;
}
