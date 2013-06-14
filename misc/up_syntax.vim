" Vim syntax file

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif


syn sync lines=250

syn keyword upConstant		true false nil
syn keyword upConditional	if else
syn keyword upStatement		return
syn keyword upRepeat		while repeat until for
syn keyword upStruct		class
syn keyword upStorage		forward builtin
syn keyword upType			float vector string entity function void 

syn keyword upTodo contained	TODO FIXME DEBUG NOTE WISH OPTIMIZE OPTIMISE


" strings
syn match	upSpecial contained "\\[\\abfnrtv\'\"]\|\\\d\{,3}"

syn region	upString  start=+"+ end=+"+ skip=+\\\\\|\\"+ contains=upSpecial


" syn match   upIdentifier		"\<[a-zA-Z_][a-zA-Z0-9_]*\>"

syn match	upDefine		"\<[a-zA-Z_][a-zA-Z0-9_]*\>\s*\(:=\)\@="

syn region  upFrame         start="[$]frame" end="$" keepend


syn match	upNumber		"-\=\<\d\+\>"
syn match	upFloat			"-\=\<\d\+\.\d\+\>"
syn match 	upFloat			"-\=\<\d\+\.\d\+[eE]-\=\d\+\>"
syn match	upHexNumber		"0[xX][0-9a-fA-F]\+\>"

syn region	upComment	start="//" skip="\\$" end="$" keepend contains=upTodo
syn region	upComment	start="/\*" end="\*/" contains=upTodo extend

syn match	upState		"\(=\)\@<=\s*\[[^]]*,[^]]*\]"


if !exists("ec_no_functions")
  " entity functions
  syn keyword upFunction	spawn remove

  " math functions
  syn keyword upFunction	random
endif


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_ec_syn_inits")
  if version < 508
    let did_ec_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink upBoolean		Boolean
  HiLink upComment		Comment
  HiLink upConditional	Conditional
  HiLink upConstant		Constant
  HiLink upDefine		Special
  HiLink upFrame		Special
  HiLink upFunction		Function
  HiLink upLabel		Label
  HiLink upNumber		Number
  HiLink upFloat		Float
  HiLink upRepeat		Repeat
  HiLink upStatement	Statement
  HiLink upStorage		StorageClass
  HiLink upString		String
  HiLink upStruct		upStatement
  HiLink upTodo			Todo
  HiLink upType			Type
  HiLink upSpecial		SpecialChar
  HiLink upState		Special
  HiLink upError		Error

  delcommand HiLink
endif


let b:current_syntax = "up"

" vim: ts=4 sw=4 noexpandtab
