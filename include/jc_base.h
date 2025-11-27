#ifndef _JCENGINE_BASE_H_
#define _JCENGINE_BASE_H_

#include <iostream>
#include <fstream>

#define JC_SUCCESS 0
#define JC_ERROR 1
#define JC_CONTINUE 2
#define JC_TERMINATE 3

// inline std::ofstream jclog = std::ofstream("jc.log");
#define jclog std::cerr

#define _DELETE_COPY_MOVE_(CLASS) CLASS(const CLASS& other) = delete; \
    CLASS(CLASS&& other) = delete; \
    CLASS& operator=(const CLASS& other) = delete; \
    CLASS& operator=(CLASS&& other) = delete; 

bool JCFileExists(const std::string &path);

#endif // _JCENGINE_BASE_H_