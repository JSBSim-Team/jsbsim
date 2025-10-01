%% UAM 횡풍 착륙 분석 - MATLAB 스크립트
% Urban Air Mobility Crosswind Landing Analysis
% 작성자: UAM Research Team
% 작성일: 2024-10-01

clear all; close all; clc;

%% 1. 기체 파라미터 정의
aircraft = struct();
aircraft.mass = 1134;           % kg
aircraft.Ixx = 700;            % kg⋅m²
aircraft.Iyy = 700;            % kg⋅m²
aircraft.Izz = 1166;           % kg⋅m²
aircraft.wingspan = 6.0;       % m
aircraft.S = 10.0;             % m² (기준면적)
aircraft.c_bar = 1.5;          % m (평균 코드)

%% 2. 공기역학 계수
aero = struct();
aero.Cy_beta = -0.25;         % 사이드슬립에 의한 측력 계수
aero.Cy_delta_r = -0.12;      % 러더에 의한 측력 계수
aero.Cy_p = 0.02;             % 롤레이트에 의한 측력 계수
aero.Cy_r = 0.08;             % 요레이트에 의한 측력 계수
aero.Cl_beta = -0.08;         % 사이드슬립에 의한 롤모멘트 계수
aero.Cl_p = -0.45;            % 롤댐핑 계수
aero.Cl_r = 0.02;             % 요레이트에 의한 롤모멘트 계수
aero.Cl_delta_a = 0.15;       % 에일러론에 의한 롤모멘트 계수
aero.Cl_delta_r = 0.01;       % 러더에 의한 롤모멘트 계수
aero.Cn_beta = 0.12;          % 방향안정성 계수
aero.Cn_r = -0.25;            % 요댐핑 계수
aero.Cn_p = -0.01;            % 롤레이트에 의한 요모멘트 계수
aero.Cn_delta_r = -0.08;      % 러더에 의한 요모멘트 계수
aero.Cn_delta_a = -0.005;     % 에일러론에 의한 요모멘트 계수

%% 3. 환경 파라미터
env = struct();
env.rho = 1.225;              % kg/m³ (공기밀도)
env.g = 9.81;                 % m/s² (중력가속도)

%% 4. 횡풍 조건 설정
wind_speeds = [2, 5, 8, 10, 12, 15];        % m/s
wind_directions = [0, 30, 60, 90, 120, 150, 180]; % degrees

% 결과 저장용 배열 초기화
num_cases = length(wind_speeds) * length(wind_directions);
results = zeros(num_cases, 7); % [wind_speed, wind_dir, max_deviation, final_pos, max_roll, max_yaw, accuracy]
case_idx = 1;

%% 5. 시뮬레이션 루프
fprintf('UAM 횡풍 착륙 분석 시뮬레이션 시작\n');
fprintf('=================================================\n');

for i = 1:length(wind_speeds)
    for j = 1:length(wind_directions)
        wind_speed = wind_speeds(i);
        wind_direction = wind_directions(j);
        
        fprintf('시뮬레이션: 풍속 %.1f m/s, 풍향 %.0f°\n', wind_speed, wind_direction);
        
        % 단일 케이스 시뮬레이션 실행
        [max_dev, final_pos, max_roll, max_yaw, accuracy] = ...
            simulate_single_case(wind_speed, wind_direction, aircraft, aero, env);
        
        % 결과 저장
        results(case_idx, :) = [wind_speed, wind_direction, max_dev, final_pos, max_roll, max_yaw, accuracy];
        case_idx = case_idx + 1;
    end
end

%% 6. 결과 분석
analyze_results(results, wind_speeds, wind_directions);

%% 7. 시각화
create_visualizations(results, wind_speeds, wind_directions);

fprintf('\n분석 완료!\n');

%% 함수 정의들

