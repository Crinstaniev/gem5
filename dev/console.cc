/*
 * Copyright (c) 2003 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* @file
 * User Console Definitions
 */

#include <sys/ioctl.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "base/misc.hh"
#include "base/socket.hh"
#include "base/trace.hh"
#include "dev/console.hh"
#include "mem/functional_mem/memory_control.hh"
#include "sim/builder.hh"
#include "targetarch/ev5.hh"
#include "dev/platform.hh"

using namespace std;

////////////////////////////////////////////////////////////////////////
//
//

SimConsole::Event::Event(SimConsole *c, int fd, int e)
    : PollEvent(fd, e), cons(c)
{
}

void
SimConsole::Event::process(int revent)
{
    if (revent & POLLIN)
        cons->data();
    else if (revent & POLLNVAL)
        cons->detach();
}

SimConsole::SimConsole(const string &name, const string &file, int num)
    : SimObject(name), event(NULL), number(num), in_fd(-1), out_fd(-1),
      listener(NULL), txbuf(16384), rxbuf(16384), outfile(NULL),
#if TRACING_ON == 1
      linebuf(16384),
#endif
      _status(0), _enable(0), intr(NULL), platform(NULL)
{
    if (!file.empty())
        outfile = new ofstream(file.c_str());

    if (outfile)
        outfile->setf(ios::unitbuf);
}

SimConsole::~SimConsole()
{
    close();

    if (outfile)
        delete outfile;
}

void
SimConsole::close()
{
    if (in_fd != -1)
        ::close(in_fd);

    if (out_fd != in_fd && out_fd != -1)
        ::close(out_fd);
}

void
SimConsole::attach(int in, int out, ConsoleListener *l)
{
    in_fd = in;
    out_fd = out;
    listener = l;

    event = new Event(this, in, POLLIN);
    pollQueue.schedule(event);

    stringstream stream;
    ccprintf(stream, "==== m5 slave console: Console %d ====", number);

    // we need an actual carriage return followed by a newline for the
    // terminal
    stream << "\r\n";

    write((const uint8_t *)stream.str().c_str(), stream.str().size());


    DPRINTFN("attach console %d\n", number);

    txbuf.readall(out);
}

void
SimConsole::detach()
{
    close();
    in_fd = -1;
    out_fd = -1;

    pollQueue.remove(event);

    if (listener) {
        listener->add(this);
        listener = NULL;
    }

    DPRINTFN("detach console %d\n", number);
}

void
SimConsole::data()
{
    uint8_t buf[1024];
    int len;

    len = read(buf, sizeof(buf));
    if (len) {
        rxbuf.write((char *)buf, len);
        raiseInt(ReceiveInterrupt);
    }
}

size_t
SimConsole::read(uint8_t *buf, size_t len)
{
    if (in_fd < 0)
        panic("SimConsole(read): Console not properly attached.\n");

    size_t ret;
    do {
      ret = ::read(in_fd, buf, len);
    } while (ret == -1 && errno == EINTR);


    if (ret < 0)
        DPRINTFN("SimConsole(read): Read failed.\n");

    if (ret <= 0) {
        detach();
        return 0;
    }

    return ret;
}

// Console output.
size_t
SimConsole::write(const uint8_t *buf, size_t len)
{
    if (out_fd < 0)
        panic("SimConsole(write): Console not properly attached.\n");

    size_t ret;
    for (;;) {
      ret = ::write(out_fd, buf, len);

      if (ret >= 0)
        break;

      if (errno != EINTR)
          detach();
    }

    return ret;
}

void
SimConsole::configTerm()
{
    struct termios ios;

    if (isatty(out_fd)) {
        if (tcgetattr(out_fd, &ios) < 0) {
            panic( "tcgetattr\n");
        }
        ios.c_iflag &= ~(ISTRIP|ICRNL|IGNCR|ICRNL|IXOFF|IXON);
        ios.c_oflag &= ~(OPOST);
        ios.c_oflag &= (ONLCR);
        ios.c_lflag &= ~(ISIG|ICANON|ECHO);
        ios.c_cc[VMIN] = 1;
        ios.c_cc[VTIME] = 0;
        if (tcsetattr(out_fd, TCSANOW, &ios) < 0) {
            panic( "tcsetattr\n");
        }
    }
}

