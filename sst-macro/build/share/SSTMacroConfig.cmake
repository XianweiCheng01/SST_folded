if (NOT TARGET SST::SSTMacro)
  add_library(SST::SSTMacro IMPORTED UNKNOWN)

  set_target_properties(SST::SSTMacro PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/home/xianwei/local/sstmacro/include;/home/xianwei/local/sstmacro/include/sprockit"
    INTERFACE_COMPILE_FEATURES cxx_std_11
    INSTALL_RPATH /home/xianwei/local/sstmacro/lib
  )
  if (APPLE)
    set_target_properties(SST::SSTMacro PROPERTIES
      IMPORTED_LOCATION /home/xianwei/local/sstmacro/lib/libsstmac.dylib
    )
  else()
    set_target_properties(SST::SSTMacro PROPERTIES
      IMPORTED_LOCATION /home/xianwei/local/sstmacro/lib/libsstmac.so
    )
  endif()
endif()
