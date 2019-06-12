//
// Created by lukashutak on 31.12.17.
//

#include <cstring>     // memcpy
#include <arpa/inet.h> // htons,...

#include "TGenerator.h"
#include <libfds.h>

TGenerator::TGenerator(uint16_t id, uint16_t field_cnt, uint16_t scope_cnt)
{
    data = std::unique_ptr<uint8_t[]>(new uint8_t[DEF_SIZE]);
    size_alloc = DEF_SIZE;

    if (scope_cnt == 0) {
        struct fds_ipfix_trec *rec = reinterpret_cast<struct fds_ipfix_trec *>(data.get());
        rec->template_id = htons(id);
        rec->count = htons(field_cnt);
        size_used = 4U;
    } else {
        struct fds_ipfix_opts_trec *rec = reinterpret_cast<struct fds_ipfix_opts_trec*>(data.get());
        rec->template_id = htons(id);
        rec->count = htons(field_cnt);
        rec->scope_field_count = htons(scope_cnt);
        size_used = 6U;
    }
}

TGenerator& TGenerator::operator=(TGenerator &&obj)
{
    if (this != &obj) {
        data = std::move(obj.data);
    }

    return *this;
}

void
TGenerator::append(uint16_t ie_id, uint16_t len, uint32_t ie_en)
{
    size_t size_req = 4U;
    if (ie_en != 0) {
        size_req = 8U;
    }

    if (size_req > size_alloc - size_used) {
        // Resize the array
        const size_t new_alloc = 2 * size_alloc;
        auto new_data = std::unique_ptr<uint8_t[]>(new uint8_t[new_alloc]);
        std::memcpy(new_data.get(), data.get(), size_used);

        data = std::move(new_data);
        size_alloc = new_alloc;
    }

    fds_ipfix_tmplt_ie *field = reinterpret_cast<fds_ipfix_tmplt_ie *>(&(data.get()[size_used]));
    field->ie.id = htons(ie_id);
    field->ie.length = htons(len);
    size_used += 4U;

    if (ie_en == 0) {
        return;
    }

    // Add PEN (Private Enterprise Number)
    field->ie.id = htons(ie_id | 0x8000);

    field++;
    field->enterprise_number = htonl(ie_en);
    size_used += 4U;
}

const void*
TGenerator::get()
{
    return static_cast<const void *>(data.get());
}

size_t
TGenerator::length()
{
    return size_used;
}
