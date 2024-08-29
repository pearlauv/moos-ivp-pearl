%/************************************************************/%
%/*    NAME: Blake Cole                                      */%
%/*    ORGN: MIT                                             */%
%/*    FILE: m300_thrust.m                                   */%
%/*    DATE: 22 SEP 2018                                     */%
%/************************************************************/%
clear all; clc;

%% USER PARAMETERS:
max_rud = 30;
max_thr = 100;
MODE = 3;       % MODE1=physics, MODE2=normal, MODE3=aggro

%% VARIABLES
alpha   = [-max_rud:1:max_rud];     % requested rudder [deg]
thrust  = [5:5:max_thr];          % requested thrust [%]

%% PHYSICS-BASED MODEL:

% Vehicle Parameters (CLEARPATH HERON):
L_sep = 800e-3;         % motor separation [m]
L_rud = 600e-3;         % length CG to virtual rudder [m]
rho   = 1000;           % water density [kg/m^3]
maxThrust = 40;         % thrust per jet [N]
maxSpeed  = 1.7;        % max speed [m/s] (@maxThrust=drag)

% Calculate max yaw moment [Nm]
maxMoment = (L_sep/2)*(maxThrust/2);

% Approximate coefficient of lift []
Cl = (1/15)*alpha;

% Calculate virtual rudder area [m^2]
A_rud = maxMoment/(0.5*L_rud*rho*(maxSpeed^2)*max(Cl));

% Differential Thrust:
speed    = [0.1:0.1:maxSpeed];
[U,CL,T] = meshgrid(speed, Cl, (thrust/100)*(maxThrust));
%[CL,T] = meshgrid(Cl, (thrust/100)*(maxThrust));
if (MODE==1)
    delta    = (L_rud/L_sep)*rho*A_rud*U.^2.*CL;
    %delta    = (L_rud/L_sep)*rho*A_rud.*CL;
    thrustL  = 0.5*(T + delta);    % left thruster [N]
    thrustR  = 0.5*(T - delta);    % right thruster [N]
    thrustL_PCT = 100*thrustL./(0.5*T); % [%]
    thrustR_PCT = 100*thrustR./(0.5*T); % [%]
end

%% NORMAL MODE:
if (MODE==2)
    [T,R] = meshgrid(thrust, alpha);
    delta   = (T/max_rud).*R;
    thrustL = T + delta;
    thrustR = T - delta;
    
    % Clip saturated values
    upper_lim = 100;
    lower_lim = 0;
    thrustL(thrustL>upper_lim) = upper_lim;
    thrustL(thrustL<lower_lim) = lower_lim;
    thrustR(thrustR>upper_lim) = upper_lim;
    thrustR(thrustR<lower_lim) = lower_lim;
end

%% AGGRO MODE:
if (MODE==3)
    [T,R] = meshgrid(thrust, alpha);
    max_revthr = -0.2*max_thr;
    max_delta  = max_thr - max_revthr;
    delta = (max_delta/max_rud)*R;
    thrustL = T + delta/2;
    thrustR = T - delta/2;
    
    % Rebalance saturated values:
    overflowL = thrustL-max_thr;
    overflowL(overflowL<0) = 0;
    overflowR = thrustR-max_thr;
    overflowR(overflowR<0) = 0;
    thrustL = thrustL-overflowL-overflowR;
    thrustR = thrustR-overflowR-overflowL;
    
    underflowL = max_revthr - thrustL;
    underflowL(underflowL<0) = 0;
    underflowR = max_revthr - thrustR;
    underflowR(underflowR<0) = 0;
    thrustL = thrustL+underflowL+underflowR;
    thrustR = thrustR+underflowR+underflowL;
end

%%


%% PLOT:
X = categorical({'L','R'});
X = reordercats(X,{'L','R'});

iv = [floor(length(speed)/4), floor(length(speed)/2), length(speed)];
ia = [1, floor(length(alpha)/4)+1, ceil(length(alpha)/2),...
      3*floor(length(alpha)/4)+1, length(alpha)];
it = [length(thrust), floor(length(thrust)/2), floor(length(thrust)/4)];

figure(MODE);
set(gcf,'Position', [60, 80, 780, 1010]);
dims = [length(it), length(ia)];
tile = tiledlayout(dims(1),dims(2));
xlabel(tile,'Rudder Angle', 'FontSize',16);
tab = '                  ';
ylabel(tile,{'Total Thrust   ',...
             sprintf([tab,'F_T = %0.0f%%', tab],...
             thrust(it(3)),thrust(it(2)),thrust(it(1)))}, 'FontSize',16);
