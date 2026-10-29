/*
 * libcli, a simple and generic command line interface with small footprint for
 * bare metal embedded projects.
 *
 * Copyright (C) 2026 Julian Friedrich
 *
 * This file is licensed under the MIT License, see LICENSE.txt for
 * details.
 *
 * SPDX-License-Identifier: MIT
 *
 * This project is hosted on GitHub:
 *   https://github.com/fjulian79/libcli
 * Please feel free to file issues, open pull requests, or contribute there.
 */

#include "cli/stream.hpp"
#include "cli/ascii.hpp"

/**
 * @brief Telnet commands and options, see RFC 854, 857 and 858.
 */
#define TELNET_SE               240
#define TELNET_SB               250
#define TELNET_WILL             251
#define TELNET_WONT             252
#define TELNET_DO               253
#define TELNET_DONT             254
#define TELNET_IAC              255

#define TELNET_OPT_ECHO         1
#define TELNET_OPT_SGA          3

/**
 * @brief Size of the buffer used to translate the output in telnet mode. Each
 * write to the parent may send a TCP packet, so the output is collected.
 */
#define CLI_STREAM_OUTBUF       32

CliStream::CliStream() :
      pParent(nullptr)
    , Mode(terminal)
    , LastWasCr(false)
    , TnState(tn_data)
    , TnCmd(0)
    , TnClientSga(false) {
}

CliStream::CliStream(Stream *pPar, mode_t mode) : CliStream() {
    begin(pPar, mode);
}

void CliStream::begin(Stream *pPar, mode_t mode) {
    pParent = pPar;
    Mode = mode;
    LastWasCr = false;
    TnState = tn_data;
    TnCmd = 0;
    TnClientSga = false;

    if (pParent != nullptr && Mode == telnet) {
        /* The server echoes and both sides suppress go ahead, the client
         * switches to character mode and stops echoing locally */
        telnetSend(TELNET_WILL, TELNET_OPT_ECHO);
        telnetSend(TELNET_WILL, TELNET_OPT_SGA);
    }
}

Stream *CliStream::parent(void) const {
    return pParent;
}

CliStream::mode_t CliStream::getMode(void) const {
    return Mode;
}

int CliStream::fetch(void) {
    if (pParent == nullptr) {
        return -1;
    }

    while (pParent->available() > 0) {
        int c = pParent->peek();

        if (c < 0) {
            break;
        }

        if (Mode == telnet && !telnetInput((uint8_t) c)) {
            continue;
        }

        if (LastWasCr && c == ascii.newline) {
            /* The second byte of Enter, CR LF from a terminal or telnet */
            pParent->read();
            LastWasCr = false;
            continue;
        }

        if (c == '\0') {
            /* No meaning on a terminal, CR NUL is Enter from telnet */
            pParent->read();
            LastWasCr = false;
            continue;
        }

        return c;
    }

    return -1;
}

bool CliStream::telnetInput(uint8_t c) {
    switch (TnState) {
        case tn_data:
            if (c != TELNET_IAC) {
                return true;
            }
            TnState = tn_command;
            break;

        case tn_command:
            if (c == TELNET_IAC) {
                /* Escaped 0xFF, a data byte. It is read by read(), which
                 * resets the state. */
                return true;
            }

            if (c >= TELNET_WILL && c <= TELNET_DONT) {
                TnCmd = c;
                TnState = tn_option;
            } else if (c == TELNET_SB) {
                TnState = tn_subneg;
            } else {
                /* A command without option, e.g. NOP or GA */
                TnState = tn_data;
            }
            break;

        case tn_option:
            TnState = tn_data;
            telnetReply(TnCmd, c);
            break;

        case tn_subneg:
            if (c == TELNET_IAC) {
                TnState = tn_subneg_iac;
            }
            break;

        case tn_subneg_iac:
            TnState = (c == TELNET_SE) ? tn_data : tn_subneg;
            break;
    }

    pParent->read();
    return false;
}

void CliStream::telnetReply(uint8_t cmd, uint8_t option) {
    /* Only requests to enable an option are answered, and only if the answer
     * differs from the current state. Anything else could end up in an
     * endless loop of acknowledgements, see RFC 854. */
    switch (cmd) {
        case TELNET_DO:
            /* ECHO and SGA have been offered by begin(), a DO acknowledges
             * them. Everything else is not supported. */
            if (option != TELNET_OPT_ECHO && option != TELNET_OPT_SGA) {
                telnetSend(TELNET_WONT, option);
            }
            break;

        case TELNET_WILL:
            if (option == TELNET_OPT_SGA) {
                if (!TnClientSga) {
                    TnClientSga = true;
                    telnetSend(TELNET_DO, option);
                }
            } else {
                telnetSend(TELNET_DONT, option);
            }
            break;

        default:
            /* DONT and WONT disable an option, nothing to acknowledge for the
             * options which are not supported anyway */
            break;
    }
}

void CliStream::telnetSend(uint8_t cmd, uint8_t option) {
    const uint8_t seq[3] = {TELNET_IAC, cmd, option};

    pParent->write(seq, sizeof(seq));
}

int CliStream::available(void) {
    return fetch() < 0 ? 0 : 1;
}

int CliStream::read(void) {
    int c = fetch();

    if (c < 0) {
        return -1;
    }

    pParent->read();
    if (TnState == tn_command) {
        /* The data byte was an escaped 0xFF */
        TnState = tn_data;
    }
    LastWasCr = (c == ascii.ret);

    return c;
}

int CliStream::peek(void) {
    return fetch();
}

size_t CliStream::write(uint8_t c) {
    return write(&c, 1);
}

size_t CliStream::write(const uint8_t *pBuf, size_t len) {
    uint8_t out[CLI_STREAM_OUTBUF];
    size_t cnt = 0;
    size_t done = 0;

    if (pParent == nullptr) {
        return 0;
    }

    if (Mode != telnet) {
        return pParent->write(pBuf, len);
    }

    for (size_t i = 0; i < len; i++) {
        uint8_t c = pBuf[i];

        /* Worst case a byte becomes two, flush before it might not fit */
        if (cnt > sizeof(out) - 2) {
            if (pParent->write(out, cnt) != cnt) {
                return done;
            }
            cnt = 0;
            done = i;
        }

        if (c == ascii.newline) {
            /* A new line is CR LF on a telnet connection */
            out[cnt++] = ascii.ret;
            out[cnt++] = c;
        } else if (c == ascii.ret) {
            /* CR LF stays as it is, a CR alone has to be sent as CR NUL */
            out[cnt++] = c;
            if (i + 1 < len && pBuf[i + 1] == ascii.newline) {
                out[cnt++] = pBuf[++i];
            } else {
                out[cnt++] = '\0';
            }
        } else if (c == TELNET_IAC) {
            /* A data byte 0xFF has to be escaped */
            out[cnt++] = c;
            out[cnt++] = c;
        } else {
            out[cnt++] = c;
        }
    }

    if (cnt > 0 && pParent->write(out, cnt) != cnt) {
        return done;
    }

    return len;
}

void CliStream::flush(void) {
    if (pParent != nullptr) {
        pParent->flush();
    }
}
