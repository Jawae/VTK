# Tests that don't produce images
unset(tests)
set(tests
  TestGlobFileNames
  TestSortFileNames
)

foreach( tfile ${tests})
  if(VTK_DATA_ROOT)
    add_test(NAME ${vtk-module}Tcl-${tfile} COMMAND ${VTK_TCL_EXE}
      ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.tcl
      -D ${VTK_DATA_ROOT}
   )
  endif()
endforeach()
