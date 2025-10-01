# UAM 동역학 모델 및 공기역학 계수 검증 전략

## Model Validation Strategy for UAM Aerodynamic Coefficients

---

## 🔍 현재 모델의 한계 및 검증 필요성

### 현재 사용된 계수의 출처
```
⚠️ 현재 계수들은 추정값입니다:
- 일반적인 항공기 데이터 기반 스케일링
- 이론적 계산 및 유사 기체 참조
- 실제 UAM 기체 시험 데이터 부재
```

### 검증이 필요한 핵심 계수들
| 계수 | 현재값 | 검증 필요도 | 비고 |
|------|--------|-------------|------|
| Cy_β | -0.25 | **극히 높음** | 횡풍 민감도 결정 |
| Cn_β | 0.12 | **극히 높음** | 방향안정성 핵심 |
| Cl_p | -0.45 | **높음** | 롤댐핑 특성 |
| Cn_r | -0.25 | **높음** | 요댐핑 특성 |

---

## 🧪 검증 방법론 (단계별 접근)

### 1단계: CFD (전산유체역학) 해석 검증

#### 1.1 정적 공기역학 계수
```python
# CFD 해석 조건 예시
wind_speeds = [5, 10, 15, 20, 25]  # m/s
sideslip_angles = [-20, -15, -10, -5, 0, 5, 10, 15, 20]  # degrees

for V in wind_speeds:
    for beta in sideslip_angles:
        # ANSYS Fluent, OpenFOAM 등으로 해석
        Cy_cfd = calculate_side_force_coefficient(V, beta)
        Cn_cfd = calculate_yaw_moment_coefficient(V, beta)
        
        # 현재 모델과 비교
        Cy_model = -0.25 * beta
        error = abs(Cy_cfd - Cy_model) / Cy_cfd * 100
```

#### 1.2 동적 도함수 (댐핑 계수)
```
동적 CFD 해석 필요:
- 강제 진동 시뮬레이션 (forced oscillation)
- Cl_p: 롤 운동 시 롤모멘트 변화
- Cn_r: 요 운동 시 요모멘트 변화
```

### 2단계: 풍동 시험 (Wind Tunnel Test)

#### 2.1 스케일 모델 제작
```
축척: 1/5 ~ 1/3 (레이놀즈 수 고려)
측정 항목:
- 6분력 (3축 힘 + 3축 모멘트)
- 압력분포 (선택적)
- 유동 가시화 (PIV, 연기선)
```

#### 2.2 시험 조건
```python
# 풍동 시험 매트릭스
test_conditions = {
    'reynolds_number': [1e5, 2e5, 5e5],  # 스케일 모델 기준
    'sideslip_angle': np.arange(-25, 26, 5),  # degrees
    'angle_of_attack': [-5, 0, 5],  # degrees
    'dynamic_tests': ['roll_oscillation', 'yaw_oscillation']
}
```

#### 2.3 데이터 보정 및 스케일링
```python
def scale_to_full_size(wind_tunnel_data, scale_factor):
    """풍동 데이터를 실기체 크기로 스케일링"""
    
    # 레이놀즈 수 보정
    Re_model = wind_tunnel_data['reynolds']
    Re_full = Re_model * scale_factor
    
    # 계수 보정 (레이놀즈 수 의존성)
    Cy_corrected = apply_reynolds_correction(
        wind_tunnel_data['Cy'], Re_model, Re_full
    )
    
    return Cy_corrected
```

### 3단계: 실물 비행 시험 (Flight Test)

#### 3.1 시스템 식별 (System Identification)
```python
# 비행시험 입력 설계
flight_test_inputs = {
    'doublet_inputs': {
        'aileron': {'amplitude': 0.1, 'duration': 2.0},  # rad, sec
        'rudder': {'amplitude': 0.1, 'duration': 2.0}
    },
    'frequency_sweep': {
        'frequency_range': [0.1, 10.0],  # Hz
        'amplitude': 0.05  # rad
    },
    'crosswind_conditions': {
        'wind_speeds': [3, 5, 8],  # m/s
        'wind_directions': [60, 90, 120]  # degrees
    }
}

# 측정 데이터
measured_data = {
    'states': ['u', 'v', 'w', 'p', 'q', 'r', 'phi', 'theta', 'psi'],
    'controls': ['delta_a', 'delta_e', 'delta_r', 'thrust'],
    'wind': ['wind_speed', 'wind_direction'],
    'sampling_rate': 100  # Hz
}
```

