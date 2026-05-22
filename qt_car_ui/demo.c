#include <iostream>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <lockzhiner_vision_module/vision/deep_learning/detection/paddle_det.h>
#include <lockzhiner_vision_module/vision/utils/visualize.h>
#include <lockzhiner_vision_module/edit/edit.h>
#include "rknpu2_backend/rknpu2_backend.h"
#include <deque> 

using namespace cv;
using namespace std;
using namespace std::chrono;

// 定义关键点索引 (根据106点模型)
const vector<int> LEFT_EYE_POINTS = {35, 41, 40, 42, 39, 37, 33, 36};   // 左眼
const vector<int> RIGHT_EYE_POINTS = {89, 95, 94, 96, 93, 91, 87, 90};  // 右眼

// 嘴部
const vector<int> MOUTH_OUTLINE = {52, 64, 63, 71, 67, 68, 61, 58, 59, 53, 56, 55};
const vector<int> MOUTH_INNER = {65, 66, 62, 70, 69, 57, 60, 54};
vector<int> MOUTH_POINTS;

// 计算眼睛纵横比(EAR)
float eye_aspect_ratio(const vector<Point2f>& eye_points) {
    // 计算垂直距离
    double A = norm(eye_points[1] - eye_points[7]);
    double B = norm(eye_points[2] - eye_points[6]);
    double C = norm(eye_points[3] - eye_points[5]);
    
    // 计算水平距离
    double D = norm(eye_points[0] - eye_points[4]);
    
    // 防止除以零
    if (D < 1e-5) return 0.0f;
    
    return (float)((A + B + C) / (3.0 * D));
}

// 计算嘴部纵横比(MAR)
float mouth_aspect_ratio(const vector<Point2f>& mouth_points) {
    // 关键点索引（基于MOUTH_OUTLINE中的位置）
    const int LEFT_CORNER = 0;    // 52 (左嘴角)
    const int UPPER_CENTER = 3;   // 71 (上唇中心)
    const int RIGHT_CORNER = 6;   // 61 (右嘴角)
    const int LOWER_CENTER = 9;   // 53 (下唇中心)
    
    // 计算垂直距离
    double A = norm(mouth_points[UPPER_CENTER] - mouth_points[LOWER_CENTER]);  // 上唇中心到下唇中心
    double B = norm(mouth_points[UPPER_CENTER] - mouth_points[LEFT_CORNER]);   // 上唇中心到左嘴角
    double C = norm(mouth_points[UPPER_CENTER] - mouth_points[RIGHT_CORNER]);  // 上唇中心到右嘴角
    
    // 计算嘴部宽度（左右嘴角距离）
    double D = norm(mouth_points[LEFT_CORNER] - mouth_points[RIGHT_CORNER]);  // 左嘴角到右嘴角
    
    // 防止除以零
    if (D < 1e-5) return 0.0f;
    
    // 计算平均垂直距离与水平距离的比值
    return static_cast<float>((A + B + C) / (3.0 * D));
}

