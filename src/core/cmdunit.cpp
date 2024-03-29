#include "cmdunit.hpp"

#include "cmdparser.hpp"
#include "inputgate.hpp"
#include "keycode.hpp"
#include "syscalldef.hpp"

#include "bind/bindedfunc.hpp"
#include "bind/bindinglist.hpp"
#include "util/def.hpp"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <sstream>


namespace vind
{
    namespace core
    {
        CmdUnit::CmdUnit()
        : keycodes_(),
          id_(0)
        {}

        CmdUnit::CmdUnit(const CmdUnitSet& codes)
        : keycodes_(codes),
          id_(compute_id(keycodes_))
        {}

        CmdUnit::CmdUnit(CmdUnitSet&& codes)
        : keycodes_(std::move(codes)),
          id_(compute_id(keycodes_))
        {}

        CmdUnit::CmdUnit(const KeyCode& code)
        : keycodes_{code},
          id_(compute_id(keycodes_))
        {}

        CmdUnit::CmdUnit(KeyCode&& code)
        : keycodes_{std::move(code)},
          id_(compute_id(keycodes_))
        {}

        CmdUnit::CmdUnit(std::initializer_list<KeyCode>&& codes)
        : keycodes_(codes),
          id_(compute_id(keycodes_))
        {}

        CmdUnit::~CmdUnit() noexcept = default ;

        CmdUnit::CmdUnit(const CmdUnit& rhs)
        : keycodes_(rhs.keycodes_),
          id_(rhs.id_)
        {}

        CmdUnit::CmdUnit(CmdUnit&&) = default ;
        CmdUnit& CmdUnit::operator=(CmdUnit&&) = default ;

        CmdUnit& CmdUnit::operator=(const CmdUnit&) = default ;
        CmdUnit& CmdUnit::operator=(const CmdUnitSet& rhs) {
            keycodes_ = rhs ;
            id_ = compute_id(keycodes_) ;
            return *this ;
        }

        std::size_t CmdUnit::compute_id(const CmdUnitSet& keys) const {
            /**
             * TODO: The unsigned char keycodes are considered as 
             *       as 8-bit characters, and compute its hash by
             *       std::string. This should be a more efficient
             *       method in the future.
             */
            std::vector<KeyCode> sorted_keys(keys.begin(), keys.end()) ;
            std::sort(sorted_keys.begin(), sorted_keys.end()) ;

            std::string str{} ;
            for(const auto& key : sorted_keys) {
                str.push_back(key.to_code()) ;
            }
            return std::hash<std::string>()(std::move(str)) ;
        }

        const CmdUnitSet& CmdUnit::get() const & noexcept {
            return keycodes_ ;
        }

        const CmdUnitSet& CmdUnit::data() const & noexcept {
            return keycodes_ ;
        }

        CmdUnitSet::const_iterator CmdUnit::begin() const noexcept {
            return keycodes_.begin() ;
        }

        CmdUnitSet::const_iterator CmdUnit::end() const noexcept {
            return keycodes_.end() ;
        }

        CmdUnitSet::const_iterator CmdUnit::cbegin() const noexcept {
            return keycodes_.cbegin() ;
        }

        CmdUnitSet::const_iterator CmdUnit::cend() const noexcept {
            return keycodes_.cend() ;
        }

        std::size_t CmdUnit::id() const noexcept {
            return id_ ;
        }

        std::size_t CmdUnit::size() const noexcept {
            return keycodes_.size() ;
        }

        bool CmdUnit::empty() const noexcept {
            return keycodes_.empty() ;
        }

        bool CmdUnit::is_containing(KeyCode key) const
        {
            return keycodes_.find(key) != keycodes_.end() ;
        }

        bool CmdUnit::operator==(const CmdUnit& rhs) const {
            return id_ == rhs.id_ ;
        }
        bool CmdUnit::operator==(CmdUnit&& rhs) const {
            return id_ == rhs.id_ ;
        }
        bool CmdUnit::operator==(const CmdUnitSet& rhs) const {
            return keycodes_ == rhs ;
        }
        bool CmdUnit::operator==(CmdUnitSet&& rhs) const {
            return keycodes_ == rhs ;
        }

        bool CmdUnit::operator!=(const CmdUnit& rhs) const {
            return id_ != rhs.id_ ;
        }
        bool CmdUnit::operator!=(CmdUnit&& rhs) const {
            return id_ != rhs.id_ ;
        }
        bool CmdUnit::operator!=(const CmdUnitSet& rhs) const {
            return keycodes_ != rhs ;
        }
        bool CmdUnit::operator!=(CmdUnitSet&& rhs) const {
            return keycodes_ != rhs ;
        }

        const CmdUnit CmdUnit::operator-(const CmdUnit& rhs) const {
            return CmdUnit(erased_diff(rhs.keycodes_)) ;
        }
        const CmdUnit CmdUnit::operator-(CmdUnit&& rhs) const {
            return CmdUnit(erased_diff(std::move(rhs.keycodes_))) ;
        }
        const CmdUnit CmdUnit::operator-(const CmdUnitSet& rhs) const {
            return CmdUnit(erased_diff(rhs)) ;
        }
        const CmdUnit CmdUnit::operator-(CmdUnitSet&& rhs) const {
            return CmdUnit(erased_diff(std::move(rhs))) ;
        }

