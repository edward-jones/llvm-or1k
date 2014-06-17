include(ExternalProject)

# Discover the projects that use CMake in the subdirectories.
# Note that explicit cmake invocation is required every time a new project is
# added or removed.
file(GLOB entries *)
foreach(entry ${entries})
  if(IS_DIRECTORY ${entry} AND EXISTS ${entry}/CMakeLists.txt)
    if((NOT ${entry} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/compiler-rt) AND
       (NOT ${entry} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/dragonegg) AND
       (NOT ${entry} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/libcxx) AND
       (NOT ${entry} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR}/cruntime))
      add_subdirectory(${entry})
    endif()
  endif()
endforeach(entry)

# Also add in libc++ and compiler-rt trees if present (and we have
# a sufficiently recent version of CMake where required).
if(${LLVM_BUILD_RUNTIME})
  # MSVC isn't quite working with libc++ yet, disable it until issues are
  # fixed.
  if(NOT MSVC)
    add_llvm_external_project(libcxx)
  endif()
  if(NOT LLVM_BUILD_EXTERNAL_COMPILER_RT)
    add_llvm_external_project(compiler-rt)
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/cruntime)
    set(CRUNTIME_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/projects/cruntime-install)
    file(MAKE_DIRECTORY ${CRUNTIME_INSTALL_PREFIX})

    # FIXME: Generic is hardcoded, introduce an LLVM configuration variable?
    ExternalProject_Add(cruntime
      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cruntime/
      CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                 -DLLVM_CONFIG_PATH=${LLVM_RUNTIME_OUTPUT_INTDIR}/llvm-config
		 -DCMAKE_INSTALL_PREFIX=${CRUNTIME_INSTALL_PREFIX}
                 -DCMAKE_SYSTEM_NAME=Generic
      STEP_TARGETS configure build install
      )
    # FIXME: Here the clang version is hardcoded, we should move this in
    # tools/clang/runtime or make the install prefix independent from clang
    set(CRUNTIME_INSTALL_PATH /lib${LLVM_LIBDIR_SUFFIX}/clang/3.5.0/)
    install(DIRECTORY ${CRUNTIME_INSTALL_PREFIX}/
      DESTINATION ${CMAKE_INSTALL_PREFIX}${CRUNTIME_INSTALL_PATH})

    add_dependencies(cruntime llvm-config clang)

    # Add a custom step to always re-configure cruntime (in case some of its
    # sources have changed).
    ExternalProject_Add_Step(cruntime force-reconfigure
      DEPENDERS configure
      ALWAYS 1
      )

    ExternalProject_Add_Step(cruntime clobber
      COMMAND ${CMAKE_COMMAND} -E remove_directory <BINARY_DIR>
      COMMAND ${CMAKE_COMMAND} -E make_directory <BINARY_DIR>
      COMMENT "Clobberring cruntime build directory..."
      DEPENDERS configure
      DEPENDS ${LLVM_RUNTIME_OUTPUT_INTDIR}/clang
      )
  endif()

endif()

add_llvm_external_project(dragonegg)