int main(int argc, char** argv)
{
    // 初始化嘴部关键点
    MOUTH_POINTS.clear();
    MOUTH_POINTS.insert(MOUTH_POINTS.end(), MOUTH_OUTLINE.begin(), MOUTH_OUTLINE.end());
    MOUTH_POINTS.insert(MOUTH_POINTS.end(), MOUTH_INNER.begin(), MOUTH_INNER.end());
    
    // 检查命令行参数
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <paddle_det_model_path> <pfld_rknn_model_path> <pfld_input_size>\n";
        cerr << "Example: " << argv[0] << " picodet_model_dir pfld.rknn 112\n";
        return -1;
    }

    const char* paddle_model_path = argv[1];
    const char* pfld_rknn_path = argv[2];
    const int pfld_size = atoi(argv[3]);  // PFLD模型输入尺寸 (112)

    // 1. 初始化PaddleDet人脸检测模型
    lockzhiner_vision_module::vision::PaddleDet face_detector;
    if (!face_detector.Initialize(paddle_model_path)) {
        cerr << "Failed to initialize PaddleDet face detector model." << endl;
        return -1;
    }

    // 2. 初始化PFLD RKNN模型
    lockzhiner_vision_module::vision::RKNPU2Backend pfld_backend;
    if (!pfld_backend.Initialize(pfld_rknn_path)) {
        cerr << "Failed to load PFLD RKNN model." << endl;
        return -1;
    }
    
    // 获取输入张量信息
    const auto& input_tensor = pfld_backend.GetInputTensor(0);
    const vector<size_t> input_dims = input_tensor.GetDims();
    const float input_scale = input_tensor.GetScale();
    const int input_zp = input_tensor.GetZp();
    
    // 打印输入信息
    cout << "PFLD Input Info:" << endl;
    cout << "  Dimensions: ";
    for (auto dim : input_dims) cout << dim << " ";
    cout << "\n  Scale: " << input_scale << "  Zero Point: " << input_zp << endl;
    
    // 3. 初始化Edit模块
    lockzhiner_vision_module::edit::Edit edit;
    if (!edit.StartAndAcceptConnection()) {
        cerr << "Error: Failed to start and accept connection." << endl;
        return EXIT_FAILURE;
    }
    cout << "Device connected successfully." << endl;

    // 4. 打开摄像头
    VideoCapture cap;
    cap.set(CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CAP_PROP_FRAME_HEIGHT, 480);
    cap.open(0);

    if (!cap.isOpened()) {
        cerr << "Error: Could not open camera." << endl;
        return -1;
    }

    Mat frame;
    const int num_landmarks = 106;
    int frame_count = 0;
    const int debug_freq = 10; // 每10帧打印一次调试信息
    
    // ================== 疲劳检测参数 ==================
    const float EAR_THRESHOLD = 0.20f;       // 眼睛纵横比阈值
    const float MAR_THRESHOLD = 0.55f;        // 嘴部纵横比阈值
    
    const int EYE_CLOSED_FRAMES = 15;        // 闭眼持续帧数阈值
    const int MOUTH_OPEN_FRAMES = 20;        // 张嘴持续帧数阈值
    
    int consecutive_eye_closed = 0;          // 连续闭眼帧数
    int consecutive_mouth_open = 0;          // 连续张嘴帧数
    
    bool is_tired = false;                   // 当前疲劳状态
    
    deque<bool> eye_state_buffer;            // 用于PERCLOS计算
    const int PERCLOS_WINDOW = 200;          // PERCLOS计算窗口大小
    float perclos = 0.0f;                    // 闭眼时间占比
    
    // 疲劳状态文本
    const string TIRED_TEXT = "TIRED";
    const string NORMAL_TEXT = "NORMAL";
    const Scalar TIRED_COLOR = Scalar(0, 0, 255);  // 红色
    const Scalar NORMAL_COLOR = Scalar(0, 255, 0); // 绿色

    while (true) {
        // 5. 捕获一帧图像
        cap >> frame;
        if (frame.empty()) {
            cerr << "Warning: Captured an empty frame." << endl;
            continue;
        }

        // 6. 人脸检测
        auto start_det = high_resolution_clock::now();
        auto face_results = face_detector.Predict(frame);
        auto end_det = high_resolution_clock::now();
        auto det_duration = duration_cast<milliseconds>(end_det - start_det);
        
        Mat result_image = frame.clone();
        bool pfld_debug_printed = false;
        
        // 7. 处理每个检测到的人脸
        for (const auto& face : face_results) {
            // 跳过非人脸检测结果
            if (face.label_id != 0) continue;
            
            // 提取人脸区域
            Rect face_rect = face.box;
            
            // 确保人脸区域在图像范围内
            face_rect.x = max(0, face_rect.x);
            face_rect.y = max(0, face_rect.y);
            face_rect.width = min(face_rect.width, frame.cols - face_rect.x);
            face_rect.height = min(face_rect.height, frame.rows - face_rect.y);
            
            if (face_rect.width <= 10 || face_rect.height <= 10) continue;
            
            // 绘制人脸框
            rectangle(result_image, face_rect, Scalar(0, 255, 0), 2);
            
            // 8. 关键点检测
            Mat face_roi = frame(face_rect);
            Mat face_resized;
            resize(face_roi, face_resized, Size(pfld_size, pfld_size));
            
            // 8.1 预处理 (转换为RKNN输入格式)
            cvtColor(face_resized, face_resized, COLOR_BGR2RGB);
            
            // 8.2 设置输入数据
            void* input_data = input_tensor.GetData();
            size_t required_size = input_tensor.GetElemsBytes();
            size_t actual_size = face_resized.total() * face_resized.elemSize();
            
            if (actual_size != required_size) {
                cerr << "Input size mismatch! Required: " << required_size 
                     << ", Actual: " << actual_size << endl;
                continue;
            }
            
            memcpy(input_data, face_resized.data, actual_size);
            
            // 8.3 执行推理
            auto start_pfld = high_resolution_clock::now();
            bool success = pfld_backend.Run();
            auto end_pfld = high_resolution_clock::now();
            auto pfld_duration = duration_cast<milliseconds>(end_pfld - start_pfld);
            
            if (!success) {
                cerr << "PFLD inference failed!" << endl;
                continue;
            }
            
            // 8.4 获取输出结果
            const auto& output_tensor = pfld_backend.GetOutputTensor(0);
            const float output_scale = output_tensor.GetScale();
            const int output_zp = output_tensor.GetZp();
            const int8_t* output_data = static_cast<const int8_t*>(output_tensor.GetData());
            const vector<size_t> output_dims = output_tensor.GetDims();
            
            // 计算输出元素数量
            size_t total_elems = 1;
            for (auto dim : output_dims) total_elems *= dim;
            
            // 打印输出信息 (调试)
            if ((frame_count % debug_freq == 0 || !pfld_debug_printed) && !face_results.empty()) {
                cout << "\n--- PFLD Output Debug ---" << endl;
                cout << "Output Scale: " << output_scale << " Zero Point: " << output_zp << endl;
                cout << "Output Dimensions: ";
                for (auto dim : output_dims) cout << dim << " ";
                cout << "\nTotal Elements: " << total_elems << endl;
                
                cout << "First 10 output values: ";
                for (int i = 0; i < min(10, static_cast<int>(total_elems)); i++) {
                    cout << (int)output_data[i] << " ";
                }
                cout << endl;
                pfld_debug_printed = true;
            }
            
            // 9. 处理关键点结果
            vector<Point2f> landmarks;
            for (int i = 0; i < num_landmarks; i++) {
                // 反量化输出
                float x = (output_data[i * 2] - output_zp) * output_scale;
                float y = (output_data[i * 2 + 1] - output_zp) * output_scale;
                
                // 关键修正: 先缩放到112x112图像坐标
                x = x * pfld_size;
                y = y * pfld_size;
                
                // 映射到原始图像坐标
                float scale_x = static_cast<float>(face_rect.width) / pfld_size;
                float scale_y = static_cast<float>(face_rect.height) / pfld_size;
                x = x * scale_x + face_rect.x;
                y = y * scale_y + face_rect.y;
                
                landmarks.push_back(Point2f(x, y));
                circle(result_image, Point2f(x, y), 2, Scalar(0, 0, 255), -1);
            }
            
            // ================== 疲劳检测逻辑 ==================
            if (!landmarks.empty()) {
                
                // 9.1 提取眼部关键点
                vector<Point2f> left_eye, right_eye;
                for (int idx : LEFT_EYE_POINTS) {
                    if (idx < landmarks.size()) {
                        left_eye.push_back(landmarks[idx]);
                    }
                }
                for (int idx : RIGHT_EYE_POINTS) {
                    if (idx < landmarks.size()) {
                        right_eye.push_back(landmarks[idx]);
                    }
                }
                
                // 9.3 提取嘴部关键点
                vector<Point2f> mouth;
                for (int idx : MOUTH_POINTS) {
                    if (idx < landmarks.size()) {
                        mouth.push_back(landmarks[idx]);
                    }
                }
                
                // 9.4 计算眼部纵横比(EAR)
                float ear_left = 0.0f, ear_right = 0.0f, ear_avg = 0.0f;
                if (!left_eye.empty() && !right_eye.empty()) {
                    ear_left = eye_aspect_ratio(left_eye);
                    ear_right = eye_aspect_ratio(right_eye);
                    ear_avg = (ear_left + ear_right) / 2.0f;
                }
                
                // 9.5 计算嘴部纵横比(MAR)
                float mar = 0.0f;
                if (!mouth.empty()) {
                    mar = mouth_aspect_ratio(mouth);
                }
                
                // 9.6 更新PERCLOS缓冲区
                if (eye_state_buffer.size() >= PERCLOS_WINDOW) {
                    eye_state_buffer.pop_front();
                }
                eye_state_buffer.push_back(ear_avg < EAR_THRESHOLD);
                
                // 计算PERCLOS (闭眼时间占比)
                int closed_count = 0;
                for (bool closed : eye_state_buffer) {
                    if (closed) closed_count++;
                }
                perclos = static_cast<float>(closed_count) / eye_state_buffer.size();
                
                // 9.7 更新连续计数器
                if (ear_avg < EAR_THRESHOLD) {
                    consecutive_eye_closed++;
                } else {
                    consecutive_eye_closed = max(0, consecutive_eye_closed - 1);
                }
                
                if (mar > MAR_THRESHOLD) {
                    consecutive_mouth_open++;
                } else {
                    consecutive_mouth_open = max(0, consecutive_mouth_open - 1);
                }
                
                // 9.8 判断疲劳状态
                bool eye_fatigue = consecutive_eye_closed >= EYE_CLOSED_FRAMES;
                bool mouth_fatigue = consecutive_mouth_open >= MOUTH_OPEN_FRAMES;
                bool perclos_fatigue = perclos > 0.5f; // PERCLOS > 50%
                
                is_tired = eye_fatigue || mouth_fatigue || perclos_fatigue;
                
                // 9.9 在图像上标注疲劳状态
                string status_text = is_tired ? TIRED_TEXT : NORMAL_TEXT;
                Scalar status_color = is_tired ? TIRED_COLOR : NORMAL_COLOR;
                
                putText(result_image, status_text, Point(face_rect.x, face_rect.y - 30),
                       FONT_HERSHEY_SIMPLEX, 0.8, status_color, 2);
                
                // 9.10 显示检测指标
                string info = format("EAR: %.2f MAR: %.2f PERCLOS: %.1f%%", 
                                    ear_avg, mar, perclos*100);
                putText(result_image, info, Point(face_rect.x, face_rect.y - 60),
                       FONT_HERSHEY_SIMPLEX, 0.5, Scalar(200, 200, 0), 1);
            }
        }
        
        // 10. 显示性能信息
        auto end_total = high_resolution_clock::now();
        auto total_duration = duration_cast<milliseconds>(end_total - start_det);
        
        string info = "Faces: " + to_string(face_results.size()) 
                    + " | Det: " + to_string(det_duration.count()) + "ms"
                    + " | Total: " + to_string(total_duration.count()) + "ms";
        putText(result_image, info, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(0, 255, 0), 2);
        
        // 11. 显示结果
        edit.Print(result_image);
        
        // 帧计数器更新
        frame_count = (frame_count + 1) % debug_freq;
        
        // 按ESC退出
        if (waitKey(1) == 27) break;
    }

    cap.release();
    return 0;
}