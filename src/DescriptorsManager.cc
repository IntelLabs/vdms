/**
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "DescriptorsManager.h"
#include <iostream>

using namespace VDMS;

DescriptorsManager *DescriptorsManager::_dm;

bool DescriptorsManager::init() {
  if (_dm)
    return false;

  _dm = new DescriptorsManager();
  return true;
}

DescriptorsManager *DescriptorsManager::instance() {
  if (_dm)
    return _dm;

  std::cerr << "ERROR: DescriptorsManager not init" << std::endl;
  return NULL;
}

DescriptorsManager::DescriptorsManager() {}

void DescriptorsManager::flush() {
  for (auto desc_set : _descriptors_handlers) {
    desc_set.second->store();
    delete desc_set.second;
  }
  _descriptors_handlers.clear();
}

VCL::DescriptorSet *
DescriptorsManager::get_descriptors_handler(std::string path) {
  VCL::DescriptorSet *desc_ptr;

  auto element = _descriptors_handlers.find(path);

  if (element == _descriptors_handlers.end()) {
    desc_ptr = new VCL::DescriptorSet(path);
    _descriptors_handlers[path] = desc_ptr;
  } else {
    desc_ptr = element->second;
  }

  return desc_ptr;
}