function [max_deviation, final_position, max_roll, max_yaw, landing_accuracy] = ...
    simulate_single_case(wind_speed, wind_direction, aircraft, aero, env)
    
    % 시뮬레이션 파라미터
    dt = 0.05;                    % 시간 간격 (s)
    t_final = 30.0;               % 시뮬레이션 시간 (s)
    t = 0:dt:t_final;
    n_steps = length(t);
    
    % 초기 조건 [x, y, z, u, v, w, phi, theta, psi, p, q, r]
    state = [0, 0, 100, 20, 0, -2, 0, deg2rad(-3), 0, 0, 0, 0];
    
    % 상태 이력 저장
    state_history = zeros(n_steps, 12);
    state_history(1, :) = state;
    
    % 풍향을 라디안으로 변환
    wind_dir_rad = deg2rad(wind_direction);
    
    % 시뮬레이션 루프
    for k = 2:n_steps
        % 현재 상태
        x = state(1); y = state(2); z = state(3);
        u = state(4); v = state(5); w = state(6);
        phi = state(7); theta = state(8); psi = state(9);
        p = state(10); q = state(11); r = state(12);
        
        % 제어 입력 계산 (간단한 PID)
        Kp_phi = -2.0; Kd_phi = -0.5;
        Kp_psi = -1.0; Kd_psi = -0.3;
        
        delta_a = Kp_phi * phi + Kd_phi * p;
        delta_a = max(-0.35, min(0.35, delta_a)); % 제한
        
        delta_r = Kp_psi * psi + Kd_psi * r;
        delta_r = max(-0.35, min(0.35, delta_r)); % 제한
        
        % 동역학 계산
        state_dot = crosswind_dynamics(state, wind_speed, wind_dir_rad, ...
                                     delta_a, delta_r, aircraft, aero, env);
        
        % 4차 Runge-Kutta 적분
        k1 = dt * state_dot;
        k2 = dt * crosswind_dynamics(state + k1/2, wind_speed, wind_dir_rad, ...
                                   delta_a, delta_r, aircraft, aero, env);
        k3 = dt * crosswind_dynamics(state + k2/2, wind_speed, wind_dir_rad, ...
                                   delta_a, delta_r, aircraft, aero, env);
        k4 = dt * crosswind_dynamics(state + k3, wind_speed, wind_dir_rad, ...
                                   delta_a, delta_r, aircraft, aero, env);
        
        state = state + (k1 + 2*k2 + 2*k3 + k4) / 6;
        state_history(k, :) = state;
        
        % 착륙 조건 확인
        if state(3) <= 0  % 고도가 0 이하로 내려가면 착륙
            state_history = state_history(1:k, :);
            break;
        end
    end
    
    % 결과 분석
    y_positions = state_history(:, 2);
    phi_angles = state_history(:, 7);
    psi_angles = state_history(:, 9);
    x_positions = state_history(:, 1);
    
    max_deviation = max(abs(y_positions));
    final_position = y_positions(end);
    max_roll = max(abs(phi_angles));
    max_yaw = max(abs(psi_angles));
    landing_accuracy = sqrt(x_positions(end)^2 + y_positions(end)^2);
end

