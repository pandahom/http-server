# HTTP Server

A small HTTP server written in C for personal practice. The project focuses on
socket programming, simple HTTP request parsing, response building,
per-connection state machines, and serving static content from a configurable
local document root.

## Features

- Listens on `127.0.0.4:8080` by default with a TCP backlog of `5`.
- Lets you configure the listen IP address, TCP port, and document root at
  startup with command-line options.
- Handles each accepted client connection in a detached POSIX thread.
- Parses the HTTP request line and headers with an internal state-machine parser.
- Accepts `HTTP/1.0` and `HTTP/1.1` requests.
- Implements:
    - `GET` for files and directories.
    - `HEAD` for files and directories.
- Generates a simple HTML directory listing for directory requests.
- Adds `Connection: Close`, `Content-Type`, and `Content-Length` headers to
  generated responses.
- Returns built-in HTML error pages for unsupported methods, missing paths, and
  unsupported HTTP versions.

## Requirements

- `gcc`
- `make`
- POSIX threads (`pthread`)

## Build

```sh
make
```

This creates the `serve-file` executable in the repository root and object files
inside `build/`.

To remove build outputs:

```sh
make clean
```

## Usage

```sh
./serve-file [-i ip_address] [-p port] [-d document_root]
```

| Option | Description                              | Default           |
| --- |------------------------------------------|-------------------|
| `-i ip_address` | IP(V4/V6) address to bind and listen on. | `127.0.0.4`       |
| `-p port` | TCP port to bind and listen on.          | `8080`            |
| `-d document_root` | Existing directory to serve files from.  | working directory |
| `-h` | Print usage information.                 | n/a               |

## Installation

1. Clone and build server:

   ```sh
   git clone https://github.com/pandahom/http-server.git
   cd http-server
   make
   ```

2. Start the server:

   ```sh
   ./serve-file
   ```

   The default server listens on `127.0.0.4:8080` and serves working directory.
   ```sh
   ./serve-file -i 127.0.0.2 -p 54312 -d $HOME
   ```
   You can serve any existing directory by passing `-d`:

## Supported responses

The server currently builds responses for these status codes:

| Status | When it is used |
| --- | --- |
| `200 OK` | Existing files or generated directory listing pages. |
| `404 Not Found` | Requested path does not exist under the configured document root. |
| `501 Not Implemented` | Request method is not currently supported. |
| `505 HTTP Version Not Supported` | Request version is not `HTTP/1.0` or `HTTP/1.1`. |

## Content types

Static file responses map common extensions to content types:

| Extension | Content-Type |
| --- | --- |
| `.html`, `.htm` | `text/html` |
| `.css` | `text/css` |
| `.js` | `application/javascript` |
| `.json` | `application/json` |
| `.png` | `image/png` |
| `.jpg`, `.jpeg` | `image/jpeg` |
| `.gif` | `image/gif` |
| `.pdf` | `application/pdf` |
| `.txt` | `text/plain` |
| anything else | `application/octet-stream` |

## Project layout

```text
.
├── Makefile
├── README.md
└── src
    ├── main.c                         # Program entry point and CLI parsing
    ├── server-sm.*                    # Server state machine
    ├── server.*                       # Socket setup, bind/listen/accept, threads
    ├── connection-sm.*                # Per-client connection state machine
    ├── conn-client.*                  # Receive, parse, process, send, cleanup
    ├── request-handler.*              # HTTP parser, method handling, document root
    ├── response-build.*               # HTTP response construction and headers
    ├── path-handler.*                 # Directory listing helpers
    ├── ds/                            # Linked list and hash table helpers
    └── static-response-bodies/        # Built-in HTML response templates
```

## Current limitations

- Request bodies are not processed, and `POST` is currently a no-op path in the
  dispatcher.
- Persistent connections are not supported; every response includes
  `Connection: Close`.
- Path normalization and URL decoding are marked as TODOs, so this should not be
  exposed to untrusted networks as-is.
- There is no TLS support.
