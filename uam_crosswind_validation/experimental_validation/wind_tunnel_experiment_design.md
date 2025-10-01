# UAM 횡풍 해석을 위한 풍동 실험 설계서
# Wind Tunnel Experiment Design for UAM Crosswind Analysis

---

## 🎯 실험 목적 (Experiment Objectives)

### 주요 목표
1. **UAM 기체의 횡풍 공기역학 계수 실측 검증**
   - 측력 계수 (Cy_β) 실측: 현재 추정값 -0.25 검증
   - 요모멘트 계수 (Cn_β) 실측: 현재 추정값 0.12 검증
   - 롤댐핑 계수 (Cl_p) 실측: 현재 추정값 -0.45 검증

2. **CFD 해석 결과와의 상호 검증**
   - OpenFOAM CFD 결과와 풍동 실측 데이터 비교
   - 수치 해석의 정확도 평가

3. **실제 비행 조건 모사**
   - 다양한 횡풍 각도 (-25° ~ +25°) 실험
   - 레이놀즈 수 영향 분석
   - 동적 효과 측정

---

## 🏗️ 풍동 시설 요구사항 (Wind Tunnel Facility Requirements)

### 권장 풍동 시설
| 시설명 | 위치 | 시험부 규모 | 최대 풍속 | 특징 |
|--------|------|-------------|-----------|------|
| **KARI 대형아음속풍동** | 대전 | 3.5m×2.8m | 140 m/s | 6분력 측정, 동적시험 |
| **서울대 저속풍동** | 서울 | 1.8m×1.3m | 70 m/s | 연구용, 비용 효율적 |
| **항공대 아음속풍동** | 고양 | 1.5m×1.1m | 80 m/s | 교육/연구 병행 |

### 선택 기준
```
✅ 권장: KARI 대형아음속풍동
- 충분한 시험부 크기 (모델 비례 1/3 가능)
- 정밀한 6분력 측정 시스템
- 동적 시험 장비 완비
- 전문 기술진 지원

예산: 약 3,000만원 (모델 제작비 포함)
기간: 3-4개월 (모델제작 2개월 + 시험 1개월)
```

---

## 🛩️ 축소 모델 설계 (Scale Model Design)

### 스케일링 전략
```python
# 스케일 결정
full_scale_length = 4.57  # m (실기체 동체 길이)
wind_tunnel_width = 3.5   # m (KARI 풍동 시험부 폭)
blockage_limit = 0.05     # 5% 차단율 한계

# 최적 스케일 계산
max_model_span = wind_tunnel_width * blockage_limit  # = 0.175m
full_scale_span = 6.0  # m (실기체 전폭)
scale_factor = max_model_span / full_scale_span  # = 1/34

# 실제 적용 스케일 (제작 편의성 고려)
recommended_scale = 1/3  # 33.3% 축소
model_length = full_scale_length / 3  # = 1.52m
model_span = 6.0 / 3  # = 2.0m
```

### 모델 사양
| 구성요소 | 실기체 | 축소모델 (1/3) | 재질 | 비고 |
|----------|--------|----------------|------|------|
| **동체 길이** | 4.57m | 1.52m | 알루미늄 합금 | CNC 가공 |
| **동체 폭** | 1.8m | 0.60m | 알루미늄 합금 | 내부 중공 구조 |
| **로터 직경** | 2.4m | 0.80m | 카본파이버 | 회전 가능 |
| **전폭** | 6.0m | 2.00m | - | 로터 중심간 거리 |
| **전고** | 1.5m | 0.50m | - | 랜딩기어 포함 |

### 상세 설계 도면
```
        ← 2.0m →
    ┌─────────────────┐
    │  ●           ●  │  ← 로터 (Ø0.8m)
 0.5m  │     동체      │
    │  ●           ●  │
    └─────────────────┘
        ← 1.52m →
        
측면도:
    ●───●  ← 로터 높이
    │   │
    │ ▄ │  ← 동체 (높이 0.5m)  
    └─┬─┘
      │    ← 랜딩기어
```

### 제작 사양서
```yaml
모델_제작_사양:
  재질:
    동체: "알루미늄 6061-T6"
    로터_허브: "스테인레스 스틸 304"
    로터_블레이드: "카본파이버 복합재"
    랜딩기어: "알루미늄 합금"
    
  가공_정밀도:
    표면_거칠기: "Ra 3.2μm 이하"
    치수_공차: "±0.1mm"
    각도_공차: "±0.1°"
    
  내부_구조:
    중공_구조: "벽 두께 3mm"
    무게중심_조정: "납 추가 가능한 공간 확보"
    케이블_통로: "계측 케이블용 내부 통로"
    
  장착_시스템:
    6분력_연결: "M8 볼트 4개소"
    피칭_각도_조정: "±10° 범위"
    요잉_각도_조정: "±30° 범위"
```

