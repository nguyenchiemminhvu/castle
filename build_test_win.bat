rd /s /q build
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCASTLE_BUILD_TESTING=ON -DCASTLE_FETCH_GTEST=ON
cmake --build build --parallel
