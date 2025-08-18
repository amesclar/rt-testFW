# python script to validate regatta timer test run
#
# assumes xml data file format similar to sampleTestResultsFile.xml
#
# usage: clear; python3 validateTestRun.py
#
# reference - buzzer activation array from Arduino
# const int buzzerOnLongMillis = 400;
# const int buzzerOnShortMillis = 200;
#
# // elapsed sec, longCount, shortCount
# // 1 minute
# unsigned long timer1Array[10][3] = {
#   { 0, 1, 0 }, { 30, 0, 3 }, { 40, 0, 2 }, { 50, 0, 1 }, { 55, 0, 1 }, { 56, 0, 1 }, { 57, 0, 1 }, { 58, 0, 1 }, { 59, 0, 1 }, { 60, 1, 0 }
# };
# // 2 minute
# unsigned long timer2Array[12][3] = {
#   { 0, 2, 0 }, { 30, 1, 3 }, { 60, 1, 0 }, { 90, 0, 3 }, { 100, 0, 2 }, { 110, 0, 1 }, { 115, 0, 1 }, { 116, 0, 1 }, { 117, 0, 1 }, { 118, 0, 1 }, { 119, 0, 1 }, { 120, 1, 0 }
# };
# // 3 minute
# unsigned long timer3Array[13][3] = {
#   { 0, 3, 0 }, { 60, 2, 0 }, { 90, 1, 3 }, { 120, 1, 0 }, { 150, 0, 3 }, { 160, 0, 2 }, { 170, 0, 1 }, { 175, 0, 1 }, { 176, 0, 1 }, { 177, 0, 1 }, { 178, 0, 1 }, { 179, 0, 1 }, { 180, 1, 0 }
# };
# // 5 minute
# // 0-class flag up, 1-P flag up, 4-P flag down, 5-class flag down
# unsigned long timer5Array[4][3] = {
#   { 0 * 60, 1, 0 }, { 1 * 60, 1, 0 }, { 4 * 60, 1, 0 }, { 5 * 60, 1, 0 }
# };
#

import xml.etree.ElementTree as ET
import pandas as pd
# import matplotlib.pyplot as plt

def load_buzzer_list(xml_dom):
    buzzer_list = []
    for buzzer in xml_dom:
        # print(buzzer.text)
        buzzer_list.append(buzzer.text)
    # print(buzzer_list)
    return(buzzer_list)

def log_df(msg, df):
    print(msg)
    print(df)
#
# *** main starts here ***
#

test_data_xml = 'test_result_samples/one_min.txt'
tree = ET.parse(test_data_xml)
root = tree.getroot()
# print(root.tag)

tests = {}
one_min_buzzer = []
two_min_buzzer = []
three_min_buzzer = []
five_min_buzzer = []

for test in root:
    # print(test.tag)

    # case statement has issues
    if (test.tag == 'OneMinute'):
        one_min_buzzer.append(load_buzzer_list(test))
    elif (test.tag == 'TwoMinute'):
        two_min_buzzer.append(load_buzzer_list(test))
    elif (test.tag == 'ThreeMinute'):
        three_min_buzzer.append(load_buzzer_list(test))
    elif (test.tag == 'FiveMinute'):
        five_min_buzzer.append(load_buzzer_list(test))

df_1min = pd.DataFrame(one_min_buzzer).astype(int)
df_2min = pd.DataFrame(two_min_buzzer).astype(int)
df_3min = pd.DataFrame(three_min_buzzer).astype(int)
df_5min = pd.DataFrame(five_min_buzzer).astype(int)

df_1min_diff = df_1min.diff(axis=1)
df_2min_diff = df_2min.diff(axis=1)
df_3min_diff = df_3min.diff(axis=1)
df_5min_diff = df_5min.diff(axis=1)

# log_df('one_min_buzzer',one_min_buzzer)
# log_df('df_1min',df_1min)
log_df('df_1min_diff',df_1min_diff)
# df_1min_diff_plot = df_1min_diff.drop(df_1min_diff.columns[0], axis=1)
# log_df('df_1min_diff_plot',df_1min_diff_plot)
# print(df_1min_diff_plot.columns)
# df_1min_diff_plot.columns = ['p1','p2','p3','p4','p5','p6','p7','p8','p9','p10','p11','p12']
# log_df('df_1min_diff_plot',df_1min_diff_plot)
# df_1min_diff_plot.plot(use_index = True)

# log_df('two_min_buzzer',two_min_buzzer)
# log_df('df_2min',df_2min)
log_df('df_2min_diff',df_2min_diff)

# log_df('three_min_buzzer',three_min_buzzer)
# log_df('df_3min',df_3min)
log_df('df_3min_diff',df_3min_diff)

# log_df('five_min_buzzer',five_min_buzzer)
# log_df('df_5min',df_5min)
log_df('df_5min_diff',df_5min_diff)
