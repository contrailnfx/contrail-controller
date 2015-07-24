/*
 * Copyright (c) 2015 Juniper Networks, Inc. All rights reserved.
 */

#ifndef __IFMAP_STRESS_TEST_H__
#define __IFMAP_STRESS_TEST_H__

#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/function.hpp>
#include <boost/program_options.hpp>
#include <list>

#include "base/logging.h"
#include "base/test/task_test_util.h"
#include "control-node/control_node.h"
#include "db/db.h"
#include "db/db_graph.h"
#include "io/event_manager.h"
#include "ifmap/ifmap_client.h"
#include "ifmap/ifmap_link_table.h"
#include "ifmap/ifmap_server.h"
#include "ifmap/ifmap_table.h"
#include "ifmap/ifmap_util.h"

#include "testing/gunit.h"

class IFMapChannelManager;
class IFMapXmppClientMock;
class IFMapServerParser;
class ServerThread;
class XmppServer;

class IFMapSTOptions {
public:
    static const int kDEFAULT_NUM_EVENTS;
    static const int kDEFAULT_NUM_XMPP_CLIENTS;
    static const int kDEFAULT_PER_CLIENT_NUM_VMS;
    static const std::string kDEFAULT_EVENT_WEIGHTS_FILE;

    IFMapSTOptions();
    void Initialize();

    int num_events() const;
    int num_xmpp_clients() const;
    int num_vms() const;
    const std::string &events_file() const;
    const std::string &event_weight_file() const;

private:
    int num_events_;
    int num_xmpp_clients_;
    int num_vms_;
    std::string event_weight_file_;
    std::string events_file_;
    boost::program_options::options_description desc_;
};

class IFMapSTEventMgr {
public:
    enum EventType {
        VR_NODE_ADD,
        VR_NODE_DELETE,
        VR_SUB,
        CLIENT_READY_VR_ADD,
        VM_NODE_ADD,
        VM_NODE_DELETE,
        VM_SUB,
        VM_UNSUB,
        VM_ADD_VM_SUB,
        VM_SUB_VM_ADD,
        VM_DEL_VM_UNSUB,
        VM_UNSUB_VM_DEL,
        XMPP_READY,
        XMPP_NOTREADY,
        IROND_CONN_DOWN,
        NUM_EVENT_TYPES,         // 15
    };
    typedef std::map<EventType, std::string> EventTypeMap;
    typedef std::map<std::string, EventType> EventStringMap;
    typedef std::vector<std::string> EventLog;
    typedef EventLog::size_type EvLogSz_t;

    IFMapSTEventMgr();
    void Initialize(const IFMapSTOptions &config_options);
    static EventTypeMap InitEventTypeMap();
    static EventStringMap InitEventStringMap();
    bool events_from_file() const;
    std::string EventToString(EventType event) const;
    EventType StringToEvent(std::string event) const;
    EventType GetNextEvent();
    bool EventAvailable() const;
    EvLogSz_t GetEventLogSize() const { return event_log_.size(); }
    int max_events() { return max_events_; }

private:
    void ReadEventsFile(const IFMapSTOptions &config_options);
    void ReadEventWeightsFile(const std::string &filename);

    static const EventTypeMap event_type_map_;
    static const EventStringMap event_string_map_;

    std::list<std::string> file_events_;
    bool events_from_file_;
    int max_events_;
    int events_processed_;
    std::vector<int> event_weights_;
    int event_weights_sum_;
    EventLog event_log_;
};

struct IFMapSTClientCounters {
    IFMapSTClientCounters() :
        vr_node_adds(0), vr_node_deletes(0), vr_subscribes(0), vm_node_adds(0),
        vm_node_deletes(0), vm_subscribes(0), vm_unsubscribes(0), xmpp_connects(0),
        xmpp_disconnects(0) {
    }
    void incr_vr_node_adds() { ++vr_node_adds; }
    void incr_vr_node_deletes() { ++vr_node_deletes; }
    void incr_vr_subscribes() { ++vr_subscribes; }
    void incr_vm_node_adds() { ++vm_node_adds; }
    void incr_vm_node_deletes() { ++vm_node_deletes; }
    void incr_vm_subscribes() { ++vm_subscribes; }
    void incr_vm_unsubscribes() { ++vm_unsubscribes; }
    void incr_xmpp_connects() { ++xmpp_connects; }
    void incr_xmpp_disconnects() { ++xmpp_disconnects; }
    uint32_t vr_node_adds;
    uint32_t vr_node_deletes;
    uint32_t vr_subscribes;
    uint32_t vm_node_adds;
    uint32_t vm_node_deletes;
    uint32_t vm_subscribes;
    uint32_t vm_unsubscribes;
    uint32_t xmpp_connects;
    uint32_t xmpp_disconnects;
};

