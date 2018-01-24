/**
 * @file   APISchema.h
 *
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

const std::string schema_json(" {   \"description\": \"VDMS API\", \
   \"type\": \"array\", \
   \"minItems\": 1, \
   \"items\": { \
     \"anyOf\": [ \
       { \"$ref\": \"#/definitions/AddEntityTop\" }, \
       { \"$ref\": \"#/definitions/FindEntityTop\" }, \
       { \"$ref\": \"#/definitions/ConnectTop\" }, \
       { \"$ref\": \"#/definitions/AddImageTop\" }, \
       { \"$ref\": \"#/definitions/FindImageTop\" } \
     ] \
   }, \
   \"uniqueItems\": false, \
   \"definitions\": { \
     \"positiveInt\": { \
       \"type\": \"integer\", \
       \"minimum\": 1 \
     }, \
     \"nonNegativeInt\": { \
       \"type\": \"integer\", \
       \"minimum\": 0 \
     }, \
     \"stringArray\": { \
       \"type\": \"array\", \
       \"items\": {\"type\": \"string\"}, \
       \"minimum\": 1 \
     }, \
     \"blockOperations\": { \
       \"type\": \"array\", \
       \"minItems\": 1, \
       \"items\": { \
         \"anyOf\": [ \
           { \"$ref\": \"#/definitions/operationThreshold\" }, \
           { \"$ref\": \"#/definitions/operationResize\" }, \
           { \"$ref\": \"#/definitions/operationCrop\" } \
         ] \
       }, \
       \"uniqueItems\": false \
     }, \
     \"operationThreshold\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"type\": { \"enum\": [ \"threshold\" ] }, \
         \"value\" : { \"$ref\": \"#/definitions/nonNegativeInt\" } \
       }, \
       \"required\": [\"type\", \"value\"], \
       \"additionalProperties\": false \
     }, \
     \"operationResize\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"type\": { \"enum\": [ \"resize\" ] }, \
         \"height\" : { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"width\"  : { \"$ref\": \"#/definitions/positiveInt\" } \
       }, \
       \"required\": [\"type\", \"height\", \"width\"], \
       \"additionalProperties\": false \
     }, \
     \"operationCrop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"type\": { \"enum\": [ \"crop\" ] }, \
         \"x\" : { \"$ref\": \"#/definitions/nonNegativeInt\" }, \
         \"y\" : { \"$ref\": \"#/definitions/nonNegativeInt\" }, \
         \"height\" : { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"width\"  : { \"$ref\": \"#/definitions/positiveInt\" } \
       }, \
       \"required\": [\"type\", \"x\", \"y\", \"height\", \"width\"], \
       \"additionalProperties\": false \
     }, \
     \"AddEntityTop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"AddEntity\" : { \"$ref\": \"#/definitions/AddEntity\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"FindEntityTop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"FindEntity\" : { \"$ref\": \"#/definitions/FindEntity\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"ConnectTop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"Connect\" : { \"$ref\": \"#/definitions/Connect\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"AddImageTop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"AddImage\" : { \"$ref\": \"#/definitions/AddImage\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"FindImageTop\": { \
       \"type\": \"object\", \
       \"properties\": { \
         \"FindImage\" : { \"$ref\": \"#/definitions/FindImage\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"AddEntity\": { \
       \"properties\": { \
         \"class\": { \"type\": \"string\" }, \
         \"_ref\": { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"properties\": { \"type\": \"object\" }, \
         \"constraints\": { \"type\": \"object\" } \
       }, \
       \"required\": [\"class\"], \
       \"additionalProperties\": false \
     }, \
     \"Connect\": { \
       \"properties\": { \
         \"class\": { \"type\": \"string\" }, \
         \"ref1\": { \"type\": \"number\" }, \
         \"ref2\": { \"type\": \"number\" }, \
         \"properties\": { \"type\": \"object\" } \
       }, \
       \"required\": [\"class\", \"ref1\", \"ref2\"], \
       \"additionalProperties\": false \
     }, \
     \"FindEntity\": { \
       \"properties\": { \
         \"class\": { \"type\": \"string\" }, \
         \"_ref\": { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"constraints\": { \"type\": \"object\" }, \
         \"results\": { \"type\": \"object\" }, \
         \"link\": { \"type\": \"object\" }, \
         \"unique\": { \"type\": \"boolean\" } \
       }, \
       \"required\": [\"class\"], \
       \"additionalProperties\": false \
     }, \
     \"AddImage\": { \
       \"properties\": { \
         \"_ref\": { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"format\": { \"type\": \"string\" }, \
         \"link\": { \"type\": \"object\" }, \
         \"operations\": { \"$ref\": \"#/definitions/blockOperations\" }, \
         \"properties\": { \"type\": \"object\" }, \
         \"collections\": { \"$ref\": \"#/definitions/stringArray\" } \
       }, \
       \"additionalProperties\": false \
     }, \
     \"FindImage\": { \
       \"properties\": { \
         \"_ref\": { \"$ref\": \"#/definitions/positiveInt\" }, \
         \"constraints\": { \"type\": \"object\" }, \
         \"operations\": { \"$ref\": \"#/definitions/blockOperations\" }, \
         \"results\": { \"type\": \"object\" }, \
         \"unique\": { \"type\": \"boolean\" } \
       }, \
       \"additionalProperties\": false \
     } \
   } \
 } ");
