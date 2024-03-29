#ifndef _INSTANT_MODE_HPP
#define _INSTANT_MODE_HPP

#include "bind/bindedfunc.hpp"

namespace vind
{
    namespace bind
    {
        class ToInstantGUINormal : public BindedFuncFlex<ToInstantGUINormal> {
        private:
            struct Impl ;
            std::unique_ptr<Impl> pimpl ;

        public:
            explicit ToInstantGUINormal() ;
            virtual ~ToInstantGUINormal() noexcept ;

            SystemCall sprocess(
                std::uint16_t count, const std::string& args) ;

            ToInstantGUINormal(ToInstantGUINormal&&) ;
            ToInstantGUINormal& operator=(ToInstantGUINormal&&) ;
            ToInstantGUINormal(const ToInstantGUINormal&)            = delete ;
            ToInstantGUINormal& operator=(const ToInstantGUINormal&) = delete ;

            bool is_mode_modifiable() const noexcept override {
                return true ;
            }
        } ;
    }
}

#endif
