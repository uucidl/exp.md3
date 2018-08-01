typedef struct kaon_canvas_CanvasCommandQueue kaon_canvas_CanvasCommandQueue;
typedef struct noir_App noir_App;
typedef struct kaon_canvas_GPUImageHandle kaon_canvas_GPUImageHandle;
typedef struct codecs_Image codecs_Image;

void
gpu_flush_commands(noir_App* noir_app, kaon_canvas_CanvasCommandQueue* queue);

kaon_canvas_GPUImageHandle
gpu_load_image(codecs_Image const* image);

void
gpu_unload_image(kaon_canvas_GPUImageHandle handle);


