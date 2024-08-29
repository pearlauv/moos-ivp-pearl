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
    def __init__(self):
        # Initialize an empty dictionary for storing vessel data
        self.vessels = {}

    def add_vessel_data(self, name, x, y, speed, theta):
        # Check if vessel name already exists in the dictionary
        if name not in self.vessels:
            # If not, create a new entry with an empty list
            self.vessels[name] = []

        # Append new data to the vessel's list
        self.vessels[name].append([x, y, speed, theta])

    def get_vessel_data(self, name):
        # Return the list of past states for the given vessel
        # If the vessel is not found, return an empty list
        return self.vessels.get(name, [])


class pongMOOS(pymoos.comms):
    """ tracks: List[VehcileTrack] """

    tracker = VesselTracker()
    trackerdxdy = VesselTracker()
    dt = 0.1

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

    def __on_connect(self):
        """OnConnect callback"""
        print("Connected to", self.server, self.port,
              "under the name ", self.name)
        ok = True
        ok &= self.register('NODE_REPORT', 0)
        # ok &= self.register('NODE_REPORT', 0)
        return ok

    def __on_new_mail(self):
        """OnNewMail callback"""
        for msg in self.fetch():
            if msg.key() == 'NODE_REPORT':
                self.iter += 1
                self.handle_node_report(msg.string())
        return True
    
    def handle_node_report(self, msg_string: str):
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
            elif value.isfloat():
                value = float(value)

            # Add the key and value to the dictionary.
            data[key] = value

        # Return the dictionary.
        """ self.add_contact_to_list( data) """

        self.tracker.add_vessel_data(data["NAME"], data["X"], data["Y"], data["SPD"],data["HDG"])

        

    """ def add_contact_to_list(self, data):
        self.tracks.append(
            VehcileTrack(
                data["NAME"],
                [
                    data["X"],
                    data["Y"],
                    data["SPD"],
                    data["HDG"] # in degrees (might want to change to radians)
                ]
            )
        ) """

    # def iterate

    ## for name in self.tracker.vessels :

    # for every vehicle: convert speed and heading into dx, dy, and run the IMM

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
        dtheta = math.pi / 180 * 35 #original is 15
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

        models = [self.kf_cv(self), self.kf_ct(self)]
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

    def test_cvt(self): ### probably replaces the iterate function
        """ z_std = z_data()
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
                ])] """
        
        ##replace the above paragraph with the list of mesurements from the AIS
        ## z_noise = list of previous measurements. Have to convert x,y,v,theta to x, dx, y, dy

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
            for i in range(50): # predict 5s
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
        )
        plot_prob(
            [p[0,0] for p in prob],
            [p[1,0] for p in prob],
            [p[1,0] for p in prob],
        ) """

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
        )


        plot_show() """

        ##take the result of the prediction, compute the distance between TS prediction and current OS position
        ## if position < range
        ##post message 




ponger = pongMOOS('localhost', 9000)


ponger.tracker.add_vessel_data("Vessel1", 1, 2, 3, 4)
ponger.tracker.add_vessel_data("Vessel2", 5, 6, 7, 8)
ponger.tracker.add_vessel_data("Vessel3", 9, 10, 11, 12)
ponger.tracker.add_vessel_data("Vessel4", 13, 14, 15, 16)
ponger.tracker.add_vessel_data("Vessel5", 17, 18, 19, 20)
ponger.tracker.add_vessel_data("Vessel1", 1, 2, 3, 4)
ponger.tracker.add_vessel_data("Vessel1", 5, 6, 7, 8)
ponger.tracker.add_vessel_data("Vessel2", 9, 10, 11, 12)

print("Hello")
for name in ponger.tracker.vessels:
    print(ponger.tracker.get_vessel_data(name))


for name in ponger.tracker.vessels:
    for messages in ponger.tracker.vessels[name]:
        print(ponger.tracker.get_vessel_data(name)[0][0])
        pos_x = ponger.tracker.get_vessel_data(name)[0][0]
        pos_y = ponger.tracker.get_vessel_data(name)[0][1]
        speed = ponger.tracker.get_vessel_data(name)[0][2]
        angle = ponger.tracker.get_vessel_data(name)[0][3]

        ponger.trackerdxdy.add_vessel_data(name, pos_x, speed*math.cos(angle), pos_y, speed*math.sin(angle))


for name in ponger.trackerdxdy.vessels:
    print(ponger.trackerdxdy.get_vessel_data(name))