function state_dot = crosswind_dynamics(state, wind_speed, wind_direction, ...
                                       delta_a, delta_r, aircraft, aero, env)
    
    % 상태 변수 추출
    u = state(4); v = state(5); w = state(6);
    phi = state(7); theta = state(8); psi = state(9);
    p = state(10); q = state(11); r = state(12);
    
    % 풍속 성분
    wind_u = wind_speed * cos(wind_direction);
    wind_v = wind_speed * sin(wind_direction);
    
    % 상대속도
    u_rel = u - wind_u;
    v_rel = v - wind_v;
    w_rel = w;
    
    V_rel = sqrt(u_rel^2 + v_rel^2 + w_rel^2);
    
    if V_rel < 0.1
        V_rel = 0.1; % 수치적 안정성
    end
    
    % 공기역학각
    alpha = atan2(w_rel, u_rel);
    beta = asin(v_rel / V_rel);
    
    % 동압
    q_bar = 0.5 * env.rho * V_rel^2;
    
    % 무차원 각속도
    p_hat = p * aircraft.wingspan / (2 * V_rel);
    r_hat = r * aircraft.wingspan / (2 * V_rel);
    
    % 공기역학적 힘
    Cy = aero.Cy_beta * beta + aero.Cy_delta_r * delta_r + ...
         aero.Cy_p * p_hat + aero.Cy_r * r_hat;
    Y = q_bar * aircraft.S * Cy;
    
    % 공기역학적 모멘트
    Cl = aero.Cl_beta * beta + aero.Cl_p * p_hat + aero.Cl_r * r_hat + ...
         aero.Cl_delta_a * delta_a + aero.Cl_delta_r * delta_r;
    L_aero = q_bar * aircraft.S * aircraft.wingspan * Cl;
    
    Cn = aero.Cn_beta * beta + aero.Cn_r * r_hat + aero.Cn_p * p_hat + ...
         aero.Cn_delta_r * delta_r + aero.Cn_delta_a * delta_a;
    N_aero = q_bar * aircraft.S * aircraft.wingspan * Cn;
    
    % 추력 (간단한 모델)
    thrust = 8000; % N
    
    % 힘의 균형 (기체 좌표계)
    X_total = thrust * cos(alpha) * cos(beta);
    Y_total = Y + thrust * sin(beta);
    Z_total = thrust * sin(alpha) * cos(beta) + ...
              aircraft.mass * env.g * cos(theta) * cos(phi);
    
    % 가속도 (기체 좌표계)
    u_dot = X_total / aircraft.mass - q*w + r*v;
    v_dot = Y_total / aircraft.mass - r*u + p*w;
    w_dot = Z_total / aircraft.mass - p*v + q*u;
    
    % 위치 변화 (지구 좌표계로 변환)
    cos_phi = cos(phi); sin_phi = sin(phi);
    cos_theta = cos(theta); sin_theta = sin(theta);
    cos_psi = cos(psi); sin_psi = sin(psi);
    
    % 방향 코사인 행렬
    DCM = [cos_theta*cos_psi, sin_phi*sin_theta*cos_psi - cos_phi*sin_psi, ...
           cos_phi*sin_theta*cos_psi + sin_phi*sin_psi;
           cos_theta*sin_psi, sin_phi*sin_theta*sin_psi + cos_phi*cos_psi, ...
           cos_phi*sin_theta*sin_psi - sin_phi*cos_psi;
           -sin_theta, sin_phi*cos_theta, cos_phi*cos_theta];
    
    position_dot = DCM * [u; v; w];
    x_dot = position_dot(1);
    y_dot = position_dot(2);
    z_dot = position_dot(3);
    
    % 각속도 변화
    p_dot = (L_aero + (aircraft.Iyy - aircraft.Izz) * q * r) / aircraft.Ixx;
    q_dot = 0; % 피치 모멘트는 간단히 처리
    r_dot = (N_aero + (aircraft.Ixx - aircraft.Iyy) * p * q) / aircraft.Izz;
    
    % 오일러각 변화
    phi_dot = p + q * sin_phi * tan(theta) + r * cos_phi * tan(theta);
    theta_dot = q * cos_phi - r * sin_phi;
    if abs(cos(theta)) > 0.1
        psi_dot = (q * sin_phi + r * cos_phi) / cos_theta;
    else
        psi_dot = 0;
    end
    
    state_dot = [x_dot, y_dot, z_dot, u_dot, v_dot, w_dot, ...
                phi_dot, theta_dot, psi_dot, p_dot, q_dot, r_dot];
end

function analyze_results(results, wind_speeds, wind_directions)
    
    fprintf('\n=== 시뮬레이션 결과 분석 ===\n');
    fprintf('총 케이스 수: %d\n', size(results, 1));
    fprintf('최대 측방편차: %.2f m\n', max(results(:, 3)));
    fprintf('평균 측방편차: %.2f m\n', mean(results(:, 3)));
    fprintf('표준편차: %.2f m\n', std(results(:, 3)));
    
    % 가장 위험한 조건 찾기
    [max_dev, max_idx] = max(results(:, 3));
    fprintf('\n가장 위험한 조건:\n');
    fprintf('  풍속: %.1f m/s\n', results(max_idx, 1));
    fprintf('  풍향: %.0f°\n', results(max_idx, 2));
    fprintf('  측방편차: %.2f m\n', max_dev);
    
    % 풍향별 분석
    fprintf('\n풍향별 영향 분석:\n');
    for dir = wind_directions
        dir_results = results(results(:, 2) == dir, :);
        if ~isempty(dir_results)
            fprintf('풍향 %3.0f°: 평균 %.2f m, 최대 %.2f m\n', ...
                   dir, mean(dir_results(:, 3)), max(dir_results(:, 3)));
        end
    end
    
    % 안전성 분석 (착륙장 폭 30m 기준)
    runway_width = 30;
    safe_cases = sum(results(:, 3) <= runway_width/2);
    safety_rate = safe_cases / size(results, 1) * 100;
    
    fprintf('\n안전성 분석 (착륙장 폭 %.0fm 기준):\n', runway_width);
    fprintf('안전 케이스: %d개 (%.1f%%)\n', safe_cases, safety_rate);
    fprintf('위험 케이스: %d개 (%.1f%%)\n', size(results, 1) - safe_cases, 100 - safety_rate);
end

