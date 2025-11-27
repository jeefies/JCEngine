#ifndef _JCENGINE_DS_H_
#define _JCENGINE_DS_H_

#include <cstdint>
#include <vector>

#include <jc_base.h>

int _trie_name_ord(char s);
char _trie_name_chr(int x);
int countBits(int64_t x);

template<typename T>
struct JCTrieNode {
    int64_t bitmask;
    T x;
    std::vector<JCTrieNode *> childs;
    int hasChild(int x);
    JCTrieNode* nextChild(int x);

    _DELETE_COPY_MOVE_(JCTrieNode)

    JCTrieNode() : bitmask(0) {}

    ~JCTrieNode() {
        for (auto p : childs)
            del_node(p);
    }

    static JCTrieNode<T>* new_node() {
        return new JCTrieNode<T>();
    }

    static int del_node(JCTrieNode<T>* ptr) {
        delete ptr;
        return JC_SUCCESS;
    }
};

template<typename T>
struct JCTrie {
    JCTrieNode<T> *root;
    T* get(const std::string &S);
    T* create(const std::string &S);
    int set(const std::string &S, T x);

    JCTrie() {
        root = JCTrieNode<T>::new_node();
    }

    ~JCTrie() {
        JCTrieNode<T>::del_node(root);
    }
};


template<typename T>
int JCTrieNode<T>::hasChild(int x) {
    return (bitmask >> x) & 1;
}

template<typename T>
JCTrieNode<T> * JCTrieNode<T>::nextChild(int x) {
    int cnt = countBits(bitmask & ((1ll << x) - 1));
    if (bitmask & (1ll << x))
        return childs[cnt];
    
    bitmask ^= (1ll << x);
    return *childs.insert(childs.begin() + cnt, JCTrieNode<T>::new_node());
}

template<typename T>
T* JCTrie<T>::get(const std::string &S) {
    JCTrieNode<T> *ptr = root;
    for (char c : S) {
        int x = _trie_name_ord(c);
        if (!ptr->hasChild(x)) return nullptr;
        ptr = ptr->nextChild(x);
    } return &ptr->x;
}

template<typename T>
T* JCTrie<T>::create(const std::string &S) {
    JCTrieNode<T> *ptr = root;
    for (char c : S) {
        int x = _trie_name_ord(c);
        ptr = ptr->nextChild(x);
    } return &ptr->x;
}

template<typename T>
int JCTrie<T>::set(const std::string &S, T x) {
    *create(S) = x;
    return JC_SUCCESS;
}

#endif // _JCENGINE_DS_H_