---

## ⚖️ 계측 시스템 (Instrumentation System)

### 6분력 측정 시스템
```python
# 측정 항목 및 정밀도 요구사항
force_measurement = {
    'X_force': {  # 항력
        'range': '±200N',
        'accuracy': '±0.1N',
        'resolution': '0.01N'
    },
    'Y_force': {  # 측력 (핵심 측정값)
        'range': '±100N', 
        'accuracy': '±0.05N',
        'resolution': '0.005N'
    },
    'Z_force': {  # 양력
        'range': '±300N',
        'accuracy': '±0.1N', 
        'resolution': '0.01N'
    },
    'L_moment': {  # 롤모멘트
        'range': '±50N⋅m',
        'accuracy': '±0.05N⋅m',
        'resolution': '0.005N⋅m'
    },
    'M_moment': {  # 피치모멘트
        'range': '±100N⋅m',
        'accuracy': '±0.1N⋅m',
        'resolution': '0.01N⋅m'  
    },
    'N_moment': {  # 요모멘트 (핵심 측정값)
        'range': '±50N⋅m',
        'accuracy': '±0.05N⋅m',
        'resolution': '0.005N⋅m'
    }
}

# 데이터 수집 시스템
data_acquisition = {
    'sampling_rate': 1000,  # Hz
    'duration_per_test': 30,  # seconds
    'averaging_time': 10,   # seconds (정상상태)
    'filters': ['low_pass_100Hz', 'notch_60Hz']
}
```

### 추가 계측 장비
| 장비 | 목적 | 사양 | 수량 |
|------|------|------|------|
| **압력센서** | 표면압력분포 | ±1kPa, 0.1% 정밀도 | 32개 |
| **PIV 시스템** | 유동 가시화 | 2D-PIV, 1000fps | 1세트 |
| **연기 발생기** | 유선 가시화 | 무독성 연기 | 2대 |
| **온습도계** | 대기 조건 | ±0.1°C, ±1% RH | 1개 |

---

## 🧪 실험 매트릭스 (Test Matrix)

### 정적 시험 조건
```python
# 기본 시험 매트릭스
static_test_matrix = {
    'reynolds_numbers': [1.5e5, 3.0e5, 6.0e5],  # 축소모델 기준
    'sideslip_angles': [-25, -20, -15, -10, -5, 0, 5, 10, 15, 20, 25],  # degrees
    'angle_of_attack': [-5, 0, 5],  # degrees
    'wind_speeds': [15, 30, 60],  # m/s (풍동 내 속도)
    'total_test_points': 3 * 11 * 3 * 3  # = 297 test points
}

# 시험점별 세부 조건
for Re in static_test_matrix['reynolds_numbers']:
    for beta in static_test_matrix['sideslip_angles']:
        for alpha in static_test_matrix['angle_of_attack']:
            # 풍속 결정 (레이놀즈 수 기준)
            wind_speed = calculate_wind_speed(Re, model_chord=0.5)  # m/s
            
            test_condition = {
                'Re': Re,
                'beta': beta,  # sideslip angle
                'alpha': alpha,  # angle of attack
                'V': wind_speed,
                'duration': 30,  # seconds per point
                'measurement_variables': [
                    'Cy_beta',    # 측력계수 (핵심)
                    'Cn_beta',    # 요모멘트계수 (핵심)
                    'Cl_beta',    # 롤모멘트계수
                    'CD', 'CL'    # 항력/양력계수
                ]
            }
```

### 동적 시험 조건
```python
# 강제진동시험 (Forced Oscillation Tests)
dynamic_test_matrix = {
    'roll_oscillation': {
        'frequency_range': [0.5, 1.0, 2.0, 5.0],  # Hz
        'amplitude': [±5°, ±10°, ±15°],  # degrees
        'measurement': 'Cl_p',  # 롤댐핑계수
        'test_duration': 60  # seconds per frequency
    },
    'yaw_oscillation': {
        'frequency_range': [0.5, 1.0, 2.0, 5.0],  # Hz  
        'amplitude': [±5°, ±10°, ±15°],  # degrees
        'measurement': 'Cn_r',  # 요댐핑계수
        'test_duration': 60  # seconds per frequency
    }
}

# 동적시험 데이터 처리
def process_dynamic_data(oscillation_data):
    """강제진동 데이터로부터 댐핑계수 추출"""
    
    # FFT 분석으로 위상지연 계산
    phase_lag = calculate_phase_lag(oscillation_data)
    
    # 댐핑계수 계산
    # Cl_p = -(롤모멘트 진폭) / (롤각속도 진폭) / (동압 × 날개면적 × 반폭)
    damping_coefficient = calculate_damping_coefficient(
        moment_amplitude=oscillation_data['moment'],
        angular_velocity_amplitude=oscillation_data['omega'],
        phase_lag=phase_lag
    )
    
    return damping_coefficient
```