#define MORE_PENDING (ULL(1) << 61)
#define RECEIVE_SUCCESS (ULL(0) << 62)
#define RECEIVE_NONE (ULL(2) << 62)
#define RECEIVE_ERROR (ULL(3) << 62)

bool
SimConsole::in(uint8_t &c)
{
    bool empty, ret;

    empty = rxbuf.empty();
    ret = !empty;
    if (!empty) {
        rxbuf.read((char *)&c, 1);
        empty = rxbuf.empty();
    }

    if (empty)
        clearInt(ReceiveInterrupt);

    DPRINTF(ConsoleVerbose, "in: \'%c\' %#02x more: %d, return: %d\n",
            isprint(c) ? c : ' ', c, !empty, ret);

    return ret;
}

uint64_t
SimConsole::console_in()
{
    uint8_t c;
    uint64_t value;

    if (in(c)) {
        value = RECEIVE_SUCCESS | c;
        if (!rxbuf.empty())
            value  |= MORE_PENDING;
    } else {
        value = RECEIVE_NONE;
    }

    DPRINTF(ConsoleVerbose, "console_in: return: %#x\n", value);

    return value;
}

void
SimConsole::out(char c, bool raise_int)
{
#if TRACING_ON == 1
    if (DTRACE(Console)) {
        static char last = '\0';

        if (c != '\n' && c != '\r' ||
            last != '\n' && last != '\r') {
            if (c == '\n' || c == '\r') {
                int size = linebuf.size();
                char *buffer = new char[size + 1];
                linebuf.read(buffer, size);
                buffer[size] = '\0';
                DPRINTF(Console, "%s\n", buffer);
                delete [] buffer;
            } else {
                linebuf.write(c);
            }
        }

        last = c;
    }
#endif

    txbuf.write(c);

    if (out_fd >= 0)
        write(c);

    if (outfile)
        outfile->write(&c, 1);

    if (raise_int)
        raiseInt(TransmitInterrupt);

    DPRINTF(ConsoleVerbose, "out: \'%c\' %#02x",
            isprint(c) ? c : ' ', (int)c);

    if (raise_int)
        DPRINTF(ConsoleVerbose, "status: %#x\n", _status);
    else
        DPRINTF(ConsoleVerbose, "\n");
}

inline bool
MaskStatus(int status, int mask)
{ return (status & mask) != 0; }

int
SimConsole::clearInt(int i)
{
    int old = _status;
    _status &= ~i;
    //if (MaskStatus(old, _enable) != MaskStatus(_status, _enable) && intr)
        platform->clearConsoleInt();

    return old;
}

void
SimConsole::raiseInt(int i)
{
    //int old = _status;
    _status |= i;
    //if (MaskStatus(old, _enable) != MaskStatus(_status, _enable) && intr)
        platform->postConsoleInt();
}

void
SimConsole::initInt(IntrControl *i)
{
    if (intr)
        panic("Console has already been initialized.");

    intr = i;
}

void
SimConsole::setInt(int bits)
{
    int old;

    if (bits & ~(TransmitInterrupt | ReceiveInterrupt))
        panic("An interrupt was not set!");

    old = _enable;
    _enable |= bits;

    //if (MaskStatus(_status, old) != MaskStatus(_status, _enable) && intr) {
    if (intr) {
        if (MaskStatus(_status, _enable))
            platform->postConsoleInt();
        else
            platform->clearConsoleInt();
    }
}

void
SimConsole::setPlatform(Platform *p)
{
    platform = p;
    platform->cons = this;
}

void
SimConsole::serialize(ostream &os)
{
    SERIALIZE_SCALAR(_status);
    SERIALIZE_SCALAR(_enable);
}

