#include "bind.hpp"

#include <algorithm>
#include <array>
#include <deque>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <windows.h>

#include "binded_func.hpp"
#include "disable_gcc_warning.hpp"
#include <nlohmann/json.hpp>
#include "enable_gcc_warning.hpp"

#include "i_params.hpp"
#include "path.hpp"
#include "err_logger.hpp"

#include "key/key_absorber.hpp"
#include "key/key_log.hpp"
#include "key/keycode_def.hpp"
#include "key/keycodecvt.hpp"
#include "key/keycode_logger.hpp"
#include "opt/virtual_cmd_line.hpp"
#include "util/def.hpp"
#include "util/string.hpp"

#include "bindings_lists.hpp"
#include "bindings_matcher.hpp"
#include "mode.hpp"

// to use std::numeric_limits<T>::max()
#undef max

//internal linkage
namespace
{
    using namespace vind ;

    std::vector<BindedFunc::shp_t> g_func_list{} ;
    std::unordered_set<unsigned char> g_unbinded_syskeys{} ;

    using mode::Mode ;
    const std::unordered_map<Mode, const char*> g_modeidxs {
        {Mode::Normal,          "guin"},
        {Mode::Insert,          "guii"},
        {Mode::Visual,          "guiv"},
        {Mode::Command,         "cmd"},
        {Mode::EdiNormal,       "edin"},
        {Mode::EdiInsert,       "edii"},
        {Mode::EdiVisual,       "ediv"},
        {Mode::EdiLineVisual,   "edivl"},
        {Mode::MyConfigWindowNormal,  "mycwn"},
        {Mode::MyConfigWindowInsert,  "mycwi"}
    } ;

    inline char get_specode(std::string k) noexcept {
        if(k == "space")   return ' ' ;
        if(k == "hbar")    return '-' ;
        if(k == "gt")      return '>' ;
        if(k == "lt")      return '<' ;
        return static_cast<char>(0) ;
    }

    auto parse_string_command(std::string cmdstr) {
        BindingsMatcher::cmd_t cmd ;

        for(std::size_t i = 0 ; i < cmdstr.length() ; i ++) {
            const auto onechar = cmdstr[i] ;
            if(onechar != '<') {
                //ascii
                if(auto vkc = keycodecvt::get_vkc(onechar)) { //ex) a
                    cmd.emplace_back(1, vkc) ;
                    continue ;
                }

                //shifted ascii
                if(auto vkc = keycodecvt::get_shifted_vkc(onechar)) { //ex) A (A is divided to a and SHIFT)
                    cmd.push_back(BindingsMatcher::keyset_t{vkc, KEYCODE_SHIFT}) ;
                    continue ;
                }

                PRINT_ERROR(onechar + std::string(" of ") + cmdstr + "\tis invalid ascii key code") ;
                continue ;
            }

            auto pairpos = cmdstr.find('>', i + 1) ;
            if(pairpos == std::string::npos) {
                throw std::runtime_error("command is bad syntax. " + cmdstr +  " does not have a greater-than sign (>)") ;
            }

            BindingsMatcher::keyset_t keyset{} ;
            const auto keystrset = util::split(cmdstr.substr(i + 1, pairpos - i - 1), "-") ;
            for(auto code = keystrset.begin() ; code != keystrset.end() ; code ++) {
                if(code != keystrset.begin() && code->length() == 1) { //ascii code
                    //ascii
                    if(auto vkc = keycodecvt::get_vkc(code->front())) {
                        keyset.push_back(vkc) ;
                        continue ;
                    }

                    //shifted ascii
                    if(auto vkc = keycodecvt::get_shifted_vkc(code->front())) {
                        keyset.push_back(vkc) ;
                        keyset.push_back(KEYCODE_SHIFT) ;
                        continue ;
                    }
                }

                auto lowercode = util::A2a(*code) ;

                //if the cmd is same as some mode's key (e.g. <guin>, <edin>),
                //its pointer use same pointer to target mode.
                for(const auto& target_index : g_modeidxs) {
                    if(lowercode == target_index.second) {
                        throw target_index.first ;
                    }
                }

                if(lowercode == "any") {
                    keyset.push_back(KEYCODE_OPTIONAL) ;
                    continue ;
                }
                if(lowercode == "num") {
                    keyset.push_back(KEYCODE_OPTNUMBER) ;
                    continue ;
                }

                if(auto ascii = get_specode(lowercode)) {
                    if(auto vkc = keycodecvt::get_vkc(ascii)) {
                        keyset.push_back(vkc) ;
                        continue ;
                    }
                    if(auto vkc = keycodecvt::get_shifted_vkc(ascii)) {
                        keyset.push_back(vkc) ;
                        keyset.push_back(KEYCODE_SHIFT) ;
                        continue ;
                    }

                    PRINT_ERROR(*code  + " is not supported. (" + path::BINDINGS() + ")") ;
                    continue ;
                }

                if(const auto vkc = keycodecvt::get_sys_vkc(lowercode)) {
                    keyset.push_back(vkc) ;

                    //If a system key is bindied as a single command.
                    if(keystrset.size() == 1) {
                        g_unbinded_syskeys.erase(vkc) ;
                    }
                    continue ;
                }

                PRINT_ERROR(*code + "\t of " + cmdstr + " is invalid system key code") ;
            }

            cmd.push_back(std::move(keyset)) ;

            i = pairpos ;
            continue ;
        }
        return cmd ;
    }
}

