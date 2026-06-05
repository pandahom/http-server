#ifndef HTTP_501
#define HTTP_501
#define BODY_501 \
       "<!DOCTYPE HTML>"\
       "<html lang=\"en\">"\
           "<head>"\
               "<title>Not Implemented</title>"\
           "</head>"\
           "<body>"\
               "<p>501 Not Implemented</p>"\
               "<p>Message: Unsupported method ('%s').</p>"\
               "<p>Error code explanation: 501 - Server does not support this operation.</p>"\
           "</body>"    \
       "</html>"
#endif