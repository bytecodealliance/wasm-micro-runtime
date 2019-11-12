/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <string.h>
#include <stdio.h>
#include "bh_common.h"
#include "er-coap.h"
#include "coap_ext.h"
#include "er-coap-constants.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]", (lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3], (lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

extern size_t
coap_serialize_array_option(unsigned int number, unsigned int current_number,
        uint8_t *buffer, uint8_t *array, size_t length, char split_char);
extern size_t
coap_serialize_int_option(unsigned int number, unsigned int current_number,
        uint8_t *buffer, uint32_t value);
extern uint16_t coap_log_2(uint16_t value);
extern uint32_t coap_parse_int_option(uint8_t *bytes, size_t length);
extern void
coap_merge_multi_option(char **dst, size_t *dst_len, uint8_t *option,
        size_t option_len, char separator);

/*
 *
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |Len=15 |  TKL  | Extended Length (32 bits)
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |               |    Code       |  Token (if any, TKL bytes) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |   Options (if any) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |1 1 1 1 1 1 1 1|    Payload (if any) ...
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

int coap_set_payload_tcp(void *packet, const void *payload, size_t length)
{
    coap_packet_t * const coap_pkt = (coap_packet_t *) packet;

    coap_pkt->payload = (uint8_t *) payload;
    coap_pkt->payload_len = MIN(REST_MAX_CHUNK_SIZE, length);

    return coap_pkt->payload_len;
}

#if 0
static size_t coap_calc_ext_len_field(int len)
{
    if(len < 13)
    return 0;
    else if(len <= (0xFF+13))
    return 1;
    else if(len <= (0xFFFF+269))
    return 2;
    else if(len < (0xFFFFFFFF+65805))
    return 4;
    else
    return 0;
}
#endif

static size_t coap_max_options_offset(void *packet)
{
    coap_packet_t * const coap_pkt = (coap_packet_t *) packet;
    return 6 + coap_pkt->token_len;
}

int coap_serialize_message_tcp(void *packet, uint8_t ** buffer_out)
{
    coap_packet_t * const coap_pkt = (coap_packet_t *) packet;
    uint8_t buffer[128];

    uint8_t *option = buffer;
    unsigned int current_number = 0;

    if (coap_pkt->uri_path_len > 100) {
        *buffer_out = 0;
        return -1;
    }

    /* Serialize options */
    current_number = 0;
    if (0 == coap_pkt->token_len) {
        bh_memcpy_s(coap_pkt->token, COAP_TOKEN_LEN, &coap_pkt->mid,
                sizeof(coap_pkt->mid));
        coap_pkt->token_len = sizeof(coap_pkt->mid);
    }PRINTF("-Serializing options at %p-\n", option);

    /* The options must be serialized in the order of their number */
    COAP_SERIALIZE_BYTE_OPTION(COAP_OPTION_IF_MATCH, if_match, "If-Match");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_HOST, uri_host, '\0',
            "Uri-Host");
    COAP_SERIALIZE_BYTE_OPTION(COAP_OPTION_ETAG, etag, "ETag");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_IF_NONE_MATCH,
            content_format - coap_pkt-> content_format /* hack to get a zero field */,
            "If-None-Match");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_OBSERVE, observe, "Observe");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_URI_PORT, uri_port, "Uri-Port");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_LOCATION_PATH, location_path, '/',
            "Location-Path");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_PATH, uri_path, 0, //'/',
            "Uri-Path");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_CONTENT_FORMAT, content_format,
            "Content-Format");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_MAX_AGE, max_age, "Max-Age");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_URI_QUERY, uri_query, '&',
            "Uri-Query");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_ACCEPT, accept, "Accept");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_LOCATION_QUERY, location_query,
            '&', "Location-Query");
    COAP_SERIALIZE_BLOCK_OPTION(COAP_OPTION_BLOCK2, block2, "Block2");
    COAP_SERIALIZE_BLOCK_OPTION(COAP_OPTION_BLOCK1, block1, "Block1");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_SIZE2, size2, "Size2");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_PROXY_URI, proxy_uri, '\0',
            "Proxy-Uri");
    COAP_SERIALIZE_STRING_OPTION(COAP_OPTION_PROXY_SCHEME, proxy_scheme, '\0',
            "Proxy-Scheme");
    COAP_SERIALIZE_INT_OPTION(COAP_OPTION_SIZE1, size1, "Size1");

    /* Pack payload */
    if (coap_pkt->payload_len) {
        *option = 0xFF;
        ++option;
    }
    uint32_t option_len = option - &buffer[0];

    uint8_t * p = (uint8_t *) os_malloc(
            coap_max_options_offset(packet) + option_len
                    + coap_pkt->payload_len);
    if (p == NULL)
        return 0;
    *buffer_out = p;

    uint8_t first_4bits;

    *p = (coap_pkt->token_len & 0xF);
    uint32_t len = option_len + coap_pkt->payload_len;

    if (len < 13) {
        first_4bits = len;
        *p++ |= first_4bits << 4;
    } else if (len <= (0xFF + 13)) {
        first_4bits = 13;
        *p++ |= first_4bits << 4;
        *p++ = len - 13;
    } else if (len <= (0xFFFF + 269)) {
        first_4bits = 14;
        *p++ |= first_4bits << 4;
        len -= 269;
        *p = (uint8_t)(len >> 8);
        p++;
        *p = (uint8_t)(len & 0xFF);
        p++;
    } else {
        first_4bits = 15;
        *p++ |= first_4bits << 4;

        len -= 65805;
        *p++ = (uint8_t)(len >> 24);
        *p++ = (uint8_t)(len >> 16);
        *p++ = (uint8_t)(len >> 8);
        *p++ = (uint8_t)(len & 0xFF);
    }

    *p = coap_pkt->code;
    p++;

    if (coap_pkt->token_len)
        bh_memcpy_s(p, coap_pkt->token_len, coap_pkt->token,
                coap_pkt->token_len);
    p += coap_pkt->token_len;

    bh_memcpy_s(p, option_len, buffer, option_len);
    p += option_len;

    bh_memcpy_s(p, coap_pkt->payload_len, coap_pkt->payload,
            coap_pkt->payload_len);
    p += coap_pkt->payload_len;

    return (p - *buffer_out); /* packet length */
}

