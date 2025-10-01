#!/bin/bash

# UAM 횡풍 매개변수 연구 자동화 스크립트

echo "📊 UAM 횡풍 매개변수 연구 시작"

# 사이드슬립각 배열
SIDESLIP_ANGLES=(-25 -20 -15 -10 -5 0 5 10 15 20 25)

# 결과 저장 디렉토리
RESULTS_DIR="parametric_results"
mkdir -p ${RESULTS_DIR}

# 각 사이드슬립각에 대해 해석 수행
for angle in "${SIDESLIP_ANGLES[@]}"; do
    echo "🔄 사이드슬립 ${angle}° 해석 시작..."
    
    # 케이스 디렉토리 생성
    CASE_DIR="${RESULTS_DIR}/sideslip_${angle}deg"
    cp -r . ${CASE_DIR}
    cd ${CASE_DIR}
    
    # 해석 실행
    ./run_cfd.sh ${angle}
    
    # 주요 결과 복사
    cp postProcessing/forces/0/coefficient.dat ../coeffs_${angle}deg.dat
    cp log.simpleFoam ../log_${angle}deg.txt
    
    cd ../..
    
    echo "✅ 사이드슬립 ${angle}° 해석 완료"
done

echo "🎯 매개변수 연구 완료!"
echo "모든 결과는 ${RESULTS_DIR}/ 에 저장되었습니다."

# 통합 후처리
python3 scripts/analyze_parametric_results.py
