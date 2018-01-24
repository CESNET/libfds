//
// Created by lukashutak on 23/01/18.
//

#ifndef LIBFDS_TMOCK_H
#define LIBFDS_TMOCK_H

#include <cstdint>

/**
 * \brief Simple generator of parsed templates
 */
class TMock {
private:
    // Disable constructor
    TMock() {};

public:
    /** Available template patterns  */
    enum class type {
        DATA_BASIC_FLOW,
        DATA_BASIC_BIFLOW,
        DATA_WITHDRAWAL,
        OPTS_MPROC_STAT,
        OPTS_MPROC_RSTAT,
        OPTS_ERPOC_RSTAT,
        OPTS_FKEY,
        OPTS_WITHDRAWAL

    };

    /**
     * \brief Create a template based on a pattern
     * \warning User is responsible for destruction of the template!
     * \param[in] id Template ID
     * \return Pointer to the template
     */
    static struct fds_template *
    create(type pattern, uint16_t id);
};

#endif //LIBFDS_TMOCK_H