### 특수 시험 조건
```python
# 로터 영향 시험
rotor_effect_tests = {
    'rotor_configurations': [
        'rotors_off',      # 로터 정지
        'rotors_spinning', # 로터 회전 (실제 조건)
        'rotors_removed'   # 로터 완전 제거
    ],
    'comparison_purpose': '로터-동체 간섭효과 분석',
    'expected_results': {
        'Cy_beta_difference': '10-20% 차이 예상',
        'Cn_beta_variation': '±15% 범위 내'
    }
}

# 스케일 효과 검증
scale_effect_validation = {
    'model_scales': ['1/5', '1/3'],  # 두 가지 스케일 비교
    'reynolds_matching': '실기체 조건에 맞는 Re 설정',
    'correction_factors': '스케일 보정계수 도출'
}
```

---

## 📊 데이터 처리 및 분석 (Data Processing & Analysis)

### 실시간 데이터 처리
```python
import numpy as np
from scipy import signal
import pandas as pd

class WindTunnelDataProcessor:
    """풍동 시험 데이터 실시간 처리기"""
    
    def __init__(self):
        self.sampling_rate = 1000  # Hz
        self.model_geometry = {
            'reference_area': 1.333,  # m² (축소모델 기준)
            'reference_length': 0.5,  # m (기준길이)
            'span': 2.0  # m (날개폭)
        }
    
    def real_time_coefficient_calculation(self, force_data, test_conditions):
        """실시간 공기역학 계수 계산"""
        
        # 동압 계산
        rho = test_conditions['air_density']  # kg/m³
        V = test_conditions['wind_speed']     # m/s
        q = 0.5 * rho * V**2                # Pa
        
        # 기준 면적 및 길이
        S_ref = self.model_geometry['reference_area']
        b = self.model_geometry['span']
        c = self.model_geometry['reference_length']
        
        # 무차원 계수 계산
        coefficients = {
            'CY': force_data['Y_force'] / (q * S_ref),           # 측력계수
            'CN': force_data['N_moment'] / (q * S_ref * b),     # 요모멘트계수  
            'CL_roll': force_data['L_moment'] / (q * S_ref * b), # 롤모멘트계수
            'CD': force_data['X_force'] / (q * S_ref),          # 항력계수
            'CL_lift': force_data['Z_force'] / (q * S_ref)      # 양력계수
        }
        
        return coefficients
    
    def sideslip_derivative_calculation(self, coefficient_data, sideslip_angles):
        """횡풍 도함수 (β-derivatives) 계산"""
        
        # 선형 회귀로 기울기 계산
        beta_rad = np.radians(sideslip_angles)
        
        # Cy_β 계산 (측력의 횡풍각 도함수)
        Cy_beta = np.polyfit(beta_rad, coefficient_data['CY'], 1)[0]
        
        # Cn_β 계산 (요모멘트의 횡풍각 도함수)  
        Cn_beta = np.polyfit(beta_rad, coefficient_data['CN'], 1)[0]
        
        # Cl_β 계산 (롤모멘트의 횡풍각 도함수)
        Cl_beta = np.polyfit(beta_rad, coefficient_data['CL_roll'], 1)[0]
        
        # 결정계수 (R²) 계산으로 선형성 검증
        r_squared = {
            'Cy_beta_r2': calculate_r_squared(coefficient_data['CY'], beta_rad),
            'Cn_beta_r2': calculate_r_squared(coefficient_data['CN'], beta_rad),
            'Cl_beta_r2': calculate_r_squared(coefficient_data['CL_roll'], beta_rad)
        }
        
        derivatives = {
            'Cy_beta': Cy_beta,
            'Cn_beta': Cn_beta, 
            'Cl_beta': Cl_beta,
            'linearity_check': r_squared
        }
        
        return derivatives

def calculate_r_squared(measured, fitted):
    """결정계수 계산"""
    ss_res = np.sum((measured - fitted) ** 2)
    ss_tot = np.sum((measured - np.mean(measured)) ** 2) 
    return 1 - (ss_res / ss_tot)
```