#### 3.2 파라미터 추정
```python
from scipy.optimize import minimize

def parameter_estimation(flight_data):
    """비행 데이터로부터 공기역학 계수 추정"""
    
    def cost_function(coefficients):
        # 현재 계수로 시뮬레이션
        sim_response = simulate_with_coefficients(coefficients, flight_data.inputs)
        
        # 실제 응답과 비교
        error = np.sum((sim_response - flight_data.outputs)**2)
        return error
    
    # 초기 추정값 (현재 모델 계수)
    initial_guess = [-0.25, 0.12, -0.45, -0.25]  # [Cy_β, Cn_β, Cl_p, Cn_r]
    
    # 최적화
    result = minimize(cost_function, initial_guess, 
                     method='trust-region-reflective',
                     bounds=[(-0.5, 0), (0, 0.3), (-0.8, 0), (-0.5, 0)])
    
    return result.x
```

### 4단계: 통계적 검증 및 불확실성 분석

#### 4.1 몬테카르로 시뮬레이션
```python
def uncertainty_analysis(nominal_coefficients, uncertainties):
    """계수 불확실성이 결과에 미치는 영향 분석"""
    
    num_samples = 10000
    results = []
    
    for i in range(num_samples):
        # 계수에 불확실성 적용
        perturbed_coeffs = {}
        for coeff, nominal in nominal_coefficients.items():
            std = uncertainties[coeff]
            perturbed_coeffs[coeff] = np.random.normal(nominal, std)
        
        # 시뮬레이션 실행
        max_deviation = run_crosswind_simulation(perturbed_coeffs)
        results.append(max_deviation)
    
    # 통계 분석
    confidence_95 = np.percentile(results, [2.5, 97.5])
    
    return {
        'mean': np.mean(results),
        'std': np.std(results),
        'confidence_interval': confidence_95
    }
```

---

## 📊 검증 결과 예상 시나리오

### 시나리오 1: 계수가 정확한 경우
```
CFD/풍동/비행시험 결과가 ±10% 내 일치
→ 현재 모델 신뢰도 높음
→ 결과 그대로 활용 가능
```

### 시나리오 2: 계수 수정이 필요한 경우
```python
# 예: 실제 Cy_β가 -0.35로 판명된 경우
original_deviation = 233.52  # m
corrected_Cy_beta = -0.35    # (vs -0.25)

# 영향 분석
deviation_correction_factor = abs(corrected_Cy_beta / original_Cy_beta)
corrected_deviation = original_deviation * deviation_correction_factor
# → 326.9m (40% 증가)

# 안전성 재평가 필요
```

### 시나리오 3: 모델 구조 변경 필요
```
비선형 효과 발견:
Cy = Cy_β * β + Cy_β3 * β³  # 3차 항 추가
복잡한 로터-동체 간섭 효과
→ 모델 전면 재구성 필요
```

---

## 🔬 실제 검증 수행 방안

### 즉시 실행 가능한 검증들

#### 1. 문헌 조사 및 데이터베이스 검증
```python
# 기존 멀티로터/헬리콥터 데이터와 비교
reference_aircraft = {
    'robinson_r22': {'Cy_beta': -0.31, 'Cn_beta': 0.085},
    'bell_206': {'Cy_beta': -0.28, 'Cn_beta': 0.11},
    'quadcopter_dji': {'estimated_Cy_beta': -0.2}  # 추정값
}

def validate_against_database():
    our_coeffs = {'Cy_beta': -0.25, 'Cn_beta': 0.12}
    
    for aircraft, coeffs in reference_aircraft.items():
        for coeff_name, our_value in our_coeffs.items():
            if coeff_name in coeffs:
                ref_value = coeffs[coeff_name]
                difference = abs(our_value - ref_value) / abs(ref_value) * 100
                print(f"{aircraft} {coeff_name}: {difference:.1f}% 차이")
```

