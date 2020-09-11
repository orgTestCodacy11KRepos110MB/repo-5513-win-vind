#ifndef _EDI_LAYOUT_HPP
#define _EDI_LAYOUT_HPP

#include "key_binding_with_creator.hpp"

struct EdiNRemoveEOL : public KeyBindingWithCreator<EdiNRemoveEOL>
{
    bool sprocess(const bool first_call) const ;
    static const std::string sname() noexcept ;

    explicit EdiNRemoveEOL() ;
    virtual ~EdiNRemoveEOL() noexcept ;

    EdiNRemoveEOL(EdiNRemoveEOL&&) noexcept ;
    EdiNRemoveEOL& operator=(EdiNRemoveEOL&&) noexcept ;

    EdiNRemoveEOL(const EdiNRemoveEOL&) = delete ;
    EdiNRemoveEOL& operator=(const EdiNRemoveEOL&) = delete ;

private:
    struct Impl ;
    std::unique_ptr<Impl> pimpl ;
} ;

#endif