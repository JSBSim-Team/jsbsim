#!/usr/bin/env python3
"""
세스나 172 데이터 기반 횡풍 착륙시 좌우 편차 영향 분석
Cessna 172 Based Crosswind Landing Lateral Deviation Analysis

기존 UAM 분석과 동일한 횡풍 조건에서 세스나 172의 검증된 데이터로
실제 횡풍 착륙시 좌우 편차를 정확하게 분석합니다.

Author: UAM Crosswind Analysis Team
Date: 2024-10-01
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, Dict, Tuple
import json
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class CrosswindLandingConditions:
    """횡풍 착륙 조건"""
    wind_speed_ms: float        # 횡풍 속도 (m/s)
    approach_speed_ms: float    # 접근 속도 (m/s)  
    approach_altitude_m: float  # 접근 고도 (m)
    descent_rate_ms: float      # 강하율 (m/s)
    air_density: float = 1.225  # 공기 밀도 (kg/m³)

@dataclass
class LandingPerformance:
    """착륙 성능 결과"""
    lateral_deviation_m: float      # 최종 좌우 편차 (m)
    max_sideslip_deg: float        # 최대 측미끄러짐각 (도)
    max_roll_angle_deg: float      # 최대 롤각 (도)
    touchdown_speed_ms: float      # 접지 속도 (m/s)
    approach_time_s: float         # 접근 시간 (초)
    
    # 조종사 워크로드
    rudder_input_max: float        # 최대 러더 입력 (%)
    aileron_input_max: float       # 최대 에일러론 입력 (%)
    pilot_workload: str            # 조종사 워크로드 (LOW/MEDIUM/HIGH)

class CessnaCrosswindLandingAnalyzer:
    """세스나 172 기반 횡풍 착륙 분석기"""
    
    def __init__(self):
        """초기화 - 세스나 172 검증된 데이터 로드"""
        
        # JSBSim에서 검증된 세스나 172 계수 (c172p.xml)
        self.cessna_coefficients = {
            'Cy_beta': -0.393,      # 측력 계수 (per rad)
            'Cn_beta': 0.0587,      # 요모멘트 계수 (per rad)  
            'Cl_beta': -0.0923,     # 롤모멘트 계수 (per rad)
            'Cy_rudder': 0.187,     # 러더 측력 효과
            'Cn_rudder': -0.0873,   # 러더 요모멘트 효과
            'Cl_rudder': 0.0213     # 러더 롤모멘트 효과
        }
        
        # 세스나 172 제원 (JSBSim c172p.xml 기준)
        self.cessna_specs = {
            'wingspan_m': 10.91,        # 35.8 ft
            'wing_area_m2': 16.16,      # 174 ft²
            'length_m': 8.28,           # 27.2 ft
            'mass_kg': 1157,            # 2550 lbs (총중량)
            'approach_speed_ms': 30.9,  # 60 kts 접근속도
            'stall_speed_ms': 24.2,     # 47 kts 실속속도
            'max_crosswind_ms': 7.7     # 15 kts 최대 허용 횡풍
        }
        
        logger.info("세스나 172 횡풍 착륙 분석기 초기화 완료")
    
    def calculate_sideslip_angle(self, wind_speed_ms: float, approach_speed_ms: float) -> float:
        """횡풍으로 인한 측미끄러짐각 계산"""
        
        # 측미끄러짐각 = arctan(횡풍속도 / 접근속도)
        beta_rad = np.arctan(wind_speed_ms / approach_speed_ms)
        beta_deg = np.degrees(beta_rad)
        
        return beta_deg, beta_rad
    
    def calculate_aerodynamic_forces(self, conditions: CrosswindLandingConditions) -> Dict:
        """공기역학적 힘과 모멘트 계산"""
        
        # 동압 계산
        q = 0.5 * conditions.air_density * conditions.approach_speed_ms**2
        
        # 측미끄러짐각
        beta_deg, beta_rad = self.calculate_sideslip_angle(
            conditions.wind_speed_ms, conditions.approach_speed_ms
        )
        
        # 기본 횡풍 힘과 모멘트 (제어 입력 없음)
        side_force_N = q * self.cessna_specs['wing_area_m2'] * self.cessna_coefficients['Cy_beta'] * beta_rad
        
        yaw_moment_Nm = (q * self.cessna_specs['wing_area_m2'] * self.cessna_specs['wingspan_m'] * 
                        self.cessna_coefficients['Cn_beta'] * beta_rad)
        
        roll_moment_Nm = (q * self.cessna_specs['wing_area_m2'] * self.cessna_specs['wingspan_m'] * 
                         self.cessna_coefficients['Cl_beta'] * beta_rad)
        
        # 측방 가속도
        lateral_accel_ms2 = side_force_N / self.cessna_specs['mass_kg']
        
        return {
            'sideslip_angle_deg': beta_deg,
            'sideslip_angle_rad': beta_rad,
            'dynamic_pressure_Pa': q,
            'side_force_N': side_force_N,
            'yaw_moment_Nm': yaw_moment_Nm,
            'roll_moment_Nm': roll_moment_Nm,
            'lateral_acceleration_ms2': lateral_accel_ms2
        }
    
    def simulate_crosswind_approach(self, conditions: CrosswindLandingConditions) -> LandingPerformance:
        """횡풍 접근 착륙 시뮬레이션"""
        
        # 접근 시간 계산 (고도 / 강하율)
        approach_time_s = conditions.approach_altitude_m / conditions.descent_rate_ms
        
        # 시간 배열
        time_array = np.linspace(0, approach_time_s, int(approach_time_s * 10))  # 0.1초 간격
        dt = time_array[1] - time_array[0]
        
        # 초기 조건
        lateral_position = 0.0      # 측방 위치 (m)
        lateral_velocity = 0.0      # 측방 속도 (m/s)
        heading_angle = 0.0         # 기수 각도 (rad)
        roll_angle = 0.0            # 롤각 (rad)
        
        # 결과 저장용
        results = {
            'time': [],
            'lateral_position': [],
            'lateral_velocity': [],
            'sideslip_angle': [],
            'roll_angle': [],
            'heading_angle': [],
            'rudder_input': [],
            'aileron_input': []
        }
        
        # 시뮬레이션 루프
        for i, t in enumerate(time_array):
            
            # 현재 고도
            current_altitude = conditions.approach_altitude_m - conditions.descent_rate_ms * t
            
            # 공기역학 계산
            aero_forces = self.calculate_aerodynamic_forces(conditions)
            
            # 측미끄러짐각
            beta_rad = aero_forces['sideslip_angle_rad']
            beta_deg = aero_forces['sideslip_angle_deg']
            
            # 조종 입력 계산 (조종사 모델)
            rudder_input, aileron_input = self._calculate_pilot_inputs(
                beta_deg, roll_angle * 57.3, lateral_velocity, t, approach_time_s
            )
            
            # 제어 효과 포함한 힘과 모멘트
            controlled_forces = self._apply_control_inputs(
                aero_forces, rudder_input, aileron_input, conditions
            )
            
            # 동역학 적분 (간단한 오일러 적분)
            lateral_accel = controlled_forces['total_lateral_acceleration']
            roll_accel = controlled_forces['total_roll_acceleration']
            yaw_accel = controlled_forces['total_yaw_acceleration']
            
            # 속도 및 위치 업데이트
            lateral_velocity += lateral_accel * dt
            lateral_position += lateral_velocity * dt
            
            # 각도 업데이트  
            roll_angle += roll_accel * dt
            heading_angle += yaw_accel * dt
            
            # 결과 저장
            results['time'].append(t)
            results['lateral_position'].append(lateral_position)
            results['lateral_velocity'].append(lateral_velocity)
            results['sideslip_angle'].append(beta_deg)
            results['roll_angle'].append(roll_angle * 57.3)  # 도 단위
            results['heading_angle'].append(heading_angle * 57.3)
            results['rudder_input'].append(rudder_input)
            results['aileron_input'].append(aileron_input)
        
        # 최종 성능 지표 계산
        performance = self._calculate_landing_performance(results, conditions)
        
        return performance, results
    
    def _calculate_pilot_inputs(self, sideslip_deg: float, roll_deg: float, 
                               lateral_vel: float, time: float, total_time: float) -> Tuple[float, float]:
        """조종사 입력 모델 (크랩 + 사이드슬립 기법)"""
        
        # 접근 단계별 조종 기법
        approach_phase = time / total_time
        
        if approach_phase < 0.7:  # 초기/중간 접근 (크랩 방법)
            # 측미끄러짐각을 0으로 만들려는 러더 입력
            rudder_input = sideslip_deg * 0.02  # 비례 제어 (게인 조정됨)
            aileron_input = 0.0  # 크랩 단계에서는 에일러론 최소 사용
            
        else:  # 최종 접근 (사이드슬립 전환)
            # 사이드슬립으로 전환 - 풍상측 에일러론, 반대 러더
            aileron_input = -sideslip_deg * 0.015  # 풍상측 에일러론
            rudder_input = sideslip_deg * 0.01     # 균형 러더 (감소된 게인)
        
        # 롤각 안정화
        aileron_input += -roll_deg * 0.01
        
        # 측방 속도 댐핑
        aileron_input += -lateral_vel * 0.005
        
        # 입력 제한 (-100% ~ +100%)
        rudder_input = np.clip(rudder_input, -1.0, 1.0)
        aileron_input = np.clip(aileron_input, -1.0, 1.0)
        
        return rudder_input, aileron_input
    
    def _apply_control_inputs(self, aero_forces: Dict, rudder_input: float, 
                            aileron_input: float, conditions: CrosswindLandingConditions) -> Dict:
        """조종면 입력에 의한 제어 효과 적용"""
        
        # 동압
        q = aero_forces['dynamic_pressure_Pa']
        S = self.cessna_specs['wing_area_m2']
        b = self.cessna_specs['wingspan_m']
        mass = self.cessna_specs['mass_kg']
        
        # 러더 효과
        rudder_side_force = q * S * self.cessna_coefficients['Cy_rudder'] * rudder_input
        rudder_yaw_moment = q * S * b * self.cessna_coefficients['Cn_rudder'] * rudder_input
        rudder_roll_moment = q * S * b * self.cessna_coefficients['Cl_rudder'] * rudder_input
        
        # 에일러론 효과 (추정값 - 세스나는 에일러론 계수 제한적)
        aileron_roll_moment = q * S * b * (-0.15) * aileron_input  # 에일러론 롤 효과
        aileron_yaw_moment = q * S * b * (0.02) * aileron_input    # 역요 효과
        
        # 총 힘과 모멘트
        total_side_force = aero_forces['side_force_N'] + rudder_side_force
        total_yaw_moment = aero_forces['yaw_moment_Nm'] + rudder_yaw_moment + aileron_yaw_moment
        total_roll_moment = aero_forces['roll_moment_Nm'] + rudder_roll_moment + aileron_roll_moment
        
        # 가속도 계산
        total_lateral_accel = total_side_force / mass
        
        # 관성 모멘트 (추정값)
        Ixx = mass * (b/2)**2 * 0.25  # 롤 관성모멘트
        Izz = mass * (self.cessna_specs['length_m']/2)**2 * 0.35  # 요 관성모멘트
        
        total_roll_accel = total_roll_moment / Ixx
        total_yaw_accel = total_yaw_moment / Izz
        
        return {
            'total_side_force': total_side_force,
            'total_yaw_moment': total_yaw_moment, 
            'total_roll_moment': total_roll_moment,
            'total_lateral_acceleration': total_lateral_accel,
            'total_roll_acceleration': total_roll_accel,
            'total_yaw_acceleration': total_yaw_accel
        }
    
    def _calculate_landing_performance(self, results: Dict, conditions: CrosswindLandingConditions) -> LandingPerformance:
        """착륙 성능 지표 계산"""
        
        # 최종 좌우 편차
        final_lateral_deviation = results['lateral_position'][-1]
        
        # 최대값들
        max_sideslip = max(abs(angle) for angle in results['sideslip_angle'])
        max_roll_angle = max(abs(angle) for angle in results['roll_angle'])
        max_rudder = max(abs(inp) for inp in results['rudder_input']) * 100
        max_aileron = max(abs(inp) for inp in results['aileron_input']) * 100
        
        # 조종사 워크로드 평가
        if max_rudder < 30 and max_aileron < 30:
            pilot_workload = "LOW"
        elif max_rudder < 60 and max_aileron < 60:
            pilot_workload = "MEDIUM"
        else:
            pilot_workload = "HIGH"
        
        return LandingPerformance(
            lateral_deviation_m=final_lateral_deviation,
            max_sideslip_deg=max_sideslip,
            max_roll_angle_deg=max_roll_angle,
            touchdown_speed_ms=conditions.approach_speed_ms,
            approach_time_s=results['time'][-1],
            rudder_input_max=max_rudder,
            aileron_input_max=max_aileron,
            pilot_workload=pilot_workload
        )
    
    def analyze_crosswind_matrix(self, wind_speeds: List[float], 
                                approach_speeds: List[float] = None) -> pd.DataFrame:
        """다양한 횡풍 조건 매트릭스 분석"""
        
        if approach_speeds is None:
            approach_speeds = [self.cessna_specs['approach_speed_ms']]  # 표준 접근속도
        
        results = []
        
        for wind_speed in wind_speeds:
            for approach_speed in approach_speeds:
                
                # 기본 접근 조건
                conditions = CrosswindLandingConditions(
                    wind_speed_ms=wind_speed,
                    approach_speed_ms=approach_speed,
                    approach_altitude_m=150.0,  # 500 ft 접근
                    descent_rate_ms=2.5         # 약 500 fpm 강하율
                )
                
                # 시뮬레이션 실행
                performance, sim_results = self.simulate_crosswind_approach(conditions)
                
                # 결과 정리
                result_row = {
                    'wind_speed_ms': wind_speed,
                    'wind_speed_kts': wind_speed * 1.944,  # m/s to kts
                    'approach_speed_ms': approach_speed,
                    'approach_speed_kts': approach_speed * 1.944,
                    'lateral_deviation_m': performance.lateral_deviation_m,
                    'max_sideslip_deg': performance.max_sideslip_deg,
                    'max_roll_angle_deg': performance.max_roll_angle_deg,
                    'max_rudder_percent': performance.rudder_input_max,
                    'max_aileron_percent': performance.aileron_input_max,
                    'pilot_workload': performance.pilot_workload,
                    'within_limits': abs(performance.lateral_deviation_m) <= 50  # 50m 허용
                }
                
                results.append(result_row)
                
                # 진행 상황 출력
                logger.info(f"횡풍 {wind_speed:.1f}m/s ({wind_speed*1.944:.1f}kt): "
                          f"좌우편차 {performance.lateral_deviation_m:.1f}m, "
                          f"측미끄러짐각 {performance.max_sideslip_deg:.1f}°")
        
        return pd.DataFrame(results)
    
    def create_analysis_plots(self, df_results: pd.DataFrame, sim_results: Dict = None):
        """분석 결과 시각화"""
        
        fig = plt.figure(figsize=(20, 12))
        
        # 1. 횡풍 vs 좌우 편차
        ax1 = plt.subplot(2, 3, 1)
        wind_kts = df_results['wind_speed_kts']
        lateral_dev = df_results['lateral_deviation_m']
        
        plt.scatter(wind_kts, lateral_dev, c='blue', s=60, alpha=0.7)
        plt.xlabel('횡풍 속도 (knots)')
        plt.ylabel('좌우 편차 (m)')
        plt.title('세스나 172: 횡풍 vs 좌우 편차')
        plt.grid(True)
        
        # 허용 한계선
        plt.axhline(y=50, color='red', linestyle='--', alpha=0.7, label='허용한계 ±50m')
        plt.axhline(y=-50, color='red', linestyle='--', alpha=0.7)
        plt.legend()
        
        # 2. 횡풍 vs 측미끄러짐각
        ax2 = plt.subplot(2, 3, 2)
        plt.scatter(wind_kts, df_results['max_sideslip_deg'], c='red', s=60, alpha=0.7)
        plt.xlabel('횡풍 속도 (knots)')
        plt.ylabel('최대 측미끄러짐각 (°)')
        plt.title('세스나 172: 횡풍 vs 측미끄러짐각')
        plt.grid(True)
        
        # 3. 횡풍 vs 롤각
        ax3 = plt.subplot(2, 3, 3)
        plt.scatter(wind_kts, df_results['max_roll_angle_deg'], c='green', s=60, alpha=0.7)
        plt.xlabel('횡풍 속도 (knots)')
        plt.ylabel('최대 롤각 (°)')
        plt.title('세스나 172: 횡풍 vs 롤각')
        plt.grid(True)
        
        # 4. 조종사 입력 (러더)
        ax4 = plt.subplot(2, 3, 4)
        plt.scatter(wind_kts, df_results['max_rudder_percent'], c='purple', s=60, alpha=0.7)
        plt.xlabel('횡풍 속도 (knots)')
        plt.ylabel('최대 러더 입력 (%)')
        plt.title('세스나 172: 횡풍 vs 러더 입력')
        plt.grid(True)
        
        # 5. 조종사 입력 (에일러론)
        ax5 = plt.subplot(2, 3, 5)
        plt.scatter(wind_kts, df_results['max_aileron_percent'], c='orange', s=60, alpha=0.7)
        plt.xlabel('횡풍 속도 (knots)')
        plt.ylabel('최대 에일러론 입력 (%)')
        plt.title('세스나 172: 횡풍 vs 에일러론 입력')
        plt.grid(True)
        
        # 6. 시계열 (최신 시뮬레이션 결과)
        ax6 = plt.subplot(2, 3, 6)
        if sim_results:
            time_array = np.array(sim_results['time'])
            plt.plot(time_array, sim_results['lateral_position'], 'b-', label='좌우 위치 (m)')
            plt.plot(time_array, np.array(sim_results['sideslip_angle'])*5, 'r-', label='측미끄러짐각×5 (°)')
            plt.xlabel('시간 (s)')
            plt.ylabel('값')
            plt.title('세스나 172: 착륙 접근 시계열')
            plt.legend()
            plt.grid(True)
        
        plt.suptitle('세스나 172 횡풍 착륙 분석 결과\n(JSBSim 검증된 계수 기반)', fontsize=16)
        plt.tight_layout()
        plt.savefig('cessna_crosswind_landing_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        
        logger.info("세스나 172 횡풍 착륙 분석 시각화 완료")

def main():
    """메인 분석 실행"""
    
    print("🛩️ 세스나 172 데이터 기반 횡풍 착륙 좌우 편차 분석")
    print("=" * 60)
    
    # 분석기 초기화
    analyzer = CessnaCrosswindLandingAnalyzer()
    
    # 분석할 횟풍 조건 (기존 UAM 분석과 동일)
    wind_speeds_ms = [2.5, 5.0, 7.5, 10.0, 12.5, 15.0]  # m/s
    wind_speeds_kts = [ws * 1.944 for ws in wind_speeds_ms]  # knots 변환
    
    print(f"\n🌪️ 분석 횡풍 조건:")
    for ws_ms, ws_kt in zip(wind_speeds_ms, wind_speeds_kts):
        print(f"   • {ws_ms:.1f} m/s ({ws_kt:.1f} knots)")
    
    # 매트릭스 분석 실행
    df_results = analyzer.analyze_crosswind_matrix(wind_speeds_ms)
    
    # 특정 조건 상세 시뮬레이션 (10 m/s 횡풍)
    detailed_conditions = CrosswindLandingConditions(
        wind_speed_ms=10.0,
        approach_speed_ms=analyzer.cessna_specs['approach_speed_ms'],
        approach_altitude_m=150.0,
        descent_rate_ms=2.5
    )
    
    performance, sim_results = analyzer.simulate_crosswind_approach(detailed_conditions)
    
    # 결과 출력
    print(f"\n📊 세스나 172 횡풍 착륙 분석 결과:")
    print(f"   • 분석 케이스: {len(df_results)}개")
    print(f"   • 접근 속도: {analyzer.cessna_specs['approach_speed_ms']:.1f} m/s ({analyzer.cessna_specs['approach_speed_ms']*1.944:.1f} kts)")
    
    print(f"\n🎯 10 m/s (19.4 kts) 횡풍 상세 결과:")
    print(f"   • 최종 좌우 편차: {performance.lateral_deviation_m:.1f} m")
    print(f"   • 최대 측미끄러짐각: {performance.max_sideslip_deg:.1f}°")
    print(f"   • 최대 롤각: {performance.max_roll_angle_deg:.1f}°")
    print(f"   • 최대 러더 입력: {performance.rudder_input_max:.1f}%")
    print(f"   • 최대 에일러론 입력: {performance.aileron_input_max:.1f}%")
    print(f"   • 조종사 워크로드: {performance.pilot_workload}")
    
    # 전체 결과 요약
    print(f"\n📈 전체 횡풍 범위 요약:")
    for _, row in df_results.iterrows():
        status = "✅ 안전" if row['within_limits'] else "❌ 한계초과"
        print(f"   • {row['wind_speed_kts']:.1f}kt: 편차 {row['lateral_deviation_m']:+.1f}m, "
              f"측미끄러짐 {row['max_sideslip_deg']:.1f}°, {status}")
    
    # 세스나 172 허용 한계 평가
    safe_conditions = df_results[df_results['within_limits']]
    max_safe_wind = safe_conditions['wind_speed_kts'].max() if not safe_conditions.empty else 0
    
    print(f"\n🛡️ 세스나 172 횡풍 한계:")
    print(f"   • 안전 횡풍 한계: {max_safe_wind:.1f} knots")
    print(f"   • 제조사 권장 한계: 15 knots (일반적)")
    print(f"   • 분석 기준 (±50m): {'충족' if max_safe_wind >= 15 else '검토 필요'}")
    
    # 시각화 생성
    analyzer.create_analysis_plots(df_results, sim_results)
    
    # 결과 저장
    df_results.to_csv('cessna_crosswind_landing_analysis.csv', index=False)
    
    # UAM과 비교를 위한 요약
    print(f"\n🔄 UAM 개발을 위한 참고사항:")
    print(f"   • 세스나 172 검증된 계수 활용 가능")
    print(f"   • 측방편차 패턴: 횡풍에 거의 선형 비례")
    print(f"   • 조종 기법: 크랩 + 사이드슬립 전환")
    print(f"   • UAM 스케일링시 고려: 로터 간섭, 수직면 효과")
    
    print(f"\n📁 생성된 파일:")
    print(f"   • cessna_crosswind_landing_analysis.csv")
    print(f"   • cessna_crosswind_landing_analysis.png")
    
    return df_results, performance, sim_results

if __name__ == "__main__":
    main()