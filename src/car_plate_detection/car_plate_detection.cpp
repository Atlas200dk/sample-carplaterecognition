/**
 * ============================================================================
 *
 * Copyright (C) 2018, Hisilicon Technologies Co., Ltd. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1 Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *
 *   2 Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *
 *   3 Neither the names of the copyright holders nor the names of the
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ============================================================================
 */

#include "car_plate_detection.h"

#include <unistd.h>
#include <vector>
#include <thread>
#include <fstream>
#include <algorithm>
#include <iostream>

#include "hiaiengine/ai_types.h"
#include "hiaiengine/ai_memory.h"
#include "hiaiengine/log.h"

using hiai::Engine;
using namespace std;

namespace {
const uint32_t kOutputPort = 0; // output port:0

const int kSleep20MicroSecs = 20000; // waiting 20ms when queue is full

const string kModelPath = "model_path"; // model path string

const int kDestImageWidth = 480; // the image width for model

const int kDestImageHeight = 640; // the image height for model

const uint32_t kMinCropPixel = 17; // minimal crop size

const int kInferenceResultSize = 2; // car detection inference result size

const int kInferenceOutputNum = 1; // inference out put number index

const int kInferenceOutputBBox = 0; // inference out put BBox index

const uint32_t kSizePerResultset = 7; // result set size

const int kDvppProcSuccess = 0; // dvpp result is success flag

const float kMinInferenceScore = 0.5; // the good enough inference score

const float kTopLeftCoeff = 0.95; // the coeff used for top left

const float kLowerRightCoeff = 1.01; // the coeff used for lower right

// standard: 1024 * 1024 * 128 = 134217728 (128M)
const int kMaxNewMemory = 134217728;

// valid bbox coordinate values
const float kLowerCoord = 0.0f;
const float kUpperCoord = 1.0f;

// infernece output data index
enum BBoxDataIndex {
  kAttribute = 1,
  kScore,
  kTopLeftX,
  kTopLeftY,
  kLowerRightX,
  kLowerRightY
};
}

//HIAI_REGISTER_DATA_TYPE("VideoDetectionImageParaT", VideoDetectionImageParaT);
//HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
//HIAI_REGISTER_DATA_TYPE("ObjectImageParaT", ObjectImageParaT);

HIAI_StatusT CarPlateDetection::Init(
    const hiai::AIConfig &config,
    const vector<hiai::AIModelDescription> &model_desc) {
  HIAI_ENGINE_LOG("[CarPlateDetection] start to initialize!");
  hiai::AIStatus ret = hiai::SUCCESS;

  if (ai_model_manager_ == nullptr) { // check ai model manager is nullptr
    ai_model_manager_ = make_shared<hiai::AIModelManager>();
  }

  vector<hiai::AIModelDescription> model_desc_vec;
  hiai::AIModelDescription model_description;

  // loop for each item in config file
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem &item = config.items(index);
    if (item.name() == kModelPath) { // check item is model path
      const char* model_path = item.value().data();
      model_description.set_path(model_path);
    }
  }

  model_desc_vec.push_back(model_description);

  ret = ai_model_manager_->Init(config, model_desc_vec);
  if (ret != hiai::SUCCESS) { // check ai model manager init result
    HIAI_ENGINE_LOG(HIAI_GRAPH_INVALID_VALUE,
                    "[CarPlateDetection] fail to initialize AI model!");
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("[CarPlateDetection] end initialized!");
  return HIAI_OK;
}

