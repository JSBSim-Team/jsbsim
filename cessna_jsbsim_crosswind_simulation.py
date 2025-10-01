#!/usr/bin/env python3
"""
세스나 172 JSBSim 횡풍 시뮬레이션
Cessna 172 JSBSim Crosswind Simulation

JSBSim을 직접 사용하여 세스나 172의 횡풍 착륙 시뮬레이션을 수행합니다.
실제 JSBSim FDM을 통해 6-DOF 횡풍 동역학을 시뮬레이션합니다.

Author: UAM Crosswind Validation Team  
Date: 2024-10-01
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import List, Dict, Tuple
import json
import logging
import subprocess
import os
from pathlib import Path

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@dataclass
class SimulationParameters:
    """시뮬레이션 매개변수"""
    aircraft_model: str = "c172p"
    initial_altitude: float = 500.0  # ft
    initial_speed: float = 70.0      # kts
    simulation_time: float = 120.0   # seconds
    dt: float = 0.01                 # time step
    
    # 횡풍 조건
    wind_speed: float = 15.0         # kts
    wind_direction: float = 90.0     # degrees (순수 횡풍)
    
    # 착륙 접근 조건
    approach_angle: float = -3.0     # degrees
    target_runway: str = "09"        # runway heading 090

@dataclass 
class FlightCondition:
    """비행 조건 데이터"""
    time: float
    altitude: float     # ft
    airspeed: float    # kts
    sideslip: float    # degrees
    heading: float     # degrees
    roll_angle: float  # degrees
    lateral_deviation: float  # ft
    
    # 힘과 모멘트
    side_force: float  # lbs
    yaw_moment: float  # ft-lbs
    roll_moment: float # ft-lbs

class CessnaJSBSimSimulator:
    """세스나 172 JSBSim 시뮬레이터"""
    
    def __init__(self, jsbsim_path: str = "/home/user/webapp"):
        """초기화"""
        self.jsbsim_path = Path(jsbsim_path)
        self.aircraft_path = self.jsbsim_path / "aircraft"
        
        # JSBSim 실행 파일 확인
        self.jsbsim_executable = self._find_jsbsim_executable()
        
        logger.info(f"JSBSim 시뮬레이터 초기화: {self.jsbsim_path}")
    
    def _find_jsbsim_executable(self) -> str:
        """JSBSim 실행 파일 찾기"""
        
        # 가능한 JSBSim 실행 파일 경로들
        possible_paths = [
            "JSBSim",
            "/usr/local/bin/JSBSim", 
            "/usr/bin/JSBSim",
            str(self.jsbsim_path / "build" / "src" / "JSBSim"),
            "python3 -c \"import jsbsim; jsbsim.FGFDMExec().run_ic()\""
        ]
        
        for path in possible_paths:
            try:
                if "python" in path:
                    # Python JSBSim 모듈 테스트
                    result = subprocess.run(["python3", "-c", "import jsbsim; print('JSBSim Python OK')"], 
                                          capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        logger.info("Python JSBSim 모듈 사용 가능")
                        return "python_jsbsim"
                else:
                    # 실행 파일 테스트
                    result = subprocess.run([path, "--version"], capture_output=True, text=True, timeout=5)
                    if result.returncode == 0:
                        logger.info(f"JSBSim 실행 파일 발견: {path}")
                        return path
            except:
                continue
        
        logger.warning("JSBSim 실행 파일을 찾을 수 없음 - 시뮬레이션 스크립트 생성만 수행")
        return None
    
    def create_crosswind_script(self, params: SimulationParameters) -> str:
        """횡풍 시뮬레이션 JSBSim 스크립트 생성"""
        
        script_content = f"""<?xml version="1.0" encoding="UTF-8"?>
