#ifndef PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
#define PA_4_KEY_VALUE_STORE_MYDEBUGGER_H

#include <iostream>


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


// TODO: comment this line when doing performance testing and when using IDE CmakeLists.txt
#define DEBUGGING_ON

#ifdef DEBUGGING_ON

    /* Yellow message */
    template <typename T>
    void log_warning(const T& msg, bool prependNewLine = false){
        if(prependNewLine) std::cerr << "\n";
        std::cerr << color(FG_YELLOW) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
        std::cerr.flush();
    }

    /* Blue message */
    template <typename T>
    void log_info(const T& msg, bool prependNewLine = false){
        if(prependNewLine) std::cout << "\n";
        std::cout << color(FG_BLUE) << "INFO: " << color(FG_DEFAULT) << msg << std::endl;
        std::cout.flush();
    }

    /* Green message */
    template <typename T>
    void log_success(const T& msg, bool prependNewLine = false){
        if(prependNewLine) std::cout << "\n";
        std::cout << color(FG_GREEN) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
        std::cout.flush();
    }

#else
    #define log_warning(...) ;
    #define log_info(...) ;
    #define log_success(...) ;
#endif


/* Red message */
template <typename T>
void log_error(const T& msg, bool prependNewLine = false, bool appendExtraNewLine = false){
    if(prependNewLine) std::cerr << "\n";
    std::cerr << color(FG_RED) << "ERROR: " << color(FG_DEFAULT) << msg << std::endl;
    if(appendExtraNewLine) std::cerr << "\n";
    std::cerr.flush();
}

#undef color

#endif // PA_4_KEY_VALUE_STORE_MYDEBUGGER_H
