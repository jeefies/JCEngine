#ifndef _JCENGINE_UTIL_CPP_
#define _JCENGINE_UTIL_CPP_

#include <fstream>
#include <string>
#include <SDL3/SDL.h>

#include <jc_base.h>

bool JCFileExists(const std::string &path) {
    std::ifstream f(path);
    if (!f.good()) SDL_SetError("File %s Not Found.", path.c_str());
    return f.good();
}

#endif // _JCENGINE_UTIL_CPP_