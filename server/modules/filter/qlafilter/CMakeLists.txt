add_library(qlafilter SHARED qlafilter.cc qlalog.cc)
target_link_libraries(qlafilter maxscale-common)
set_target_properties(qlafilter PROPERTIES VERSION "1.1.1" LINK_FLAGS -Wl,-z,defs)
install_module(qlafilter core)
