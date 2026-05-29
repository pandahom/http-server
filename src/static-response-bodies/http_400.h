#ifndef HTTP_400
#define HTTP_400
#define BODY_400 \
    "<!DOCTYPE html>\r\n" \
    "<html>\r\n" \
    "  <head><title>400 Bad Request</title></head>\r\n" \
    "  <body>\r\n" \
    "    <h1>400 Bad Request</h1>\r\n" \
    "    <p>The server could not understand your request.</p>\r\n" \
    "  </body>\r\n" \
    "</html>\r\n"
#define BODY_400_LEN (sizeof(BODY_400) - 1)

#endif