<runscript xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
           xsi:noNamespaceSchemaLocation="http://jsbsim.sourceforge.net/JSBSimScript.xsd"
           name="Cessna 172 Crosswind Landing">
           
  <description>
    Cessna 172 crosswind landing simulation
    Wind: {params.wind_speed} kts from {params.wind_direction} degrees
    Approach: {params.approach_angle} degree glideslope
  </description>

  <use aircraft="{params.aircraft_model}" initialize="cruise"/>

  <!-- 초기 조건 설정 -->
  <run start="0.0" end="{params.simulation_time}" dt="{params.dt}">
    
    <!-- 초기 위치 및 자세 -->
    <property value="0.0">ic/lat-gc-deg</property>
    <property value="0.0">ic/long-gc-deg</property>
    <property value="{params.initial_altitude}">ic/h-sl-ft</property>
    <property value="{params.initial_speed}">ic/vc-kts</property>
    <property value="0.0">ic/gamma-deg</property>
    <property value="90.0">ic/psi-true-deg</property>
    
    <!-- 횡풍 조건 설정 -->
    <property value="{params.wind_speed}">atmosphere/wind-mag-fps</property>
    <property value="{params.wind_direction}">atmosphere/wind-dir-deg</property>
    <property value="0.0">atmosphere/wind-down-fps</property>
    
    <!-- 자동조종장치 해제 (수동 조종) -->
    <property value="0">ap/autopilot_engage</property>
    
    <!-- 엔진 설정 (접근 파워) -->
    <property value="0.4">fcs/throttle-cmd-norm</property>
    <property value="0.4">fcs/mixture-cmd-norm</property>
    
    <!-- 착륙장치 전개 -->
    <property value="1">gear/gear-cmd-norm</property>
    
    <!-- 플랩 설정 (착륙 설정) -->
    <property value="30">fcs/flap-cmd-deg</property>
    
    <!-- 횡풍 접근 이벤트들 -->
    
    <!-- 0-30초: 안정화된 접근 -->
    <event name="stable_approach" persistent="false">
      <condition>simulation/sim-time-sec ge 0.0</condition>
      <set name="fcs/elevator-cmd-norm" value="-0.1"/>
      <set name="fcs/aileron-cmd-norm" value="0.0"/>
      <set name="fcs/rudder-cmd-norm" value="0.0"/>
    </event>
    
    <!-- 30-60초: 횡풍 보정 시작 (크랩 방법) -->
    <event name="crab_correction" persistent="false">
      <condition>simulation/sim-time-sec ge 30.0</condition>
      <set name="fcs/rudder-cmd-norm" value="0.2"/>  <!-- 풍향으로 기수 돌림 -->
      <notify>
        <property>simulation/sim-time-sec</property>
        <property>aero/beta-deg</property>
        <property>attitude/psi-deg</property>
      </notify>
    </event>
    
    <!-- 60-90초: 사이드슬립 전환 -->
    <event name="sideslip_transition" persistent="false">
      <condition>simulation/sim-time-sec ge 60.0</condition>
      <set name="fcs/aileron-cmd-norm" value="-0.15"/>  <!-- 풍상측 에일러론 -->
      <set name="fcs/rudder-cmd-norm" value="0.1"/>     <!-- 반대 러더로 균형 -->
      <notify>
        <property>simulation/sim-time-sec</property>
        <property>aero/beta-deg</property>
        <property>attitude/phi-deg</property>
      </notify>
    </event>
    
    <!-- 90-120초: 최종 접지 접근 -->
    <event name="final_approach" persistent="false">
      <condition>simulation/sim-time-sec ge 90.0</condition>
      <set name="fcs/aileron-cmd-norm" value="-0.2"/>
      <set name="fcs/rudder-cmd-norm" value="0.15"/>
      <set name="fcs/elevator-cmd-norm" value="0.05"/>  <!-- 플레어 시작 -->
      <notify>
        <property>simulation/sim-time-sec</property>
        <property>position/h-agl-ft</property>
        <property>aero/beta-deg</property>
      </notify>
    </event>

    <!-- 데이터 로깅 설정 -->
    <event name="log_data" persistent="true">
      <condition>simulation/sim-time-sec ge 0.0</condition>
      <notify format="CSV">
        <property caption="Time">simulation/sim-time-sec</property>
        <property caption="Altitude_AGL">position/h-agl-ft</property>
        <property caption="Airspeed">velocities/vc-kts</property>
        <property caption="Sideslip">aero/beta-deg</property>
        <property caption="Heading">attitude/psi-deg</property>
        <property caption="Roll">attitude/phi-deg</property>
        <property caption="Pitch">attitude/theta-deg</property>
        <property caption="Lateral_Pos">position/distance-from-start-lon-mt</property>
        <property caption="Side_Force">forces/fby-lbs</property>
        <property caption="Yaw_Moment">moments/n-aero-lbsft</property>
        <property caption="Roll_Moment">moments/l-aero-lbsft</property>
        <property caption="Wind_Speed">atmosphere/wind-mag-fps</property>
        <property caption="Wind_Dir">atmosphere/wind-dir-deg</property>
        <property caption="Aileron">fcs/aileron-pos-norm</property>
        <property caption="Rudder">fcs/rudder-pos-norm</property>
        <property caption="Elevator">fcs/elevator-pos-norm</property>
      </notify>
    </event>
    
  </run>

