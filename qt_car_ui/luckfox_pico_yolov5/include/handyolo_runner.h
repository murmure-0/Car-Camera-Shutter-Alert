#ifndef HANDYOLO_RUNNER_H
#define HANDYOLO_RUNNER_H

#include <cstdint>
#include "rknn_api.h"

#define HANDYOLO_OBJ_MAX 128
#define HANDYOLO_CLASS_NUM 11

struct HandYoloRect {
    int left;
    int top;
    int right;
    int bottom;
};

struct HandYoloDetection {
    HandYoloRect box;
    float prop;
    int cls_id;
};

struct HandYoloDetections {
    int count;
    HandYoloDetection results[HANDYOLO_OBJ_MAX];
};

class HandYoloRunner
{
public:
    HandYoloRunner();
    ~HandYoloRunner();

    bool init(const char *model_path);
    void deinit();
    bool isReady() const;

    uint8_t *inputBuffer();
    int inputWidth() const;
    int inputHeight() const;
    int inputChannels() const;
    float inputScale() const;
    int inputZp() const;

    int run(HandYoloDetections *results);

private:
    rknn_context m_rknn = 0;
    rknn_input_output_num m_ioNum;
    rknn_tensor_attr *m_inputAttrs = nullptr;
    rknn_tensor_attr *m_outputAttrs = nullptr;
    rknn_tensor_mem *m_inputMems[1] = {nullptr};
    rknn_tensor_mem *m_outputMems[3] = {nullptr, nullptr, nullptr};
    int m_modelWidth = 0;
    int m_modelHeight = 0;
    int m_modelChannel = 0;
    bool m_isQuant = false;
    bool m_ready = false;
};

#endif
