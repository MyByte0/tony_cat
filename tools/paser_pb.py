#!/usr/bin/python
# -*- coding:utf-8 -*-

import sys

class FieldMeta:
	def __init__(self):
		self.name = ''
		self.type = ''
		self.tag = ''
		self.opt = ''
		self.sub = []
		self.comment = []

class PbMeta:
	def __init__(self):
		self.name = ''
		self.pb_type = ''
		self.namespace = ''
		self.fields = []
		self.comment = []

	def GetPbMetaFieldByName(self, name):
		for field in self.fields:
			if field.name == name:
				return field
		return

def SplitLine(line):
	line_data_define = ''
	line_comment = ''
	# devide comments first
	if line.find('//') != -1 :
		line_data_define = line[:line.find('//')]
		line_comment = line[line.find('//')+len('//'):]
	else:
		line_data_define = line

	# splite by '=', remove char ';'
	line_words = []
	for word in line_data_define.strip().split(" "):
		word.strip()
		if word.find('=') != -1:
			wordFront = word[0 : word.find('=')]
			if wordFront != "":
				line_words.append(wordFront)
			wordBehind = word[word.find('=') + len('=') : ]
			if wordBehind != "":
				line_words.append(wordBehind)
		if word.find(';') != -1:
			word = word[0 : word.find(';')]
		if word != "":
			line_words.append(word)

	# add '//' and comments
	if line_comment != '':
		line_words.append('//')
		for word in line_comment.strip().split(" "):
			word.strip()
			if word != "":
				line_words.append(word)

	return line_words

def LoadFiledLine(pb_version, pb_type, line):
	cur_field_meta = FieldMeta()
	
	# handle format map<,> example: map<int,string> data = 1;
	if line.find('>') != -1 :
		cur_field_meta.type = line[:line.find('>') + len('>')]
		line = line[line.find('>') + len('>') : ]
		line_words = SplitLine(line)
		cur_field_meta.opt = 'optional'
		cur_field_meta.name = line_words[0]
		cur_field_meta.tag = line_words[1]
		if len(line_words) > 3:
			cur_field_meta.comment = line_words[3:]
		return cur_field_meta

	# for one line words
	line_words = SplitLine(line)

	if pb_type == 'message':
		if pb_version == '"proto2"' and len(line_words) > 4:
			cur_field_meta.opt = line_words[0]
			cur_field_meta.type = line_words[1]
			cur_field_meta.name = line_words[2]
			cur_field_meta.tag = line_words[3]
			if len(line_words) > 5:
				cur_field_meta.comment = line_words[5:]
		if pb_version == '"proto3"' and len(line_words) > 2:
			offset_opt = 0
			cur_field_meta.opt = 'optional'
			if line_words[0] == 'repeated':
				cur_field_meta.opt = line_words[0]
				offset_opt = 1

			cur_field_meta.type = line_words[0+offset_opt]
			cur_field_meta.name = line_words[1+offset_opt]
			cur_field_meta.tag = line_words[2+offset_opt]
			if len(line_words) > (4+offset_opt):
				cur_field_meta.comment = line_words[4+offset_opt:]

	if pb_type == 'enum' and len(line_words) > 2:
		cur_field_meta.name = line_words[0]
		cur_field_meta.type = line_words[2]

	return cur_field_meta

def LoadPbFile(file_path):
	with open(file_path,"r") as load_pb_file:
		load_pb_file_lines = load_pb_file.readlines()

	pb_namespace = ""
	pb_version = ""
	pb_metas = []
	cur_braces_count = 0
	cur_pb_meta = PbMeta()

	msg_name = ''
	msg_type = ''
	msg_comments = []

	# for lines
	for line in load_pb_file_lines:
		line_words = SplitLine(line)
		if len(line_words) == 0:
			continue
		if line_words[0] == 'import':
			continue
		if line_words[0] == 'package':
			pb_namespace = line_words[1]
			continue

		# read pb version syntax, example syntax = "proto3"
		if line_words[0] == 'syntax':
			if len(line_words) > 2 :
				pb_version = line_words[2] 
			else:
				print("read syntax error on %s!" % file_path)
				continue

		if cur_braces_count == 0:
			## read '//' comments
			if line.find('//') != -1:
				msg_comments = line_words
			# handle char "{" and start a message
			elif line.find('{') != -1:
				if cur_braces_count == 0:
					cur_pb_meta.name = msg_name
					cur_pb_meta.pb_type = msg_type
					cur_pb_meta.comment = msg_comments
					cur_pb_meta.namespace = pb_namespace
				msg_comments = []
				cur_braces_count = cur_braces_count + 1
			# read message/enum define
			elif len(line_words) > 1:
				msg_type = line_words[0]
				msg_name = line_words[1]
		elif cur_braces_count == 1:
		# handle char "}" and end a message
			if line.find('}') != -1:
				cur_braces_count = cur_braces_count - 1
				if len(cur_pb_meta.name) > 0:
					pb_metas.append(cur_pb_meta)
					cur_pb_meta = PbMeta()
			else:
				cur_field_meta = LoadFiledLine(pb_version, cur_pb_meta.pb_type, line)
				cur_pb_meta.fields.append(cur_field_meta)
		elif cur_braces_count > 1:
		# not paser sub struct
			continue

	return pb_metas

if __name__ == "__main__":
	if len(sys.argv) > 1:
		LoadPbFile(sys.argv[1])