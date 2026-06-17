# Kiki Runner game
add_executable(demo)

target_sources(demo
  PRIVATE
    main.cpp


)

target_link_libraries(demo
  PRIVATE
    kiki
)