void CarPlateDetection::BatchImageResize(
    shared_ptr<VideoDetectionImageParaT> &image_input,
    shared_ptr<VideoDetectionImageParaT> &image_output) {
  image_output->image = image_input->image;
  //image_output->box = image_input->box;

  // resize for each image
  for (vector<ObjectImageParaT>::iterator iter = image_input->obj_imgs.begin();
      iter != image_input->obj_imgs.end(); ++iter) {
    ascend::utils::DvppBasicVpcPara dvpp_basic_vpc_para;

    /**
     * when use dvpp_process only for resize function:
     *
     * 1.DVPP limits crop_right and crop_down should be Odd number,
     * if it is even number, subtract 1, otherwise Equal to origin width
     * or height.
     *
     * 2.crop_left and crop_up should be set to zero.
     */
    dvpp_basic_vpc_para.input_image_type = INPUT_YUV420_SEMI_PLANNER_UV; // nv12
    dvpp_basic_vpc_para.output_image_type = OUTPUT_YUV420SP_UV; // nv12
    dvpp_basic_vpc_para.src_resolution.width = (int) iter->img.width;
    dvpp_basic_vpc_para.src_resolution.height = (int) iter->img.height;
    dvpp_basic_vpc_para.dest_resolution.width = kDestImageWidth;
    dvpp_basic_vpc_para.dest_resolution.height = kDestImageHeight;
    // DVPP limits crop_left should be even number, 0 means without crop
    dvpp_basic_vpc_para.crop_left = 0;
    // DVPP limits crop_right should be Odd number
    dvpp_basic_vpc_para.crop_right =
        iter->img.width % 2 == 0 ? iter->img.width - 1 : iter->img.width;
    // DVPP limits crop_up should be even number, 0 means without crop
    dvpp_basic_vpc_para.crop_up = 0;
    // DVPP limits crop_down should be Odd number
    dvpp_basic_vpc_para.crop_down =
        iter->img.height % 2 == 0 ? iter->img.height - 1 : iter->img.height;
    dvpp_basic_vpc_para.is_input_align = true;

    ascend::utils::DvppProcess dvpp_process(dvpp_basic_vpc_para);

    ascend::utils::DvppVpcOutput dvpp_out;
    int ret = dvpp_process.DvppBasicVpcProc(iter->img.data.get(),
                                            (int32_t) iter->img.size,
                                            &dvpp_out);
    if (ret != ascend::utils::kDvppOperationOk) { // check call vpc result
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "[CarColorInferenceEngine] resize image failed with error code:%d",
          ret);
      continue;
    }

    std::shared_ptr<ObjectImageParaT> obj_image = std::make_shared<
        ObjectImageParaT>();

    obj_image->box = iter->box;
    obj_image->object_info.object_id = iter->object_info.object_id;
    obj_image->object_info.score = iter->object_info.score;
    obj_image->img.width = kDestImageWidth;
    obj_image->img.height = kDestImageHeight;
    obj_image->img.channel = iter->img.channel;
    obj_image->img.depth = iter->img.depth;
    obj_image->img.size = dvpp_out.size;
    obj_image->img.data.reset(dvpp_out.buffer,
                              std::default_delete<uint8_t[]>());
    image_output->obj_imgs.push_back(*obj_image);
  }
}

bool CarPlateDetection::CheckBBoxData(const OutputT &out_bbox,
                                      const OutputT &out_num) {
  // check out bbox is valid
  if (out_bbox.data == nullptr || out_bbox.data.get() == nullptr
      || out_num.data == nullptr || out_num.data.get() == nullptr) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[CarPlateDetection] car plate detection result is invalid!");
    return false;
  }

  return true;
}

