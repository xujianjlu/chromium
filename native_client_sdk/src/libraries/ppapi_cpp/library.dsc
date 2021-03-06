{
  'TOOLS': ['win'],
  'SEARCH' : [
      '../../../../ppapi/cpp',
      '../../../../ppapi/utility',
      '../../../../ppapi/utility/graphics',
      '../../../../ppapi/utility/threading',
      '../../../../ppapi/utility/websocket',
  ],
  'TARGETS': [
    {
      'NAME' : 'ppapi_cpp',
      'TYPE' : 'lib',
      'SOURCES' : [
          'module_embedder.h',
          'ppp_entrypoints.cc',
          'array_output.cc',
          'array_output.h',
          'audio.cc',
          'audio.h',
          'audio_config.cc',
          'audio_config.h',
          'completion_callback.h',
          'core.cc',
          'core.h',
          'file_io.cc',
          'file_io.h',
          'file_ref.cc',
          'file_ref.h',
          'file_system.cc',
          'file_system.h',
          'fullscreen.cc',
          'fullscreen.h',
          'graphics_2d.cc',
          'graphics_2d.h',
          'graphics_3d.cc',
          'graphics_3d.h',
          'graphics_3d_client.cc',
          'graphics_3d_client.h',
          'image_data.cc',
          'image_data.h',
          'input_event.cc',
          'input_event.h',
          'instance.cc',
          'instance.h',
          'instance_handle.cc',
          'instance_handle.h',
          'lock.cc',
          'lock.h',
          'logging.h',
          'module.cc',
          'module.h',
          'module_impl.h',
          'mouse_cursor.cc',
          'mouse_cursor.h',
          'mouse_lock.cc',
          'mouse_lock.h',
          'output_traits.h',
          'point.h',
          'rect.cc',
          'rect.h',
          'resource.cc',
          'resource.h',
          'size.h',
          'url_loader.cc',
          'url_loader.h',
          'url_request_info.cc',
          'url_request_info.h',
          'url_response_info.cc',
          'url_response_info.h',
          'var.cc',
          'var.h',
          'var_array_buffer.cc',
          'var_array_buffer.h',
          'view.cc',
          'view.h',
          'websocket.cc',
          'websocket.h',


          # Utility sources.
          'paint_aggregator.cc',
          'paint_manager.cc',
          'simple_thread.cc',
          'websocket_api.cc',
      ],
    }
  ],
  'DEST': 'src',
  'NAME': 'ppapi_cpp',
}

