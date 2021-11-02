#include "bind/syscmd/source.hpp"

#include "bind/binded_func.hpp"
#include "entry.hpp"
#include "err_logger.hpp"
#include "g_maps.hpp"
#include "g_params.hpp"
#include "key/char_logger.hpp"
#include "mode.hpp"
#include "opt/vcmdline.hpp"
#include "parser/bindings_parser.hpp"
#include "parser/rc_parser.hpp"
#include "path.hpp"
#include "util/def.hpp"
#include "util/winwrap.hpp"

#include "bind/syscmd/command.hpp"
#include "bind/syscmd/map.hpp"
#include "bind/syscmd/set.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(DEBUG)
#include <iostream>
#endif


namespace
{
    template <typename Str>
    auto load_remote_vindrc(Str&& args) {
        using namespace vind ;

        static const auto repo_store_path = path::ROOT_PATH() / "repo" ;

        if(!std::filesystem::exists(repo_store_path)) {
            std::filesystem::create_directories(repo_store_path) ;
        }

        auto slash = args.find_first_of("/") ;
        if(slash == std::string::npos) {
            throw RUNTIME_EXCEPT("Specify in the form user/repo") ;
        }

        const auto repo_dir = args.substr(0, slash) + "_" + args.substr(slash + 1) ;
        const auto target_repo_path = repo_store_path / repo_dir ;

        if(!std::filesystem::exists(target_repo_path)) {
            const auto remote_url = "https://github.com/" + args + ".git" ;

            util::create_process(
                repo_store_path, "git",
                util::concat_args("clone", "--depth=1", remote_url, repo_dir),
                false) ;

            using namespace std::chrono ;
            auto start = system_clock::now() ;
            while(true) {
                Sleep(500) ;
                if(std::filesystem::exists(target_repo_path)) {
                    break ;
                }
                if(system_clock::now() - start > 30s) {
                    break ;
                }
            }
        }
        else {
            util::create_process(target_repo_path, "git", "pull", false) ;
            Sleep(100) ;
        }

        return target_repo_path / ".vindrc" ;
    }


    template <typename Str>
    void do_runcommand(vind::rcparser::RunCommandsIndex rcindex, Str&& args) {
        using namespace vind ;
        using vind::rcparser::RunCommandsIndex ;

        switch(rcindex) {
            case RunCommandsIndex::SET: {
                SyscmdSet::sprocess(std::forward<Str>(args), false) ;
                return ;
            }
            case RunCommandsIndex::COMMAND: {
                SyscmdCommand::sprocess(std::forward<Str>(args), false) ;
                return ;

            }
            case RunCommandsIndex::DELCOMMAND: {
                SyscmdDelcommand::sprocess(std::forward<Str>(args), false) ;
                return ;
            }
            case RunCommandsIndex::COMCLEAR: {
                if(!args.empty()) {
                    throw std::invalid_argument("comclear") ;
                }
                SyscmdComclear::sprocess(false) ;
                return ;
            }
            case RunCommandsIndex::SOURCE: {
                if(args.empty()) {
                    throw std::invalid_argument("source") ;
                }

                auto args_path = std::filesystem::u8path(path::replace_magic(args)) ;

                if(std::filesystem::equivalent(path::RC(), args_path)) {
                    throw std::invalid_argument(
                            "Recursive references to the same .vindrc are not allowed.") ;
                }

                if(args_path.filename().u8string() != ".vindrc") {
                    args_path = load_remote_vindrc(std::forward<Str>(args)) ;
                }
                SyscmdSource::sprocess(args_path, false, false) ; //overload .vindrc
                return ;
            }

            default: {
                break ;
            }
        }

        using mode::Mode ;
        auto mode = static_cast<Mode>(rcindex & RunCommandsIndex::MASK_MODE) ;

        if(rcindex & RunCommandsIndex::MASK_MAP) {
            SyscmdMap::sprocess(mode, std::forward<Str>(args), false) ;
        }
        else if(rcindex & RunCommandsIndex::MASK_NOREMAP) {
            SyscmdNoremap::sprocess(mode, std::forward<Str>(args), false) ;
        }
        else if(rcindex & RunCommandsIndex::MASK_UNMAP) {
            SyscmdUnmap::sprocess(mode, std::forward<Str>(args), false) ;
        }
        else if(rcindex & RunCommandsIndex::MASK_MAPCLEAR) {
            if(!args.empty()) {
                throw std::invalid_argument("mapclear") ;
            }
            SyscmdMapclear::sprocess(mode, false) ;
        }
        else {
            throw std::domain_error(std::to_string(static_cast<int>(rcindex))) ;
        }
    }
}