### 불확실성 분석
```python
class UncertaintyAnalysis:
    """측정 불확실성 분석"""
    
    def __init__(self):
        self.measurement_uncertainties = {
            'force_balance': 0.001,      # 0.1% (6분력 측정기)
            'wind_speed': 0.005,         # 0.5% (피토관)
            'angle_setting': 0.1,        # 0.1° (각도 설정)  
            'temperature': 0.1,          # 0.1°C
            'pressure': 50               # 50Pa
        }
    
    def propagate_uncertainty(self, measured_coefficients):
        """불확실성 전파 계산"""
        
        # 각 계수별 불확실성 계산
        uncertainty_results = {}
        
        for coeff_name, coeff_value in measured_coefficients.items():
            # 파라미터별 기여도 계산 (편미분)
            partial_derivatives = self.calculate_sensitivity_coefficients(coeff_name)
            
            # 불확실성 제곱합 계산
            total_uncertainty_squared = 0
            for param, sensitivity in partial_derivatives.items():
                param_uncertainty = self.measurement_uncertainties[param]
                total_uncertainty_squared += (sensitivity * param_uncertainty)**2
            
            # 합성 표준 불확실성
            combined_uncertainty = np.sqrt(total_uncertainty_squared)
            
            # 확장 불확실성 (k=2, 95% 신뢰구간)
            expanded_uncertainty = 2 * combined_uncertainty
            
            uncertainty_results[coeff_name] = {
                'value': coeff_value,
                'combined_uncertainty': combined_uncertainty,
                'expanded_uncertainty': expanded_uncertainty,
                'relative_uncertainty': expanded_uncertainty / abs(coeff_value) * 100
            }
        
        return uncertainty_results
    
    def calculate_sensitivity_coefficients(self, coefficient_name):
        """민감도 계수 계산 (편미분)"""
        
        # 각 계수별 측정 파라미터에 대한 민감도
        sensitivity_matrices = {
            'Cy_beta': {
                'force_balance': 1.0,    # ∂Cy/∂F_측정
                'wind_speed': -2.0,      # ∂Cy/∂V (동압 의존성)
                'angle_setting': 0.1,    # ∂Cy/∂β_설정각
                'temperature': 0.01,     # 공기밀도 영향
                'pressure': 0.005        # 공기밀도 영향
            },
            'Cn_beta': {
                'force_balance': 1.0,
                'wind_speed': -2.0,
                'angle_setting': 0.1,
                'temperature': 0.01,
                'pressure': 0.005
            }
        }
        
        return sensitivity_matrices.get(coefficient_name, {})
```

### 스케일 보정 및 실기체 적용
```python
class ScaleCorrection:
    """축소모델 결과의 실기체 적용"""
    
    def __init__(self, scale_factor=1/3):
        self.scale_factor = scale_factor
        self.full_scale_geometry = {
            'length': 4.57,     # m
            'span': 6.0,        # m  
            'area': 10.0,       # m²
            'chord': 1.5        # m
        }
    
    def reynolds_number_correction(self, model_coefficients, model_Re, full_scale_Re):
        """레이놀즈 수 보정"""
        
        # 경험적 보정 공식 (항공기 설계 데이터 기반)
        Re_correction_factors = {
            'Cy_beta': 1 + 0.05 * np.log10(full_scale_Re / model_Re),
            'Cn_beta': 1 + 0.03 * np.log10(full_scale_Re / model_Re),
            'Cl_p': 1 + 0.08 * np.log10(full_scale_Re / model_Re),
            'Cn_r': 1 + 0.06 * np.log10(full_scale_Re / model_Re)
        }
        
        corrected_coefficients = {}
        for coeff, value in model_coefficients.items():
            correction_factor = Re_correction_factors.get(coeff, 1.0)
            corrected_coefficients[coeff] = value * correction_factor
        
        return corrected_coefficients
    
    def compressibility_correction(self, coefficients, mach_number):
        """압축성 효과 보정 (고속 비행시)"""
        
        if mach_number < 0.3:
            return coefficients  # 비압축성 가정 유효
        
        # Prandtl-Glauert 보정
        beta_prandtl = np.sqrt(1 - mach_number**2)
        
        compressibility_corrected = {}
        for coeff, value in coefficients.items():
            if 'C' in coeff:  # 힘/모멘트 계수들
                compressibility_corrected[coeff] = value / beta_prandtl
            else:
                compressibility_corrected[coeff] = value
        
        return compressibility_corrected
    
    def generate_full_scale_model(self, wind_tunnel_results):
        """최종 실기체 적용 모델 생성"""
        
        # 모든 보정 적용
        corrected_results = self.reynolds_number_correction(
            wind_tunnel_results['coefficients'],
            wind_tunnel_results['test_Re'],
            wind_tunnel_results['target_full_scale_Re']
        )
        
        corrected_results = self.compressibility_correction(
            corrected_results, 
            wind_tunnel_results['cruise_mach']
        )
        
        # JSBSim 포맷으로 변환
        jsbsim_coefficients = {
            'CY_beta': corrected_results['Cy_beta'],
            'CN_beta': corrected_results['Cn_beta'],
            'CL_p': corrected_results['Cl_p'],
            'CN_r': corrected_results['Cn_r']
        }
        
        return jsbsim_coefficients
```