function create_visualizations(results, wind_speeds, wind_directions)
    
    % 1. 풍속 vs 측방편차
    figure('Position', [100, 100, 1200, 800]);
    
    subplot(2, 2, 1);
    hold on;
    colors = lines(length(wind_directions));
    
    for i = 1:length(wind_directions)
        dir = wind_directions(i);
        dir_results = results(results(:, 2) == dir, :);
        if ~isempty(dir_results)
            plot(dir_results(:, 1), dir_results(:, 3), 'o-', ...
                'Color', colors(i, :), 'LineWidth', 2, 'MarkerSize', 8, ...
                'DisplayName', sprintf('풍향 %.0f°', dir));
        end
    end
    
    xlabel('풍속 [m/s]', 'FontSize', 12);
    ylabel('최대 측방편차 [m]', 'FontSize', 12);
    title('풍속별 측방편차 분석', 'FontSize', 14, 'FontWeight', 'bold');
    legend('Location', 'northwest', 'FontSize', 10);
    grid on;
    
    % 2. 풍향별 편차 분포
    subplot(2, 2, 2);
    wind_dir_means = zeros(size(wind_directions));
    wind_dir_stds = zeros(size(wind_directions));
    
    for i = 1:length(wind_directions)
        dir = wind_directions(i);
        dir_results = results(results(:, 2) == dir, :);
        if ~isempty(dir_results)
            wind_dir_means(i) = mean(dir_results(:, 3));
            wind_dir_stds(i) = std(dir_results(:, 3));
        end
    end
    
    errorbar(wind_directions, wind_dir_means, wind_dir_stds, 'bo-', ...
            'LineWidth', 2, 'MarkerSize', 8, 'MarkerFaceColor', 'b');
    xlabel('풍향 [도]', 'FontSize', 12);
    ylabel('평균 측방편차 [m]', 'FontSize', 12);
    title('풍향별 측방편차 분포', 'FontSize', 14, 'FontWeight', 'bold');
    grid on;
    
    % 3. 히트맵
    subplot(2, 2, 3);
    deviation_matrix = zeros(length(wind_directions), length(wind_speeds));
    
    for i = 1:length(wind_directions)
        for j = 1:length(wind_speeds)
            mask = (results(:, 2) == wind_directions(i)) & ...
                   (results(:, 1) == wind_speeds(j));
            if any(mask)
                deviation_matrix(i, j) = results(mask, 3);
            end
        end
    end
    
    imagesc(wind_speeds, wind_directions, deviation_matrix);
    colorbar;
    colormap(hot);
    xlabel('풍속 [m/s]', 'FontSize', 12);
    ylabel('풍향 [도]', 'FontSize', 12);
    title('횡풍 조건별 측방편차 히트맵', 'FontSize', 14, 'FontWeight', 'bold');
    
    % 4. 안전성 평가
    subplot(2, 2, 4);
    runway_width = 30;
    safe_mask = results(:, 3) <= runway_width/2;
    
    scatter(results(safe_mask, 1), results(safe_mask, 2), 100, 'g', 'filled', ...
           'DisplayName', '안전 (편차 ≤ 15m)');
    hold on;
    scatter(results(~safe_mask, 1), results(~safe_mask, 2), 100, 'r', 'filled', ...
           'DisplayName', '위험 (편차 > 15m)');
    
    xlabel('풍속 [m/s]', 'FontSize', 12);
    ylabel('풍향 [도]', 'FontSize', 12);
    title('안전성 평가', 'FontSize', 14, 'FontWeight', 'bold');
    legend('FontSize', 10);
    grid on;
    
    sgtitle('UAM 횡풍 착륙 분석 결과', 'FontSize', 16, 'FontWeight', 'bold');
    
    % 결과를 CSV 파일로 저장
    headers = {'Wind_Speed_ms', 'Wind_Direction_deg', 'Max_Deviation_m', ...
              'Final_Position_m', 'Max_Roll_deg', 'Max_Yaw_deg', 'Landing_Accuracy_m'};
    
    % 각도를 도 단위로 변환
    results_deg = results;
    results_deg(:, 5) = rad2deg(results(:, 5)); % Roll angle
    results_deg(:, 6) = rad2deg(results(:, 6)); % Yaw angle
    
    filename = 'UAM_crosswind_analysis_results.csv';
    
    % CSV 파일 작성
    fid = fopen(filename, 'w');
    fprintf(fid, '%s,', headers{1:end-1});
    fprintf(fid, '%s\n', headers{end});
    
    for i = 1:size(results_deg, 1)
        fprintf(fid, '%.2f,%.1f,%.2f,%.2f,%.2f,%.2f,%.2f\n', results_deg(i, :));
    end
    fclose(fid);
    
    fprintf('\n결과가 %s 파일로 저장되었습니다.\n', filename);
end