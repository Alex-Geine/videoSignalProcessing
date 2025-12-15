 cmake -S . -B build_msvc -G "Visual Studio 17 2022" -A x64 -DOpenCV_DIR=ext/opencv/build
 cmake --build build_msvc --config Debug -j 15
 Copy-Item -Path "D:\uni\videoSignalProcessing\ext\opencv\build\x64\vc16\bin\*.dll" -Destination ".\" -Force