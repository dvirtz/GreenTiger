add_library(testMain STATIC Test.h TestMain.cpp checkedCompile.cpp)
target_link_libraries(testMain PUBLIC 
  Catch2::Catch
  Microsoft.GSL::GSL
  Boost::filesystem
  Boost::system 
  includeHeaders
  Chapter09
)

function(add_chapter09_test name)
  add_executable(${name} ${name}.cpp)

  target_link_libraries(${name}
    PRIVATE
      testMain
  )
  
  parse_unit_tests(${name} --timeout 10 ARCHS m68k x64)
endfunction()

add_chapter09_test(compileFiles)
add_chapter09_test(lvalue)
add_chapter09_test(for)
add_chapter09_test(integer)
add_chapter09_test(comparison)
add_chapter09_test(typeDeclarations)
add_chapter09_test(boolean)
add_chapter09_test(array)
add_chapter09_test(nil)
add_chapter09_test(functionCall)
add_chapter09_test(string)
add_chapter09_test(assignment)
add_chapter09_test(arithmetic)
add_chapter09_test(ifThen)
add_chapter09_test(while)
add_chapter09_test(library)
add_chapter09_test(record)
add_chapter09_test(sequence)
add_chapter09_test(functionDeclarations)
add_chapter09_test(break)