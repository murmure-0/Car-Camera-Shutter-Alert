#include "yolov5_runner.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

namespace {
static const int kPropBoxSize = 5 + YOLOV5_OBJ_CLASS_NUM;
static const float kNmsThresh = 0.45f;
static const float kBoxThresh = 0.25f;
static const int kAnchor[3][6] = {{10, 13, 16, 30, 33, 23},
                                  {30, 61, 62, 45, 59, 119},
                                  {116, 90, 156, 198, 373, 326}};

/**
 * @brief Read a single line from a text file.
 * @param fp File pointer.
 * @param buffer Line buffer (allocated/extended internally).
 * @param len Output line length (excluding terminator).
 * @return Line buffer pointer on success, nullptr on failure.
 */
static char *readLine(FILE *fp, char *buffer, int *len)
{
    int ch;
    int i = 0;
    size_t buff_len = 0;
    buffer = static_cast<char *>(malloc(buff_len + 1));
    if (!buffer) {
        return nullptr;
    }
    while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
        buff_len++;
        void *tmp = realloc(buffer, buff_len + 1);
        if (!tmp) {
            free(buffer);
            return nullptr;
        }
        buffer = static_cast<char *>(tmp);
        buffer[i] = static_cast<char>(ch);
        i++;
    }
    buffer[i] = '\0';
    *len = static_cast<int>(buff_len);
    if (ch == EOF && (i == 0 || ferror(fp))) {
        free(buffer);
        return nullptr;
    }
    return buffer;
}

/**
 * @brief Read multiple lines from a text file into a string array.
 * @param fileName Text file path.
 * @param lines Output line pointer array.
 * @param max_line Maximum number of lines to read.
 * @return Number of lines read on success, -1 on failure.
 */
static int readLines(const char *fileName, char *lines[], int max_line)
{
    FILE *file = fopen(fileName, "r");
    char *s = nullptr;
    int i = 0;
    int n = 0;
    if (file == nullptr) {
        return -1;
    }
    while ((s = readLine(file, s, &n)) != nullptr) {
        lines[i++] = s;
        if (i >= max_line) {
            break;
        }
    }
    fclose(file);
    return i;
}

/**
 * @brief Load class label names from a file.
 * @param locationFilename Label file path.
 * @param label Output label array pointer.
 * @return 0 on success, -1 on failure.
 */
static int loadLabelName(const char *locationFilename, char *label[])
{
    readLines(locationFilename, label, YOLOV5_OBJ_CLASS_NUM);
    return 0;
}

/**
 * @brief Clamp a float value to integer range.
 * @param val Input float value.
 * @param min Minimum value.
 * @param max Maximum value.
 * @return Clamped integer value.
 */
static inline int clampInt(float val, int min, int max)
{
    return val > min ? (val < max ? static_cast<int>(val) : max) : min;
}

/**
 * @brief Compute Intersection-over-Union (IoU) of two rectangles.
 * @param xmin0 First rect top-left x.
 * @param ymin0 First rect top-left y.
 * @param xmax0 First rect bottom-right x.
 * @param ymax0 First rect bottom-right y.
 * @param xmin1 Second rect top-left x.
 * @param ymin1 Second rect top-left y.
 * @param xmax1 Second rect bottom-right x.
 * @param ymax1 Second rect bottom-right y.
 * @return IoU value.
 */
static float CalculateOverlap(float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                              float ymax1)
{
    float w = std::fmax(0.f, std::fmin(xmax0, xmax1) - std::fmax(xmin0, xmin1) + 1.0f);
    float h = std::fmax(0.f, std::fmin(ymax0, ymax1) - std::fmax(ymin0, ymin1) + 1.0f);
    float i = w * h;
    float u = (xmax0 - xmin0 + 1.0f) * (ymax0 - ymin0 + 1.0f) + (xmax1 - xmin1 + 1.0f) * (ymax1 - ymin1 + 1.0f) - i;
    return u <= 0.f ? 0.f : (i / u);
}

/**
 * @brief Perform Non-Maximum Suppression for a given class.
 * @param validCount Number of valid candidates.
 * @param outputLocations Candidate box position array.
 * @param classIds Candidate class ID array.
 * @param order Score-sorted index array.
 * @param filterId Target class ID.
 * @param threshold NMS threshold.
 * @return 0 on success.
 */