        CmdUnit& CmdUnit::operator-=(const CmdUnit& rhs) {
            for(const auto& k : rhs) {
                keycodes_.erase(k) ;
            }
            id_ = compute_id(keycodes_) ;
            return *this ;
        }
        CmdUnit& CmdUnit::operator-=(CmdUnit&& rhs) {
            for(const auto& k : rhs) {
                keycodes_.erase(k) ;
            }
            id_ = compute_id(keycodes_) ;
            return *this ;
        }
        CmdUnit& CmdUnit::operator-=(const CmdUnitSet& rhs) {
            for(const auto& k : rhs) {
                keycodes_.erase(k) ;
            }
            id_ = compute_id(keycodes_) ;
            return *this ;
        }
        CmdUnit& CmdUnit::operator-=(CmdUnitSet&& rhs) {
            for(const auto& k : rhs) {
                keycodes_.erase(k) ;
            }
            id_ = compute_id(keycodes_) ;
            return *this ;
        }

        SystemCall CmdUnit::execute(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            return SystemCall::SUCCEEDED ;
        }

        SystemCall InternalCmdUnit::execute(
                std::uint16_t UNUSED(count),
                const std::string& UNUSED(args)) {
            return SystemCall::SUCCEEDED ;
        }

        SystemCall ExternalCmdUnit::execute(
                std::uint16_t count,
                const std::string& UNUSED(args)) {
            // To reproduce the keystroke, should consider the order for pressing.
            // The value of keycode are designed as sortable object for this purpose.
            std::vector<KeyCode> sequential(data().begin(), data().end()) ;
            std::sort(sequential.begin(), sequential.end()) ;
            for(decltype(count) i = 0 ; i < count ; i ++) {
                InputGate::get_instance().pushup(sequential.begin(), sequential.end()) ;
            }
            return SystemCall::SUCCEEDED ;
        }

        bool FunctionalCmdUnit::has_function() const noexcept {
            return func_ != nullptr ;
        }

        const bind::BindedFunc::SPtr& FunctionalCmdUnit::get_function() const {
            return func_ ;
        }

        std::size_t FunctionalCmdUnit::id() const noexcept {
            if(has_function()) {
                return func_->id() ;
            }
            return 0 ;
        }

        SystemCall FunctionalCmdUnit::execute(
                std::uint16_t count,
                const std::string& args) {
            if(!has_function()) {
                throw RUNTIME_EXCEPT("Does not have an associated function.") ;
            }
            return func_->process(count, args) ;
        }

        // Stream operators
        std::ostream& operator<<(std::ostream& stream, const CmdUnit::SPtr& rhs) {
            if(auto func_cmdunit = std::dynamic_pointer_cast<FunctionalCmdUnit>(rhs)) {
                stream << "<" << func_cmdunit->get_function()->name() << ">" ;
                return stream ;
            }

            if(!rhs || rhs->empty()) {
                return stream ;
            }

            if(rhs->size() == 1) {
                const auto& rhs_f = *(rhs->begin()) ;
                if(rhs_f.is_major_system()) {
                    stream << "<" << rhs_f << ">" ;
                }
                else {
                    stream << rhs_f ;
                }

                return stream;
            }

            std::vector<KeyCode> sorted(rhs->begin(), rhs->end()) ;
            std::sort(sorted.begin(), sorted.end()) ;

            stream << "<" ;
            for(auto itr = sorted.begin() ; itr != sorted.end() ; itr ++) {
                if(itr != sorted.begin()) {
                    stream << "-" ;
                }
                stream << *itr ;
            }
            stream << ">" ;
            return stream;
        }

        std::ostream& operator<<(std::ostream& stream, const CmdUnitSet& rhs) {
            stream << std::make_shared<CmdUnit>(rhs) ;
            return stream ;
        }

        std::ostream& operator<<(
            std::ostream& stream,
            const std::vector<CmdUnit>& rhs) {
            for(const auto& keyset : rhs) {
                stream << keyset ;
            }
            return stream ;
        }

        std::ostream& operator<<(
            std::ostream& stream,
            const std::vector<CmdUnitSet>& rhs) {
            for(const auto& keyset : rhs) {
                stream << keyset ;
            }
            return stream ;
        }

        std::ostream& operator<<(
            std::ostream& stream,
            const std::vector<CmdUnit::SPtr>& rhs) {
            for(const auto& keyset : rhs) {
                stream << keyset ;
            }
            return stream ;
        }

        std::ostream& operator<<(
            std::ostream& stream,
            const std::vector<std::unique_ptr<CmdUnit>>& rhs) {
            for(const auto& keyset : rhs) {
                stream << *keyset ;
            }
            return stream ;
        }


        // String operators
        std::string to_string(const CmdUnit::SPtr& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const CmdUnitSet& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const CmdUnit& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const std::vector<CmdUnit>& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const std::vector<CmdUnitSet>& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const std::vector<std::shared_ptr<CmdUnit>>& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
        std::string to_string(const std::vector<std::unique_ptr<CmdUnit>>& rhs) {
            std::stringstream ss ;
            ss << rhs ;
            return ss.str() ;
        }
    }
}