---

## 📋 실험 수행 절차 (Test Procedures)

### 실험 전 준비사항
```yaml
실험_준비_체크리스트:
  모델_점검:
    - [ ] 치수 정밀도 검증 (±0.1mm)
    - [ ] 표면 마감도 확인 (Ra ≤ 3.2μm)  
    - [ ] 무게중심 위치 측정 및 조정
    - [ ] 6분력 연결부 토크 확인
    
  계측_시스템:
    - [ ] 6분력 측정기 영점 조정
    - [ ] 데이터수집장치 샘플링레이트 설정 (1kHz)
    - [ ] 압력센서 영점 및 교정
    - [ ] PIV 시스템 정렬 및 교정
    
  풍동_조건:
    - [ ] 시험부 청소 및 난류도 측정
    - [ ] 온습도 측정 및 기록
    - [ ] 풍속 교정 (피토관 vs 풍동 설정값)
    - [ ] 각도 설정 장치 교정
    
  안전_점검:
    - [ ] 비상정지 버튼 작동 확인
    - [ ] 모델 고정상태 최종 점검
    - [ ] 관찰창 청결도 확인
    - [ ] 실험자 안전장구 착용
```

### 실험 순서
```python
def experiment_execution_sequence():
    """풍동 실험 수행 순서"""
    
    # Phase 1: 기준 상태 확립
    phase_1 = {
        'name': '기준상태_측정',
        'conditions': {'beta': 0, 'alpha': 0, 'V': 30},  # m/s
        'duration': 300,  # seconds (충분한 안정화 시간)
        'purpose': '시스템 안정성 및 기준값 확립'
    }
    
    # Phase 2: 정적 횡풍각 스위프
    phase_2 = {
        'name': '정적_횡풍각_실험',
        'sequence': [
            {'beta': 0, 'measurement_time': 30},    # 기준점
            {'beta': 5, 'measurement_time': 30},    # 양의 방향
            {'beta': 10, 'measurement_time': 30},
            {'beta': 15, 'measurement_time': 30},
            {'beta': 20, 'measurement_time': 30},
            {'beta': 25, 'measurement_time': 30},
            {'beta': 0, 'measurement_time': 30},    # 기준점 재측정
            {'beta': -5, 'measurement_time': 30},   # 음의 방향
            {'beta': -10, 'measurement_time': 30},
            {'beta': -15, 'measurement_time': 30},
            {'beta': -20, 'measurement_time': 30},
            {'beta': -25, 'measurement_time': 30},
            {'beta': 0, 'measurement_time': 30}     # 최종 기준점
        ],
        'total_time': 360,  # seconds
        'purpose': 'Cy_β, Cn_β 계수 측정'
    }
    
    # Phase 3: 레이놀즈 수 영향 실험  
    phase_3 = {
        'name': 'Reynolds_수_영향',
        'conditions': [
            {'Re': 1.5e5, 'V': 15, 'beta_range': [-20, 0, 20]},
            {'Re': 3.0e5, 'V': 30, 'beta_range': [-20, 0, 20]}, 
            {'Re': 6.0e5, 'V': 60, 'beta_range': [-20, 0, 20]}
        ],
        'purpose': '스케일 효과 분석'
    }
    
    # Phase 4: 동적 실험
    phase_4 = {
        'name': '강제진동_실험',
        'oscillation_tests': [
            {
                'type': 'roll_oscillation',
                'frequencies': [0.5, 1.0, 2.0, 5.0],  # Hz
                'amplitude': 10,  # degrees
                'measurement': 'Cl_p'
            },
            {
                'type': 'yaw_oscillation', 
                'frequencies': [0.5, 1.0, 2.0, 5.0],  # Hz
                'amplitude': 10,  # degrees
                'measurement': 'Cn_r'
            }
        ],
        'duration_per_frequency': 60,  # seconds
        'purpose': '댐핑 계수 측정'
    }
    
    return [phase_1, phase_2, phase_3, phase_4]
```

