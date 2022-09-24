disp('Compiling S-function from JSBSim...');
% For Windows:
mex -v -R2017b COMPFLAGS='$COMPFLAGS /DJSBSIM_STATIC_LINK' ./matlab/JSBSim_SFunction.cpp ./matlab/JSBSimInterface.cpp -I".\out\install\x64-Release\include\JSBSim" -L".\out\install\x64-Release\lib" -lJSBSim  wsock32.lib ws2_32.lib
% For Linux:
%mex -v -R2017b CXXFLAGS='$CXXFLAGS -std=c++14' ./matlab/JSBSim_SFunction.cpp ./matlab/JSBSimInterface.cpp -I./src -L./src/.libs -lJSBSim
% For MacOS:
%mex -v -R2017b CXXFLAGS='$CXXFLAGS -std=c++14' ./JSBSim_SFunction.cpp ./JSBSimInterface.cpp -I../src -L../build/src -lJSBSim
disp('Finished.')

% -v verbose
% -I is to include a path
% -L is to specify path to library file.

% Requires use of gcc-8 for this to work in the end. Type "!gcc --version!"
% in the command window to check which gcc is used.
% If wrong gcc is used, see the following solution:
% https://codeyarns.com/tech/2015-02-26-how-to-switch-gcc-version-using-update-alternatives.html
