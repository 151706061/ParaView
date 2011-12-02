# This file is used by generate_htmls_from_xmls function in ParaViewMacros.cmake
# to split a file consisting of multiple htmls into multiple files.
cmake_minimum_required(VERSION 2.8)

# INPUT VARIABLES:
# xmlpatterns       :- xmlpatterns executable.
# xml_to_xml_xsl    :- xsl file to convert SM xml to internal xml Model.
# xml_to_html_xsl   :- xsl file to conevrt the internal xml to html.
# input_xmls        :- + separated list of SM xml files
# input_gui_xmls    :- + separated list of GUI xml files used to generate the
#                        CatergoryIndex.html
# output_dir        :- Directory where all HTMLs are written out.
# output_file       :- File written out on successful completion.
#                      This file is also used to save intermediate results.

if (NOT EXISTS "${xmlpatterns}")
  message(FATAL_ERROR "No xmlpatterns executable was defined!!!")
endif()

# input_xmls is a pseudo-list. Convert it to a real CMake list.
string(REPLACE "+" ";" input_xmls "${input_xmls}")
string(REPLACE "+" ";" input_gui_xmls "${input_gui_xmls}")

set (xslt_xml)

# Generate intermediate xml using the input_xmls and XSL file.
# It also processes GUI xmls to add catergory-index sections.

foreach(xml ${input_xmls} ${input_gui_xmls})
  get_filename_component(xml_name_we  ${xml} NAME_WE)
  get_filename_component(xml_name  ${xml} NAME)

  set (temp)
  # process each XML using the XSL to generate the html.
  execute_process(
    COMMAND "${xmlpatterns}" "${xml_to_xml_xsl}" "${xml}"
    OUTPUT_VARIABLE  temp
    )
    
  # combine results.
  set (xslt_xml "${xslt_xml}\n${temp}")
endforeach()

# write the combined XML out in a single file.
set (xslt_xml "<xml>\n${xslt_xml}\n</xml>")
file (WRITE "${output_file}" "${xslt_xml}")

# process the temporary.xml using the second XSL to generate a combined html
# file.
set (multiple_htmls)
execute_process(
  COMMAND "${xmlpatterns}"
          "${xml_to_html_xsl}"
          "${output_file}"
  OUTPUT_VARIABLE multiple_htmls
  )

# if the contents of input_file contains ';', then CMake gets confused.
# So replace all semicolons with a placeholder.
string(REPLACE ";" "\\semicolon" multiple_htmls "${multiple_htmls}")

# Convert the single string into a list split at </html> markers.
string(REPLACE "</html>" "</html>;" multiple_htmls_as_list "${multiple_htmls}")

# Generate output HTML for each <html>..</html> chunk in the input.
foreach (single_html ${multiple_htmls_as_list})
  string(REGEX MATCH "<meta name=\"filename\" contents=\"([a-zA-Z0-9._-]+)\"" tmp "${single_html}")
  set (filename ${CMAKE_MATCH_1})
  if (filename)
    # process formatting strings.
    string (REPLACE "\\semicolon" ";" single_html "${single_html}")
    string (REGEX REPLACE "\\\\bold{([^}]+)}" "<b>\\1</b>" single_html "${single_html}")
    string (REGEX REPLACE "\\\\emph{([^}]+)}" "<i>\\1</i>" single_html "${single_html}")
    file (WRITE "${output_dir}/${filename}" "${single_html}")
  endif()
endforeach()