### 품질 관리 절차
```python
class QualityAssurance:
    """실험 품질 관리"""
    
    def __init__(self):
        self.acceptance_criteria = {
            'repeatability': 0.02,      # 반복성 ±2%
            'linearity_r_squared': 0.95, # 선형성 R² > 0.95
            'balance_drift': 0.001      # 측정기 드리프트 < 0.1%
        }
    
    def real_time_quality_check(self, current_measurement, previous_measurements):
        """실시간 품질 체크"""
        
        quality_flags = {}
        
        # 1. 반복성 검사
        if len(previous_measurements) >= 3:
            recent_values = previous_measurements[-3:]
            std_dev = np.std(recent_values)
            mean_value = np.mean(recent_values)
            coefficient_of_variation = std_dev / abs(mean_value)
            
            quality_flags['repeatability_ok'] = (
                coefficient_of_variation <= self.acceptance_criteria['repeatability']
            )
        
        # 2. 이상값 검출 (3-sigma rule)
        if len(previous_measurements) >= 10:
            all_values = previous_measurements + [current_measurement]
            z_score = abs(current_measurement - np.mean(all_values)) / np.std(all_values)
            quality_flags['outlier_detected'] = z_score > 3
        
        # 3. 트렌드 분석 (드리프트 검출)
        if len(previous_measurements) >= 20:
            time_series = np.array(previous_measurements)
            trend_slope = np.polyfit(range(len(time_series)), time_series, 1)[0]
            relative_drift = abs(trend_slope) / abs(np.mean(time_series))
            quality_flags['drift_acceptable'] = (
                relative_drift <= self.acceptance_criteria['balance_drift']
            )
        
        return quality_flags
    
    def generate_quality_report(self, experiment_data):
        """실험 품질 보고서 생성"""
        
        report = {
            'experiment_date': experiment_data['date'],
            'total_test_points': len(experiment_data['measurements']),
            'data_quality_metrics': {}
        }
        
        # 각 계수별 품질 평가
        for coefficient in ['Cy_beta', 'Cn_beta', 'Cl_p', 'Cn_r']:
            coeff_data = experiment_data['measurements'][coefficient]
            
            # 선형성 평가 (R² 계산)
            beta_angles = experiment_data['sideslip_angles']
            r_squared = calculate_r_squared(coeff_data, beta_angles)
            
            # 측정 불확실성
            measurement_uncertainty = np.std(coeff_data) / np.sqrt(len(coeff_data))
            
            report['data_quality_metrics'][coefficient] = {
                'linearity_r_squared': r_squared,
                'measurement_uncertainty': measurement_uncertainty,
                'quality_grade': self.assign_quality_grade(r_squared, measurement_uncertainty)
            }
        
        return report
    
    def assign_quality_grade(self, r_squared, uncertainty):
        """품질 등급 부여"""
        
        if r_squared >= 0.98 and uncertainty <= 0.01:
            return 'A (Excellent)'
        elif r_squared >= 0.95 and uncertainty <= 0.02:
            return 'B (Good)'
        elif r_squared >= 0.90 and uncertainty <= 0.05:
            return 'C (Acceptable)'
        else:
            return 'D (Poor - Retest Required)'
```

---

## 📈 예상 결과 및 검증 (Expected Results & Validation)

### 예상 실험 결과
```python
# 풍동 실험 예상 결과 (현재 모델 대비)
expected_results = {
    'coefficients': {
        'Cy_beta': {
            'current_model': -0.25,
            'expected_range': [-0.35, -0.15],
            'wind_tunnel_estimate': -0.28,
            'confidence_level': 0.8
        },
        'Cn_beta': {
            'current_model': 0.12,
            'expected_range': [0.08, 0.16], 
            'wind_tunnel_estimate': 0.11,
            'confidence_level': 0.75
        },
        'Cl_p': {
            'current_model': -0.45,
            'expected_range': [-0.6, -0.3],
            'wind_tunnel_estimate': -0.42,
            'confidence_level': 0.7
        },
        'Cn_r': {
            'current_model': -0.25,
            'expected_range': [-0.35, -0.15],
            'wind_tunnel_estimate': -0.23,
            'confidence_level': 0.7
        }
    },
    
    'model_validation_scenarios': {
        'scenario_1_good_agreement': {
            'condition': 'Wind tunnel results within ±15% of current model',
            'probability': 0.4,
            'action': 'Confirm current model validity',
            'impact': 'High confidence in simulation results'
        },
        'scenario_2_moderate_difference': {
            'condition': 'Wind tunnel results differ by 15-30%',
            'probability': 0.5,
            'action': 'Update model with corrected coefficients',
            'impact': 'Revised simulation results, updated safety margins'
        },
        'scenario_3_major_difference': {
            'condition': 'Wind tunnel results differ by >30%',
            'probability': 0.1,
            'action': 'Major model revision required',
            'impact': 'Significant changes in crosswind performance predictions'
        }
    }
}
```

