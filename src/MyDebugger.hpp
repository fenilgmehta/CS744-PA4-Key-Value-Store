#ifndef PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
#define PA_4_KEY_VALUE_STORE_MYDEBUGGER_H

#include <iostream>

// TODO: comment this line when doing performance testing and when using IDE CmakeLists.txt
#define DEBUGGING_ON

#ifdef DEBUGGING_ON

    // REFER: https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
    enum Code {
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_YELLOW   = 33,
        FG_BLUE     = 34,
        FG_DEFAULT  = 39,

        BG_RED      = 41,
        BG_GREEN    = 42,
        BG_YELLOW   = 43,
        BG_BLUE     = 44,
        BG_DEFAULT  = 49
    };

    #define color(enum_code) "\033[" << enum_code << "m"

    /* Red message */
    void log_error(const char* msg){
        std::cerr << color(FG_RED) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
    }

    /* Yellow message */
    void log_warning(const char* msg){
        std::cerr << color(FG_YELLOW) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
    }

    /* Blue message */
    void log_info(const char* msg){
        std::cout << color(FG_BLUE) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
    }

    /* Green message */
    void log_success(const char* msg){
        std::cout << color(FG_GREEN) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
    }

    #undef color

#else
    #define log_NO_ACTION(...)
    #define log_error(...) log_NO_ACTION()
    #define log_warning(...) log_NO_ACTION()
    #define log_info(...) log_NO_ACTION()
    #define log_success(...) log_NO_ACTION()
#endif

#endif // PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
