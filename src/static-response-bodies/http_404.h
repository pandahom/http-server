#ifndef HTTP_404
#define HTTP_404

#define BODY_404 \
    "<!DOCTYPE html>\r\n" \
    "<html>\r\n" \
    "  <head><title>404 Not Found</title></head>\r\n" \
    "  <body>\r\n" \
    "    <h1>404 Not Found</h1>\r\n" \
    "    <p>The requested resource could not be found on this server.</p>\r\n" \
    "  </body>\r\n" \
    "</html>\r\n"

#define BODY_404_LEN (sizeof(BODY_404) - 1)
#endif