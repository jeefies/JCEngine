#ifndef _JCENGINE_SUBSYS_EVENT_CPP_
#define _JCENGINE_SUBSYS_EVENT_CPP_

#include "jc_event.h"


int JCEventTrieNode::emit(void *userdata) {
    for (const auto & cmd : cmds) {
        int result = cmd(userdata);
        if (result == JC_CONTINUE) continue;
        return result == JC_ERROR;
    }
    return JC_SUCCESS;
}
    
JCEventCenter::JCEventCenter() {
}
    
JCEventCenter::~JCEventCenter() {
} 

int JCEventCenter::registerEvent(const std::string &S, const cmd_type& cmd, int place) {
    checkNameValid(S);
    JCEventTrieNode* node = trie.create(S);
    node->cmds.push_back(cmd);
    return JC_SUCCESS; // useless ? always success !
}

int JCEventCenter::emitEvent(const std::string &S, void *userdata) {
    checkNameValid(S);
    JCEventTrieNode* node = trie.get(S);
    if (node == nullptr) return JC_ERROR;
    return node->emit(userdata);
}

inline void JCEventCenter::checkNameValid(const std::string &S) {
    for (char s : S) {
        if (!('a' <= s && s <= 'z') && !('A' <= s && s <= 'Z') && !('0' <= s && s <= '9')
            && s != '_' && s != '.')
            throw std::string("Invalid Event Name, can only contain [A-Z][a-z][0-9]._");
    }
}


JCEventTrieNode* JCAllocateEventTrieNode() {
    return new JCEventTrieNode();
}

void JCDeallocateEventTrieNode(JCEventTrieNode *node) {
    delete node;
}

#endif // _JCENGINE_SUBSYS_EVENT_CPP_