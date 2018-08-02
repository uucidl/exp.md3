typedef struct kaon_canvas_CanvasCommandQueue kaon_canvas_CanvasCommandQueue;
typedef struct noir_App noir_App;
typedef struct kaon_canvas_GPUImageHandle kaon_canvas_GPUImageHandle;
typedef struct codecs_Image codecs_Image;
typedef struct kaon_canvas_GPUFontHandle kaon_canvas_GPUFontHandle;

void
gpu_flush_commands(noir_App* noir_app, kaon_canvas_CanvasCommandQueue* queue);

kaon_canvas_GPUImageHandle
gpu_load_image(codecs_Image const* image);

void
gpu_unload_image(kaon_canvas_GPUImageHandle handle);

kaon_canvas_GPUFontHandle
gpu_load_font_by_filepath(char const* path);

void
gpu_unload_font(kaon_canvas_GPUFontHandle handle);

int
gpu_text_width(kaon_canvas_GPUFontHandle handle, int font_height, char const* str, char const* end);