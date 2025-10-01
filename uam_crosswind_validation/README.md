# UAM 기체 횡풍 착륙 분석 연구 프로젝트

## Urban Air Mobility Crosswind Landing Analysis

[![JSBSim](https://img.shields.io/badge/JSBSim-Flight%20Dynamics-blue)](https://jsbsim.sourceforge.net/)
[![Python](https://img.shields.io/badge/Python-3.8+-green)](https://www.python.org/)
[![MATLAB](https://img.shields.io/badge/MATLAB-R2020a+-orange)](https://www.mathworks.com/)

---

## 📋 프로젝트 개요

본 프로젝트는 도심항공교통(UAM) 기체의 횡풍 조건에서 착륙 시 발생하는 좌우 편차를 정량적으로 분석하고, 안전한 운항 기준을 수립하기 위한 연구입니다.

### 🎯 연구 목적
- UAM 기체의 횡풍 조건 착륙 성능 정량적 평가
- 좌우 편차(lateral deviation) 영향 요소 분석
- 임계 기상 조건 및 안전 운항 기준 도출
- 기체 설계 개선 방향 제시

### 🔬 연구 방법론
- **6자유도 비선형 비행동역학 시뮬레이션**
- **JSBSim 기반 고정밀도 수치해석**
- **Monte Carlo 시뮬레이션** (42개 시나리오)
- **통계적 안전성 평가 및 리스크 분석**

---

## 📁 프로젝트 구조

```
uam_crosswind_analysis/
├── aircraft/                          # 기체 모델 파일들
│   ├── uam_quadcopter.xml             # UAM 기체 주 모델
│   ├── uam_crosswind_aerodynamics.xml # 횡풍 공기역학 모델
│   ├── rotorcraft_electric_motor.xml  # 전기 모터 모델
│   └── rotor_*.xml                    # 로터 모델들
│
├── scripts/                           # JSBSim 시뮬레이션 스크립트
│   ├── crosswind_landing_script.xml   # 횡풍 착륙 시뮬레이션
│   └── reset01.xml                    # 초기 조건 설정
│
├── python_tools/                      # Python 분석 도구
│   └── crosswind_landing_simulation.py # 메인 시뮬레이션 스크립트
│
├── matlab_tools/                      # MATLAB 분석 도구
│   └── crosswind_analysis.m           # MATLAB 분석 스크립트
│
├── results/                           # 시뮬레이션 결과
│   ├── simulation_summary.csv         # 결과 요약 데이터
│   ├── detailed_analysis.json         # 상세 분석 결과
│   ├── mathematical_model.json        # 수학적 모델 정보
│   ├── lateral_deviation_analysis.png # 측방편차 분석 그래프
│   ├── crosswind_heatmap.png         # 횡풍 영향 히트맵
│   └── time_series_analysis.png       # 시계열 분석 그래프
│
└── documentation/                     # 문서화
    ├── UAM_Crosswind_Landing_Analysis_Report.md # 메인 R&D 보고서
    └── Executive_Summary.md           # 경영진 요약보고서
```

---

## 🚀 빠른 시작

### 1. 환경 설정

**Python 환경:**
```bash
pip install numpy pandas matplotlib scipy
```

**MATLAB 환경:**
- MATLAB R2020a 이상
- Control System Toolbox
- Aerospace Toolbox (권장)

### 2. 시뮬레이션 실행

**Python 시뮬레이션:**
```bash
cd uam_crosswind_analysis
python python_tools/crosswind_landing_simulation.py
```

**MATLAB 분석:**
```matlab
cd('uam_crosswind_analysis/matlab_tools')
crosswind_analysis
```

**JSBSim 직접 실행:**
```bash
JSBSim --script=scripts/crosswind_landing_script.xml
```

---

## 📊 주요 결과

### 시뮬레이션 조건
- **풍속**: 2, 5, 8, 10, 12, 15 m/s (6단계)
- **풍향**: 0°, 30°, 60°, 90°, 120°, 150°, 180° (7단계)
- **총 케이스**: 42개 조합
- **기체 모델**: 멀티로터 타입 UAM (질량 1,134kg)

### 핵심 발견사항

| 항목 | 결과 | 비고 |
|------|------|------|
| **최대 측방편차** | 233.52 m | 풍속 15m/s, 풍향 120° |
| **평균 측방편차** | 108.64 m | 전체 시나리오 평균 |
| **현재 안전률** | 14.3% | 착륙장 폭 30m 기준 |
| **임계 풍속** | 10 m/s | 이상에서 200m+ 편차 |
| **가장 위험한 풍향** | 120°-150° | 사풍(斜風) 조건 |

### 풍향별 위험도 평가

```
  정면풍 (0°)  : 안전 (편차 0m)
  측풍 (90°)   : 높음 (평균 153.4m)
  사풍 (120°)  : 매우높음 (평균 171.7m) ⚠️
  후방풍(180°) : 낮음 (평균 58.8m)
```

---

## 🔬 수학적 모델

### 횡방향 동역학 방정식

**병진 운동:**
```
dv/dt = (Y/m) - ru + pw
```

**회전 운동:**
```
dp/dt = (L + (Iyy - Izz)qr)/Ixx
dr/dt = (N + (Ixx - Iyy)pq)/Izz
```

**공기역학적 힘:**
```
Y = q̄S[Cy_β·β + Cy_δr·δr + Cy_p·p̂ + Cy_r·r̂]
L = q̄Sb[Cl_β·β + Cl_p·p̂ + Cl_r·r̂ + Cl_δa·δa + Cl_δr·δr]
N = q̄Sb[Cn_β·β + Cn_r·r̂ + Cn_p·p̂ + Cn_δr·δr + Cn_δa·δa]
```

### 주요 공기역학 계수

| 계수 | 값 | 물리적 의미 |
|------|----|-----------| 
| Cy_β | -0.25 | 사이드슬립에 의한 측력 |
| Cl_β | -0.08 | 상반각 효과 |
| Cn_β | 0.12 | 방향 안정성 |
| Cl_p | -0.45 | 롤 댐핑 |
| Cn_r | -0.25 | 요 댐핑 |

---

## 📈 시각화 결과

### 1. 측방편차 분석
- 풍속별, 풍향별 측방편차 변화 추이
- 임계 조건 식별 및 안전 구간 표시

### 2. 횡풍 영향 히트맵
- 풍속-풍향 조합별 위험도 매트릭스
- 색상 코딩을 통한 직관적 리스크 표시

### 3. 시계열 분석
- 대표 케이스의 비행 궤적 및 상태 변화
- 제어 입력 및 기체 응답 특성 분석

---

## ⚠️ 안전성 평가

### 현재 상황
- **위험 케이스**: 85.7% (36/42 케이스)
- **안전 케이스**: 14.3% (6/42 케이스)
- **권장 착륙장 폭**: 최소 50m (현재 30m 대비 67% 증가)

### 운항 제한 권고
1. **횡풍 한계**: 8 m/s 이하로 제한
2. **금지 풍향**: 90°-150° 강풍 시 운항 중단
3. **기상 모니터링**: 실시간 풍속/풍향 감시 필수

---

## 🔧 개선방안

### 1. 단기 개선 (6개월)
- **적응제어 시스템** 도입
- **풍속 추정 알고리즘** 개발
- **파일럿 훈련 프로그램** 강화

### 2. 중기 개선 (1년)
- **수직꼬리날개 확대** (면적 40% 증대)
- **로터 제어 최적화**
- **착륙장 안전 기준** 재정립

### 3. 장기 개선 (2년)
- **차세대 기체 설계**
- **AI 기반 자율착륙 시스템**
- **목표 안전률**: 95% 이상

---

## 📚 참고문헌

1. Nelson, R.C., "Flight Stability and Automatic Control", McGraw-Hill, 1998
2. Stevens, B.L. & Lewis, F.L., "Aircraft Control and Simulation", Wiley, 2003
3. JSBSim Development Team, "JSBSim Reference Manual", 2023
4. EASA, "Certification Specifications for VTOL Aircraft", CS-27, 2021
5. FAA, "Urban Air Mobility Concept of Operations", 2020

---

## 👥 연구진

- **Project Lead**: UAM Research Team
- **Flight Dynamics**: Dr. Flight Dynamics Specialist
- **Control Systems**: Control Systems Engineer
- **Safety Analysis**: Aviation Safety Expert

---

## 📞 연락처

**프로젝트 문의**: uam-research@aviation.org  
**기술 지원**: tech-support@aviation.org  
**협력 제안**: partnership@aviation.org

---

## 📜 라이선스

본 연구는 교육 및 연구 목적으로 공개되며, 상업적 사용 시 별도 협의가 필요합니다.

**Copyright © 2024 UAM Research Division. All rights reserved.**

---

## 🚧 개발 현황

- ✅ **기체 모델링**: 완료
- ✅ **시뮬레이션 환경**: 완료  
- ✅ **횡풍 모델**: 완료
- ✅ **분석 도구**: 완료
- ✅ **결과 시각화**: 완료
- 🔄 **실험 검증**: 진행 중
- 📋 **표준화**: 계획됨

---

*마지막 업데이트: 2024년 10월 1일*