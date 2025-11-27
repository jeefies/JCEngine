#include <iostream>

#include <SDL3/SDL.h>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

#include "jc_math.h"
#include "jc_event.h"

int createSDLWinAndGPU(SDL_Window **win, SDL_GPUDevice **gpudev, int width, int height, const char* name,
        SDL_WindowFlags winflag = 0, SDL_GPUShaderFormat gpuformat = SDL_GPU_SHADERFORMAT_SPIRV) noexcept {
    *win = SDL_CreateWindow(name, width, height, winflag);
    if (*win == nullptr) return JC_ERROR;

    *gpudev = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (*gpudev == NULL) return JC_ERROR;

    if (!SDL_ClaimWindowForGPUDevice(*gpudev, *win)) return JC_ERROR;
    return JC_SUCCESS;
}

struct GameWindow {
    SDL_Window *window;
    SDL_GPUDevice *gpudev;

    GameWindow(int width, int height, const char* name,
        SDL_WindowFlags winflag = 0, SDL_GPUShaderFormat gpuformat = SDL_GPU_SHADERFORMAT_SPIRV) {
        if (createSDLWinAndGPU(&window, &gpudev, width, height, name, winflag, gpuformat) == JC_ERROR) {
            jclog << "Failed " << SDL_GetError() << "\n";
            std::terminate();
        }
    }

    int getsize(int *w, int *h) {
        return SDL_GetWindowSize(window, w, h);
    }
    