namespace vind
{
    namespace keybind {
        void init() {
            g_func_list.clear() ;
            g_func_list = BindingsLists::get() ;

            g_unbinded_syskeys.clear() ;
            g_unbinded_syskeys = keycodecvt::get_all_sys_vkc() ;

            easyclick::initialize() ;
        }

        void load_config() {
            std::ifstream ifs(path::to_u8path(path::BINDINGS())) ;
            nlohmann::json jp ;
            ifs >> jp ;
            if(jp.empty()) {
                throw std::runtime_error(path::BINDINGS() + " is empty.") ;
            }

            if(!jp.is_array()) {
                throw std::runtime_error("The root element of " + path::BINDINGS() + " should be array.") ;
            }

            constexpr auto mode_num = static_cast<unsigned char>(Mode::NUM) ;

            std::array<BindingsMatcher::shp_t, mode_num> matcher_list ;

            //if JSON's data is "edin": ["<guin>"], index_links[edin-index] = guin-index
            std::array<unsigned char, mode_num> index_links ;

            if(g_func_list.empty()) {
                throw std::logic_error("keybind has no defined BindFunc.") ;
            }

            //initialize the ignoring key list
            g_unbinded_syskeys = keycodecvt::get_all_sys_vkc() ;

            //create name lists of BindidFunc
            std::unordered_map<std::string, BindedFunc::shp_t> funclist ;
            for(auto& func : g_func_list) {
                funclist[func->name()] = func ;
            }

            for(auto& obj : jp) {
                try {
                    auto& func = funclist.at(obj.at("name")) ;
                    if(!obj.is_object()) {
                        PRINT_ERROR("The child of root-array should be object. (" \
                                + path::BINDINGS() + ", name: " + obj["name"].get<std::string>() + ").") ;
                    }

                    matcher_list.fill(nullptr) ;
                    index_links.fill(static_cast<unsigned char>(Mode::None)) ;

                    for(const auto& index : g_modeidxs) {
                        try {
                            const auto& cmds = obj.at(index.second) ;
                            if(!cmds.is_array()) {
                                PRINT_ERROR("The command lists should be array (" \
                                        + func->name() + "/" + index.second + ").") ;
                                continue ;
                            }
                            if(cmds.empty()) {
                                continue ;
                            }
                            BindingsMatcher::cmdlist_t cmdlist ;

                            for(std::string cmdstr : cmds) {
                                if(cmdstr.empty()) continue ;
                                BindingsMatcher::cmd_t cmd ;
                                try {
                                    cmd = parse_string_command(cmdstr) ;
                                }
                                catch(const std::runtime_error& e) {
                                    PRINT_ERROR(func->name() + "::" + index.second \
                                            + " in " + path::BINDINGS() + " " + e.what()) ;
                                    continue ;
                                }
                                catch(const Mode m) {
                                    index_links[static_cast<unsigned char>(index.first)] \
                                        = static_cast<unsigned char>(m) ;
                                    cmdlist.clear() ;
                                    break ;
                                }
                                cmdlist.push_back(cmd) ;
                            }

                            if(cmdlist.empty()) continue ;

                            //create BindingsMatcher for one mode
                            matcher_list[static_cast<unsigned char>(index.first)] \
                                = std::make_shared<BindingsMatcher>(std::move(cmdlist)) ;
                        }
                        catch(const std::out_of_range& e) {
                            PRINT_ERROR(e.what()) ;
                            continue ;
                        }
                    }

                    //If there are some key-bindings fields of the mode having <mode-name> (e.q. <guin>, <edin>) in bindings.json ,
                    //they are copied key-bindings from the first mode in json-array to them.
                    //Ex) "guin": ["<Esc>", "happy"]
                    //    "edin": ["<guin>", "<guii>"]    -> same as "guin"'s key-bindings(<Esc>, "happy")
                    for(std::size_t i = 0 ; i < index_links.size() ; i ++) {
                        const auto link_idx = index_links[i] ;
                        if(link_idx == static_cast<unsigned char>(Mode::None)) 
                            continue ;

                        matcher_list[i] = matcher_list[link_idx] ;
                    }

                    for(std::size_t i = 0 ; i < matcher_list.size() ; i ++) {
                        func->register_matcher(static_cast<mode::Mode>(i), matcher_list[i]) ;
                    }
                }
                catch(const std::out_of_range& e) {
                    PRINT_ERROR(std::string(e.what()) + ". The following syntax is invalid." + obj.dump()) ;
                    continue ;
                }
            }

            //post process
            Jump2Any::load_config() ;
            exapp::load_config() ;
        }

