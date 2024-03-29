#include "jumpcaret.hpp"

#include "bind/saferepeat.hpp"
#include "core/inputgate.hpp"
#include "core/mode.hpp"
#include "motionids.hpp"
#include "textsel.hpp"
#include "util/debug.hpp"
#include "util/def.hpp"
#include "util/string.hpp"


namespace vind
{
    namespace bind
    {
        //JumpCaretToBOL
        JumpCaretToBOL::JumpCaretToBOL()
        : BindedFuncVoid("jump_caret_to_BOL")
        {
            MotionIds::get_instance().register_id(id()) ;
        }
        void JumpCaretToBOL::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto& igate = core::InputGate::get_instance() ;
            if(core::get_global_mode() == core::Mode::EDI_VISUAL) {
                igate.pushup(KEYCODE_LSHIFT, KEYCODE_HOME) ;
            }
            else {
                igate.pushup(KEYCODE_HOME) ;
            }
        }

        //JumpCaretToEOL
        JumpCaretToEOL::JumpCaretToEOL()
        : BindedFuncVoid("jump_caret_to_EOL")
        {
            MotionIds::get_instance().register_id(id()) ;
        }
        void JumpCaretToEOL::sprocess(
                std::uint16_t count,
                const std::string& UNUSED(args)) {
            auto& igate = core::InputGate::get_instance() ;

            //down caret N - 1
            safe_for(count - 1, [&igate] {
                igate.pushup(KEYCODE_DOWN) ;
            }) ; 

            if(core::get_global_mode() == core::Mode::EDI_VISUAL) {
                igate.pushup(KEYCODE_LSHIFT, KEYCODE_END) ;
            }
            else {
                igate.pushup(KEYCODE_END) ;
                igate.pushup(KEYCODE_LEFT) ;
            }
        }

        //EdiJumpCaret2NLine_DfBOF
        JumpCaretToBOF::JumpCaretToBOF()
        : BindedFuncVoid("jump_caret_to_BOF")
        {
            MotionIds::get_instance().register_id(id()) ;
        }
        void JumpCaretToBOF::sprocess(
                std::uint16_t count,
                const std::string& args) {
            if(!args.empty()) {
                if(auto num = util::extract_num<std::uint16_t>(args)) {
                    count = num ;
                }
            }

            if(is_first_line_selection()) {
                select_line_EOL2BOL() ;
            }

            auto& igate = core::InputGate::get_instance() ;

            if(core::get_global_mode() == core::Mode::EDI_VISUAL) {
                igate.pushup(KEYCODE_LSHIFT, KEYCODE_LCTRL, KEYCODE_HOME) ;

                //down caret N - 1
                safe_for(count - 1, [&igate] {
                    igate.pushup(KEYCODE_LSHIFT, KEYCODE_DOWN) ;
                }) ;
            }
            else {
                igate.pushup(KEYCODE_LCTRL, KEYCODE_HOME) ;

                //down caret N - 1
                safe_for(count - 1, [&igate] {
                    igate.pushup(KEYCODE_DOWN) ;
                }) ;
            }
        }

        //EdiJumpCaret2NLine_DfEOF
        JumpCaretToEOF::JumpCaretToEOF()
        : BindedFuncVoid("jump_caret_to_EOF")
        {
            MotionIds::get_instance().register_id(id()) ;
        }
        void JumpCaretToEOF::sprocess(
                std::uint16_t count,
                const std::string& args) {
            auto& igate = core::InputGate::get_instance() ;

            if(count == 1) {
                if(core::get_global_mode() == core::Mode::EDI_VISUAL) {
                    if(is_first_line_selection()) {
                        select_line_BOL2EOL() ;
                    }

                    igate.pushup(KEYCODE_LSHIFT, KEYCODE_LCTRL, KEYCODE_END) ;

                    if(!(core::get_global_mode_flags() & core::ModeFlags::VISUAL_LINE)) {
                        igate.pushup(KEYCODE_LSHIFT, KEYCODE_HOME) ;
                    }
                }
                else {
                    igate.pushup(KEYCODE_LCTRL, KEYCODE_END) ;
                }
            }
            else {
                JumpCaretToBOF::sprocess(count, args) ;
            }
        }
    }
}
