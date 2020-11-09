#ifndef PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
#define PA_4_KEY_VALUE_STORE_MYDEBUGGER_H

#include <iostream>
#include <thread>

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

// NOTE: TID is Thread ID
// REFER: https://www.geeksforgeeks.org/thread-get_id-function-in-c/

// TODO: comment this line when doing performance testing and when using IDE CmakeLists.txt
#define DEBUGGING_ON

#ifdef DEBUGGING_ON

// REFER: https://stackoverflow.com/questions/7432100/how-to-get-integer-thread-id-in-c11
#define GET_TID() (std::hash<std::thread::id>{}(std::this_thread::get_id()) % 10000000)
// (std::this_thread::get_id())

    /* Yellow message */
    template <typename T>
    void log_warning(const T& msg, bool prependNewLine = false, bool appendExtraNewLine = false) {
        if(prependNewLine) std::cerr << '\n';
        std::cerr << color(FG_YELLOW) << "WARNING [TID=" << GET_TID() << "] : " << color(FG_DEFAULT) << msg << '\n';
        if(appendExtraNewLine) std::cerr << '\n';
        std::cerr.flush();
    }

    /* Blue message */
    template <typename T>
    void log_info(const T& msg, bool prependNewLine = false, bool appendExtraNewLine = false) {
        if(prependNewLine) std::cout << '\n';
        std::cout << color(FG_BLUE) << "INFO [TID=" << GET_TID() << "] : " << color(FG_DEFAULT) << msg << '\n';
        if(appendExtraNewLine) std::cout << '\n';
        std::cout.flush();
    }

    /* Green message */
    template <typename T>
    void log_success(const T& msg, bool prependNewLine = false, bool appendExtraNewLine = false) {
        if(prependNewLine) std::cout << '\n';
        std::cout << color(FG_GREEN) << "SUCCESS [TID=" << GET_TID() << "] : " << color(FG_DEFAULT) << msg << '\n';
        if(appendExtraNewLine) std::cout << '\n';
        std::cout.flush();
    }

#undef GET_TID

#else

    template<typename T>
    void log_warning(const T &msg, bool prependNewLine = false, bool appendExtraNewLine = false) {}

    template<typename T>
    void log_info(const T &msg, bool prependNewLine = false, bool appendExtraNewLine = false) {}

    template<typename T>
    void log_success(const T &msg, bool prependNewLine = false, bool appendExtraNewLine = false) {}

    // #define log_warning(...) ;
    // #define log_info(...) ;
    // #define log_success(...) ;
#endif


/* Red message */
template <typename T>
void log_error(const T& msg, bool prependNewLine = false, bool appendExtraNewLine = false){
    if(prependNewLine) std::cerr << '\n';
    std::cerr << color(FG_RED) << "ERROR [TID=" << std::this_thread::get_id() << "] : " << color(FG_DEFAULT) << msg << std::endl;
    if(appendExtraNewLine) std::cerr << '\n';
    std::cerr.flush();
}

#undef color

#endif // PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
