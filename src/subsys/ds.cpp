#ifndef _JCENGINE_DS_CPP_
#define _JCENGINE_DS_CPP_

#include <jc_ds.h>

int _trie_name_ord(char s) {
    if ('a' <= s && s <= 'z') return s - 'a';
    if ('A' <= s && s <= 'Z') return s - 'A' + 26;
    if ('0' <= s && s <= '9') return s - '0' + 52;
    return s == '.' ? 62 : 63;
}

char _trie_name_chr(int x) {
    if (x < 26) return x + 'a';
    else if (x < 52) return x - 26 + 'A';
    else if (x < 62) return x - 52 + '0';
    return x == 62 ? '.' : '_';
}

int countBits(int64_t x) {
    int cnt = 0;
    while (x) x -= x & -x, ++cnt;
    return cnt;
}


#endif // _JCENGINE_DS_CPP_