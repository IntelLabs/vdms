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

#pragma once

#include <thread>
#include <vector>
#include <mutex>
#include <queue>

#include "VCL.h"

namespace VDMS {

    class DescriptorsManager
    {
        static DescriptorsManager* _dm;
        std::map<std::string, VCL::DescriptorSet* > _descriptors_handlers;

        // Need this lock till we have concurrency support in JL
        // TODO: Make this reader writer.
        std::mutex _lock;

        DescriptorsManager();

    public:

        static bool init();
        static DescriptorsManager* instance();

        /**
         *  Handles descriptors and lock for the descriptor
         *  This is a blocking call until the descriptor is free
         *
         *  @param path  Path to the descriptor set
         */
        VCL::DescriptorSet* get_descriptors_handler(std::string path);
        void flush();
    };
};