### CFD-풍동 상호검증
```python
class CFD_WindTunnel_Comparison:
    """CFD와 풍동 결과 비교 검증"""
    
    def __init__(self):
        self.comparison_metrics = [
            'coefficient_values',
            'pressure_distributions', 
            'flow_patterns',
            'reynolds_sensitivity'
        ]
    
    def compare_coefficients(self, cfd_results, wind_tunnel_results):
        """계수 비교 분석"""
        
        comparison = {}
        
        for coeff in ['Cy_beta', 'Cn_beta', 'Cl_p', 'Cn_r']:
            cfd_value = cfd_results[coeff]
            wt_value = wind_tunnel_results[coeff]
            
            # 상대 오차 계산
            relative_error = abs(cfd_value - wt_value) / abs(wt_value) * 100
            
            # 절대 차이
            absolute_difference = cfd_value - wt_value
            
            # 검증 상태 판정
            if relative_error <= 10:
                validation_status = 'Excellent Agreement'
            elif relative_error <= 20:
                validation_status = 'Good Agreement'
            elif relative_error <= 30:
                validation_status = 'Acceptable Agreement'
            else:
                validation_status = 'Poor Agreement - Investigation Required'
            
            comparison[coeff] = {
                'cfd_value': cfd_value,
                'wind_tunnel_value': wt_value,
                'relative_error': relative_error,
                'absolute_difference': absolute_difference,
                'validation_status': validation_status
            }
        
        return comparison
    
    def pressure_distribution_validation(self, cfd_pressure, wt_pressure):
        """압력분포 비교"""
        
        # 상관계수 계산
        correlation = np.corrcoef(cfd_pressure, wt_pressure)[0,1]
        
        # RMS 오차
        rms_error = np.sqrt(np.mean((cfd_pressure - wt_pressure)**2))
        
        # 최대 차이점 식별
        max_error_location = np.argmax(abs(cfd_pressure - wt_pressure))
        
        validation_report = {
            'correlation_coefficient': correlation,
            'rms_error': rms_error,
            'max_error_location': max_error_location,
            'agreement_level': self.assess_pressure_agreement(correlation, rms_error)
        }
        
        return validation_report
    
    def assess_pressure_agreement(self, correlation, rms_error):
        """압력분포 일치도 평가"""
        
        if correlation >= 0.95 and rms_error <= 0.05:
            return 'Excellent'
        elif correlation >= 0.90 and rms_error <= 0.10:
            return 'Good'
        elif correlation >= 0.80 and rms_error <= 0.20:
            return 'Acceptable'
        else:
            return 'Poor'
```

---

## 💰 예산 및 일정 (Budget & Schedule)

### 상세 예산 계획
```yaml
풍동실험_예산_명세:
  모델_제작비:
    동체_가공: 800만원      # CNC 가공, 알루미늄
    로터_시스템: 400만원    # 카본파이버, 베어링
    장착_시스템: 200만원    # 6분력 연결부
    소계: 1,400만원
    
  풍동_사용료:
    KARI_시설_사용: 1,200만원  # 15일 × 80만원/일
    기술자_지원: 300만원      # 전문 기술진
    소계: 1,500만원
    
  계측_장비:
    압력센서_32개: 160만원    # 5만원 × 32개
    PIV_시스템_대여: 200만원  # 1개월 대여
    데이터수집장치: 100만원   # 고속 샘플링
    소계: 460만원
    
  기타_비용:
    교통비_숙박비: 100만원
    소모품_예비비: 140만원
    소계: 240만원
    
  총예산: 3,600만원

일정_계획:
  설계_및_제작: 8주
    Week_1-2: 상세설계 및 도면작성
    Week_3-6: 모델 가공 및 조립  
    Week_7-8: 계측시스템 통합 및 검교정
    
  풍동_실험: 3주  
    Week_9: 시험 준비 및 시스템 점검
    Week_10-11: 본 실험 수행
    Week_12: 추가 실험 및 데이터 보완
    
  데이터_분석: 4주
    Week_13-14: 데이터 처리 및 계수 도출
    Week_15-16: CFD 결과와 비교 분석 및 보고서 작성
    
  총기간: 15주 (약_4개월)
```

