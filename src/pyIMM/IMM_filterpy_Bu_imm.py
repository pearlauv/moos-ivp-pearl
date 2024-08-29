#!/usr/bin/env python3

# for pearl_heron giveway headon circle, works well with sigma pos = 0.5, v=0.01, theta=0
# h=5, nb_pred_step = 11, # for i in range(5, nb_prediction_steps-1):
# need to define the values of h, nb_pred_steps, noise, for
# -KF prediction error estimation
# -avoidance in situ pearl_heron
# -avoidance in simulation with julie gilda henry brian
# CV: R=I*1000 , Q= I*1
# CT: R=I*1 , Q= I*1000

import pymoos 
import math
import numpy as np
import matplotlib.pyplot as plt
import csv

from filterpy.kalman import KalmanFilter 
from filterpy.kalman import IMMEstimator 
from filterpy.common import Q_discrete_white_noise

""" from kalman_filter import KalmanFilter
from imm import Imm
import data
from plot import * """

import time
from dataclasses import dataclass
from typing import List

@dataclass
class VesselTracker:

    
    def __init__(self):
        # Initialize an empty dictionary for storing vessel data
        self.vessels = {} #will store x, y, speed, theta
        self.vessels_no_noise = {} #will store x, y, speed, theta
        self.vessels_for_filterpy = {}  #will store x, y, vx, vy because that what the filterpy uses (more specifically I got the covariance matrices for this format from papers)
        self.time_stamps = {}
        ##self.notify('INIT_TRACKER', 1, -1)

    def add_vessel_data_no_noise(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels_no_noise:
            # If not, create a new entry with an empty list
            self.vessels_no_noise[name] = []

        # # Append new data to the vessel's list, while keeping the format from x,y,speed, theta 
        self.vessels_no_noise[name].append(np.array([
                [x], 
                [y],
                [speed], 
                [theta] 
                ]))

    def add_vessel_data(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels:
            # If not, create a new entry with an empty list
            self.vessels[name] = []

        # # Append new data to the vessel's list, while keeping the format from x,y,speed, theta 
        self.vessels[name].append(np.array([
                [x], 
                [y],
                [speed], 
                [theta] 
                ]))
        # Append new data to the vessel's list, while keeping the format from x,y,speed, theta, and adds noise to the data
        # sigma_pos = 0.5
        # sigma_speed = 0.01
        # sigma_theta = 0 #degrees
        # self.vessels[name].append(np.array([
        #         [x+np.random.normal(0, sigma_pos)], 
        #         [y+np.random.normal(0, sigma_pos)],
        #         [speed+np.random.normal(0, sigma_speed)], 
        #         [theta+np.random.normal(0, sigma_theta)] 
        #         ]))

    def add_time_stamps(self, name, time):
        if name not in self.time_stamps:
            # If not, create a new entry with an empty list
            self.time_stamps[name] = []
        self.time_stamps[name].append(time)

    def get_time_stamps(self, name):
        
        return self.time_stamps.get(name, [])
    

    def add_vessel_data_for_filterpy(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels_for_filterpy:
            # If not, create a new entry with an empty list
            self.vessels_for_filterpy[name] = []

        # Append new data to the vessel's list, while changing the format from x,y,speed, theta -> x, y, dx, dy (expected by the kf_ct filter from filterpy). /!\ Will prbly have to convert deg-rad
        self.vessels_for_filterpy[name].append(np.array([
                [x], 
                [y],
                [speed*math.sin(theta*3.141592/180)],  #x is sin because of the referential in MOOS (north reference and clockwise positive angle)
                [speed*math.cos(theta*3.141592/180)] 
                ]))
        
        # Append new data to the vessel's list, while keeping the format from x,y,speed, theta, and adds noise to the data
        # sigma_pos = 0.5
        # sigma_speed = 0.01
        # sigma_theta = 0 #degrees
        # self.vessels_for_filterpy[name].append(np.array([
        #         [x+np.random.normal(0, sigma_pos)], 
        #         [y+np.random.normal(0, sigma_pos)],
        #         [(speed+np.random.normal(0, sigma_speed))*math.sin((theta+np.random.normal(0, sigma_theta))*3.141592/180)],  #x is sin because of the referential in MOOS (north reference and clockwise positive angle)
        #         [(speed+np.random.normal(0, sigma_speed))*math.cos((theta+np.random.normal(0, sigma_theta))*3.141592/180)] 
        #         ]))
        


        
    def get_vessel_data_for_filterpy(self, name):
        # Return the list of past states for the given vessel exploitable for IMM filter
        # If the vessel is not found, return an empty list
        return self.vessels_for_filterpy.get(name, [])
    
    def get_vessel_data(self, name):
        # Return the list of past states for the given vessel
        # If the vessel is not found, return an empty list
        return self.vessels.get(name, [])
    
    def get_vessel_data_no_noise(self, name):
        # Return the list of past states for the given vessel
        # If the vessel is not found, return an empty list
        return self.vessels_no_noise.get(name, [])


class pongMOOS(pymoos.comms):
    """ tracks: List[VehcileTrack] """

    tracker = VesselTracker()
    
    h= 30 #for the IMM
    
    def __init__(self, moos_community, moos_port):
        """Initiates MOOSComms, sets the callbacks and runs the loop"""
        super(pongMOOS, self).__init__()
        self.server = moos_community
        self.port = moos_port
        self.name = 'pyIMM_Bu'
        self.iter = 0

        self.set_on_connect_callback(self.__on_connect)
        self.set_on_mail_callback(self.__on_new_mail)
        self.run(self.server, self.port, self.name)

        self.notify('INIT_pongMOOS', 1, -1)

    def __on_connect(self):
        """OnConnect callback"""
        print("Connected to", self.server, self.port,
              "under the name ", self.name)
        ok = True
        ok &= self.register('NODE_REPORT_TRUE', 0)   #use NODE_REPORT_TRUE if want to spawn bhv with pcontactmanager, or use NODE_REPORT_LOCAL if just want to predict 
        
        self.notify('CONNECTED', 1, -1)

        return ok

    def __on_new_mail(self):            #### here we need to exclude the OS data or store them diffrently. Do they come as NAV_ or also as NODE_REPORT? How to parse? 
        """OnNewMail callback"""
        for msg in self.fetch():
            if msg.key() == 'NODE_REPORT_TRUE': #use NODE_REPORT_TRUE if want to spawn bhv with pcontactmanager, or use NODE_REPORT_LOCAL if just want to predict 
                
                self.iter += 1
                self.notify('NODE_REPOR_DETECTED', self.iter, -1)
                self.handle_node_report(msg.string())
            
        return True
    
    def handle_node_report(self, msg_string: str):
        self.notify('HANDLE_NODE_REPOR_CALLED', 1, -1)
        
        
        def is_float(value: str) -> bool:
            try:
                float(value)
                return True
            except ValueError:
                return False

        parts = msg_string.split(",")

        # Create a dictionary to store the data.
        data = {}

        # Iterate over the parts.
        for part in parts:

            # Split the part on the equals sign.
            key, value = part.split("=")

            # Convert the value to the appropriate type.
            if value.isdigit():
                value = int(value)
            elif is_float(value):
                value = float(value)

            # Add the key and value to the dictionary.
            data[key] = value

        # Return the dictionary.
        self.tracker.add_vessel_data(data["NAME"], data["X"], data["Y"], data["SPD"],data["HDG"])
        self.tracker.add_vessel_data_no_noise(data["NAME"], data["X"], data["Y"], data["SPD"],data["HDG"])
        self.tracker.add_vessel_data_for_filterpy(data["NAME"], data["X"], data["Y"], data["SPD"],data["HDG"])
        self.tracker.add_time_stamps(data["NAME"], data["TIME"])

        if data["NAME"] == "julie":
            stg = f"X={data['X']},Y={data['Y']},SPD={data['SPD']},HDG={data['HDG']},NAME={data['NAME']},TIME={time.time()}"
            #print(stg)
            self.notify("NODE_REPORT", stg ,-1)
        



    
    def _iterate(self): ### probably replaces the iterate function. what's the syntax?
        
        
        self.notify('ITERATE_CALLED', 1, -1)

        for name in self.tracker.vessels:

            if name == "abe" : #only generate prediction points for abe, not for other vessels

                z_from_AIS = self.tracker.get_vessel_data(name) ## Data in x,y,speed, theta format
                time_stamps = self.tracker.get_time_stamps(name)
                print(time_stamps)
                ## Compare the heading transmitted by the AIS with the positions of the vessel. Ex:If heading is 180 but the ship moves in diagonal, we know something is off with compass
                ## If heading seams unrelyable too often, recalculate heading and speed based on trig and diff in x and y divided by h

                heading_error = 0 ## count number of mistakes in heading

                for info in range (1, len (z_from_AIS)-1):
                    theta_approx = math.atan((z_from_AIS[info+1][1])-z_from_AIS[info-1][1]/(z_from_AIS[info+1][0])-z_from_AIS[info-1][0])
                    if z_from_AIS[info][3]< theta_approx - 10 or z_from_AIS[info][3]> theta_approx + 10:  ## here check if we should use degree or rad and see how atan works, and change margin value (10). Also maybe add modulo here for 360deg?
                        heading_error+=1
                ## if heading_error > acceptable threshold
                    ## replace the thetas from compass by the approximated thetas

                print("vessel name: " + name)
                print("z_from_AIS:  ")
                print(len(z_from_AIS))

                ##create the u vector

                # Assuming Δt is a constant time interval
                #delta_t = 10  # Replace with your desired value of Δt  marche mieux avec =1

                # Initialize the list to store ax and ay values
                u = []
                u.append(np.array([
                    [0], 
                    [0]]
                    ))

                # Iterate over the rows in the table
                for i in range(1, len(z_from_AIS)):
                    # Retrieve the values from the table
                    delta_t = time_stamps[i]-time_stamps[i-1]
                    v_k = z_from_AIS[i][2]
                    theta_k = z_from_AIS[i][3]*3.141592/180 #converts deg to rads

                    
                    v_k_minus_1 = z_from_AIS[i - 1][2]
                    theta_k_minus_1 = z_from_AIS[i - 1][3]*3.141592/180 #converts deg to rads

                    # Calculate the angular velocity omega
                    omega_k = (theta_k - theta_k_minus_1) / delta_t

                    # Calculate ax and ay using the given formulas
                    ax = (v_k * math.sin(theta_k + omega_k * delta_t) - v_k_minus_1 * math.sin(theta_k_minus_1)) / delta_t #replaced cos by sin to be consistent 
                    ay = (v_k * math.cos(theta_k + omega_k * delta_t) - v_k_minus_1 * math.cos(theta_k_minus_1)) / delta_t

                    ax = (v_k * math.sin(theta_k) - v_k_minus_1 * math.sin(theta_k_minus_1)) / delta_t #replaced cos by sin to be consistent 
                    ay = (v_k * math.cos(theta_k) - v_k_minus_1 * math.cos(theta_k_minus_1)) / delta_t

                    # Append the ax and ay values to the list
                    u.append(np.array([
                    [ax], 
                    [ay] 
                    ]))

                ## If not enough data to use IMM and get reliable prediction, use our home made predictor:

                if len(z_from_AIS)<3:

                

                    h= 30 ## maybe modify this with the black out period of the AIS and pNodeReport
                    size = len(z_from_AIS)
                    nb_values_averaged = 3
                    average_dtheta = 0
                    margin_dtheta = 30
                    margin_ddtheta = 0.01
                    number_ct_steps = 0

                    if size > nb_values_averaged: ## Averages the rate of turn based on the last (nb_values_averaged) theta values
                        average_dtheta = (z_from_AIS[size-1][3] - z_from_AIS[size-1 - nb_values_averaged][3]) / (nb_values_averaged*h)
                        
                        last_dthetas = []  ## Creates a list to store the calculated rates of turns dthetas for each of the last (nb_values_averaged) measurements 
                        last_ddthetas = []

                        for i in range(size - nb_values_averaged, size-1):  ## Calculate rate of turn dtheta 
                            last_dthetas.append(z_from_AIS[i+1][3] - z_from_AIS[i][3])

                        for i in range(len(last_dthetas)):      ##Counts how many of the dthetas are approximately equal to the average dtheta
                            if last_dthetas[i] >= average_dtheta - margin_dtheta and last_dthetas[i] <= average_dtheta + margin_dtheta:
                                number_ct_steps += 1

                    nb_prediction_steps = 5
                    list_predictions = []
                    list_predictions.append(z_from_AIS[size-1])

                    for i in range(nb_prediction_steps):
                        new_predicted_x = list_predictions[-1][0] + list_predictions[-1][2]*math.sin(list_predictions[-1][3]*3.141592/180)*h## Here the sin and cos are depending on how the heading info is measured. The reference is the north axis and theta increases clockwise.
                        new_predicted_y = list_predictions[-1][1] + list_predictions[-1][2]*math.cos(list_predictions[-1][3]*3.141592/180)*h
                        new_predicted_v = list_predictions[-1][2]
                        new_predicted_theta = list_predictions[-1][3] ##assumes constant velocity, no rate of turn

                        if number_ct_steps/nb_values_averaged >= 0.5: ##assumes constant rate of turn because at least half of the last data points fit with this model (confidence>50%)
                            new_predicted_theta = list_predictions[-1][3] + (number_ct_steps/nb_values_averaged)*average_dtheta*h##the average rate of turn is weighed with the confidence we have 

                        new_predicted_info = [new_predicted_x, new_predicted_y, new_predicted_v, new_predicted_theta]
                        list_predictions.append(new_predicted_info)

                    ## View the home-made predicted trajectory (how long does this stay on screen?)
                    seglist_string = 'pts={'
                    for i in range (0,nb_prediction_steps-1 ):
                        #print(list_predictions[i][0][0])
                        seglist_string += str(list_predictions[i][0][0]) + ',' + str(list_predictions[i][1][0]) + ':'
                    seglist_string = seglist_string[:-1] # removes last ':'
                    seglist_string+= "}, label=" + "pred initial"+name

                    self.notify('VIEW_SEGLIST', seglist_string, -1)

                else :
                ## If enough data and not corrupted heading data, do the IMM filter:

                    
                    h= 30 
                    z_noise = self.tracker.get_vessel_data_for_filterpy(name)
                    z_no_noise = self.tracker.get_vessel_data_no_noise(name)

                    #print("z_noise: ") 
                    #print(z_noise)  

                    ############################## Constant turn (ie constant acceleration) model ###########################################################
                    kf_ct = KalmanFilter(dim_x=4, dim_z=4, dim_u=2)

                    kf_ct.x = np.array([z_noise[0][0], z_noise[0][1],z_noise[0][2],z_noise[0][3]])
                    

                    kf_ct.F = np.array([[1., 0., h, 0.],   # x   
                                    [0., 1., 0., h],   # y  
                                    [0., 0., 1., 0.],   # vx  
                                    [0., 0., 0., 1.]])  # vy  

                    kf_ct.H = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]])
                    
                    kf_ct.B = np.array([[0.5*h**2,   0.],    #x
                                    [0.,     0.5*h**2],   #y
                                    [h,          0.],      #vx                       
                                    [0.,         h]])     #vy
                    
                    # kf_ct.R = np.array([[100., 0., 0., 0.], #using data from Jaskólski Two dimensional kf_ct in downloads

                    #                 [0., 100., 0., 0.],
                    #                 [0., 0., 0.04, 0.],
                    #                 [0., 0., 0., 0.04]])  
                    # kf_ct.Q = np.array([[100., 100., 2., 2.],
                    #                 [100., 100., 2., 2.],
                    #                 [2., 2., 0.04, 0.04],
                    #                 [2., 2., 0.04, 0.04]])  
                    
                    # kf_ct.P = np.array([[100., 100., 2., 2.],
                    #                 [100., 100., 2., 2.],
                    #                 [2., 2., 0.04, 0.04],
                    #                 [2., 2., 0.04, 0.04]])  
                

                    ############## Identity
                    kf_ct.R = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]]) * 1 #1000 works well wih all other identity and small noise on mesurement 0.1 m 
                    kf_ct.Q = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]]) * 1000 
                    kf_ct.P = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]])      

                
                    ############################## Constant velocity model ###########################################################
                    kf_cv = KalmanFilter(dim_x=4, dim_z=4, dim_u=2)

                    kf_cv.x = np.array([z_noise[0][0], z_noise[0][1],z_noise[0][2],z_noise[0][3]])
                    

                    kf_cv.F = np.array([[1., 0., h, 0.],   # x   
                                    [0., 1., 0., h],   # y  
                                    [0., 0., 1., 0.],   # vx  
                                    [0., 0., 0., 1.]])  # vy  

                    kf_cv.H = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]])
                    
                    kf_cv.B = np.array([[0.,   0.],    #x
                                    [0.,     0.],   #y
                                    [0.,          0.],      #vx                       
                                    [0.,         0.]])     #vy
                    
                    # kf_cv.R = np.array([[100., 0., 0., 0.], #using data from Jaskólski Two dimensional kf_ct in downloads

                    #                 [0., 100., 0., 0.],
                    #                 [0., 0., 0.04, 0.],
                    #                 [0., 0., 0., 0.04]])  
                    # kf_cv.Q = np.array([[100., 100., 2., 2.],
                    #                 [100., 100., 2., 2.],
                    #                 [2., 2., 0.04, 0.04],
                    #                 [2., 2., 0.04, 0.04]])  
                    
                    # kf_cv.P = np.array([[100., 100., 2., 2.],
                    #                 [100., 100., 2., 2.],
                    #                 [2., 2., 0.04, 0.04],
                    #                 [2., 2., 0.04, 0.04]])  
                    

                    ############## Identity
                    kf_cv.R = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]]) * 1000
                    kf_cv.Q = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]])
                    kf_cv.P = np.array([[1., 0., 0., 0.],
                                    [0., 1., 0., 0.],
                                    [0., 0., 1., 0.],
                                    [0., 0., 0., 1.]])

                    ## IMM ################################################################################################################
                    filters = [kf_cv, kf_ct]
                    mu = [0.001, 0.999]
                    trans = np.array([[0.999, 0.001], [0.001, 0.999]])
                    imm = IMMEstimator(filters, mu, trans)

                    prob_history = []
                    proba_model1 = []

                    xs, ys,  X_s = [], [], []
                    x, y = [], []
                    xp, yp, vxp, vyp = [], [], [],[] #they store the 10 predicted steps after the last signal received, using constant u from last data and last estimated position

                    for n in range (1, len(z_noise)-1):
                        z = z_noise[n]
                        """ print("in loop iteration n: ")
                        print(n)
                        print("z")
                        print(z) """
                        u_val  = u[n]  
                        u_val = u_val.reshape((2, 1)) #if not then its 2,1,1
                        imm.predict(u_val)
                        imm.update(z)
                        """ print("imm.x:")
                        print(imm.x)
                        print("imm.x[0]:")
                        print(imm.x[0]) """
                        prob_history.append(imm.mu.copy())
                        xs.append(imm.x[0].copy()[-1])
                        ys.append(imm.x[1].copy()[-1])
                        
                        # print("prob_history:")
                        # print(prob_history)

                        # print("prob_history[0]:")
                        # print(prob_history[0])

                        
                        #print("ys:")
                        #print(ys)
                        X_s.append(imm.x.copy())  # X_s stores all the estimates x,y,vx,vy, the last set of values of this list is used for further prediction. eventually it contains all the estimated positions and the 10 predicted position
                        
                        x.append(z_no_noise[n][0][0])      # x and y store all the measurements x and y
                        y.append(z_no_noise[n][1][0])
                        
                    #xp.append(X_s[-1][0])
                    #yp.append(X_s[-1][1])

                    #log error in estimate
                    # Compute xse, yse, and se
                    xse = [math.sqrt((xi - xsi)**2) for xi, xsi in zip(x, xs)]
                    yse = [math.sqrt((yi - ysi)**2) for yi, ysi in zip(y, ys)]
                    se = [math.sqrt(xsei**2 + ysei**2) for xsei, ysei in zip(xse, yse)]
                    

                    # Prepare the data to write
                    data = list(zip(x, xs, xse, y, ys, yse, se))

                    # Write the data
                    with open('estimate_error.csv', 'w', newline='') as file:
                        writer = csv.writer(file)

                        # Write the header
                        writer.writerow(["x", "xs", "xse", "y", "ys", "yse", "se"])

                        # Write the computed data
                        for row in data:
                            writer.writerow(row)
                    
                    nb_prediction_steps = 3
                    for i in range (1, nb_prediction_steps):
                        imm.predict(u[-1].reshape((2, 1)))
                        X_s.append(imm.x.copy())
                        xp.append(imm.x[0].copy())
                        yp.append(imm.x[1].copy())
                        vxp.append(imm.x[2].copy())
                        vyp.append(imm.x[3].copy())

                    # p1 = plt.scatter(x, y, color='r', marker='.', s=75, alpha=0.5)
                    # p2, = plt.plot(xs, ys, lw=2)
                    # p3 = plt.scatter(xp, yp, color='b', marker='.', s=75, alpha=0.5)
                    # plt.legend([p3, p2, p1], ['Pred', 'Kalman filter', 'Measurements'],
                    # scatterpoints=1)  
                    # plt.figure(figsize=(10, 6))

                    # for i, history in enumerate(zip(*prob_history)):
                    #     plt.plot(history, label=f'Model {i + 1}')

                    # plt.title('Model probabilities over time')
                    # plt.xlabel('Time step')
                    # plt.ylabel('Probability')
                    # plt.ylim([0, 1])
                    # plt.legend()
                    # plt.show()

                    # print("vxp")
                    # print(vxp)
                    # print("vyp")
                    # print(vyp)

                    ## View the kf_ct predicted trajectory
                    seglist_string = 'pts={'
                    
                    for i in range(1, nb_prediction_steps-1):
                        seglist_string += str(xp[i][0]) + ',' + str(yp[i][0]) + ':' #here we select the last list of predictions and we print all the points
                        self.notify("NODE_REPORT", f"X={xp[i][0]},Y={yp[i][0]},SPD={math.sqrt(vxp[i][0]**2+vyp[i][0]**2)},HDG={math.atan2(vxp[i][0], vyp[i][0])*180/3.141592},NAME=prediction{name}_{i},TIME={time.time()}",-1)
                        #print("HDG={math.atan2(vxp[i][0], vyp[i][0])*180/3.141592}")
                        #print(math.atan2(vxp[i][0], vyp[i][0])*180/3.141592)
                    seglist_string = seglist_string[:-1] # removes last ':'
                    seglist_string+= "}, label=" + "pred kf"+ name

                    self.notify('VIEW_SEGLIST', seglist_string, -1) 

                    print(seglist_string)

                    #plt.show()    

                # return True

def main():
    
    ##self.notify('MAIN_CALLED', 1, -1)

    ponger1 = pongMOOS('localhost', 9002)
    

    while True:
        
        time.sleep(1)
        if ponger1.iter > 0: #Im not sure if this is supposed to be where you call the iterate function. This condition is to wait for a new mail before iterating again
            ponger1._iterate()
            ponger1.iter=0 
        ponger1.notify('WHILE_TRUE', 1, -1)

        
            
if __name__ == "__main__":

    ##self.notify('_MAIN_called', 1, -1)
    main()


