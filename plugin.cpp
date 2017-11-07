// Copyright 2016 Coppelia Robotics GmbH. All rights reserved.
// marc@coppeliarobotics.com
// www.coppeliarobotics.com
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -------------------------------------------------------------------
// Authors:
// Federico Ferri <federico.ferri.it at gmail dot com>
// -------------------------------------------------------------------

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "v_repPlusPlus/Plugin.h"
#include "plugin.h"
#include "stubs.h"
#include "config.h"
#include <b0/b0.h>

template<typename T>
struct Handle
{
    static std::string str(const T *t)
    {
        static boost::format fmt("%s:%lld:%d");
        return (fmt % tag() % reinterpret_cast<long long int>(t) % crc_ptr(t)).str();
    }

    static T * obj(std::string h)
    {
        boost::cmatch m;
        static boost::regex re("([^:]+):([^:]+):([^:]+)");
        if(boost::regex_match(h.c_str(), m, re) && m[1] == tag())
        {
            T *t = reinterpret_cast<T*>(boost::lexical_cast<long long int>(m[2]));
            int crc = boost::lexical_cast<int>(m[3]);
            if(crc == crc_ptr(t)) return t;
        }
        return nullptr;
    }

private:
    static std::string tag()
    {
        return "unknown";
    }

    static int crc_ptr(const T *t)
    {
        auto x = reinterpret_cast<long long int>(t);
        x = x ^ (x >> 32);
        x = x ^ (x >> 16);
        x = x ^ (x >> 8);
        x = x ^ (x >> 4);
        x = x & 0x000000000000000F;
        x = x ^ 0x0000000000000008;
        return int(x);
    }
};

using NodeHandle = Handle<b0::Node>;
template<> std::string NodeHandle::tag() { return "b0.node"; }

using PubHandle = Handle<b0::Publisher<std::string, true> >;
template<> std::string PubHandle::tag() { return "b0.pub"; }

using SubHandle = Handle<b0::Subscriber<std::string, true> >;
template<> std::string SubHandle::tag() { return "b0.sub"; }

using CliHandle = Handle<b0::ServiceClient<std::string, std::string, true> >;
template<> std::string CliHandle::tag() { return "b0.cli"; }

using SrvHandle = Handle<b0::ServiceServer<std::string, std::string, true> >;
template<> std::string SrvHandle::tag() { return "b0.srv"; }

void topicCallbackWrapper(int scriptID, std::string callback, std::string topic, const std::string &payload)
{
    topicCallback_in in;
    in.topic = topic;
    in.payload = payload;
    topicCallback_out out;
    topicCallback(scriptID, callback.c_str(), &in, &out);
}

void serviceCallbackWrapper(int scriptID, std::string callback, const std::string &req_payload, std::string &rep_payload)
{
    serviceCallback_in in;
    in.payload = req_payload;
    serviceCallback_out out;
    serviceCallback(scriptID, callback.c_str(), &in, &out);
    rep_payload = out.payload;
}

void create(SScriptCallBack *p, const char *cmd, create_in *in, create_out *out)
{
    auto *pnode = new b0::Node(in->name);
    out->handle = NodeHandle::str(pnode);
}

void init(SScriptCallBack *p, const char *cmd, init_in *in, init_out *out)
{
    auto *pnode = NodeHandle::obj(in->handle);
    pnode->init();
    out->name = pnode->getName();
}

void spin(SScriptCallBack *p, const char *cmd, spin_in *in, spin_out *out)
{
    auto *pnode = NodeHandle::obj(in->handle);
    pnode->spin();
}

void spinOnce(SScriptCallBack *p, const char *cmd, spinOnce_in *in, spinOnce_out *out)
{
    auto *pnode = NodeHandle::obj(in->handle);
    pnode->spinOnce();
}

void cleanup(SScriptCallBack *p, const char *cmd, cleanup_in *in, cleanup_out *out)
{
    auto *pnode = NodeHandle::obj(in->handle);
    pnode->cleanup();
}

void destroy(SScriptCallBack *p, const char *cmd, destroy_in *in, destroy_out *out)
{
    auto *pnode = NodeHandle::obj(in->handle);
    delete pnode;
}

void createPublisher(SScriptCallBack *p, const char *cmd, createPublisher_in *in, createPublisher_out *out)
{
    auto *pnode = NodeHandle::obj(in->nodeHandle);
    auto *ppub = new b0::Publisher<std::string, true>(pnode, in->topic);
    out->handle = PubHandle::str(ppub);
}

void publish(SScriptCallBack *p, const char *cmd, publish_in *in, publish_out *out)
{
    auto *ppub = PubHandle::obj(in->handle);
    ppub->publish(in->payload);
}

void destroyPublisher(SScriptCallBack *p, const char *cmd, destroyPublisher_in *in, destroyPublisher_out *out)
{
    auto *ppub = PubHandle::obj(in->handle);
    delete ppub;
}

void createSubscriber(SScriptCallBack *p, const char *cmd, createSubscriber_in *in, createSubscriber_out *out)
{
    auto *pnode = NodeHandle::obj(in->nodeHandle);
    auto callback = boost::bind(topicCallbackWrapper, p->scriptID, in->callback, _1, _2);
    auto *psub = new b0::Subscriber<std::string, true>(pnode, in->topic, callback);
    out->handle = SubHandle::str(psub);
}

void destroySubscriber(SScriptCallBack *p, const char *cmd, destroySubscriber_in *in, destroySubscriber_out *out)
{
    auto *psub = SubHandle::obj(in->handle);
    delete psub;
}

void createServiceClient(SScriptCallBack *p, const char *cmd, createServiceClient_in *in, createServiceClient_out *out)
{
    auto *pnode = NodeHandle::obj(in->nodeHandle);
    auto *pcli = new b0::ServiceClient<std::string, std::string, true>(pnode, in->service);
    out->handle = CliHandle::str(pcli);
}

void call(SScriptCallBack *p, const char *cmd, call_in *in, call_out *out)
{
    auto *pcli = CliHandle::obj(in->handle);
    pcli->call(in->payload, out->payload);
}

void destroyServiceClient(SScriptCallBack *p, const char *cmd, destroyServiceClient_in *in, destroyServiceClient_out *out)
{
    auto *pcli = CliHandle::obj(in->handle);
    delete pcli;
}

void createServiceServer(SScriptCallBack *p, const char *cmd, createServiceServer_in *in, createServiceServer_out *out)
{
    auto *pnode = NodeHandle::obj(in->nodeHandle);
    auto callback = boost::bind(serviceCallbackWrapper, p->scriptID, in->callback, _1, _2);
    auto *psrv = new b0::ServiceServer<std::string, std::string, true>(pnode, in->service, callback);
    out->handle = SrvHandle::str(psrv);
}

void destroyServiceServer(SScriptCallBack *p, const char *cmd, destroyServiceServer_in *in, destroyServiceServer_out *out)
{
    auto *psrv = SrvHandle::obj(in->handle);
    delete psrv;
}

class Plugin : public vrep::Plugin
{
public:
    void onStart()
    {
        if(!registerScriptStuff())
            throw std::runtime_error("failed to register script stuff");
    }
};

VREP_PLUGIN(PLUGIN_NAME, PLUGIN_VERSION, Plugin)