namespace vind
{
    SyscmdSource::SyscmdSource()
    : BindedFuncCreator("system_command_source")
    {
        std::ifstream ifs(path::RC()) ;
        if(!ifs.is_open()) {
            std::ofstream ofs(path::RC(), std::ios::trunc) ;
        }
    }

    void SyscmdSource::sprocess(
            const std::filesystem::path& path,
            bool reload_config,
            bool start_from_default) {

        auto return_to_default = [] {
            gparams::reset() ;
            gmaps::reset() ;
        } ;

        if(start_from_default) {
            return_to_default() ;
        }

        std::ifstream ifs(path, std::ios::in) ;
        if(!ifs.is_open()) {
            VCmdLine::print(ErrorMessage("Could not open \"" + path.u8string() + "\".\n")) ;
            return ;
        }

        std::string aline ;
        std::size_t lnum = 0 ;
        while(getline(ifs, aline)) {
            lnum ++ ;

            try {
                rcparser::remove_dbquote_comment(aline) ;

                auto [cmd, args] = rcparser::divide_cmd_and_args(aline) ;
                if(cmd.empty()) {
                    continue ;
                }

                auto rcindex = rcparser::parse_run_command(cmd) ;

                do_runcommand(rcindex, args) ;
            }
            catch(const std::domain_error& e) {
                auto ltag = "L:" + std::to_string(lnum) ;
                VCmdLine::print(ErrorMessage("E: Not command (" + ltag + ")")) ;

                std::stringstream ss ;
                ss << "RunCommandsIndex: " << e.what() << " is not supported." ;
                ss << " (" << path.u8string() << ", " << ltag << ") " ;
                PRINT_ERROR(ss.str()) ;

                return_to_default() ;
                break ;
            }
            catch(const std::invalid_argument& e) {
                auto ltag = "L:" + std::to_string(lnum) ;
                VCmdLine::print(ErrorMessage("E: Invalid Argument (" + ltag + ")")) ;

                std::stringstream ss ;
                ss << e.what() << " is recieved invalid arguments." ;
                ss << " (" << path.u8string() << ", " << ltag << ") " ;
                PRINT_ERROR(ss.str()) ;

                return_to_default() ;
                break ;
            }
            catch(const std::logic_error& e) {
                auto ltag = "L:" + std::to_string(lnum) ;
                VCmdLine::print(ErrorMessage("E: Invalid Syntax (" + ltag + ")")) ;

                std::stringstream ss ;
                ss << e.what() ;
                ss << " (" + path.u8string() + ", " + ltag + ")" ;
                PRINT_ERROR(ss.str()) ;

                return_to_default() ;
                break ;
            }
            catch(const std::runtime_error& e) {
                auto ltag = "L:" + std::to_string(lnum) ;
                VCmdLine::print(ErrorMessage("E: Invalid Syntax (" + ltag + ")")) ;

                std::stringstream ss ;
                ss << e.what() ;
                ss << " (" + path.u8string() + ", " + ltag + ")" ;
                PRINT_ERROR(ss.str()) ;

                return_to_default() ;
                break ;
            }
        }

        if(reload_config) {
            vind::reconstruct_all_components() ; // Apply settings
        }
    }
    void SyscmdSource::sprocess(NTypeLogger&) {
    }
    void SyscmdSource::sprocess(const CharLogger& parent_lgr) {
        try {
            auto str = parent_lgr.to_str() ;
            if(str.empty()) {
                throw RUNTIME_EXCEPT("Empty command") ;
            }
            auto [cmd, args] = rcparser::divide_cmd_and_args(str) ;
            if(args.empty()) {
                sprocess(path::RC(), true) ;
            }
            else {
                sprocess(path::replace_magic(args), true) ;
            }
        }
        // If received syntax error as std::logic_error,
        // convert to runtime_error not to terminate application.
        catch(const std::exception& e) {
            throw std::runtime_error(e.what()) ;
        }
    }
}