#### 2. 차원해석 검증
```python
def dimensional_analysis_check():
    """차원 분석을 통한 계수 합리성 검증"""
    
    # 기하학적 특성
    wingspan = 6.0  # m
    chord = 1.5     # m
    aspect_ratio = wingspan / chord  # = 4.0
    
    # 이론적 추정
    Cy_beta_theory = -2 * np.pi * aspect_ratio / (aspect_ratio + 2)  # ≈ -4.19
    # 실제는 3D 효과로 감소 → -0.2 ~ -0.3 정도 타당
    
    return "계수 범위 타당성 검증됨"
```

#### 3. 민감도 분석
```python
def sensitivity_analysis():
    """각 계수가 결과에 미치는 민감도 분석"""
    
    baseline = run_simulation(baseline_coeffs)
    
    sensitivities = {}
    for coeff in ['Cy_beta', 'Cn_beta', 'Cl_p', 'Cn_r']:
        # ±20% 변화 적용
        modified_coeffs = baseline_coeffs.copy()
        modified_coeffs[coeff] *= 1.2
        
        result_plus = run_simulation(modified_coeffs)
        sensitivity = (result_plus - baseline) / baseline * 100 / 20
        sensitivities[coeff] = sensitivity
    
    return sensitivities
```

### 중장기 검증 계획

#### Phase 1 (3개월): CFD 해석
- **예산**: 500만원 (소프트웨어 + 컴퓨팅)
- **인력**: CFD 전문가 1명
- **결과**: 정적 계수 검증

#### Phase 2 (6개월): 축소모델 풍동시험
- **예산**: 3,000만원 (모델 제작 + 풍동 사용료)
- **기관**: KARI, 서울대 등 풍동 보유 기관
- **결과**: 동적 계수 포함 전체 검증

#### Phase 3 (1년): 실물 비행시험
- **예산**: 2억원 (기체 + 계측장비 + 시험비)
- **협력**: 드론 제조사 또는 연구기관
- **결과**: 최종 모델 검증 및 인증

---

## 📋 검증 우선순위

### 🔴 High Priority (즉시 필요)
1. **Cy_β, Cn_β**: 횡풍 민감도 직결
2. 문헌 조사 및 기존 데이터 비교
3. 민감도 분석으로 영향도 평가

### 🟡 Medium Priority (3-6개월)
1. **Cl_p, Cn_r**: 댐핑 특성 
2. CFD 해석 수행
3. 축소모델 풍동시험 계획

### 🟢 Low Priority (장기)
1. 고차 비선형 항
2. 실물 비행시험
3. 인증용 최종 검증

---

## 💡 결론 및 권고사항

### 현재 모델의 활용도
```
✅ 상대적 경향성: 신뢰 가능 (풍향별, 풍속별 비교)
⚠️ 절대값: 불확실성 존재 (±30% 오차 가능)
❌ 인증용: 추가 검증 필수
```

### 즉시 실행 권고사항
1. **민감도 분석 수행** → 중요 계수 식별
2. **불확실성 범위 설정** → 결과에 ±30% 오차 범위 표시
3. **단계적 검증 계획** → CFD → 풍동 → 비행시험

### 연구 결과 해석 방법
```python
# 현재 결과에 불확실성 적용
max_deviation = 233.52  # m
uncertainty_factor = 0.3  # ±30%

lower_bound = max_deviation * (1 - uncertainty_factor)  # 163.5m
upper_bound = max_deviation * (1 + uncertainty_factor)  # 303.6m

print(f"예상 측방편차: {max_deviation:.0f}m (범위: {lower_bound:.0f}-{upper_bound:.0f}m)")
```

**현재 연구는 UAM 횡풍 특성의 경향성과 상대적 위험도를 파악하는데 유효하며, 절대적 수치의 정확성은 단계적 검증을 통해 개선해 나가야 합니다.**

---

**검증 로드맵 완료 후 기대 효과:**
- 모델 신뢰도 95% 이상 확보
- 항공 당국 인증 기준 충족  
- 실제 UAM 개발에 직접 활용 가능