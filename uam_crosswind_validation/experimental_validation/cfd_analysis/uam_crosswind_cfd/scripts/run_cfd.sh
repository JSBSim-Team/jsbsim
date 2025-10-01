#!/bin/bash

# UAM CFD 해석 실행 스크립트

SIDESLIP_ANGLE=${1:-0}  # 명령행 인수 또는 기본값 0

echo "🌊 UAM 횡풍 CFD 해석 시작 (사이드슬립: ${SIDESLIP_ANGLE}°)"

# 경계조건 생성 (사이드슬립각 적용)
echo "Step 1: 경계조건 설정"
python3 scripts/set_boundary_conditions.py ${SIDESLIP_ANGLE}

# simpleFoam 솔버 실행
echo "Step 2: CFD 해석 실행"
simpleFoam > log.simpleFoam 2>&1 &

# 진행상황 모니터링
SOLVER_PID=$!
echo "솔버 PID: ${SOLVER_PID}"

# 잔차 모니터링 (백그라운드)
gnuplot -persist <<EOF &
set logscale y
set xlabel 'Iteration'
set ylabel 'Residual'
set title 'Convergence History'
plot 'postProcessing/residuals/0/residuals.dat' using 1:2 with lines title 'U'
pause 10
reread
EOF

# 솔버 완료 대기
wait ${SOLVER_PID}

echo "✅ CFD 해석 완료!"
echo "결과 파일들은 postProcessing/ 디렉토리에 저장되었습니다."

# 후처리 자동 실행
echo "Step 3: 후처리 실행"
python3 scripts/postprocess_results.py ${SIDESLIP_ANGLE}
