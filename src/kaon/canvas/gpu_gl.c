#include "nanovg.h"

#include "glad/glad.h"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

#include <assert.h>

static NVGcontext* gpu_vg()
{
    // @todo @leak
    static NVGcontext* vg;
    if (!vg) {
        if (!gladLoadGL()) { 
            fprintf(stderr, "ERROR: could not initialize GL\n");
            assert(0);
            exit(1);
        }
        vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
    }
    assert(vg);
    return vg;
}

void gpu_flush_commands(noir_App* noir_app, kaon_canvas_CanvasCommandQueue* queue)
{
    typedef kaon_canvas_CanvasCommandDrawRect CommandDrawRect;
    typedef kaon_canvas_CanvasCommandDrawImage CommandDrawImage;

    typedef kaon_canvas_CanvasCommand Command;

    NVGcontext* vg = gpu_vg();

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    nvgBeginFrame(vg, noir_app->window.size.x, noir_app->window.size.y, noir_app->display.dpi / 96.0);
    for (int i = 0; i<queue->num_commands; i++) {
        Command const* command = &queue->commands[i];
        switch (command->kind) {
            case KAON_CANVAS_COMMAND_CLEAR: {
                nvgBeginPath(vg);
                nvgRect(vg, 0, 0, noir_app->window.size.x, noir_app->window.size.y);
                nvgFillColor(vg, nvgRGB(command->clear.r, command->clear.g, command->clear.b));
                nvgFill(vg);
            } break;

            case KAON_CANVAS_COMMAND_DRAW_IMAGE: {
                CommandDrawImage const *draw_image = &command->draw_image;
                int wh[2];
                nvgImageSize(vg, draw_image->image.id, &wh[0], &wh[1]);
                NVGpaint image_paint = nvgImagePattern(vg, draw_image->x, draw_image->y, wh[0], wh[1], 0.0, draw_image->image.id, 1.0f);
                nvgBeginPath(vg);
                nvgRect(vg, draw_image->x, draw_image->y, draw_image->w, draw_image->h);
                nvgFillPaint(vg, image_paint);
                nvgFill(vg);
            } break;

            case KAON_CANVAS_COMMAND_DRAW_RECT: {
                CommandDrawRect const *draw_rect = &command->draw_rect;
                nvgBeginPath(vg);
                nvgRect(vg, draw_rect->l, draw_rect->t, draw_rect->w, draw_rect->h);
                if (draw_rect->flags & KAON_CANVAS_RECT_FLAGS_FILLED) {
                    nvgFillColor(vg, nvgRGB(draw_rect->fill_color.r, draw_rect->fill_color.g, draw_rect->fill_color.b));
                    nvgFill(vg);
                }
                if (draw_rect->flags & KAON_CANVAS_RECT_FLAGS_STROKED) {
                    nvgStrokeWidth(vg, 1);
                    nvgStrokeColor(vg, nvgRGB(draw_rect->stroke_color.r, draw_rect->stroke_color.g, draw_rect->stroke_color.b));
                    nvgStroke(vg);
                }
            } break;

            case KAON_CANVAS_COMMAND_DRAW_SEGMENT: {
                nvgLineCap(vg, NVG_SQUARE);

                nvgBeginPath(vg);
                nvgMoveTo(vg, command->draw_segment.x0, command->draw_segment.y0);
                nvgLineTo(vg, command->draw_segment.x1, command->draw_segment.y1);
                nvgStrokeColor(vg, nvgRGB(command->draw_segment.color.r, command->draw_segment.color.g, command->draw_segment.color.b));
                nvgStrokeWidth(vg, command->draw_segment.width);
                nvgStroke(vg);
                nvgFillColor(vg, nvgRGB(command->draw_segment.color.r, command->draw_segment.color.g, command->draw_segment.color.b));
                nvgFill(vg);
            } break;

            case KAON_CANVAS_COMMAND_SCISSOR_RECT: {

            } break;

            default: {
                assert(false); // Unimplemented?
            }
        }
    }
    nvgEndFrame(vg);
}

kaon_canvas_GPUImageHandle
gpu_load_image(codecs_Image const* image)
{
    return (kaon_canvas_GPUImageHandle){ 
        .id = nvgCreateImageRGBA(gpu_vg(), image->width, image->height, 0, image->interleaved_channels),
    };
}

void
gpu_unload_image(kaon_canvas_GPUImageHandle handle)
{
    nvgDeleteImage(gpu_vg(), handle.id);
}

#pragma comment(lib, "Opengl32.lib")
#include "deps/nanovg/src/nanovg.c"
#include "deps/GL3/src/glad.c"
