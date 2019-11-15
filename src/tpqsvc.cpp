#include "ptmp/api.h"
#include "ptmp/actors.h"
#include "ptmp/factory.h"
#include "../include/tpq_server.hpp"

#include <json.hpp>
#include <czmq.h>

using json = nlohmann::json;

class TPQService : public ptmp::TPAgent {
    zactor_t* m_actor;
public:
    TPQService(const std::string& config) {
        auto jcfg = json::parse(config);
        std::string name = "tpqsvc";
        if (jcfg["name"].is_string()) {
            name = jcfg["name"];
        }
        m_actor = zactor_new(tpq_server, (void*)name.c_str());

        zstr_sendx(m_actor, "INGEST", config.c_str()); // {input: ...}
        char* answer = zstr_recv(m_actor);
        assert(streq(answer, "INGEST OK"));
        free (answer);
    
        uint64_t detmask = 0xFFFFFFFFFFFFFFFF;
        if (jcfg["detmask"].is_number()) {
            detmask = jcfg["detmask"];
        }
        zsock_send(m_actor, "s8", "DETMASK", detmask);
        answer = zstr_recv(m_actor);
        assert(streq(answer, "DETMASK OK"));
        free (answer);

        uint64_t qlwm = 5e8;        // 10s of 50 MHz
        uint64_t qhwm = 2*qlwm;
        if (! jcfg["queue"].is_null() ) {
            qlwm = jcfg["queue"]["lwm"];
            qhwm = jcfg["queue"]["hwm"];
        }
        zsock_send(m_actor, "s88", "QUEUE", qlwm, qhwm);
        answer = zstr_recv(m_actor);
        assert(streq(answer, "QUEUE OK"));
        free (answer);
    }

    virtual ~TPQService() {
        zsock_signal(zactor_sock(m_actor), 0); // signal quit
        zclock_sleep(1000);
        zactor_destroy(&m_actor);
    }
};

PTMP_AGENT(TPQService, tpqsvc)