static int nms(int validCount, std::vector<float> &outputLocations, std::vector<int> classIds, std::vector<int> &order,
               int filterId, float threshold)
{
    for (int i = 0; i < validCount; ++i) {
        if (order[i] == -1 || classIds[i] != filterId) {
            continue;
        }
        int n = order[i];
        for (int j = i + 1; j < validCount; ++j) {
            int m = order[j];
            if (m == -1 || classIds[i] != filterId) {
                continue;
            }
            float xmin0 = outputLocations[n * 4 + 0];
            float ymin0 = outputLocations[n * 4 + 1];
            float xmax0 = outputLocations[n * 4 + 0] + outputLocations[n * 4 + 2];
            float ymax0 = outputLocations[n * 4 + 1] + outputLocations[n * 4 + 3];

            float xmin1 = outputLocations[m * 4 + 0];
            float ymin1 = outputLocations[m * 4 + 1];
            float xmax1 = outputLocations[m * 4 + 0] + outputLocations[m * 4 + 2];
            float ymax1 = outputLocations[m * 4 + 1] + outputLocations[m * 4 + 3];

            float iou = CalculateOverlap(xmin0, ymin0, xmax0, ymax0, xmin1, ymin1, xmax1, ymax1);

            if (iou > threshold) {
                order[j] = -1;
            }
        }
    }
    return 0;
}

/**
 * @brief Sort score array in descending order, returning index array.
 * @param input Score array (modified in place).
 * @param left Left boundary.
 * @param right Right boundary.
 * @param indices Index array (modified in place).
 * @return Pivot element position index.
 */
static int quick_sort_indice_inverse(std::vector<float> &input, int left, int right, std::vector<int> &indices)
{
    float key;
    int key_index;
    int low = left;
    int high = right;
    if (left < right) {
        key_index = indices[left];
        key = input[left];
        while (low < high) {
            while (low < high && input[high] <= key) {
                high--;
            }
            input[low] = input[high];
            indices[low] = indices[high];
            while (low < high && input[low] >= key) {
                low++;
            }
            input[high] = input[low];
            indices[high] = indices[low];
        }
        input[low] = key;
        indices[low] = key_index;
        quick_sort_indice_inverse(input, left, low - 1, indices);
        quick_sort_indice_inverse(input, low + 1, right, indices);
    }
    return low;
}

/**
 * @brief Dequantize an int8 value to float32.
 * @param qnt Quantized value.
 * @param zp Zero point.
 * @param scale Scale factor.
 * @return Dequantized float value.
 */
static inline float deqnt_affine_to_f32(int8_t qnt, int32_t zp, float scale)
{
    return (static_cast<float>(qnt) - static_cast<float>(zp)) * scale;
}

/**
 * @brief Quantize a float32 value to int8.
 * @param f32 Float value.
 * @param zp Zero point.
 * @param scale Scale factor.
 * @return Quantized int8 value.
 */
static inline int8_t qnt_f32_to_affine(float f32, int32_t zp, float scale)
{
    float dst_val = (f32 / scale) + zp;
    if (dst_val < -128.f) {
        dst_val = -128.f;
    } else if (dst_val > 127.f) {
        dst_val = 127.f;
    }
    return static_cast<int8_t>(dst_val);
}

#if !defined(RV1106_1103)
/**
 * @brief Decode i8 output format.
 * @param input Output feature map pointer.
 * @param anchor Current branch anchor array.
 * @param grid_h Grid height.
 * @param grid_w Grid width.
 * @param height Model input height.
 * @param width Model input width.
 * @param stride Current branch stride.
 * @param boxes Output candidate box array.
 * @param objProbs Output confidence array.
 * @param classId Output class ID array.
 * @param threshold Confidence threshold.
 * @param zp Quantization zero point.
 * @param scale Quantization scale factor.
 * @return Number of valid candidates.
 */
