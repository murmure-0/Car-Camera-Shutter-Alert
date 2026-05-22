#ifndef YOLOV5_RUNNER_H
#define YOLOV5_RUNNER_H

#include <cstdint>
#include "rknn_api.h"

#define YOLOV5_OBJ_NAME_MAX 64
#define YOLOV5_OBJ_MAX 128
#define YOLOV5_OBJ_CLASS_NUM 80

struct YoloV5Rect {
    int left;
    int top;
    int right;
    int bottom;
};

struct YoloV5Detection {
    YoloV5Rect box;
    float prop;
    int cls_id;
};

struct YoloV5Detections {
    int id;
    int count;
    YoloV5Detection results[YOLOV5_OBJ_MAX];
};

class YoloV5Runner
{
public:
    YoloV5Runner();
    ~YoloV5Runner();

    bool init(const char *model_path, const char *label_path = nullptr);
    void deinit();
    bool isReady() const;
    const char *className(int cls_id) const;

    uint8_t *inputBuffer();
    int inputWidth() const;
    int inputHeight() const;
    int inputChannels() const;

    int run(YoloV5Detections *results);

private:
    rknn_context m_rknn = 0;
    rknn_input_output_num m_ioNum;
    rknn_tensor_attr *m_inputAttrs = nullptr;
    rknn_tensor_attr *m_outputAttrs = nullptr;
    rknn_tensor_mem *m_inputMems[1] = {nullptr};
    rknn_tensor_mem *m_outputMems[3] = {nullptr};
    int m_modelWidth = 0;
    int m_modelHeight = 0;
    int m_modelChannel = 0;
    bool m_isQuant = false;
    char *m_labels[YOLOV5_OBJ_CLASS_NUM] = {nullptr};
    bool m_ready = false;
};

#endif
