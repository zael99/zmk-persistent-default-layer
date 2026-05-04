target_include_directories(app PRIVATE include)

if ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    if (CONFIG_ZMK_BEHAVIOR_PDF)
      target_sources(app PRIVATE src/behaviors/behavior_persistent_default_layer.c)
      target_sources(app PRIVATE src/persistent_layer.c)
    endif()
    
  zephyr_include_directories(include)
endif() # ((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
