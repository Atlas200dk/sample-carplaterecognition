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

#include "car_plate_recognition.h"

#include <thread>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <vector>

#include <unistd.h>

#include <hiaiengine/log.h>
#include <hiaiengine/ai_types.h>
#include "hiaiengine/ai_model_parser.h"

using hiai::Engine;
using namespace std;

namespace {
// car plate characters
const string kCarPlateChars[65] = { "京", "沪", "津", "渝", "冀", "晋", "蒙", "辽", "吉",
    "黑", "苏", "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂", "琼", "川", "贵",
    "云", "藏", "陕", "甘", "青", "宁", "新", "0", "1", "2", "3", "4", "5", "6", "7",
    "8", "9", "A", "B", "C", "D", "E", "F", "G", "H", "J", "K", "L", "M", "N",
    "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };

// time for waitting when send queue is full
const int kWaitTime = 20000;

// the image width for model
const int kDestImageWidth = 272;

// the image height for model
const int kDestImageHeight = 72;

// car plate number length
const int kCarPlateLength = 7;

// standard: 1024 * 1024 * 128 = 134217728 (128M)
const int kMaxNewMemory = 134217728;

// the name of model_path in the config file
const string kModelPathItemName = "model_path";

// the name of passcode in the config file
const string kPasscodeItemName = "passcode";

// the name of batch_size in the config file
const string kBatchSizeItemName = "batch_size";
}

//HIAI_REGISTER_DATA_TYPE("BatchCarInfoT", BatchCarInfoT);
//HIAI_REGISTER_DATA_TYPE("VideoImageInfoT", VideoImageInfoT);
//HIAI_REGISTER_DATA_TYPE("CarInfoT", CarInfoT);
//HIAI_REGISTER_DATA_TYPE("VideoDetectionImageParaT", VideoDetectionImageParaT);

HIAI_StatusT CarPlateRecognition::Init(
    const hiai::AIConfig &config,
    const std::vector<hiai::AIModelDescription> &model_desc) {
  HIAI_ENGINE_LOG("[CarPlateRecognition] start init!");
  hiai::AIStatus ret = hiai::SUCCESS;

  if (ai_model_manager_ == nullptr) { // check ai model manager is nullptr
    ai_model_manager_ = std::make_shared<hiai::AIModelManager>();
  }

  std::vector<hiai::AIModelDescription> model_desc_vec;
  hiai::AIModelDescription model_description;

  // loop for each item
  for (int index = 0; index < config.items_size(); ++index) {
    const ::hiai::AIConfigItem &item = config.items(index);
    if (item.name() == kModelPathItemName) { // get model path
      const char* model_path = item.value().data();
      model_description.set_path(model_path);
    } else if (item.name() == kPasscodeItemName) { // get pass code
      const char* passcode = item.value().data();
      model_description.set_key(passcode);
    } else if (item.name() == kBatchSizeItemName) { // get bitch size
      std::stringstream ss(item.value());
      ss >> batch_size_;
    }
  }

  if (batch_size_ < 1) { // check batch_size_ is valid, it should bigger than 1
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateRecognition] invalid batch_size:%d", batch_size_);
    return HIAI_ERROR;
  }

  model_desc_vec.push_back(model_description);
  ret = ai_model_manager_->Init(config, model_desc_vec);
  if (ret != hiai::SUCCESS) { // check ai model manager initialize result
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateRecognition] fail to initialize ai model");
    return HIAI_ERROR;
  }

  HIAI_ENGINE_LOG("[CarPlateRecognition] end init!");
  return HIAI_OK;
}

HIAI_StatusT CarPlateRecognition::SendResultData(
    const std::shared_ptr<BatchCarInfoT> &tran_data) {
  HIAI_StatusT hiai_ret = HIAI_OK;

  do {
    // this engine only have one out port, the port number set to zero
    hiai_ret = SendData(0, "BatchCarInfoT",
                        std::static_pointer_cast<void>(tran_data));
    if (hiai_ret == HIAI_QUEUE_FULL) { // check send data result
      HIAI_ENGINE_LOG("[CarPlateRecognition] the queue for send data is full");
      usleep(kWaitTime); // sleep 20ms
    }
  } while (hiai_ret == HIAI_QUEUE_FULL); // loop for send data when queue full

  if (hiai_ret != HIAI_OK) { // check send data result
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateRecognition] send data to next engine failed!");
    return HIAI_ERROR;
  }

  return HIAI_OK;
}

