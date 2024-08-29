#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# By yongcong.wang @ 13/10/2020

""" #not bad values: 
dt = 10
kf_ct dtheta = *2
p trans = 0.1 0.1 0.8
count = 10 cv, 25 ct
turn rate = 1 
no noise

Idea: every time there is a new measurement, add it to the list of measurements, update the dtheta with the difference in angle with the last measurement, then run the new kalman filter
"""

import math
import numpy as np

from kalman_filter import KalmanFilter
from imm import Imm
import data
from plot import *

dt = 0.1

######################## Define the data to be used by the filters in test_cvt and test_cvat

def z_data(): #with dt=10, "AIS"data i'd like to test
    
    z_std = data.cv_z(0., 20, 0., 20, dt, 30)
    z_std += data.ct_z(z_std[-1][0,0], z_std[-1][1,0],
                       z_std[-1][2,0], z_std[-1][3,0], math.pi/180*35, dt, 30)
    z_std += data.cv_z(z_std[-1][0,0], z_std[-1][1,0], z_std[-1][2,0], z_std[-1][3,0], dt, 30)

    return z_std

""" def z_data(): #original data from github
    cnt = 100
    z_std = data.cv_z(0., 10., 0., 10., dt, cnt)
    z_std += data.ct_z(z_std[-1][0,0], z_std[-1][1,0],
                       z_std[-1][2,0], z_std[-1][3,0], math.pi/180*25, dt, cnt)
    z_std += data.ca_z(z_std[-1][0,0], z_std[-1][1,0], 6.,
                       z_std[-1][2,0], z_std[-1][3,0], 8., dt, cnt)

    return z_std """





############################ Define the kalman filters


""" def kf_cv(): #original one
    A = np.array([
        [1., dt, 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., dt],
        [0., 0., 0., 1.]
    ])
    H = np.array([
        [1., 0., 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]
    ])
    return KalmanFilter(A, H) """

def kf_cv(): #original one with extensive constructor
    A = np.array([
            [1., dt, 0., 0.],
            [0., 1., 0., 0.],
            [0., 0., 1., dt],
            [0., 0., 0., 1.]
            ])
    B = np.eye(A.shape[0])
    H = np.array([
        [1., 0., 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]
        ])
    #Q = np.eye(A.shape[0]) * 10. ** 2

    Q = np.array([
        [1., 0., 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]
        ]) *5
    
    """ Q = np.array([ #not original
        [dt*dt*dt*dt/4., dt*dt*dt/2., 0., 0.],
        [dt*dt*dt/2., dt*dt, 0., 0.],
        [0., 0., dt*dt*dt*dt/4., dt*dt*dt/2.],
        [0., 0., dt*dt*dt/2., dt*dt]
        ]) *1. """

    #R = np.eye(4) 

    R = np.array([
        [1., 0., 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]
        ])

    kf = KalmanFilter(A, B, H, Q, R)
    return kf




""" def kf_ca():
    A = np.array([
        [1., dt, 0.5 * dt**2, 0., 0., 0.],
        [0., 1., dt, 0., 0., 0.],
        [0., 0., 1., 0., 0., 0.],
        [0., 0., 0., 1., dt, 0.5 * dt**2],
        [0., 0., 0., 0., 1., dt],
        [0., 0., 1., 0., 0., 1.]
    ])
    H = np.array([
        [1., 0., 0., 0., 0., 0.],
        [0., 1., 0., 0., 0., 0.],
        [0., 0., 0., 1., 0., 0.],
        [0., 0., 0., 0., 1., 0.]
    ])
    return KalmanFilter(A, H) """

def kf_ca():
    A = np.array([
            [1., dt, 0.5 * dt**2, 0., 0., 0.],
            [0., 1., dt, 0., 0., 0.],
            [0., 0., 1., 0., 0., 0.],
            [0., 0., 0., 1., dt, 0.5 * dt**2],
            [0., 0., 0., 0., 1., dt],
            [0., 0., 1., 0., 0., 1.]
            ])
    B = np.eye(A.shape[0])
    H = np.array([
        [1., 0., 0., 0., 0., 0.],
        [0., 1., 0., 0., 0., 0.],
        [0., 0., 0., 1., 0., 0.],
        [0., 0., 0., 0., 1., 0.]
        ])
    Q = np.eye(A.shape[0])
    R = np.eye(4) * 1

    kf = KalmanFilter(A, B, H, Q, R)
    return kf

