// Copyright (c) 2021 PaddlePaddle Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "model_deploy/ppdet/include/det_model.h"
#include "model_deploy/ppdet/include/det_standard_config.h"

namespace PaddleDeploy {

bool DetModel::GenerateTransformsConfig(const YAML::Node& src) {
  assert(src["Preprocess"].IsDefined());
  assert(src["arch"].IsDefined());
  std::string model_arch = src["arch"].as<std::string>();
//  yaml_config_["transforms"]["BGR2RGB"]["null"] = true;
  for (const auto& op : src["Preprocess"]) {
    assert(op["type"].IsDefined());
    std::string op_name = op["type"].as<std::string>();
    if (op_name == "Normalize") {
      DetNormalize(op, &yaml_config_);
    } else if (op_name == "Permute") {
      DetPermute(op, &yaml_config_);
    } else if (op_name == "Resize") {
      DetResize(op, &yaml_config_, model_arch);
    } else if (op_name == "PadStride") {
      DetPadStride(op, &yaml_config_);
    } else {
      std::cerr << "Unexpected transforms op name: '"
                << op_name << "'" << std::endl;
      return false;
    }
  }
  return true;
}

bool DetModel::YamlConfigInit(const std::string& cfg_file) {
  YAML::Node det_config = YAML::LoadFile(cfg_file);

  yaml_config_["model_format"] = "Paddle";
  // arch support value:YOLO, SSD, RetinaNet, RCNN, Face
  if (!det_config["arch"].IsDefined()) {
    std::cerr << "Fail to find arch in PaddleDection yaml file" << std::endl;
    return false;
  } else if (!det_config["label_list"].IsDefined()) {
    std::cerr << "Fail to find label_list in "
              << "PaddleDection yaml file"
              << std::endl;
    return false;
  }
  yaml_config_["model_name"] = det_config["arch"].as<std::string>();
  yaml_config_["toolkit"] = "PaddleDetection";
  yaml_config_["toolkit_version"] = "Unknown";

  int i = 0;
  for (const auto& label : det_config["label_list"]) {
    yaml_config_["labels"][i] = label.as<std::string>();
    i++;
  }

  // Generate Standard Transforms Configuration
  if (!GenerateTransformsConfig(det_config)) {
    std::cerr << "Fail to generate standard configuration "
              << "of tranforms" << std::endl;
    return false;
  }
  return true;
}

bool DetModel::PreProcessInit() {
  preprocess_ = std::make_shared<DetPreProcess>();
  if (!preprocess_->Init(yaml_config_))
    return false;
  return true;
}

bool DetModel::PostProcessInit(bool use_cpu_nms) {
  postprocess_ = std::make_shared<DetPostProcess>();
  if (!postprocess_->Init(yaml_config_, use_cpu_nms))
    return false;
  return true;
}

}  // namespace PaddleDeploy
