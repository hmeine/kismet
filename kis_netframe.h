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

#ifndef __KISNETFRAME_H__
#define __KISNETFRAME_H__

#include "config.h"

#include "util.h"
#include "messagebus.h"
#include "netframework.h"
#include "tracktypes.h"

// Forward prototype
class KisNetFramework;

// Client command structure for incoming commands.  Given the ID of the client
// and the parsed ID of the command, the server framework, the globals, and the
// remainder of the command line (after cmdid and command itself).  For extra
// fun we pass the cmdline we split apart
#define CLIENT_PARMS int in_clid, KisNetFramework *framework, \
                     GlobalRegistry *globalreg, char *errstr, string cmdline, \
                     vector<smart_word_token> *parsedcmdline
typedef int (*ClientCommand)(CLIENT_PARMS);

// Protocol parameters
#define PROTO_PARMS string& out_string, const vector<int> *field_vec, \
        const void *data, GlobalRegistry *globalreg
typedef int (*ProtoCallback)(PROTO_PARMS);

#define PROTO_ENABLE_PARMS int in_fd, GlobalRegistry *globalreg
typedef void (*ProtoEnableCallback)(PROTO_ENABLE_PARMS);

// Lowlevel protocols that get inserted into the server during setup, these
// MUST be supported ASAP
enum KISMET_fields {
    KISMET_version, KISMET_starttime, KISMET_servername, KISMET_timestamp,
    KISMET_chanhop, KISMET_newversion
};

enum ERROR_fields {
    ERROR_cmdid, ERROR_cmdtext
};

enum ACK_fields {
    ACK_cmdid, ACK_cmdtext
};

enum PROTOCOL_fields {
    PROTOCOL_protocols
};

enum CAPABILITY_fields {
    CAPABILITY_capabilities
};

enum TERMINATE_fields {
    TERMINATE_text
};

enum TIME_fields {
    TIME_timesec
};

enum STATUS_fields {
    STATUS_text
};

enum PACKET_fields {
    PACKET_type, PACKET_subtype, PACKET_timesec, PACKET_encrypted,
    PACKET_weak, PACKET_beaconrate, PACKET_sourcemac, PACKET_destmac,
    PACKET_bssid, PACKET_ssid, PACKET_prototype, PACKET_sourceip,
    PACKET_destip, PACKET_sourceport, PACKET_destport, PACKET_nbtype,
    PACKET_nbsource, PACKET_sourcename
};

enum STRING_fields {
    STRING_bssid, STRING_sourcemac, STRING_text
};

enum CISCO_fields {
    CISCO_placeholder
};

enum INFO_fields {
    INFO_networks, INFO_packets, INFO_crypt, INFO_weak,
    INFO_noise, INFO_dropped, INFO_rate, INFO_signal
};

enum WEPKEY_fields {
    WEPKEY_origin, WEPKEY_bssid, WEPKEY_key, WEPKEY_decrypted, WEPKEY_failed
};

enum CARD_fields {
    CARD_interface, CARD_type, CARD_username, CARD_channel, CARD_id, CARD_packets,
    CARD_hopping
};

extern char *KISMET_fields_text[];
extern char *ERROR_fields_text[];
extern char *ACK_fields_text[];
extern char *PROTOCOLS_fields_text[];
extern char *CAPABILITY_fields_text[];
extern char *TERMINATE_fields_text[];
extern char *TIME_fields_text[];

extern char *CARD_fields_text[];
extern char *CISCO_fields_text[];
extern char *INFO_fields_text[];
extern char *PACKET_fields_text[];
extern char *STATUS_fields_text[];
extern char *STRING_fields_text[];
extern char *WEPKEY_fields_text[];

// Client/server protocol data structures.  These get passed as void *'s to each of the
// protocol functions.
// These are all done in two main ways - a var for each field, or a vector in the
// same order as the field names.  For shorter ones, the code is a lot more maintainable
// to have named vars, for longer ones it just makes sense to use a big ordered vector

typedef struct INFO_data {
    string networks, packets, crypt, weak, noise, dropped, rate, signal;
};

typedef struct PACKET_data {
    vector<string> pdvec;
};

typedef struct STRING_data {
    string bssid, sourcemac, text;
};