""" def kf_ct(): #original one
    dtheta = math.pi / 180 *2
    theta = dtheta * dt
    A = np.array([
         [1., math.sin(theta)/dtheta, 0., -(1 - math.cos(theta))/dtheta, 0.],
         [0., math.cos(theta), 0., -math.sin(theta), 0.],
         [0., (1 - math.cos(theta)) / dtheta, 1., math.sin(theta)/dtheta, 0.],
         [0., math.sin(theta), 0., math.cos(theta), 0.],
         [0., 0., 0., 0., 1.],
         ])
    H = np.array([
        [1., 0., 0., 0., 0.],
        [0., 1., 0., 0., 0.],
        [0., 0., 1., 0., 0.],
        [0., 0., 0., 1., 0.]
        ])
    return KalmanFilter(A, H) """

def kf_ct(): #original one with extensive constructor
    dtheta = math.pi / 180 * 35 #original is 15
    theta = dtheta * dt
    A = np.array([
         [1., math.sin(theta)/dtheta, 0., -(1 - math.cos(theta))/dtheta, 0.],
         [0., math.cos(theta), 0., -math.sin(theta), 0.],
         [0., (1 - math.cos(theta)) / dtheta, 1., math.sin(theta)/dtheta, 0.],
         [0., math.sin(theta), 0., math.cos(theta), 0.],
         [0., 0., 0., 0., 1.],
         ])
    B = np.eye(A.shape[0])
    H = np.array([
        [1., 0., 0., 0., 0.],
        [0., 1., 0., 0., 0.],
        [0., 0., 1., 0., 0.],
        [0., 0., 0., 1., 0.]
        ])
    Q = np.eye(A.shape[0])
    
    """ Q = np.array([
        [1., 0., 0., 0., 0.],
        [0., 10., 0., 0., 0.],
        [0., 0., 1., 0., 0.],
        [0., 0., 0., 10., 0.],
        [0., 0., 0., 0., 1.]
        ]) """
    
    R = np.eye(4) * 1 #mettre 1 au lieu de 150 ca a l'air pas mal

    """ R = np.array([
        [1., 0., 0., 0.],
        [0., 1., 0., 0.],
        [0., 0., 1., 0.],
        [0., 0., 0., 1.]
        ]) *1 """
    
    return KalmanFilter(A, B, H, Q, R)



############################################ Define the IMM filters (what models are used)

#### IMM: CVAT constant velocity, constant acceleration, constant turn

def imm_cvat():
    P_trans = np.array([
        [0.95, 0.025, 0.025],
        [0.025, 0.95, 0.025],
        [0.025, 0.025, 0.95]
    ])
    U_prob = np.array([0.5, 0.25, 0.25]).reshape((-1, 1))

    models = [kf_cv(), kf_ca(), kf_ct()]
    r = np.array([
        [5.],
        [2.],
        [5.],
        [2.5]
    ])
    for model in models:
        model.R *= r

    T12 = np.array([
        [1, 0, 0, 0],
        [0, 1, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 1, 0],
        [0, 0, 0, 1],
        [0, 0, 0, 0]
    ])
    T23 = np.array([
        [1, 0, 0, 0, 0, 0],
        [0, 1, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0]
    ])
    model_trans = [
        [np.eye(models[0].A.shape[0]), T12.T, np.dot(T12.T, T23.T)],
        [T12, np.eye(models[1].A.shape[0]), T23.T],
        [np.dot(T23, T12), T23, np.eye(models[2].A.shape[0])]
    ]

    return Imm(models, model_trans, P_trans, U_prob)

#########################

def imm_cvt():
    P_trans = np.array([
        [0.98, 0.02],
        [0.02, 0.98]
    ])
    U_prob = np.array([0.5, 0.5]).reshape((-1, 1))

    models = [kf_cv(), kf_ct()]
    r = np.array([
        [10.],
        [1.],
        [10.],
        [1.]
    ])
    for model in models:
        model.R *= r

    T12 = np.array([
        [1, 0, 0, 0],
        [0, 1, 0, 0],
        [0, 0, 0, 0],
        [0, 0, 1, 0],
        [0, 0, 0, 1],
        [0, 0, 0, 0]
    ])
    T23 = np.array([
        [1, 0, 0, 0, 0, 0],
        [0, 1, 0, 0, 0, 0],
        [0, 0, 0, 1, 0, 0],
        [0, 0, 0, 0, 1, 0],
        [0, 0, 0, 0, 0, 0]
    ])
    T13 = np.dot(T23, T12)
    model_trans = [
        [np.eye(models[0].A.shape[0]), T13.T],
        [T13, np.eye(models[1].A.shape[0])]
    ]

    return Imm(models, model_trans, P_trans, U_prob)

######################## Test the IMM filters :

