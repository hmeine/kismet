/*
    This file is part of Kismet

    Kismet is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kismet is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Kismet; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include "clinetframework.h"

NetworkClient::NetworkClient() {
    fprintf(stderr, "*** NetworkClient() called with no global registry\n");
}

NetworkClient::NetworkClient(GlobalRegistry *in_globalreg) {
    globalreg = in_globalreg;
    cl_valid = 0;
    cli_fd = -1;
    read_buf = NULL;
    write_buf = NULL;
}

NetworkClient::~NetworkClient() {
    KillConnection();
}

unsigned int NetworkClient::MergeSet(fd_set in_rset, fd_set in_wset, 
                                     unsigned int in_max_fd,
                                     fd_set *out_rset, fd_set *out_wset) {
    unsigned int max;

    /*
    Trust the caller to have given us good sets
    FD_ZERO(out_rset);
    FD_ZERO(out_wset);
    */

    if ((int) in_max_fd < cli_fd && cl_valid)
        max = cli_fd;
    else
        max = in_max_fd;

    for (unsigned int x = 0; x <= max; x++) {
        if (FD_ISSET(x, &in_rset))
            FD_SET(x, out_rset);
        if (FD_ISSET(x, &in_wset))
            FD_SET(x, out_wset);
    }

    if (cl_valid) {
        FD_SET(cli_fd, out_rset);

        if (write_buf->FetchLen() > 0)
            FD_SET(cli_fd, out_wset);
    }

    return max;
}

int NetworkClient::Poll(fd_set& in_rset, fd_set& in_wset) {
    int ret = 0;

    if (!cl_valid)
        return 0;

    // Look for stuff to read
    if (FD_ISSET(cli_fd, &in_rset)) {
        // If we failed reading, die.  The read handler will kill the link.
        if ((ret = ReadBytes()) < 0)
            return ret;

        // If we've got new data, try to parse.  if we fail, die.
        if (ret != 0 && cliframework->ParseData() < 0) {
            KillConnection();
            return -1;
        }
    }

    // Look for stuff to write
    if (FD_ISSET(cli_fd, &in_wset)) {
        // If we can't write data, die.  The write handler will kill the link.
        if ((ret = WriteBytes()) < 0)
            return ret;
    }
    
    
    return ret;
}

int NetworkClient::FlushRings() {
    if (!cl_valid)
        return -1;

    fd_set rset, wset;
    int max;
    
    time_t flushtime = time(0);

    // Nuke the fatal conditon so we can track our own failures
    int old_fcon = globalreg->fatal_condition;
    globalreg->fatal_condition = 0;
    
    while ((time(0) - flushtime) < 2) {
        if (write_buf->FetchLen() <= 0)
            return 1;

        max = 0;
        FD_ZERO(&rset);
        FD_ZERO(&wset);
       
        max = MergeSet(rset, wset, max, &rset, &wset);

        struct timeval tm;
        tm.tv_sec = 0;
        tm.tv_usec = 100000;

        if (select(max + 1, &rset, &wset, NULL, &tm) < 0) {
            if (errno != EINTR) {
                globalreg->fatal_condition = 1;
                return -1;
            }
        }

        if (Poll(rset, wset) < 0 || globalreg->fatal_condition != 0)
            return -1;
    }

    globalreg->fatal_condition = old_fcon;

    return 1;
}

void NetworkClient::KillConnection() {
    cliframework->KillConnection();

    if (cl_valid) {
        delete read_buf;
        delete write_buf;
    }

    if (cli_fd)
        close(cli_fd);

    cl_valid = 0;

    return;
}

int NetworkClient::WriteData(void *in_data, int in_len) {
    if (write_buf->InsertDummy(in_len) == 0) {
        snprintf(errstr, STATUS_MAX, "NetworkClient::WriateData no room in ring "
                 "buffer to insert %d bytes", in_len);
        globalreg->messagebus->InjectMessage(errstr, MSGFLAG_ERROR);
        return -1;
    }

    write_buf->InsertData((uint8_t *) in_data, in_len);
    
    return 1;
}

int NetworkClient::FetchReadLen() {
    return (int) read_buf->FetchLen();
}

int NetworkClient::ReadData(void *ret_data, int in_max, int *ret_len) {
    read_buf->FetchPtr((uint8_t *) ret_data, in_max, ret_len);

    return (*ret_len);
}

int NetworkClient::MarkRead(int in_readlen) {
    read_buf->MarkRead(in_readlen);

    return 1;
}

int ClientFramework::Shutdown() {
    int ret;

    if (netclient != NULL)
        ret = netclient->FlushRings();

    return ret;
}