//class IFMapStressTest : public ::testing::TestWithParam<TestParams> {
class IFMapStressTest : public ::testing::Test {
protected:
    static const std::string kXMPP_CLIENT_PREFIX;
    static const uint64_t kUUID_MSLONG = 1361480977053469917;
    static const uint64_t kUUID_LSLONG = 1324108391580000000;
    static const std::string kDefaultXmppServerName;
    static const int kMAX_LOG_NUM_EVENTS = 100000; // events in circular buffer

    typedef IFMapSTEventMgr::EventType EventType;
    typedef boost::function<void(void)> EvCb;
    typedef std::map<EventType, EvCb> EvCbMap;
    typedef std::set<std::string> VrNameSet;
    typedef std::set<std::string> VmNameSet;
    typedef std::set<int> IntSet;
    typedef IntSet ClientIdSet;
    typedef IntSet VmIdSet;
    typedef std::vector<VmIdSet> PerClientVmIds;
    typedef std::vector<IFMapSTClientCounters> ClientCounters;

    IFMapStressTest();
    void SetUp();
    void TearDown();
    std::string XmppClientNameCreate(int id);
    void CreateXmppClients();
    void DeleteXmppClients();
    void XmppClientInits();
    std::string VirtualRouterNameCreate(int id);
    void VirtualRouterNodeAdd();
    void VirtualRouterNodeDelete();
    void VirtualRouterSubscribe();
    std::string VirtualMachineNameCreate(int client_id, int vm_id);
    void VirtualMachineNodeAdd();
    void VirtualMachineNodeDelete();
    void VirtualMachineSubscribe();
    void VirtualMachineUnsubscribe();
    int GetXmppDisconnectedClientId();
    int GetXmppConnectedClientId();
    void XmppConnect();
    void XmppDisconnect();
    void SetupEventCallbacks();
    EvCb GetCallback(EventType event);
    std::string EventToString(EventType event) const;
    void VerifyNodes();
    bool ConnectedToXmppServer(const std::string &client_name);
    int PickRandomId(const IntSet &client_set);
    int GetVmIdToAddNode(int client_id);
    int GetVmIdToDeleteNode(int client_id);
    int GetVmIdToSubscribe(int client_id);
    int GetVmIdToUnsubscribe(int client_id);
    void Log(std::string log_string);
    void PrintTestInfo();

    DB db_;
    DBGraph db_graph_;
    EventManager evm_;
    IFMapServer ifmap_server_;
    IFMapServerParser *parser_;
    std::auto_ptr<ServerThread> thread_;
    XmppServer *xmpp_server_;
    std::auto_ptr<IFMapChannelManager> ifmap_channel_mgr_;
    IFMapSTOptions config_options_;
    IFMapSTEventMgr event_generator_;
    std::vector<IFMapXmppClientMock *> xmpp_clients_;
    VrNameSet vr_nodes_created_; // VR names that have a config node
    VmNameSet vm_nodes_created_; // VM names that have a config node
    EvCbMap callbacks_;
    ClientIdSet xmpp_connected_; // xmpp connected client ids
    ClientIdSet xmpp_disconnected_; // xmpp disconnected client ids
    ClientIdSet vr_subscribed_; // VR subscribed client ids
    PerClientVmIds vm_to_add_ids_; // VM ids that dont have a config node
    PerClientVmIds vm_to_delete_ids_; // VM ids that have a config node
    PerClientVmIds vm_sub_pending_ids_; // VM ids that have not subscribed
    PerClientVmIds vm_unsub_pending_ids_; // VM ids that have subscribed
    boost::circular_buffer<std::string> log_buffer_;
    ClientCounters client_counters_;
};

#endif // #define __IFMAP_STRESS_TEST_H__
