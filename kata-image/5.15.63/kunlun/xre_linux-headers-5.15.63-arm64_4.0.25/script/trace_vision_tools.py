#!/usr/bin/python
"""
Parse a RAW trace file into a trace event file
\author fangweikang
"""
import sys
import os
import re
import json
json.encoder.FLOAT_REPR = lambda x: format(x, '3.f')

"""timestamp interval"""
ts_interval = 30;
sdnn_enable_list = ['disabled', 'disabled', 'disabled', 'disabled', 'disabled', 'disabled']
sdnn_cluster_enable_list = ['disabled', 'disabled', 'disabled', 'disabled', 'disabled', 'disabled']
cluster_enable_list = ['disabled', 'disabled', 'disabled', 'disabled', 'disabled', 'disabled']
sse_enable = 'disabled'

sdnn_pid_list = ['SDNN0', 'SDNN1', 'SDNN2', 'SDNN3', 'SDNN4', 'SDNN5']
sdnn_tid_list = ['DMAI_0', 'DMAI_1', 'DS_0', 'DS_1', 'MAC', 'EW', 'RS', 'DMAO']
sdnn_coprocessor_list = ['DS_0', 'DS_1', 'MAC', 'EW', 'RS', 'DMAI_0', 'DMAI_1', 'DMAO']
sse_tid_list = ['Stream0', 'Stream1', 'Stream2', 'Stream3', 'Stream4', 'Stream5', 'Stream6',
                'Stream7', 'Stream8', 'Stream9', 'Stream10', 'Stream11']
lost_trace_tid_list = ['SDNN', 'SSE', 'Cluster']
event_field = {'args': '', 'cat': '', 'cname': '', 'id': '', 'name': '', 'ph': '', 'pid': '', 'tid': '', 'ts': ''}
""" sdnn_cluster """
xpu8_pid_list = ['SDNN_Cluster_0', 'SDNN_Cluster_1', 'SDNN_Cluster_2',
                 'SDNN_Cluster_3', 'SDNN_Cluster_4', 'SDNN_Cluster_5']
xpu8_tid_list = ['DMA', 'Core0', 'Core1', 'Core2', 'Core3', 'Core4', 'Core5', 'Core6', 'Core7']
""" cluster  """
xpu64_pid_list = ['Cluster_0', 'Cluster_1', 'Cluster_2', 'Cluster_3', 'Cluster_4', 'Cluster_5']
xpu64_tid_list = ['DMA0', 'DMA1', 'Core0', 'Core16', 'Core32', 'Core48', 'Group0_SIMD', 'Group0_Sync']
grab_config_64 = {'grab0': {'high': 39, 'low': 0, 'mask': 0},
                  'grab1': {'high': 10, 'low': 0, 'mask': 0},
                  'grab2': {'high': 21, 'low': 12, 'mask': 0},
                  'grab3': {'high': 21, 'low': 9, 'mask': 0},
                  'grab4': {'high': 21, 'low': 10, 'mask': 0},
                  'grab5': {'high': 8, 'low': 0, 'mask': 0},
                  'grab6': {'high': 9, 'low': 0, 'mask': 0},
                  'grab7': {'high': 0, 'low': 0, 'mask': 0}
                 }
grab_config_8 = {'grab0': {'high': 0, 'low': 0, 'mask': 0},
                 'grab1': {'high': 1, 'low': 0, 'mask': 0},
                 'grab2': {'high': 2, 'low': 0, 'mask': 0},
                 'grab3': {'high': 3, 'low': 0, 'mask': 0},
                 'grab4': {'high': 6, 'low': 0, 'mask': 0},
                 'grab5': {'high': 5, 'low': 0, 'mask': 0},
                 'grab6': {'high': 20, 'low': 0, 'mask': 0},
                 'grab7': {'high': 7, 'low': 0, 'mask': 0}
                }
#simd_busy = {'Cluster_0': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}},
#             'Cluster_1': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}},
#             'Cluster_2': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}},
#             'Cluster_3': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}},
#             'Cluster_4': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}},
#             'Cluster_5': {'grab0': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab1': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab2': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab3': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab4': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab5': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab6': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0},
#                          'grab7': {'a0': 0, 'b0': 0, 'a1': 0, 'b1': 0}}}

simd_busy = {'Cluster_0': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0}, 'Cluster_1': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0},
             'Cluster_2': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0}, 'Cluster_3': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0},
             'Cluster_4': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0}, 'Cluster_5': {'h0': 0, 'h1': 0, 'l0': 0, 'l1': 0}}

dma_8core_busy = {'SDNN_Cluster_0': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_1': {'v0': 0, 'v1': 0, 'ts': 0},
                  'SDNN_Cluster_2': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_3': {'v0': 0, 'v1': 0, 'ts': 0},
                  'SDNN_Cluster_4': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_5': {'v0': 0, 'v1': 0, 'ts': 0}}

dma0_busy = {'Cluster_0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_1': {'v0': 0, 'v1': 0, 'ts': 0},
             'Cluster_2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_3': {'v0': 0, 'v1': 0, 'ts': 0},
             'Cluster_4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_5': {'v0': 0, 'v1': 0, 'ts': 0}}

dma1_busy = {'Cluster_0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_1': {'v0': 0, 'v1': 0, 'ts': 0},
             'Cluster_2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_3': {'v0': 0, 'v1': 0, 'ts': 0},
             'Cluster_4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Cluster_5': {'v0': 0, 'v1': 0, 'ts': 0}}

#l2_8core_busy = {'SDNN_Cluster_0': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_1': {'v0': 0, 'v1': 0, 'ts': 0},
#                 'SDNN_Cluster_2': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_3': {'v0': 0, 'v1': 0, 'ts': 0},
#                 'SDNN_Cluster_4': {'v0': 0, 'v1': 0, 'ts': 0}, 'SDNN_Cluster_5': {'v0': 0, 'v1': 0, 'ts': 0}}

dma_cmd_8core_busy = {'SDNN_Cluster_0': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_1': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_2': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_3': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_4': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_5': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}}}

sfu_cmd_8core_busy = {'SDNN_Cluster_0': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_1': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_2': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_3': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_4': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                      'SDNN_Cluster_5': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                  'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                  'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                  'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}}}

sdnn_cmd_busy = {'SDNN_Cluster_0': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}},
                 'SDNN_Cluster_1': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}},
                 'SDNN_Cluster_2': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}},
                 'SDNN_Cluster_3': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}},
                 'SDNN_Cluster_4': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}},
                 'SDNN_Cluster_5': {'Core0': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core1': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core2': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core3': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core4': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core5': {'v0': 0, 'v1': 0, 'ts': 0},
                             'Core6': {'v0': 0, 'v1': 0, 'ts': 0}, 'Core7': {'v0': 0, 'v1': 0, 'ts': 0}}}

sdnn_cmd_xfence_done_busy = {'SDNN_Cluster_0': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                             'SDNN_Cluster_1': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                             'SDNN_Cluster_2': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                             'SDNN_Cluster_3': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                             'SDNN_Cluster_4': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                             'SDNN_Cluster_5': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                         'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                         'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                         'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}}}

sdnn_cmd_wb_busy = {'SDNN_Cluster_0': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                    'SDNN_Cluster_1': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                    'SDNN_Cluster_2': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                    'SDNN_Cluster_3': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                    'SDNN_Cluster_4': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}},
                    'SDNN_Cluster_5': {'Core0': {'v0': 0, 'v1': 0}, 'Core1': {'v0': 0, 'v1': 0},
                                'Core2': {'v0': 0, 'v1': 0}, 'Core3': {'v0': 0, 'v1': 0},
                                'Core4': {'v0': 0, 'v1': 0}, 'Core5': {'v0': 0, 'v1': 0},
                                'Core6': {'v0': 0, 'v1': 0}, 'Core7': {'v0': 0, 'v1': 0}}}

