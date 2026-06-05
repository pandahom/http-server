#ifndef DEFAULT_DIR_LIST_PAGE
#define DEFAULT_DIR_LIST_PAGE


#define DEFAULT_PAGE \
        "<!DOCTYPE html>" \
        "<html>" \
           "<head>"\
               "<title>Directory Listing %s</title>" \
               "<style>"\
                 "body {"\
                   "font-family: monospace;"\
                   "margin: 2rem;"\
                 "}"\
                 "table {"\
                   "border-collapse: collapse;"\
                   "width: 100%;"\
                 "}"\
                 "th, td {"\
                   "padding: 0.4rem 0.8rem;"\
                   "text-align: left;"\
                 "}"\
                 "th {"\
                   "border-bottom: 1px solid #ccc;"\
                 "}"\
                 "a {"\
                   "text-decoration: none;"\
                 "}"\
               "</style>"\
           "</head>"\
           "<body>" \
               "<h1><h1>Index of %s</h1>" \
               "<table>"\
                  "<thead>"\
                    "<tr>"\
                        "<th>Name</th>"\
                        "<th>Type</th>"\
                    "</tr>"\
                  "</thead>"\
                  "<tbody>"\
                    "%s"\
                  "</tbody>"\
               "</table>"\
           "</body>" \
        "</html>"

#define ELEMENT_FORMAT \
    "<tr>"\
        "<td><a href=\"%s\">%s</a></td>"\
        "<td>%s</td>"\
    "</tr>"\

#endif
