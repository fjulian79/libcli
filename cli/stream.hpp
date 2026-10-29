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

#pragma once

#include <Arduino.h>

/**
 * @brief The stream libCli and its commands use for io operations.
 *
 * Wraps the stream given to Cli::begin(), its parent, and does what the line
 * discipline of the kernel does on Linux, see doc/line-endings.md:
 *
 * - The second byte of Enter is dropped, i.e. CR LF and CR NUL are read as a
 *   single CR, regardless of when the second byte arrives. A NUL byte is
 *   dropped everywhere.
 * - In telnet mode the telnet protocol is handled as well. The telnet
 *   commands sent by the client are removed and answered, the client is
 *   switched to character mode and a new line is sent as CR LF.
 *
 * Every command gets this stream as ioStream, so a command reading input only
 * ever sees a single CR for Enter. A command needing the unfiltered data, e.g.
 * to transfer binary data, uses the parent stream.
 *
 * No data is buffered, the filter works on the parent by peek() and read().
 * The available() count is therefore 1 while data is available, and not the
 * number of bytes.
 */
class CliStream : public Stream {

    public:

        /**
         * @brief The protocol spoken on the parent stream.
         */
        typedef enum {
            /* A terminal, e.g. on a serial port */
            terminal = 0,

            /* A telnet client, e.g. a WiFiClient accepted by a server */
            telnet
        } mode_t;

        CliStream();

        /**
         * @brief Construct a stream already attached to a parent.
         */
        CliStream(Stream *pParent, mode_t mode = terminal);

        /**
         * @brief Attaches the stream to a parent and resets the state.
         *
         * In telnet mode the negotiation of character mode is sent to the
         * client, to be called once for every new connection.
         *
         * @param pParent   The stream to wrap, nullptr to detach.
         * @param mode      The protocol spoken on the parent.
         */
        void begin(Stream *pParent, mode_t mode = terminal);

        /**
         * @brief The wrapped stream, nullptr if detached.
         */
        Stream *parent(void) const;

        /**
         * @brief The protocol spoken on the parent stream.
         */
        mode_t getMode(void) const;

        /**
         * @return 1 if a data byte is available, 0 otherwise.
         */
        int available(void) override;

        int read(void) override;

        int peek(void) override;

        size_t write(uint8_t c) override;

        /**
         * @brief Writes to the parent, in telnet mode an LF is sent as CR LF,
         * a CR not followed by LF as CR NUL and 0xFF as IAC IAC.
         *
         * @return The number of bytes of pBuf written, less than len if the
         * parent failed.
         */
        size_t write(const uint8_t *pBuf, size_t len) override;

        void flush(void) override;

        using Print::write;

    private:

        /**
         * @brief Skips and processes all bytes of the parent which are no
         * data, never blocks.
         *
         * @return The next data byte, which is still in the parent, or -1 if
         * none is available.
         */
        int fetch(void);

        /**
         * @brief Processes a byte of the telnet protocol.
         *
         * @param c The next byte of the parent, not read yet.
         * @return true if it is a data byte, false if it has been read and
         * processed as part of a telnet command.
         */
        bool telnetInput(uint8_t c);

        /**
         * @brief Answers an option request of the telnet client.
         */
        void telnetReply(uint8_t cmd, uint8_t option);

        /**
         * @brief Sends a telnet command to the client.
         */
        void telnetSend(uint8_t cmd, uint8_t option);

        Stream *pParent;

        mode_t Mode;

        /**
         * @brief true if the last data byte read was a CR.
         */
        bool LastWasCr;

        /**
         * @brief State of the telnet command parser.
         */
        enum {
            tn_data = 0,
            tn_command,
            tn_option,
            tn_subneg,
            tn_subneg_iac
        } TnState;

        /**
         * @brief The telnet command of an option request being parsed.
         */
        uint8_t TnCmd;

        /**
         * @brief true once suppress go ahead of the client has been accepted.
         */
        bool TnClientSga;
};
