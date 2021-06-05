#ifndef _EDI_CHANGE_MOVE_HPP
#define _EDI_CHANGE_MOVE_HPP

#include "bind/binded_func_creator.hpp"

namespace vind
{
    struct Change2EdiBOLInsert : public BindedFuncCreator<Change2EdiBOLInsert> {
        explicit Change2EdiBOLInsert() ;
        static void sprocess(bool vclmodeout=true) ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;

    struct Change2EdiBkInsert : public BindedFuncCreator<Change2EdiBkInsert> {
        explicit Change2EdiBkInsert() ;
        static void sprocess(bool vclmodeout=true) ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;

    struct Change2EdiEOLInsert : public BindedFuncCreator<Change2EdiEOLInsert> {
        explicit Change2EdiEOLInsert() ;
        static void sprocess(bool vclmodeout=true) ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;

    struct Change2EdiNlInsertBelow : public BindedFuncCreator<Change2EdiNlInsertBelow> {
        explicit Change2EdiNlInsertBelow() ;
        static void sprocess(bool vclmodeout=true) ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;

    struct Change2EdiNlInsertAbove : public BindedFuncCreator<Change2EdiNlInsertAbove> {
        explicit Change2EdiNlInsertAbove() ;
        static void sprocess(bool vclmodeout=true) ;
        static void sprocess(NTypeLogger& parent_lgr) ;
        static void sprocess(const CharLogger& parent_lgr) ;
    } ;
}

#endif
