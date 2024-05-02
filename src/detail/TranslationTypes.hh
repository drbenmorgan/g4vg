//----------------------------------*-C++-*----------------------------------//
// Copyright 2023-2024 UT-Battelle, LLC, and other Celeritas developers.
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: (Apache-2.0 OR MIT)
//---------------------------------------------------------------------------//
//! \file g4vg/detail/TransformerTypes.hh
//---------------------------------------------------------------------------//
#pragma once

#include <array>
#include <ostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <G4GDMLWriteStructure.hh>
#include <G4LogicalVolume.hh>
#include <G4ReflectionFactory.hh>
#include <VecGeom/base/Global.h>
#include <VecGeom/management/Logger.h>

#ifdef __GNUG__
#    include <cstdlib>
#    include <cxxabi.h>
#endif  // __GNUG__


// Function related macros can be empty for now
#define CELER_FUNCTION
#define CELER_CONSTEXPR_FUNCTION

// Expect/Assert Macros are just noops for now
#define CELER_ASSERT(COND)
#define CELER_DISCARD(COND)
#define CELER_ENSURE(COND)
#define CELER_EXPECT(COND)
#define CELER_ENSURE(COND)
#define CELER_UNLIKELY(COND) (COND)

#define CELER_RUNTIME_THROW(WHICH, WHAT, COND) \
    throw ::g4vg::RuntimeError({               \
        WHICH,                                 \
        WHAT,                                  \
        COND,                                  \
        __FILE__,                              \
        __LINE__,                              \
    })

