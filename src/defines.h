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

// This defines should be included into *Command.cc files
// (RSCommand.cc, ImageCommand.cc, DescriptorsCommand.cc)

/* Some conventions:
* Must start with VD: (not VDMS since we have a 16 char limit)
* Tags (for nodes and edges) are all upper case.
* Properties are cammel case, where the first word is lower case.
*/

// General

#define VDMS_GENERIC_LINK   "VD:LINK"

// Entities

#define VDMS_EN_BLOB_PATH_PROP "VD:blobPath"

// Images

#define VDMS_IM_TAG         "VD:IMG"
#define VDMS_IM_EDGE_TAG    "VD:IMGLINK"
#define VDMS_IM_PATH_PROP   "VD:imgPath"

// Descriptor Set

#define VDMS_DESC_SET_TAG       "VD:DESCSET"
#define VDMS_DESC_SET_EDGE_TAG  "VD:DESCSETLINK" // link between set and desc
#define VDMS_DESC_SET_PATH_PROP "VD:descSetPath"
#define VDMS_DESC_SET_NAME_PROP "VD:name"
#define VDMS_DESC_SET_DIM_PROP  "VD:dimensions"

// Descriptor

#define VDMS_DESC_TAG             "VD:DESC"
#define VDMS_DESC_EDGE_TAG        "VD:DESCLINK"
#define VDMS_DESC_LABEL_PROP      "VD:label"
#define VDMS_DESC_ID_PROP         "VD:descId"

#define VDMS_DESC_LABEL_TAG       "VD:DESCLABEL"
#define VDMS_DESC_LABEL_NAME_PROP "VD:labelName"
#define VDMS_DESC_LABEL_ID_PROP   "VD:labelId"

// Regions

#define VDMS_ROI_TAG        "VD:ROI"
#define VDMS_ROI_EDGE_TAG   "VD:ROILINK"
#define VDMS_ROI_IMAGE_EDGE "VD:ROIIMGLINK"

#define VDMS_ROI_COORD_X_PROP "VD:x1"
#define VDMS_ROI_COORD_Y_PROP "VD:y1"
#define VDMS_ROI_WIDTH_PROP   "VD:width"
#define VDMS_ROI_HEIGHT_PROP  "VD:height"

// Videos

#define VDMS_VID_TAG           "VD:VID"
#define VDMS_VID_EDGE          "VD:VIDLINK"
#define VDMS_VID_PATH_PROP     "VD:videoPath"

// Key frames (KF)

#define VDMS_KF_TAG       "VD:KF"
#define VDMS_KF_EDGE      "VD:KFLINK"
#define VDMS_KF_IDX_PROP  "VD:frameIndex"
#define VDMS_KF_BASE_PROP "VD:frameBase"