void CarPlateRecognition::BatchImageResize(
    std::shared_ptr<VideoDetectionImageParaT> &batch_image_input,
    std::shared_ptr<VideoDetectionImageParaT> &batch_image_output) {
  batch_image_output->image = batch_image_input->image;
  //batch_image_output->box = batch_image_input->box;

  // resize for each image
  for (std::vector<ObjectImageParaT>::iterator iter = batch_image_input
      ->obj_imgs.begin(); iter != batch_image_input->obj_imgs.end(); ++iter) {
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
    if (ret != ascend::utils::kDvppOperationOk) { // check resize result
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "[CarPlateRecognition] resize image failed with result:%d !", ret);
      continue;
    }

    std::shared_ptr<ObjectImageParaT> obj_image = std::make_shared<
        ObjectImageParaT>();

    obj_image->object_info.object_id = iter->object_info.object_id;
    obj_image->object_info.score = iter->object_info.score;
    obj_image->box = iter->box;
    obj_image->img.width = kDestImageWidth;
    obj_image->img.height = kDestImageHeight;
    obj_image->img.channel = iter->img.channel;
    obj_image->img.depth = iter->img.depth;
    obj_image->img.size = dvpp_out.size;
    obj_image->img.data.reset(dvpp_out.buffer,
                              std::default_delete<uint8_t[]>());
    batch_image_output->obj_imgs.push_back(*obj_image);
  }
}

bool CarPlateRecognition::ConstructBatchBuffer(
    int batch_index,
    const std::shared_ptr<VideoDetectionImageParaT> &image_handle,
    uint8_t* temp) {
  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);

  //loop for each image in one batch size
  for (int j = 0; j < batch_size_; j++) {
    if (batch_index + j < image_number) { // check is valid image number
      errno_t memcpy_ret = memcpy_s(
          temp + j * image_size, image_size,
          image_handle->obj_imgs[batch_index + j].img.data.get(), image_size);
      if (memcpy_ret != EOK) { // check memory copy result
        HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                        "[CarPlateRecognition] copy image buffer failed");
        return false;
      }
    } else { //invalid image number, padding value
      errno_t err = memset_s(temp + j * image_size, image_size,
                             static_cast<char>(0), image_size);
      if (err != EOK) { // check memory set result
        HIAI_ENGINE_LOG(
            HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
            "[CarPlateRecognition] batch padding for image data failed");
        return false;
      }
    }
  }

  return true;
}

bool CarPlateRecognition::ConstructInferenceResult(
    const std::vector<std::shared_ptr<hiai::IAITensor>> &output_data_vec,
    int batch_index,
    const std::shared_ptr<VideoDetectionImageParaT> &image_handle,
    const std::shared_ptr<BatchCarInfoT> &tran_data) {

  if (output_data_vec.empty()) { // check the output data vector is empty
    HIAI_ENGINE_LOG("[CarPlateRecognition] output_data_vec is empty!");
    return false;
  }

  int image_number = image_handle->obj_imgs.size();
  for (int ind = 0; ind < 1; ind++) { // loop for each batch
    if (batch_index + ind >= image_number) { // check is valid image number
      break;
    }

    string car_palte_str = "";
    float car_plate_score = 0;
    // loop for each output data vector, the vector size is 7
    for (int i_vec = 0; i_vec < output_data_vec.size(); ++i_vec) {
      shared_ptr<hiai::AINeuralNetworkBuffer> result_tensor =
          static_pointer_cast<hiai::AINeuralNetworkBuffer>(
              output_data_vec[i_vec]);
      //get confidence result
      int size = result_tensor->GetSize() / sizeof(float);
      float* result = (float*) result_tensor->GetBuffer();

      int max_index = 0;
      int oneResultSize = size / batch_size_;
      for (int i = 0; i < oneResultSize; i++) { // loop for each result value
        if (*(result + i) > *(result + max_index)) { // get max value index
          max_index = i;
        }
      }

      car_palte_str += kCarPlateChars[max_index];
      printf("max_index %d\n",max_index);
      printf("car_plate_str if for is %s\n",kCarPlateChars[max_index].c_str());
      car_plate_score += *(result + max_index);
    }

    CarInfoT out; // create car plate recognition result data
    out.box = image_handle->obj_imgs[batch_index + ind].box;
    //out.attribute_name = kCarPlateStr;
    out.inference_result = car_palte_str;
    printf("car_plate_str is %s\n",car_palte_str.c_str());
    // get the mean value of 7 car plate number inference score
    out.confidence = car_plate_score / kCarPlateLength;
	  //out.box = image_handle->box;

    HIAI_ENGINE_LOG("[CarPlateRecognition] car plate number:%s",
                    car_palte_str.c_str());

    tran_data->car_infos.push_back(out);
  }

  if (tran_data->car_infos.empty()) { // check car_infos vector is empty
    HIAI_ENGINE_LOG("[CarPlateRecognition] car_infos vector is empty!");
    return false;
  }

  return true;
}

