cmake_minimum_required(VERSION 2.8.12.2)

project(miniservo)

include(FindPkgConfig)

pkg_check_modules (GTK3 gtk+-3.0)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/cef3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${GTK3_INCLUDE_DIRS})

add_definitions(-DUSING_CEF_SHARED=1)

set(C_COMPILER_FLAGS_DEBUG      "-g -O0")
set(C_COMPILER_FLAGS_RELEASE    "-O3")
set(C_COMPILER_FLAGS            "-pthread")
set(CXX_COMPILER_FLAGS          "-std=c++11 -pthread")

set(CMAKE_C_FLAGS                     "${COMPILER_FLAGS} ${C_COMPILER_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG               "${COMPILER_FLAGS_DEBUG} ${C_COMPILER_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE             "${COMPILER_FLAGS_RELEASE} ${C_COMPILER_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS                   "${COMPILER_FLAGS} ${CXX_COMPILER_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG             "${COMPILER_FLAGS_DEBUG} ${CXX_COMPILER_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE           "${COMPILER_FLAGS_RELEASE} ${CXX_COMPILER_FLAGS_RELEASE}")

set(MINISERVO_SRCS
    src/main.cpp
    src/tegtkgl.c
    src/libcef_dll/base/cef_atomicops_x86_gcc.cc
    src/libcef_dll/base/cef_bind_helpers.cc
    src/libcef_dll/base/cef_callback_helpers.cc
    src/libcef_dll/base/cef_callback_internal.cc
    src/libcef_dll/base/cef_lock.cc
    src/libcef_dll/base/cef_lock_impl.cc
    src/libcef_dll/base/cef_logging.cc
    src/libcef_dll/base/cef_ref_counted.cc
    src/libcef_dll/base/cef_string16.cc
    src/libcef_dll/base/cef_thread_checker_impl.cc
    src/libcef_dll/base/cef_thread_collision_warner.cc
    src/libcef_dll/base/cef_weak_ptr.cc
    src/libcef_dll/wrapper/cef_byte_read_handler.cc
    src/libcef_dll/wrapper/cef_closure_task.cc
    src/libcef_dll/wrapper/cef_message_router.cc
    src/libcef_dll/wrapper/cef_stream_resource_handler.cc
    src/libcef_dll/wrapper/cef_xml_object.cc
    src/libcef_dll/wrapper/cef_zip_archive.cc
    src/libcef_dll/wrapper/libcef_dll_wrapper2.cc
    src/libcef_dll/wrapper/libcef_dll_wrapper.cc
    src/libcef_dll/ctocpp/allow_certificate_error_callback_ctocpp.cc
    src/libcef_dll/ctocpp/auth_callback_ctocpp.cc
    src/libcef_dll/ctocpp/before_download_callback_ctocpp.cc
    src/libcef_dll/ctocpp/binary_value_ctocpp.cc
    src/libcef_dll/ctocpp/browser_ctocpp.cc
    src/libcef_dll/ctocpp/browser_host_ctocpp.cc
    src/libcef_dll/ctocpp/callback_ctocpp.cc
    src/libcef_dll/ctocpp/command_line_ctocpp.cc
    src/libcef_dll/ctocpp/context_menu_params_ctocpp.cc
    src/libcef_dll/ctocpp/cookie_manager_ctocpp.cc
    src/libcef_dll/ctocpp/dictionary_value_ctocpp.cc
    src/libcef_dll/ctocpp/domdocument_ctocpp.cc
    src/libcef_dll/ctocpp/domnode_ctocpp.cc
    src/libcef_dll/ctocpp/download_item_callback_ctocpp.cc
    src/libcef_dll/ctocpp/download_item_ctocpp.cc
    src/libcef_dll/ctocpp/drag_data_ctocpp.cc
    src/libcef_dll/ctocpp/file_dialog_callback_ctocpp.cc
    src/libcef_dll/ctocpp/frame_ctocpp.cc
    src/libcef_dll/ctocpp/geolocation_callback_ctocpp.cc
    src/libcef_dll/ctocpp/jsdialog_callback_ctocpp.cc
    src/libcef_dll/ctocpp/list_value_ctocpp.cc
    src/libcef_dll/ctocpp/menu_model_ctocpp.cc
    src/libcef_dll/ctocpp/post_data_ctocpp.cc
    src/libcef_dll/ctocpp/post_data_element_ctocpp.cc
    src/libcef_dll/ctocpp/print_dialog_callback_ctocpp.cc
    src/libcef_dll/ctocpp/print_job_callback_ctocpp.cc
    src/libcef_dll/ctocpp/print_settings_ctocpp.cc
    src/libcef_dll/ctocpp/process_message_ctocpp.cc
    src/libcef_dll/ctocpp/quota_callback_ctocpp.cc
    src/libcef_dll/ctocpp/request_context_ctocpp.cc
    src/libcef_dll/ctocpp/request_ctocpp.cc
    src/libcef_dll/ctocpp/response_ctocpp.cc
    src/libcef_dll/ctocpp/scheme_registrar_ctocpp.cc
    src/libcef_dll/ctocpp/stream_reader_ctocpp.cc
    src/libcef_dll/ctocpp/stream_writer_ctocpp.cc
    src/libcef_dll/ctocpp/task_runner_ctocpp.cc
    src/libcef_dll/ctocpp/urlrequest_ctocpp.cc
    src/libcef_dll/ctocpp/v8context_ctocpp.cc
    src/libcef_dll/ctocpp/v8exception_ctocpp.cc
    src/libcef_dll/ctocpp/v8stack_frame_ctocpp.cc
    src/libcef_dll/ctocpp/v8stack_trace_ctocpp.cc
    src/libcef_dll/ctocpp/v8value_ctocpp.cc
    src/libcef_dll/ctocpp/web_plugin_info_ctocpp.cc
    src/libcef_dll/ctocpp/xml_reader_ctocpp.cc
    src/libcef_dll/ctocpp/zip_reader_ctocpp.cc
    src/libcef_dll/cpptoc/app_cpptoc.cc
    src/libcef_dll/cpptoc/browser_process_handler_cpptoc.cc
    src/libcef_dll/cpptoc/client_cpptoc.cc
    src/libcef_dll/cpptoc/completion_callback_cpptoc.cc
    src/libcef_dll/cpptoc/context_menu_handler_cpptoc.cc
    src/libcef_dll/cpptoc/cookie_visitor_cpptoc.cc
    src/libcef_dll/cpptoc/dialog_handler_cpptoc.cc
    src/libcef_dll/cpptoc/display_handler_cpptoc.cc
    src/libcef_dll/cpptoc/domvisitor_cpptoc.cc
    src/libcef_dll/cpptoc/download_handler_cpptoc.cc
    src/libcef_dll/cpptoc/drag_handler_cpptoc.cc
    src/libcef_dll/cpptoc/end_tracing_callback_cpptoc.cc
    src/libcef_dll/cpptoc/focus_handler_cpptoc.cc
    src/libcef_dll/cpptoc/geolocation_handler_cpptoc.cc
    src/libcef_dll/cpptoc/get_geolocation_callback_cpptoc.cc
    src/libcef_dll/cpptoc/jsdialog_handler_cpptoc.cc
    src/libcef_dll/cpptoc/keyboard_handler_cpptoc.cc
    src/libcef_dll/cpptoc/life_span_handler_cpptoc.cc
    src/libcef_dll/cpptoc/load_handler_cpptoc.cc
    src/libcef_dll/cpptoc/print_handler_cpptoc.cc
    src/libcef_dll/cpptoc/read_handler_cpptoc.cc
    src/libcef_dll/cpptoc/render_handler_cpptoc.cc
    src/libcef_dll/cpptoc/render_process_handler_cpptoc.cc
    src/libcef_dll/cpptoc/request_context_handler_cpptoc.cc
    src/libcef_dll/cpptoc/request_handler_cpptoc.cc
    src/libcef_dll/cpptoc/resource_bundle_handler_cpptoc.cc
    src/libcef_dll/cpptoc/resource_handler_cpptoc.cc
    src/libcef_dll/cpptoc/run_file_dialog_callback_cpptoc.cc
    src/libcef_dll/cpptoc/scheme_handler_factory_cpptoc.cc
    src/libcef_dll/cpptoc/string_visitor_cpptoc.cc
    src/libcef_dll/cpptoc/task_cpptoc.cc
    src/libcef_dll/cpptoc/urlrequest_client_cpptoc.cc
    src/libcef_dll/cpptoc/v8accessor_cpptoc.cc
    src/libcef_dll/cpptoc/v8handler_cpptoc.cc
    src/libcef_dll/cpptoc/web_plugin_info_visitor_cpptoc.cc
    src/libcef_dll/cpptoc/web_plugin_unstable_callback_cpptoc.cc
    src/libcef_dll/cpptoc/write_handler_cpptoc.cc
    src/libcef_dll/transfer_util.cpp
   )

link_directories(${GTK3_LIBRARY_DIRS} ../servo/ports/cef/target)

add_executable(miniservo ${MINISERVO_SRCS})

target_link_libraries(miniservo ${GTK3_LIBRARIES} X11 GL embedding-e48887f1c3955080)
