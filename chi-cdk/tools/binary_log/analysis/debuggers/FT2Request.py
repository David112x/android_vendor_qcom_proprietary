#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################

from collections import OrderedDict
import Utils


class FRORequest:

    fro_states = [  'Initialized',
                    'Ready to Execute',
                    'Executing',
                    'Input Resource Pending',
                    'Output Resource Pending',
                    'Output Error Resource Pending',
                    'Output Notification Pending',
                    'Output Error Notification Pending',
                    'Complete',
                    'Invalid'   ]

    def __init__(self, request_index):
        self.request_index             = request_index
        self.submitted_session_request = False
        self.stage_id                  = None
        self.stage_sequence_id         = None
        self.session_requests          = OrderedDict()
        self.current_state             = 0                #FRORequest state tracking
        self.complete                  = False

    def print_summary(self, scope=None):
        """ Print summary of this FRO Request """
        if self.submitted_session_request:
            request_state = (self.fro_states[self.current_state]).rjust(68,' ')
            print('      Req: {}{}'.format(self.request_index, request_state))
            for session_request in self.session_requests.values():
                session_request.print_summary(scope=scope)

    def update_state(self, state_info):
        """ Update state of this FRO Request """
        request_state = state_info['requestState']
        if request_state < len(self.fro_states):
            self.current_state = request_state
            if self.fro_states[self.current_state] == 'Complete':
                self.complete = True

    def is_complete(self):
        """ Get completion status """
        return self.complete

    def get_state(self):
        """ Get state status as readable string """
        return self.fro_states[self.current_state]

    def get_state_index(self):
        """ Get state status """
        return self.current_state

    def submit_session_request(self, session_request):
        """ Update this FRORequest with information from its session request """
        self.submitted_session_request = True
        self.stage_id                  = session_request['stageInfo']['stageId']
        self.stage_sequence_id         = session_request['stageInfo']['stageSequenceId']

    def update_session_request(self, session_result, session_request_id):
        # Update this FRORequest's session request with session results
        self.session_requests[session_request_id].process_result(session_result)


class FRO:

    def __init__(self, fro_init):
        self.frame_number        = fro_init['frameNumber']
        self.feature_name        = 'UnknownFeature'
        self.feature_id          = fro_init['featureId']
        self.feature_instance_id = fro_init['featureInstanceId']
        self.complete            = False                                    # FRO status tracking
        self.requests            = []                                       # FRO request index tracking
        for request_index in range(0, fro_init['numRequests']):
            self.requests.insert(request_index, FRORequest(request_index))


    def print_summary(self, scope=None):
        """ Print summary of every FRO request index"""
        print('      Number of Requests: {}'.format(len(self.requests)))
        if all(not request.submitted_session_request for request in self.requests):
            print('      No requests submitted to session')
        else:
            for request_id in range(0, len(self.requests)):
                request = self.requests[request_id]
                request.print_summary(scope=scope)

    def submit_request(self, session_req):
        """ Update FRO request at request id to have session request information """
        request_id  = Utils.get_fro_request_id(session_req)
        fro_request = FRORequest(session_req)
        self.requests.insert(request_id, fro_request)

    def update_request(self, state_info):
        """ Update proper FRORequest state """
        request_id = state_info['requestIndex']
        if request_id in self.requests:
            self.requests[request_id].update_state(state_info)
        else:
            print('FRO - Unknown request id: {} num_requests {}'.format(request_id, len(self.requests)))

    def is_complete(self):
        """ Return whether all FRORequests are complete """
        if all(req.is_complete() for req in self.requests):
            self.complete = True
        return self.complete


class URO:

    uro_states = [  'Initialized',
                    'InputConfigPending',
                    'OutputPending',
                    'Complete',
                    'Invalid']

    event_ids = [  'FT2_Graph_Init',
                    'FT2_Graph_FeatureInit',
                    'FT2_FRO_Init',
                    'FT2_FRO_StateInfo',
                    'FT2_Base_SubmitSessionRequest']

    def __init__(self, uro_init):
        self.graph_name = 'Unknown Graph'
        self.features = []
        self.fro_map = {}
        self.current_state = 0
        self.complete = False

    def print_summary(self, scope=None):
        """ Print Graph + Feature information """
        print('  ------------------------------------------------------------------------------')
        print('  CHI Feature2')
        print('  ------------------------------------------------------------------------------')
        print('    Graph                             {}'.format(self.graph_name))
        for fro_handle, fro in self.fro_map.items():
            feature_name = '{}_{}'.format(fro.feature_name, fro.feature_instance_id).ljust(25, ' ')
            print('    Feature {} FRO {}'.format(feature_name, fro_handle))
            fro.print_summary(scope=scope)

    def get_state(self):
        """ Get URO status as readable string """
        if self.complete:
            return 'Complete'
        else:
            return self.uro_states[self.current_state]

    def update_state(self, state_info):
        """ Update URO status """
        request_state = state_info['requestState']
        if request_state < len(self.uro_states):
            self.current_state = request_state
            if self.uro_states[self.current_state] == 'Complete':
                self.complete = True

    def is_complete(self):
        """ Return URO status """
        return self.complete

    def handle_event(self, event_id, log):
        """ Handle events that modify this URO """
        if event_id == 'FT2_Graph_Init':
            self.graph_name = log['rGraphId']
        elif event_id == 'FT2_Graph_FeatureInit':
            feature_id = '{}_{}'.format(log['featureId'], log['featureInstanceId'])
            self.features.append(feature_id)
        elif event_id == 'FT2_FRO_Init':
            fro_id = Utils.get_fro_handle(event_id, log)
            if fro_id not in self.fro_map:
                self.fro_map[fro_id] = FRO(log)
        elif event_id == 'FT2_FRO_StateInfo':
            fro_id = Utils.get_fro_handle(event_id, log)
            if fro_id != '0x0':
                request_id = Utils.get_fro_request_id(event_id, log)
                if fro_id in self.fro_map:
                    fro = self.fro_map[fro_id]
                    fro.requests[request_id].update_state(log)
                else:
                    print('Skipping {}, No FRO found for handle {}'.format(event_id, fro_id))
        elif event_id == 'FT2_URO_StateInfo':
            self.update_state(log)
