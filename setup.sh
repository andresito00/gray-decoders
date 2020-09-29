#!/bin/bash

# Download 3rd party source
git clone --depth 1 --branch v1.2.2 https://github.com/mirror/tclap.git ./cpp/lib/tclap/
git clone --depth 1 --branch v1.0.2 https://github.com/cameron314/concurrentqueue.git ./cpp/lib/concurrentqueue/
