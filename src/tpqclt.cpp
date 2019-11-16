#include "ptmp/api.h"
#include "ptmp/actors.h"
#include "ptmp/factory.h"
#include "../include/tpq_client.hpp"

#include <json.hpp>
#include <czmq.h>

using json = nlohmann::json;

void tpqclt_actor(zsock_t* pipe, void* vargs)
{
    auto jcfg = json::parse((const char*) vargs);
    std::string name = "tpqsvc";
    if (jcfg["name"].is_string()) {
        name = jcfg["name"];
    }

    double minbias = jcfg["minbias"];
    zsock_t* isock = ptmp::internals::endpoint(jcfg["input"].dump());
    
    std::string server_address = jcfg["service"];
    tpq_client_t* client = tpq_client_new();
    tpq_client_say_hello(client, name.c_str(), server_address.c_str());

    size_t count = 0;
    zpoller_t* poller = zpoller_new(pipe, isock, NULL);
    bool got_quit = false;
    while (!zsys_interrupted) {
        void * which = zpoller_wait(poller, -1);

        if (which == pipe) {
            got_quit = true;
            break;
        }
        if (which == isock) {
            zmsg_t* msg = zmsg_recv(isock);
            ++count;
            if (count * minbias < 1) {
                zmsg_destroy(&msg);
                continue;
            }
            // 1. parse to get t
            ptmp::data::TPSet tpset;
            ptmp::internals::recv(&msg, tpset);
            // 2. query
            int rc = tpq_client_query(client,
                                      tpset.tstart(),
                                      tpset.tspan(),
                                      tpset.detid(),
                                      1000);
            // 3. do something with it
            int seqno = tpq_client_seqno(client);
            int status = tpq_client_status(client);
            msg = tpq_client_payload(client);
            int nframes = zmsg_size(msg);
            zsys_info("%d %d %d %d", count, seqno, status, nframes);

            count = 0;
            continue;
        }
        if (zpoller_terminated(poller)) {
            break;
        }
    }

    tpq_client_destroy(&client);
    zpoller_destroy(&poller);
    zsock_destroy(&isock);
    if (got_quit) {
        return;
    }
    zsock_wait(pipe);
}



class TPQClient : public ptmp::TPAgent {
    zactor_t* m_actor;
public:
    TPQClient(const std::string& config)
        : m_actor(zactor_new(tpqclt_actor, (void*)config.c_str()))
        {

    }
    virtual ~TPQClient() {}

    
};


// A silly component which eats up TPSets for a while then picks one
// to use as fodder for querying a TP Query Service.  Mostly for
// testing.
PTMP_AGENT(TPQClient, tpqclt)
