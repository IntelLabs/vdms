
/**
 * @file   VDMSConfigHelper.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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
#include <algorithm>
#include <iostream>
#include <limits.h>
#include <map>
#include <string>

#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>

namespace VDMS {

// E N U M S
enum class StorageType { LOCAL = 0, AWS = 1, INVALID_TYPE = INT_MAX };

// C O N S T A N T S
const std::map<std::string, StorageType> storage_types_map = {
    {"local", StorageType::LOCAL}, {"aws", StorageType::AWS}};

const std::map<std::string, Aws::Utils::Logging::LogLevel> aws_log_level_map = {
    {"off", Aws::Utils::Logging::LogLevel::Off},
    {"fatal", Aws::Utils::Logging::LogLevel::Fatal},
    {"error", Aws::Utils::Logging::LogLevel::Error},
    {"warn", Aws::Utils::Logging::LogLevel::Warn},
    {"info", Aws::Utils::Logging::LogLevel::Info},
    {"debug", Aws::Utils::Logging::LogLevel::Debug},
    {"trace", Aws::Utils::Logging::LogLevel::Trace}};

} // namespace VDMS