void
SimConsole::unserialize(Checkpoint *cp, const std::string &section)
{
    UNSERIALIZE_SCALAR(_status);
    UNSERIALIZE_SCALAR(_enable);
}


BEGIN_DECLARE_SIM_OBJECT_PARAMS(SimConsole)

    SimObjectParam<ConsoleListener *> listener;
    SimObjectParam<IntrControl *> intr_control;
    SimObjectParam<Platform *> platform;
    Param<string> output;
    Param<bool> append_name;
    Param<int> number;

END_DECLARE_SIM_OBJECT_PARAMS(SimConsole)

BEGIN_INIT_SIM_OBJECT_PARAMS(SimConsole)

    INIT_PARAM(listener, "console listener"),
    INIT_PARAM(intr_control, "interrupt controller"),
    INIT_PARAM(platform, "platform"),
    INIT_PARAM_DFLT(output, "file to dump output to", ""),
    INIT_PARAM_DFLT(append_name, "append name() to filename", true),
    INIT_PARAM_DFLT(number, "console number", 0)

END_INIT_SIM_OBJECT_PARAMS(SimConsole)

CREATE_SIM_OBJECT(SimConsole)
{
    string filename = output;
    if (filename.empty()) {
        if (!outputDirectory.empty())
            filename = outputDirectory + getInstanceName();
    } else {
        if (append_name)
            filename += "." + getInstanceName();
        if (!outputDirectory.empty())
            filename = outputDirectory + filename;
    }

    SimConsole *console = new SimConsole(getInstanceName(), filename, number);
    ((ConsoleListener *)listener)->add(console);
    ((SimConsole *)console)->initInt(intr_control);
    ((SimConsole *)console)->setPlatform(platform);
    //((SimConsole *)console)->setInt(SimConsole::TransmitInterrupt |
    //                                SimConsole::ReceiveInterrupt);

    return console;
}

REGISTER_SIM_OBJECT("SimConsole", SimConsole)

////////////////////////////////////////////////////////////////////////
//
//

ConsoleListener::ConsoleListener(const string &name)
    : SimObject(name), event(NULL)
{}

ConsoleListener::~ConsoleListener()
{
    if (event)
        delete event;
}

void
ConsoleListener::Event::process(int revent)
{
    listener->accept();
}

///////////////////////////////////////////////////////////////////////
// socket creation and console attach
//

void
ConsoleListener::listen(int port)
{
    while (!listener.listen(port, true)) {
        DPRINTF(Console,
                ": can't bind address console port %d inuse PID %d\n",
                port, getpid());
        port++;
    }

    ccprintf(cerr, "Listening for console connection on port %d\n", port);

    event = new Event(this, listener.getfd(), POLLIN);
    pollQueue.schedule(event);
}

void
ConsoleListener::add(SimConsole *cons)
{ ConsoleList.push_back(cons);}

void
ConsoleListener::accept()
{
    if (!listener.islistening())
        panic("%s: cannot accept a connection if not listening!", name());

    int sfd = listener.accept(true);
    if (sfd != -1) {
        iter_t i = ConsoleList.begin();
        iter_t end = ConsoleList.end();
        if (i == end) {
            close(sfd);
        } else {
            (*i)->attach(sfd, this);
            i = ConsoleList.erase(i);
        }
    }
}

BEGIN_DECLARE_SIM_OBJECT_PARAMS(ConsoleListener)

    Param<int> port;

END_DECLARE_SIM_OBJECT_PARAMS(ConsoleListener)

BEGIN_INIT_SIM_OBJECT_PARAMS(ConsoleListener)

    INIT_PARAM_DFLT(port, "listen port", 3456)

END_INIT_SIM_OBJECT_PARAMS(ConsoleListener)

CREATE_SIM_OBJECT(ConsoleListener)
{
    ConsoleListener *listener = new ConsoleListener(getInstanceName());
    listener->listen(port);

    return listener;
}

REGISTER_SIM_OBJECT("ConsoleListener", ConsoleListener)
