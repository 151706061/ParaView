configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
  "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  COPYONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/about.html.in"
  "${CMAKE_CURRENT_BINARY_DIR}/about.html"
  @ONLY)

paraview_client_documentation(
  TARGET  paraview_doc
  XMLS    ${xmls}
          ${paraview_server_manager_files})

set(table_of_contents
  "<toc>
  <section title=\"ParaView User Manual\" ref=\"index.html\">
    <section title=\"Sources\" ref=\"Sources.html\"/>
    <section title=\"Filters\" ref=\"Filters.html\"/>
    <section title=\"Readers\" ref=\"Readers.html\"/>
    <section title=\"Writers\" ref=\"Writers.html\"/>
  </section>
</toc>")

set(paraview_documentation_qrc
  "${CMAKE_CURRENT_BINARY_DIR}/paraview_documentation.qrc")
paraview_client_generate_help(
  NAME        paraview
  OUTPUT_PATH paraview_qch
  TARGET      paraview_qhp
  OUTPUT_DIR  "${CMAKE_CURRENT_BINARY_DIR}"
  SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}"
  PATTERNS    "*.html"
              "*.css"
              "*.png"
              "*.jpg"
              "images/*.jpg"
  DEPENDS     paraview_doc
  TABLE_OF_CONTENTS "${table_of_contents}"
  RESOURCE_FILE   "${paraview_documentation_qrc}"
  RESOURCE_PREFIX "/ParaView/Documentation")

set(CMAKE_AUTORCC 1)

find_package("Qt${PARAVIEW_QT_MAJOR_VERSION}" REQUIRED QUIET COMPONENTS Core)

add_library(vtkParaViewDocumentation STATIC
  ParaViewDocumentationInitializer.cxx
  ParaViewDocumentationInitializer.h
  "${paraview_documentation_qrc}")
target_include_directories(vtkParaViewDocumentation
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_link_libraries(vtkParaViewDocumentation
  PRIVATE
    "Qt${PARAVIEW_QT_MAJOR_VERSION}::Core")

set_property(GLOBAL PROPERTY paraview_qch_build_file "${paraview_qch}")
set_property(GLOBAL PROPERTY paraview_qch_install_file "${CMAKE_INSTALL_DOCDIR}/paraview.qch")

install(
  FILES       "${paraview_qch}"
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  COMPONENT   doc)
