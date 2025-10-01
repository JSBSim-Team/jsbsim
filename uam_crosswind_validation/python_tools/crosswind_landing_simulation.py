#!/usr/bin/env python3
"""
UAM 기체 횡풍 착륙 시뮬레이션
Urban Air Mobility Crosswind Landing Simulation

이 모듈은 UAM 기체의 횡풍 조건에서의 착륙 시 좌우 편차를 분석합니다.
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib
matplotlib.use('Agg')  # GUI가 없는 환경에서 사용
from scipy import signal
from scipy.integrate import solve_ivp
import json
import os
from datetime import datetime

class UAMCrosswindAnalysis:
    """UAM 횡풍 착륙 분석 클래스"""
    
    def __init__(self):
        """초기화"""
        # 기체 파라미터 (UAM Quadcopter 기준)
        self.aircraft_params = {
            'mass': 1134.0,  # kg (2500 lbs)
            'Ixx': 700.0,    # kg⋅m² (518 slug⋅ft²)
            'Iyy': 700.0,    # kg⋅m²
            'Izz': 1166.0,   # kg⋅m² (865 slug⋅ft²)
            'wingspan': 6.0,  # m (로터간 거리)
            'length': 4.57,   # m (15 ft)
            'S': 10.0,       # m² (가상 기준면적)
            'c_bar': 1.5     # m (평균 코드길이)
        }
        
        # 공기역학 계수 (횡풍 특성)
        self.aero_coeffs = {
            'Cy_beta': -0.25,      # 사이드슬립에 의한 측력 계수
            'Cy_delta_r': -0.12,   # 러더에 의한 측력 계수  
            'Cy_p': 0.02,          # 롤레이트에 의한 측력 계수
            'Cy_r': 0.08,          # 요레이트에 의한 측력 계수
            'Cl_beta': -0.08,      # 사이드슬립에 의한 롤모멘트 계수
            'Cl_p': -0.45,         # 롤댐핑 계수
            'Cl_r': 0.02,          # 요레이트에 의한 롤모멘트 계수
            'Cl_delta_a': 0.15,    # 에일러론에 의한 롤모멘트 계수
            'Cl_delta_r': 0.01,    # 러더에 의한 롤모멘트 계수
            'Cn_beta': 0.12,       # 방향안정성 계수
            'Cn_r': -0.25,         # 요댐핑 계수
            'Cn_p': -0.01,         # 롤레이트에 의한 요모멘트 계수
            'Cn_delta_r': -0.08,   # 러더에 의한 요모멘트 계수
            'Cn_delta_a': -0.005   # 에일러론에 의한 요모멘트 계수 (adverse yaw)
        }
        
        # 환경 조건
        self.env_params = {
            'rho': 1.225,      # 공기밀도 (kg/m³)
            'g': 9.81,         # 중력가속도 (m/s²)
            'mu': 1.81e-5     # 동점성계수 (kg/m⋅s)
        }
        
        # 시뮬레이션 결과 저장
        self.results = {}
        
    def calculate_dynamic_pressure(self, velocity):
        """동압 계산"""
        return 0.5 * self.env_params['rho'] * velocity**2
        
    def crosswind_force_model(self, t, state, wind_velocity, wind_direction, control_inputs):
        """
        횡풍 조건에서의 동역학 모델
        
        state: [x, y, z, u, v, w, phi, theta, psi, p, q, r]
        """
        x, y, z, u, v, w, phi, theta, psi, p, q, r = state
        
        # 제어 입력
        delta_a = control_inputs.get('aileron', 0.0)  # 에일러론 [rad]
        delta_e = control_inputs.get('elevator', 0.0)  # 엘리베이터 [rad]
        delta_r = control_inputs.get('rudder', 0.0)    # 러더 [rad]
        thrust = control_inputs.get('thrust', 0.0)     # 추력 [N]
        
        # 풍속 성분
        wind_u = wind_velocity * np.cos(wind_direction)  # 정면풍 성분
        wind_v = wind_velocity * np.sin(wind_direction)  # 횡풍 성분
        
        # 상대속도 (기체 기준)
        u_rel = u - wind_u
        v_rel = v - wind_v
        w_rel = w
        
        # 총 상대속도
        V_rel = np.sqrt(u_rel**2 + v_rel**2 + w_rel**2)
        
        # 공기역학각
        alpha = np.arctan2(w_rel, u_rel) if u_rel != 0 else 0  # 받음각
        beta = np.arcsin(v_rel / V_rel) if V_rel != 0 else 0    # 사이드슬립각
        
        # 동압
        q_bar = self.calculate_dynamic_pressure(V_rel)
        
        # 무차원 각속도
        p_hat = p * self.aircraft_params['wingspan'] / (2 * V_rel) if V_rel != 0 else 0
        q_hat = q * self.aircraft_params['c_bar'] / (2 * V_rel) if V_rel != 0 else 0
        r_hat = r * self.aircraft_params['wingspan'] / (2 * V_rel) if V_rel != 0 else 0
        
        # 공기역학적 힘 계산
        # 측력 (Y방향)
        Cy = (self.aero_coeffs['Cy_beta'] * beta + 
              self.aero_coeffs['Cy_delta_r'] * delta_r +
              self.aero_coeffs['Cy_p'] * p_hat +
              self.aero_coeffs['Cy_r'] * r_hat)
        Y = q_bar * self.aircraft_params['S'] * Cy
        
        # 공기역학적 모멘트 계산
        # 롤 모멘트 (L)
        Cl = (self.aero_coeffs['Cl_beta'] * beta +
              self.aero_coeffs['Cl_p'] * p_hat +
              self.aero_coeffs['Cl_r'] * r_hat +
              self.aero_coeffs['Cl_delta_a'] * delta_a +
              self.aero_coeffs['Cl_delta_r'] * delta_r)
        L_aero = q_bar * self.aircraft_params['S'] * self.aircraft_params['wingspan'] * Cl
        
        # 요 모멘트 (N)
        Cn = (self.aero_coeffs['Cn_beta'] * beta +
              self.aero_coeffs['Cn_r'] * r_hat +
              self.aero_coeffs['Cn_p'] * p_hat +
              self.aero_coeffs['Cn_delta_r'] * delta_r +
              self.aero_coeffs['Cn_delta_a'] * delta_a)
        N_aero = q_bar * self.aircraft_params['S'] * self.aircraft_params['wingspan'] * Cn
        
        # 중력 성분
        mg = self.aircraft_params['mass'] * self.env_params['g']
        
        # 좌표 변환 행렬 (Body to Earth)
        cos_phi = np.cos(phi)
        sin_phi = np.sin(phi)
        cos_theta = np.cos(theta)
        sin_theta = np.sin(theta)
        cos_psi = np.cos(psi)
        sin_psi = np.sin(psi)
        
        # 병진 운동 방정식
        # 힘의 균형 (기체 좌표계)
        X_total = thrust * np.cos(alpha) * np.cos(beta)  # 추력의 X성분
        Y_total = Y + thrust * np.sin(beta)              # 측력 + 추력의 Y성분
        Z_total = thrust * np.sin(alpha) * np.cos(beta) + mg * cos_theta * cos_phi  # 수직력
        
        # 가속도 (기체 좌표계)
        u_dot = X_total / self.aircraft_params['mass'] - q*w + r*v
        v_dot = Y_total / self.aircraft_params['mass'] - r*u + p*w
        w_dot = Z_total / self.aircraft_params['mass'] - p*v + q*u
        
        # 위치 변화 (지구 좌표계)
        position_dot = np.array([
            [cos_theta*cos_psi, sin_phi*sin_theta*cos_psi - cos_phi*sin_psi, cos_phi*sin_theta*cos_psi + sin_phi*sin_psi],
            [cos_theta*sin_psi, sin_phi*sin_theta*sin_psi + cos_phi*cos_psi, cos_phi*sin_theta*sin_psi - sin_phi*cos_psi],
            [-sin_theta, sin_phi*cos_theta, cos_phi*cos_theta]
        ]) @ np.array([u, v, w])
        
        x_dot, y_dot, z_dot = position_dot
        
        # 회전 운동 방정식
        # 각속도 변화
        p_dot = (L_aero + (self.aircraft_params['Iyy'] - self.aircraft_params['Izz']) * q * r) / self.aircraft_params['Ixx']
        q_dot = 0  # 피치 모멘트는 간단히 처리
        r_dot = (N_aero + (self.aircraft_params['Ixx'] - self.aircraft_params['Iyy']) * p * q) / self.aircraft_params['Izz']
        
        # 오일러각 변화
        phi_dot = p + q * sin_phi * np.tan(theta) + r * cos_phi * np.tan(theta)
        theta_dot = q * cos_phi - r * sin_phi
        psi_dot = (q * sin_phi + r * cos_phi) / cos_theta if cos_theta != 0 else 0
        
        return np.array([x_dot, y_dot, z_dot, u_dot, v_dot, w_dot, 
                        phi_dot, theta_dot, psi_dot, p_dot, q_dot, r_dot])
    
    def simulate_crosswind_landing(self, wind_speeds, wind_directions, simulation_time=60.0, dt=0.01):
        """
        다양한 횡풍 조건에서 착륙 시뮬레이션 수행
        
        Parameters:
        -----------
        wind_speeds : list
            풍속 리스트 [m/s]
        wind_directions : list  
            풍향 리스트 [rad] (0: 정면풍, π/2: 우측풍)
        simulation_time : float
            시뮬레이션 시간 [s]
        dt : float
            시간 간격 [s]
        """
        
        results_summary = []
        detailed_results = {}
        
        for wind_speed in wind_speeds:
            for wind_direction in wind_directions:
                
                print(f"시뮬레이션 실행: 풍속 {wind_speed:.1f} m/s, 풍향 {np.degrees(wind_direction):.1f}°")
                
                # 초기 조건 설정 (착륙 접근 단계)
                initial_state = np.array([
                    0.0,    # x [m]
                    0.0,    # y [m] 
                    100.0,  # z [m] (고도)
                    20.0,   # u [m/s] (전진속도)
                    0.0,    # v [m/s] (측방속도)
                    -2.0,   # w [m/s] (하강속도)
                    0.0,    # phi [rad] (롤각)
                    np.radians(-3.0),  # theta [rad] (피치각, 착륙 접근각)
                    0.0,    # psi [rad] (요각)
                    0.0,    # p [rad/s] (롤각속도)
                    0.0,    # q [rad/s] (피치각속도) 
                    0.0     # r [rad/s] (요각속도)
                ])
                
                # 시간 배열
                t_span = (0, simulation_time)
                t_eval = np.arange(0, simulation_time, dt)
                
                # 제어 입력 (간단한 PID 제어기)
                def control_law(t, state):
                    """간단한 자동 제어 법칙"""
                    phi, psi = state[6], state[8]
                    p, r = state[9], state[11]
                    
                    # 롤각 제어 (수평 유지)
                    Kp_phi = -2.0
                    Kd_phi = -0.5
                    aileron = Kp_phi * phi + Kd_phi * p
                    aileron = np.clip(aileron, -0.35, 0.35)  # 제한
                    
                    # 요각 제어 (방향 유지)
                    Kp_psi = -1.0
                    Kd_psi = -0.3
                    rudder = Kp_psi * psi + Kd_psi * r
                    rudder = np.clip(rudder, -0.35, 0.35)  # 제한
                    
                    return {
                        'aileron': aileron,
                        'elevator': 0.0,
                        'rudder': rudder,
                        'thrust': 8000.0  # N
                    }
                
                # 동역학 함수 정의
                def dynamics(t, state):
                    controls = control_law(t, state)
                    return self.crosswind_force_model(t, state, wind_speed, wind_direction, controls)
                
                # 적분 해법
                try:
                    sol = solve_ivp(dynamics, t_span, initial_state, t_eval=t_eval, 
                                  method='RK45', rtol=1e-6, atol=1e-8)
                    
                    if sol.success:
                        # 결과 분석
                        lateral_deviation = np.max(np.abs(sol.y[1]))  # 최대 측방편차
                        final_lateral_pos = sol.y[1][-1]              # 최종 측방위치
                        roll_angle_max = np.max(np.abs(sol.y[6]))     # 최대 롤각
                        yaw_angle_max = np.max(np.abs(sol.y[8]))      # 최대 요각
                        
                        # 착륙 정확도 평가
                        landing_accuracy = np.sqrt(sol.y[0][-1]**2 + sol.y[1][-1]**2)
                        
                        # 결과 저장
                        case_id = f"wind_{wind_speed:.1f}ms_dir_{np.degrees(wind_direction):.0f}deg"
                        
                        results_summary.append({
                            'wind_speed_ms': wind_speed,
                            'wind_direction_deg': np.degrees(wind_direction),
                            'lateral_deviation_m': lateral_deviation,
                            'final_lateral_pos_m': final_lateral_pos,
                            'roll_angle_max_deg': np.degrees(roll_angle_max),
                            'yaw_angle_max_deg': np.degrees(yaw_angle_max),
                            'landing_accuracy_m': landing_accuracy,
                            'case_id': case_id
                        })
                        
                        detailed_results[case_id] = {
                            'time': sol.t,
                            'states': sol.y,
                            'wind_speed': wind_speed,
                            'wind_direction': wind_direction
                        }
                        
                    else:
                        print(f"시뮬레이션 실패: {sol.message}")
                        
                except Exception as e:
                    print(f"시뮬레이션 오류: {e}")
                    
        # 결과 저장
        self.results = {
            'summary': pd.DataFrame(results_summary),
            'detailed': detailed_results
        }
        
        return self.results
    
    def analyze_lateral_deviation(self):
        """좌우 편차 상세 분석"""
        
        if 'summary' not in self.results:
            raise ValueError("시뮬레이션 결과가 없습니다. simulate_crosswind_landing()을 먼저 실행하세요.")
        
        df = self.results['summary']
        
        analysis = {
            'wind_speed_correlation': {},
            'wind_direction_effects': {},
            'critical_conditions': {},
            'safety_margins': {}
        }
        
        # 풍속과 측방편차 상관관계 분석
        for direction in df['wind_direction_deg'].unique():
            subset = df[df['wind_direction_deg'] == direction]
            correlation = np.corrcoef(subset['wind_speed_ms'], subset['lateral_deviation_m'])[0,1]
            analysis['wind_speed_correlation'][f'{direction:.0f}_deg'] = correlation
        
        # 풍향별 영향 분석
        for direction in df['wind_direction_deg'].unique():
            subset = df[df['wind_direction_deg'] == direction]
            analysis['wind_direction_effects'][f'{direction:.0f}_deg'] = {
                'mean_deviation': subset['lateral_deviation_m'].mean(),
                'max_deviation': subset['lateral_deviation_m'].max(),
                'std_deviation': subset['lateral_deviation_m'].std()
            }
        
        # 임계 조건 식별
        critical_threshold = df['lateral_deviation_m'].quantile(0.9)  # 상위 10%
        critical_cases = df[df['lateral_deviation_m'] >= critical_threshold]
        
        analysis['critical_conditions'] = {
            'threshold_m': critical_threshold,
            'critical_cases': critical_cases[['wind_speed_ms', 'wind_direction_deg', 'lateral_deviation_m']].to_dict('records'),
            'most_critical': {
                'wind_speed': critical_cases.loc[critical_cases['lateral_deviation_m'].idxmax(), 'wind_speed_ms'],
                'wind_direction': critical_cases.loc[critical_cases['lateral_deviation_m'].idxmax(), 'wind_direction_deg'],
                'deviation': critical_cases['lateral_deviation_m'].max()
            }
        }
        
        # 안전 여유도 분석 (착륙장 폭을 30m로 가정)
        runway_width = 30.0  # m
        analysis['safety_margins'] = {
            'runway_width_m': runway_width,
            'safe_cases': len(df[df['lateral_deviation_m'] <= runway_width/2]),
            'unsafe_cases': len(df[df['lateral_deviation_m'] > runway_width/2]),
            'safety_rate': len(df[df['lateral_deviation_m'] <= runway_width/2]) / len(df) * 100
        }
        
        return analysis
    
    def generate_mathematical_model(self):
        """수학적 모델 및 수식 생성"""
        
        model_equations = {
            'lateral_dynamics': {
                'description': '횡방향 동역학 방정식',
                'equations': [
                    'dv/dt = (Y/m) - ru + pw',
                    'dp/dt = (L + (I_yy - I_zz)qr)/I_xx', 
                    'dr/dt = (N + (I_xx - I_yy)pq)/I_zz',
                    'dφ/dt = p + q sin(φ)tan(θ) + r cos(φ)tan(θ)',
                    'dψ/dt = (q sin(φ) + r cos(φ))/cos(θ)'
                ]
            },
            'aerodynamic_forces': {
                'description': '공기역학적 힘과 모멘트',
                'equations': [
                    'Y = q̄S[C_y_β β + C_y_δr δ_r + C_y_p p̂ + C_y_r r̂]',
                    'L = q̄Sb[C_l_β β + C_l_p p̂ + C_l_r r̂ + C_l_δa δ_a + C_l_δr δ_r]',
                    'N = q̄Sb[C_n_β β + C_n_r r̂ + C_n_p p̂ + C_n_δr δ_r + C_n_δa δ_a]'
                ]
            },
            'crosswind_effects': {
                'description': '횡풍 영향 모델링',
                'equations': [
                    'β = arcsin(v_rel/V_rel)',
                    'v_rel = v - V_wind sin(ψ_wind)',
                    'u_rel = u - V_wind cos(ψ_wind)',
                    'V_rel = √(u_rel² + v_rel² + w_rel²)'
                ]
            },
            'coefficients': {
                'description': '공기역학 계수 (UAM 기체)',
                'values': self.aero_coeffs
            }
        }
        
        return model_equations
        
    def create_visualizations(self, output_dir):
        """결과 시각화 생성"""
        
        if 'summary' not in self.results:
            raise ValueError("시뮬레이션 결과가 없습니다.")
        
        os.makedirs(output_dir, exist_ok=True)
        df = self.results['summary']
        
        # 1. 풍속 vs 측방편차
        plt.figure(figsize=(12, 8))
        
        for direction in sorted(df['wind_direction_deg'].unique()):
            subset = df[df['wind_direction_deg'] == direction]
            plt.plot(subset['wind_speed_ms'], subset['lateral_deviation_m'], 
                    'o-', label=f'풍향 {direction:.0f}°', linewidth=2, markersize=8)
        
        plt.xlabel('풍속 [m/s]', fontsize=14)
        plt.ylabel('최대 측방편차 [m]', fontsize=14)
        plt.title('UAM 기체 횡풍 착륙 시 측방편차 분석', fontsize=16, fontweight='bold')
        plt.grid(True, alpha=0.3)
        plt.legend(fontsize=12)
        plt.tight_layout()
        plt.savefig(f'{output_dir}/lateral_deviation_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        # 2. 풍향별 영향 분석 (히트맵)
        pivot_data = df.pivot(index='wind_direction_deg', columns='wind_speed_ms', values='lateral_deviation_m')
        
        plt.figure(figsize=(12, 8))
        im = plt.imshow(pivot_data.values, aspect='auto', cmap='YlOrRd', interpolation='nearest')
        plt.colorbar(im, label='측방편차 [m]')
        plt.xticks(range(len(pivot_data.columns)), [f'{x:.1f}' for x in pivot_data.columns])
        plt.yticks(range(len(pivot_data.index)), [f'{x:.0f}°' for x in pivot_data.index])
        plt.xlabel('풍속 [m/s]', fontsize=14)
        plt.ylabel('풍향 [도]', fontsize=14)
        plt.title('횡풍 조건별 측방편차 히트맵', fontsize=16, fontweight='bold')
        plt.tight_layout()
        plt.savefig(f'{output_dir}/crosswind_heatmap.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        # 3. 시계열 데이터 (대표 사례)
        if self.results['detailed']:
            case_key = list(self.results['detailed'].keys())[0]
            case_data = self.results['detailed'][case_key]
            
            fig, axes = plt.subplots(3, 2, figsize=(15, 12))
            
            # 위치
            axes[0,0].plot(case_data['time'], case_data['states'][0], label='X (종방향)')
            axes[0,0].plot(case_data['time'], case_data['states'][1], label='Y (횡방향)', linewidth=2)
            axes[0,0].plot(case_data['time'], case_data['states'][2], label='Z (수직)')
            axes[0,0].set_ylabel('위치 [m]')
            axes[0,0].set_title('위치 변화')
            axes[0,0].legend()
            axes[0,0].grid(True, alpha=0.3)
            
            # 속도
            axes[0,1].plot(case_data['time'], case_data['states'][3], label='u (종방향)')
            axes[0,1].plot(case_data['time'], case_data['states'][4], label='v (횡방향)', linewidth=2)
            axes[0,1].plot(case_data['time'], case_data['states'][5], label='w (수직)')
            axes[0,1].set_ylabel('속도 [m/s]')
            axes[0,1].set_title('속도 변화')
            axes[0,1].legend()
            axes[0,1].grid(True, alpha=0.3)
            
            # 오일러각
            axes[1,0].plot(case_data['time'], np.degrees(case_data['states'][6]), label='φ (롤)')
            axes[1,0].plot(case_data['time'], np.degrees(case_data['states'][7]), label='θ (피치)')
            axes[1,0].plot(case_data['time'], np.degrees(case_data['states'][8]), label='ψ (요)')
            axes[1,0].set_ylabel('각도 [도]')
            axes[1,0].set_title('자세각 변화')
            axes[1,0].legend()
            axes[1,0].grid(True, alpha=0.3)
            
            # 각속도
            axes[1,1].plot(case_data['time'], np.degrees(case_data['states'][9]), label='p (롤레이트)')
            axes[1,1].plot(case_data['time'], np.degrees(case_data['states'][10]), label='q (피치레이트)')
            axes[1,1].plot(case_data['time'], np.degrees(case_data['states'][11]), label='r (요레이트)')
            axes[1,1].set_ylabel('각속도 [도/s]')
            axes[1,1].set_title('각속도 변화')
            axes[1,1].legend()
            axes[1,1].grid(True, alpha=0.3)
            
            # 궤적 (상면도)
            axes[2,0].plot(case_data['states'][0], case_data['states'][1], 'b-', linewidth=2)
            axes[2,0].plot(case_data['states'][0][0], case_data['states'][1][0], 'go', markersize=10, label='시작점')
            axes[2,0].plot(case_data['states'][0][-1], case_data['states'][1][-1], 'ro', markersize=10, label='착륙점')
            axes[2,0].set_xlabel('X 위치 [m]')
            axes[2,0].set_ylabel('Y 위치 [m]')
            axes[2,0].set_title('비행 궤적 (상면도)')
            axes[2,0].legend()
            axes[2,0].grid(True, alpha=0.3)
            axes[2,0].axis('equal')
            
            # 고도 프로파일
            axes[2,1].plot(case_data['states'][0], case_data['states'][2], 'b-', linewidth=2)
            axes[2,1].set_xlabel('X 위치 [m]')
            axes[2,1].set_ylabel('고도 [m]')
            axes[2,1].set_title('고도 프로파일')
            axes[2,1].grid(True, alpha=0.3)
            
            plt.suptitle(f'시계열 분석 - {case_key}', fontsize=16, fontweight='bold')
            plt.tight_layout()
            plt.savefig(f'{output_dir}/time_series_analysis.png', dpi=300, bbox_inches='tight')
            plt.close()
        
        print(f"시각화 결과가 {output_dir}에 저장되었습니다.")

def main():
    """메인 실행 함수"""
    
    print("UAM 기체 횡풍 착륙 분석 시뮬레이션 시작")
    print("=" * 50)
    
    # 분석 객체 생성
    analyzer = UAMCrosswindAnalysis()
    
    # 시뮬레이션 조건 설정
    wind_speeds = [2, 5, 8, 10, 12, 15]  # m/s
    wind_directions = [np.radians(angle) for angle in [0, 30, 60, 90, 120, 150, 180]]  # 다양한 풍향
    
    print(f"풍속 조건: {wind_speeds} m/s")
    print(f"풍향 조건: {[np.degrees(d) for d in wind_directions]} 도")
    
    # 시뮬레이션 실행
    results = analyzer.simulate_crosswind_landing(
        wind_speeds=wind_speeds,
        wind_directions=wind_directions,
        simulation_time=30.0,
        dt=0.05
    )
    
    if results and 'summary' in results:
        print(f"\n시뮬레이션 완료: {len(results['summary'])} 케이스")
        
        # 상세 분석
        analysis = analyzer.analyze_lateral_deviation()
        
        # 수학적 모델
        math_model = analyzer.generate_mathematical_model()
        
        # 결과 저장 디렉토리
        results_dir = "/home/user/webapp/uam_crosswind_analysis/results"
        os.makedirs(results_dir, exist_ok=True)
        
        # 결과 저장
        results['summary'].to_csv(f"{results_dir}/simulation_summary.csv", index=False)
        
        # JSON 직렬화를 위한 데이터 변환
        def convert_numpy_types(obj):
            if isinstance(obj, np.integer):
                return int(obj)
            elif isinstance(obj, np.floating):
                return float(obj)
            elif isinstance(obj, np.ndarray):
                return obj.tolist()
            elif isinstance(obj, dict):
                return {k: convert_numpy_types(v) for k, v in obj.items()}
            elif isinstance(obj, list):
                return [convert_numpy_types(v) for v in obj]
            return obj
        
        analysis_serializable = convert_numpy_types(analysis)
        
        with open(f"{results_dir}/detailed_analysis.json", 'w', encoding='utf-8') as f:
            json.dump(analysis_serializable, f, ensure_ascii=False, indent=2)
            
        with open(f"{results_dir}/mathematical_model.json", 'w', encoding='utf-8') as f:
            json.dump(math_model, f, ensure_ascii=False, indent=2)
        
        # 시각화 생성
        analyzer.create_visualizations(results_dir)
        
        # 요약 출력
        print("\n=== 시뮬레이션 결과 요약 ===")
        print(f"최대 측방편차: {results['summary']['lateral_deviation_m'].max():.2f} m")
        print(f"평균 측방편차: {results['summary']['lateral_deviation_m'].mean():.2f} m")
        print(f"가장 위험한 조건:")
        critical_case = results['summary'].loc[results['summary']['lateral_deviation_m'].idxmax()]
        print(f"  풍속: {critical_case['wind_speed_ms']:.1f} m/s")  
        print(f"  풍향: {critical_case['wind_direction_deg']:.0f}°")
        print(f"  측방편차: {critical_case['lateral_deviation_m']:.2f} m")
        
        print(f"\n결과 파일이 {results_dir}에 저장되었습니다.")
        
    else:
        print("시뮬레이션 실패")

if __name__ == "__main__":
    main()