HIAI_StatusT CarPlateDetection::PerformInference(
    shared_ptr<VideoDetectionImageParaT> &resized_image,
    shared_ptr<VideoDetectionImageParaT> &car_plate_image) {
  car_plate_image->image = resized_image->image;
  //car_plate_image->box = resized_image->box;

  // detect and crop car plate image from resized image
  for (vector<ObjectImageParaT>::iterator iter =
      resized_image->obj_imgs.begin(); iter != resized_image->obj_imgs.end();
      ++iter) {
    vector<OutputT> inference_result;
    
    // init neural buffer.
    shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer = shared_ptr<
        hiai::AINeuralNetworkBuffer>(
        new (nothrow) hiai::AINeuralNetworkBuffer(),
        default_delete<hiai::AINeuralNetworkBuffer>());
    if (neural_buffer.get() == nullptr) { // check new memory result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to new memory when initialize neural buffer!");
      printf("neual_buffer get failed\n");
      return HIAI_ERROR;
    }

    ImageData<u_int8_t> input_img = iter->img;
    neural_buffer->SetBuffer((void*) input_img.data.get(), input_img.size);

    shared_ptr<hiai::IAITensor> input_tensor = static_pointer_cast<
        hiai::IAITensor>(neural_buffer);

    vector<shared_ptr<hiai::IAITensor>> input_tensors;
    vector<shared_ptr<hiai::IAITensor>> output_tensors;
    input_tensors.push_back(input_tensor);

    HIAI_StatusT ret = hiai::SUCCESS;
    ret = ai_model_manager_->CreateOutputTensor(input_tensors, output_tensors);
    if (ret != hiai::SUCCESS) { // check create output tenser result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[CarPlateDetection] create output tensor failed!");
      printf("OutputTensor create failed\n");
      return HIAI_ERROR;
    }
    hiai::AIContext ai_context;

    // neural network inference
    ret = ai_model_manager_->Process(ai_context, input_tensors, output_tensors,
                                     0);
    if (ret != hiai::SUCCESS) { // check  neural network inference result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[CarPlateDetection] car plate detect inference failed!");
      printf("model process failed\n");
      return HIAI_ERROR;
    }

    // parse detection inference result and get car plate image
    for (uint32_t index = 0; index < output_tensors.size(); ++index) {
      shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor =
          static_pointer_cast<hiai::AINeuralNetworkBuffer>(
              output_tensors[index]);

      const char* res_data = reinterpret_cast<char*>(result_tensor->GetBuffer());
      
      printf("%x %x %x %x\n", res_data[0], res_data[1], res_data[2], res_data[3]);

      OutputT out;
      out.size = result_tensor->GetSize();
      // check data size is valid
      if (out.size <= 0 || out.size > kMaxNewMemory) {
        HIAI_ENGINE_LOG(
            HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
            "the inference output size:%d is invalid! value range: 1~134217728",
            out.size);
        printf("outsize is empty\n");
        return HIAI_ERROR;
      }

      out.data = std::shared_ptr<uint8_t>(new (nothrow) uint8_t[out.size],
                                          std::default_delete<uint8_t[]>());
      if (out.data.get() == nullptr) { // check new memory result
        HIAI_ENGINE_LOG(
            HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
            "Fail to new memory when handle ai model inference output!");
        printf("outdata is empty\n");
        return HIAI_ERROR;
      }

      errno_t ret = memcpy_s(out.data.get(), out.size,
                             result_tensor->GetBuffer(), out.size);
      if (ret != EOK) { // check memory copy result
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[CarPlateDetection] copy output tensor failure!");
        printf("memcpy_s is empty\n");
        continue;
      }

      inference_result.push_back(out);
    }

    // check inferece result is valid
    if (inference_result.empty()
        || inference_result.size() < kInferenceResultSize) {
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "[CarPlateDetection] detection result tensor is empty or invalid!");
      printf("inference_result is empty\n");
      continue;
    }

    OutputT out_num = inference_result[kInferenceOutputNum];
    OutputT out_bbox = inference_result[kInferenceOutputBBox];

    // check out bbox is valid
    if (!CheckBBoxData(out_bbox, out_num)) {
      printf("checkBBoxData failed\n");
      return HIAI_ERROR;
    }

    float* bbox_buffer = reinterpret_cast<float*>(out_bbox.data.get());
    float bbox_number = *reinterpret_cast<float*>(out_num.data.get());
    int32_t bbox_buffer_size = bbox_number * kSizePerResultset;
    HIAI_ENGINE_LOG("[CarPlateDetection] the number of bbox:%d", bbox_number);
    printf("[CarPlateDetection] the number of bbox:%f\n", bbox_number);
    printf("[CarPlateDetection] the bbox_buffer_size:%d\n", bbox_buffer_size);

    float* ptr = bbox_buffer;

    uint32_t base_width = iter->img.width;
    uint32_t base_height = iter->img.height;

    // get every car plate image and push it to result vector
    for (int32_t k = 0; k < bbox_buffer_size; k += kSizePerResultset) {
      ptr = bbox_buffer + k;
      float score = ptr[BBoxDataIndex::kScore];

      if (score < kMinInferenceScore) { // check score is good enough
        HIAI_ENGINE_LOG(
            "[CarPlateDetection] car plate detection score:%f is too less",
            score);
        printf("score is failed\n");
        printf("score is %f\n",score);
        continue;
      }

      // bbox coordinate should between 0.0f and 1.0f
      uint32_t lt_x = CorrectCoordinate(
          kTopLeftCoeff * ptr[BBoxDataIndex::kTopLeftX]) * base_width, lt_y =
          CorrectCoordinate(kTopLeftCoeff * ptr[BBoxDataIndex::kTopLeftY])
              * base_height, rb_x = CorrectCoordinate(
          kLowerRightCoeff * ptr[BBoxDataIndex::kLowerRightX]) * base_width,
          rb_y = CorrectCoordinate(
              kLowerRightCoeff * ptr[BBoxDataIndex::kLowerRightY])
              * base_height;

      // check cropped image size is valid
      if (rb_x - lt_x < kMinCropPixel || rb_y - lt_x < kMinCropPixel) {
        HIAI_ENGINE_LOG(
            "[CarPlateDetection] the car plate image is too small!");
        printf("zuo biao dian  you wen ti\n");
        continue;
      }

      // crop image
      ObjectImageParaT object_image;
      BoundingBox bbox = { lt_x, lt_y, rb_x, rb_y };
      HIAI_StatusT crop_ret = CropObjectFromImage(iter->img, object_image.img,
                                                  bbox);
      if (crop_ret != HIAI_OK) { // check crop result
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[CarPlateDetection] fail to crop car plate image!");
        printf("CropObjectFromImage failed\n");
        continue;
      }

      object_image.object_info = iter->object_info;
      object_image.box = iter->box;
      car_plate_image->obj_imgs.push_back(object_image);
	    //car_plate_image->box = bbox;
      printf("%d,%d,%d,%d\n",iter->box.lt_x,iter->box.lt_y,iter->box.rb_x,iter->box.rb_y);
    }
  }

  return HIAI_OK;
}