int Protocol_KISMET(PROTO_PARMS);
int Protocol_ERROR(PROTO_PARMS);
int Protocol_ACK(PROTO_PARMS);
int Protocol_PROTOCOLS(PROTO_PARMS);
int Protocol_CAPABILITY(PROTO_PARMS);
int Protocol_TERMINATE(PROTO_PARMS);
int Protocol_TIME(PROTO_PARMS);
int Protocol_INFO(PROTO_PARMS); // INFO_data
int Protocol_STATUS(PROTO_PARMS); // string
void Protocol_Packet2Data(const kis_packet *info, PACKET_data *data);
int Protocol_PACKET(PROTO_PARMS); // PACKET_data
int Protocol_STRING(PROTO_PARMS); // STRING_data
int Protocol_WEPKEY(PROTO_PARMS); // wep_key_info
int Protocol_CARD(PROTO_PARMS); // captype
int Protocol_CISCO(PROTO_PARMS);

typedef struct KISMET_data {
    string version;
    string starttime;
    string servername;
    string timestamp;
    string newversion;
};

typedef struct CLIRESP_data {
    int cmdid;
    string resptext;
};

// Builtin commands we have to handle
int Clicmd_CAPABILITY(CLIENT_PARMS);
int Clicmd_ENABLE(CLIENT_PARMS);
int Clicmd_REMOVE(CLIENT_PARMS);
int Clicmd_CHANLOCK(CLIENT_PARMS);
int Clicmd_CHANHOP(CLIENT_PARMS);
int Clicmd_PAUSE(CLIENT_PARMS);
int Clicmd_RESUME(CLIENT_PARMS);
int Clicmd_LISTWEPKEYS(CLIENT_PARMS);
int Clicmd_ADDWEPKEY(CLIENT_PARMS);
int Clicmd_DELWEPKEY(CLIENT_PARMS);

// Messagebus subscriber to pass data to the client
class KisNetframe_MessageClient : public MessageClient {
public:
    KisNetframe_MessageClient(GlobalRegistry *in_globalreg) :
        MessageClient(in_globalreg) { };
    void ProcessMessage(string in_msg, int in_flags);
};

// Timer events
int KisNetFrame_TimeEvent(Timetracker::timer_event *evt, void *parm, GlobalRegistry *globalreg);

// Kismet server framework for sending data to clients and processing
// commands from clients
class KisNetFramework : public ServerFramework {
public:
    typedef struct server_protocol {
        int ref_index;
        string header;
        int required;
        // Double-listed (burns a little extra ram but not much) to make mapping requested
        // fields fast.
        map<string, int> field_map;
        vector<string> field_vec;
        int (*printer)(PROTO_PARMS);
        void (*enable)(PROTO_ENABLE_PARMS);
    };

    KisNetFramework();
    KisNetFramework(GlobalRegistry *in_globalreg);
    virtual ~KisNetFramework();
 
    virtual int Accept(int in_fd);
    virtual int ParseData(int in_fd);
    virtual int KillConnection(int in_fd);

    // Send a protocol to a specific client
    int SendToClient(int in_fd, int in_refnum, const void *in_data);
    // Send to all clients
    int SendToAll(int in_refnum, const void *in_data);
    
    // Learn a client command
    int RegisterClientCommand(string in_cmdword, ClientCommand in_cmd);

    // Register an output sentence.  This needs:
    // * A header (ie, NETWORK)
    // * A NULL-terminated array of fields
    // * A pointer to a printer that takes a void * and a vector of field numbers
    //   and outputs a c++ string
    // * An optional pointer to a function that takes the file descriptor of a client
    //   that triggers whatever events should happen the the client enables this kind
    //   of protocol.  (ie, send all networks when the client enables the *NETWORK
    //   protocol)
    // It returns the index number of the sentence added.
    int RegisterProtocol(string in_header, int in_required, char **in_fields,
                                 int (*in_printer)(PROTO_PARMS),
                                 void (*in_enable)(PROTO_ENABLE_PARMS));
    int FetchProtocolRef(string in_header);
    KisNetFramework::server_protocol *FetchProtocol(int in_ref);

    // Manipulate client info
    void AddProtocolClient(int in_fd, int in_refnum, vector<int> in_fields);
    void DelProtocolClient(int in_fd, int in_refnum);
    
    // How many clients are using this protocol type?
    int FetchNumClientRefs(int in_refnum);

    // How many clients total?
    int FetchNumClients();

protected:
    // Messagebus client
    KisNetframe_MessageClient *kisnet_msgcli;

    // Client options
    struct client_opt {
        // Map of sentence references to field lists
        map<int, vector<int> > protocols;
    };

    // Client commands we understand
    map<string, ClientCommand> client_cmd_map;

    // Map of reference numbers to sentences
    map<int, KisNetFramework::server_protocol *> protocol_map;
    // Map of headers to reference numbers
    map<string, int> ref_map;
    // Protocols clients are required to support
    vector<int> required_protocols;
    // Map of protocols to the number of clients using them
    map<int, int> client_mapped_protocols;

    // Client options
    map<int, KisNetFramework::client_opt *> client_optmap;

};

#endif
