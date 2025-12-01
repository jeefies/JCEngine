#ifndef _JCENGINE_SUBSYS_EVENT_H_
#define _JCENGINE_SUBSYS_EVENT_H_

#include <functional>
#include <vector>
#include <cstdint>
#include <string>
#include <chrono>
#include <queue>
#include <thread>
#include <mutex>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <numeric>

#include <jc_base.h>
#include <jc_ds.h>

using namespace std::chrono_literals;

using cmd_type = std::function<int(void *)>;
using ms_timepoint = std::chrono::time_point<std::chrono::steady_clock>;

int _event_name_ord(char s);

char _event_name_chr(int x);

int countBits(int64_t x);

struct JCEventTrieNode {
    std::vector<cmd_type> cmds;
    int emit(void *userdata);
};

struct JCEventCenter {
    JCTrie<JCEventTrieNode> trie;
    JCEventCenter();
    ~JCEventCenter();
    int registerEvent(const std::string &S, const cmd_type& cmd, int place = -1);
    int emitEvent(const std::string &S, void *userdata = nullptr);
    inline void checkNameValid(const std::string &S);
};

struct JCEventTimerNode {
    ms_timepoint expire;
    int interval;
    cmd_type callback;
    void * userdata;
};


template<int buffer_size>
struct JCEventTimer {
    int _tick_ms;
    ms_timepoint _now_tick;
    
    enum EventStatus {
        EMPTY,
        WAITING,
        CANCELED,
    };
    
    int _node_idx;
    std::array<int, buffer_size> unused_id;
    std::array<EventStatus, buffer_size> status;
    std::array<JCEventTimerNode, buffer_size> _node_pool;

    struct JCEventTimerNodeCmp {
        const std::array<JCEventTimerNode, buffer_size>* pool;
        JCEventTimerNodeCmp(const std::array<JCEventTimerNode, buffer_size>* p) : pool(p) {}
        bool operator()(int i, int j) const noexcept {
            return (*pool)[i].expire > (*pool)[j].expire;
        }
    };

    int _slot_index;
    std::priority_queue<int, std::vector<int>, JCEventTimerNodeCmp> _waits;
    std::array<std::vector<int>, buffer_size> _slots;

    using mutex_guard = std::lock_guard<std::mutex>;
    int running_;
    std::mutex mtx;
    std::thread task_thread;

    using error_cmd = std::function<int(int, int)>;
    error_cmd error_callback = nullptr;

    JCEventTimer(int tick_ms = 0);
    ~JCEventTimer();

    void basic_init();
    int setTickMS(int _tick_ms);
    int registerEvent(int timeout, int interval, cmd_type callback, void * userdata = nullptr);
    int cancelEvent(int ev_id);
    int getEventStatus(int ev_id);
    int _put_in(int ev_id);
    int _new_node();
    int _del_node(int ev_id);
    void _stop();
    void _start();
    void _tick();
    JCEventTimerNode* getJCEventTimerNode(int ev_id);
};

JCEventTrieNode* JCAllocateEventTrieNode();
void JCDeallocateEventTrieNode(JCEventTrieNode *node);

template<int buffer_size>
void JCEventTimer<buffer_size>::basic_init() {
    _node_idx = buffer_size;
    _slot_index = 0;
    std::iota(begin(unused_id), end(unused_id), 0);
    std::fill(begin(status), end(status), EMPTY);
    _now_tick = std::chrono::steady_clock::now();
    running_ = false;
}

template<int buffer_size>
JCEventTimer<buffer_size>::JCEventTimer(int tick_ms) : _waits(&_node_pool) {
    basic_init();
    if (tick_ms == 0) return ;
    _tick_ms = tick_ms;
    _start();
}

template<int buffer_size>
int JCEventTimer<buffer_size>::setTickMS(int tick_ms) {
    _stop();
    _tick_ms = tick_ms;
    _start();
    return JC_SUCCESS;
}

template<int buffer_size>
JCEventTimer<buffer_size>::~JCEventTimer() {
    _stop();
}

template<int buffer_size>
void JCEventTimer<buffer_size>::_stop() {
    if (!running_) return ;
    running_ = false;
    assert(task_thread.joinable());
    task_thread.join();
    while (!_waits.empty()) _waits.pop();
    for (int i = 0; i < buffer_size; ++i)
        _slots[i].clear(), status[i] = EMPTY;
}

template<int buffer_size>
void JCEventTimer<buffer_size>::_start() {
    running_ = true;
    task_thread = std::thread([this]() {
        #ifdef DEBUG
        jclog << "Task Thread Started...\n";
        #endif
        const auto tick_duration = std::chrono::milliseconds(_tick_ms);
        this->running_ = true;
        while (this->running_) {
            #ifdef DEBUG 
                jclog << "Task Thread Calling Tick...\n";
            #endif
            _tick();
            std::this_thread::sleep_until(_now_tick);
        }
    });
}

