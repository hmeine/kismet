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

#ifndef __CLINETFRAMEWORK_H__
#define __CLINETFRAMEWORK_H__

#include "config.h"

#include <stdio.h>
#include <string>
#include <time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <map>
#include <vector>

#include "messagebus.h"
#include "timetracker.h"
#include "globalregistry.h"
#include "ringbuf.h"

// Basic superclass frameworks for network clients.  Same basic sctructure
// as the server framework

// Forward prototypes
class NetworkClient;
class ClientFramework;

// Skeleton for a network server
class NetworkClient {
public:
    NetworkClient();
    NetworkClient(GlobalRegistry *in_globalreg);
    virtual ~NetworkClient() = 0;

    // Register a client protocol framework
    virtual void RegisterClientFramework(ClientFramework *in_frm) {
        cliframework = in_frm;
    }

    // Is the client valid for any other ops?
    virtual int Valid() {
        return cl_valid;
    }

    // Core select loop merge - combine FDs with the master FD list, and
    // handle a strobe across pending FDs
    virtual unsigned int MergeSet(fd_set in_rset, fd_set in_wset, 
                                  unsigned int in_max_fd,
                                  fd_set *out_rset, fd_set *out_wset);
    virtual int Poll(fd_set& in_rset, fd_set& in_wset);

    // Flush all output buffers if we can
    virtual int FlushRings();

    // Connect
    virtual int Connect(const char *in_remotehost, short int in_port) = 0;
    
    // Kill the connection
    virtual void KillConnection();

    // Read, write, and mark are essentially fallthroughs directly to
    // the ringbuffers.  We actually define these since it's 
    // unlikely that they'd be drastically overridden

    // Write data 
    virtual int WriteData(void *in_data, int in_len);

    // Amount of data pending in a client ring
    virtual int FetchReadLen();

    // Read data 
    virtual int ReadData(void *ret_data, int in_max, int *ret_len);

    // Mark data as read (ie, we've extracted it and parsed it)
    virtual int MarkRead(int in_readlen);

protected:
    // Validate a connection
    virtual int Validate() = 0;
   
    // Read pending bytes from whereever into the ringbuffer
    virtual int ReadBytes() = 0;
    // Write pending bytes from the ringbuffer to whatever
    virtual int WriteBytes() = 0;

    char errstr[STATUS_MAX];

    int cl_valid;
    int cli_fd;

    GlobalRegistry *globalreg;
    ClientFramework *cliframework;

    RingBuffer *read_buf;
    RingBuffer *write_buf;

    struct sockaddr_in client_sock, local_sock;
    struct hostent *client_host;
};

// Skeleton to a protocol interface
class ClientFramework {
public:
    ClientFramework() {
        globalreg = NULL;
        netclient = NULL;
        fprintf(stderr, "*** ClientFramework called with no global registry\n");
    };

    ClientFramework(GlobalRegistry *in_reg) {
        globalreg = in_reg;
        netclient = NULL;
    }

    virtual ~ClientFramework() { };

    // Register the network server core that we use to talk out
    void RegisterNetworkClient(NetworkClient *in_netc) {
        netclient = in_netc;
    }

    // Parse data 
    virtual int ParseData() = 0;

    // Kill a connection
    virtual int KillConnection() = 0;

    // Shutdown the protocol
    virtual int Shutdown();
    
protected:
    char errstr[STATUS_MAX];

    GlobalRegistry *globalreg;
    NetworkClient *netclient;
};

#endif
