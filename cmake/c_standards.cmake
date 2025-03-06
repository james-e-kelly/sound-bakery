#
# Copyright (C) 2021 by George Cave - gcave@stablecoder.ca
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# Set the compiler standard to C89/90
macro(c_90 TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES
  C_STANDARD 90
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF)
endmacro()

# Set the compiler standard to C99
macro(c_99 TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES
  C_STANDARD 99
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF)
endmacro()

# Set the compiler standard to C11
macro(c_11 TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES
  C_STANDARD 11
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF)
endmacro()

# Set the compiler standard to C17
macro(c_17 TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES
  C_STANDARD 17
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF)
endmacro()

# Set the compiler standard to C23
macro(c_23 TARGET_NAME)
  set_target_properties(${TARGET_NAME} PROPERTIES
  C_STANDARD 23
  C_STANDARD_REQUIRED ON
  C_EXTENSIONS OFF)
endmacro()