#pc_real_time = {'Cluster_0': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}},
#                'Cluster_1': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}},
#                'Cluster_2': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}},
#                'Cluster_3': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}},
#                'Cluster_4': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}},
#                'Cluster_5': {'grab0': {'s': -1, 'e': -1}, 'grab1': {'s': -1, 'e': -1},
#                             'grab2': {'s': -1, 'e': -1}, 'grab3': {'s': -1, 'e': -1},
#                             'grab4': {'s': -1, 'e': -1}, 'grab5': {'s': -1, 'e': -1},
#                             'grab6': {'s': -1, 'e': -1}, 'grab7': {'s': -1, 'e': -1}}}

all_sdnn_coprocessor_total_time = {
    'SDNN0': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
    'SDNN1': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
    'SDNN2': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
    'SDNN3': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
    'SDNN4': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
    'SDNN5': {
        'DS_0': 0,
        'DS_1': 0,
        'MAC': 0,
        'EW': 0,
        'RS': 0,
        'DMAI_0': 0,
        'DMAI_1': 0,
        'DMAO': 0,
        },
}
all_sdnn_coprocessor_prev_ts = {
    'SDNN0': {},
    'SDNN1': {},
    'SDNN2': {},
    'SDNN3': {},
    'SDNN4': {},
    'SDNN5': {},
}


