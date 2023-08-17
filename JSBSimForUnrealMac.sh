#!/bin/bash

# make build folder and cd into it
mkdir -p build
cd build

# build the jsbsim library with cmake
cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-stdlib=libc++" ..
make -j4

# cd back to the root directory
cd ..

# set the unreal plugin folder for use in the script
UNREAL_PLUGIN_FOLDER=./UnrealEngine/Plugins/JSBSimFlightDynamicsModel
UNREAL_PLUGIN_INCLUDE_FOLDER=$UNREAL_PLUGIN_FOLDER/Source/ThirdParty/JSBSim/Include
UNREAL_PLUGIN_LIB_FOLDER=$UNREAL_PLUGIN_FOLDER/Source/ThirdParty/JSBSim/Lib/Mac
UNREAL_PLUGIN_RESOURCES_FOLDER=$UNREAL_PLUGIN_FOLDER/Resources/JSBSim

echo "Copying JSBSim header files to Unreal plugin folder: $UNREAL_PLUGIN_INCLUDE_FOLDER"

# make the  unreal plugin thirdparty/jsbsim/include folder
rm -rf $UNREAL_PLUGIN_INCLUDE_FOLDER
mkdir -p $UNREAL_PLUGIN_INCLUDE_FOLDER
# copy the include files (.h,.hxx) from src (and its subdirectories) into unreal
# plugin thirdparty/jsbsim/include folder, keeping the same directory structure
# as in src. Since we're on macos, we use rsync instead of cp to preserve the
# directory structure

# Copy headers
rsync -avm --include='*.h' --include='*.hpp' --include='*.hxx' -f 'hide,! */' src/ UnrealEngine/Plugins/JSBSimFlightDynamicsModel/Source/ThirdParty/JSBSim/Include/

echo "Copying JSBSim library to Unreal plugin folder: 
$UNREAL_PLUGIN_LIB_FOLDER"
# make the unreal plugin thirdparty/jsbsim/lib folder
mkdir -p $UNREAL_PLUGIN_LIB_FOLDER
# copy the jsbsim library from the build folder into the unreal plugin
# thirdparty/jsbsim/lib folder
# cp ./build/src/libJSBSim.a $UNREAL_PLUGIN_LIB_FOLDER/.
cp ./build/src/*.dylib $UNREAL_PLUGIN_LIB_FOLDER/.

echo "Copying JSBSim resources to Unreal plugin folder: 
$UNREAL_PLUGIN_RESOURCES_FOLDER"
# make the unreal plugin resources folder
mkdir -p $UNREAL_PLUGIN_RESOURCES_FOLDER
# copy the aircraft, engine, and systems folders into the unreal plugin resources folder
cp -r ./aircraft $UNREAL_PLUGIN_FOLDER/Resources/JSBSim/.
cp -r ./engine $UNREAL_PLUGIN_FOLDER/Resources/JSBSim/.
cp -r ./systems $UNREAL_PLUGIN_FOLDER/Resources/JSBSim/.
