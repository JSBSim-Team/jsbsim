# UAM 횡풍 CFD 해석 사용 가이드

## 개요
이 OpenFOAM 케이스는 UAM 기체의 횡풍 조건에서의 공기역학적 특성을 해석합니다.

## 시스템 요구사항
- OpenFOAM v2312 이상
- Python 3.8 이상 (numpy, pandas, matplotlib)
- 최소 8GB RAM, 4코어 CPU 권장
- 디스크 공간: 케이스당 약 2-5GB

## 케이스 구조
```
uam_crosswind_cfd/
├── 0/                      # 초기 경계조건
├── constant/               # 물성치 및 격자
├── system/                 # 솔버 설정
├── scripts/               # 실행 스크립트들
├── postProcessing/        # 결과 파일들
└── geometry/             # 지오메트리 파일들
```

## 실행 순서

### 1. 격자 생성
```bash
./scripts/generate_mesh.sh
```
- UAM 지오메트리 STL 생성
- 배경 격자 생성 (blockMesh)
- 물체 주위 세밀화 (snappyHexMesh)
- 예상 시간: 30분-2시간

### 2. 단일 해석 실행
```bash
./scripts/run_cfd.sh [사이드슬립각]
```
예시:
```bash
./scripts/run_cfd.sh 0     # 정면풍
./scripts/run_cfd.sh 15    # 15° 사풍
./scripts/run_cfd.sh -10   # -10° 사풍
```
- 예상 시간: 2-8시간 (격자 크기에 따라)

### 3. 매개변수 연구
```bash
./scripts/parametric_study.sh
```
- 사이드슬립 -25° ~ +25° 자동 해석
- 예상 시간: 1-3일

## 결과 확인

### 실시간 모니터링
```bash
tail -f log.simpleFoam          # 솔버 로그
gnuplot postProcessing/residuals/0/residuals.dat  # 잔차 그래프
```

### 후처리 결과
- `postProcessing/forces/0/coefficient.dat`: 힘 계수 시간이력
- `postProcessing/plots/`: 수렴 그래프들
- `postProcessing/summary_*.csv`: 수렴된 계수값들

## 해석 조건

### 기체 제원
- 전장: 4.57m
- 전폭: 1.8m  
- 로터 직경: 2.4m
- 로터 4개 (쿼드콥터 형태)

### 유동 조건
- 기준 속도: 20.0 m/s
- 공기 밀도: 1.225 kg/m³
- 동점성계수: 1.81e-05 kg/m⋅s
- 난류 모델: k-ω SST

### 격자 정보
- 기본 셀 크기: 0.5m
- 경계층 레이어: 10층
- y+ 목표값: 1.0

## 결과 해석

### 주요 계수들
- **Cd**: 항력계수 (X 방향 힘)
- **Cs**: 측력계수 (Y 방향 힘) ← 횡풍 분석의 핵심
- **Cl**: 양력계수 (Z 방향 힘)
- **Cn**: 요모멘트계수 ← 방향안정성

### 검증 방법
1. 잔차가 1e-4 이하로 수렴 확인
2. 힘 계수가 안정된 값으로 수렴 확인
3. 격자 독립성 테스트 (선택사항)

## 문제 해결

### 수렴 문제
- relaxationFactors 값 낮추기 (0.3 → 0.1)
- 시간 간격 줄이기 (transient 해석 고려)
- 초기 조건 개선

### 메모리 부족
- 격자 조밀도 줄이기 (blockMeshDict 수정)
- 병렬 해석 사용 (decomposePar)

### 격자 품질 문제
- snappyHexMeshDict 설정 조정
- STL 지오메트리 품질 개선

## 추가 기능

### 병렬 해석
```bash
# 4코어로 분할
decomposePar
mpirun -np 4 simpleFoam -parallel
reconstructPar
```

### 다른 난류 모델
system/turbulenceProperties에서 변경:
- kEpsilon: 빠르지만 정확도 낮음
- kOmegaSST: 균형잡힌 선택 (기본값)
- LES: 정확하지만 매우 느림

## 검증 데이터 비교
현재 모델의 계수들과 CFD 결과를 비교:

| 계수 | 현재 모델 | CFD 목표 | 허용 오차 |
|------|-----------|----------|-----------|
| Cy_β | -0.25 | TBD | ±20% |
| Cn_β | 0.12 | TBD | ±20% |

## 연락처
- 기술 지원: cfd-support@uam-research.org
- 버그 제보: github.com/uam-research/cfd-validation