</runscript>"""
        
        script_path = "cessna_crosswind_simulation.xml"
        with open(script_path, 'w') as f:
            f.write(script_content)
            
        logger.info(f"JSBSim 스크립트 생성: {script_path}")
        return script_path
    
    def run_simulation(self, script_path: str) -> bool:
        """JSBSim 시뮬레이션 실행"""
        
        if self.jsbsim_executable is None:
            logger.warning("JSBSim 실행 파일이 없어 시뮬레이션 생략")
            return False
        
        try:
            if self.jsbsim_executable == "python_jsbsim":
                # Python JSBSim 모듈 사용
                success = self._run_python_jsbsim(script_path)
            else:
                # 실행 파일 사용
                cmd = [
                    self.jsbsim_executable,
                    f"--script={script_path}",
                    f"--aircraft-path={self.aircraft_path}",
                    "--logdirectivefile=data_output/flightgear.xml",
                    "--nice"
                ]
                
                logger.info(f"JSBSim 시뮬레이션 실행: {' '.join(cmd)}")
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
                
                if result.returncode == 0:
                    logger.info("JSBSim 시뮬레이션 성공")
                    success = True
                else:
                    logger.error(f"JSBSim 시뮬레이션 실패: {result.stderr}")
                    success = False
            
            return success
            
        except Exception as e:
            logger.error(f"시뮬레이션 실행 오류: {e}")
            return False
    
    def _run_python_jsbsim(self, script_path: str) -> bool:
        """Python JSBSim 모듈을 사용한 시뮬레이션"""
        
        python_script = f"""
import jsbsim
import csv
import numpy as np

# JSBSim FDM 초기화
fdm = jsbsim.FGFDMExec()
fdm.set_aircraft_path('{self.aircraft_path}')

# 항공기 모델 로드
fdm.load_model('c172p')

# 초기 조건 설정
ic = fdm.get_ic()
ic.set_altitude_sl_ft(500.0)
ic.set_vc_kts(70.0)
ic.set_psi_true_deg(90.0)
ic.set_lat_gc_deg(0.0)
ic.set_long_gc_deg(0.0)

# 대기 조건 설정  
fdm.set_property_value('atmosphere/wind-mag-fps', 15.0 * 1.68781)  # kts to fps
fdm.set_property_value('atmosphere/wind-dir-deg', 90.0)

# 초기화 완료
fdm.run_ic()

# 시뮬레이션 데이터 저장용
results = []
dt = 0.1  # 10 Hz 데이터 수집
time = 0.0
max_time = 120.0

print("Python JSBSim 시뮬레이션 시작...")

while time <= max_time:
    # 조종 입력 (시간에 따른 프로그램된 입력)
    if time >= 30.0 and time < 60.0:
        # 크랩 수정
        fdm.set_property_value('fcs/rudder-cmd-norm', 0.2)
    elif time >= 60.0 and time < 90.0:  
        # 사이드슬립 전환
        fdm.set_property_value('fcs/aileron-cmd-norm', -0.15)
        fdm.set_property_value('fcs/rudder-cmd-norm', 0.1)
    elif time >= 90.0:
        # 최종 접근
        fdm.set_property_value('fcs/aileron-cmd-norm', -0.2)
        fdm.set_property_value('fcs/rudder-cmd-norm', 0.15)
        
    # 시뮬레이션 스텝
    fdm.run()
    
    # 데이터 수집
    if time % 1.0 < dt:  # 1초마다 기록
        data_point = {{
            'time': time,
            'altitude_agl': fdm.get_property_value('position/h-agl-ft'),
            'airspeed': fdm.get_property_value('velocities/vc-kts'),
            'sideslip': fdm.get_property_value('aero/beta-deg'),
            'heading': fdm.get_property_value('attitude/psi-deg'),
            'roll': fdm.get_property_value('attitude/phi-deg'),
            'lateral_pos': fdm.get_property_value('position/distance-from-start-lon-mt'),
            'side_force': fdm.get_property_value('forces/fby-lbs'),
            'yaw_moment': fdm.get_property_value('moments/n-aero-lbsft'),
            'roll_moment': fdm.get_property_value('moments/l-aero-lbsft')
        }}
        results.append(data_point)
    
    time += dt

