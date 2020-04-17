#!/usr/bin/env python
# coding=utf-8
import re
import class_labels
import struct

current_grams = 0
all_records = []

fp = open('lm.arpa','r')

#size_of_vocabulary_code = len(class_lables) / 256 + 1

vocabulary_map = dict(zip(class_labels.vocabulary,range(1, len(class_labels.vocabulary) + 1)))
pad_str = [255]

while True:
	line = fp.readline()
	if not line:
		break
	line = line.strip()
	if not line:
		continue
	if line == '\\end\\':
		break
	match_obj = re.search(r'\\(\d+)-grams:',line)
	if match_obj:
		current_grams = int(match_obj.group(1))
		while len(all_records) < current_grams:
			all_records.append([])
	else:
		if current_grams != 0:
			one_record = line.split()
			pro = 10**float(one_record[0])
			mystr = one_record[1:current_grams+1]
			######
			mystr = [vocabulary_map[x] for x in mystr]
			pad = -len(mystr) % 4
			mystr += pad * pad_str
			######
			if len(one_record) == current_grams + 2:
				back_pro = 10**float(one_record[-1])
			else:
				back_pro = 1.0
			one_record_list = [pro, back_pro, mystr]
			all_records[current_grams-1].append(one_record_list)
#print(all_records)
fp.close()
mybyte_array = b''
mybyte_array += struct.pack('i', len(all_records))
for i in range(len(all_records)):
	mybyte_array += struct.pack('i', len(all_records[i]))

for i in range(len(all_records)):
    for c in all_records[i]:
	for aaa in c:
            if isinstance(aaa, float):
		mybyte_array += struct.pack('f', aaa)
            if isinstance(aaa, list):
		for bbb in aaa:
		    if isinstance(bbb, int):
			mybyte_array += struct.pack('B', bbb)
fp = open('lm-4bit-align.bin','wb')
fp.write(mybyte_array)
