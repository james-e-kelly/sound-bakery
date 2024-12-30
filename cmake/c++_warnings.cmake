# From https://github.com/joseasoler/cmake_cpp_warnings/blob/master/cmake/cpp_warnings.cmake

# This module defines a list of common warnings for the project called PROJECT_WARNING_FLAGS.
# By default this set includes as many reasonable warnings as possible. Only warnings that are supported by the compiler
# version in use will be enabled. The list of warnings is meant to be adapted and tweaked as needed for each project.
# For more details see https://lefticus.gitbooks.io/cpp-best-practices/content/02-Use_the_Tools_Available.html

include_guard(GLOBAL)

option(WARNINGS_AS_ERRORS "Targets using PROJECT_WARNING_FLAGS will treat warnings as errors." OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  option(CLANG_ENABLE_ALL_WARNINGS "PROJECT_WARNING_FLAGS will add all warnings (except C++98 compatibility ones) when using Clang" OFF)
endif ()

# When using MSVC in CMake 3.14 and below, /W3 is added to CMAKE_CXX_FLAGS by default.
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND "${CMAKE_CXX_FLAGS}" MATCHES "/W3")
  message(STATUS "Disabling /W3 flag added by default by CMake. See policy CMP0092.")
  string(REGEX REPLACE "/W3 " "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif ()


message(STATUS "Configuring PROJECT_WARNING_FLAGS.")
# Generate a warning flags list.
set(PROJECT_WARNING_FLAGS)

if (CLANG_ENABLE_ALL_WARNINGS)
  list(APPEND PROJECT_WARNING_FLAGS
    -Weverything                       # Enables every Clang warning.
    -Wno-c++98-compat                  # This project is not compatible with C++98.
    -Wno-c++98-compat-pedantic         # This project is not compatible with C++98.
    )
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Warnings present on all supported versions of GCC and Clang.
  list(APPEND PROJECT_WARNING_FLAGS
    -Wall                # Enables most warnings.
    -Wextra              # Enables an extra set of warnings.
    -pedantic            # Strict compliance to the standard is not met.
    -Wcast-align         # Pointer casts which increase alignment.
    -Wcast-qual          # A pointer is cast to remove a type qualifier, or add an unsafe one.
    -Wconversion         # Implicit type conversions that may change a value.
    -Wformat=2           # printf/scanf/strftime/strfmon format string anomalies.
    -Wnon-virtual-dtor   # Non-virtual destructors are found.
    -Wold-style-cast     # C-style cast is used in a program.
    -Woverloaded-virtual # Overloaded virtual function names.
    -Wsign-conversion    # Implicit conversions between signed and unsigned integers.
    -Wshadow             # One variable shadows another.
    -Wswitch-enum        # A switch statement has an index of enumerated type and lacks a case.
    -Wundef              # An undefined identifier is evaluated in an #if directive.
    -Wunused             # Enable all -Wunused- warnings.
    )
  # Enable additional warnings depending on the compiler and compiler version in use.
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND PROJECT_WARNING_FLAGS
      -Wdisabled-optimization       # GCC’s optimizers are unable to handle the code effectively.
      -Weffc++                      # Warnings related to guidelines from Scott Meyers’ Effective C++ books.
      -Wlogical-op                  # Warn when a logical operator is always evaluating to true or false.
      -Wsign-promo                  # Overload resolution chooses a promotion from unsigned to a signed type.
      -Wswitch-default              # A switch statement does not have a default case.
      -Wredundant-decls             # Something is declared more than once in the same scope.
      )
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
      list(APPEND PROJECT_WARNING_FLAGS
        -Wdouble-promotion          # Warn about implicit conversions from "float" to "double".
        )
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
      list(APPEND PROJECT_WARNING_FLAGS
        -Wuseless-cast              # Warn about useless casts.
        )
    endif ()
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5))
      list(APPEND PROJECT_WARNING_FLAGS
        -Wdate-time                 # Warn when encountering macros that might prevent bit-wise-identical compilations.
        -Wsuggest-final-methods     # Virtual methods that could be declared final or in an anonymous namespace.
        -Wsuggest-final-types       # Types with virtual methods that can be declared final or in an anonymous namespace.
        -Wsuggest-override          # Overriding virtual functions that are not marked with the override keyword.
        )
    endif ()
    if (NOT (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6))
      list(APPEND PROJECT_WARNING_FLAGS
        -Wduplicated-cond           # Warn about duplicated conditions in an if-else-if chain.
        -Wmisleading-indentation    # Warn when indentation does not reflect the block structure.
        -Wmultiple-inheritance      # Do not allow multiple inheritance.
        -Wnull-dereference          # Dereferencing a pointer may lead to undefined behavior.
        )
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7)
      list(APPEND PROJECT_WARNING_FLAGS
        -Walloca                    # Warn on any usage of alloca in the code.
        -Wduplicated-branches       # Warn about duplicated branches in if-else statements.
        )
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
      list(APPEND PROJECT_WARNING_FLAGS
        -Wextra-semi                # Redundant semicolons after in-class function definitions.
        -Wunsafe-loop-optimizations # The loop cannot be optimized because the compiler cannot assume anything.
        )
    endif ()
    if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10)
      list(APPEND PROJECT_WARNING_FLAGS
        -Warith-conversion          # Stricter implicit conversion warnings in arithmetic operations.
        -Wredundant-tags            # Redundant class-key and enum-key where it can be eliminated.
        )
    endif ()
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND PROJECT_WARNING_FLAGS
      -Wdouble-promotion            # Warn about implicit conversions from "float" to "double".
      -Wnull-dereference            # Dereferencing a pointer may lead to erroneous or undefined behavior.
      -Wno-unknown-warning-option   # Ignore unknown warning options.
      )
  endif ()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  list(APPEND PROJECT_WARNING_FLAGS
    /permissive- # Specify standards conformance mode to the compiler.
    /W4          # Enable level 4 warnings.
    /w14062      # Enumerator 'identifier' in a switch of enum 'enumeration' is not handled.
    /w14242      # The types are different, possible loss of data. The compiler makes the conversion.
    /w14254      # A larger bit field was assigned to a smaller bit field, possible loss of data.
    /w14263      # Member function does not override any base class virtual member function.
    /w14265      # 'class': class has virtual functions, but destructor is not virtual.
    /w14287      # 'operator': unsigned/negative constant mismatch.
    /w14289      # Loop control variable is used outside the for-loop scope.
    /w14296      # 'operator': expression is always false.
    /w14311      # 'variable' : pointer truncation from 'type' to 'type'.
    /w14545      # Expression before comma evaluates to a function which is missing an argument list.
    /w14546      # Function call before comma missing argument list.
    /w14547      # Operator before comma has no effect; expected operator with side-effect.
    /w14549      # Operator before comma has no effect; did you intend 'operator2'?
    /w14555      # Expression has no effect; expected expression with side-effect.
    /w14619      # #pragma warning: there is no warning number 'number'.
    /w14640      # 'instance': construction of local static object is not thread-safe.
    /w14826      # Conversion from 'type1' to 'type2' is sign-extended.
    /w14905      # Wide string literal cast to 'LPSTR'.
    /w14906      # String literal cast to 'LPWSTR'.
    /w14928      # Illegal copy-initialization; applied more than one user-defined conversion.
    )
endif ()

# Enable warnings as errors.
if (WARNINGS_AS_ERRORS)
  if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND PROJECT_WARNING_FLAGS -Werror)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    list(APPEND PROJECT_WARNING_FLAGS /WX)
  endif ()
endif ()