    SDL_GPUShader* loadShader(const char *path, SDL_GPUShaderStage stage) {
        size_t codeSize;
        void* code = SDL_LoadFile(path, &codeSize);
        SDL_GPUShaderCreateInfo info = {
            .code_size = codeSize,
            .code = (Uint8*)code,
            .entrypoint = "main",
            .format = SDL_GetGPUShaderFormats(gpudev),
            .stage = stage,
        };
        SDL_GPUShader* shader = SDL_CreateGPUShader(gpudev, &info);
        if (shader == NULL) {
            jclog << "着色器创建失败！" << SDL_GetError() << std::endl;
            return nullptr;
        }
        SDL_free(code);
        return shader;
    }
};

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't init SDL: %s", SDL_GetError());
        return 1;
    }

    const int WIDTH = 1080, HEIGHT = 720;
    GameWindow gwin(WIDTH, HEIGHT, "SDL GPU Starter");

    
    // 渲染管线创建（1）目标信息
    // ----颜色目标描述
    SDL_GPUColorTargetDescription colorTargetDesc = {};
    //colorTargetDesc.blend_state = ...; // 后续可以在这里启用混合
    colorTargetDesc.format = SDL_GetGPUSwapchainTextureFormat(gwin.gpudev, gwin.window);
    // ----目标信息创建
    SDL_GPUGraphicsPipelineTargetInfo targetInfo = {};
    targetInfo.num_color_targets = 1;
    targetInfo.color_target_descriptions = &colorTargetDesc;

    // 渲染管线创建（2）着色器
    SDL_GPUShader *vertShader = gwin.loadShader("build/src/shaders/shader.vert.spv", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader *fragShader = gwin.loadShader("build/src/shaders/shader.frag.spv", SDL_GPU_SHADERSTAGE_FRAGMENT);

    // 渲染管线创建（3）顶点输入状态
    // ----顶点属性信息
    // --------0号位: 位置XYZ
    SDL_GPUVertexAttribute vertexAttributes = {
        .location = 0,
        .buffer_slot = 0,
        .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
        .offset = 0,
    };
    // ----顶点缓冲区描述
    SDL_GPUVertexBufferDescription vertexBufferDesc = {
        .slot = 0,
        .pitch = sizeof(float) * 3, // 这里填一个顶点的字节大小
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
    };
    // ----输入状态创建
    SDL_GPUVertexInputState vertexInputState = {
        .vertex_buffer_descriptions = &vertexBufferDesc,
        .num_vertex_buffers = 1,
        .vertex_attributes = &vertexAttributes,
        .num_vertex_attributes = 1,
    };

    // 渲染管线创建（4）最终创建
    
    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .vertex_shader = vertShader,
        .fragment_shader = fragShader,
        .vertex_input_state = vertexInputState,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .target_info = targetInfo,
    };

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gwin.gpudev, &pipelineCreateInfo);
    if (pipeline == NULL) {
        std::cout << "渲染管线创建失败！" << std::endl;
        return -1;
    }

    SDL_ReleaseGPUShader(gwin.gpudev, vertShader);
    SDL_ReleaseGPUShader(gwin.gpudev, fragShader);

    float vertices[] = {
        //X     Y     Z
        -0.5f, -0.5f, 0.0f,  //Vertex1
         0.5f, -0.5f, 0.0f,  //Vertex2
         0.0f,  0.5f, 0.0f,  //Vertex3
    };

    // 创建顶点缓冲区对象
    SDL_GPUBufferCreateInfo vboCreateInfo = {};
    vboCreateInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vboCreateInfo.size = sizeof(vertices);
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(gwin.gpudev, &vboCreateInfo);


    // 顶点缓冲区数据上传（1）创建传输缓冲区
    SDL_GPUTransferBufferCreateInfo tboCreateInfo = {};
    tboCreateInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tboCreateInfo.size = sizeof(vertices);
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gwin.gpudev, &tboCreateInfo);

    // 顶点缓冲区数据上传（2）顶点数据复制到传输缓冲区
    void* transferData = SDL_MapGPUTransferBuffer(gwin.gpudev, transferBuffer, false);
    memcpy(transferData, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(gwin.gpudev, transferBuffer);

    // 顶点缓冲区数据上传（3）创建“上传用”命令缓冲区，开始复制过程
    SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(gwin.gpudev);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);

    // 顶点缓冲区数据上传（4）将传输缓冲区的内容上传到顶点缓冲区
    SDL_GPUTransferBufferLocation location = {};
    location.transfer_buffer = transferBuffer;
    location.offset = 0;
    SDL_GPUBufferRegion region = {};
    region.buffer = vertexBuffer;
    region.offset = 0;
    region.size = sizeof(vertices);
    SDL_UploadToGPUBuffer(copyPass, &location, &region, false);

    // 顶点缓冲区数据上传（5）结束复制过程，提交“上传用”命令缓冲区，释放传输缓冲区
    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(uploadCommandBuffer);
    SDL_ReleaseGPUTransferBuffer(gwin.gpudev, transferBuffer);
    jclog << "Upload OK\n";

    auto frame = [&](void *_gwin) {
        jclog << "Framing...\n";
        GameWindow *gwin = (GameWindow *)_gwin;
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gwin->gpudev);
        if (commandBuffer == nullptr) {
            jclog << "命令缓冲区创建失败！" << std::endl;
            return 0;
        }

        SDL_GPUTexture* swapchainTexture;
        unsigned int swapchainTextureWidth, swapchainTextureHeight;
        if (!SDL_AcquireGPUSwapchainTexture(commandBuffer, gwin->window, &swapchainTexture, &swapchainTextureWidth, &swapchainTextureHeight)) {
            jclog << "交换链纹理获取失败！" << std::endl;
            return 0;
        }



        if (swapchainTexture != NULL) {
            // 接下来对“交换链纹理”进行渲染操作
            // 定义颜色目标
            SDL_GPUColorTargetInfo colorTargetInfo = {
                .texture = swapchainTexture,
                .clear_color = {1.0f, 0.0f, 0.0f, 1.0f},
                .load_op = SDL_GPU_LOADOP_CLEAR,
                .store_op = SDL_GPU_STOREOP_STORE,
            };
            SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, NULL);
            // 三角形绘制: 绑定渲染管线
            SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

            // 三角形绘制: 绑定顶点缓冲区
            SDL_GPUBufferBinding vboBinding = {};
            vboBinding.buffer = vertexBuffer;
            vboBinding.offset = 0;
            SDL_BindGPUVertexBuffers(renderPass, 0, &vboBinding, 1);

            // 三角形绘制: 绘制图元
            SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);

            // 结束渲染过程
            SDL_EndGPURenderPass(renderPass);
        }

        SDL_SubmitGPUCommandBuffer(commandBuffer);
        return JC_SUCCESS;
    };

    JCEventTimer<128> timer(5);
    timer.registerEvent(0, 1000 / 25, frame, (void *)&gwin);
    
    bool close = false;
    timer.registerEvent(2000, 0, [&close](void *ptr) {
        close = true;
        jclog << "close set\n";
        return JC_SUCCESS;
    }, nullptr);

    while (!close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            // 点击退出按钮时退出循环
            if (event.type == SDL_EVENT_QUIT) {
                close = true;
            }
        }
    }
}