#define CELER_VALIDATE(COND, MSG)                                            \
    do                                                                       \
    {                                                                        \
        if (CELER_UNLIKELY(!(COND)))                                         \
        {                                                                    \
            std::ostringstream celer_runtime_msg_;                           \
            celer_runtime_msg_ MSG;                                          \
            CELER_RUNTIME_THROW("runtime", celer_runtime_msg_.str(), #COND); \
        }                                                                    \
    } while (0)

// Celeritas log interface is same as Vecgeom
#define CELER_LOG(STATUS) VECGEOM_LOG(STATUS)

namespace g4vg
{
// real_type is basically VecGeom's Precision
using real_type = vecgeom::Precision;

// Size_t for non-device
using size_type = std::size_t;

namespace constants
{
inline constexpr real_type pi = 3.14159265358979323846;
}

// Celeritas' Array is a basically std::array with device support. We don't use
// device here
template<typename T, size_type N>
using Array = std::array<T, N>;

// Celeritas' range is basically a count from zero to the input arg
// This is a filthy hack around that just to get things working for the use cases
// present in G4VG
template<typename N>
auto range(N const& i) -> std::vector<N>
{
    std::vector<N> r(i, 0);
    N counter{0};
    for (auto& v : r)
    {
        v = counter;
        ++counter;
    }
    return r;
}

// OpaqueId is used in a handful of places
template<class ValueT, class SizeT = size_type>
class OpaqueId
{
    static_assert(std::is_unsigned_v<SizeT> && !std::is_same_v<SizeT, bool>,
                  "SizeT must be unsigned.");

  public:
    //!@{
    //! \name Type aliases
    using value_type = ValueT;
    using size_type = SizeT;
    //!@}

  public:
    //! Default to invalid state
    OpaqueId() : value_(OpaqueId::invalid_value()) {}

    //! Construct explicitly with stored value
    explicit OpaqueId(size_type index) : value_(index) {}

    //! Whether this ID is in a valid (assigned) state
    explicit operator bool() const { return value_ != invalid_value(); }

    //! Pre-increment of the ID
    OpaqueId& operator++()
    {
        // CELER_EXPECT(*this);
        value_ += 1;
        return *this;
    }

    //! Post-increment of the ID
    OpaqueId operator++(int)
    {
        // CELER_EXPECT(*this);
        OpaqueId old{*this};
        ++*this;
        return old;
    }

    //! Get the ID's value
    size_type get() const
    {
        // CELER_EXPECT(*this);
        return value_;
    }

    //! Get the value without checking for validity (atypical)
    size_type unchecked_get() const { return value_; }

  private:
    size_type value_;

    //// IMPLEMENTATION FUNCTIONS ////

    //! Value indicating the ID is not assigned
    static size_type invalid_value() { return static_cast<size_type>(-1); }
    // friend detail::LdgLoader<OpaqueId<value_type, size_type> const, void>;
};

// VolumeId is an OpaqueId<struct Volume_>
using VolumeId = OpaqueId<struct Volume_>;

//---------------------------------------------------------------------------//
//! Wrap around a G4LogicalVolume to get a descriptive output.
struct PrintableLV
{
    G4LogicalVolume const* lv{nullptr};
};

// Print the logical volume name, ID, and address.
std::ostream& operator<<(std::ostream& os, PrintableLV const& pnh);

//---------------------------------------------------------------------------//
/*!
 * Print the logical volume name, ID, and address.
 */
inline
std::ostream& operator<<(std::ostream& os, PrintableLV const& plv)
{
    if (plv.lv)
    {
        os << '"' << plv.lv->GetName() << "\"@"
           << static_cast<void const*>(plv.lv)
           << " (ID=" << plv.lv->GetInstanceID() << ')';
    }
    else
    {
        os << "{null G4LogicalVolume}";
    }
    return os;
}

//---------------------------------------------------------------------------//
// Generate the GDML name for a Geant4 logical volume
std::string make_gdml_name(G4LogicalVolume const&);
//---------------------------------------------------------------------------//
/*!
 * Generate the GDML name for a Geant4 logical volume.
 */
inline
std::string make_gdml_name(G4LogicalVolume const& lv)
{
    // Run the LV through the GDML export name generator so that the volume is
    // uniquely identifiable in VecGeom. Reuse the same instance to reduce
    // overhead: note that the method isn't const correct.
    static G4GDMLWriteStructure temp_writer;

    auto const* refl_factory = G4ReflectionFactory::Instance();
    if (auto const* unrefl_lv
        = refl_factory->GetConstituentLV(const_cast<G4LogicalVolume*>(&lv)))
    {
        // If this is a reflected volume, add the reflection extension after
        // the final pointer to match the converted VecGeom name
        std::string name
            = temp_writer.GenerateName(unrefl_lv->GetName(), unrefl_lv);
        name += refl_factory->GetVolumesNameExtension();
        return name;
    }

    return temp_writer.GenerateName(lv.GetName(), &lv);
}

// TypeDemangling is fairly simple
//---------------------------------------------------------------------------//
/*!
 * Utility function for demangling C++ types (specifically with GCC).
 *
 * See:
 * http://stackoverflow.com/questions/281818/unmangling-the-result-of-stdtype-infoname
 * Example:
 * \code
    std::string int_type = demangled_typeid_name(typeid(int).name());
    TypeDemangler<Base> demangle;
    std::string static_type = demangle();
    std::string dynamic_type = demangle(Derived());
   \endcode
 */
template<class T>
class TypeDemangler
{
  public:
    // Get the pretty typename of the instantiated type (static)
    inline std::string operator()() const;
    // Get the *dynamic* pretty typename of a variable (dynamic)
    inline std::string operator()(T const&) const;
};

//---------------------------------------------------------------------------//
// FREE FUNCTIONS
//---------------------------------------------------------------------------//
// Demangle the name that comes from `typeid`
std::string demangled_typeid_name(char const* typeid_name);

//---------------------------------------------------------------------------//
//! Demangle the type name of any variable
template<class T>
std::string demangled_type(T&&)
{
    return demangled_typeid_name(typeid(T).name());
}

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Get the pretty typename of the instantiated type (static).
 */
template<class T>
inline
std::string TypeDemangler<T>::operator()() const
{
    return demangled_typeid_name(typeid(T).name());
}

//---------------------------------------------------------------------------//
/*!
 * Get the *dynamic* pretty typename of a variable (dynamic).
 */
template<class T>
inline
std::string TypeDemangler<T>::operator()(T const& t) const
{
    return demangled_typeid_name(typeid(t).name());
}

//---------------------------------------------------------------------------//
inline
std::string demangled_typeid_name(char const* typeid_name)
{
#ifdef __GNUG__
    int status = -1;
    // Return a null-terminated string allocated with malloc
    char* demangled
        = abi::__cxa_demangle(typeid_name, nullptr, nullptr, &status);

    // Copy the C string to a STL string if successful, or the mangled name if
    // not
    std::string result(status == 0 ? demangled : typeid_name);

    // Free the returned memory
    std::free(demangled);
#else  // __GNUG__
    std::string result(typeid_name);
#endif  // __GNUG__

    return result;
}

// Runtime error
//! Detailed properties of a runtime error
struct RuntimeErrorDetails
{
    std::string which{};  //!< Type of error (runtime, Geant4, MPI)
    std::string what{};  //!< Descriptive message
    std::string condition{};  //!< Code/test that failed
    std::string file{};  //!< Source file
    int line{0};  //!< Source line
};

inline
char const* color_code(char abbrev)
{
    return "";
}

//---------------------------------------------------------------------------//
/*!
 * Construct a runtime assertion message for printing.
 */
inline
std::string build_runtime_error_msg(RuntimeErrorDetails const& d)
{
    static bool const verbose_message = [] {
        /*
        #if CELERITAS_DEBUG
                // Always verbose if debug flags are enabled
                return true;
        #else
                // Verbose if the CELER_LOG environment variable is defined
                return !celeritas::getenv("CELER_LOG").empty();
        #endif
        */
        return true;
    }();

    std::ostringstream msg;

    msg << "celeritas: " << color_code('R')
        << (d.which.empty() ? "unknown" : d.which)
        << " error: " << color_code(' ');
    if (d.which == "configuration")
    {
        msg << "required dependency is disabled in this build: ";
    }
    else if (d.which == "implementation")
    {
        msg << "feature is not yet implemented: ";
    }
    msg << d.what;

    if (verbose_message || d.what.empty() || d.which != "runtime")
    {
        msg << '\n'
            << color_code(d.condition.empty() ? 'x' : 'W')
            << (d.file.empty() ? "unknown source" : d.file);
        if (d.line && !d.file.empty())
        {
            msg << ':' << d.line;
        }

        msg << ':' << color_code(' ');
        if (!d.condition.empty())
        {
            msg << " '" << d.condition << '\'' << " failed";
        }
        else
        {
            msg << " failure";
        }
    }

    return msg.str();
}

//---------------------------------------------------------------------------//
/*!
 * Error thrown by working code from unexpected runtime conditions.
 */
class RuntimeError : public std::runtime_error
{
  public:
    // Construct from details
    explicit RuntimeError(RuntimeErrorDetails&&);

    //! Access detailed information
    RuntimeErrorDetails const& details() const { return details_; }

  private:
    RuntimeErrorDetails details_;
};

//---------------------------------------------------------------------------//
/*!
 * Construct a runtime error from detailed descriptions.
 */
inline
RuntimeError::RuntimeError(RuntimeErrorDetails&& d)
    : std::runtime_error(build_runtime_error_msg(d)), details_(std::move(d))
{
}

// SoftEqual is a little involved...
namespace detail
{
//---------------------------------------------------------------------------//
/*!
 * Provide relative errors for soft_equiv based on type.
 *
 * This also gives compile-time checking for bad values.
 */
template<class T>
struct SoftEqualTraits
{
    using value_type = T;

    //! Default relative error
    static CELER_CONSTEXPR_FUNCTION value_type rel_prec()
    {
        static_assert(sizeof(T) == 0, "Invalid type for softeq!");
        return T();
    }

    //! Default absolute error
    static CELER_CONSTEXPR_FUNCTION value_type abs_thresh()
    {
        static_assert(sizeof(T) == 0, "Invalid type for softeq!");
        return T();
    }
};

template<>
struct SoftEqualTraits<double>
{
    using value_type = double;
    static CELER_CONSTEXPR_FUNCTION value_type sqrt_prec() { return 1.0e-6; }
    static CELER_CONSTEXPR_FUNCTION value_type rel_prec() { return 1.0e-12; }
    static CELER_CONSTEXPR_FUNCTION value_type abs_thresh() { return 1.0e-14; }
};

template<>
struct SoftEqualTraits<float>
{
    using value_type = float;
    static CELER_CONSTEXPR_FUNCTION value_type sqrt_prec() { return 1.0e-3f; }
    static CELER_CONSTEXPR_FUNCTION value_type rel_prec() { return 1.0e-6f; }
    static CELER_CONSTEXPR_FUNCTION value_type abs_thresh() { return 1.0e-6f; }
};
//---------------------------------------------------------------------------//
}  // namespace detail

//---------------------------------------------------------------------------//
/*!
 * Functor for noninfinite floating point equality.
 *
 * This function-like class considers an \em absolute tolerance for values near
 * zero, and a \em relative tolerance for values far from zero. It correctly
 * returns "false" if either value being compared is NaN.  The call operator is
 * \em commutative: \c eq(a,b) should always give the same as \c eq(b,a).
 *
 * The actual comparison is: \f[
 |a - b| < \max(\epsilon_r \max(|a|, |b|), \epsilon_a)
 \f]
 *
 * \note The edge case where both values are infinite (with the same sign)
 * returns *false* for equality, which could be considered reasonable because
 * relative error is meaningless. To explicitly allow infinities to compare
 * equal, you must test separately, e.g., `a == b || soft_eq(a, b)`.
 */
template<class RealType = real_type>
class SoftEqual
{
  public:
    //!@{
    //! \name Type aliases
    using value_type = RealType;
    //!@}

  public:
    // Construct with default relative/absolute precision
    CELER_CONSTEXPR_FUNCTION SoftEqual();

    // Construct with default absolute precision
    explicit CELER_FUNCTION SoftEqual(value_type rel);

    // Construct with both relative and absolute precision
    CELER_FUNCTION SoftEqual(value_type rel, value_type abs);

    //// COMPARISON ////

    // Compare two values (implicitly casting arguments)
    bool CELER_FUNCTION operator()(value_type a, value_type b) const;

    //// ACCESSORS ////

    //! Relative allowable error
    CELER_CONSTEXPR_FUNCTION value_type rel() const { return rel_; }

    //! Absolute tolerance
    CELER_CONSTEXPR_FUNCTION value_type abs() const { return abs_; }

  private:
    value_type rel_;
    value_type abs_;

    using SETraits = detail::SoftEqualTraits<value_type>;
};

//---------------------------------------------------------------------------//
// TEMPLATE DEDUCTION GUIDES
//---------------------------------------------------------------------------//
template<class T>
CELER_FUNCTION SoftEqual(T) -> SoftEqual<T>;
template<class T>
CELER_FUNCTION SoftEqual(T, T) -> SoftEqual<T>;

//---------------------------------------------------------------------------//
// INLINE DEFINITIONS
//---------------------------------------------------------------------------//
/*!
 * Construct with default relative/absolute precision.
 */
template<class RealType>
CELER_CONSTEXPR_FUNCTION SoftEqual<RealType>::SoftEqual()
    : rel_{SETraits::rel_prec()}, abs_{SETraits::abs_thresh()}
{
}

//---------------------------------------------------------------------------//
/*!
 * Construct with scaled absolute precision.
 */
template<class RealType>
CELER_FUNCTION SoftEqual<RealType>::SoftEqual(value_type rel)
    : SoftEqual(rel, rel * (SETraits::abs_thresh() / SETraits::rel_prec()))
{
    CELER_EXPECT(rel > 0);
}

//---------------------------------------------------------------------------//
/*!
 * Construct with both relative and absolute precision.
 *
 * \param rel tolerance of relative error (default 1.0e-12 for doubles)
 *
 * \param abs threshold for absolute error when comparing small quantities
 *           (default 1.0e-14 for doubles)
 */
template<class RealType>
CELER_FUNCTION SoftEqual<RealType>::SoftEqual(value_type rel, value_type abs)
    : rel_{rel}, abs_{abs}
{
    CELER_EXPECT(rel > 0);
    CELER_EXPECT(abs > 0);
}

//---------------------------------------------------------------------------//
/*!
 * Compare two values, implicitly casting arguments.
 */
template<class RealType>
CELER_FUNCTION bool
SoftEqual<RealType>::operator()(value_type a, value_type b) const
{
    real_type rel = rel_ * std::fmax(std::fabs(a), std::fabs(b));
    return std::fabs(a - b) < std::fmax(abs_, rel);
}


//---------------------------------------------------------------------------//
/*!
 * Return an integer power of the input value.
 *
 * Example: \code
  assert(9.0 == ipow<2>(3.0));
  assert(256 == ipow<8>(2));
  static_assert(256 == ipow<8>(2));
 \endcode
 */
template<unsigned int N, class T>
CELER_CONSTEXPR_FUNCTION T ipow(T v) noexcept
{
    if constexpr (N == 0)
    {
        CELER_DISCARD(v)  // Suppress warning in older compilers
        return 1;
    }
    else if constexpr (N % 2 == 0)
    {
        return ipow<N / 2>(v) * ipow<N / 2>(v);
    }
    else
    {
        return v * ipow<(N - 1) / 2>(v) * ipow<(N - 1) / 2>(v);
    }
/*
#if (__CUDACC_VER_MAJOR__ < 11) \
    || (__CUDACC_VER_MAJOR__ == 11 && __CUDACC_VER_MINOR__ < 5)
    // "error: missing return statement at end of non-void function"
    return T{};
#endif
*/
}

}  // namespace g4vg