        bool is_invalid_log(const KeyLog& log, const InvalidPolicy ip) {

            if(log.empty()) return true ;

            auto must_ignore = [&log](auto&& set) {
                return std::all_of(log.cbegin(), log.cend(), [&set](const auto& key) {
                    return set.find(key) != set.end() ;
                }) ;
            } ;

            switch(ip) {
                case InvalidPolicy::None: {
                    return false ;
                }
                case InvalidPolicy::AllSystemKey: {
                    static const auto system_keys = keycodecvt::get_all_sys_vkc() ;
                    return must_ignore(system_keys) ;
                }
                case InvalidPolicy::UnbindedSystemKey: {
                    return must_ignore(g_unbinded_syskeys) ;
                }
                default: {
                    return false ;
                }
            }
        }

        //This function regards as other functions is stronger than the running function.
        //If the 2nd argument is not passed, it regards as not processing.
        const BindedFunc::shp_t find_func(
                const KeyLoggerBase& lgr,
                const BindedFunc::shp_t& running_func,
                const bool full_scan,
                mode::Mode mode) {

            unsigned int most_matched_num  = 0 ;
            BindedFunc::shp_t matched_func = nullptr ;

            auto choose = [&most_matched_num, &matched_func](auto& func, auto num) {
                if(num > most_matched_num) {
                    most_matched_num = num ;
                    matched_func     = func ;
                }
                else if(num == most_matched_num && func->is_callable()) {
                    //On same matching level, the callable function is the strongest.
                    matched_func = func ;
                }
            } ;

            if(!running_func) { //lower cost version
                if(full_scan) {
                    for(const auto& func : g_func_list)
                        choose(func, func->validate_if_fullmatch(lgr, mode)) ;
                }
                else {
                    for(const auto& func : g_func_list)
                        choose(func, func->validate_if_match(lgr, mode)) ;
                }
                return matched_func ;
            }

            unsigned int matched_num ;
            if(full_scan) {
                for(const auto& func : g_func_list) {
                    matched_num = func->validate_if_fullmatch(lgr, mode) ;
                    if(running_func == func) continue ;
                    choose(func, matched_num) ;
                }
            }
            else {
                for(const auto& func : g_func_list) {
                    matched_num = func->validate_if_match(lgr, mode) ;
                    if(running_func == func) continue ;
                    choose(func, matched_num) ;
                }
            }

            //New matched function is given priority over running func.
            if(matched_func)
                return matched_func ;

            if(running_func->is_callable())
                return running_func ;

            return nullptr ;
        }

        const BindedFunc::shp_t find_func_byname(const std::string& name) {
                for(const auto& func : g_func_list) {
                    if(func->name() == name) return func ;
                }
                return nullptr ;
        }
    }
}


