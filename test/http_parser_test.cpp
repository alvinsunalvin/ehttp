/*
 * http_parser_test.cpp
 *
 *  Created on: Oct 30, 2014
 *      Author: liao
 */
#include "sim_parser.h"
#include "http_parser.h"
#include "simple_log.h"

#include "gtest/gtest.h"

const std::string test_req = "GET /hello?name=aaa&kw=%E4%B8%89%E6%98%8E HTTP/1.1\r\n" \
                             "Host: api.yeelink.net\r\n" \
                             "U-ApiKey: 121234132432143\r\n" \
                             "\r\n";

const std::string TEST_POST_REQ = "POST /login?u=a HTTP/1.0\r\n" \
                                  "Content-Type: application/x-www-form-urlencoded\r\n" \
                                  "Content-Length: 14\r\n" \
                                  "\r\n" \
                                  "name=aa&pwd=xx";

const std::string TEST_POST_REQ1 = "POST /lo";
const std::string TEST_POST_REQ2 = 
                                   "Hgin HTTP/1.0\r\nContent-Type: application/x-www-form-"
                                   "urlencoded\r\n"
                                   "Content-Length: 14\r\n"
                                   "\r\n"
                                   "name=aa&pwd=xx";


TEST(SimParserTest, test_get_unescape) {
    Request req;
    int ret = req.parse_request(test_req.c_str(), test_req.size());
    if (ret != 0) {
        LOG_ERROR("PARSE request error which ret:%d, req str:%s", ret, test_req.c_str());
    }
    ASSERT_EQ(0, ret);

    std::string name = req.get_param("name");
    std::string kw = req.get_unescape_param("kw");
    LOG_INFO("Get 'name' param :%s", name.c_str());
    LOG_INFO("Get 'kw' param :%s", kw.c_str());
    ASSERT_STREQ("aaa", name.c_str());
    ASSERT_STREQ("三明", kw.c_str());
}

TEST(SimParserTest, test_parse_post) {
    Request req;
    int ret = req.parse_request(TEST_POST_REQ.c_str(), TEST_POST_REQ.size());
    if (ret != 0) {
        LOG_ERROR("PARSE request error which ret:%d, req str:%s", ret, TEST_POST_REQ.c_str());
    }
    ASSERT_EQ(0, ret);
    std::string name = req.get_param("name");
    std::string pwd = req.get_param("pwd");
    LOG_INFO("Get 'name' param :%s", name.c_str());
    LOG_INFO("Get 'pwd' param :%s", pwd.c_str());
    ASSERT_STREQ("aa", name.c_str());
    ASSERT_STREQ("xx", pwd.c_str());
}

int on_message_begin(http_parser *p) {
    LOG_INFO("MESSAGE START!");
    return 0;
}

int on_status(http_parser *p, const char *buf, size_t len) {
    std::string status;
    status.assign(buf, len);
    LOG_INFO("on status:%s!", status.c_str());
    return 0;
}

int request_url_cb (http_parser *p, const char *buf, size_t len) {
    std::string url; 
    url.assign(buf, len);
    LOG_INFO("GET URL:%s", url.c_str());
    return 0;
}

int header_field_cb (http_parser *p, const char *buf, size_t len) {
    LOG_INFO("method:%d,http_major:%d,http_minor:%d ", p->method, p->http_major, p->http_minor);
    std::string field;
    field.assign(buf, len);
    LOG_INFO("GET field:%s", field.c_str());
    return 0;
}

int header_value_cb (http_parser *p, const char *buf, size_t len) {
    std::string field;
    field.assign(buf, len);
    LOG_INFO("GET value:%s", field.c_str());
    return 0;
}

int on_headers_complete(http_parser *p) {
    LOG_INFO("HEADERS COMPLETE!");
    return 0;
}

int on_body(http_parser *p, const char *buf, size_t len) {
    std::string field;
    field.assign(buf, len);
    LOG_INFO("GET body:%s", field.c_str());
    return 0;
}

int on_message_complete(http_parser *p) {
    LOG_INFO("msg COMPLETE! content-len:%d", p->content_length);
    return 0;
}

int on_chunk_header(http_parser *p) {
    LOG_INFO("on chunk header");
    return 0;
}

int test_http_parser() {
    http_parser_settings settings;
    settings.on_message_begin = NULL;
    settings.on_url = request_url_cb;
    settings.on_status = on_status;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_chunk_header = NULL;
    settings.on_chunk_complete = NULL;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    Request req;
    parser.data = &req;
    int nparsed = http_parser_execute(&parser, &settings, TEST_POST_REQ.c_str(), TEST_POST_REQ.size());
    LOG_INFO("parsed size:%d, parser->upgrade:%d,total_size:%u", nparsed, parser.upgrade, TEST_POST_REQ.size());
    if (parser.http_errno) {
        LOG_INFO("ERROR:%s", http_errno_description(HTTP_PARSER_ERRNO(&parser)));
    }
    return 0;
}

TEST(SimParserTest, test_http_parser_stream) {
    http_parser_settings settings;
    settings.on_message_begin = on_message_begin;
    settings.on_url = request_url_cb;
    settings.on_status = on_status;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_chunk_header = NULL;
    settings.on_chunk_complete = NULL;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    Request req;
    parser.data = &req;

    size_t nparsed = http_parser_execute(&parser, &settings, TEST_POST_REQ1.c_str(), TEST_POST_REQ1.size());
    LOG_INFO("parsed size:%d, parser->upgrade:%d,total_size:%u", nparsed, parser.upgrade, TEST_POST_REQ1.size());
    ASSERT_EQ(TEST_POST_REQ1.size(), nparsed);
    if (parser.http_errno) {
        LOG_INFO("ERROR:%s", http_errno_description(HTTP_PARSER_ERRNO(&parser)));
    }
    ASSERT_EQ((unsigned int)0, parser.http_errno);

    nparsed = http_parser_execute(&parser, &settings, TEST_POST_REQ2.c_str(), TEST_POST_REQ2.size());
    LOG_INFO("parsed size:%d, parser->upgrade:%d,total_size:%u", nparsed, parser.upgrade, TEST_POST_REQ2.size());
    ASSERT_EQ(TEST_POST_REQ2.size(), nparsed);
    if (parser.http_errno) {
        LOG_INFO("ERROR:%s", http_errno_description(HTTP_PARSER_ERRNO(&parser)));
    }
    ASSERT_EQ((unsigned int)0, parser.http_errno);
}

int test_http_parser_get() {
    http_parser_settings settings;
    settings.on_message_begin = NULL;
    settings.on_url = request_url_cb;
    settings.on_status = on_status;
    settings.on_header_field = header_field_cb;
    settings.on_header_value = header_value_cb;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_chunk_header = NULL;
    settings.on_chunk_complete = NULL;

    http_parser parser;
    http_parser_init(&parser, HTTP_REQUEST);
    Request req;
    parser.data = &req;
    int nparsed = http_parser_execute(&parser, &settings, test_req.c_str(), test_req.size());
    LOG_INFO("parsed size:%d, parser->upgrade:%d,total_size:%u", nparsed, parser.upgrade, test_req.size());
    if (parser.http_errno) {
        LOG_INFO("ERROR:%s", http_errno_description(HTTP_PARSER_ERRNO(&parser)));
    }
    return 0;
}
