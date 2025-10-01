#!/usr/bin/env python3
"""
UAM CFD 결과 후처리 도구
"""

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import sys
import os
from pathlib import Path

def read_force_coefficients(coeff_file):
    """힘 계수 데이터 읽기"""
    
    try:
        # OpenFOAM forceCoeffs 출력 파일 읽기
        data = pd.read_csv(coeff_file, delimiter='\t', comment='#', 
                          names=['Time', 'Cd', 'Cs', 'Cl', 'CmRoll', 'CmPitch', 'CmYaw',
                                'Cd_p', 'Cs_p', 'Cl_p', 'Cd_v', 'Cs_v', 'Cl_v',
                                'CmRoll_p', 'CmPitch_p', 'CmYaw_p',
                                'CmRoll_v', 'CmPitch_v', 'CmYaw_v'])
        
        return data
    except Exception as e:
        print(f"계수 파일 읽기 오류: {e}")
        return None

def analyze_convergence(data):
    """수렴성 분석"""
    
    if data is None or len(data) == 0:
        return None
    
    # 후반부 데이터로 수렴값 계산 (마지막 20%)
    n_total = len(data)
    n_converged = max(int(0.2 * n_total), 10)
    
    converged_data = data.tail(n_converged)
    
    results = {
        'Cd_converged': converged_data['Cd'].mean(),
        'Cs_converged': converged_data['Cs'].mean(), 
        'Cl_converged': converged_data['Cl'].mean(),
        'CmYaw_converged': converged_data['CmYaw'].mean(),
        'Cd_std': converged_data['Cd'].std(),
        'Cs_std': converged_data['Cs'].std(),
        'Cl_std': converged_data['Cl'].std(),
        'CmYaw_std': converged_data['CmYaw'].std()
    }
    
    return results

def create_visualizations(data, sideslip_angle, output_dir):
    """결과 시각화"""
    
    if data is None:
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(15, 10))
    
    # 1. 항력계수 수렴 히스토리
    axes[0,0].plot(data['Time'], data['Cd'], 'b-', linewidth=2)
    axes[0,0].set_xlabel('Iteration')
    axes[0,0].set_ylabel('Drag Coefficient (Cd)')
    axes[0,0].set_title(f'Drag Convergence (β={sideslip_angle}°)')
    axes[0,0].grid(True, alpha=0.3)
    
    # 2. 측력계수 수렴 히스토리
    axes[0,1].plot(data['Time'], data['Cs'], 'r-', linewidth=2)
    axes[0,1].set_xlabel('Iteration')
    axes[0,1].set_ylabel('Side Force Coefficient (Cs)')
    axes[0,1].set_title(f'Side Force Convergence (β={sideslip_angle}°)')
    axes[0,1].grid(True, alpha=0.3)
    
    # 3. 양력계수 수렴 히스토리
    axes[1,0].plot(data['Time'], data['Cl'], 'g-', linewidth=2)
    axes[1,0].set_xlabel('Iteration')
    axes[1,0].set_ylabel('Lift Coefficient (Cl)')
    axes[1,0].set_title(f'Lift Convergence (β={sideslip_angle}°)')
    axes[1,0].grid(True, alpha=0.3)
    
    # 4. 요모멘트계수 수렴 히스토리
    axes[1,1].plot(data['Time'], data['CmYaw'], 'm-', linewidth=2)
    axes[1,1].set_xlabel('Iteration')
    axes[1,1].set_ylabel('Yaw Moment Coefficient (Cn)')
    axes[1,1].set_title(f'Yaw Moment Convergence (β={sideslip_angle}°)')
    axes[1,1].grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(f'{output_dir}/convergence_beta_{sideslip_angle}deg.png', 
               dpi=300, bbox_inches='tight')
    plt.close()

def main():
    """메인 실행 함수"""
    
    if len(sys.argv) > 1:
        sideslip_angle = float(sys.argv[1])
    else:
        sideslip_angle = 0.0
    
    print(f"🔍 CFD 결과 후처리 (사이드슬립: {sideslip_angle}°)")
    
    # 결과 파일 경로
    coeff_file = 'postProcessing/forces/0/coefficient.dat'
    
    if not os.path.exists(coeff_file):
        print("⚠️ 계수 파일을 찾을 수 없습니다.")
        print("CFD 해석이 완료되지 않았거나 파일 경로를 확인하세요.")
        return
    
    # 데이터 읽기
    data = read_force_coefficients(coeff_file)
    
    if data is None:
        print("❌ 데이터 읽기 실패")
        return
    
    # 수렴성 분석
    convergence = analyze_convergence(data)
    
    if convergence:
        print(f"\n📊 수렴된 계수값:")
        print(f"   항력계수 (Cd):    {convergence['Cd_converged']:.6f} ± {convergence['Cd_std']:.6f}")
        print(f"   측력계수 (Cs):    {convergence['Cs_converged']:.6f} ± {convergence['Cs_std']:.6f}")
        print(f"   양력계수 (Cl):    {convergence['Cl_converged']:.6f} ± {convergence['Cl_std']:.6f}")
        print(f"   요모멘트계수 (Cn): {convergence['CmYaw_converged']:.6f} ± {convergence['CmYaw_std']:.6f}")
    
    # 시각화
    output_dir = 'postProcessing/plots'
    os.makedirs(output_dir, exist_ok=True)
    
    create_visualizations(data, sideslip_angle, output_dir)
    
    # 결과 요약 저장
    if convergence:
        summary = {
            'sideslip_angle': sideslip_angle,
            'Cd': convergence['Cd_converged'],
            'Cs': convergence['Cs_converged'],
            'Cl': convergence['Cl_converged'],
            'Cn': convergence['CmYaw_converged']
        }
        
        summary_df = pd.DataFrame([summary])
        summary_df.to_csv(f'postProcessing/summary_beta_{sideslip_angle}deg.csv', index=False)
    
    print(f"✅ 후처리 완료!")
    print(f"   그래프: {output_dir}/convergence_beta_{sideslip_angle}deg.png")
    print(f"   요약: postProcessing/summary_beta_{sideslip_angle}deg.csv")

if __name__ == "__main__":
    main()
