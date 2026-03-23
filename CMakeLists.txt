cmake_minimum_required(VERSION 3.22)

project(mc_rtc-imgui LANGUAGES CXX VERSION 1.0.0)
include(cmake/get-jrl-cmakemodules.cmake)
jrl_configure_defaults()

jrl_find_package(imgui REQUIRED)
jrl_find_package(implot REQUIRED)
jrl_find_package(mc_rtc 2.14 REQUIRED)

set(mc_rtc-imgui-SRC
  src/widgets/Schema.cpp
  src/widgets/form/schema.cpp
  src/widgets/form/widgets.cpp
  src/Category.cpp
  src/Client.cpp
  src/Plot.cpp
)

set(mc_rtc-imgui-HDR
include/mc_rtc-imgui/Category.h
include/mc_rtc-imgui/Widget.h
include/mc_rtc-imgui/Client.h
include/mc_rtc-imgui/InteractiveMarker.h
include/mc_rtc-imgui/widgets/details/SingleInput.h
include/mc_rtc-imgui/widgets/form/widgets.h
include/mc_rtc-imgui/widgets/form/schema.h
include/mc_rtc-imgui/widgets/Widget.h
include/mc_rtc-imgui/widgets/IntegerInput.h
include/mc_rtc-imgui/widgets/Button.h
include/mc_rtc-imgui/widgets/Schema.h
include/mc_rtc-imgui/widgets/ArrayLabel.h
include/mc_rtc-imgui/widgets/Table.h
include/mc_rtc-imgui/widgets/Label.h
include/mc_rtc-imgui/widgets/DataComboInput.h
include/mc_rtc-imgui/widgets/ComboInput.h
include/mc_rtc-imgui/widgets/StringInput.h
include/mc_rtc-imgui/widgets/IndentedSeparator.h
include/mc_rtc-imgui/widgets/NumberSlider.h
include/mc_rtc-imgui/widgets/Checkbox.h
include/mc_rtc-imgui/widgets/ArrayInput.h
include/mc_rtc-imgui/widgets/Form.h
include/mc_rtc-imgui/widgets/NumberInput.h
include/mc_rtc-imgui/Plot.h


  include/mc_rtc-imgui/widgets/NumberInput.h
  include/mc_rtc-imgui/widgets/Table.h
  include/mc_rtc-imgui/widgets/Label.h
  include/mc_rtc-imgui/widgets/details/SingleInput.h
  include/mc_rtc-imgui/widgets/NumberSlider.h
  include/mc_rtc-imgui/widgets/IntegerInput.h
  include/mc_rtc-imgui/widgets/ComboInput.h
  include/mc_rtc-imgui/widgets/ArrayLabel.h
  include/mc_rtc-imgui/widgets/StringInput.h
  include/mc_rtc-imgui/widgets/ArrayInput.h
  include/mc_rtc-imgui/widgets/Button.h
  include/mc_rtc-imgui/widgets/Checkbox.h
  include/mc_rtc-imgui/widgets/Form.h
  include/mc_rtc-imgui/widgets/DataComboInput.h
  include/mc_rtc-imgui/widgets/Schema.h
  include/mc_rtc-imgui/widgets/form/schema.h
  include/mc_rtc-imgui/widgets/form/widgets.h
  include/mc_rtc-imgui/Widget.h
  include/mc_rtc-imgui/Category.h
  include/mc_rtc-imgui/Client.h
  include/mc_rtc-imgui/Plot.h
)

add_library(mc_rtc-imgui SHARED ${mc_rtc-imgui-SRC})
jrl_target_headers(mc_rtc-imgui PUBLIC HEADERS ${mc_rtc-imgui-HDR} BASE_DIRS include)
target_include_directories(mc_rtc-imgui
  PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(mc_rtc-imgui
                      PUBLIC
                      imgui::imgui
                      implot::implot
                      mc_rtc::mc_control_client)
jrl_add_export_component(NAME mc_rtc-imgui TARGETS mc_rtc-imgui)
jrl_export_package()
#jrl_print_dependencies_summary()
jrl_print_options_summary()
