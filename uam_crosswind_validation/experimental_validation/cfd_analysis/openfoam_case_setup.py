#!/usr/bin/env python3
"""
UAM 횡풍 해석을 위한 OpenFOAM CFD 케이스 자동 생성기
OpenFOAM CFD Case Generator for UAM Crosswind Analysis

이 스크립트는 UAM 기체의 횡풍 조건 CFD 해석을 위한
완전한 OpenFOAM 케이스를 자동으로 생성합니다.
"""

import os
import numpy as np
import json
from pathlib import Path
import shutil

class UAM_CFD_CaseGenerator:
    """UAM CFD 케이스 생성 클래스"""
    
    def __init__(self, case_name="uam_crosswind_analysis"):
        """초기화"""
        self.case_name = case_name
        self.case_dir = Path(f"./{case_name}")
        
        # UAM 기체 기하학적 특성
        self.geometry = {
            'fuselage_length': 4.57,      # m
            'fuselage_width': 1.8,        # m  
            'fuselage_height': 1.5,       # m
            'rotor_diameter': 2.4,        # m
            'rotor_positions': [          # 로터 중심 좌표 (x,y,z)
                [1.5, -3.0, 0.5],       # 전방 좌측
                [1.5,  3.0, 0.5],       # 전방 우측  
                [3.0, -3.0, 0.5],       # 후방 좌측
                [3.0,  3.0, 0.5]        # 후방 우측
            ],
            'landing_gear_height': 0.8    # m
        }
        
        # CFD 해석 조건
        self.flow_conditions = {
            'reynolds_numbers': [1e5, 2e5, 5e5],
            'mach_numbers': [0.05, 0.1, 0.15], 
            'sideslip_angles': np.arange(-25, 26, 5),  # degrees
            'air_density': 1.225,         # kg/m³
            'air_viscosity': 1.81e-5,     # kg/m⋅s
            'reference_velocity': 20.0,   # m/s
            'reference_area': 10.0        # m²
        }
        
        # 격자 설정
        self.mesh_settings = {
            'domain_size': {
                'x': [-50, 100],    # m (기체 전후)
                'y': [-50, 50],     # m (기체 좌우)  
                'z': [-20, 30]      # m (기체 상하)
            },
            'base_cell_size': 0.5,       # m
            'boundary_layers': 10,
            'y_plus_target': 1.0,
            'refinement_levels': {
                'fuselage': 4,
                'rotors': 5, 
                'wake': 3
            }
        }
        
    def create_directory_structure(self):
        """OpenFOAM 케이스 디렉토리 구조 생성"""
        
        print(f"🗂️ OpenFOAM 케이스 디렉토리 생성: {self.case_dir}")
        
        # 기본 디렉토리 구조
        directories = [
            'system',
            'constant',
            'constant/polyMesh',
            'constant/triSurface', 
            '0',
            'postProcessing',
            'scripts',
            'geometry'
        ]
        
        for directory in directories:
            (self.case_dir / directory).mkdir(parents=True, exist_ok=True)
            
    def generate_stl_geometry(self):
        """UAM 기체 STL 지오메트리 생성 (간단한 형태)"""
        
        print("🛩️ UAM 기체 지오메트리 생성...")
        
        # STL 생성을 위한 Python 스크립트
        stl_script = f"""#!/usr/bin/env python3
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
"""
        
        # STL 생성 스크립트 저장
        with open(self.case_dir / 'scripts' / 'generate_geometry.py', 'w') as f:
            f.write(stl_script)
            
        print("✅ 지오메트리 생성 스크립트 작성 완료")
        
    def create_meshing_files(self):
        """격자 생성 파일들 작성"""
        
        print("🔷 격자 생성 파일 작성...")
        
        # 1. blockMeshDict (배경 격자)
        block_mesh_dict = f"""/*--------------------------------*- C++ -*----------------------------------*\\
| =========                 |                                                 |
| \\\\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox           |
|  \\\\    /   O peration     | Version:  v2312                                 |
|   \\\\  /    A nd           | Website:  www.openfoam.com                      |
|    \\\\/     M anipulation  |                                                 |
\\*---------------------------------------------------------------------------*/
FoamFile
{{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      blockMeshDict;
}}

// UAM 횡풍 해석을 위한 배경 격자 생성

scale   1;

vertices
(
    // 계산 영역 경계점들
    ({self.mesh_settings['domain_size']['x'][0]} {self.mesh_settings['domain_size']['y'][0]} {self.mesh_settings['domain_size']['z'][0]})  // 0
    ({self.mesh_settings['domain_size']['x'][1]} {self.mesh_settings['domain_size']['y'][0]} {self.mesh_settings['domain_size']['z'][0]})  // 1
    ({self.mesh_settings['domain_size']['x'][1]} {self.mesh_settings['domain_size']['y'][1]} {self.mesh_settings['domain_size']['z'][0]})  // 2
    ({self.mesh_settings['domain_size']['x'][0]} {self.mesh_settings['domain_size']['y'][1]} {self.mesh_settings['domain_size']['z'][0]})  // 3
    ({self.mesh_settings['domain_size']['x'][0]} {self.mesh_settings['domain_size']['y'][0]} {self.mesh_settings['domain_size']['z'][1]})  // 4
    ({self.mesh_settings['domain_size']['x'][1]} {self.mesh_settings['domain_size']['y'][0]} {self.mesh_settings['domain_size']['z'][1]})  // 5
    ({self.mesh_settings['domain_size']['x'][1]} {self.mesh_settings['domain_size']['y'][1]} {self.mesh_settings['domain_size']['z'][1]})  // 6
    ({self.mesh_settings['domain_size']['x'][0]} {self.mesh_settings['domain_size']['y'][1]} {self.mesh_settings['domain_size']['z'][1]})  // 7
);

blocks
(
    hex (0 1 2 3 4 5 6 7) (100 60 40) simpleGrading (1 1 1)
);

edges
(
);

boundary
(
    inlet
    {{
        type patch;
        faces
        (
            (0 4 7 3)
        );
    }}
    
    outlet
    {{
        type patch;
        faces
        (
            (2 6 5 1)
        );
    }}
    
    sides
    {{
        type symmetryPlane;
        faces
        (
            (1 5 4 0)
            (3 7 6 2)
        );
    }}
    
    top
    {{
        type symmetryPlane;
        faces
        (
            (4 5 6 7)
        );
    }}
    
    bottom
    {{
        type wall;
        faces
        (
            (0 3 2 1)
        );
    }}
);

mergePatchPairs
(
);
"""

        # 2. snappyHexMeshDict (물체 주위 격자 세밀화)
        snappy_dict = f"""/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      snappyHexMeshDict;
}}

// UAM 기체 주위 격자 세밀화 설정

castellatedMesh true;
snap            true;
addLayers       true;

geometry
{{
    uam_geometry.stl
    {{
        type triSurfaceMesh;
        name uam;
        
        regions
        {{
            fuselage
            {{
                name fuselage;
            }}
            rotors
            {{
                name rotors;
            }}
        }}
    }}
    
    // 세밀화 박스들
    refinementBox_near
    {{
        type searchableBox;
        min (-5 -10 -2);
        max (15  10  8);
    }}
    
    refinementBox_wake
    {{
        type searchableBox;
        min (5 -15 -2);
        max (50  15  8);
    }}
}};

castellatedMeshControls
{{
    maxLocalCells 10000000;
    maxGlobalCells 20000000;
    minRefinementCells 10;
    maxLoadUnbalance 0.10;
    nCellsBetweenLevels 3;
    
    features
    (
        {{
            file "uam_geometry.eMesh";
            level 4;
        }}
    );
    
    refinementSurfaces
    {{
        uam
        {{
            level (3 4);
            
            regions
            {{
                fuselage
                {{
                    level (3 4);
                }}
                rotors
                {{
                    level (4 5);
                }}
            }}
        }}
    }}
    
    refinementRegions
    {{
        refinementBox_near
        {{
            mode inside;
            levels ((1E15 3));
        }}
        
        refinementBox_wake
        {{
            mode inside;
            levels ((1E15 2));
        }}
    }}
    
    locationInMesh (25 0 5);
    
    allowFreeStandingZoneFaces true;
}}

snapControls
{{
    nSmoothPatch 3;
    tolerance 2.0;
    nSolveIter 30;
    nRelaxIter 5;
    
    nFeatureSnapIter 10;
    implicitFeatureSnap false;
    explicitFeatureSnap true;
    multiRegionFeatureSnap false;
}}

addLayersControls
{{
    layers
    {{
        uam
        {{
            nSurfaceLayers {self.mesh_settings['boundary_layers']};
        }}
    }}
    
    relativeSizes true;
    
    expansionRatio 1.3;
    finalLayerThickness 0.3;
    minThickness 0.1;
    
    nGrow 0;
    featureAngle 60;
    maxFaceThicknessRatio 0.5;
    maxThicknessToMedialRatio 0.3;
    minMedianAxisAngle 90;
    nBufferCellsNoExtrude 0;
    nLayerIter 50;
}}

meshQualityControls
{{
    maxNonOrtho 65;
    maxBoundarySkewness 20;
    maxInternalSkewness 4;
    maxConcave 80;
    minFlatness 0.5;
    minVol 1e-13;
    minTetQuality 1e-9;
    minArea -1;
    minTwist 0.02;
    minDeterminant 0.001;
    minFaceWeight 0.02;
    minVolRatio 0.01;
    minTriangleTwist -1;
    
    nSmoothScale 4;
    errorReduction 0.75;
}}

writeFlags
(
    scalarLevels
    layerSets
    layerFields
);

mergeTolerance 1e-6;
"""

        # 파일 저장
        with open(self.case_dir / 'system' / 'blockMeshDict', 'w') as f:
            f.write(block_mesh_dict)
            
        with open(self.case_dir / 'system' / 'snappyHexMeshDict', 'w') as f:
            f.write(snappy_dict)
            
        print("✅ 격자 생성 파일 작성 완료")
        
    def create_solver_settings(self):
        """해석 조건 및 솔버 설정 파일 생성"""
        
        print("⚙️ 솔버 설정 파일 작성...")
        
        # 1. controlDict (해석 제어)
        control_dict = """/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      controlDict;
}

// UAM 횡풍 해석 제어 설정

application     simpleFoam;

startFrom       startTime;
startTime       0;
stopAt          endTime;
endTime         2000;

deltaT          1;

writeControl    timeStep;
writeInterval   100;

purgeWrite      2;
writeFormat     ascii;
writePrecision  6;
writeCompression off;

timeFormat      general;
timePrecision   6;

runTimeModifiable true;

// 후처리 함수들
functions
{
    forces
    {
        type            forceCoeffs;
        libs            ("libforces.so");
        
        writeControl    timeStep;
        writeInterval   10;
        
        patches         (uam);
        
        rho             rhoInf;
        rhoInf          1.225;
        
        CofR            (2.285 0 0.75);  // 기체 무게중심
        liftDir         (0 0 1);
        dragDir         (1 0 0);
        sideDir         (0 1 0);
        pitchAxis       (0 1 0);
        rollAxis        (1 0 0);
        yawAxis         (0 0 1);
        
        magUInf         20.0;
        lRef            4.57;    // 기체 길이
        Aref            10.0;    // 기준면적
    }
    
    pressureCoeffs
    {
        type            pressure;
        libs            ("libfieldFunctionObjects.so");
        
        writeControl    writeTime;
        
        fields          (p);
    }
    
    residuals
    {
        type            residuals;
        libs            ("libutilityFunctionObjects.so");
        
        writeControl    timeStep;
        writeInterval   1;
        
        fields          (U p k omega);
    }
}
"""

        # 2. fvSchemes (수치기법)
        fv_schemes = """/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      fvSchemes;
}

// 수치해석 기법 설정

ddtSchemes
{
    default         steadyState;
}

gradSchemes
{
    default         Gauss linear;
    grad(p)         Gauss linear;
    grad(U)         Gauss linear;
}

divSchemes
{
    default         none;
    
    div(phi,U)      bounded Gauss upwind;
    div(phi,k)      bounded Gauss upwind;
    div(phi,omega)  bounded Gauss upwind;
    
    div((nuEff*dev2(T(grad(U))))) Gauss linear;
}

laplacianSchemes
{
    default         Gauss linear corrected;
}

interpolationSchemes
{
    default         linear;
}

snGradSchemes
{
    default         corrected;
}

wallDist
{
    method meshWave;
}
"""

        # 3. fvSolution (솔버 설정)
        fv_solution = """/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      fvSolution;
}

// 선형해석기 및 수렴 설정

solvers
{
    p
    {
        solver          GAMG;
        tolerance       1e-06;
        relTol          0.1;
        smoother        GaussSeidel;
    }

    U
    {
        solver          smoothSolver;
        smoother        GaussSeidel;
        tolerance       1e-05;
        relTol          0.1;
    }

    k
    {
        solver          smoothSolver;
        smoother        GaussSeidel;
        tolerance       1e-05;
        relTol          0.1;
    }

    omega
    {
        solver          smoothSolver;
        smoother        GaussSeidel;
        tolerance       1e-05;
        relTol          0.1;
    }
}

SIMPLE
{
    nNonOrthogonalCorrectors 0;
    consistent      yes;
    
    residualControl
    {
        p               1e-4;
        U               1e-4;
        k               1e-4;
        omega           1e-4;
    }
}

relaxationFactors
{
    fields
    {
        p               0.3;
    }
    equations
    {
        U               0.7;
        k               0.7;
        omega           0.7;
    }
}
"""

        # 파일들 저장
        files_to_write = [
            ('system/controlDict', control_dict),
            ('system/fvSchemes', fv_schemes), 
            ('system/fvSolution', fv_solution)
        ]
        
        for filename, content in files_to_write:
            with open(self.case_dir / filename, 'w') as f:
                f.write(content)
                
        print("✅ 솔버 설정 파일 작성 완료")
        
    def create_boundary_conditions(self, sideslip_angle=0):
        """경계조건 파일 생성"""
        
        print(f"🌊 경계조건 파일 생성 (사이드슬립: {sideslip_angle}°)...")
        
        # 사이드슬립각에 따른 속도 성분 계산
        velocity_x = self.flow_conditions['reference_velocity'] * np.cos(np.radians(sideslip_angle))
        velocity_y = self.flow_conditions['reference_velocity'] * np.sin(np.radians(sideslip_angle))
        
        # U (속도) 경계조건
        u_bc = f"""/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{{
    version     2.0;
    format      ascii;
    class       volVectorField;
    object      U;
}}

// 속도 경계조건 (사이드슬립: {sideslip_angle}°)

dimensions      [0 1 -1 0 0 0 0];

internalField   uniform ({velocity_x:.3f} {velocity_y:.3f} 0);

boundaryField
{{
    inlet
    {{
        type            fixedValue;
        value           uniform ({velocity_x:.3f} {velocity_y:.3f} 0);
    }}
    
    outlet
    {{
        type            zeroGradient;
    }}
    
    sides
    {{
        type            symmetryPlane;
    }}
    
    top
    {{
        type            symmetryPlane;
    }}
    
    bottom
    {{
        type            noSlip;
    }}
    
    uam
    {{
        type            noSlip;
    }}
}}
"""

        # p (압력) 경계조건
        p_bc = """/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{
    version     2.0;
    format      ascii;
    class       volScalarField;
    object      p;
}

// 압력 경계조건

dimensions      [0 2 -2 0 0 0 0];

internalField   uniform 0;

boundaryField
{
    inlet
    {
        type            zeroGradient;
    }
    
    outlet
    {
        type            fixedValue;
        value           uniform 0;
    }
    
    sides
    {
        type            symmetryPlane;
    }
    
    top
    {
        type            symmetryPlane;
    }
    
    bottom
    {
        type            zeroGradient;
    }
    
    uam
    {
        type            zeroGradient;
    }
}
"""

        # k (난류운동에너지) 경계조건
        turbulent_intensity = 0.05  # 5%
        k_value = 1.5 * (turbulent_intensity * self.flow_conditions['reference_velocity'])**2
        
        k_bc = f"""/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{{
    version     2.0;
    format      ascii;
    class       volScalarField;
    object      k;
}}

// 난류운동에너지 경계조건

dimensions      [0 2 -2 0 0 0 0];

internalField   uniform {k_value:.6f};

boundaryField
{{
    inlet
    {{
        type            fixedValue;
        value           uniform {k_value:.6f};
    }}
    
    outlet
    {{
        type            zeroGradient;
    }}
    
    sides
    {{
        type            symmetryPlane;
    }}
    
    top
    {{
        type            symmetryPlane;
    }}
    
    bottom
    {{
        type            kqRWallFunction;
        value           uniform {k_value:.6f};
    }}
    
    uam
    {{
        type            kqRWallFunction;
        value           uniform {k_value:.6f};
    }}
}}
"""

        # omega (비소산율) 경계조건  
        mixing_length = 0.07 * 4.57  # 7% of characteristic length
        omega_value = np.sqrt(k_value) / (0.09**0.25 * mixing_length)
        
        omega_bc = f"""/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{{
    version     2.0;
    format      ascii;
    class       volScalarField;
    object      omega;
}}

// 비소산율 경계조건

dimensions      [0 0 -1 0 0 0 0];

internalField   uniform {omega_value:.3f};

boundaryField
{{
    inlet
    {{
        type            fixedValue;
        value           uniform {omega_value:.3f};
    }}
    
    outlet
    {{
        type            zeroGradient;
    }}
    
    sides
    {{
        type            symmetryPlane;
    }}
    
    top
    {{
        type            symmetryPlane;
    }}
    
    bottom
    {{
        type            omegaWallFunction;
        value           uniform {omega_value:.3f};
    }}
    
    uam
    {{
        type            omegaWallFunction;
        value           uniform {omega_value:.3f};
    }}
}}
"""

        # 물성치 파일
        transport_props = f"""/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      transportProperties;
}}

// 유체 물성치

transportModel  Newtonian;

nu              [{self.flow_conditions['air_viscosity'] / self.flow_conditions['air_density']:.9f}];
"""

        turbulence_props = """/*--------------------------------*- C++ -*----------------------------------*\\
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      turbulenceProperties;
}

// 난류 모델 설정

simulationType  RAS;

RAS
{
    RASModel        kOmegaSST;
    
    turbulence      on;
    
    printCoeffs     on;
}
"""

        # 경계조건 파일들 저장
        bc_files = [
            ('0/U', u_bc),
            ('0/p', p_bc),
            ('0/k', k_bc),
            ('0/omega', omega_bc),
            ('constant/transportProperties', transport_props),
            ('constant/turbulenceProperties', turbulence_props)
        ]
        
        for filename, content in bc_files:
            with open(self.case_dir / filename, 'w') as f:
                f.write(content)
                
        print("✅ 경계조건 파일 작성 완료")
        
    def create_run_scripts(self):
        """실행 스크립트 생성"""
        
        print("🔧 실행 스크립트 생성...")
        
        # 1. 격자 생성 스크립트
        mesh_script = """#!/bin/bash

# UAM CFD 격자 생성 자동화 스크립트

echo "🔷 UAM CFD 격자 생성 시작..."

# 1. STL 지오메트리 생성
echo "Step 1: STL 지오메트리 생성"
cd scripts
python3 generate_geometry.py
cd ..

# 2. 배경 격자 생성
echo "Step 2: 배경 격자 생성"
blockMesh

# 3. STL feature edge 추출
echo "Step 3: Feature edge 추출"
surfaceFeatureExtract

# 4. snappyHexMesh 실행
echo "Step 4: 물체 주위 격자 생성"
snappyHexMesh -overwrite

# 5. 격자 품질 검사
echo "Step 5: 격자 품질 검사"
checkMesh -allGeometry -allTopology

echo "✅ 격자 생성 완료!"
echo "격자 파일들은 constant/polyMesh/ 에 저장되었습니다."
"""

        # 2. 해석 실행 스크립트
        solve_script = """#!/bin/bash

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
"""

        # 3. 매개변수 연구 스크립트
        parametric_script = """#!/bin/bash

# UAM 횡풍 매개변수 연구 자동화 스크립트

echo "📊 UAM 횡풍 매개변수 연구 시작"

# 사이드슬립각 배열
SIDESLIP_ANGLES=(-25 -20 -15 -10 -5 0 5 10 15 20 25)

# 결과 저장 디렉토리
RESULTS_DIR="parametric_results"
mkdir -p ${RESULTS_DIR}

# 각 사이드슬립각에 대해 해석 수행
for angle in "${SIDESLIP_ANGLES[@]}"; do
    echo "🔄 사이드슬립 ${angle}° 해석 시작..."
    
    # 케이스 디렉토리 생성
    CASE_DIR="${RESULTS_DIR}/sideslip_${angle}deg"
    cp -r . ${CASE_DIR}
    cd ${CASE_DIR}
    
    # 해석 실행
    ./run_cfd.sh ${angle}
    
    # 주요 결과 복사
    cp postProcessing/forces/0/coefficient.dat ../coeffs_${angle}deg.dat
    cp log.simpleFoam ../log_${angle}deg.txt
    
    cd ../..
    
    echo "✅ 사이드슬립 ${angle}° 해석 완료"
done

echo "🎯 매개변수 연구 완료!"
echo "모든 결과는 ${RESULTS_DIR}/ 에 저장되었습니다."

# 통합 후처리
python3 scripts/analyze_parametric_results.py
"""

        # 스크립트 파일들 저장
        scripts = [
            ('scripts/generate_mesh.sh', mesh_script),
            ('scripts/run_cfd.sh', solve_script),
            ('scripts/parametric_study.sh', parametric_script)
        ]
        
        for filename, content in scripts:
            script_path = self.case_dir / filename
            with open(script_path, 'w') as f:
                f.write(content)
            # 실행 권한 부여
            script_path.chmod(0o755)
            
        print("✅ 실행 스크립트 생성 완료")
        
    def create_postprocessing_tools(self):
        """후처리 도구 생성"""
        
        print("📈 후처리 도구 생성...")
        
        postprocess_script = '''#!/usr/bin/env python3
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
        data = pd.read_csv(coeff_file, delimiter='\\t', comment='#', 
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
        print(f"\\n📊 수렴된 계수값:")
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
'''

        # 후처리 스크립트 저장
        with open(self.case_dir / 'scripts' / 'postprocess_results.py', 'w') as f:
            f.write(postprocess_script)
            
        print("✅ 후처리 도구 생성 완료")
        
    def generate_complete_case(self):
        """완전한 CFD 케이스 생성"""
        
        print("🚀 완전한 OpenFOAM CFD 케이스 생성 시작...")
        print("=" * 60)
        
        # 순차적으로 모든 파일 생성
        self.create_directory_structure()
        self.generate_stl_geometry()
        self.create_meshing_files()
        self.create_solver_settings()
        self.create_boundary_conditions(sideslip_angle=0)  # 기본 조건
        self.create_run_scripts()
        self.create_postprocessing_tools()
        
        # 사용 가이드 생성
        self.create_usage_guide()
        
        print("=" * 60)
        print("✅ OpenFOAM CFD 케이스 생성 완료!")
        print(f"📁 케이스 디렉토리: {self.case_dir}")
        print("\\n🚀 실행 방법:")
        print(f"   1. cd {self.case_dir}")
        print("   2. ./scripts/generate_mesh.sh")
        print("   3. ./scripts/run_cfd.sh [사이드슬립각]")
        print("   4. 또는 매개변수 연구: ./scripts/parametric_study.sh")
        
    def create_usage_guide(self):
        """사용 가이드 생성"""
        
        guide = f"""# UAM 횡풍 CFD 해석 사용 가이드

## 개요
이 OpenFOAM 케이스는 UAM 기체의 횡풍 조건에서의 공기역학적 특성을 해석합니다.

## 시스템 요구사항
- OpenFOAM v2312 이상
- Python 3.8 이상 (numpy, pandas, matplotlib)
- 최소 8GB RAM, 4코어 CPU 권장
- 디스크 공간: 케이스당 약 2-5GB

## 케이스 구조
```
{self.case_name}/
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
- 전장: {self.geometry['fuselage_length']}m
- 전폭: {self.geometry['fuselage_width']}m  
- 로터 직경: {self.geometry['rotor_diameter']}m
- 로터 4개 (쿼드콥터 형태)

### 유동 조건
- 기준 속도: {self.flow_conditions['reference_velocity']} m/s
- 공기 밀도: {self.flow_conditions['air_density']} kg/m³
- 동점성계수: {self.flow_conditions['air_viscosity']} kg/m⋅s
- 난류 모델: k-ω SST

### 격자 정보
- 기본 셀 크기: {self.mesh_settings['base_cell_size']}m
- 경계층 레이어: {self.mesh_settings['boundary_layers']}층
- y+ 목표값: {self.mesh_settings['y_plus_target']}

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
"""

        with open(self.case_dir / 'README.md', 'w') as f:
            f.write(guide)
            
        print("✅ 사용 가이드 생성 완료")

def main():
    """메인 실행 함수"""
    
    print("🌪️ UAM 횡풍 CFD 해석 케이스 생성기")
    print("=" * 60)
    
    # CFD 케이스 생성기 초기화
    cfd_generator = UAM_CFD_CaseGenerator("uam_crosswind_cfd")
    
    # 완전한 케이스 생성
    cfd_generator.generate_complete_case()

if __name__ == "__main__":
    main()