//internal linkage
namespace
{
    KeycodeLogger g_logger{} ;
    BindedFunc::shp_t g_running_func       = nullptr ;
    unsigned int g_repeat_num              = 0 ;
    bool g_must_release_key_after_repeated = false ;
}

namespace vind
{
    namespace keybind {
        void call_matched_funcs() {
            static const KeyLog c_nums {
                KEYCODE_0, KEYCODE_1, KEYCODE_2, KEYCODE_3, KEYCODE_4,
                KEYCODE_5, KEYCODE_6, KEYCODE_7, KEYCODE_8, KEYCODE_9
            } ;

            g_logger.update() ;
            if(!g_logger.is_changed()) {
                if(!g_running_func) {
                    g_logger.remove_from_back(1) ;
                    return ;
                }
                g_running_func->process(false, 1, &g_logger, nullptr) ;
                g_logger.remove_from_back(1) ;
                return ;
            }

            if(g_repeat_num != 0) {
                if(g_logger.latest().is_containing(KEYCODE_ESC)) {
                    g_repeat_num = 0 ;
                    VirtualCmdLine::reset() ;
                }
            }

            //Note
            //it ignores solo system keys.
            //Ex)
            //  ______________________________________________________
            // |                |                       |             |
            // |   input keys   |        Shift          |  Shift + t  |
            // |                | (unbinded key only)   |             | 
            // |----------------|-----------------------|-------------|
            // |   behavior     |        ignore         |    pass     |
            // |________________|_______________________|_____________|
            //
            if(is_invalid_log(g_logger.latest(), InvalidPolicy::UnbindedSystemKey)) {
                g_logger.remove_from_back(1) ;
                g_running_func = nullptr ;

                if(g_must_release_key_after_repeated) {
                    g_must_release_key_after_repeated = false ;
                }

                return ;
            }

            // Note about g_must_release_key_after_repeated:
            // false : same as default.
            // true  : wait until some unbinded sytem keys are inputed or no keys is inputed.
            // 
            // This behavior is needed to prohibit following case.
            // Ex)
            //  ________________________________________________________________________________________
            // |                            |      |         |                   |                      |
            // |         input keys         |  2   |  Shift  |      Shift + j    |         j            |
            // |----------------------------|------|---------|-------------------|----------------------|
            // | called func name (without) |  -   |    -    |  edi_n_remove_EOL | edi_move_caret_down  |
            // |----------------------------|------|---------|-------------------|----------------------|
            // | called func name (with)    |  -   |    -    |  edi_n_remove_EOL |          -           |
            // |____________________________|______|_________|___________________|______________________|
            //
            if(g_must_release_key_after_repeated) {
                g_logger.remove_from_back(1) ;
                g_running_func = nullptr ;
                return ;
            }
            auto topvkc = *(g_logger.latest().begin()) ;

            //If some numbers has inputed, ignore commands binded by numbers.
            if(g_repeat_num != 0) {
                g_logger.latest() -= c_nums ;
            }

            auto matched_func = find_func(g_logger, g_running_func) ;

            if(!matched_func) {
                if(!keycodecvt::is_number(topvkc)) {
                    //If inputed non-numeric key, reset the repeat number.
                    if(g_repeat_num != 0) {
                        g_repeat_num = 0 ;
                        VirtualCmdLine::reset() ;
                    }
                }
                else {
                    static constexpr auto max = std::numeric_limits<unsigned int>::max() / 10 ;
                    if(g_repeat_num < max && !mode::is_insert()) { //Whether it is not out of range?
                        g_repeat_num = g_repeat_num * 10 + keycodecvt::to_number(topvkc) ;
                        VirtualCmdLine::cout(std::to_string(g_repeat_num)) ;
                    }
                }

                g_logger.clear() ;
                g_running_func = nullptr ;
                return ;
            }

            if(matched_func->is_callable()) {
                g_running_func = matched_func ;

                if(g_repeat_num == 0) {
                    g_running_func->process(true, 1, &g_logger, nullptr) ;
                }
                else {
                    VirtualCmdLine::reset() ;
                    g_running_func->process(true, g_repeat_num, &g_logger, nullptr) ;
                    g_repeat_num = 0 ;
                    g_must_release_key_after_repeated = true ;
                }

                g_logger.clear() ;
                return ;
            }
        }
    }
}
