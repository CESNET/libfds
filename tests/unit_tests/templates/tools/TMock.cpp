#include <libfds.h>
#include <stdexcept>
#include <gtest/gtest.h>
#include "TMock.h"
#include "TGenerator.h"

struct fds_template*
TMock::create(type pattern, uint16_t id)
{
    struct fds_template *result = nullptr;
    uint16_t len;
    int ret_code = FDS_ERR_FORMAT;

    switch (pattern) {
    case type::DATA_BASIC_FLOW: {
        TGenerator data(id, 10);
        data.append(  8, 4); // sourceIPv4Address
        data.append( 12, 4); // destinationIPv4Address
        data.append(  7, 2); // sourceTransportPort
        data.append( 11, 2); // destinationTransportPort
        data.append(  4, 1); // protocolIdentifier
        data.append(  6, 1); // tcpControlBits
        data.append(152, 8); // flowStartMilliseconds
        data.append(153, 8); // flowEndMilliseconds
        data.append(  2, 4); // packetDeltaCount
        data.append(  1, 4); // octetDeltaCount

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE, data.get(), &len, &result);
        }
        break;

    case type::DATA_BASIC_BIFLOW: {
        TGenerator data(id, 15);
        data.append(  8, 4, 0);     // sourceIPv4Address
        data.append( 12, 4, 0);     // destinationIPv4Address
        data.append(  7, 2, 0);     // sourceTransportPort
        data.append( 11, 2, 0);     // destinationTransportPort
        data.append(  4, 1, 0);     // protocolIdentifier
        data.append(  6, 1, 0);     // tcpControlBits
        data.append(152, 8, 0);     // flowStartMilliseconds
        data.append(153, 8, 0);     // flowEndMilliseconds
        data.append(  2, 4, 0);     // packetDeltaCount
        data.append(  1, 4, 0);     // octetDeltaCount
        data.append(  6, 1, 29305); // tcpControlBits (reverse)
        data.append(152, 8, 29305); // flowStartMilliseconds (reverse)
        data.append(153, 8, 29305); // flowEndMilliseconds (reverse)
        data.append(  2, 4, 29305); // packetDeltaCount (reverse)
        data.append(  1, 4, 29305); // octetDeltaCount (reverse)

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE, data.get(), &len, &result);
        }
        break;

    case type::DATA_WITHDRAWAL: {
        TGenerator data(id, 0);

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE, data.get(), &len, &result);
    }
    break;

    case type::OPTS_MPROC_STAT: {
        TGenerator data(id, 5, 2);
        data.append(149, 4, 0); // observationDomainId
        data.append(143, 4, 0); // meteringProcessId
        data.append( 40, 8, 0); // exportedOctetTotalCount
        data.append( 41, 8, 0); // exportedMessageTotalCount
        data.append( 42, 8, 0); // exportedFlowRecordTotalCount

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE_OPTS, data.get(), &len, &result);
        }
        break;

    case type::OPTS_MPROC_RSTAT:  {
        TGenerator data(id, 6, 1);
        data.append(149, 4); // observationDomainId
        data.append(164, 8); // ignoredPacketTotalCount
        data.append(165, 8); // ignoredOctetTotalCount
        data.append(323, 8); // observationTimeMilliseconds
        data.append(323, 8); // observationTimeMilliseconds
        data.append(166, 8); // (extra) notSentFlowTotalCount

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE_OPTS, data.get(), &len, &result);

        }
        break;

    case type::OPTS_ERPOC_RSTAT: {
        TGenerator data(id, 6, 1);
        data.append(131, 16);// exporterIPv6Address
        data.append(166, 8); // notSentFlowTotalCount
        data.append(167, 8); // notSentPacketTotalCount
        data.append(168, 8); // notSentOctetTotalCount
        data.append(324, 8); // observationTimeMicroseconds
        data.append(324, 8); // observationTimeMicroseconds

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE_OPTS, data.get(), &len, &result);
        }
        break;

    case type::OPTS_FKEY: {
        TGenerator data(id, 2, 1);
        data.append(145, 2); // templateId
        data.append(173, 8); // flowKeyIndicator

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE_OPTS, data.get(), &len, &result);
        }
        break;

    case type::OPTS_WITHDRAWAL: {
        TGenerator data(id, 0);

        len = data.length();
        ret_code = fds_template_parse(FDS_TYPE_TEMPLATE, data.get(), &len, &result);
        }
        break;

    default:
        throw std::runtime_error("Not implemented!");
    }

    if (ret_code != FDS_OK) {
        throw std::runtime_error("Something is wrong! (invalid return code)");
    }
    return result;
}
