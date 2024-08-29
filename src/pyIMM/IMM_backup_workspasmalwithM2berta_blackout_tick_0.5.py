#!/usr/bin/env python3
import pymoos 
import math
import numpy as np

from kalman_filter import KalmanFilter
from imm import Imm
import data
from plot import *
import time
from dataclasses import dataclass
from typing import List

@dataclass
class VesselTracker:

    ## to calculate the CPA with the current own ship position
    OS_last_x =0
    OS_last_y =0 
    ## to calculate the CPA with the predicted own ship postion if we want to try this later
    OS_last_v =0
    OS_last_theta =0   

    def __init__(self):
        # Initialize an empty dictionary for storing vessel data
        self.vessels = {} #will store x, y, speed, theta
        self.vessels_for_IMM = {} #will store x, vx, y, vy because that what the IMM uses
        ##self.notify('INIT_TRACKER', 1, -1)

    def add_vessel_data(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels:
            # If not, create a new entry with an empty list
            self.vessels[name] = []

        # Append new data to the vessel's list, while keeping the format from x,y,speed, theta 
        self.vessels[name].append(np.array([
                [x], 
                [y],
                [speed], 
                [theta] 
                ]))

    def add_vessel_data_for_IMM(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels_for_IMM:
            # If not, create a new entry with an empty list
            self.vessels_for_IMM[name] = []

        # Append new data to the vessel's list, while changing the format from x,y,speed, theta -> x, dx, y, dy (expected by the IMM filter). /!\ Will prbly have to convert rad-deg
        self.vessels_for_IMM[name].append(np.array([
                [x], 
                [speed*math.sin(theta*3.141592/180)],
                [y], 
                [speed*math.cos(theta*3.141592/180)] 
                ]))
        
    def add_vessel_data_dx_dy_for_IMM(self, name, x, dx, y, dy):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels_for_IMM:
            # If not, create a new entry with an empty list
            self.vessels_for_IMM[name] = []

        # Append new data to the vessel's list, while keeping  x, dx, y, dy (expected by the IMM filter). Used only for testing if data are generated from a program instead of pNodeReport
        self.vessels_for_IMM[name].append(np.array([
                [x], 
                [dx],
                [y], 
                [dy] 
                ]))


    def get_vessel_data_for_IMM(self, name):
        # Return the list of past states for the given vessel exploitable for IMM filter
        # If the vessel is not found, return an empty list
        return self.vessels_for_IMM.get(name, [])
    
    def get_vessel_data(self, name):
        # Return the list of past states for the given vessel
        # If the vessel is not found, return an empty list
        return self.vessels.get(name, [])
    
    


class pongMOOS(pymoos.comms):
    """ tracks: List[VehcileTrack] """

    tracker = VesselTracker()
    #dt = 0.1 #have to hardcode it because IMM and KFs need it to iterate. Might think of a solution later. Also, need to be consistent with pNodeReport appticks or blackout
    dt = 0.1 #for the IMM
    """pongMOOS is an example python MOOS app.
    It registers for 'PING' and responds with 'PONG' and the number of received
    'PING's
    Attributes:
        moos_community: a string representing the address of the Community
        moos_port:      an interger defining the port
    """
    def __init__(self, moos_community, moos_port):
        """Initiates MOOSComms, sets the callbacks and runs the loop"""
        super(pongMOOS, self).__init__()
        self.server = moos_community
        self.port = moos_port
        self.name = 'pyIMM'
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
        ok &= self.register('NODE_REPORT', 0)
        ok &= self.register('NAV_X', 0) #is this necessary or can we just use NODE_REPORT?
        ok &= self.register('NAV_Y', 0)

        self.notify('CONNECTED', 1, -1)

        return ok

    def __on_new_mail(self):            #### here we need to exclude the OS data or store them diffrently. Do they come as NAV_ or also as NODE_REPORT? How to parse? 
        """OnNewMail callback"""
        for msg in self.fetch():
            if msg.key() == 'NODE_REPORT':
                
                self.iter += 1
                self.notify('NODE_REPOR_DETECTED', self.iter, -1)
                self.handle_node_report(msg.string())
            
        return True
    
    def handle_node_report(self, msg_string: str):
        self.notify('HANDLE_NODE_REPOR_CALLED', 1, -1)
        """ parts = msg_string.split(",")

        # Create a dictionary to store the data.
        data = {}

        # Iterate over the parts.
        for part in parts:

            # Split the part on the equals sign.
            key, value = part.split("=")

            # Convert the value to the appropriate type.
            if value.isdigit():
                value = int(value)
            elif value.isfloat():
                value = float(value)

            # Add the key and value to the dictionary.
            data[key] = value """
        
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
        self.tracker.add_vessel_data_for_IMM(data["NAME"], data["X"], data["Y"], data["SPD"],data["HDG"])



    def kf_cv(self): #original one with extensive constructor
        A = np.array([
                [1., self.dt, 0., 0.],
                [0., 1., 0., 0.],
                [0., 0., 1., self.dt],
                [0., 0., 0., 1.]
                ])
        B = np.eye(A.shape[0])
        H = np.array([
            [1., 0., 0., 0.],
            [0., 1., 0., 0.],
            [0., 0., 1., 0.],
            [0., 0., 0., 1.]
            ])
        

        Q = np.array([
            [1., 0., 0., 0.],
            [0., 1., 0., 0.],
            [0., 0., 1., 0.],
            [0., 0., 0., 1.]
            ]) *5
        

        R = np.array([
            [1., 0., 0., 0.],
            [0., 1., 0., 0.],
            [0., 0., 1., 0.],
            [0., 0., 0., 1.]
            ])

        kf = KalmanFilter(A, B, H, Q, R)
        return kf
    

    def kf_ct(self): #original one with extensive constructor
        #dtheta = math.pi / 180 * 35
        dtheta = math.pi / 180 * 2
        theta = dtheta * self.dt
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
        
        
        R = np.eye(4) * 1 #mettre 1 au lieu de 150 ca a l'air pas mal
        
        return KalmanFilter(A, B, H, Q, R)
    
    def imm_cvt(self):
        P_trans = np.array([
            [0.98, 0.02],
            [0.02, 0.98]
        ])
        U_prob = np.array([0.5, 0.5]).reshape((-1, 1))

        models = [self.kf_cv(), self.kf_ct()]
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

    def _iterate(self): ### probably replaces the iterate function. what's the syntax?
        
        
        self.notify('ITERATE_CALLED', 1, -1)

        for name in self.tracker.vessels:

            z_from_AIS = self.tracker.get_vessel_data(name) ## Data in x,y,speed, theta format
            ## Compare the heading transmitted by the AIS with the positions of the vessel. Ex:If heading is 180 but the ship moves in diagonal, we know something is off with compass
            ## If heading seams unrelyable too often, recalculate heading and speed based on trig and diff in x and y divided by dt

            heading_error = 0 ## count number of mistakes in heading

            for info in range (1, len (z_from_AIS)-1):
                theta_approx = math.atan((z_from_AIS[info+1][1])-z_from_AIS[info-1][1]/(z_from_AIS[info+1][0])-z_from_AIS[info-1][0])
                if z_from_AIS[info][3]< theta_approx - 10 or z_from_AIS[info][3]> theta_approx + 10:  ## here check if we should use degree or rad and see how atan works, and change margin value (10). Also maybe add modulo here for 360deg?
                    heading_error+=1
            ## if heading_error > acceptable threshold
                ## replace the thetas from compass by the approximated thetas


            ## If not enough data to use IMM and get reliable prediction, use our home made predictor:

            if len(z_from_AIS)<4:

            

                dt = 10 ## maybe modify this with the black out period of the AIS and pNodeReport
                size = len(z_from_AIS)
                nb_values_averaged = 3
                average_dtheta = 0
                margin_dtheta = 30
                margin_ddtheta = 0.01
                number_ct_steps = 0

                if size > nb_values_averaged: ## Averages the rate of turn based on the last (nb_values_averaged) theta values
                    average_dtheta = (z_from_AIS[size-1][3] - z_from_AIS[size-1 - nb_values_averaged][3]) / (nb_values_averaged*dt)
                    print("z_from_AIS[size-1][3]")
                    print(z_from_AIS[size-1][3])
                    print("z_from_AIS[size-1 - nb_values_averaged][3])")
                    print(z_from_AIS[size-1 - nb_values_averaged][3])
                    print("average_dtheta") ## comment: when ship turns counter clockwise the value of dtheta was negative, which should be positive in trigo
                    print(average_dtheta)

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
                    new_predicted_x = list_predictions[-1][0] + list_predictions[-1][2]*math.sin(list_predictions[-1][3]*3.141592/180)*dt ## Here the sin and cos are depending on how the heading info is measured. The reference is the north axis and theta increases clockwise.
                    new_predicted_y = list_predictions[-1][1] + list_predictions[-1][2]*math.cos(list_predictions[-1][3]*3.141592/180)*dt
                    new_predicted_v = list_predictions[-1][2]
                    new_predicted_theta = list_predictions[-1][3] ##assumes constant velocity, no rate of turn

                    if number_ct_steps/nb_values_averaged >= 0.5: ##assumes constant rate of turn because at least half of the last data points fit with this model (confidence>50%)
                        new_predicted_theta = list_predictions[-1][3] + (number_ct_steps/nb_values_averaged)*average_dtheta*dt ##the average rate of turn is weighed with the confidence we have 

                    new_predicted_info = [new_predicted_x, new_predicted_y, new_predicted_v, new_predicted_theta]
                    list_predictions.append(new_predicted_info)

                ## View the home-made predicted trajectory (how long does this stay on screen?)
                seglist_string = 'pts={'
                for i in range (0,nb_prediction_steps-1 ):
                    #print(list_predictions[i][0][0])
                    seglist_string += str(list_predictions[i][0][0]) + ',' + str(list_predictions[i][1][0]) + ':'
                seglist_string = seglist_string[:-1] # removes last ':'
                seglist_string+= "}, label=" + "pipi"+name

                self.notify('VIEW_SEGLIST', seglist_string, -1)

            else :
            ## If enough data and not corrupted heading data, do the IMM filter:

                z_noise = self.tracker.get_vessel_data_for_IMM(name)

                imm = self.imm_cvt();
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
                    #for i in range(50): # predict 5s
                    number_of_predictions_IMM = 300
                    for i in range(number_of_predictions_IMM): # predict 5s
                        for i in range(len(states)): # each model predict
                            states[i] = np.dot(imm.models[i].A, states[i])
                        x_step = np.zeros(x.shape)
                        for i in range(len(imm.models)): # merge predict
                            x_step += np.dot(imm.model_trans[0][i], states[i]) * prob[-1][i]
                        pred_step.append(x_step.copy())
                    pred_z.append(pred_step)

                """ plot_position(
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
                ) """
                """ plot_prob(
                    [p[0,0] for p in prob],
                    [p[1,0] for p in prob],
                    [p[1,0] for p in prob],
                )
 """
                pred_x = []
                pred_y = []
                for step_z in pred_z:
                    curr_x = [z[0,0] for z in step_z]
                    pred_x.append(curr_x)
                    curr_y = [z[2,0] for z in step_z]
                    pred_y.append(curr_y)
                """ plot_prediction(
                    [z[0,0] for z in z_noise],
                    [z[2,0] for z in z_noise],
                    pred_x,
                    pred_y
                ) """

                plot_all(
                    [p[0,0] for p in prob],
                    [p[1,0] for p in prob],
                    [p[1,0] for p in prob],
                    [z[0,0] for z in z_noise],
                    [z[2,0] for z in z_noise],
                    pred_x,
                    pred_y

                )


                plot_show()

                #take the result of the prediction, compute the distance between TS prediction and current OS position. Need to find a way to define what ship name is the OS. Use 0 as place holder

                #current_x_os = self.tracker.get_vessel_data("pearl")[-1][0]
                #current_y_os = self.tracker.get_vessel_data("pearl")[-1][2]
                current_x_os = 0 # OS_last_x ?
                current_y_os = 0 #OS_last_y?
                

                pred_x_for_cpa = pred_x[-1][-1]
                pred_y_for_cpa = pred_y[-1][-1]

                """ print(" here are the pred_x     :")
                print(pred_x)
                print(" here are the pred_x[-1]     :")
                print(pred_x[-1])
                #print(pred_x_for_cpa)
                
                print(" here are the pred_x[len(pred_x)-1]     :")
                print(pred_x[len(pred_x)-1]) """

                predicted_cpa = math.sqrt((current_x_os-pred_x_for_cpa)**2+(current_y_os-pred_y_for_cpa)**2) #we use current OS and predicted TS. We could maybe also to predicted OS, depends if IMM good

                self.notify('PREDICTED_CPA', float(predicted_cpa), -1)

                ## Could be interesting to post a message to say if the iMM is used, what mode it think it is, etc... 

                ## View the IMM predicted trajectory
                seglist_string = 'pts={'
                self.notify('VIEW_SEGLIST', seglist_string, -1)
                for i in range(0, number_of_predictions_IMM-1):
                    seglist_string += str(pred_x[len(pred_x)-1][i]) + ',' + str(pred_y[len(pred_x)-1][i]) + ':'
                seglist_string = seglist_string[:-1] # removes last ':'
                seglist_string+= "}, label=" + "caca"+ name

                self.notify('VIEW_SEGLIST', seglist_string, -1)


                #here need to find what should be the real message to spawn a ColAv objective function 

                ## if position < range
                ##post actual message 
            
                
                return True

def main():
    
    ##self.notify('MAIN_CALLED', 1, -1)

    ponger = pongMOOS('localhost', 9000)

    while True:
        
        time.sleep(1)
        if ponger.iter > 0: #Im not sure if this is supposed to be where you call the iterate function. This condition is to wait for a new mail before iterating again
            ponger._iterate()
            ponger.iter=0 
        ponger.notify('WHILE_TRUE', 1, -1)
            
if __name__ == "__main__":

    ##self.notify('_MAIN_called', 1, -1)
    main()