float CarPlateDetection::CorrectCoordinate(float value) {
  float tmp = value < kLowerCoord ? kLowerCoord : value;
  return tmp > kUpperCoord ? kUpperCoord : tmp;
}

HIAI_StatusT CarPlateDetection::CropObjectFromImage(
    const ImageData<u_int8_t> &src_img, ImageData<u_int8_t> &target_img,
    const BoundingBox &bbox) {
  ascend::utils::DvppBasicVpcPara dvpp_basic_vpc_para;

  /**
   * when use dvpp_process only for resize function:
   *
   * 1.DVPP limits crop_right and crop_down should be Odd number,
   * if it is even number, subtract 1, otherwise Equal to origin width
   * or height.
   *
   * 2.crop_left and crop_up should be set to zero.
   */
  dvpp_basic_vpc_para.input_image_type = INPUT_YUV420_SEMI_PLANNER_UV; // nv12
  dvpp_basic_vpc_para.output_image_type = OUTPUT_YUV420SP_UV; // nv12
  dvpp_basic_vpc_para.src_resolution.width = src_img.width;
  dvpp_basic_vpc_para.src_resolution.height = src_img.height;

  // the value of horz_max and vert_max must be odd and
  // horz_min and vert_min must be even
  int crop_horz_min = bbox.lt_x % 2 == 0 ? bbox.lt_x : bbox.lt_x + 1;
  int crop_horz_max = bbox.rb_x % 2 == 0 ? bbox.rb_x - 1 : bbox.rb_x;
  int crop_vert_min = bbox.lt_y % 2 == 0 ? bbox.lt_y : bbox.lt_y + 1;
  int crop_vert_max = bbox.rb_y % 2 == 0 ? bbox.rb_y - 1 : bbox.rb_y;

  // calculate cropped image width and height.
  int dest_width = crop_horz_max - crop_horz_min + 1;
  int dest_height = crop_vert_max - crop_vert_min + 1;

  // dest width and height should be odd values
  int dest_resolution_width = dest_width % 2 == 0 ? dest_width : dest_width + 1;
  int dest_resolutiont_height =
      dest_height % 2 == 0 ? dest_height : dest_height + 1;
  dvpp_basic_vpc_para.dest_resolution.width = dest_resolution_width;
  dvpp_basic_vpc_para.dest_resolution.height = dest_resolutiont_height;
  // DVPP limits crop_left should be even number
  dvpp_basic_vpc_para.crop_left = crop_horz_min;
  // DVPP limits crop_right should be Odd number
  dvpp_basic_vpc_para.crop_right = crop_horz_max;
  // DVPP limits crop_up should be even number
  dvpp_basic_vpc_para.crop_up = crop_vert_min;
  // DVPP limits crop_down should be Odd number
  dvpp_basic_vpc_para.crop_down = crop_vert_max;
  dvpp_basic_vpc_para.is_input_align = true;

  ascend::utils::DvppProcess dvpp_process(dvpp_basic_vpc_para);

  ascend::utils::DvppVpcOutput dvpp_out;
  int ret = dvpp_process.DvppBasicVpcProc(src_img.data.get(),
                                          (int32_t) src_img.size, &dvpp_out);
  if (ret != kDvppProcSuccess) { // check call vpc result
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "[CarPlateDetection] crop image failed with error code:%d !", ret);
    return HIAI_ERROR;
  }

  target_img.channel = src_img.channel;
  target_img.format = src_img.format;
  target_img.data.reset(dvpp_out.buffer, default_delete<uint8_t[]>());
  target_img.width = dest_resolution_width;
  target_img.height = dest_resolutiont_height;
  target_img.size = dvpp_out.size;

  return HIAI_OK;
}

