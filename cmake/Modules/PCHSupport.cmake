if (CMAKE_VERSION VERSION_LESS "3.16.0")
	if (MSVC)
	  set(COTIRE_PCH_MEMORY_SCALING_FACTOR 500)
	endif()

	include(cotire)

	function(GEN_CXX_PCH TARGET_LIST PCH_HEADERS)
	  message(STATUS "Generate PCH for Project ${TARGET_LIST}")
	  # iterate through targets
	  foreach(TARGET_HEADER ${TARGET_LIST})
		# unity builds disabled
		set_target_properties(${TARGET_HEADER} PROPERTIES COTIRE_ADD_UNITY_BUILD OFF)

		# prefix header
		set_target_properties(${TARGET_HEADER} PROPERTIES COTIRE_CXX_PREFIX_HEADER_INIT ${PCH_HEADERS})
		
		# Workaround for cotire bug: https://github.com/sakra/cotire/issues/138
		set_property(TARGET ${TARGET_HEADER} PROPERTY CXX_STANDARD 17)
	  endforeach()
	  cotire(${TARGET_LIST})
	endfunction(GEN_CXX_PCH)
else()
	function(GEN_CXX_PCH TARGET_LIST PCH_HEADERS)
		message(STATUS "Generate PCH for Project ${TARGET_LIST}")
		foreach(TARGET_HEADER ${TARGET_LIST})
			target_precompile_headers(${TARGET_HEADER} PRIVATE ${PCH_HEADERS})
		endforeach()
	endfunction(GEN_CXX_PCH)
endif()