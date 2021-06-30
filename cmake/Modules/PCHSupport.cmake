include(cotire)

function(GEN_CXX_PCH TARGET_HEADER_LIST PCH_HEADERS)
    foreach(TARGET_HEADER ${TARGET_HEADER_LIST})
      target_precompile_headers(${TARGET_HEADER} PRIVATE ${PCH_HEADERS})
    endforeach()
endfunction(GEN_CXX_PCH)