HIAI_StatusT CarPlateDetection::SendDetectionResult(
    shared_ptr<VideoDetectionImageParaT> &detection_trans) {
  HIAI_StatusT ret;
  do {
     //send data to next engine.
    ret = SendData(kOutputPort, "VideoDetectionImageParaT",
                   static_pointer_cast<void>(detection_trans));
    if (ret == HIAI_QUEUE_FULL) { // check send data result
      HIAI_ENGINE_LOG("[CarPlateDetection] output queue full");
      usleep(kSleep20MicroSecs); // sleep 20ms
    }
  } while (ret == HIAI_QUEUE_FULL); // loop for send data when queue is full

  if (ret != HIAI_OK) { // check send result
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateDetection] send inference data failed!");
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("car_plate_detection", CarPlateDetection, INPUT_SIZE) {
  HIAI_ENGINE_LOG("[CarPlateDetection] start process!");

  if (arg0 == nullptr) { // check input data is nullptr
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateDetection] input data is null!");
    return HIAI_ERROR;
  }

  shared_ptr<VideoDetectionImageParaT> image_input = static_pointer_cast<
      VideoDetectionImageParaT>(arg0);

  //ԭͼƬ������Ҫչʾ��
  if (image_input->obj_imgs.empty()) { // check input vector is empty
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateDetection] the cropped image vector is empty!");
	  image_input->status = false;
    SendDetectionResult(image_input);
	  return HIAI_ERROR;
  }
  else
  {
    printf("not not not empty-----car_plate_detection\n");
  }

  shared_ptr<VideoDetectionImageParaT> resized_image = std::make_shared<
      VideoDetectionImageParaT>();

  // resize input image ͼƬ����ģ�������и�ŵ�resized_image����
  BatchImageResize(image_input, resized_image);
  if (resized_image->obj_imgs.empty()) 
  {
    printf("BatchImageResize is empty!!!\n");
  }
  else
  {
    printf("BatchImageResize is not not not not not empty!!!\n");
  }
  // detect and crop car plate image from input image
  shared_ptr<VideoDetectionImageParaT> car_plate_image = std::make_shared<
      VideoDetectionImageParaT>();

  // check car plate image detection result
  if (PerformInference(resized_image, car_plate_image) != HIAI_OK) {
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateDetection] fail to detect car plate!");
	  car_plate_image->status = false;
	  SendDetectionResult(car_plate_image);
    return HIAI_ERROR;
  }
  car_plate_image->status = true;
  // send car plate image to next engine
  return SendDetectionResult(car_plate_image);
}
