#!/usr/bin/env python3
import numpy as np
from stl import mesh
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def create_uam_geometry():
    '''UAM 기체 STL 지오메트리 생성'''
    
    # 1. 동체 (Fuselage) - 간단한 직육면체
    fuselage_vertices = np.array([
        # 하단면 (z=0)
        [0, -0.9, 0], [4.57, -0.9, 0], [4.57, 0.9, 0], [0, 0.9, 0],
        # 상단면 (z=1.5) 
        [0, -0.9, 1.5], [4.57, -0.9, 1.5], [4.57, 0.9, 1.5], [0, 0.9, 1.5]
    ])
    
    # 동체 면 정의 (삼각형)
    fuselage_faces = np.array([
        # 하단면
        [0,1,2], [0,2,3],
        # 상단면 
        [4,6,5], [4,7,6],
        # 측면들
        [0,4,5], [0,5,1],  # 전면
        [2,6,7], [2,7,3],  # 후면
        [0,3,7], [0,7,4],  # 좌측면
        [1,5,6], [1,6,2]   # 우측면
    ])
    
    # STL 메시 생성
    fuselage_mesh = mesh.Mesh(np.zeros(fuselage_faces.shape[0], dtype=mesh.Mesh.dtype))
    for i, face in enumerate(fuselage_faces):
        for j in range(3):
            fuselage_mesh.vectors[i][j] = fuselage_vertices[face[j],:]
    
    # 2. 로터 디스크 (간단한 원판형태)
    def create_rotor_disk(center, radius, normal_vector):
        '''로터 디스크 생성'''
        theta = np.linspace(0, 2*np.pi, 16)
        
        # 원판의 점들
        points = []
        center = np.array(center)
        
        for t in theta:
            x = radius * np.cos(t)
            y = radius * np.sin(t)
            point = center + x * np.array([0,1,0]) + y * np.array([0,0,1])
            points.append(point)
        
        # 중심점 추가
        points.append(center)
        points = np.array(points)
        
        # 삼각형 면 생성
        faces = []
        n_points = len(theta)
        for i in range(n_points):
            next_i = (i + 1) % n_points
            faces.append([i, next_i, n_points])  # 중심점과 연결
        
        return points, np.array(faces)
    
    # 로터 디스크들 생성
    rotor_positions = [[1.5, -3.0, 0.5], [1.5, 3.0, 0.5], 
                      [3.0, -3.0, 0.5], [3.0, 3.0, 0.5]]
    
    all_vertices = fuselage_vertices.tolist()
    all_faces = fuselage_faces.tolist()
    
    for pos in rotor_positions:
        rotor_verts, rotor_faces = create_rotor_disk(pos, 1.2, [0,0,1])
        
        # 기존 면 인덱스 오프셋
        offset = len(all_vertices)
        rotor_faces_offset = rotor_faces + offset
        
        all_vertices.extend(rotor_verts.tolist())
        all_faces.extend(rotor_faces_offset.tolist())
    
    # 전체 메시 생성
    all_vertices = np.array(all_vertices)
    all_faces = np.array(all_faces)
    
    complete_mesh = mesh.Mesh(np.zeros(all_faces.shape[0], dtype=mesh.Mesh.dtype))
    for i, face in enumerate(all_faces):
        for j in range(3):
            complete_mesh.vectors[i][j] = all_vertices[face[j],:]
    
    return complete_mesh

if __name__ == "__main__":
    # UAM 지오메트리 생성
    uam_mesh = create_uam_geometry()
    
    # STL 파일로 저장
    uam_mesh.save('constant/triSurface/uam_geometry.stl')
    print("UAM 지오메트리가 uam_geometry.stl로 저장되었습니다.")
    
    # 시각화 (선택사항)
    fig = plt.figure(figsize=(12, 8))
    ax = fig.add_subplot(111, projection='3d')
    
    ax.add_collection3d(plt.art3d.Poly3DCollection(uam_mesh.vectors, alpha=0.7))
    
    # 축 설정
    scale = uam_mesh.points.flatten()
    ax.auto_scale_xyz(scale, scale, scale)
    ax.set_xlabel('X [m]')
    ax.set_ylabel('Y [m]')
    ax.set_zlabel('Z [m]')
    ax.set_title('UAM 기체 지오메트리')
    
    plt.savefig('geometry/uam_geometry_preview.png', dpi=300, bbox_inches='tight')
    plt.close()
    
    print("지오메트리 미리보기가 저장되었습니다.")
