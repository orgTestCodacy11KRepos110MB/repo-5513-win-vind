#include <doctest.h>

#include "mode.cpp"

using namespace vind ;
using namespace vind::mode ;

TEST_CASE("(mode::set_global_mode, mode::get_global_mode) Check set-get") {
    CHECK_NOTHROW(set_global_mode(Mode::GUI_NORMAL)) ;
    CHECK_EQ(get_global_mode(), Mode::GUI_NORMAL) ;

    CHECK_NOTHROW(set_global_mode(static_cast<int>(Mode::GUI_VISUAL))) ;
    CHECK_EQ(get_global_mode(), Mode::GUI_VISUAL) ;
}

TEST_CASE("(mode::set_global_flags, mode::get_global_flags) Check set-get") {
    CHECK_NOTHROW(set_global_mode(Mode::GUI_NORMAL, ModeFlags::VISUAL_LINE)) ;
    CHECK_EQ(get_global_flags(), ModeFlags::VISUAL_LINE) ;
}

TEST_CASE("(mode::convert_to_prefix) Check i/o") {
    CHECK_EQ(to_prefix(Mode::INSERT), "i") ;
}

TEST_CASE("(mode::parse_prefix) Check i/o") {
    CHECK_EQ(parse_prefix("i"), Mode::INSERT) ;
}

TEST_CASE("(mode::to_name) Check i/o") {
    CHECK_EQ(to_name(Mode::INSERT), "Insert") ;
    CHECK_EQ(to_name(static_cast<int>(Mode::INSERT)), "Insert") ;
}
