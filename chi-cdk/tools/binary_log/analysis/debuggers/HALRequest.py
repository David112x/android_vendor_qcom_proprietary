#!/usr/bin/env python
###############################################################################################################################
#
# Copyright (c) 2019 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Confidential and Proprietary - Qualcomm Technologies, Inc.
#
###############################################################################################################################


class HALRequest:

    event_ids = ['HAL3_BufferInfo',
                 'HAL3_ProcessCaptureResult',
                 'HAL3_Notify']

    partial_metadata = 2

    def __init__(self, log):
        self.capture_request     = log['rCaptureRequest']
        self.usecase_request_obj = None
        self.buffers             = {}                                           # Buffer tracking
        self.num_output_buffers  = log['rCaptureRequest']['num_output_buffers']
        self.received_buffers    = 0
        self.results             = []                                           # Result tracking
        self.num_metadata        = 2
        self.received_metadata   = 0
        self.notifications       = []                                           # Notification tracking
        self.shutter_notified    = False
        self.is_error_state      = False

    def print_summary(self, scope=None):
        metadata_status = '  Complete' if self.received_metadata else 'Incomplete'
        shutter_status  = '  Complete' if self.shutter_notified else 'Incomplete'
        print('================================================================================')
        print('  Frame Number: {}'.format(self.capture_request['frame_number']))
        print('================================================================================')
        print('  HAL3 Request')
        print('--------------------------------------------------------------------------------')
        print('  Metadata                                                      {} ({}/{})'.format(metadata_status, self.received_metadata, self.partial_metadata))
        print('  Shutter Notified                                                    {}'.format(shutter_status))
        print('  Num Output Buffers                                                           {}'.format(self.num_output_buffers))
        for buffer_id, buffer_status in self.buffers.items():
            buffer_status = buffer_status.rjust(11, ' ')
            print('  Buffer {}                                                {}'.format(buffer_id, buffer_status))
        if scope != 'HAL':
            if self.usecase_request_obj is not None:
                self.usecase_request_obj.print_summary(scope=scope)

    def is_complete(self):
        buffers_done = self.received_buffers == self.num_output_buffers and \
                       all(buffer_status == 'Complete' for buffer_status in self.buffers.values())
        metadata_done = self.received_metadata == self.num_metadata
        return buffers_done and metadata_done and self.shutter_notified and not self.is_error_state

    def handle_event(self, event_id, log, is_flushing):
        if event_id == 'HAL3_BufferInfo':
            self.add_buffer(log, is_flushing)
        elif event_id == 'HAL3_ProcessCaptureResult':
            self.add_result(log)
        elif event_id == 'HAL3_Notify':
            self.add_notification(log, is_flushing)
        else:
            print('HALRequest cannot handle event: {}'.format(event_id))

    def add_buffer(self, buf, is_flushing):
        """ Update request with incoming buffer"""
        buffer_id = buf['rBuffer']['buffer']
        if buffer_id not in self.buffers:
            self.buffers[buffer_id] = 'Incomplete'
        else:
            buffer_status = buf['rBuffer']['status']
            if buffer_status == 0:
                self.buffers[buffer_id] = 'Complete'
            elif is_flushing:
                self.buffers[buffer_id] = 'Error_Flush'
            else:
                self.buffers[buffer_id] = 'Error'

    def add_result(self, res):
        """ Update request status with incoming result """
        self.received_buffers += res['rCaptureResult']['num_output_buffers']
        self.received_metadata = max(self.received_metadata, res['rCaptureResult']['partial_result'])
        self.results.append(res)

    def add_notification(self, notif, is_flushing):
        """ Update request status with incoming notification """
        notify_type = notif['rNotifyMessage']['type']
        if notify_type == 1: # Error notif
            notify_error = notif['rNotifyMessage']['message']['error']['error_code']
            if is_flushing and notify_error in [2, 3, 4]:
                self.received_metadata = 2
                self.shutter_notified = True
            else:
                self.is_error_state = True
        elif notify_type == 2: # Shutter notif
            self.shutter_notified = True
        self.notifications.append(notif)
