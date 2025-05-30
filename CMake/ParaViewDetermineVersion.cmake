# SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
# SPDX-License-Identifier: BSD-3-Clause

# Used to determine the version for ParaView source using "git describe", if git
# is found. On success sets following variables in caller's scope:
#   ${var_prefix}_VERSION
#   ${var_prefix}_VERSION_MAJOR
#   ${var_prefix}_VERSION_MINOR
#   ${var_prefix}_VERSION_PATCH
#   ${var_prefix}_VERSION_PATCH_EXTRA
#   ${var_prefix}_VERSION_FULL
#   ${var_prefix}_VERSION_IS_RELEASE is true, if patch-extra is empty.
#
# If git is not found, or git describe cannot be run successfully, then these
# variables are left unchanged and status message is printed.
#
# Arguments are:
#   source_dir : Source directory
#   git_command : git executable
#   var_prefix : prefix for variables e.g. "PARAVIEW".
function(determine_version source_dir git_command var_prefix)
  if ("$Format:$" STREQUAL "")
    # We are in an exported tarball and should use the shipped version
    # information. Just return here to avoid the warning message at the end of
    # this function.
    return ()
  elseif (NOT PARAVIEW_GIT_DESCRIBE AND
          EXISTS ${git_command} AND
          EXISTS ${source_dir}/.git)
    execute_process(
      COMMAND ${git_command} describe
      WORKING_DIRECTORY ${source_dir}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE output
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE)
    if (NOT result EQUAL 0)
      # git describe failed (bad return code).
      set(output "")
    endif()
  else ()
    # note, output may be set to empty if PARAVIEW_GIT_DESCRIBE is not defined.
    set(output "${PARAVIEW_GIT_DESCRIBE}")
  endif()

  unset(tmp_VERSION)
  extract_version_components(${source_dir} "${output}" tmp)
  if(DEFINED tmp_VERSION)
    if (${var_prefix}_VERSION_PATCH GREATER "20200101")
      if (NOT tmp_VERSION_MAJOR STREQUAL ${var_prefix}_VERSION_MAJOR OR
          NOT tmp_VERSION_MINOR STREQUAL ${var_prefix}_VERSION_MINOR)
        message(WARNING
          "Version from git (${tmp_VERSION}) disagrees with hard coded version (${${var_prefix}_VERSION}). Either update the git tags or version.txt.")
        if (NOT "$ENV{CI}" STREQUAL "")
          message(WARNING
            "Please update your fork tags, using this command: `git fetch origin --tags && git push gitlab --tags`.")
        endif ()
      endif ()
      # git describe rely on tags that do not have access to the patch version, correct the ${var_prefix}_VERSION accordingly
      if (${var_prefix}_VERSION_PATCH GREATER tmp_VERSION_PATCH)
        set(tmp_VERSION_PATCH "${${var_prefix}_VERSION_PATCH}")
        set(tmp_VERSION "${tmp_VERSION_MAJOR}.${tmp_VERSION_MINOR}.${tmp_VERSION_PATCH}")
      endif ()
    elseif (NOT "${tmp_VERSION}" STREQUAL "${${var_prefix}_VERSION}")
      message(WARNING
        "Version from git (${tmp_VERSION}) disagrees with hard coded version (${${var_prefix}_VERSION}). Either update the git tags or version.txt.")
      if (NOT "$ENV{CI}" STREQUAL "")
        message(WARNING
          "Please update your fork tags, using this command: `git fetch origin --tags && git push gitlab --tags`.")
      endif ()
    endif()
    foreach(suffix BRANCH VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH
                   VERSION_PATCH_EXTRA VERSION_FULL VERSION_IS_RELEASE)
      set(${var_prefix}_${suffix} ${tmp_${suffix}} PARENT_SCOPE)
    endforeach()
  else()
    message(STATUS
      "Could not use git to determine source version, using version ${${var_prefix}_VERSION_FULL}")
  endif()
endfunction()

# Extracts components from a version string. See determine_version() for usage.
function(extract_version_components source_dir version_string var_prefix)
  string(REGEX MATCH "^v?(([0-9]+)\\.([0-9]+)\\.([0-9]+)-?(.*))$"
    version_matches "${version_string}")
  if(CMAKE_MATCH_0)
    # note, we don't use CMAKE_MATCH_0 for `full` since it may or may not have
    # the `v` prefix.
    set(full ${CMAKE_MATCH_1})
    set(major ${CMAKE_MATCH_2})
    set(minor ${CMAKE_MATCH_3})
    set(patch ${CMAKE_MATCH_4})
    set(patch_extra ${CMAKE_MATCH_5})
    set(branch "")
    if (patch_extra)
      if (NOT source_dir)
        set(source_dir "${ParaView_SOURCE_DIR}")
      endif ()
      execute_process(
        COMMAND ${git_command}
                name-rev
                --name-only
                --no-undefined          # error if these names don't work
                --refs=refs/tags/*      # tags
                --refs=refs/heads/*     # branches
                --refs=refs/pipelines/* # CI
                HEAD
        WORKING_DIRECTORY ${source_dir}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)
      if (result EQUAL 0)
        # The branch name.
        set(branch "${output}")
      endif()
    endif ()

    set(${var_prefix}_BRANCH "${branch}" PARENT_SCOPE)
    set(${var_prefix}_VERSION "${major}.${minor}" PARENT_SCOPE)
    set(${var_prefix}_VERSION_MAJOR ${major} PARENT_SCOPE)
    set(${var_prefix}_VERSION_MINOR ${minor} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH ${patch} PARENT_SCOPE)
    set(${var_prefix}_VERSION_PATCH_EXTRA ${patch_extra} PARENT_SCOPE)
    set(${var_prefix}_VERSION_FULL ${full} PARENT_SCOPE)
    if("${major}.${minor}.${patch}" VERSION_EQUAL "${full}" AND
       # Date-based patch versions are never releases.
       patch LESS "20000101")
      set(${var_prefix}_VERSION_IS_RELEASE TRUE PARENT_SCOPE)
    else()
      set(${var_prefix}_VERSION_IS_RELEASE FALSE PARENT_SCOPE)
    endif()
  endif()
endfunction()