def test_cvt(x_moos, vx_moos,y_moos, vy_moos):
    z_std = z_data()
    z_noise = data.add_noise(z_std, np.array([
        [1.],
        [.2],
        [1.],
        [.2]
    ]))

    if (x_moos - z_std[-1][0,0])**2 < 100 and ((y_moos - z_std[-1][0,0])**2 < 100 ) : 
        z_noise += [np.array([
            [x_moos],
            [vx_moos],
            [y_moos],
            [vy_moos]
            ])]
    

    imm = imm_cvt();
    z0 = z_noise[0]
    imm.models[0].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]]
        ])
    imm.models[1].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
        ])


    
    prob = []
    z_filt = []
    pred_z = []
    for z in z_noise:
        prob.append(np.copy(imm.filt(z)))
        # merge
        x = np.zeros(imm.models[0].X.shape)
        for i in range(len(imm.models)):
            x += np.dot(imm.model_trans[0][i], imm.models[i].X) * prob[-1][i]
        z_filt.append(x)

        # predict trajectory
        states = [imm.models[0].X.copy(),
                  imm.models[1].X.copy()]
        pred_step = []
        for i in range(50): # predict 5s
            for i in range(len(states)): # each model predict
                states[i] = np.dot(imm.models[i].A, states[i])
            x_step = np.zeros(x.shape)
            for i in range(len(imm.models)): # merge predict
                x_step += np.dot(imm.model_trans[0][i], states[i]) * prob[-1][i]
            pred_step.append(x_step.copy())
        pred_z.append(pred_step)

    plot_position(
        [z[0,0] for z in z_std],
        [z[2,0] for z in z_std],
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_filt],
        [z[2,0] for z in z_filt]
    )
    plot_speed(
        [z[1,0] for z in z_std],
        [z[3,0] for z in z_std],
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_filt],
        [z[3,0] for z in z_filt]
    )
    plot_prob(
        [p[0,0] for p in prob],
        [p[1,0] for p in prob],
        [p[1,0] for p in prob],
    )

    pred_x = []
    pred_y = []
    for step_z in pred_z:
        curr_x = [z[0,0] for z in step_z]
        pred_x.append(curr_x)
        curr_y = [z[2,0] for z in step_z]
        pred_y.append(curr_y)
    plot_prediction(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        pred_x,
        pred_y
    )


    plot_show()

    ### Write some code here to communicate to moos the future predicted position to be used by pContactMngr

    ###


######################### Acquire new data from MOOS

### Some sort of read function for x, y, v, theta

### 60, -19, 50 for dt = 0.1, 

x_moos = 60 #60 
y_moos = -19 #-8
v_moos = 50
theta_moos = 4.75

### Calculate vx, vy based on v and theta

vx_moos = math.cos(theta_moos)*v_moos
vy_moos = math.sin(theta_moos)*v_moos

### Add it to the list of z data (if not abberant value)

test_cvt(x_moos,vx_moos, y_moos,vy_moos)

def test_cvat():
    z_std = z_data()
    z_noise = data.add_noise(z_std, np.array([
        [5.],
        [2],
        [5.],
        [2]
    ]))

    
    imm = imm_cvat();
    z0 = z_noise[0]
    imm.models[0].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]]
    ])
    imm.models[1].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [0.],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])
    imm.models[2].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])

    prob = []
    z_filt = []
    pred_z = []
    for z in z_noise:
        prob.append(np.copy(imm.filt(z)))
        # merge
        x = np.zeros(imm.models[0].X.shape)
        for i in range(len(imm.models)):
            x += np.dot(imm.model_trans[0][i], imm.models[i].X) * prob[-1][i]
        z_filt.append(x)

        # predict trajectory
        states = [imm.models[0].X.copy(),
                  imm.models[1].X.copy(),
                  imm.models[2].X.copy()]
        pred_step = []
        for i in range(5): # predict 5s
            for i in range(len(states)): # each model predict
                states[i] = np.dot(imm.models[i].A, states[i])
            x_step = np.zeros(x.shape)
            for i in range(len(imm.models)): # merge predict
                x_step += np.dot(imm.model_trans[0][i], states[i]) * prob[-1][i]
            pred_step.append(x_step.copy())
        pred_z.append(pred_step)
            

    plot_position(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_filt],
        [z[2,0] for z in z_filt]
    )
    plot_speed(
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_filt],
        [z[3,0] for z in z_filt]
    )
    plot_prob(
        [p[0,0] for p in prob],
        [p[1,0] for p in prob],
        [p[2,0] for p in prob],
    )
    pred_x = []
    pred_y = []
    for step_z in pred_z:
        curr_x = [z[0,0] for z in step_z]
        pred_x.append(curr_x)
        curr_y = [z[2,0] for z in step_z]
        pred_y.append(curr_y)
    plot_prediction(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        pred_x,
        pred_y
    )

    plot_show()

#test_cvat()

