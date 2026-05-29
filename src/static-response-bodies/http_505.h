#ifndef HTTP_505
#define HTTP_505
#define BODY_505 \
    "<!DOCTYPE html>" \
    "<html>" \
    "<head><title>505 HTTP version not supported</title></head>" \
    "<body>"            \
    "<h1>505 HTTP version not supported</h1>" \
    "</body>" \
    "</html>"

#define BODY_505_LEN (strlen(BODY_505))


#endif

