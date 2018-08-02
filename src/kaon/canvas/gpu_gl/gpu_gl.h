typedef struct kaon_canvas_CanvasCommandQueue kaon_canvas_CanvasCommandQueue;
typedef struct noir_App noir_App;
typedef struct kaon_canvas_gpu_gl_ImageHandle kaon_canvas_gpu_gl_ImageHandle;
typedef struct codecs_Image codecs_Image;
typedef struct kaon_canvas_gpu_gl_FontHandle kaon_canvas_gpu_gl_FontHandle;

void
gpu_flush_commands(noir_App* noir_app, kaon_canvas_CanvasCommandQueue* queue);

kaon_canvas_gpu_gl_ImageHandle
gpu_load_image(codecs_Image const* image);

void
gpu_unload_image(kaon_canvas_gpu_gl_ImageHandle handle);

kaon_canvas_gpu_gl_FontHandle
gpu_load_font_by_filepath(char const* path);

void
gpu_unload_font(kaon_canvas_gpu_gl_FontHandle handle);

int
gpu_text_width(kaon_canvas_gpu_gl_FontHandle handle, int font_height, char const* str, char const* end);