for i = 1:dims(1)
    for j = 1:dims(2)
        if (MODE==1)
            title(tile,'Differential Thrust Map (Physics Mode)','FontSize',20);
            % Differential Thrust = fn(speed, rudder angle, total thrust)
            Y = [thrustL_PCT(ia(j),iv(1),it(i)), thrustL_PCT(ia(j),iv(2),it(i)), thrustL_PCT(ia(j),iv(3),it(i));...
                 thrustR_PCT(ia(j),iv(1),it(i)), thrustR_PCT(ia(j),iv(2),it(i)), thrustR_PCT(ia(j),iv(3),it(i))];
            
        elseif (MODE==2)
            title(tile,'Differential Thrust Map (Normal Mode)','FontSize',20);
            % Differential Thrust = fn(rudder angle, total thrust)
            iv = 1;
            Y = [thrustL(ia(j),it(i)); thrustR(ia(j),it(i))];
            
        elseif (MODE==3)
            title(tile,'Differential Thrust Map (Aggro Mode)','FontSize',20);
            % Differential Thrust = fn(rudder angle, total thrust)
            iv = 1;
            Y = [thrustL(ia(j),it(i)); thrustR(ia(j),it(i))];
        end
        nexttile;
        b = bar(X,Y,0.4);
        
        % Titles
        title(strcat('\alpha =  ', string(alpha(ia(j))), '\circ'));
        
        % Data labels for tops of bars
        for k = 1:length(iv)
            xtips  = b(k).XEndPoints;
            ytips  = b(k).YEndPoints;
            for l=1:length(ytips)
                if (ytips(l)<0)
                    ytips(l) = ytips(l) - 10;
                end
            end
            labels = string(round(b(k).YData, 1));
            text(xtips,ytips,labels,'HorizontalAlignment','center',...
                'VerticalAlignment','bottom');
        end
        
        % Set Y-axis limits
        set(gca,'YLim',[-120,120]);
        set(gca,'FontSize',12)
        
    end
end

%{
set(gcf, 'Position',  [50, 650, 1600, 1000])
subplot(3,3,1);
Y
bar(X,Y)
plot(alpha', thrustL(:,1,1), alpha', thrustR(:,1,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,2);
plot(alpha', thrustL(:,2,1), alpha', thrustR(:,2,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,3);
plot(alpha', thrustL(:,3,1), alpha', thrustR(:,3,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,4);
plot(alpha', thrustL(:,1,2), alpha', thrustR(:,1,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,5);
plot(alpha', thrustL(:,2,2), alpha', thrustR(:,2,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,6);
plot(alpha', thrustL(:,3,2), alpha', thrustR(:,3,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,7);
plot(alpha', thrustL(:,1,3), alpha', thrustR(:,1,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,8);
plot(alpha', thrustL(:,2,3), alpha', thrustR(:,2,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,9);
plot(alpha', thrustL(:,3,3), alpha', thrustR(:,3,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-20 20]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', T(1,1,3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');


figure(2);
set(gcf, 'Position',  [50, 650, 1600, 1000])
subplot(3,3,1);
plot(alpha', thrustL_PCT(:,1,1), alpha', thrustR_PCT(:,1,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', thrust(1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,2);
plot(alpha', thrustL_PCT(:,2,1), alpha', thrustR_PCT(:,2,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', thrust(1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,3);
plot(alpha', thrustL_PCT(:,3,1), alpha', thrustR_PCT(:,3,1), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', thrust(1))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,4);
plot(alpha', thrustL_PCT(:,1,2), alpha', thrustR_PCT(:,1,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', thrust(2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,5);
plot(alpha', thrustL_PCT(:,2,2), alpha', thrustR_PCT(:,2,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', thrust(2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,6);
plot(alpha', thrustL_PCT(:,3,2), alpha', thrustR_PCT(:,3,2), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', thrust(2))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,7);
plot(alpha', thrustL_PCT(:,1,3), alpha', thrustR_PCT(:,1,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(1)),...
    sprintf('Requested Thrust = %0.1fN', thrust(3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,8);
plot(alpha', thrustL_PCT(:,2,3), alpha', thrustR_PCT(:,2,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(2)),...
    sprintf('Requested Thrust = %0.1fN', thrust(3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');

subplot(3,3,9);
plot(alpha', thrustL_PCT(:,3,3), alpha', thrustR_PCT(:,3,3), 'LineWidth', 2);
xlim([-max_rud max_rud]);
ylim([-1 1]);
title({sprintf('Velocity = %0.1fm/s', speed(3)),...
    sprintf('Requested Thrust = %0.1fN', thrust(3))});
xlabel('Rudder Angle');
ylabel('Differential Thrust [N]');
legend('Left','Right');
%}