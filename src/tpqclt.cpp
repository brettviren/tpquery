// Note, this component is for testing tpqsvc and doesn't have much
// practical use otherwise.

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

    int verbose = 1;            // for now, hard code
    if (jcfg["verbose"].is_number()) {
        verbose = jcfg["verbose"];
    }
    double minbias = jcfg["minbias"];
    std::string server_address = jcfg["service"];

    zsys_debug("tpqclt %s minbias=%f server: %s",
               name.c_str(), minbias, server_address.c_str());

    zsock_signal(pipe,0);       // signal ready

    zsock_t* isock = ptmp::internals::endpoint(jcfg["input"].dump());
    tpq_client_t* client = tpq_client_new();
    tpq_client_say_hello(client, name.c_str(), server_address.c_str());

    int64_t first_seen = 0;
    size_t count = 0;
    zpoller_t* poller = zpoller_new(pipe, isock, NULL);
    bool got_quit = false;
    uint64_t nrecv = 0;
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

            if (! first_seen) {
                first_seen = tpset.tstart();
            }

            // 2. query
            // zsys_info("tpqclt: #%d, detid: %ld query: %ld + %d",
            //           count,
            //           tpset.detid(),
            //           tpset.tstart(),
            //           tpset.tspan());
            int rc = tpq_client_query(client,
                                      tpset.tstart(),
                                      tpset.tspan(),
                                      tpset.detid(),
                                      1000);
            assert (rc >= 0);

            // 3. do something with it
            int seqno = tpq_client_seqno(client);
            int status = tpq_client_status(client);
            msg = tpq_client_payload(client);
            int nframes = zmsg_size(msg);
            ++nrecv;
            zsys_info("tpqclt: count: %d, seqno: %d, nrecv: %d, status: %d, nmsgs: %d, "
                      "detid: 0x%x, "
                      "query: %.3f + %.3f MTicks",
                      count, seqno, nrecv, status, nframes,
                      tpset.detid(),
                      1e-6*(tpset.tstart() - first_seen),
                      1e-6*(tpset.tspan()));

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