def test_imm_veh():
    z_noise = data.veh_z_mia()

    imm = imm_cvat();
    z0 = z_noise[0]
    imm.models[0].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]]
    ])
    imm.models[1].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [0.],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])
    imm.models[2].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])

    prob = []
    z_filt = []
    for z in z_noise:
        prob.append(np.copy(imm.filt(z)))
        # merge
        x = np.zeros(imm.models[0].X.shape)
        for i in range(len(imm.models)):
            x += np.dot(imm.model_trans[0][i], imm.models[i].X) * prob[-1][i]
        z_filt.append(x)
        #return

    plot_position(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_filt],
        [z[2,0] for z in z_filt]
    )
    plot_speed(
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_filt],
        [z[3,0] for z in z_filt]
    )
    plot_prob(
        [p[0,0] for p in prob],
        [p[1,0] for p in prob],
        [p[2,0] for p in prob],
    )
    plot_show()

def test_imm_veh_pred():


    z_noise = data.veh_z_mia() # Uses the data created as bank of trajectory
    
    #z_noise = z_data() # If we don't want to add noise

    """ z_std = z_data()  #If we want to add noise 
    z_noise = data.add_noise(z_std, np.array([
        [5.],
        [2],
        [5.],
        [2]
    ])) """


    imm = imm_cvat();
    z0 = z_noise[0]
    imm.models[0].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]]
    ])
    imm.models[1].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [0.],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])
    imm.models[2].X = np.array([
        [z0[0, 0]],
        [z0[1, 0]],
        [z0[2, 0]],
        [z0[3, 0]],
        [0.]
    ])

    prob = []
    z_filt = []
    pred_z = []
    for z in z_noise:
        prob.append(np.copy(imm.filt(z)))
        # merge
        x = np.zeros(imm.models[0].X.shape)
        for i in range(len(imm.models)):
            x += np.dot(imm.model_trans[0][i], imm.models[i].X) * prob[-1][i]
        z_filt.append(x)

        # predict trajectory
        states = [imm.models[0].X.copy(),
                  imm.models[1].X.copy(),
                  imm.models[2].X.copy()]
        pred_step = []
        for i in range(5): # predict 5s
            for i in range(len(states)): # each model predict
                states[i] = np.dot(imm.models[i].A, states[i])
            x_step = np.zeros(x.shape)
            for i in range(len(imm.models)): # merge predict
                x_step += np.dot(imm.model_trans[0][i], states[i]) * prob[-1][i]
            pred_step.append(x_step.copy())
        pred_z.append(pred_step)
            

    plot_position(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        [z[0,0] for z in z_filt],
        [z[2,0] for z in z_filt]
    )
    plot_speed(
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_noise],
        [z[3,0] for z in z_noise],
        [z[1,0] for z in z_filt],
        [z[3,0] for z in z_filt]
    )
    plot_prob(
        [p[0,0] for p in prob],
        [p[1,0] for p in prob],
        [p[2,0] for p in prob],
    )
    pred_x = []
    pred_y = []
    for step_z in pred_z:
        curr_x = [z[0,0] for z in step_z]
        pred_x.append(curr_x)
        curr_y = [z[2,0] for z in step_z]
        pred_y.append(curr_y)
    plot_prediction(
        [z[0,0] for z in z_noise],
        [z[2,0] for z in z_noise],
        pred_x,
        pred_y
    )

    plot_show()

#test_imm_veh_pred()



################### test interpolation

from scipy.optimize import curve_fit

# Sample loop coordinates
x_test = [1, 2, 3, 4, 5, 4, 3, 2, 1]
y_test = [1, 3, 2, 4, 5, 6, 7, 6, 5]

# Define the function to fit (circular equation)
def circle_func(theta, R, cx, cy):
    return cx + R * np.cos(theta), cy + R * np.sin(theta)

# Normalize coordinates
mean_x = np.mean(x_test)
mean_y = np.mean(y_test)
x_norm = x_test - mean_x
y_norm = y_test - mean_y

# Convert Cartesian coordinates to polar coordinates
theta_test = np.arctan2(y_norm, x_norm)

# Perform curve fitting
popt, _ = curve_fit(circle_func, theta_test, x_norm)

# Get the optimized parameters
R_opt, cx_opt, cy_opt = popt
center_x = cx_opt + mean_x
center_y = cy_opt + mean_y

# Generate points for plotting the fitted curve
theta_fit = np.linspace(0, 2 * np.pi, 100)
x_fit, y_fit = circle_func(theta_fit, R_opt, center_x, center_y)

# Plot the original loop and the fitted curve
plt.plot(x_test, y_test, 'bo', label='Original Loop')
plt.plot(x_fit, y_fit, 'r-', label='Fitted Curve')
plt.xlabel('x')
plt.ylabel('y')
plt.axis('equal')
plt.legend()
plt.show()