coap_status_t coap_parse_message_tcp(void *packet, uint8_t *data,
        uint32_t data_len)
{
    coap_packet_t * const coap_pkt = (coap_packet_t *) packet;

    /* initialize packet */
    memset(coap_pkt, 0, sizeof(coap_packet_t));

    /* pointer to packet bytes */
    coap_pkt->buffer = data;

    /* parse header fields */
    coap_pkt->version = 1;
    coap_pkt->type = COAP_TYPE_NON;
    coap_pkt->token_len = MIN(COAP_TOKEN_LEN, data[0] & 0xF);
    coap_pkt->mid = 0;

    uint8_t *p = data;
    uint8_t first_4bits = data[0] >> 4;

    uint32_t options_payload_size;
    uint8_t ext_len_field = 0;
    if (first_4bits < 13) {
        options_payload_size = first_4bits;
        p++;
    } else if (first_4bits == 13) {
        ext_len_field = 1;
        options_payload_size = data[1] + 13;
        p += 2;
    } else if (first_4bits == 14) {
        ext_len_field = 2;
        options_payload_size = (uint16_t)(data[1] << 8) + data[2] + 269;
        p += 3;
    } else if (first_4bits == 15) {
        ext_len_field = 4;
        options_payload_size = (data[1] << 24) + (data[2] << 16)
                + (data[3] << 8) + data[4] + 65805;
        p += 5;
    }

    // check the data size is smaller than the size indicated by the packet
    if (ext_len_field + coap_pkt->token_len + 2 + options_payload_size
            > data_len)
        return BAD_REQUEST_4_00;

    coap_pkt->code = *p++;
    if (coap_pkt->token_len)
        bh_memcpy_s(coap_pkt->token, COAP_TOKEN_LEN, p, coap_pkt->token_len);

    if (coap_pkt->token_len >= 2) {
        union {
            uint16_t *mid;
            uint8_t *token;
        } mid_token_union;

        mid_token_union.token = coap_pkt->token;
        coap_pkt->mid = *(mid_token_union.mid);
    }

    p += coap_pkt->token_len;

    uint8_t *current_option = p;
    uint8_t * option_start = p;

    /* parse options */
    memset(coap_pkt->options, 0, sizeof(coap_pkt->options));

    unsigned int option_number = 0;
    unsigned int option_delta = 0;
    size_t option_length = 0;

    while (current_option < data + data_len) {
        /* payload marker 0xFF, currently only checking for 0xF* because rest is reserved */
        if ((current_option[0] & 0xF0) == 0xF0) {
            coap_pkt->payload = ++current_option;
            coap_pkt->payload_len = options_payload_size
                    - (coap_pkt->payload - option_start);
            //coap_pkt->payload_len = data_len - (coap_pkt->payload - data);
            break;
        }

        option_delta = current_option[0] >> 4;
        option_length = current_option[0] & 0x0F;
        ++current_option;

        /* avoids code duplication without function overhead */
        unsigned int *x = &option_delta;

        do {
            if (*x == 13) {
                *x += current_option[0];
                ++current_option;
            } else if (*x == 14) {
                *x += 255;
                *x += current_option[0] << 8;
                ++current_option;
                *x += current_option[0];
                ++current_option;
            }
        } while (x != (unsigned int*) &option_length && (x =
                (unsigned int*) &option_length));
        option_length = *x;
        option_number += option_delta;

        PRINTF("OPTION %u (delta %u, len %u): ", option_number, option_delta,
                option_length);

        SET_OPTION(coap_pkt, option_number);

        switch (option_number) {

        case COAP_OPTION_CONTENT_FORMAT:
            coap_pkt->content_format = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Content-Format [%u]\n", coap_pkt->content_format);
            break;
        case COAP_OPTION_MAX_AGE:
            coap_pkt->max_age = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Max-Age [%lu]\n", coap_pkt->max_age);
            break;
        case COAP_OPTION_ETAG:
            coap_pkt->etag_len = MIN(COAP_ETAG_LEN, option_length);
            bh_memcpy_s(coap_pkt->etag, COAP_ETAG_LEN, current_option,
                    coap_pkt->etag_len);
            PRINTF("ETag %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
                    coap_pkt->etag_len, coap_pkt->etag[0], coap_pkt->etag[1],
                    coap_pkt->etag[2], coap_pkt->etag[3], coap_pkt->etag[4],
                    coap_pkt->etag[5], coap_pkt->etag[6], coap_pkt->etag[7]
            ); /*FIXME always prints 8 bytes */
            break;
        case COAP_OPTION_ACCEPT:
            coap_pkt->accept = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Accept [%u]\n", coap_pkt->accept);
            break;
        case COAP_OPTION_IF_MATCH:
            /* TODO support multiple ETags */
            coap_pkt->if_match_len = MIN(COAP_ETAG_LEN, option_length);
            bh_memcpy_s(coap_pkt->if_match, COAP_ETAG_LEN, current_option,
                    coap_pkt->if_match_len);
            PRINTF("If-Match %u [0x%02X%02X%02X%02X%02X%02X%02X%02X]\n",
                    coap_pkt->if_match_len, coap_pkt->if_match[0],
                    coap_pkt->if_match[1], coap_pkt->if_match[2],
                    coap_pkt->if_match[3], coap_pkt->if_match[4],
                    coap_pkt->if_match[5], coap_pkt->if_match[6],
                    coap_pkt->if_match[7]
            ); /* FIXME always prints 8 bytes */
            break;
        case COAP_OPTION_IF_NONE_MATCH:
            coap_pkt->if_none_match = 1;
            PRINTF("If-None-Match\n");
            break;

        case COAP_OPTION_PROXY_URI:
#if COAP_PROXY_OPTION_PROCESSING
            coap_pkt->proxy_uri = (char *)current_option;
            coap_pkt->proxy_uri_len = option_length;
#endif
            PRINTF("Proxy-Uri NOT IMPLEMENTED [%.*s]\n", coap_pkt->proxy_uri_len,
                    coap_pkt->proxy_uri);
            coap_error_message = "This is a constrained server (Contiki)";
            return PROXYING_NOT_SUPPORTED_5_05;
            break;
        case COAP_OPTION_PROXY_SCHEME:
#if COAP_PROXY_OPTION_PROCESSING
            coap_pkt->proxy_scheme = (char *)current_option;
            coap_pkt->proxy_scheme_len = option_length;
#endif
            PRINTF("Proxy-Scheme NOT IMPLEMENTED [%.*s]\n",
                    coap_pkt->proxy_scheme_len, coap_pkt->proxy_scheme);
            coap_error_message = "This is a constrained server (Contiki)";
            return PROXYING_NOT_SUPPORTED_5_05;
            break;

        case COAP_OPTION_URI_HOST:
            coap_pkt->uri_host = (char *) current_option;
            coap_pkt->uri_host_len = option_length;
            PRINTF("Uri-Host [%.*s]\n", coap_pkt->uri_host_len, coap_pkt->uri_host);
            break;
        case COAP_OPTION_URI_PORT:
            coap_pkt->uri_port = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Uri-Port [%u]\n", coap_pkt->uri_port);
            break;
        case COAP_OPTION_URI_PATH:
            /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
            coap_merge_multi_option((char **) &(coap_pkt->uri_path),
                    &(coap_pkt->uri_path_len), current_option, option_length,
                    '/');
            PRINTF("Uri-Path [%.*s]\n", coap_pkt->uri_path_len, coap_pkt->uri_path);
            break;
        case COAP_OPTION_URI_QUERY:
            /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
            coap_merge_multi_option((char **) &(coap_pkt->uri_query),
                    &(coap_pkt->uri_query_len), current_option, option_length,
                    '&');
            PRINTF("Uri-Query [%.*s]\n", coap_pkt->uri_query_len,
                    coap_pkt->uri_query);
            break;

        case COAP_OPTION_LOCATION_PATH:
            /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
            coap_merge_multi_option((char **) &(coap_pkt->location_path),
                    &(coap_pkt->location_path_len), current_option,
                    option_length, '/');
            PRINTF("Location-Path [%.*s]\n", coap_pkt->location_path_len,
                    coap_pkt->location_path);
            break;
        case COAP_OPTION_LOCATION_QUERY:
            /* coap_merge_multi_option() operates in-place on the IPBUF, but final packet field should be const string -> cast to string */
            coap_merge_multi_option((char **) &(coap_pkt->location_query),
                    &(coap_pkt->location_query_len), current_option,
                    option_length, '&');
            PRINTF("Location-Query [%.*s]\n", coap_pkt->location_query_len,
                    coap_pkt->location_query);
            break;

        case COAP_OPTION_OBSERVE:
            coap_pkt->observe = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Observe [%lu]\n", coap_pkt->observe);
            break;
        case COAP_OPTION_BLOCK2:
            coap_pkt->block2_num = coap_parse_int_option(current_option,
                    option_length);
            coap_pkt->block2_more = (coap_pkt->block2_num & 0x08) >> 3;
            coap_pkt->block2_size = 16 << (coap_pkt->block2_num & 0x07);
            coap_pkt->block2_offset = (coap_pkt->block2_num & ~0x0000000F)
                    << (coap_pkt->block2_num & 0x07);
            coap_pkt->block2_num >>= 4;
            PRINTF("Block2 [%lu%s (%u B/blk)]\n", coap_pkt->block2_num,
                    coap_pkt->block2_more ? "+" : "", coap_pkt->block2_size);
            break;
        case COAP_OPTION_BLOCK1:
            coap_pkt->block1_num = coap_parse_int_option(current_option,
                    option_length);
            coap_pkt->block1_more = (coap_pkt->block1_num & 0x08) >> 3;
            coap_pkt->block1_size = 16 << (coap_pkt->block1_num & 0x07);
            coap_pkt->block1_offset = (coap_pkt->block1_num & ~0x0000000F)
                    << (coap_pkt->block1_num & 0x07);
            coap_pkt->block1_num >>= 4;
            PRINTF("Block1 [%lu%s (%u B/blk)]\n", coap_pkt->block1_num,
                    coap_pkt->block1_more ? "+" : "", coap_pkt->block1_size);
            break;
        case COAP_OPTION_SIZE2:
            coap_pkt->size2 = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Size2 [%lu]\n", coap_pkt->size2);
            break;
        case COAP_OPTION_SIZE1:
            coap_pkt->size1 = coap_parse_int_option(current_option,
                    option_length);
            PRINTF("Size1 [%lu]\n", coap_pkt->size1);
            break;
        default:
            PRINTF("unknown (%u)\n", option_number);
            /* check if critical (odd) */
            if (option_number & 1) {
                coap_error_message = "Unsupported critical option";
                return BAD_OPTION_4_02;
            }
        }

        current_option += option_length;
    } /* for */
    PRINTF("-Done parsing-------\n");

    return NO_ERROR;
}