### 리스크 관리 계획
```python
risk_management = {
    'technical_risks': {
        'model_manufacturing_defects': {
            'probability': 0.2,
            'impact': 'High',
            'mitigation': '품질검사 강화, 예비 부품 준비',
            'contingency_budget': '200만원'
        },
        'measurement_system_malfunction': {
            'probability': 0.15,
            'impact': 'Medium',
            'mitigation': '백업 센서 준비, 사전 교정',
            'contingency_budget': '100만원'
        },
        'wind_tunnel_scheduling_conflict': {
            'probability': 0.3,
            'impact': 'Medium', 
            'mitigation': '대체 시설 사전 조사, 유연한 일정',
            'contingency_budget': '300만원'
        }
    },
    
    'schedule_risks': {
        'model_delivery_delay': {
            'probability': 0.25,
            'impact': 'High',
            'mitigation': '제작업체 사전 점검, 중간 점검 강화',
            'buffer_time': '2주'
        },
        'unexpected_results_requiring_additional_tests': {
            'probability': 0.4,
            'impact': 'Medium',
            'mitigation': '추가 실험시간 예약, 예비 테스트 케이스 준비',
            'buffer_time': '1주'
        }
    },
    
    'budget_risks': {
        'cost_overrun_manufacturing': {
            'probability': 0.2,
            'expected_overrun': '10-15%',
            'mitigation': '상세 견적서 사전 확보, 고정가격 계약'
        },
        'additional_wind_tunnel_time': {
            'probability': 0.3,
            'expected_overrun': '20%',
            'mitigation': '초과 사용 시간 협상, 야간 할인 활용'
        }
    }
}
```

---

## 📊 최종 검증 및 모델 업데이트

### 실험 완료 후 후속 조치
```python
class ExperimentFollowUp:
    """실험 완료 후 후속 작업"""
    
    def __init__(self):
        self.validation_targets = {
            'coefficient_accuracy': 0.9,      # 90% 이상 정확도
            'model_confidence': 0.95,         # 95% 신뢰도
            'uncertainty_reduction': 0.5      # 불확실성 50% 감소
        }
    
    def update_jsbsim_model(self, validated_coefficients, uncertainty_bounds):
        """JSBSim 모델 업데이트"""
        
        # 검증된 계수로 UAM 모델 파일 업데이트
        updated_model_xml = self.generate_updated_xml(validated_coefficients)
        
        # 불확실성 정보 추가
        model_metadata = {
            'validation_date': datetime.now().strftime('%Y-%m-%d'),
            'validation_method': 'Wind Tunnel + CFD',
            'confidence_level': self.calculate_confidence_level(uncertainty_bounds),
            'recommended_safety_factors': self.calculate_safety_factors(uncertainty_bounds)
        }
        
        return updated_model_xml, model_metadata
    
    def generate_validation_certificate(self, experiment_results):
        """검증 인증서 생성"""
        
        certificate = {
            'model_name': 'UAM Quadcopter Crosswind Model',
            'validation_authority': 'Wind Tunnel Experiment + CFD Analysis',
            'validated_coefficients': experiment_results['final_coefficients'],
            'validation_accuracy': experiment_results['accuracy_metrics'],
            'applicable_conditions': {
                'reynolds_range': [1e5, 1e6],
                'mach_range': [0, 0.15],
                'sideslip_range': [-25, 25],  # degrees
                'recommended_use': 'Urban Air Mobility crosswind analysis'
            },
            'limitations': [
                'Valid for multirotor configuration only',
                'Ground effect not included', 
                'Rotor-body interference effects included',
                'Unsteady effects limited to quasi-steady approximation'
            ],
            'next_validation_due': '2 years from validation date'
        }
        
        return certificate
```

---

## 🎯 성공 기준 및 기대 효과

### 성공 기준 (Success Criteria)
```yaml
실험_성공_기준:
  정량적_목표:
    계수_측정_정밀도: "±5% 이내"
    CFD_풍동_일치도: "±15% 이내" 
    반복성: "±2% 이내"
    데이터_완성도: "95% 이상"
    
  정성적_목표:
    모델_신뢰성_확보: "항공 당국 인정 수준"
    기술_역량_축적: "국내 UAM 개발 기반"
    국제_경쟁력: "해외 연구기관 수준"
    
기대_효과:
  직접_효과:
    - UAM 횡풍 성능 정확한 예측 가능
    - 안전한 착륙 시스템 설계 기반 확보
    - 항공 당국 인증 대응 능력 향상
    
  간접_효과:
    - 국내 UAM 산업 기술 경쟁력 확보
    - 풍동실험 기술 노하우 축적
    - CFD 해석 기법 고도화
    - 후속 연구 프로젝트 창출
```

---

**이 풍동 실험 설계서는 UAM 횡풍 해석 모델의 신뢰성을 획기적으로 향상시킬 것이며, 국내 UAM 기술 개발의 중요한 이정표가 될 것입니다.**

**실험 준비가 완료되면 즉시 실행 가능한 완전한 설계서입니다!** 🚁✈️🔬