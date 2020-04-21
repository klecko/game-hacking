#! /usr/bin/env python

""" METIN2 COLOR FORMATTER
	A simple class which convert the values into a string by a specific color name.

	= Python Color Constants Module:
		https://www.webucator.com/blog/2015/03/python-color-constants-module/
		https://www.color-hex.com/color/ccffff

	= Ex-How-To-Use:
		text = CFF.format('Metin2', 'green')
		text = CFF.format(8000, 'banana')
		text = CFF.format(412.55, 'red')

		text = CFF.format(('a', 'b', 'c'), 'red')
			* return a tuple list | text[0], text[1], text[2]

		text = CFF.format('Item name:', 'springgreen', CFF.FLAG_NEW_TAB) + CFF.format(item.GetItemName(), 'chocolate')
"""


class CFF:
	FLAG_NEW_LINE = 0
	FLAG_NEW_TAB = 1
	FLAG_NEW_SPACE = 2
	FLAG_EMPTY_STRING = 3

	PATTERN_BEGIN = '|cff'
	PATTERN_END = '|h|r'

	# Flags
	FLAG_DICT = {
		FLAG_NEW_LINE: '\n',
		FLAG_NEW_TAB: '\t',
		FLAG_NEW_SPACE: ' ',
		FLAG_EMPTY_STRING: ''
	}

	# Colors
	COLOR_HEX_DICT = {
		'green': '#15FF00',
		'blue': '#00C5FF',
		'banana': '#E3CF57',
		'chocolate': '#D2691E',
		'darkred': '#8B0000',
		'gray': '#808080',
		'red': '#FF0000',
		'skyblue': '#87CEEB',
		'slateblue': '#6A5ACD',
		'snow': '#FFFAFA',
		'springgreen': '#00FF7F',
		'steelblue': '#4682B4',
		'white': '#FFFFFF',
		'yellow': '#FFFF00',
	}

	@staticmethod
	def format(token, color_name, add_flag=FLAG_EMPTY_STRING):
		def get_format(color, src):
			return '{0}{1}{2}{3}{4}'.format(CFF.PATTERN_BEGIN, color, src, CFF.PATTERN_END, CFF.FLAG_DICT[add_flag])

		def get_color(color):
			# if color starts with #xxyyzz then return it
			if color and color[0] == '#':
				return color[1:]
			# otherwise grab it from the color dict, return 'white' if missing
			return CFF.COLOR_HEX_DICT.get(color_name.lower(), '#FFFFFF')[1:]

		color_hex = get_color(color_name)

		return get_format(color_hex, token)

	@staticmethod
	def multi_format(tokens, color_name, add_flag=FLAG_EMPTY_STRING):
		res = []
		for t in tokens:
			res.append(CFF.format(t, color_name, add_flag))
		return res


def test():
	print CFF.format('Metin2', 'green')
	print CFF.format(8000, 'banana')
	print CFF.format(412.55, 'red')
	print CFF.format(412.55, '#113355')

	print CFF.multi_format(('Ananas', 'Banana', 'Cachi'), 'red')
	# print CFF.format('Item name:', 'springgreen', CFF.FLAG_NEW_TAB) + CFF.format(item.GetItemName(), 'chocolate')


if __name__ == "__main__":
	test()
