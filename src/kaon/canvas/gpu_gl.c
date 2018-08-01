#include "nanovg.h"

#include "glad/glad.h"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

#include <assert.h>

void gpu_flush_commands(noir_App* noir_app, kaon_canvas_CanvasCommandQueue* queue)
{
    typedef kaon_canvas_CanvasCommandDrawRect CommandDrawRect;
    typedef kaon_canvas_CanvasCommand Command;


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

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    nvgBeginFrame(vg, noir_app->window.size.x, noir_app->window.size.y, noir_app->display.dpi / 96.0);
    for (int i = 0; i<queue->num_commands; i++) {
        Command const* command = &queue->commands[i];
        switch (command->kind) {
            case KAON_CANVAS_COMMAND_DRAW_RECT: {
                CommandDrawRect const *draw_rect = &command->draw_rect;
                nvgBeginPath(vg);
                nvgRect(vg, draw_rect->l, draw_rect->t, draw_rect->w, draw_rect->h);
                if (draw_rect->flags & KAON_CANVAS_RECT_FLAGS_FILLED) {
                    nvgFillColor(vg, nvgRGBA(255, draw_rect->fill_color.r, draw_rect->fill_color.g, draw_rect->fill_color.b));
                    nvgFill(vg);
                }
                if (draw_rect->flags & KAON_CANVAS_RECT_FLAGS_STROKED) {
                    nvgStrokeColor(vg, nvgRGBA(255, draw_rect->stroke_color.r, draw_rect->stroke_color.g, draw_rect->stroke_color.b));
                    nvgStroke(vg);
                }
            } break;

            case KAON_CANVAS_COMMAND_DRAW_SEGMENT: {
                nvgBeginPath(vg);
                nvgMoveTo(vg, command->draw_segment.x0, command->draw_segment.y0);
                nvgLineTo(vg, command->draw_segment.x1, command->draw_segment.y1);
                nvgStrokeColor(vg, nvgRGBA(255, command->draw_segment.color.r, command->draw_segment.color.g, command->draw_segment.color.b));
                nvgStrokeWidth(vg, command->draw_segment.width);
                nvgStroke(vg);
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

#pragma comment(lib, "Opengl32.lib")
#include "deps/nanovg/src/nanovg.c"
#include "deps/GL3/src/glad.c"
