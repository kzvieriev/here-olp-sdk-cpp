#
# Copyright (C) 2019 HERE Europe B.V.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
# License-Filename: LICENSE
#

import sys
import matplotlib.pyplot as plt

assert(len(sys.argv) == 3)
input_file_name = sys.argv[1]
output_image_name = sys.argv[2]

times = list()
heap_sizes = list()

for line in open(input_file_name, 'r').readlines():
    if line.find("time=") != -1:
        time = int(line.split('=')[1])
        times.append(time)
    elif line.find("mem_heap_B=") != -1:
        heap_size = int(line.split('=')[1])
        heap_sizes.append(heap_size)

assert(len(times) == len(heap_sizes))

plt.plot(times, heap_sizes)
plt.xlabel("Time (ms)")
plt.ylabel("Heap size (bytes)")
plt.savefig(output_image_name)