static int process_i8(int8_t *input, const int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                      std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId, float threshold,
                      int32_t zp, float scale)
{
    (void)height;
    (void)width;
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    int8_t thres_i8 = qnt_f32_to_affine(threshold, zp, scale);
    for (int a = 0; a < 3; a++) {
        for (int i = 0; i < grid_h; i++) {
            for (int j = 0; j < grid_w; j++) {
                int8_t box_confidence = input[(kPropBoxSize * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= thres_i8) {
                    int offset = (kPropBoxSize * a) * grid_len + i * grid_w + j;
                    int8_t *in_ptr = input + offset;
                    float box_x = (deqnt_affine_to_f32(*in_ptr, zp, scale)) * 2.0f - 0.5f;
                    float box_y = (deqnt_affine_to_f32(in_ptr[grid_len], zp, scale)) * 2.0f - 0.5f;
                    float box_w = (deqnt_affine_to_f32(in_ptr[2 * grid_len], zp, scale)) * 2.0f;
                    float box_h = (deqnt_affine_to_f32(in_ptr[3 * grid_len], zp, scale)) * 2.0f;
                    box_x = (box_x + j) * static_cast<float>(stride);
                    box_y = (box_y + i) * static_cast<float>(stride);
                    box_w = box_w * box_w * static_cast<float>(anchor[a * 2]);
                    box_h = box_h * box_h * static_cast<float>(anchor[a * 2 + 1]);
                    box_x -= (box_w / 2.0f);
                    box_y -= (box_h / 2.0f);

                    int8_t maxClassProbs = in_ptr[5 * grid_len];
                    int maxClassId = 0;
                    for (int k = 1; k < YOLOV5_OBJ_CLASS_NUM; ++k) {
                        int8_t prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs) {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    if (maxClassProbs > thres_i8) {
                        objProbs.push_back((deqnt_affine_to_f32(maxClassProbs, zp, scale)) *
                                           (deqnt_affine_to_f32(box_confidence, zp, scale)));
                        classId.push_back(maxClassId);
                        validCount++;
                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                    }
                }
            }
        }
    }
    return validCount;
}
#endif

/**
 * @brief Decode RV1106 i8 NHWC output format.
 * @param input Output feature map pointer.
 * @param anchor Current branch anchor array.
 * @param grid_h Grid height.
 * @param grid_w Grid width.
 * @param height Model input height.
 * @param width Model input width.
 * @param stride Current branch stride.
 * @param boxes Output candidate box array.
 * @param boxScores Output confidence array.
 * @param classId Output class ID array.
 * @param threshold Confidence threshold.
 * @param zp Quantization zero point.
 * @param scale Quantization scale factor.
 * @return Number of valid candidates.
 */
static int process_i8_rv1106(int8_t *input, const int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                             std::vector<float> &boxes, std::vector<float> &boxScores, std::vector<int> &classId,
                             float threshold, int32_t zp, float scale)
{
    (void)height;
    (void)width;
    int validCount = 0;
    int8_t thres_i8 = qnt_f32_to_affine(threshold, zp, scale);
    int anchor_per_branch = 3;
    int align_c = kPropBoxSize * anchor_per_branch;

    for (int h = 0; h < grid_h; h++) {
        for (int w = 0; w < grid_w; w++) {
            for (int a = 0; a < anchor_per_branch; a++) {
                int hw_offset = h * grid_w * align_c + w * align_c + a * kPropBoxSize;
                int8_t *hw_ptr = input + hw_offset;
                int8_t box_confidence = hw_ptr[4];

                if (box_confidence >= thres_i8) {
                    int8_t maxClassProbs = hw_ptr[5];
                    int maxClassId = 0;
                    for (int k = 1; k < YOLOV5_OBJ_CLASS_NUM; ++k) {
                        int8_t prob = hw_ptr[5 + k];
                        if (prob > maxClassProbs) {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }

                    float box_conf_f32 = deqnt_affine_to_f32(box_confidence, zp, scale);
                    float class_prob_f32 = deqnt_affine_to_f32(maxClassProbs, zp, scale);
                    float limit_score = box_conf_f32 * class_prob_f32;

                    if (limit_score > threshold) {
                        float box_x, box_y, box_w, box_h;

                        box_x = deqnt_affine_to_f32(hw_ptr[0], zp, scale) * 2.0f - 0.5f;
                        box_y = deqnt_affine_to_f32(hw_ptr[1], zp, scale) * 2.0f - 0.5f;
                        box_w = deqnt_affine_to_f32(hw_ptr[2], zp, scale) * 2.0f;
                        box_h = deqnt_affine_to_f32(hw_ptr[3], zp, scale) * 2.0f;
                        box_w = box_w * box_w;
                        box_h = box_h * box_h;

                        box_x = (box_x + w) * static_cast<float>(stride);
                        box_y = (box_y + h) * static_cast<float>(stride);
                        box_w *= static_cast<float>(anchor[a * 2]);
                        box_h *= static_cast<float>(anchor[a * 2 + 1]);

                        box_x -= (box_w / 2.0f);
                        box_y -= (box_h / 2.0f);

                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                        boxScores.push_back(limit_score);
                        classId.push_back(maxClassId);
                        validCount++;
                    }
                }
            }
        }
    }
    return validCount;
}

#if !defined(RV1106_1103)
/**
 * @brief Decode fp32 output format.
 * @param input Output feature map pointer.
 * @param anchor Current branch anchor array.
 * @param grid_h Grid height.
 * @param grid_w Grid width.
 * @param height Model input height.
 * @param width Model input width.
 * @param stride Current branch stride.
 * @param boxes Output candidate box array.
 * @param objProbs Output confidence array.
 * @param classId Output class ID array.
 * @param threshold Confidence threshold.
 * @return Number of valid candidates.
 */
static int process_fp32(float *input, const int *anchor, int grid_h, int grid_w, int height, int width, int stride,
                        std::vector<float> &boxes, std::vector<float> &objProbs, std::vector<int> &classId,
                        float threshold)
{
    (void)height;
    (void)width;
    int validCount = 0;
    int grid_len = grid_h * grid_w;
    for (int a = 0; a < 3; a++) {
        for (int i = 0; i < grid_h; i++) {
            for (int j = 0; j < grid_w; j++) {
                float box_confidence = input[(kPropBoxSize * a + 4) * grid_len + i * grid_w + j];
                if (box_confidence >= threshold) {
                    int offset = (kPropBoxSize * a) * grid_len + i * grid_w + j;
                    float *in_ptr = input + offset;
                    float box_x = *in_ptr * 2.0f - 0.5f;
                    float box_y = in_ptr[grid_len] * 2.0f - 0.5f;
                    float box_w = in_ptr[2 * grid_len] * 2.0f;
                    float box_h = in_ptr[3 * grid_len] * 2.0f;
                    box_x = (box_x + j) * static_cast<float>(stride);
                    box_y = (box_y + i) * static_cast<float>(stride);
                    box_w = box_w * box_w * static_cast<float>(anchor[a * 2]);
                    box_h = box_h * box_h * static_cast<float>(anchor[a * 2 + 1]);
                    box_x -= (box_w / 2.0f);
                    box_y -= (box_h / 2.0f);

                    float maxClassProbs = in_ptr[5 * grid_len];
                    int maxClassId = 0;
                    for (int k = 1; k < YOLOV5_OBJ_CLASS_NUM; ++k) {
                        float prob = in_ptr[(5 + k) * grid_len];
                        if (prob > maxClassProbs) {
                            maxClassId = k;
                            maxClassProbs = prob;
                        }
                    }
                    if (maxClassProbs > threshold) {
                        objProbs.push_back(maxClassProbs * box_confidence);
                        classId.push_back(maxClassId);
                        validCount++;
                        boxes.push_back(box_x);
                        boxes.push_back(box_y);
                        boxes.push_back(box_w);
                        boxes.push_back(box_h);
                    }
                }
            }
        }
    }
    return validCount;
}
#endif
} 

/**
 * @brief Constructor, initialize internal state.
 */
YoloV5Runner::YoloV5Runner()
{
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
}

/**
 * @brief Destructor, release resources.
 */
YoloV5Runner::~YoloV5Runner()
{
    deinit();
}

/**
 * @brief Initialize model and post-processing resources.
 * @param model_path Model file path.
 * @param label_path Label file path, can be nullptr.
 * @return true on success, false on failure.
 */
bool YoloV5Runner::init(const char *model_path, const char *label_path)
{
    if (m_ready) {
        deinit();
    }
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
    if (rknn_init(&m_rknn, const_cast<char *>(model_path), 0, 0, nullptr) < 0) {
        return false;
    }

    if (rknn_query(m_rknn, RKNN_QUERY_IN_OUT_NUM, &m_ioNum, sizeof(m_ioNum)) != RKNN_SUCC) {
        return false;
    }

    rknn_tensor_attr input_attrs[m_ioNum.n_input];
    std::memset(input_attrs, 0, sizeof(input_attrs));
    for (uint32_t i = 0; i < m_ioNum.n_input; i++) {
        input_attrs[i].index = i;
        if (rknn_query(m_rknn, RKNN_QUERY_NATIVE_INPUT_ATTR, &(input_attrs[i]), sizeof(rknn_tensor_attr)) != RKNN_SUCC) {
            return false;
        }
    }

    rknn_tensor_attr output_attrs[m_ioNum.n_output];
    std::memset(output_attrs, 0, sizeof(output_attrs));
    for (uint32_t i = 0; i < m_ioNum.n_output; i++) {
        output_attrs[i].index = i;
        if (rknn_query(m_rknn, RKNN_QUERY_NATIVE_NHWC_OUTPUT_ATTR, &(output_attrs[i]), sizeof(rknn_tensor_attr)) !=
            RKNN_SUCC) {
            return false;
        }
    }

    input_attrs[0].type = RKNN_TENSOR_UINT8;
    input_attrs[0].fmt = RKNN_TENSOR_NHWC;
    m_inputMems[0] = rknn_create_mem(m_rknn, input_attrs[0].size_with_stride);
    if (rknn_set_io_mem(m_rknn, m_inputMems[0], &input_attrs[0]) < 0) {
        return false;
    }

    for (uint32_t i = 0; i < m_ioNum.n_output; ++i) {
        m_outputMems[i] = rknn_create_mem(m_rknn, output_attrs[i].size_with_stride);
        if (rknn_set_io_mem(m_rknn, m_outputMems[i], &output_attrs[i]) < 0) {
            return false;
        }
    }

    m_isQuant = (output_attrs[0].qnt_type == RKNN_TENSOR_QNT_AFFINE_ASYMMETRIC);

    m_inputAttrs = static_cast<rknn_tensor_attr *>(malloc(m_ioNum.n_input * sizeof(rknn_tensor_attr)));
    std::memcpy(m_inputAttrs, input_attrs, m_ioNum.n_input * sizeof(rknn_tensor_attr));
    m_outputAttrs = static_cast<rknn_tensor_attr *>(malloc(m_ioNum.n_output * sizeof(rknn_tensor_attr)));
    std::memcpy(m_outputAttrs, output_attrs, m_ioNum.n_output * sizeof(rknn_tensor_attr));

    if (input_attrs[0].fmt == RKNN_TENSOR_NCHW) {
        m_modelChannel = input_attrs[0].dims[1];
        m_modelHeight = input_attrs[0].dims[2];
        m_modelWidth = input_attrs[0].dims[3];
    } else {
        m_modelHeight = input_attrs[0].dims[1];
        m_modelWidth = input_attrs[0].dims[2];
        m_modelChannel = input_attrs[0].dims[3];
    }

    const char *labels = label_path ? label_path : "./model/coco_80_labels_list.txt";
    if (loadLabelName(labels, m_labels) < 0) {
        return false;
    }

    m_ready = true;
    return true;
}

/**
 * @brief Release model and post-processing resources.
 */
void YoloV5Runner::deinit()
{
    if (!m_ready) {
        return;
    }
    for (int i = 0; i < YOLOV5_OBJ_CLASS_NUM; i++) {
        if (m_labels[i]) {
            free(m_labels[i]);
            m_labels[i] = nullptr;
        }
    }
    if (m_inputAttrs) {
        free(m_inputAttrs);
        m_inputAttrs = nullptr;
    }
    if (m_outputAttrs) {
        free(m_outputAttrs);
        m_outputAttrs = nullptr;
    }
    for (uint32_t i = 0; i < m_ioNum.n_input; i++) {
        if (m_inputMems[i]) {
            rknn_destroy_mem(m_rknn, m_inputMems[i]);
            free(m_inputMems[i]);
            m_inputMems[i] = nullptr;
        }
    }
    for (uint32_t i = 0; i < m_ioNum.n_output; i++) {
        if (m_outputMems[i]) {
            rknn_destroy_mem(m_rknn, m_outputMems[i]);
            free(m_outputMems[i]);
            m_outputMems[i] = nullptr;
        }
    }
    if (m_rknn != 0) {
        rknn_destroy(m_rknn);
        m_rknn = 0;
    }
    std::memset(&m_ioNum, 0, sizeof(m_ioNum));
    m_modelWidth = 0;
    m_modelHeight = 0;
    m_modelChannel = 0;
    m_isQuant = false;
    m_ready = false;
}

/**
 * @brief Check if the runner is initialized.
 * @return true if initialized, false otherwise.
 */
bool YoloV5Runner::isReady() const
{
    return m_ready;
}

/**
 * @brief Get class name by ID.
 * @param cls_id Class ID.
 * @return Class name string pointer.
 */
const char *YoloV5Runner::className(int cls_id) const
{
    static char null_str[] = "null";
    if (cls_id < 0 || cls_id >= YOLOV5_OBJ_CLASS_NUM) {
        return null_str;
    }
    if (m_labels[cls_id]) {
        return m_labels[cls_id];
    }
    return null_str;
}

/**
 * @brief Get model input buffer pointer.
 * @return Input buffer pointer, nullptr on failure.
 */
uint8_t *YoloV5Runner::inputBuffer()
{
    if (!m_ready || !m_inputMems[0]) {
        return nullptr;
    }
    return static_cast<uint8_t *>(m_inputMems[0]->virt_addr);
}

/**
 * @brief Get model input width.
 * @return Model input width.
 */
int YoloV5Runner::inputWidth() const
{
    return m_modelWidth;
}

/**
 * @brief Get model input height.
 * @return Model input height.
 */
int YoloV5Runner::inputHeight() const
{
    return m_modelHeight;
}

/**
 * @brief Get model input channel count.
 * @return Model input channel count.
 */
int YoloV5Runner::inputChannels() const
{
    return m_modelChannel;
}

/**
 * @brief Run inference and produce detection results.
 * @param results Output detection results pointer.
 * @return 0 on success, -1 on failure.
 */
int YoloV5Runner::run(YoloV5Detections *results)
{
    if (!m_ready || !results) {
        return -1;
    }
    int ret = rknn_run(m_rknn, nullptr);
    if (ret < 0) {
        return -1;
    }

    std::vector<float> filterBoxes;
    std::vector<float> objProbs;
    std::vector<int> classId;
    int validCount = 0;
    int stride = 0;
    int grid_h = 0;
    int grid_w = 0;
    int model_in_w = m_modelWidth;
    int model_in_h = m_modelHeight;

    std::memset(results, 0, sizeof(YoloV5Detections));

    for (int i = 0; i < 3; i++) {
#if defined(RV1106_1103)
        grid_h = m_outputAttrs[i].dims[2];
        grid_w = m_outputAttrs[i].dims[1];
        stride = model_in_h / grid_h;
        if (m_isQuant) {
            validCount += process_i8_rv1106(static_cast<int8_t *>(m_outputMems[i]->virt_addr), kAnchor[i], grid_h, grid_w,
                                            model_in_h, model_in_w, stride, filterBoxes, objProbs, classId, kBoxThresh,
                                            m_outputAttrs[i].zp, m_outputAttrs[i].scale);
        }
#else
        grid_h = m_outputAttrs[i].dims[2];
        grid_w = m_outputAttrs[i].dims[3];
        stride = model_in_h / grid_h;
        if (m_isQuant) {
            validCount += process_i8(static_cast<int8_t *>(m_outputMems[i]->virt_addr), kAnchor[i], grid_h, grid_w,
                                     model_in_h, model_in_w, stride, filterBoxes, objProbs, classId, kBoxThresh,
                                     m_outputAttrs[i].zp, m_outputAttrs[i].scale);
        } else {
            validCount += process_fp32(static_cast<float *>(m_outputMems[i]->virt_addr), kAnchor[i], grid_h, grid_w,
                                       model_in_h, model_in_w, stride, filterBoxes, objProbs, classId, kBoxThresh);
        }
#endif
    }

    if (validCount <= 0) {
        return 0;
    }

    std::vector<int> indexArray;
    indexArray.reserve(validCount);
    for (int i = 0; i < validCount; ++i) {
        indexArray.push_back(i);
    }
    quick_sort_indice_inverse(objProbs, 0, validCount - 1, indexArray);

    std::set<int> class_set(std::begin(classId), std::end(classId));
    for (auto c : class_set) {
        nms(validCount, filterBoxes, classId, indexArray, c, kNmsThresh);
    }

    int last_count = 0;
    results->count = 0;
    for (int i = 0; i < validCount; ++i) {
        if (indexArray[i] == -1 || last_count >= YOLOV5_OBJ_MAX) {
            continue;
        }
        int n = indexArray[i];
        float x1 = filterBoxes[n * 4 + 0];
        float y1 = filterBoxes[n * 4 + 1];
        float x2 = x1 + filterBoxes[n * 4 + 2];
        float y2 = y1 + filterBoxes[n * 4 + 3];
        int id = classId[n];
        float obj_conf = objProbs[i];

        results->results[last_count].box.left = clampInt(x1, 0, model_in_w);
        results->results[last_count].box.top = clampInt(y1, 0, model_in_h);
        results->results[last_count].box.right = clampInt(x2, 0, model_in_w);
        results->results[last_count].box.bottom = clampInt(y2, 0, model_in_h);
        results->results[last_count].prop = obj_conf;
        results->results[last_count].cls_id = id;
        last_count++;
    }
    results->count = last_count;
    return 0;
}
