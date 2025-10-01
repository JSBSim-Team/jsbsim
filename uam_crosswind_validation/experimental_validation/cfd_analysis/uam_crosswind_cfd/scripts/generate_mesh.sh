#!/bin/bash

# UAM CFD 격자 생성 자동화 스크립트

echo "🔷 UAM CFD 격자 생성 시작..."

# 1. STL 지오메트리 생성
echo "Step 1: STL 지오메트리 생성"
cd scripts
python3 generate_geometry.py
cd ..

# 2. 배경 격자 생성
echo "Step 2: 배경 격자 생성"
blockMesh

# 3. STL feature edge 추출
echo "Step 3: Feature edge 추출"
surfaceFeatureExtract

# 4. snappyHexMesh 실행
echo "Step 4: 물체 주위 격자 생성"
snappyHexMesh -overwrite

# 5. 격자 품질 검사
echo "Step 5: 격자 품질 검사"
checkMesh -allGeometry -allTopology

echo "✅ 격자 생성 완료!"
echo "격자 파일들은 constant/polyMesh/ 에 저장되었습니다."