# 결과 저장
with open('cessna_crosswind_results.csv', 'w', newline='') as csvfile:
    if results:
        writer = csv.DictWriter(csvfile, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

print(f"시뮬레이션 완료: {{len(results)}}개 데이터 포인트")
print("결과 파일: cessna_crosswind_results.csv")
"""
        
        # Python 스크립트 임시 파일로 저장 후 실행
        with open("temp_jsbsim_simulation.py", "w") as f:
            f.write(python_script)
        
        try:
            result = subprocess.run(["python3", "temp_jsbsim_simulation.py"], 
                                  capture_output=True, text=True, timeout=300)
            
            if result.returncode == 0:
                logger.info("Python JSBSim 시뮬레이션 성공")
                return True
            else:
                logger.error(f"Python JSBSim 오류: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"Python JSBSim 실행 오류: {e}")
            return False
        finally:
            # 임시 파일 정리
            if os.path.exists("temp_jsbsim_simulation.py"):
                os.remove("temp_jsbsim_simulation.py")
    
    def analyze_crosswind_results(self, results_file: str = "cessna_crosswind_results.csv") -> Dict:
        """횡풍 시뮬레이션 결과 분석"""
        
        try:
            df = pd.read_csv(results_file)
            
            # 주요 성능 지표 계산
            max_sideslip = df['sideslip'].abs().max()
            max_roll_angle = df['roll'].abs().max() 
            final_lateral_deviation = df['lateral_pos'].iloc[-1]
            
            # 착륙 성능 평가
            landing_criteria = {
                'max_sideslip_deg': max_sideslip,
                'max_roll_angle_deg': max_roll_angle,
                'lateral_deviation_m': final_lateral_deviation,
                'sideslip_within_limits': max_sideslip <= 10.0,  # 일반적인 한계
                'roll_within_limits': max_roll_angle <= 15.0,
                'lateral_within_limits': abs(final_lateral_deviation) <= 50.0  # 50m 허용
            }
            
            # 전체 성공 여부
            overall_success = all([
                landing_criteria['sideslip_within_limits'],
                landing_criteria['roll_within_limits'], 
                landing_criteria['lateral_within_limits']
            ])
            
            analysis_results = {
                'simulation_success': True,
                'landing_performance': landing_criteria,
                'overall_success': overall_success,
                'data_points': len(df),
                'simulation_file': results_file
            }
            
            logger.info(f"시뮬레이션 결과 분석 완료: {'성공' if overall_success else '한계 초과'}")
            return analysis_results
            
        except FileNotFoundError:
            logger.warning(f"결과 파일을 찾을 수 없음: {results_file}")
            return {'simulation_success': False, 'error': 'Results file not found'}
    
    def create_validation_plots(self, results_file: str = "cessna_crosswind_results.csv"):
        """시뮬레이션 결과 시각화"""
        
        try:
            df = pd.read_csv(results_file)
            
            fig, axes = plt.subplots(2, 3, figsize=(18, 12))
            fig.suptitle('세스나 172 JSBSim 횡풍 착륙 시뮬레이션 결과', fontsize=16)
            
            # 1. 고도 vs 시간
            axes[0,0].plot(df['time'], df['altitude_agl'])
            axes[0,0].set_xlabel('Time (s)')
            axes[0,0].set_ylabel('Altitude AGL (ft)')
            axes[0,0].set_title('Approach Profile')
            axes[0,0].grid(True)
            
            # 2. 측미끄러짐각 vs 시간  
            axes[0,1].plot(df['time'], df['sideslip'], 'r-')
            axes[0,1].set_xlabel('Time (s)')
            axes[0,1].set_ylabel('Sideslip Angle (deg)')
            axes[0,1].set_title('Sideslip Angle History')
            axes[0,1].axhline(y=10, color='r', linestyle='--', alpha=0.5, label='Limit')
            axes[0,1].axhline(y=-10, color='r', linestyle='--', alpha=0.5)
            axes[0,1].legend()
            axes[0,1].grid(True)
            
            # 3. 롤각 vs 시간
            axes[0,2].plot(df['time'], df['roll'], 'g-')
            axes[0,2].set_xlabel('Time (s)')
            axes[0,2].set_ylabel('Roll Angle (deg)')
            axes[0,2].set_title('Roll Angle History')
            axes[0,2].axhline(y=15, color='r', linestyle='--', alpha=0.5, label='Limit')
            axes[0,2].axhline(y=-15, color='r', linestyle='--', alpha=0.5)
            axes[0,2].legend()
            axes[0,2].grid(True)
            
            # 4. 측방 위치 vs 시간
            axes[1,0].plot(df['time'], df['lateral_pos'], 'b-')
            axes[1,0].set_xlabel('Time (s)')
            axes[1,0].set_ylabel('Lateral Position (m)')
            axes[1,0].set_title('Lateral Displacement')
            axes[1,0].grid(True)
            
            # 5. 측력 vs 시간
            axes[1,1].plot(df['time'], df['side_force'], 'm-')
            axes[1,1].set_xlabel('Time (s)')
            axes[1,1].set_ylabel('Side Force (lbs)')
            axes[1,1].set_title('Aerodynamic Side Force')
            axes[1,1].grid(True)
            
            # 6. 요모멘트 vs 시간
            axes[1,2].plot(df['time'], df['yaw_moment'], 'c-')
            axes[1,2].set_xlabel('Time (s)')
            axes[1,2].set_ylabel('Yaw Moment (ft-lbs)')
            axes[1,2].set_title('Yaw Moment')
            axes[1,2].grid(True)
            
            plt.tight_layout()
            plt.savefig('cessna_jsbsim_crosswind_results.png', dpi=300, bbox_inches='tight')
            plt.close()
            
            logger.info("JSBSim 결과 시각화 완료")
            
        except Exception as e:
            logger.error(f"시각화 오류: {e}")

def main():
    """메인 실행 함수"""
    
    print("🛩️ 세스나 172 JSBSim 횡풍 시뮬레이션")
    print("=" * 50)
    
    # 시뮬레이터 초기화
    simulator = CessnaJSBSimSimulator()
    
    # 시뮬레이션 매개변수 설정
    params = SimulationParameters(
        wind_speed=15.0,      # 15 노트 횡풍
        wind_direction=90.0,   # 순수 측풍
        simulation_time=120.0  # 2분 시뮬레이션
    )
    
    # JSBSim 스크립트 생성
    script_path = simulator.create_crosswind_script(params)
    print(f"✅ JSBSim 스크립트 생성: {script_path}")
    
    # 시뮬레이션 실행
    success = simulator.run_simulation(script_path)
    
    if success:
        # 결과 분석
        analysis = simulator.analyze_crosswind_results()
        
        print(f"\n📊 시뮬레이션 결과:")
        if analysis['simulation_success']:
            perf = analysis['landing_performance']
            print(f"   • 최대 측미끄러짐각: {perf['max_sideslip_deg']:.2f}°")
            print(f"   • 최대 롤각: {perf['max_roll_angle_deg']:.2f}°")
            print(f"   • 측방편차: {perf['lateral_deviation_m']:.1f}m")
            print(f"   • 착륙 성공: {'✅' if analysis['overall_success'] else '❌'}")
        
        # 시각화 생성
        simulator.create_validation_plots()
        print(f"   • 결과 그래프: cessna_jsbsim_crosswind_results.png")
        
    else:
        print("⚠️ JSBSim 시뮬레이션이 실행되지 않았습니다.")
        print("   스크립트 파일이 생성되었으므로 수동으로 실행할 수 있습니다:")
        print(f"   JSBSim --script={script_path}")
    
    print(f"\n🎯 생성된 파일:")
    print(f"   • {script_path} (JSBSim 스크립트)")
    if success:
        print(f"   • cessna_crosswind_results.csv (시뮬레이션 데이터)")
        print(f"   • cessna_jsbsim_crosswind_results.png (결과 그래프)")
    
    print(f"\n🎉 세스나 172 JSBSim 횡풍 시뮬레이션 완료!")

if __name__ == "__main__":
    main()