HIAI_StatusT CarPlateRecognition::BatchInferenceProcess(
    const std::shared_ptr<VideoDetectionImageParaT> &image_handle,
    std::shared_ptr<BatchCarInfoT> &tran_data) {
  HIAI_ENGINE_LOG("[CarPlateRecognition] start inference process!");

  hiai::AIStatus ret = hiai::SUCCESS;
  HIAI_StatusT hiai_ret = HIAI_OK;
  int image_number = image_handle->obj_imgs.size();
  int image_size = image_handle->obj_imgs[0].img.size * sizeof(uint8_t);
  int batch_buffer_size = image_size * batch_size_;

  // check data size is valid
  if (batch_buffer_size <= 0 || batch_buffer_size > kMaxNewMemory) {
    HIAI_ENGINE_LOG(
        HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
        "the batch buffer size:%d is invalid! value range: 1~134217728",
        batch_buffer_size);
    return HIAI_ERROR;
  }

  // the loop for each batch ,maybe image_number greater than batch_size_
  for (int i = 0; i < image_number; i += batch_size_) {
    std::vector<std::shared_ptr<hiai::IAITensor> > input_data_vec;
    std::vector<std::shared_ptr<hiai::IAITensor> > output_data_vec;

    if (tran_data == nullptr) { // check tran_data is nullptr
      tran_data = std::make_shared<BatchCarInfoT>();
      tran_data->image = image_handle->image;
    }

    //1.prepare input buffer for each batch
    uint8_t* temp = new (nothrow) uint8_t[batch_buffer_size];
    if (temp == nullptr) { // check new memory result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to new memory for input buffer!");
      return HIAI_ERROR;
    }

    if (!ConstructBatchBuffer(i, image_handle, temp)) { // construct input data
      HIAI_ENGINE_LOG(
          HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
          "[CarPlateRecognition] batch input buffer construct failed!");
      delete[] temp;
      return HIAI_ERROR;
    }

    std::shared_ptr<hiai::AINeuralNetworkBuffer> neural_buffer =
        std::shared_ptr<hiai::AINeuralNetworkBuffer>(
            new (nothrow) hiai::AINeuralNetworkBuffer());
    if (neural_buffer.get() == nullptr) { // check new memory result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "Fail to new memory when initialize neural buffer!");
      return HIAI_ERROR;
    }

    neural_buffer->SetBuffer((void*) (temp), batch_buffer_size);
    std::shared_ptr<hiai::IAITensor> input_data = std::static_pointer_cast<
        hiai::IAITensor>(neural_buffer);
    input_data_vec.push_back(input_data);

    // 2.Call ai model manager process, predict
    ret = ai_model_manager_->CreateOutputTensor(input_data_vec,
                                                output_data_vec);
    if (ret != hiai::SUCCESS) { // check create output tensor result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[CarPlateRecognition] CreateOutputTensor failed");
      delete[] temp;
      return HIAI_ERROR;
    }

    hiai::AIContext ai_context;
    HIAI_ENGINE_LOG("[CarPlateRecognition] ai_model_manager_->Process start!");
    ret = ai_model_manager_->Process(ai_context, input_data_vec,
                                     output_data_vec, 0);
    delete[] temp;
    if (ret != hiai::SUCCESS) { // check ai mode inference result
      HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                      "[CarPlateRecognition] ai_model_manager Process failed");
      return HIAI_ERROR;
    }

    input_data_vec.clear();
	  ConstructInferenceResult(output_data_vec, i, image_handle,
                                  tran_data);
  }
  //4. send ai model inference result to next engine
  tran_data->status = true;
  hiai_ret = SendResultData(tran_data);
  if (hiai_ret != HIAI_OK) { // check send data result
  HIAI_ENGINE_LOG("[CarPlateRecognition] send data failed! error code:%d",
				   hiai_ret);
  }

  HIAI_ENGINE_LOG("[CarPlateRecognition] end process!");
  return HIAI_OK;
}

HIAI_IMPL_ENGINE_PROCESS("car_plate_recognition", CarPlateRecognition,
    INPUT_SIZE) {
  if (arg0 == nullptr) { // check input data
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateRecognition] input data is null!");
    return HIAI_ERROR;
  }

  shared_ptr<VideoDetectionImageParaT> image_input = static_pointer_cast<
      VideoDetectionImageParaT>(arg0);
  std::shared_ptr<BatchCarInfoT> tran_data = std::make_shared<BatchCarInfoT>();
  // origin image information is transmitted to next Engine directly
  tran_data->image = image_input->image;

  // the input data is finished
  //if (image_input->video_image_info.is_finished) {
  //  HIAI_ENGINE_LOG("[CarPlateRecognition] get is_finished data!");
  //  return SendResultData(tran_data);
  //}

  if (image_input->obj_imgs.empty()) { // check input vector is empty
    HIAI_ENGINE_LOG(HIAI_ENGINE_RUN_ARGS_NOT_RIGHT,
                    "[CarPlateRecognition] input object image is empty!");
	tran_data->status = false;
	SendResultData(tran_data);
    return HIAI_ERROR;
  }
  else
  {
    printf("not not not not empty------car_plate_recognition\n");
  }

  std::shared_ptr<VideoDetectionImageParaT> image_handle = std::make_shared<
      VideoDetectionImageParaT>();

  // resize input image;
  BatchImageResize(image_input, image_handle);
  if (image_handle->obj_imgs.empty()) { // check result vector is empty
    HIAI_ENGINE_LOG("[CarPlateRecognition] image_input resize failed");
    printf("print ResultData\n");
    tran_data->status = false;
	  SendResultData(tran_data);
    return HIAI_ERROR;
  }
  printf("print inference result\n");
  // inference and send inference result;
  return BatchInferenceProcess(image_handle, tran_data);
}
