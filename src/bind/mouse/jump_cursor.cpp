#include "jump_cursor.hpp"

#include "core/settable.hpp"
#include "util/def.hpp"
#include "util/rect.hpp"
#include "util/screen_metrics.hpp"
#include "util/winwrap.hpp"

namespace vind
{
    namespace bind
    {
        //JumpToLeft
        JumpToLeft::JumpToLeft()
        : BindedFuncVoid("jump_cursor_to_left")
        {}
        void JumpToLeft::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;
            auto box = util::get_combined_metrics() ;
            util::set_cursor_pos(box.left(), pos.y()) ;
        }

        //JumpToRight
        JumpToRight::JumpToRight()
        : BindedFuncVoid("jump_cursor_to_right")
        {}
        void JumpToRight::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;

            auto box = util::get_combined_metrics() ;

            auto& settable = core::SetTable::get_instance() ;
            util::set_cursor_pos(
                    box.right() - settable.get("jump_margin").get<int>(),
                    pos.y()) ;
        }

        //JumpToTop
        JumpToTop::JumpToTop()
        : BindedFuncVoid("jump_cursor_to_top")
        {}
        void JumpToTop::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;
            auto box = util::get_combined_metrics() ;

            util::set_cursor_pos(pos.x(), box.top()) ;
        }

        //JumpToBottom
        JumpToBottom::JumpToBottom()
        : BindedFuncVoid("jump_cursor_to_bottom")
        {}
        void JumpToBottom::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;
            auto box = util::get_combined_metrics() ;

            auto& settable = core::SetTable::get_instance() ;
            util::set_cursor_pos(
                    pos.x(),
                    box.height() - settable.get("jump_margin").get<int>()) ;
        }

        //JumpToHorizontalCenter
        JumpToHorizontalCenter::JumpToHorizontalCenter()
        : BindedFuncVoid("jump_cursor_to_hcenter")
        {}
        void JumpToHorizontalCenter::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;
            auto box = util::get_combined_metrics() ;
            util::set_cursor_pos(box.center_x(), pos.y()) ;
        }

        //JumpToVerticalCenter
        JumpToVerticalCenter::JumpToVerticalCenter()
        : BindedFuncVoid("jump_cursor_to_vcenter")
        {}
        void JumpToVerticalCenter::sprocess(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            auto pos = util::get_cursor_pos() ;
            auto box = util::get_combined_metrics() ;
            util::set_cursor_pos(pos.x(), box.center_y()) ;
        }
    }
}