def parse_grab_config(grab_option_name, grab_option_val):
    """ parse_grab_config from config file"""
    #print(grab_option_name, grab_option_val)
    if re.match(r"XRE_TRACER_CLUSTER_GRAB_MASK", grab_option_name):
        opt_val = int(grab_option_val, 16)
        for i in range(8):
            if ((opt_val >> i) & 0x1) == 0x1:
                grab_config_64['grab' + str(i)]['mask'] = 1
            else:
                grab_config_64['grab' + str(i)]['mask'] = 0
    elif re.match(r"XRE_TRACER_CLUSTER_GRAB_OPTION0", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_64['grab1']['high'] = (opt_val >> 24) &0xff
        grab_config_64['grab1']['low']  = (opt_val >> 16) &0xff
        grab_config_64['grab0']['high'] = (opt_val >> 8)  &0xff
        grab_config_64['grab0']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_CLUSTER_GRAB_OPTION1", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_64['grab3']['high'] = (opt_val >> 24) &0xff
        grab_config_64['grab3']['low']  = (opt_val >> 16) &0xff
        grab_config_64['grab2']['high'] = (opt_val >> 8)  &0xff
        grab_config_64['grab2']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_CLUSTER_GRAB_OPTION2", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_64['grab5']['high'] = (opt_val >> 24) &0xff
        grab_config_64['grab5']['low']  = (opt_val >> 16) &0xff
        grab_config_64['grab4']['high'] = (opt_val >> 8)  &0xff
        grab_config_64['grab4']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_CLUSTER_GRAB_OPTION3", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_64['grab7']['high'] = (opt_val >> 24) &0xff
        grab_config_64['grab7']['low']  = (opt_val >> 16) &0xff
        grab_config_64['grab6']['high'] = (opt_val >> 8)  &0xff
        grab_config_64['grab6']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_SDNN_CLUSTER_GRAB_MASK", grab_option_name):
        opt_val = int(grab_option_val, 16)
        for i in range(8):
            if ((opt_val >> i) & 0x1) == 0x1:
                grab_config_8['grab' + str(i)]['mask'] = 1
            else:
                grab_config_8['grab' + str(i)]['mask'] = 0
    elif re.match(r"XRE_TRACER_SDNN_CLUSTER_GRAB_OPTION0", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_8['grab1']['high'] = (opt_val >> 24) &0xff
        grab_config_8['grab1']['low']  = (opt_val >> 16) &0xff
        grab_config_8['grab0']['high'] = (opt_val >> 8)  &0xff
        grab_config_8['grab0']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_SDNN_CLUSTER_GRAB_OPTION1", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_8['grab3']['high'] = (opt_val >> 24) &0xff
        grab_config_8['grab3']['low']  = (opt_val >> 16)  &0xff
        grab_config_8['grab2']['high'] = (opt_val >> 8)  &0xff
        grab_config_8['grab2']['low']  = (opt_val >> 0)   &0xff
    elif re.match(r"XRE_TRACER_SDNN_CLUSTER_GRAB_OPTION2", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_8['grab5']['high'] = (opt_val >> 24) &0xff
        grab_config_8['grab5']['low']  = (opt_val >> 16) &0xff
        grab_config_8['grab4']['high'] = (opt_val >> 8)  &0xff
        grab_config_8['grab4']['low']  = (opt_val >> 0)  &0xff
    elif re.match(r"XRE_TRACER_SDNN_CLUSTER_GRAB_OPTION3", grab_option_name):
        opt_val = int(grab_option_val, 16)
        grab_config_8['grab7']['high'] = (opt_val >> 24) &0xff
        grab_config_8['grab7']['low']  = (opt_val >> 16) &0xff
        grab_config_8['grab6']['high'] = (opt_val >> 8)  &0xff
        grab_config_8['grab6']['low']  = (opt_val >> 0)  &0xff


def parse_sdnn_trace(trace, mile_stone, event_file):
    """parse sdnn trace"""
    trace_set = {}
    trace_set['args'] = ""
    trace_set['id'] = 0
    #trace_set['ts'] = mile_stone
    sdnn_id = (trace >> 29) & 0b111
    processor_id = (trace >> 25) & 0b1111
    busy_begin = (trace >> 22) & 0b1
    busy_end = (trace >> 23) & 0b1
    xflag_done = (trace >> 24) & 0b1
    if sdnn_id == 0:
        trace_set['pid'] = "SDNN0"
        us_new = (mile_stone + 4) / 1300.0
    elif sdnn_id == 1:
        trace_set['pid'] = "SDNN1"
        us_new = (mile_stone + 4) / 1300.0
    elif sdnn_id == 2:
        trace_set['pid'] = "SDNN2"
        us_new = (mile_stone + 4) / 1300.0
    elif sdnn_id == 3:
        trace_set['pid'] = "SDNN3"
        us_new = (mile_stone + 8) / 1300.0
    elif sdnn_id == 4:
        trace_set['pid'] = "SDNN4"
        us_new = (mile_stone + 10) / 1300.0
    elif sdnn_id == 5:
        trace_set['pid'] = "SDNN5"
        us_new = (mile_stone + 13) / 1300.0

    trace_set['ts'] = us_new

    dict_key_1 = "SDNN{}".format(sdnn_id)
    if processor_id == 0:
        dict_key_2 = "DS_0"
        trace_set['tid'] = "DS_0"
        trace_set['cname'] = "thread_state_uninterruptible"
    elif processor_id == 1:
        dict_key_2 = "DS_1"
        trace_set['tid'] = "DS_1"
        trace_set['cname'] = "thread_state_iowait"
    elif processor_id == 2:
        dict_key_2 = "MAC"
        trace_set['tid'] = "MAC"
        trace_set['cname'] = "thread_state_running"
    elif processor_id == 3:
        dict_key_2 = "EW"
        trace_set['tid'] = "EW"
        trace_set['cname'] = "thread_state_runnable"
    elif processor_id == 4:
        dict_key_2 = "RS"
        trace_set['tid'] = "RS"
        trace_set['cname'] = "detailed_memory_dump"
    elif processor_id == 5:
        dict_key_2 = "DMAI_0"
        trace_set['tid'] = "DMAI_0"
        trace_set['cname'] = "thread_state_unknown"
    elif processor_id == 6:
        dict_key_2 = "DMAI_1"
        trace_set['tid'] = "DMAI_1"
        trace_set['cname'] = "background_memory_dump"
    elif processor_id == 7:
        dict_key_2 = "DMAO"
        trace_set['tid'] = "DMAO"
        trace_set['cname'] = "light_memory_dump"

    if busy_begin == 1:
        trace_set['cat'] = "fsm"
        trace_set['name'] = "busy"
        trace_set['ph'] = "B"
        beat = json.dumps(trace_set)
        event_file.write(",\n")
        event_file.write(beat)
        prev_ts = us_new
        all_sdnn_coprocessor_prev_ts[dict_key_1][dict_key_2] = prev_ts
    elif busy_end == 1:
        trace_set['cat'] = "fsm"
        trace_set['name'] = "busy"
        trace_set['ph'] = "E"
        beat = json.dumps(trace_set)
        event_file.write(",\n")
        event_file.write(beat)
        prev_ts = all_sdnn_coprocessor_prev_ts[dict_key_1][dict_key_2]
        duration = us_new - prev_ts
        all_sdnn_coprocessor_total_time[dict_key_1][dict_key_2] += duration

    if xflag_done == 1:
        trace_set['cat'] = "issue_sdnn"
        trace_set['name'] = "xflag_done"
        trace_set['ph'] = "I"
        beat = json.dumps(trace_set)
        event_file.write(",\n")
        event_file.write(beat)
    return


def parse_sse_trace(trace, mile_stone, event_file):
    """parse sse trace"""
    trace_set = {}
    trace_set['id'] = 0
    trace_set['ts'] = mile_stone / 1300.0
    trace_set['pid'] = "SSE"
    trace_set['cname'] = "rail_load"
    trace_task_type = trace & 0b1111
    begin = (trace >> 4) & 0b1
    end = (trace >> 5) & 0b1
    xpu_finish = (trace >> 6) & 0b1
    stream_id = (trace >> 23) & 0b1111
    xpu_finish_num = (trace >> 7) & 0b1111
    token = (trace >> 11) & 0b111111111111
    trace_set['args'] = {'token': token}

    if stream_id == 0:
        trace_set['tid'] = "Stream0"
    elif stream_id == 1:
        trace_set['tid'] = "Stream1"
    elif stream_id == 2:
        trace_set['tid'] = "Stream2"
    elif stream_id == 3:
        trace_set['tid'] = "Stream3"
    elif stream_id == 4:
        trace_set['tid'] = "Stream4"
    elif stream_id == 5:
        trace_set['tid'] = "Stream5"
    elif stream_id == 6:
        trace_set['tid'] = "Stream6"
    elif stream_id == 7:
        trace_set['tid'] = "Stream7"
    elif stream_id == 8:
        trace_set['tid'] = "Stream8"
    elif stream_id == 9:
        trace_set['tid'] = "Stream9"
    elif stream_id == 10:
        trace_set['tid'] = "Stream10"
    elif stream_id == 11:
        trace_set['tid'] = "Stream11"

    if trace_task_type == 0:
        trace_set['name'] = "xpu-cluster task"
        trace_set['cname'] = "background_memory_dump"
        trace_set['cat'] = "fsm"
        if begin == 1:
            trace_set['ph'] = "B"
        elif end == 1:
            trace_set['ph'] = "E"
    elif trace_task_type == 1:
        trace_set['name'] = "xpu-sdnn task"
        trace_set['cname'] = "cq_build_attempt_failed"
        trace_set['cat'] = "fsm"
        if begin == 1:
            trace_set['ph'] = "B"
        elif end == 1:
            trace_set['ph'] = "E"
    elif trace_task_type == 2:
        trace_set['name'] = "interrupt"
        trace_set['cat'] = "issue_sse"
        trace_set['ph'] = "I"
    elif trace_task_type == 3:
        trace_set['name'] = "event record"
        trace_set['cat'] = "issue_sse"
        trace_set['ph'] = "I"
    elif trace_task_type == 4:
        trace_set['name'] = "event wait"
        trace_set['cname'] = "vsync_highlight_color"
        trace_set['cat'] = "fsm"
        if begin == 1:
            trace_set['ph'] = "B"
        elif end == 1:
            trace_set['ph'] = "E"

    beat = json.dumps(trace_set)
    event_file.write(",\n")
    event_file.write(beat)

    if xpu_finish == 1:
        trace_set['name'] = "xpu finish"
        trace_set['cname'] = "black"
        trace_set['cat'] = "issue_sse"
        trace_set['ph'] = "I"
        trace_set['args'] = {'xpu_finish_num': xpu_finish_num}
        beat = json.dumps(trace_set)
        event_file.write(",\n")
        event_file.write(beat)

    return


def parse_xpu_trace(ts_interval, trace, mile_stone, event_file, grab_config_64, grab_config_8):
    """parse xpu trace"""
    trace_set = {}
    trace_set['id'] = 0
    #trace_set['ts'] = mile_stone
    trace_set['args'] = ""
    trace_set['cat'] = "fsm"

    xpu_id = (trace >> 24) & 0b1111
    grab_id = (trace >> 21) & 0b111
    if xpu_id == 0:
        trace_set['pid'] = "SDNN_Cluster_0"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 1:
        trace_set['pid'] = "SDNN_Cluster_1"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 2:
        trace_set['pid'] = "SDNN_Cluster_2"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 3:
        trace_set['pid'] = "SDNN_Cluster_3"
        mile_stone_new = mile_stone + 7
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 4:
        trace_set['pid'] = "SDNN_Cluster_4"
        mile_stone_new = mile_stone + 12
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 5:
        trace_set['pid'] = "SDNN_Cluster_5"
        mile_stone_new = mile_stone + 14
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 6:
        trace_set['pid'] = "Cluster_0"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 7:
        trace_set['pid'] = "Cluster_1"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 8:
        trace_set['pid'] = "Cluster_2"
        mile_stone_new = mile_stone + 4
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 9:
        trace_set['pid'] = "Cluster_3"
        mile_stone_new = mile_stone + 8
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 10:
        trace_set['pid'] = "Cluster_4"
        mile_stone_new = mile_stone + 11
        us_new = mile_stone_new / 1300.0
    elif xpu_id == 11:
        trace_set['pid'] = "Cluster_5"
        mile_stone_new = mile_stone + 14
        us_new = mile_stone_new / 1300.0

    trace_set['ts'] = us_new

    #if xpu_id > 5 or grab_id == 0:
    #    for i in range(0,8):
    #        program_start = (trace >> (i + 8)) & 0b1
    #        program_done = (trace >> i) & 0b1
    #        core_id = (grab_id * 8) + i
    #        core_id_str = str(core_id)
    #        trace_set['tid'] = "Core" + core_id_str

    #        if program_start == 1:
    #            trace_set['ph'] = "B"
    #        elif program_done == 1:
    #            trace_set['ph'] = "E"

    #        if program_start == 1 or program_done == 1:
    #            beat = json.dumps(trace_set)
    #            event_file.write(",\n")
    #            event_file.write(beat)

    if xpu_id < 6:
        parse_xpu8_trace(ts_interval, grab_id, trace, trace_set, event_file, grab_config_8, xpu_id, mile_stone_new)
    else:
        parse_xpu64_trace(ts_interval, grab_id, trace, trace_set, event_file, grab_config_64, xpu_id, mile_stone_new)

    return


def parse_xpu8_trace(ts_interval, grab_id, trace, trace_set, event_file, grab_config_8, xpu_id, mile_stone):
    """ parse xpu8 trace"""
    grab_id_str = str(grab_id)
    config = grab_config_8['grab' + grab_id_str]
    xpu_id_str = str(xpu_id)
    i_X = 'SDNN_Cluster_' + xpu_id_str
    if config['high'] == 0 and config['low'] == 0:
        trace_set['name'] = "program"
        trace_set['cname'] = "rail_load"
        for i in range(0, 8):
            program_start = (trace >> (i + 8)) & 0b1
            program_done = (trace >> i) & 0b1
            core_id = i
            core_id_str = str(core_id)
            trace_set['tid'] = "Core" + core_id_str

            if program_start == 1:
                trace_set['ph'] = "B"
            elif program_done == 1:
                trace_set['ph'] = "E"

            if program_start == 1 or program_done == 1:
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    elif config['high'] == 5 and config['low'] == 0:
        #print("find dma trace")
        dma_8core_busy[i_X]['v1'] = dma_8core_busy[i_X]['v0']
        dma_8core_busy[i_X]['v0'] = (((trace >> 3) & 0b1) | ((trace >> 5) & 0b1)
                                                       | ((trace >> 7) & 0b1) | ((trace >> 9) & 0b1))
        #l2_8core_busy[i_X]['v1'] = l2_8core_busy[i_X]['v0']
        #l2_8core_busy[i_X]['v0'] = (trace & 0b1) | (((trace >> 1) & 0b1) & ((trace >> 2) & 0b1))

        if dma_8core_busy[i_X]['v0'] != dma_8core_busy[i_X]['v1']:
            trace_set['name'] = "busy"
            trace_set['cname'] = "heap_dump_object_type"
            trace_set['tid'] = "DMA"
            if dma_8core_busy[i_X]['v0'] != 0:
                trace_set['ph'] = "B"
                if mile_stone >= dma_8core_busy[i_X]['ts']:
                    beat = json.dumps(trace_set)
                    event_file.write(",\n")
                    event_file.write(beat)
                    dma_8core_busy[i_X]['ts'] = mile_stone
                else:
                    dma_8core_busy[i_X]['v0'] = 0
            else:
                trace_set['ph'] = "E"
                if mile_stone == dma_8core_busy[i_X]['ts']:
                    trace_set['ts'] = (mile_stone + ts_interval + 1) / 1300.0
                    dma_8core_busy[i_X]['ts'] = mile_stone + ts_interval + 1
                else:
                    dma_8core_busy[i_X]['ts'] = mile_stone
                    beat = json.dumps(trace_set)
                    event_file.write(",\n")
                    event_file.write(beat)
                    #if l2_8core_busy[i_X]['v0'] != l2_8core_busy[i_X]['v1']:
                    #    trace_set['name'] = "busy"
                    #    trace_set['cname'] = "cq_build_attempt_failed"
                    #    trace_set['tid'] = "L2_iCache"
                    #    if l2_8core_busy[i_X]['v0'] != 0:
                    #        trace_set['ph'] = "B"
                    #        if mile_stone >= l2_8core_busy[i_X]['ts']:
                    #            trace_set['ts'] = mile_stone / 1300.0
                    #            beat = json.dumps(trace_set)
                    #            event_file.write(",\n")
                    #            event_file.write(beat)
                    #            l2_8core_busy[i_X]['ts'] = mile_stone
                    #        else:
                    #            l2_8core_busy[i_X]['v0'] = 0
                    #    else:
                    #        trace_set['ph'] = "E"
                    #        if mile_stone == l2_8core_busy[i_X]['ts']:
                    #            trace_set['ts'] = (mile_stone + 6) / 1300.0
                    #            l2_8core_busy[i_X]['ts'] = mile_stone + 6
                    #        else:
                    #            trace_set['ts'] = mile_stone / 1300.0
                    #            l2_8core_busy[i_X]['ts'] = mile_stone
                    #        beat = json.dumps(trace_set)
                    #        event_file.write(",\n")
                    #        event_file.write(beat)
                    #elif config['high'] == 6 and config['low'] == 0:
                    #    l2_8core_busy[i_X]['v1'] = l2_8core_busy[i_X]['v0']
                    #    l2_8core_busy[i_X]['v0'] = ((trace >> 8) & 0b1) | ((trace >> 9) & 0b1) | ((trace >> 10) & 0b1) | ((trace >> 11) & 0b1) | ((trace >> 12) & 0b1) | ((trace >> 13) & 0b1) | ((trace >> 14) & 0b1) | ((trace >> 15) & 0b1)
                    #    #if l2_8core_busy[i_X]['v0'] != l2_8core_busy[i_X]['v1']:
                    #    #    trace_set['name'] = "busy"
                    #    #    trace_set['cname'] = "cq_build_attempt_failed"
                    #    #    trace_set['tid'] = "L2_iCache"
                    #    #    if l2_8core_busy[i_X]['v0'] != 0:
                    #    #        trace_set['ph'] = "B"
                    #    #    else:
                    #    #        trace_set['ph'] = "E"
                    #    #    beat = json.dumps(trace_set)
                    #    #    event_file.write(",\n")
                    #    #    event_file.write(beat)
                    #    if l2_8core_busy[i_X]['v0'] != l2_8core_busy[i_X]['v1']:
                    #        trace_set['name'] = "busy"
                    #        trace_set['cname'] = "cq_build_attempt_failed"
                    #        trace_set['tid'] = "L2_iCache"
                    #        if l2_8core_busy[i_X]['v0'] != 0:
                    #            trace_set['ph'] = "B"
                    #            if mile_stone >= l2_8core_busy[i_X]['ts']:
                    #                trace_set['ts'] = mile_stone / 1300.0
                    #                beat = json.dumps(trace_set)
                    #                event_file.write(",\n")
                    #                event_file.write(beat)
                    #                l2_8core_busy[i_X]['ts'] = mile_stone
                    #            else:
                    #                l2_8core_busy[i_X]['v0'] = 0
                    #        else:
                    #            trace_set['ph'] = "E"
                    #            if mile_stone == l2_8core_busy[i_X]['ts']:
                    #                trace_set['ts'] = (mile_stone + 6) / 1300.0
                    #                l2_8core_busy[i_X]['ts'] = mile_stone + 6
                    #            else:
                    #                trace_set['ts'] = mile_stone / 1300.0
                    #                l2_8core_busy[i_X]['ts'] = mile_stone
                    #            beat = json.dumps(trace_set)
                    #            event_file.write(",\n")
                    #            event_file.write(beat)
    elif config['high'] == 20 and config['low'] == 0:
        #print(hex(trace))
        for i in range(0, 8):
            i_C = 'Core' + str(i)
            dma_cmd_8core_busy[i_X][i_C]['v1'] = dma_cmd_8core_busy[i_X][i_C]['v0']
            dma_cmd_8core_busy[i_X][i_C]['v0'] = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            #dma_cmd = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            #if dma_cmd == 1:
            if dma_cmd_8core_busy[i_X][i_C]['v1'] != dma_cmd_8core_busy[i_X][i_C]['v0']:
                trace_set['name'] = "dma cmd"
                trace_set['cname'] = "cq_build_attempt_failed"
                trace_set['tid'] = "Core" + str(i)
                if dma_cmd_8core_busy[i_X][i_C]['v0'] != 0:
                    trace_set['ph'] = "B"
                else:
                    trace_set['ph'] = "E"
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    elif config['high'] == 7 and config['low'] == 0:
        for i in range(0, 8):
            i_C = 'Core' + str(i)
            sfu_cmd_8core_busy[i_X][i_C]['v1'] = sfu_cmd_8core_busy[i_X][i_C]['v0']
            sfu_cmd_8core_busy[i_X][i_C]['v0'] = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            if sfu_cmd_8core_busy[i_X][i_C]['v1'] != sfu_cmd_8core_busy[i_X][i_C]['v0']:
                trace_set['name'] = "sfu_cmd"
                trace_set['cname'] = "thread_state_runnable"
                trace_set['tid'] = "Core" + str(i)
                if sfu_cmd_8core_busy[i_X][i_C]['v0'] != 0:
                    trace_set['ph'] = "B"
                else:
                    trace_set['ph'] = "E"
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    elif config['high'] == 1 and config['low'] == 0:
        for i in range(0, 8):
            i_C = 'Core' + str(i)
            sdnn_cmd_busy[i_X][i_C]['v1'] = sdnn_cmd_busy[i_X][i_C]['v0']
            sdnn_cmd_busy[i_X][i_C]['v0'] = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            if sdnn_cmd_busy[i_X][i_C]['v1'] != sdnn_cmd_busy[i_X][i_C]['v0']:
                trace_set['name'] = "sdnn_cmd"
                trace_set['cname'] = "rail_idle"
                trace_set['tid'] = "Core" + str(i)
                if sdnn_cmd_busy[i_X][i_C]['v0'] != 0:
                    trace_set['ph'] = "B"
                    if mile_stone >= sdnn_cmd_busy[i_X][i_C]['ts']:
                        trace_set['ts'] = mile_stone / 1300.0
                        beat = json.dumps(trace_set)
                        event_file.write(",\n")
                        event_file.write(beat)
                        sdnn_cmd_busy[i_X][i_C]['ts'] = mile_stone
                    else:
                        sdnn_cmd_busy[i_X][i_C]['v0'] = 0
                else:
                    trace_set['ph'] = "E"
                    if mile_stone == sdnn_cmd_busy[i_X][i_C]['ts']:
                        trace_set['ts'] = (mile_stone + ts_interval + 1) / 1300.0
                        sdnn_cmd_busy[i_X][i_C]['ts'] = mile_stone + ts_interval + 1
                    else:
                        trace_set['ts'] = mile_stone / 1300.0
                        sdnn_cmd_busy[i_X][i_C]['ts'] = mile_stone
                    beat = json.dumps(trace_set)
                    event_file.write(",\n")
                    event_file.write(beat)
    elif config['high'] == 2 and config['low'] == 0:
        for i in range(0, 8):
            i_C = 'Core' + str(i)
            sdnn_cmd_xfence_done_busy[i_X][i_C]['v1'] = sdnn_cmd_xfence_done_busy[i_X][i_C]['v0']
            sdnn_cmd_xfence_done_busy[i_X][i_C]['v0'] = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            if sdnn_cmd_xfence_done_busy[i_X][i_C]['v1'] != sdnn_cmd_xfence_done_busy[i_X][i_C]['v0']:
                trace_set['name'] = "sdnn_cmd_xfence_done"
                trace_set['cname'] = "black"
                trace_set['tid'] = "Core" + str(i)
                if sdnn_cmd_xfence_done_busy[i_X][i_C]['v0'] != 0:
                    trace_set['ph'] = "B"
                else:
                    trace_set['ph'] = "E"
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    elif config['high'] == 3 and config['low'] == 0:
        for i in range(0, 8):
            i_C = 'Core' + str(i)
            sdnn_cmd_wb_busy[i_X][i_C]['v1'] = sdnn_cmd_wb_busy[i_X][i_C]['v0']
            sdnn_cmd_wb_busy[i_X][i_C]['v0'] = ((trace >> i) & 0b1) & (((trace >> 8) >> i) & 0b1)
            if sdnn_cmd_wb_busy[i_X][i_C]['v1'] != sdnn_cmd_wb_busy[i_X][i_C]['v0']:
                trace_set['name'] = "sdnn_cmd_wb"
                trace_set['cname'] = "light_memory_dump"
                trace_set['tid'] = "Core" + str(i)
                if sdnn_cmd_wb_busy[i_X][i_C]['v0'] != 0:
                    trace_set['ph'] = "B"
                else:
                    trace_set['ph'] = "E"
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    return


def parse_xpu64_trace(ts_interval, grab_id, trace, trace_set, event_file, grab_config_64, xpu_id, mile_stone):
    """parse_xpu64_trace"""
    grab_id_str = str(grab_id)
    config = grab_config_64['grab' + grab_id_str]
    xpu_id_str = str(xpu_id - 6)
    i_X = 'Cluster_' + xpu_id_str
    if config['high'] == 0 and config['low'] == 0:
        trace_set['name'] = "program"
        trace_set['cname'] = "rail_load"
        for i in range(0, 4):
            program_start = (trace >> (i + 8)) & 0b1
            program_done = (trace >> i) & 0b1
            core_id = i * 16
            core_id_str = str(core_id)
            trace_set['tid'] = "Core" + core_id_str

            if program_start == 1:
                trace_set['ph'] = "B"
            elif program_done == 1:
                trace_set['ph'] = "E"

            if program_start == 1 or program_done == 1:
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    elif config['high'] == 21 and config['low'] == 9:
        simd_busy[i_X]['l1'] = simd_busy[i_X]['l0']
        simd_busy[i_X]['l0'] = ((trace & 0b1111111111) != 0) | (((trace >> 10) & 0b1111111111) != 0)
        trace_set['name'] = "busy"
        trace_set['cname'] = "cq_build_attempt_failed"
        trace_set['tid'] = "Group0_SIMD"
        if simd_busy[i_X]['l1'] != simd_busy[i_X]['l0']:
            if simd_busy[i_X]['l0'] != 0:
                trace_set['ph'] = "B"
            else:
                trace_set['ph'] = "E"
            beat = json.dumps(trace_set)
            event_file.write(",\n")
            event_file.write(beat)
    elif config['high'] == 21 and config['low'] == 10:
        simd_busy[i_X]['h1'] = simd_busy[i_X]['h0']
        simd_busy[i_X]['h0'] = ((trace & 0b1111111111) != 0) | (((trace >> 10) & 0b1111111111) != 0)
        trace_set['name'] = "busy"
        trace_set['cname'] = "cq_build_attempt_failed"
        trace_set['tid'] = "Group0_SIMD"
        if simd_busy[i_X]['h1'] != simd_busy[i_X]['h0']:
            if simd_busy[i_X]['h0'] != 0:
                trace_set['ph'] = "B"
            else:
                trace_set['ph'] = "E"
            beat = json.dumps(trace_set)
            event_file.write(",\n")
            event_file.write(beat)
    elif config['high'] == 8 and config['low'] == 0:
        trace_set['name'] = "sync"
        trace_set['cname'] = "startup"
        trace_set['tid'] = "Group0_Sync"
        if (trace & 0b1) != 0:
            trace_set['ph'] = "B"
            beat = json.dumps(trace_set)
            event_file.write(",\n")
            event_file.write(beat)
    elif config['high'] == 9 and config['low'] == 0:
        trace_set['name'] = "sync"
        trace_set['cname'] = "startup"
        trace_set['tid'] = "Group0_Sync"
        if (trace & 0b1) != 0:
            trace_set['ph'] = "E"
            beat = json.dumps(trace_set)
            event_file.write(",\n")
            event_file.write(beat)
    elif config['high'] == 39 and config['low'] == 0:
        dma0_busy[i_X]['v1'] = dma0_busy[i_X]['v0']
        dma0_busy[i_X]['v0'] = (trace & 0b1) | ((trace >> 2) & 0b1) | ((trace >> 4) & 0b1) | ((trace >> 6) & 0b1)
        dma1_busy[i_X]['v1'] = dma1_busy[i_X]['v0']
        dma1_busy[i_X]['v0'] = (((trace >> 8) & 0b1) | ((trace >> 10) & 0b1)
                                | ((trace >> 12) & 0b1) | ((trace >> 14) & 0b1))
        trace_set['name'] = "busy"
        trace_set['cname'] = "vsync_highlight_color"
        if dma0_busy[i_X]['v1'] != dma0_busy[i_X]['v0']:
            trace_set['tid'] = "DMA0"
            if dma0_busy[i_X]['v0'] != 0:
                trace_set['ph'] = "B"
                if mile_stone >= dma0_busy[i_X]['ts']:
                    trace_set['ts'] = mile_stone / 1300.0
                    beat = json.dumps(trace_set)
                    event_file.write(",\n")
                    event_file.write(beat)
                    dma0_busy[i_X]['ts'] = mile_stone
                else:
                    dma0_busy[i_X]['v0'] = 0
            else:
                trace_set['ph'] = "E"
                if mile_stone == dma0_busy[i_X]['ts']:
                    trace_set['ts'] = (mile_stone + ts_interval + 1) / 1300.0
                    dma0_busy[i_X]['ts'] = mile_stone + ts_interval + 1
                else:
                    trace_set['ts'] = mile_stone / 1300.0
                    dma0_busy[i_X]['ts'] = mile_stone
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
        if dma1_busy[i_X]['v1'] != dma1_busy[i_X]['v0']:
            trace_set['tid'] = "DMA1"
            if dma1_busy[i_X]['v0'] != 0:
                trace_set['ph'] = "B"
                if mile_stone >= dma1_busy[i_X]['ts']:
                    trace_set['ts'] = mile_stone / 1300.0
                    beat = json.dumps(trace_set)
                    event_file.write(",\n")
                    event_file.write(beat)
                    dma1_busy[i_X]['ts'] = mile_stone
                else:
                    dma1_busy[i_X]['v0'] = 0
            else:
                trace_set['ph'] = "E"
                if mile_stone == dma1_busy[i_X]['ts']:
                    trace_set['ts'] = (mile_stone + ts_interval + 1) / 1300.0
                    dma1_busy[i_X]['ts'] = mile_stone + ts_interval + 1
                else:
                    trace_set['ts'] = mile_stone / 1300.0
                    dma1_busy[i_X]['ts'] = mile_stone
                beat = json.dumps(trace_set)
                event_file.write(",\n")
                event_file.write(beat)
    #if config['high'] < 8 and config['low'] == 0:
    #    trace_set['name'] = "program"
    #    trace_set['cname'] = "rail_load"
    #    for i in range(0, 8):
    #        program_start = (trace >> (i + 8)) & 0b1
    #        program_done = (trace >> i) & 0b1
    #        core_id = (config['high'] * 8) + i
    #        core_id_str = str(core_id)
    #        trace_set['tid'] = "Core" + core_id_str

    #        if program_start == 1:
    #            trace_set['ph'] = "B"
    #        elif program_done == 1:
    #            trace_set['ph'] = "E"

    #        if program_start == 1 or program_done == 1:
    #            beat = json.dumps(trace_set)
    #            event_file.write(",\n")
    #            event_file.write(beat)
    #elif config['high'] > 20 and config['high'] < 37:
    #    if config['low'] == 9 or config['low'] == 10:
    #        simd_busy[i_X]['grab' + grab_id_str]['b0'] = simd_busy[i_X]['grab' + grab_id_str]['a0']
    #        simd_busy[i_X]['grab' + grab_id_str]['a0'] = (trace >> 9) & 0b1
    #        simd_busy[i_X]['grab' + grab_id_str]['b1'] = simd_busy[i_X]['grab' + grab_id_str]['a1']
    #        simd_busy[i_X]['grab' + grab_id_str]['a1'] = (trace >> 19) & 0b1

    #        if simd_busy[i_X]['grab' + grab_id_str]['b0'] != simd_busy[i_X]['grab' + grab_id_str]['a0']:
    #            trace_set['name'] = "simd busy"
    #            trace_set['cname'] = "cq_build_attempt_failed"
    #            if config['low'] == 9:
    #                core_id = config['high'] - 21
    #            elif config['low'] == 10:
    #                core_id = config['high'] - 21 + 32
    #            core_id_str = str(core_id)
    #            trace_set['tid'] = "Core" + core_id_str

    #            if simd_busy[i_X]['grab' + grab_id_str]['b0'] == 0:
    #                trace_set['ph'] = "B"
    #            else:
    #                trace_set['ph'] = "E"

    #            beat = json.dumps(trace_set)
    #            event_file.write(",\n")
    #            event_file.write(beat)

    #        if simd_busy[i_X]['grab' + grab_id_str]['b1'] != simd_busy[i_X]['grab' + grab_id_str]['a1']:
    #            trace_set['name'] = "simd_busy"
    #            if config['low'] == 9:
    #                core_id = config['high'] - 21 + 16
    #            elif config['low'] == 10:
    #                core_id = config['high'] - 21 + 48
    #            core_id_str = str(core_id)
    #            trace_set['tid'] = "Core" + core_id_str

    #            if simd_busy[i_X]['grab' + grab_id_str]['b1'] == 0:
    #                trace_set['ph'] = "B"
    #            else:
    #                trace_set['ph'] = "E"

    #            beat = json.dumps(trace_set)
    #            event_file.write(",\n")
    #            event_file.write(beat)

    #    elif config['low'] > 14 or config['low'] < 19:
    #        pc_value = trace & 0b111111111111111111111
    #        pc_value_str = str(pc_value)
    #        #trace_set['name'] = "PC:" + pc_value_str
    #        trace_set['cname'] = "generic_work"
    #        if config['low'] == 15:
    #            core_id = config['high'] - 21
    #        elif config['low'] == 16:
    #            core_id = config['high'] - 21 + 16
    #        elif config['low'] == 17:
    #            core_id = config['high'] - 21 + 32
    #        else:
    #            core_id = config['high'] - 21 + 48
    #        core_id_str = str(core_id)
    #        trace_set['tid'] = "Core" + core_id_str

    #        pc_real_time[i_X]['grab' + grab_id_str]['s'] = pc_real_time[i_X]['grab' + grab_id_str]['e']
    #        pc_real_time[i_X]['grab' + grab_id_str]['e'] = pc_value

    #
    #        if pc_real_time[i_X]['grab' + grab_id_str]['e'] != -1:
    #            if pc_real_time[i_X]['grab' + grab_id_str]['s'] == -1:
    #                trace_set['name'] = "PC:0"
    #                trace_set['ts'] = 0
    #                trace_set['ph'] = "B"

    #                beat = json.dumps(trace_set)
    #                event_file.write(",\n")
    #                event_file.write(beat)

    #                trace_set['ts'] = mile_stone
    #                trace_set['ph'] = "E"

    #                beat = json.dumps(trace_set)
    #                event_file.write(",\n")
    #                event_file.write(beat)

    #                trace_set['name'] = "PC:" + str(pc_real_time[i_X]['grab' + grab_id_str]['e'])
    #                trace_set['ph'] = "B"

    #                beat = json.dumps(trace_set)
    #                event_file.write(",\n")
    #                event_file.write(beat)

    #            else:
    #                trace_set['name'] = "PC:" + str(pc_real_time[i_X]['grab' + grab_id_str]['s'])
    #                trace_set['ts'] = mile_stone
    #                trace_set['ph'] = "E"

    #                beat = json.dumps(trace_set)
    #                event_file.write(",\n")
    #                event_file.write(beat)

    #                trace_set['name'] = "PC:" + str(pc_real_time[i_X]['grab' + grab_id_str]['e'])
    #                trace_set['ph'] = "B"

    #                beat = json.dumps(trace_set)
    #                event_file.write(",\n")
    #                event_file.write(beat)

    return

def parse_trace(trace_file_name, config_file_name):
    """parse_trace"""
    global dma_8core_busy
    global dma_cmd_8core_busy
    #global l2_8core_busy
    global sfu_cmd_8core_busy
    global sdnn_cmd_busy
    global simd_busy
    global dma0_busy
    global dma1_busy
    if not os.path.isfile(trace_file_name):
        return
    event_file_name = os.path.splitext(trace_file_name)[0] + ".event"
    trace_file = open(trace_file_name, 'r')
    event_file = open(event_file_name, 'w')
    config_file = open(config_file_name, 'r')
    opt = 0
    # Write Header format
    event_file.write('{\n')
    event_file.write("\"displayTimeUnit\":\"ns\",\n")
    event_file.write("\"traceEvents\" : [\n")
    # Generated a virtual start event for each item
    for line in config_file:
        opt += 1
        option = re.split(r': ', line)
        option_name = option[0].strip()
        option_val = option[1].strip()
        if re.match(r"SSE", option_name) and str(option_val) == "OPEN":
            for i in sse_tid_list:
                event_file.write(json.dumps({'args': {'begin': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'B',
                                             'pid': 'SSE',
                                             'tid': i,
                                             'ts': 0}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': {'end': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'E',
                                             'pid': 'SSE',
                                             'tid': i,
                                             'ts': 0}))
                event_file.write(',\n')

        elif re.match(r"XPU_CLUSTER[0-9]*", option_name) and str(option_val) == "OPEN":
            #print("XPU_CLUSTER")
            for j in xpu64_tid_list:
                event_file.write(json.dumps({'args': {'begin': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'B',
                                             'pid': xpu64_pid_list[opt - 14],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': {'end': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'E',
                                             'pid': xpu64_pid_list[opt - 14],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')

        elif re.match(r"SDNN[0-9]*_CLUSTER", option_name) and str(option_val) == "OPEN":
            #print("SDNN*CLUSTER")
            for j in xpu8_tid_list:
                event_file.write(json.dumps({'args': {'begin': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'B',
                                             'pid': xpu8_pid_list[opt - 8],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': {'end': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'E',
                                             'pid': xpu8_pid_list[opt - 8],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')

        elif re.match(r"SDNN[0-9]*", option_name) and str(option_val) == "OPEN":
            #print("SDNN")
            for j in sdnn_tid_list:
                event_file.write(json.dumps({'args': {'begin': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'B',
                                             'pid': sdnn_pid_list[opt - 1],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': {'end': '3ns'},
                                             'cat': 'sync',
                                             'cname': 'grey',
                                             'id': 0,
                                             'name': 'sync',
                                             'ph': 'E',
                                             'pid': sdnn_pid_list[opt - 1],
                                             'tid': j,
                                             'ts': 0}))
                event_file.write(',\n')

    opt = 0

    event_file.write(json.dumps({'args': {'begin': '3ns'},
                                 'cat': 'sync',
                                 'cname': 'grey',
                                 'id': 0,
                                 'name': 'sync',
                                 'ph': 'B',
                                 'pid': 'Time_Stamp',
                                 'tid': 'Time_Stamp',
                                 'ts': 0}))
    event_file.write(',\n')
    event_file.write(json.dumps({'args': {'end': '3ns'},
                                 'cat': 'sync',
                                 'cname': 'grey',
                                 'id': 0,
                                 'name': 'sync',
                                 'ph': 'E',
                                 'pid': 'Time_Stamp',
                                 'tid': 'Time_Stamp',
                                 'ts': 0}))
    event_file.write(',\n')

    for i in lost_trace_tid_list:
        event_file.write(json.dumps({'args': {'begin': '3ns'},
                                     'cat': 'sync',
                                     'cname': 'grey',
                                     'id': 0,
                                     'name': 'sync',
                                     'ph': 'B',
                                     'pid': 'Lost_Trace',
                                     'tid': i,
                                     'ts': 0}))
        event_file.write(',\n')
        event_file.write(json.dumps({'args': {'end': '3ns'},
                                     'cat': 'sync',
                                     'cname': 'grey',
                                     'id': 0,
                                     'name': 'sync',
                                     'ph': 'E',
                                     'pid': 'Lost_Trace',
                                     'tid': i,
                                     'ts': 0}))
        event_file.write(',\n')
        event_file.write(json.dumps({'args': '',
                                     'cat': 'fsm',
                                     'cname': 'cq_build_attempt_failed',
                                     'id': 0,
                                     'name': '0',
                                     'ph': 'B',
                                     'pid': 'Lost_Trace',
                                     'tid': i,
                                     'ts': 0}))
        if i != 'Cluster':
            event_file.write(',\n')

    # Parse timestamp
    lines = trace_file.readlines()
    time_stamp_a = 0
    mile_stone = 0
    us = 0
    #first_write = 0
    sdnn_lost_num = 0
    sse_lost_num = 0
    xpu_lost_num = 0
    for line in lines:
        line_bin = int(line, 16)
        if ((line_bin >> 27) & 0b11111) == 28:
            if time_stamp_a == 0:
                time_stamp_a = line_bin
            else:
                time_stamp_b = time_stamp_a
                time_stamp_a = line_bin
                time_value_b = time_stamp_b & 0b111111111111111111111111111
                time_value_a = time_stamp_a & 0b111111111111111111111111111
                if time_value_b > time_value_a:
                    time_value_b = time_value_b | 0b1000000000000000000000000000
                mile_stone += (time_value_a - time_value_b)
                us = mile_stone / 1300.0
                event_file.write(",\n")
                event_file.write(json.dumps({'args': '',
                                             'cat': 'issue_time_stamp',
                                             'cname': 'olive',
                                             'id': 0,
                                             'name': 'time_stamp_instr',
                                             'ph': 'I',
                                             'pid': 'Time_Stamp',
                                             'tid': 'Time_Stamp',
                                             'ts': us}))
        elif ((line_bin >> 29) & 0b111) <= 5:
            if ((line_bin >> 25) & 0b1111) <= 7:
                parse_sdnn_trace(line_bin, mile_stone, event_file)
        elif ((line_bin >> 28) & 0b1111) == 12:
            parse_sse_trace(line_bin, mile_stone, event_file)
        elif ((line_bin >> 28) & 0b1111) == 13:
            parse_xpu_trace(ts_interval, line_bin, mile_stone, event_file, grab_config_64, grab_config_8)
        elif ((line_bin >> 27) & 0b11111) == 29:
            sdnn_lost_add = line_bin & 0b11111111
            sse_lost_add = (line_bin >> 8) & 0b11111111
            xpu_lost_add = (line_bin >> 16) & 0b11111111
            if sdnn_lost_add != 0:
                sdnn_lost_num += sdnn_lost_add
                sdnn_lost_num_str = str(sdnn_lost_num)
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': sdnn_lost_num_str,
                                             'ph': 'E',
                                             'pid': 'Lost_Trace',
                                             'tid': 'SDNN',
                                             'ts': us}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': sdnn_lost_num_str,
                                             'ph': 'B',
                                             'pid': 'Lost_Trace',
                                             'tid': 'SDNN',
                                             'ts': us}))

            if sse_lost_add != 0:
                sse_lost_num += sse_lost_add
                sse_lost_num_str = str(sse_lost_num)
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': sse_lost_num_str,
                                             'ph': 'E',
                                             'pid': 'Lost_Trace',
                                             'tid': 'SSE',
                                             'ts': us}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': sse_lost_num_str,
                                             'ph': 'B',
                                             'pid': 'Lost_Trace',
                                             'tid': 'SSE',
                                             'ts': us}))
            if xpu_lost_add != 0:
                xpu_lost_num += xpu_lost_add
                xpu_lost_num_str = str(xpu_lost_num)
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': xpu_lost_num_str,
                                             'ph': 'E',
                                             'pid': 'Lost_Trace',
                                             'tid': 'XPU',
                                             'ts': us}))
                event_file.write(',\n')
                event_file.write(json.dumps({'args': '',
                                             'cat': 'fsm',
                                             'cname': 'cq_build_attempt_failed',
                                             'id': 0,
                                             'name': xpu_lost_num_str,
                                             'ph': 'B',
                                             'pid': 'Lost_Trace',
                                             'tid': 'XPU',
                                             'ts': us}))

    #for i in pc_real_time:
    #    for j in pc_real_time[i]:
    #        if pc_real_time[i][j]['e'] != -1:
    #            core_id = grab_config_64[j]['high'] - 21 + (grab_config_64[j]['low'] - 15) * 16
    #            core_id_str = str(core_id)
    #            pc_value_str = str(pc_real_time[i][j]['e'])
    #            event_file.write(',\n')
    #            event_file.write(json.dumps({'args': '',
    #                                         'cat': 'fsm',
    #                                         'cname': 'generic_work',
    #                                         'id': 0,
    #                                         'name': 'PC:' + pc_value_str,
    #                                         'ph': 'E',
    #                                         'pid': i,
    #                                         'tid': 'Core' + core_id_str,
    #                                         'ts': mile_stone}))

    sdnn_lost_num_str = str(sdnn_lost_num)
    event_file.write(',\n')
    event_file.write(json.dumps({'args': '',
                                 'cat': 'fsm',
                                 'cname': 'cq_build_attempt_failed',
                                 'id': 0,
                                 'name': sdnn_lost_num_str,
                                 'ph': 'E',
                                 'pid': 'Lost_Trace',
                                 'tid': 'SDNN',
                                 'ts': us}))
    sse_lost_num_str = str(sse_lost_num)
    event_file.write(',\n')
    event_file.write(json.dumps({'args': '',
                                 'cat': 'fsm',
                                 'cname': 'cq_build_attempt_failed',
                                 'id': 0,
                                 'name': sse_lost_num_str,
                                 'ph': 'E',
                                 'pid': 'Lost_Trace',
                                 'tid': 'SSE',
                                 'ts': us}))
    xpu_lost_num_str = str(xpu_lost_num)
    event_file.write(',\n')
    event_file.write(json.dumps({'args': '',
                                 'cat': 'fsm',
                                 'cname': 'cq_build_attempt_failed',
                                 'id': 0,
                                 'name': xpu_lost_num_str,
                                 'ph': 'E',
                                 'pid': 'Lost_Trace',
                                 'tid': 'XPU',
                                 'ts': us}))

    event_file.write('\n')
    event_file.write(']\n')
    event_file.write('}\n')
    event_file.close()
    trace_file.close()
    config_file.close()
    return

def check_lostcnt(lostcnt_file_name):
    """ check lost count of tracer """
    if not os.path.isfile(lostcnt_file_name):
        print("lostcnt_file {} is not exist".format(lostcnt_file_name))
        return -1
    elif os.path.splitext(lostcnt_file_name)[1] != ".lost_cnt":
        print("lostcnt_file {} has a wrong filetype".format(lostcnt_file_name))
        return -1

    lostcnt_file = open(lostcnt_file_name, 'r')
    state = ""

    for line in lostcnt_file:
        if re.match(r"XPU CLUSTER.*", line):
            state = "Cluster"
            match_res = re.match(r"XPU CLUSTER([0-9]*)", line)
            cluster_idx = int(match_res.group(1), 10);
            grab_idx = 0
            continue
        elif re.match(r"SDNN[0-9]*:", line):
            state = "SDNN"
            match_res = re.match(r"SDNN([0-9]*):", line)
            sdnn_idx = int(match_res.group(1), 10)
            coprocessor_idx = 0
            continue
        elif re.match(r"SDNN[0-9]* CLUSTER", line):
            state = "SDNN_Cluster"
            match_res = re.match(r"SDNN([0-9]*) CLUSTER", line)
            sdnn_cluster_idx = int(match_res.group(1), 10)
            grab_idx = 0
            continue
        elif re.match(r"DMA\(DebugMaster\).*", line):
            state = "DebugMaster"
            continue
        if state == "Cluster":
            if grab_config_64['grab' + str(grab_idx)]['mask'] == 0:
                option = re.split(r'=', line)
                cluster_grabx_lostcnt = int(option[1].strip(), 16)
                if cluster_grabx_lostcnt != 0:
                    print("Cluster{} grab{} lostcnt {}".format(cluster_idx, grab_idx, cluster_grabx_lostcnt))
            grab_idx += 1
        elif state == "SDNN":
            option = re.split(r'=', line)
            coprocessor_lostcnt = int(option[1].strip(), 16)
            if coprocessor_lostcnt != 0:
                print("SDNN{} {} lostcnt {}".format(sdnn_idx,
                                                    sdnn_coprocessor_list[coprocessor_idx],
                                                    coprocessor_lostcnt))
            coprocessor_idx += 1
        elif state == "SDNN_Cluster":
            if grab_config_8['grab' + str(grab_idx)]['mask'] == 0:
                option = re.split(r'=', line)
                cluster_grabx_lostcnt = int(option[1].strip(), 16)
                if cluster_grabx_lostcnt != 0:
                    print("SDNN_Cluster{} grab{} lostcnt {}".format(sdnn_cluster_idx, grab_idx, cluster_grabx_lostcnt))
            grab_idx += 1
        elif state == "DebugMaster":
            option = re.split(r'=', line)
            dma_lost_addr = option[0].strip()
            dma_lost_stat = option[1].strip()
            if int(dma_lost_stat, 16) != 0:
                print("DebugMaster(DMA) is not work well,there are lostcnt")
    lostcnt_file.close()
    return 0

def parse_config_file(config_file_name):
    """ parse config_file  """
    global ts_interval
    if not os.path.isfile(config_file_name):
        print("config_file {} is not exist".format(config_file_name))
        return -1
    elif os.path.splitext(config_file_name)[1] != ".config":
        print("config_file {} has a wrong filetype".format(config_file_name))
        return -1
    config_file = open(config_file_name, 'r')

    for line in config_file:
        option = re.split(r': ', line)
        option_name = option[0].strip()
        option_val = option[1].strip()
        if re.match(r"SSE", option_name) and str(option_val) == "OPEN":
            sse_enable = 'enable'
        elif re.match(r"XPU_CLUSTER[0-9]*", option_name) and str(option_val) == "OPEN":
             match_res = re.match(r"XPU_CLUSTER([0-9]*)", option_name)
             cluster_enable_list[int(match_res.group(1), 10)] = 'enable'
        elif re.match(r"SDNN[0-9]*_CLUSTER", option_name) and str(option_val) == "OPEN":
             match_res = re.match(r"SDNN([0-9]*)_CLUSTER", option_name)
             sdnn_cluster_enable_list[int(match_res.group(1), 10)] = 'enable'
        elif re.match(r"SDNN[0-9]*", option_name) and str(option_val) == "OPEN":
             match_res = re.match(r"SDNN([0-9]*)", option_name)
             sdnn_enable_list[int(match_res.group(1), 10)] = 'enable'
        elif re.match(r"XRE_TRACER.*", option_name):
            parse_grab_config(option_name, option_val)
        elif re.match(r"TS_INTERVAL.*", option_name):
            ts_interval = int(option_val, 10)
    config_file.close
    return 0

usage = "Usage:\n\t trace_vision_tools.py <trace_file> <config_file> <lostcnt_file>"

if __name__ == "__main__":
    if len(sys.argv) <= 3:
        print(usage)
        exit(1)
    else:
        file_name = sys.argv[1]
        config_file_name = sys.argv[2]
        lostcnt_file_name = sys.argv[3]

    if parse_config_file(config_file_name):
        print(usage)
        exit(1)
    if check_lostcnt(lostcnt_file_name):
        print(usage)
        exit(1)

    if parse_trace(file_name, config_file_name):
        print(usage)
        exit(1)

    #print all_sdnn_coprocessor_total_time[]
    for sdnn_idx in range(6):
        if sdnn_enable_list[sdnn_idx] == "enable":
            print("SDNN{}:".format(sdnn_idx))
            for item in ["DMAI_0", "DMAI_1", "DS_0", "DS_1", "MAC", "EW", "RS", "DMAO"]:
                print("{}, ".format(item))
            print("")
            for item in ["DMAI_0", "DMAI_1", "DS_0", "DS_1", "MAC", "EW", "RS", "DMAO"]:
                total_time = all_sdnn_coprocessor_total_time["SDNN" + str(sdnn_idx)][item]
                total_time = int(total_time * 1e3)
                print("{}, ".format(total_time))
            print("")