template<int buffer_size>
void JCEventTimer<buffer_size>::_tick() {
    #ifdef DEBUG
    jclog << "_tick Aquiring lock\n";
    #endif
    mutex_guard lock(mtx);
    
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  _now_tick.time_since_epoch());
    #ifdef DEBUG
    jclog << "Ticking at " << ms.count()
        << " with _slot_index "
        << _slot_index << "\n";
    #endif

    std::vector<int> retimer;
    for (int ev_id : _slots[_slot_index]) {
        if (status[ev_id] == CANCELED) {
            _del_node(ev_id);
            continue;
        }
        
        if (status[ev_id] == EMPTY) continue;

        auto task = &_node_pool[ev_id];
        int ret_code = task->callback(task->userdata);
        if (ret_code != JC_SUCCESS && error_callback != nullptr) {
            ret_code = error_callback(ev_id, ret_code);
            if (ret_code == JC_TERMINATE) {
                _del_node(ev_id);
                continue;
            }
        }

        if (task->interval != 0) {
            task->expire += std::chrono::milliseconds(task->interval);
            retimer.push_back(ev_id);
        } else _del_node(ev_id);
    }

    // Put far away events in _waits in _slots.
    _slots[_slot_index].clear();
    const auto tick_duration = std::chrono::milliseconds(_tick_ms);
    ms_timepoint _slots_end_tick = _now_tick + buffer_size * tick_duration;
    while (!_waits.empty() && _node_pool[_waits.top()].expire < _slots_end_tick) {
        if (status[_waits.top()] == CANCELED) _del_node(_waits.top());
        else retimer.push_back(_waits.top());
        _waits.pop();
    }

    _slot_index += 1;
    if (_slot_index == buffer_size) _slot_index = 0;
    _now_tick += tick_duration;
    
    for (int ev_id : retimer) _put_in(ev_id);
}

template<int buffer_size>
int JCEventTimer<buffer_size>::_del_node(int ev_id) {
    unused_id[_node_idx++] = ev_id;
    status[ev_id] = EMPTY;
    return JC_SUCCESS;
}

template<int buffer_size>
int JCEventTimer<buffer_size>::_new_node() {
    if (_node_idx == 0)
        return -1;
    int ev_id = unused_id[--_node_idx];
    return ev_id;
}

template<int buffer_size>
int JCEventTimer<buffer_size>::cancelEvent(int ev_id) {
    mutex_guard lock(mtx);
    if (status[ev_id] == WAITING) status[ev_id] = CANCELED;
    return status[ev_id] == CANCELED;
}

template<int buffer_size>
int JCEventTimer<buffer_size>::getEventStatus(int ev_id) {
    mutex_guard lock(mtx);
    return status[ev_id];
}

template<int buffer_size>
int JCEventTimer<buffer_size>::_put_in(int ev_id) {
    assert(status[ev_id] == WAITING);
    #ifdef DEBUG
    jclog << "put in " << ev_id << " with expire " <<
        std::chrono::duration_cast<std::chrono::milliseconds>(_node_pool[ev_id].expire.time_since_epoch()).count()
        << " and interval " << _node_pool[ev_id].interval << "ms\n";
    #endif

    if (_tick_ms == 0) {
        _waits.push(ev_id);
        return JC_SUCCESS;
    }
    
    const auto tick_duration = std::chrono::milliseconds(_tick_ms);
    ms_timepoint _slots_end_tick = _now_tick + (buffer_size - 1) * tick_duration;

    auto expire = _node_pool[ev_id].expire;
    if (expire < _now_tick) {
        _slots[_slot_index].push_back(ev_id);
        #ifdef DEBUG
        jclog << "put " << ev_id << " in " << _slot_index << '\n';
        #endif
        return JC_SUCCESS;
    }
    
    int t = (expire - _now_tick - std::chrono::milliseconds(1)) / tick_duration + 1;
    if (t >= buffer_size) _waits.push(ev_id);
    else _slots[(_slot_index + t) % buffer_size].push_back(ev_id);

    #ifdef DEBUG
    if (t >= buffer_size) jclog << "put " << ev_id << " in _waits\n";
    jclog << "put " << ev_id << " in " << (_slot_index + t) % buffer_size << '\n';
    #endif

    return JC_SUCCESS;
}

template<int buffer_size>
int JCEventTimer<buffer_size>::registerEvent(int timeout, int interval, cmd_type callback, void * userdata) {
    mutex_guard lock(mtx);
    int ev_id = _new_node();
    if (ev_id == -1) return -1;

    assert(status[ev_id] == EMPTY);
    status[ev_id] = WAITING;
    _node_pool[ev_id] = {
        _now_tick + std::chrono::milliseconds(timeout),
        interval,
        callback,
        userdata,
    };

    #ifdef DEBUG
    jclog << "new event expire at "
        << std::chrono::duration_cast<std::chrono::milliseconds>(
            _node_pool[ev_id].expire.time_since_epoch()).count()
        << "\n";
    #endif

    assert(_put_in(ev_id) == JC_SUCCESS);
    return ev_id;
}


#endif // _JCENGINE_SUBSYS_EVENT_H_
